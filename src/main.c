/**
 * Copyright 2017 Comcast Cable Communications Management, LLC
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>

#include <libparodus.h>
#include <msgpack.h>

#include "aker_log.h"
#include "schedule.h"
#include "wrp_interface.h"
#include "scheduler.h"
#include "process_data.h"
#include "aker_md5.h"
#include "aker_mem.h"
#include "aker_help.h"
#include "aker_metrics.h"
#include "time.h"

#ifdef INCLUDE_BREAKPAD
#include "breakpad_wrapper.h"
#endif

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define LIBPD_CLOSED_MSG_RECEIVED 2

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
size_t max_macs = INT_MAX;


/*----------------------------------------------------------------------------*/
/*                            Global Variables                                */
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void sig_handler(int sig);
static void import_existing_schedule( const char *data_file, const char *md5_file );
static int main_loop(libpd_cfg_t *cfg, char *data_file, char *md5_file, const char *device_id );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main( int argc, char **argv)
{
    const char *option_string = "p:c:w:d:f:m:i:h::";
    static const struct option options[] = {
        { "help",         optional_argument, 0, 'h' },
        { "parodus-url",  required_argument, 0, 'p' },
        { "client-url",   required_argument, 0, 'c' },
        { "firewall-cmd", required_argument, 0, 'w' },
        { "data-file",    required_argument, 0, 'd' },
        { "md5-file",     required_argument, 0, 'f' },
        { "max-macs",     required_argument, 0, 'm' },
        { "device-id",    required_argument, 0, 'i' },
        { 0, 0, 0, 0 }
    };

    libpd_cfg_t cfg = { .service_name = "aker",
                        .receive = true,
                        .keepalive_timeout_secs = 64,
                        .parodus_url = NULL,
                        .client_url = NULL
                      };

    char *firewall_cmd = NULL;
    char *data_file = NULL;
    char *md5_file = NULL;
    const char *device_id = NULL;
    int item = 0;
    int opt_index = 0;
    int rv = 0;
    pthread_t thread_id;

    time_t start_unix_time = 0;
    debug_info("********** Starting component: aker **********\n ");
#if defined(ENABLE_FEATURE_TELEMETRY2_0)
    t2_init("aker");
    debug_info("aker T2 init done\n");
#endif

    start_unix_time = get_unix_time();
    srand((unsigned int)start_unix_time);
    debug_info("start_unix_time is %ld\n", start_unix_time);
    aker_metric_set_process_start_time(start_unix_time);
    
    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);
    signal(SIGKILL, sig_handler);
    signal(SIGQUIT, sig_handler);
    signal(SIGHUP, sig_handler);
    signal(SIGALRM, sig_handler);
#ifdef INCLUDE_BREAKPAD
    /* breakpad handles the signals SIGSEGV, SIGBUS, SIGFPE, and SIGILL */
    breakpad_ExceptionHandler();
#else
    signal(SIGSEGV, sig_handler);
    signal(SIGBUS, sig_handler); 
    signal(SIGFPE, sig_handler);
    signal(SIGILL, sig_handler);
