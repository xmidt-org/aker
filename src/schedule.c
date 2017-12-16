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

#include "schedule.h"
#include "time.h"
#include "process_data.h"
#include "aker_log.h"
#include "aker_mem.h"
#include "main.h"

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
char* __convert_event_to_string( schedule_t *s, schedule_event_t *e );
int __validate_mac( const char *mac, size_t len );



/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
 
/* See schedule.h for details. */
schedule_t* create_schedule( void )
{
    schedule_t *s;

    s = (schedule_t*) aker_malloc( sizeof(schedule_t) );
    if( NULL != s ) {
        memset( s, 0, sizeof(schedule_t) );
    }

    return s;
}


/* See schedule.h for details. */
schedule_event_t* create_schedule_event( size_t block_count )
{
    schedule_event_t *s = NULL;
    size_t size;
    size_t max_macs = get_max_mac_limit();

    if (block_count > max_macs) {
        debug_error("create_schedule_event() Error Request %d exceeds maximum %d\n",
                     block_count, max_macs);
        return s;
    }

    size = sizeof(schedule_event_t) + block_count * sizeof(int);

    s = (schedule_event_t*) aker_malloc( size );
    if( NULL != s ) {
        memset( s, 0, size );
        s->block_count = block_count;
    }

    return s;
}

/* See schedule.h for details. */
schedule_event_t* copy_schedule_event( schedule_event_t *e )
{
    schedule_event_t *n;

    n = NULL;
    if( NULL != e ) {
        n = create_schedule_event( e->block_count );
        if( NULL != n ) {
            size_t i;

            for( i = 0; i < e->block_count; i++ ) {
                n->block[i] = e->block[i];
            }
        }
    }

    return n;
}


/* See schedule.h for details. */
void insert_event(schedule_event_t **head, schedule_event_t *e )
{
    schedule_event_t *cur, *prev;

    if( (NULL == head) || (NULL == e) ) {
        return;
    }

    cur = prev = *head;
    while( (NULL != cur) && (cur->time < e->time) ) {
        prev = cur;
        cur = cur->next;
    }

    e->next = cur;
    if( (NULL == prev) || (e->time < prev->time) ) {
        *head = e;
    } else {
        prev->next = e;
    }
}


/* See schedule.h for details. */
int finalize_schedule( schedule_t *s )
{
    int rv = 0;

    if( NULL != s ) {
        if( NULL != s->weekly ) {
            /* Ensure that we have the right starting point: the last event
             * from the previous week's schedule. */
            if( 0 < s->weekly->time ) {
                schedule_event_t *e, *p;

                p = s->weekly;
                while( NULL != p->next ) {
                    p = p->next;
                }

                e = copy_schedule_event( p );
                if( NULL != e ) {
                    e->time = p->time - SECONDS_IN_A_WEEK;
                    insert_event( &s->weekly, e );
                } else {
                    rv = -1;
                }
            }
        }
    }

    return rv;
}

/* See schedule.h for details. */
void destroy_schedule( schedule_t *s )
{
    if( NULL != s ) {
        schedule_event_t *n;

        while( NULL != s->absolute ) {
            n = s->absolute->next;
            aker_free( s->absolute );
            s->absolute = n;
        }

        while( NULL != s->weekly ) {
            n = s->weekly->next;
            aker_free( s->weekly );
            s->weekly = n;
        }

        if( NULL != s->macs ) {
            aker_free( s->macs );
        }
        
        if (NULL != s->time_zone) {
            aker_free( s->time_zone);
        }

        aker_free( s );
    }
}


/* See schedule.h for details. */
char* get_blocked_at_time( schedule_t *s, time_t unixtime )
{
    schedule_event_t *abs_prev, *abs_cur, *w_prev, *w_cur;
    char *rv;
    time_t weekly, last_abs;

    weekly = convert_unix_time_to_weekly( unixtime );

    rv = NULL;

    if( NULL != s ) {
        /* Check absolute schedule first */
        abs_prev = s->absolute;
        abs_cur = NULL;
        if( NULL != abs_prev ) {
            abs_cur = abs_prev->next;
        }

        while( (NULL != abs_cur) && (abs_cur->time <= unixtime) ) {
            abs_prev = abs_cur;
            abs_cur = abs_cur->next;
        }

        /* Make the default relative value of the absolute time in the future
         * so it's ignored. */
        last_abs = weekly + 1;
        if( NULL != abs_prev ) {
            if( (NULL != abs_cur) && (abs_prev->time <= unixtime) ) {
                /* In the absolute schedule */
                rv = __convert_event_to_string( s, abs_prev );
                goto done;
            }

            last_abs = convert_unix_time_to_weekly( abs_prev->time );
        }

        /* Either we're not in the abs schedule or it just ended
         * and we need to figure out the next event time for the end. */

        /* Get the relative schedule */
        w_prev = s->weekly;
        w_cur = NULL;
        if( NULL != w_prev ) {
            w_cur = w_prev->next;
        }

        while( (NULL != w_cur) && (w_cur->time <= weekly) ) {
            w_prev = w_cur;
            w_cur = w_cur->next;
        }

        /* If the abs time event is the most recent, use it as long
         * as it's in the past.  Otherwise use the weekly schedule. */
        if( NULL != w_prev) {
            if( (w_prev->time < last_abs) && (last_abs <= weekly) ) {
                rv = __convert_event_to_string( s, abs_prev );
            } else {
                rv = __convert_event_to_string( s, w_prev );
            }
        } else {
            if( last_abs <= weekly ) {
                rv = __convert_event_to_string( s, abs_prev );
            }
        }
    }

done:
    debug_info( "Time: %ld (%ld) -> '%s'\n", unixtime, weekly, rv );
    return rv;
}


