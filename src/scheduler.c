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
#include <time.h>

#include "schedule.h"
#include "process_data.h"
#include "aker_log.h"
#include "scheduler.h"
#include "decode.h"
#include "time.h"
#include "aker_mem.h"
#include "aker_metrics.h"

#ifdef INCLUDE_BREAKPAD
#include "breakpad_wrapper.h"
#endif


/* Local Functions and file-scoped variables */
static void calculate_report_jitter( uint32_t rate, uint32_t *jitter );
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
    schedule_t *s = NULL;
    int rv = 0;

    debug_info("process_schedule_data()\n");

    if (0 == len) {
        pthread_mutex_lock( &schedule_lock );
        s = current_schedule;
        current_schedule = NULL;
        pthread_mutex_unlock( &schedule_lock );
        pthread_cond_signal(&cond_var);
        destroy_schedule( s );
        aker_metric_set_schedule_enabled(0);    //Schedule_Enabled is 0 as schedule is empty
        aker_metric_set_tz("NULL");
        aker_metric_set_tz_offset(0);
        debug_info( "process_schedule_data() empty schedule\n" );
    } else {
        rv = decode_schedule( len, data, &s );

        
        if (0 == rv ) {  
            schedule_t *tmp;
            print_schedule( s );
            pthread_mutex_lock( &schedule_lock );
            tmp = current_schedule;
            current_schedule = s;
            pthread_mutex_unlock( &schedule_lock );
            pthread_cond_signal(&cond_var);
            destroy_schedule(tmp);
            debug_info( "process_schedule_data() New schedule\n" );
        } else {
            destroy_schedule( s );
            debug_error( "process_schedule_data() Failed to decode\n" );
        }
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
    time_t last_report_time;
    time_t next_report_time = INT_MAX;
    time_t current_unix_time = 0;
    int rv = ETIMEDOUT;
    uint32_t last_report_rate = 0;
    uint32_t report_jitter = 0; /* seconds */
    
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
    
    firewall_cmd = (const char*) args;

    call_firewall( firewall_cmd, NULL );

    last_report_time = get_unix_time();

    while( __keep_going__ ) {
        int info_period = 3;
        int schedule_changed = 0;
   
        pthread_mutex_lock( &schedule_lock );
        
        if( current_schedule ) {
            char *blocked_macs;

            current_unix_time = get_unix_time();
            blocked_macs = get_blocked_at_time(current_schedule, current_unix_time);
            debug_info("Time to process current schedule event is %ld seconds\n", (get_unix_time() - current_unix_time));

            if (NULL == current_blocked_macs) {
                if (NULL != blocked_macs) {
                    current_blocked_macs = blocked_macs;
                    schedule_changed = 1;
                }
            } else {
                if (NULL != blocked_macs) {
                    if (0 != strcmp(current_blocked_macs, blocked_macs)) {
                        aker_free(current_blocked_macs);
                        current_blocked_macs = blocked_macs;

                        schedule_changed = 1;
                    } else {/* No Change In Schedule */
                        if (0 == (info_period++ % 3)) {/* Reduce Clutter */
                            debug_print("scheduler_thread(): No Change\n");
                        }
                        aker_free(blocked_macs);
                    }
                } else {
                    aker_free(current_blocked_macs);
                    current_blocked_macs = NULL;
                    schedule_changed = 1;
                }
            }
        } else {
            if( current_blocked_macs ) {
                aker_free(current_blocked_macs);
                current_blocked_macs = NULL;
                schedule_changed = 1;
            }
        }

        if( 0 != schedule_changed ) {
           if( NULL != current_blocked_macs ) { //To set schedule_enabled parameter
                aker_metric_inc_schedule_set_count();
                aker_metric_set_schedule_enabled(1);
                if(current_schedule->time_zone != NULL) {
                    aker_metric_set_tz(current_schedule->time_zone);
                    set_unix_time_zone(current_schedule->time_zone);

                    debug_print("The timezone is %s and %s\n", tzname[0], tzname[1]);
                    debug_print("The offset is %+ld seconds\n", timezone);

                    //"timezone" parameter is not defined in aker code and will be set from tzset()
                    aker_metric_set_tz_offset(timezone);
                } else {
                    debug_info("The timezone set is NULL\n");
                    aker_metric_set_tz("NULL");
                    aker_metric_set_tz_offset(0);
                }
            } else {
                aker_metric_set_schedule_enabled(0);
                aker_metric_set_tz("NULL");
                aker_metric_set_tz_offset(0);
            }
            call_firewall( firewall_cmd, current_blocked_macs );

            /* Only if the reporting rate changes, calculate a new report rate jitter */
            if( last_report_rate != current_schedule->report_rate_s ) {
                last_report_rate = current_schedule->report_rate_s;

                /* Remove the previously calculated jitter so we don't compound
                 * the jitter. */
                last_report_time -= report_jitter;
                calculate_report_jitter( current_schedule->report_rate_s,
                                         &report_jitter );
                last_report_time += report_jitter;
            }
        }

        /* Report if it is time. */
        if( next_report_time <= current_unix_time ) {
            aker_metrics_report(current_unix_time);
            last_report_time = current_unix_time;
        }

        /* Never report if disabled */
        next_report_time = INT_MAX;
        if( current_schedule && (0 < current_schedule->report_rate_s) ) {
            /* Calculate the next report time. */
            next_report_time = last_report_time + current_schedule->report_rate_s;
        }

        tm.tv_sec = get_next_unixtime(current_schedule, current_unix_time);

        /* Choose the earlier time of reporting or the next event. */
        if( next_report_time < tm.tv_sec ) {
            tm.tv_sec = next_report_time;
        }

        rv = pthread_cond_timedwait(&cond_var, &schedule_lock, &tm);
        if( (0 != rv) && (ETIMEDOUT != rv) ) {
            debug_error("pthread_cond_timedwait error: %d(%s)\n", rv, strerror(rv));
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

        buf = (char*) aker_malloc( len * sizeof(char) );
        if( NULL != buf ) {
            int rv;
            if( NULL != blocked ) {
                sprintf( buf, "%s %s", firewall_cmd, blocked );
                aker_metric_inc_device_block_count(get_blocked_mac_count(blocked));
            } else {
                sprintf( buf, "%s", firewall_cmd );
            }
            debug_info( "Firewall command: '%s'\n", buf );
            rv = system( buf );
            aker_metric_inc_window_trans_count();
            aker_free( buf );
            debug_info( "command result: %d\n", rv );
        } else {
            debug_error( "Failed to allocate buffer needed to call firewall cmd.\n" );
        }
    }
}

static void calculate_report_jitter( uint32_t rate, uint32_t *jitter )
{
    if( 0 == rate ) {
        *jitter = 0;
        return;
    }

    /* This doesn't need to be a high quality random number, just somewhat
     * random. */
    *jitter = rand() % rate;
}

static void sig_handler(int sig)
{
    if( sig == SIGINT ) {
        signal(SIGINT, sig_handler); /* reset it to this function */
        cleanup();
        debug_info("Scheduler - SIGINT received!\n");
        exit(0);
    } else if( sig == SIGUSR1 ) {
        signal(SIGUSR1, sig_handler); /* reset it to this function */
        debug_info("Scheduler - SIGUSR1 received!\n");
    } else if( sig == SIGUSR2 ) {
        signal(SIGUSR2, sig_handler);
        debug_info("Scheduler - SIGUSR2 received!\n");
    } else if( sig == SIGCHLD ) {
        signal(SIGCHLD, sig_handler); /* reset it to this function */
        debug_info("Scheduler - SIGHLD received!\n");
    } else if( sig == SIGPIPE ) {
        signal(SIGPIPE, sig_handler); /* reset it to this function */
        debug_info("Scheduler - SIGPIPE received!\n");
    } else if( sig == SIGALRM ) {
        signal(SIGALRM, sig_handler); /* reset it to this function */
        debug_info("Scheduler - SIGALRM received!\n");
    } else {
        cleanup();
        debug_info("Scheduler - Signal %d received!\n", sig);
        exit(0);
    }
}

void cleanup (void ) 
{
    pthread_mutex_unlock( &schedule_lock );
    pthread_mutex_destroy(&schedule_lock);
    destroy_schedule(current_schedule);
    destroy_akermetrics();
}
