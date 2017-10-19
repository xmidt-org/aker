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

#include <stdbool.h>
#include <stdint.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

typedef struct schedule_event {
    uint32_t time;                  /* Time is either minutes since last sunday
                                     * or UTC Unix time. */
    struct schedule_event *next ;   /* The next node in the SLL or NULL. */
    
    size_t block_count;             /* Number of mac addresses to block. */
    uint32_t block[];               /* The list of mac addresses to block. */
} schedule_event_t;

typedef struct mac_address_t {
    char mac[18];                   /* MAC addresses stored/used: "11:22:33:44:55:66" */
} mac_address;

typedef struct schedule {
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
 *  Decodes the MsgPacked structure (bytes) into a new schedule object.
 *
 *  @param len  [in]  the number of bytes to process
 *  @param data [in]  the msgpack bytes to process
 *  @param s    [out] the resulting schedule struture, or untouched on error
 *
 *  @return 0 on success, error otherwise.
 */
int decode_schedule( size_t len, uint8_t *bytes, schedule_t **s );

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */

#endif
