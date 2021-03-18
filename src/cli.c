/**
 * Copyright 2018 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#include <curl/curl.h>
#include <wrp-c.h>

#include "schedule.h"
#include "process_data.h"
#include "decode.h"
#include "aker_mem.h"
#include "time.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
struct memory {
    uint8_t *buffer;
    size_t len;
};

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
uint8_t* get_from_url( const char *url, const char *auth, const char *cpe, size_t *len, bool verbose );
static int process( uint8_t *data, size_t len, int start, int end );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
size_t get_max_mac_limit(void)
{
    return 128;
}

/* Main function */
int main( int argc, char **argv)
{
    const char *option_string = "u:c:f:s:e:h::";
    static const struct option options[] = {
        { "help",    optional_argument, 0, 'h' },
        { "file",    optional_argument, 0, 'f' },
        { "url",     optional_argument, 0, 'u' },
        { "cpe",     optional_argument, 0, 'c' },
        { "verbose", optional_argument, 0, 'v' },
        { "start",   optional_argument, 0, 's' },
        { "end",     optional_argument, 0, 'e' },
        { 0, 0, 0, 0 }
    };
    const char *filename = NULL;
    const char *url = NULL;
    const char *auth = NULL;
    const char *cpe = NULL;
    int start = 0;
    int end = 0;
    int i = 0;
    int item = 0;
    uint8_t *data;
    size_t len;
    bool verbose = false;

    extern int cimplog_debug_level;

    cimplog_debug_level = -1;

    while( -1 != (item = getopt_long(argc, argv, option_string, options, &i)) ) {
        switch( item ) {
            case 'f':
                filename = optarg;
                break;
            case 'u':
                url = optarg;
                break;
            case 'c':
                cpe = optarg;
                break;
            case 's':
                start = atoi(optarg);
                break;
            case 'e':
                end = atoi(optarg);
                break;
            case 'v':
                verbose = true;
                break;

            default:
                fprintf( stderr, "Usage:\naker-cli [-u xmidt url -c cpe [-v]] [-f filename] [-s starting_unixtime] [-e ending_unixtime]\n\n" );
                fprintf( stderr, "    Outputs the schedule according to aker and how it interprets a schedule\n" );
                fprintf( stderr, "    over a window of time.\n\n" );
                fprintf( stderr, "    The schedule can be acquired via a local file or via the xmidt api url specified.\n" );
                fprintf( stderr, "    If the url is used the following header 'Authorization: ${XMIDT_AUTH}' is used\n" );
                fprintf( stderr, "    for authorization.  The XMIDT_AUTH is an env variable.  For JWT based authentication\n" );
                fprintf( stderr, "    the variable should be in the form of XMIDT_AUTH=\"Bearer asdlfkasdlfkasdf...\"\n" );
                fprintf( stderr, "    Additionally the cpe should be in the form of a device-identifier like this: mac:112233445566\n\n" );
                fprintf( stderr, "    The -v option enables verbose curl to help debug usage mistakes.\n\n" );
                fprintf( stderr, "    If ending_unixtime is 0, process the entire current week.\n" );
                return -1;
        }
    }

    if( NULL == filename && (NULL == cpe || NULL == url) ) {
        fprintf( stderr, "Filename or (URL & cpe) is missing.  One group must be specified.\n" );
        return -2;
    }

    data = NULL;
    len = 0;
    if( NULL != url ) {
        auth = getenv( "XMIDT_AUTH" );
        data = get_from_url( url, auth, cpe, &len, verbose );
    } else {
        len = read_file_from_disk( filename, &data );
    }

    return process( data, len, start, end );
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
void get_uuid( char *dst )
{
    unsigned int a, b, c, d, e, f;

    a = (unsigned int) rand();

    b = (unsigned int) rand();
    c = 0xffff & b;
    b = 0xffff & (b >> 16);

    d = (unsigned int) rand();
    e = 0xffff & d;
    d = 0xffff & (d >> 16);

    f = (unsigned int) rand();
    sprintf( dst, "%08x-%04x-%04x-%04x-%04x%08x", a, b, c, d, e, f );
}

static size_t curl_callback( void *contents, size_t size, size_t nmemb, void *user )
{
    struct memory *mem = (struct memory*) user;
    size_t len = size * nmemb;
    uint8_t *p;

    p = realloc( mem->buffer, mem->len + len );
    if( NULL == p ) {
        /* OOM */
        return 0;
    }

    mem->buffer = p;
    memcpy( &(mem->buffer[mem->len]), contents, len );
    mem->len += len;

    return len;
}

uint8_t* get_from_url( const char *url, const char *auth, const char *cpe, size_t *len, bool verbose )
{
    const char *authentication_fmt = "Authorization: %s";
    uint8_t *data = NULL;

    CURL *curl;

    curl = curl_easy_init();
    if( NULL != curl ) {
        struct curl_slist *headers = NULL;
        char *auth_header = NULL;
        int auth_header_len;
        wrp_msg_t req;
        char dest[1024];
        char uuid[1024];
        void *out;
        ssize_t out_len;
        struct memory mem;

        /* Deal with the auth header */
        auth_header_len = snprintf( NULL, 0, authentication_fmt, auth ) + 1;
        auth_header = (char*) malloc( sizeof(char) * auth_header_len );
        if( NULL == auth_header ) {
            return NULL;
        }
        sprintf( auth_header, authentication_fmt, auth );
        headers = curl_slist_append( headers, auth_header );
        headers = curl_slist_append( headers, "Content-Type: application/msgpack" );

        /* Make the wrp to send */
        memset( &req, 0, sizeof(req) );
        req.msg_type = WRP_MSG_TYPE__RETREIVE;
        req.u.crud.content_type = "text/plain";
        req.u.crud.source = "dns:aker-cli.example.com";
        snprintf( dest, sizeof(dest), "%s/aker/schedule", cpe );
        req.u.crud.dest = dest;
        get_uuid( uuid );
        req.u.crud.transaction_uuid = uuid;

        out_len = wrp_struct_to( &req, WRP_BYTES, &out );
        if( out_len < 1 ) {
            curl_easy_cleanup( curl );
            free( auth_header );
            return NULL;
        }

        memset( &mem, 0, sizeof(mem) );
        curl_easy_setopt( curl, CURLOPT_POSTFIELDSIZE, (long) out_len );
        curl_easy_setopt( curl, CURLOPT_POSTFIELDS, (char*) out );
        curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, curl_callback );
        curl_easy_setopt( curl, CURLOPT_WRITEDATA, (void*) &mem );
        if( true == verbose ) {
            curl_easy_setopt( curl, CURLOPT_VERBOSE, 1L );
        }

        curl_easy_setopt( curl, CURLOPT_URL, url );
        curl_easy_setopt( curl, CURLOPT_HTTPHEADER, headers );

        if( CURLE_OK == curl_easy_perform(curl) ) {
            long response_code;
            curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &response_code );
            if( 200 == response_code ) {
                wrp_msg_t *resp;
                ssize_t rv;

                rv = wrp_to_struct( mem.buffer, mem.len, WRP_BYTES, &resp );
                if( (0 < rv) && (WRP_MSG_TYPE__RETREIVE == resp->msg_type) ) {
                    data = (uint8_t*) malloc( sizeof(uint8_t) * rv );
                    if( NULL != data ) {
                        
                        *len = resp->u.crud.payload_size;
                        memcpy( data, resp->u.crud.payload, *len );
                        wrp_free_struct( resp );
                    }
                }
            } else {
                printf( "Failure in some way: %ld\n", response_code );
            }
        }

        curl_easy_cleanup( curl );
        curl_global_cleanup();
        free( out );
    }

    return data;
}

