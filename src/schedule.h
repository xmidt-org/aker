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
#ifndef __SCHEDULE_H__
#define __SCHEDULE_H__

#include <stdint.h>
#include <time.h>
#include <pthread.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define MAC_ADDRESS_SIZE         18
#define MAXIMUM_BLOCKED_MAC_LIST 128
#define MAX_TIME_ZONE_SIZE       128

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

typedef struct schedule_event {
    time_t time;                    /* Time is either minutes since last sunday
                                     * or UTC Unix time. */
    struct schedule_event *next ;   /* The next node in the SLL or NULL. */
    
    size_t block_count;             /* Number of mac addresses to block. */
    uint32_t block[];               /* The list of mac addresses to block. */
} schedule_event_t;


typedef struct mac_address_t {
    char mac[MAC_ADDRESS_SIZE];    /* MAC addresses                    */ 
                                   /* stored/used: "11:22:33:44:55:66" */
} mac_address;

typedef struct time_zone_t {
    char tz[MAX_TIME_ZONE_SIZE];    /* time_zone string                   */
} time_zone;

typedef struct schedule {
    time_zone               tz_struct;
    schedule_event_t *absolute;     /* The absolute schedule to apply if
                                     * a matching time window is found. */

    schedule_event_t *weekly;       /* The list of re-occuring rules to apply
                                     * until a new schedule is acquired. */

    size_t mac_count;               /* The count of the macs. */
    mac_address *macs;              /* The shared list of mac addresses to block. */
} schedule_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/


/**
 *  Create an empty schedule.
 *
 *  @return NULL on error, valid pointer to a schedule_t otherwise
 */
schedule_t* create_schedule( void );


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
schedule_event_t* create_schedule_event( size_t block_count );


/**
 *  Inserts a schedule_event_t in sorted order (smallest to largest) into
 *  the specified list (head).
 *
 *  @param head the pointer to the list head
 *  @param e    the schedule_event_t pointer to add to the list
 */
void insert_event(schedule_event_t **head, schedule_event_t *e );


/**
 *  Performs the tasks needed to make the scheduler's job a bit easier.
 *
 *  @param s the schedule to finalize
 */
int finalize_schedule( schedule_t *s );


/**
 *  Destroys the schedule passed in.
 *
 *  @param s the schedule to destroy
 */
void destroy_schedule( schedule_t *s );


/**
 *  Gets the string with the blocked MAC addresses at this time.
 *
 *  @param s        the schedule to apply
 *  @param unixtime the unixtime representation
 *
 *  @return the string with the list of blocked addresses (may be NULL and valid)
 */
char* get_blocked_at_time( schedule_t *s, time_t unixtime );


/**
 *  Creates the schedule's table of mac addresses.
 *
 *  @param t the schedule to work with
 *  @param count the size of the table to allocate
 *
 *  @return 0 on success, failure otherwise
 */
int create_mac_table( schedule_t *s, size_t count );


/**
 *  Deep copy a schedule entry so it can be altered and used easily.
 *
 *  @param e the event to deep copy
 *
 *  @return NULL on error, valid pointer to a schedule_event_t otherwise
 */
schedule_event_t* copy_schedule_event( schedule_event_t *e );


/**
 *  Validates and copies the MAC address passed in into the right location in
 *  the schedule.
 *
 *  @param s   the schedule to alter
 *  @param mac the MAC address to check and copy
 *  @param len the length of the MAC address passed in
 *  @param index the location to copy into
 *
 *  @return 0 if successful, failure otherwise
 */
int set_mac_index( schedule_t *s, const char *mac, size_t len, uint32_t index );


/**
 *  Prints the schedule object out to stdout.
 *
 *  @param s the schedule to print
 */
void print_schedule( schedule_t *s );

/**
 * Find the next imminent event and return its time since Epoch.
 *
 * @param s        schedule
 * @param unixtime the unixtime representation.
 *
 * @return the Epoch time of next imminent schedule event
 */
time_t get_next_unixtime(schedule_t *s, time_t unixtime);

#endif
