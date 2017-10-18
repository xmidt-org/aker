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
#include <msgpack.h>

#include "schedule.h"

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
/* none */

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

/* See schedule.h for details. */
int decode_schedule( size_t count, uint8_t *bytes, schedule_t **s )
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

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