/* See schedule.h for details. */
int create_mac_table( schedule_t *s, size_t count )
{
    s->macs = (mac_address*) aker_malloc( count * sizeof(mac_address) );
    if( NULL == s->macs ) {
        return -1;
    }
    s->mac_count = count;

    memset( s->macs, 0, count * sizeof(mac_address) );

    return 0;
}



/* See schedule.h for details. */
int set_mac_index( schedule_t *s, const char *mac, size_t len, uint32_t index )
{
    int rv;

    rv = -1;
    if( (NULL != s) && (index < s->mac_count) ) {
        rv = __validate_mac( mac, len );
        if( 0 == rv ) {
            memcpy( &s->macs[index].mac[0], mac, len );
            s->macs[index].mac[len] = '\0';
        }
    }

    return rv;
}


/* See schedule.h for details. */
time_t get_next_unixtime(schedule_t *s, time_t unixtime)
{
    schedule_event_t *p;
    time_t next_unixtime = INT_MAX, first_weekly = INT_MAX;
    uint32_t num_events = 0;

    if( NULL != s ) {
        time_t weekly;

        /* Check absolute schedule first */
        for( p = s->absolute; NULL != p; p = p->next ) {
            if( (p->time > unixtime) && (p->time < next_unixtime) ) {
                next_unixtime = p->time;
                goto done;
            }
        }

        /* Check the relative schedule next */
        weekly = convert_unix_time_to_weekly( unixtime );

        for( p = s->weekly; NULL != p; p = p->next ) {
            time_t t = (unixtime - weekly) + p->time;
            if( (p->time > weekly) && (t < next_unixtime) ) {
                next_unixtime = t;
            }

            if( 0 < p->time ) {
                if( 0 == num_events ) {
                    first_weekly = p->time;
                }
                num_events++;
            }
        }

        if( 0 == num_events ) {
            next_unixtime = INT_MAX;
        } else if ( INT_MAX == next_unixtime ) {
            next_unixtime = (unixtime - weekly) + first_weekly + SECONDS_IN_A_WEEK;
        }
    }

done:
    debug_info( "Next unix time: %ld\n", next_unixtime );
    return next_unixtime;
}


/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/


/**
 *  Convert a block pointing to a list of macs in a schedule into a string
 *  of the MAC addresses.
 *
 *  @param s the schedule to use to for resolution
 *  @param e the event to convert
 *
 *  @return the string with the list of blocked addresses (may be NULL and valid)
 */
char* __convert_event_to_string( schedule_t *s, schedule_event_t *e )
{
    char *rv;

    rv = NULL;
    if( (NULL != s) && (NULL != e) ) {
        size_t count;

        count = e->block_count;

        rv = (char*) aker_malloc( sizeof(char) * (count * MAC_ADDRESS_SIZE + 1) );
        if( NULL != rv ) {
            char *p = rv;
            bool string_ok = false;
            size_t i;

            for( i = 0; i < count; i++ ) {
                if( e->block[i] < s->mac_count ) {
                    string_ok = true;
                    memcpy( p, &s->macs[e->block[i]], 17 );
                    p[17] = ' ';
                } else {
                    debug_error("__convert_event_to_string():Invalid mac index\n");
                    string_ok = false;
                    break;
                }
                p = &p[18];
            }
            *p = '\0';

            /* Don't send back an empty string, just put it out of it's
             * misery here. */
            if( false == string_ok ) {
                aker_free( rv );
                rv = NULL;
            } else {
                /* Chomp the extra ' ' and make it a '\0'. */
                p[-1] = '\0';
            }
        }
    }

    return rv;
}


/**
 *  Validates that the MAC address is in the expected format.
 *
 *  @param mac the MAC address to validate
 *  @param len the length of the mac string
 *
 *  @return 0 if valid, failure otherwise
 */
int __validate_mac( const char *mac, size_t len )
{
    int mask = -1;

    if( (NULL != mac) && (17 == len) ) {
        mask  = ! isxdigit(mac[0]);
        mask |= ! isxdigit(mac[1]);
        mask |= mac[2] - ':';
        mask |= ! isxdigit(mac[3]);
        mask |= ! isxdigit(mac[4]);
        mask |= mac[5] - ':';
        mask |= ! isxdigit(mac[6]);
        mask |= ! isxdigit(mac[7]);
        mask |= mac[8] - ':';
        mask |= ! isxdigit(mac[9]);
        mask |= ! isxdigit(mac[10]);
        mask |= mac[11] - ':';
        mask |= ! isxdigit(mac[12]);
        mask |= ! isxdigit(mac[13]);
        mask |= mac[14] - ':';
        mask |= ! isxdigit(mac[15]);
        mask |= ! isxdigit(mac[16]);
    }

    return mask;
}
