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

static void decodeRequest(msgpack_object *deserialized, schedule_t *);

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
    
    switch (unpack_ret) {
        case MSGPACK_UNPACK_SUCCESS:
                if( deserialized.via.map.size != 0 ) {
                    decodeRequest( &deserialized, schedule );
                }
                msgpack_zone_destroy( &mempool );
                break;
                
        default:
            free(schedule);
            return -3;
    }    
    
    return 0;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

void decodeRequest(msgpack_object *deserialized, schedule_t *schedule)
{
    msgpack_object_kv* p = deserialized->via.map.ptr;

    while( deserialized->via.map.size--) {
        //msgpack_object keyType = p->key;
        //msgpack_object ValueType = p->val;
        // key name keyType.via.str.ptr
        switch (p->val.type) {
            case MSGPACK_OBJECT_ARRAY:
                /*
                    msgpack_object_array array = ValueType.via.array;
                    msgpack_object *ptr = array.ptr;
                    int num_elements = array.size;
                 
                    if (!strcmp(p->key.via.str.ptr, "weekly-schedule")) {
                 
                 } else next one "macs, and "absolute-schedule"
                 */
                break;
                
            default:
                break;
        }
        
    }
    
}