#endif  
    
    while( -1 != (item = getopt_long(argc, argv, option_string, options, &opt_index)) ) {
        switch( item ) {
            case 'p':
                cfg.parodus_url = strdup(optarg);
                break;
            case 'c':
                cfg.client_url = strdup(optarg);
                break;
            case 'w':
                firewall_cmd = strdup(optarg);
                break;
            case 'd':
                data_file = strdup(optarg);
                break;
            case 'f':
                md5_file = strdup(optarg);
                break;
            case 'm':
                max_macs = atoi(optarg);
                break;
            case 'i':
                /* This is basically a constant for the program.  No need to
                 * duplicate, just point to the original arguement. */
                device_id = optarg;
                break;
            case 'h':
                aker_help(argv[0], optarg);
                break;
            case '?':
                if (strchr(option_string, optopt)) {
                    printf("%s Option %c requires an argument!\n", argv[0], optopt);
                    aker_help(argv[0], NULL);
                    rv = -7;
                    break;
                } else {
                    printf("%s Unrecognized option %c\n", argv[0], optopt);
                  }

                break;

            default:
                break;
        }
    }

    if (max_macs <= 0) {
        max_macs = INT_MAX;
    }

    if( (rv == 0) &&
        (NULL != cfg.parodus_url) &&
        (NULL != cfg.client_url) &&
        (NULL != firewall_cmd) &&
        (NULL != data_file) &&
        (NULL != md5_file) &&
        (NULL != device_id) )
    {
        scheduler_start( &thread_id, firewall_cmd );

        import_existing_schedule( data_file, md5_file );
        
        main_loop(&cfg, data_file, md5_file, device_id);
        rv = 0;
    } else {
        if (!cfg.parodus_url) {
            debug_error("%s parodus_url not specified!\n", argv[0]);
            rv = -1;
        }
        if (!cfg.client_url) {
            debug_error("%s client_url not specified !\n", argv[0]);
            rv = -2;
        }
        if (!firewall_cmd) {
            debug_error("%s firewall_cmd not specified!\n", argv[0]);
            rv = -3;
        }
        if (!data_file) {
            debug_error("%s data_file not specified!\n", argv[0]);
            rv = -4;
        }
        if (!md5_file) {
            debug_error("%s md5_file not specified!\n", argv[0]);
            rv = -5;
        }
        if (!device_id) {
            debug_error("%s device_id not specified!\n", argv[0]);
            rv = -6;
        }
     }
    
    if (rv != 0) {
        debug_error("%s  program terminating\n", argv[0]);
    }

    if( NULL != md5_file )          aker_free( md5_file );
    if( NULL != data_file )         aker_free( data_file );
    if( NULL != firewall_cmd )      aker_free( firewall_cmd );
    if( NULL != cfg.parodus_url )   aker_free( (char*) cfg.parodus_url );
    if( NULL != cfg.client_url )    aker_free( (char*) cfg.client_url );

    return rv;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static void sig_handler(int sig)
{
    if( sig == SIGINT ) {
        debug_info("SIGINT received! Program Terminating!\n");
        exit(0);
    } else if ( sig == SIGTERM ) {
        debug_info("SIGTERM received! Program Terminating!\n");
        exit(0);
    } else if( sig == SIGUSR1 ) {
        signal(SIGUSR1, sig_handler); /* reset it to this function */
        debug_info("SIGUSR1 received!\n");
    } else if( sig == SIGUSR2 ) {
        signal(SIGUSR2, sig_handler);
        debug_info("SIGUSR2 received!\n");
    } else if( sig == SIGCHLD ) {
        signal(SIGCHLD, sig_handler); /* reset it to this function */
        debug_info("SIGHLD received!\n");
    } else if( sig == SIGPIPE ) {
        signal(SIGPIPE, sig_handler); /* reset it to this function */
        debug_info("SIGPIPE received!\n");
    } else if( sig == SIGALRM ) {
        signal(SIGALRM, sig_handler); /* reset it to this function */
        debug_info("SIGALRM received!\n");
    } else {
        debug_info("Signal %d received!\n", sig);
        exit(0);
    }
}

static void import_existing_schedule( const char *data_file, const char *md5_file )
{
    size_t len;
    uint8_t *data = NULL;

    if (0 != verify_md5_signatures(data_file, md5_file)) {
        debug_error("import_existing_schedule() data or md5 corruption\n");
        aker_metric_inc_md5_err_count();
    }

    len = read_file_from_disk( data_file, &data );
    if( 0 < len ) {
        process_schedule_data( len, data );
        aker_free( data );
    }
}


static int main_loop(libpd_cfg_t *cfg, char *data_file, char *md5_file, const char *device_id )
{
    int rv;
    wrp_msg_t *wrp_msg;
    libpd_instance_t hpd_instance;
    int backoff_retry_time = 0;
    int max_retry_sleep = (1 << 9) - 1;
    int c = 2;

    while( true ) {
        rv = libparodus_init( &hpd_instance, cfg );
        if( 0 != rv ) {
            backoff_retry_time = (1 << c) - 1;
            sleep(backoff_retry_time);
            c++;

            if( backoff_retry_time >= max_retry_sleep ) {
                c = 2;
                backoff_retry_time = 0;
            }
        }
        debug_info("Init for parodus (url %s): %d '%s'\n", cfg->parodus_url, rv, libparodus_strerror(rv) );
        if( 0 == rv ) {
            break;
        }
        libparodus_shutdown(&hpd_instance);
    }

    aker_metric_init(device_id, hpd_instance);

    debug_print("starting the main loop...\n");
    while( true ) {
        rv = libparodus_receive(hpd_instance, &wrp_msg, 2000);

        if( 0 == rv ) {
            wrp_msg_t response;

            debug_print("Got something from parodus.\n");
            memset(&response, 0, sizeof(wrp_msg_t));
            rv = process_wrp(data_file, md5_file, wrp_msg, &response);
            if( 0 == rv ) {
                libparodus_send(hpd_instance, &response);
            }
            cleanup_wrp(&response);
        } else if( 1 == rv || LIBPD_CLOSED_MSG_RECEIVED == rv ) {
            debug_print("Timed out or message closed.\n");
            continue;
        } else {
            debug_info("Libparodus failed to receive message: '%s'\n",libparodus_strerror(rv));
        }

        if( (NULL != wrp_msg) && (LIBPD_CLOSED_MSG_RECEIVED != rv) ) {
            wrp_free_struct(wrp_msg);
            wrp_msg = NULL;
        }
    }

    (void ) libparodus_shutdown(&hpd_instance);
    debug_print("End of parodus_upstream\n");
    return 0;
}


int32_t get_max_mac_limit(void)
{
    return max_macs;
}