/**
 *  Do all the fun work and none of the argument parsing.
 *
 *  @param filename the filename to process
 *  @param start    the starting unix time or 0 for current week
 *  @param end      the ending unix time or 0 for current week
 *
 *  @return 0 on success, error otherwise
 */
static int process( uint8_t *data, size_t len, int start, int end )
{
    int rv = -1;

    if( 0 < len ) {
        schedule_t *s = NULL;

        rv = decode_schedule( len, data, &s );
        if( 0 == rv ) {
            time_t i;
            int offset; // offset from the start of the week in seconds */
            char *last = NULL;

            set_unix_time_zone( s->time_zone );

            /* Default to just this week. */
            if( 0 == end ) {
                time_t now;
                
                now = get_unix_time();
                start = now - convert_unix_time_to_weekly(now);
                end = start + 7 * 24 * 3600;
            }

            offset = convert_unix_time_to_weekly(start);

            print_schedule( s );

            printf( "\n" );
            printf( "Using Timezone: %s\n", s->time_zone );
            printf( "Range from: %d until: %d\n", start, end );
            printf( "\n" );
            printf( "Weekly (S) | Unixtime     | Date                | Blocked Devices\n" );
            printf( "-----------+--------------+---------------------+-------------------------\n" );

            for( i = start; i < end; i++, offset++ ) {
                char *macs;

                macs = get_blocked_at_time( s, i );

                if( (NULL == macs && NULL != last) ||
                    (NULL != macs && NULL == last) ||
                    (NULL != macs && NULL != last && 0 != strcmp(macs, last)) )
                {
                    struct tm ts;

                    ts = *localtime(&i);

                    printf( " %9.d | %-12.ld | %d-%02d-%02d %02d:%02d:%02d | %s\n",
                            offset, i,
                            (ts.tm_year+1900), (ts.tm_mon+1), ts.tm_mday,
                            ts.tm_hour, ts.tm_min, ts.tm_sec, (NULL != macs) ? macs : "" );

                    if( NULL != last ) {
                        aker_free( last );
                    }
                    last = macs;
                } else {
                    aker_free( macs );
                }
            }

            if( NULL != last ) {
                aker_free( last );
            }

            rv = 0;
        }

        if( NULL != s ) {
            destroy_schedule( s );
            s = NULL;
        }
    }

    if( NULL != data ) {
        aker_free( data );
        data = NULL;
    }

    return rv;
}
