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
#include <stdint.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
struct schedule_event {
    uint32_t start;                 /* Time is either minutes since last sunday
                                     * or UTC Unix time. */
    size_t block_count;             /* Number of mac addresses to block. */
    struct mac_address *block[];    /* The list of mac addresses to block.
                                     * DO NOT FREE THIS LIST. */
                                    
    struct schedule_event *next;    /* The next node in the SLL or NULL. */
};

struct mac_address {
    char mac[18];   /* MAC addresses stored/used: "11:22:33:44:55:66" */
}

typedef struct {
    uint32_t abs_start;                 /* UTC Unix time starting time for
                                         * the absolute rules. */
    uint32_t abs_end;                   /* UTC Unix time ending time for
                                         * the absolute rules. */
    struct schedule_event *absolute;    /* The absolute schedule to apply if
                                         * a matching time window is found. */

    struct schedule_event *reoccuring;  /* The list of re-occuring rules to
                                         * apply until a new schedule is
                                         * acquired. */

    size_t mac_count;                   /* The count of the macs. */
    struct mac_address *macs;           /* The shared list of mac addresses to
                                         * block.  FREE THIS LIST. */
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
/* none */

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
