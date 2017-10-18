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
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <getopt.h>

#include <libparodus.h>
#include <msgpack.h>

#include "aker_log.h"
#include "aker_types.h"
#include "wrp_interface.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define debug_error(...)      cimplog_error("aker", __VA_ARGS__)
#define debug_info(...)       cimplog_info("aker", __VA_ARGS__)
#define debug_print(...)      cimplog_debug("aker", __VA_ARGS__)

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static libpd_instance_t hpd_instance;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void sig_handler(int sig);
static int main_loop(libpd_cfg_t *cfg);
static void connect_parodus(libpd_cfg_t *cfg);

int decode_aker( size_t count, uint8_t *bytes, schedule_t **s );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main( int argc, char **argv)
{
    static const struct option options[] = {
        { "parodus_url", required_argument, 0, 'p' },
        { "client_url",  required_argument, 0, 'c' },
        { 0, 0, 0, 0 }
    };

    libpd_cfg_t cfg = { .service_name = "parental-control",
                        .receive = true, 
                        .keepalive_timeout_secs = 64,
                        .parodus_url = NULL,
                        .client_url = NULL
                      };

    int item = 0;
    int opt_index = 0;

    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);
    signal(SIGSEGV, sig_handler);
    signal(SIGBUS, sig_handler);
    signal(SIGKILL, sig_handler);
    signal(SIGFPE, sig_handler);
    signal(SIGILL, sig_handler);
    signal(SIGQUIT, sig_handler);
    signal(SIGHUP, sig_handler);
    signal(SIGALRM, sig_handler);

    while( -1 != (item = getopt_long(argc, argv, "p:c", options, &opt_index)) ) {
        switch( item ) {
            case 'p':
                cfg.parodus_url = strdup(optarg);
                break;
            case 'c':
                cfg.client_url = strdup(optarg);
                break;
            default:
                break;
        }    
    }

    if( (NULL != cfg.parodus_url) && (NULL != cfg.client_url) ) {
        main_loop(&cfg);
    }

    if( NULL != cfg.parodus_url ) {
        free((char *) cfg.parodus_url);
    }
    if( NULL != cfg.client_url ) {
        free((char *) cfg.client_url);
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static void sig_handler(int sig)
{
    if( sig == SIGINT ) {
        signal(SIGINT, sig_handler); /* reset it to this function */
        debug_info("SIGINT received!\n");
        exit(0);
    } else if( sig == SIGUSR1 ) {
        signal(SIGUSR1, sig_handler); /* reset it to this function */
        debug_info("SIGUSR1 received!\n");
    } else if( sig == SIGUSR2 ) {
        debug_info("SIGUSR2 received!\n");
    } else if( sig == SIGCHLD ) {
        signal(SIGCHLD, sig_handler); /* reset it to this function */
        debug_info("SIGHLD received!\n");
    } else if( sig == SIGPIPE ) {
        signal(SIGPIPE, sig_handler); /* reset it to this function */
        debug_info("SIGPIPE received!\n");
    } else if( sig == SIGALRM )	{
        signal(SIGALRM, sig_handler); /* reset it to this function */
        debug_info("SIGALRM received!\n");
    } else {
        debug_info("Signal %d received!\n", sig);
        exit(0);
    }
}

static void connect_parodus(libpd_cfg_t *cfg)
{
    int backoffRetryTime = 0;
    int max_retry_sleep = (1 << 5) - 1; /* 2^5 - 1 */
    int c = 2;   //Retry Backoff count shall start at c=2 & calculate 2^c - 1.
    

    // TODO This needs to be re-worked so 1 thread can do everything.
    while( 1 ) {
        if( backoffRetryTime < max_retry_sleep ) {
            backoffRetryTime = (1 << c) - 1;
        }
        debug_print("New backoffRetryTime value calculated as %d seconds\n", backoffRetryTime);
        int ret = libparodus_init(&hpd_instance, cfg);
        if( ret ==0 ) {
            debug_info("Init for parodus Success..!!\n");
            break;
        } else {
            debug_error("Init for parodus (url %s) failed: '%s'\n", cfg->parodus_url, libparodus_strerror(ret));
            sleep(backoffRetryTime);
            c++;
         
	    if( backoffRetryTime == max_retry_sleep ) {
		c = 2;
		backoffRetryTime = 0;
		debug_print("backoffRetryTime reached max value, reseting to initial value\n");
	    }
        }
	libparodus_shutdown(&hpd_instance);
    }
}

static int main_loop(libpd_cfg_t *cfg)
{
    int rtn;
    wrp_msg_t *wrp_msg;

    connect_parodus(cfg);

    debug_print("starting the main loop...\n");
    while( 1 ) {
        rtn = libparodus_receive(hpd_instance, &wrp_msg, 2000);
        debug_print("    rtn = %d\n", rtn);

        if( 0 == rtn ) {
            uint8_t *bytes = NULL;
            debug_info("Got something from parodus.\n");
            wrp_to_object(wrp_msg, &bytes);
        } else if( 1 == rtn || 2 == rtn ) {
            debug_info("Timed out or message closed.\n");
            continue;
        } else {
            debug_info("Libparodus failed to receive message: '%s'\n",libparodus_strerror(rtn));
        }
        if( NULL != wrp_msg ) {
            free(wrp_msg);
        }
        sleep(5);
    }
    libparodus_shutdown(&hpd_instance);
    sleep(1);
    debug_print("End of parodus_upstream\n");
    return 0;
}


int decode_aker( size_t count, uint8_t *bytes, schedule_t **s )
{
    int ret = 0;
    schedule_t *schedule;
    msgpack_zone mempool;
    msgpack_object deserialized;
    msgpack_unpack_return unpack_ret; 
    
    if (!count || !bytes) {
        return -1;
    }
    
    schedule = malloc(sizeof(schedule_t));
    if (!schedule) {
        return -2;
    }
    
    *s = schedule;
    
    msgpack_zone_init( &mempool, 2048 );
    unpack_ret = msgpack_unpack( (const char *) bytes, count, NULL, &mempool, &deserialized );   
    (void ) unpack_ret;
    
    return ret;
}
