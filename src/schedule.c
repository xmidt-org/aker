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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <msgpack.h>
#include <ctype.h>
#include <time.h>


#include "schedule.h"
#include "decode.h"

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


/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
 
/**
 *  Create an empty schedule.
 *
 *  @return NULL on error, valid pointer to a schedule_t otherwise
 */
schedule_t* create_schedule( void )
{
    schedule_t *s;

    s = (schedule_t*) malloc( sizeof(schedule_t) );
    if( NULL != s ) {
        memset( s, 0, sizeof(schedule_t) );
    }

    return s;
}

/**
 *  Create a correctly sized but otherwise empty schedule_event_t struct.
 *
 *  @note Only the block_count is set and the space for the block entries has
 *        been allocated.  The rest is up to the user.
 *
 *  @param block_count the number of blocked mac addresses to size for
 *
 *  @return NULL on error, valid pointer to a schedule_event_t otherwise
 */
schedule_event_t* create_schedule_event( size_t block_count )
{
    schedule_event_t *s;
    size_t size;

    size = sizeof(schedule_event_t) + block_count * sizeof(int);

    s = (schedule_event_t*) malloc( size );
    if( NULL != s ) {
        memset( s, 0, size );
        s->block_count = block_count;
    }

    return s;
}

/**
 *  Inserts a schedule_event_t in sorted order (smallest to largest) into
 *  the specified list (head).
 *
 *  @param head the pointer to the list head
 *  @param e    the schedule_event_t pointer to add to the list
 */
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
    if( NULL == prev ) {
        *head = e;
    } else {
        prev->next = e;
    }
}

/**
 *  Prune all the expired absolute events.
 *
 *  @param s        the schedule to clean up
 *  @param unixtime the time to clean up to
 */
void prune_expired_events( schedule_t *s, uint32_t unixtime )
{
    if( NULL == s ) {
        return;
    }

    while( (NULL != s->absolute) && (s->absolute->time < unixtime) ) {
        schedule_event_t *p;

        p = s->absolute;
        s->absolute = s->absolute->next;
        free(p);
    }
}

/**
 *  Destroys the schedule passed in.
 *
 *  @param s the schedule to destroy
 */
void destroy_schedule( schedule_t *s )
{
    if( NULL != s ) {
        schedule_event_t *n;

        while( NULL != s->absolute ) {
            n = s->absolute->next;
            free( s->absolute );
            s->absolute = n;
        }

        while( NULL != s->weekly ) {
            n = s->weekly->next;
            free( s->weekly );
            s->weekly = n;
        }

        if( NULL != s->macs ) {
            free( s->macs );
        }

        free( s );
    }
}

/**
 *  Convert a block pointing to a list of macs in a schedule into a string
 *  of the MAC addresses.
 *
 *  @param s     the schedule to use to for resolution
 *  @param count the number of indexes in the block
 *  @param block the array of indexes to convert
 *
 *  @return the string with the list of blocked addresses (may be NULL and valid)
 */
char* convert_index_to_string( schedule_t *s, uint32_t count, uint32_t *block )
{
    char *rv;

    rv = NULL;
    if( (NULL != s) && (NULL != block) ) {
        rv = (char*) malloc( sizeof(char) * (count * 18 + 1) );
        if( NULL != rv ) {
            char *p = rv;
            bool string_ok = false;
            size_t i;

            for( i = 0; i < count; i++ ) {
                if( block[i] < s->mac_count ) {
                    string_ok = true;
                    memcpy( p, &s->macs[block[i]], 17 );
                    p[17] = ' ';
                }
                p = &p[18];
            }
            *p = '\0';

            /* Don't send back an empty string, just put it out of it's
             * misery here. */
            if( false == string_ok ) {
                free( rv );
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
 *  Gets the string with the blocked MAC addresses at this time.
 *
 *  @param s       the schedule to apply
 *  @param unitime the unixtime representation
 *  @param weekly  the weekly relative time representation
 *
 *  @return the string with the list of blocked addresses (may be NULL and valid)
 */
char* get_blocked_at_time( schedule_t *s, uint32_t unixtime, uint32_t weekly )
{
    schedule_event_t *prev, *cur;
    char *rv;

    rv = NULL;

    if( NULL != s ) {
        /* Check absolute schedule first */
        cur = prev = s->absolute;
        while( (NULL != cur) && (unixtime < cur->time) ) {
            prev = cur;
            cur = cur->next;
        }

        if( NULL != prev ) {
            /* In the absolute schedule */
            rv = convert_index_to_string( s, prev->block_count, prev->block );
            prune_expired_events( s, unixtime );
        } else {
            /* Check the weekly schedule */
            cur = prev = s->weekly;
            while( (NULL != cur) && (weekly < cur->time) ) {
                prev = cur;
                cur = cur->next;
            }

            if( NULL != prev ) {
                /* In the weekly schedule */
                rv = convert_index_to_string( s, prev->block_count, prev->block );
            }
        }
    }

    return rv;
}

/**
 *  Creates the schedule's table of mac addresses.
 *
 *  @param t the schedule to work with
 *  @param count the size of the table to allocate
 *
 *  @return 0 on success, failure otherwise
 */
int create_mac_table( schedule_t *s, size_t count )
{
    s->macs = (mac_address*) malloc( count * sizeof(mac_address) );
    if( NULL == s->macs ) {
        return -1;
    }
    s->mac_count = count;

    memset( s->macs, 0, count * sizeof(mac_address) );

    return 0;
}

/**
 *  Validates that the MAC address is in the expected format.
 *
 *  @param mac the MAC address to validate
 *  @param len the length of the mac string
 *
 *  @return true if valid, false otherwise
 */
bool validate_mac( const char *mac, size_t len )
{
    bool rv;

    rv = false;
    if( (NULL != mac) && (17 == len) ) {
        int mask = 0;

        mask |= ! isxdigit(mac[0]);
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

        if( 0 == mask ) {
            rv = true;
        }
    }

    return rv;
}

bool set_mac_index( schedule_t *s, const char *mac, size_t len, uint32_t index )
{
    bool rv;

    rv = false;
    if( (NULL != s) && (index < s->mac_count) ) {
        rv = validate_mac( mac, len );
        if( true == rv ) {
            memcpy( &s->macs[index].mac[0], mac, len );
            s->macs[index].mac[len] = '\0';
        }
    }

    return rv;
}

#if 0
uint8_t *extract_mac_addresses_for_time_window(schedule_t *t, int relative_time, int abs_time) {
    uint8_t *cp = NULL;
    struct tm calendar_time;
    time_t time_now = time(NULL);

    (void ) t; (void ) relative_time; (void ) abs_time;
    
    if (NULL == localtime_r(&time_now, &calendar_time)) {
        return cp;
    }

    return cp;
}
#endif
