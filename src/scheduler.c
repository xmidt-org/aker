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
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>

#include "schedule.h"
#include "process_data.h"
#include "aker_log.h"
#include "scheduler.h"
#include "decode.h"
#include "time.h"


/* Local Functions and file-scoped variables */
static void sig_handler(int sig);
static void cleanup(void);
static void *scheduler_thread(void *args);
static void call_firewall( const char* firewall_cmd, char *blocked );

static schedule_t *current_schedule = NULL;
static char *current_blocked_macs = NULL;
static pthread_mutex_t schedule_lock;
static pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;



/*----------------------------------------------------------------------------*/
/*                             External functions                             */
/*----------------------------------------------------------------------------*/

static int __keep_going__ = 1;
void terminate_scheduler_thread(void)
{
    __keep_going__ = 0;
}


/* See scheduler.h for details. */
int scheduler_start( pthread_t *thread, const char *firewall_cmd )
{
    pthread_t t, *p;
    pthread_mutex_init( &schedule_lock, NULL );
    int rv;

    p = &t;
    if( NULL != thread ) {
        p = thread;
    }

    rv = pthread_create( p, NULL, scheduler_thread, (void*) firewall_cmd );
    if( 0 != rv ) {
        pthread_mutex_destroy(&schedule_lock);
    }

    return rv;
}


/* See scheduler.h for details. */
int process_schedule_data( size_t len, uint8_t *data )
{
    schedule_t *s;
    int rv;

    debug_info("process_schedule_data()\n");
    rv = decode_schedule( len, data, &s );

    if (0 == rv ) {
        print_schedule( s );
        pthread_mutex_lock( &schedule_lock );
        destroy_schedule(current_schedule);
        current_schedule = s;
        pthread_mutex_unlock( &schedule_lock );
        pthread_cond_signal(&cond_var);
        debug_info( "process_schedule_data() New schedule\n" );
    } else {
        destroy_schedule( s );
        debug_error( "process_schedule_data() Failed to decode\n" );
    }

    return rv;
}


/* See scheduler.h for details. */
char *get_current_blocked_macs( void )
{
    char *macs = NULL;
    int rv;

    rv = pthread_mutex_lock( &schedule_lock );
    if( (0 == rv) && current_blocked_macs ) {
        macs = strdup(current_blocked_macs);
    }
    pthread_mutex_unlock( &schedule_lock );

    return macs;
}


/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

/**
 *  Main scheduler thread.
 */
void *scheduler_thread(void *args)
{
    const char *firewall_cmd;
    struct timespec tm = { INT_MAX, 0 };
    time_t unix_time = 0, process_time = 0;
    int rv = ETIMEDOUT;
    
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
    
    firewall_cmd = (const char*) args;

    call_firewall( firewall_cmd, NULL );

    unix_time = get_unix_time();
    while( __keep_going__ ) {
        int info_period = 3;
   
        pthread_mutex_lock( &schedule_lock );
        
        if( current_schedule ) {
            char *blocked_macs;

            process_time = get_unix_time();
            blocked_macs = get_blocked_at_time(current_schedule, unix_time);
            process_time = get_unix_time() - process_time;
            debug_info("Time to process current schedule event is %ld seconds\n", process_time);

            if (NULL == current_blocked_macs) {
                if (NULL != blocked_macs) {
                    current_blocked_macs = blocked_macs;
                    call_firewall( firewall_cmd, current_blocked_macs );
                }
            } else {
                if (NULL != blocked_macs) {
                    if (0 != strcmp(current_blocked_macs, blocked_macs)) {
                        free(current_blocked_macs);
                        current_blocked_macs = blocked_macs;

                        call_firewall( firewall_cmd, current_blocked_macs );
                    } else {/* No Change In Schedule */
                        if (0 == (info_period++ % 3)) {/* Reduce Clutter */
                            debug_print("scheduler_thread(): No Change\n");
                        }
                        free(blocked_macs);
                    }
                } else {
                    free(current_blocked_macs);
                    current_blocked_macs = NULL;
                }
            }
        }

        tm.tv_sec = get_next_unixtime(current_schedule, unix_time);
        rv = pthread_cond_timedwait(&cond_var, &schedule_lock, &tm);
        if( ETIMEDOUT != rv) {
            debug_error("pthread_cond_timedwait: %d(%s)\n", rv, strerror(rv));
        }

        pthread_mutex_unlock( &schedule_lock );
    }
    
    cleanup();
    return NULL;    
}

/**
 *  Takes the firewall cmd and the blocked list and makes the call.
 *
 *  @param firewall_cmd the firewall cmd to call
 *  @param blocked      the list of mac addresses to block
 */
static void call_firewall( const char* firewall_cmd, char *blocked )
{
    if( NULL != firewall_cmd ) {
        char *buf;
        size_t len;

        len = strlen( firewall_cmd );
        if( NULL != blocked ) {
            len++; /* for space between */
            len += strlen( blocked );
        }
        len++; /* For trailing '\0' */

        buf = (char*) malloc( len * sizeof(char) );
        if( NULL != buf ) {
            if( NULL != blocked ) {
                sprintf( buf, "%s %s", firewall_cmd, blocked );
            } else {
                sprintf( buf, "%s", firewall_cmd );
            }
            debug_info( "Firewall command: '%s'\n", buf );
            system( buf );
            free( buf );
        } else {
            debug_error( "Failed to allocate buffer needed to call firewall cmd.\n" );
        }
    }
}

static void sig_handler(int sig)
{
    if( sig == SIGINT ) {
        signal(SIGINT, sig_handler); /* reset it to this function */
        cleanup();
        debug_info("SIGINT received!\n");
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
        cleanup();
        debug_info("Signal %d received!\n", sig);
        exit(0);
    }
}

void cleanup (void ) 
{
    pthread_mutex_unlock( &schedule_lock );
    pthread_mutex_destroy(&schedule_lock);
    destroy_schedule(current_schedule);
}