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
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static int process( const char *filename, int start, int end );

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
    const char *option_string = "f:s:e:h::";
    static const struct option options[] = {
        { "help",  optional_argument, 0, 'h' },
        { "file",  required_argument, 0, 'f' },
        { "start", required_argument, 0, 's' },
        { "end",   required_argument, 0, 'e' },
        { 0, 0, 0, 0 }
    };
    const char *filename = NULL;
    int start = 0;
    int end = 0;
    int i = 0;
    int item = 0;

    extern int cimplog_debug_level;

    cimplog_debug_level = -1;

    while( -1 != (item = getopt_long(argc, argv, option_string, options, &i)) ) {
        switch( item ) {
            case 'f':
                filename = optarg;
                break;
            case 's':
                start = atoi(optarg);
                break;
            case 'e':
                end = atoi(optarg);
                break;

            default:
                fprintf( stderr, "Usage:\naker-cli -f filename [-s starting_unixtime] [-e ending_unixtime]\n\n" );
                fprintf( stderr, "    Outputs the schedule according to aker and how it interprets a schedule\n" );
                fprintf( stderr, "    over a window of time.\n\n" );
                fprintf( stderr, "    If ending_unixtime is 0, process the entire current week.\n" );
                return -1;
        }
    }

    if( NULL == filename ) {
        fprintf( stderr, "Filename is missing.\n" );
        return -2;
    }

    return process( filename, start, end );
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

/**
 *  Do all the fun work and none of the argument parsing.
 *
 *  @param filename the filename to process
 *  @param start    the starting unix time or 0 for current week
 *  @param end      the ending unix time or 0 for current week
 *
 *  @return 0 on success, error otherwise
 */
static int process( const char *filename, int start, int end )
{
    uint8_t *data = NULL;
    size_t len;
    int rv = -1;

    len = read_file_from_disk( filename, &data );
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
