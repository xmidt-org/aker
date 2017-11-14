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

#include <msgpack.h>

#include "schedule.h"
#include "decode.h"
#include "aker_log.h"


#define WEEKLY_SCHEDULE   "weekly"
#define MACS              "macs"
#define ABSOLUTE_SCHEDULE "absolute"
#define RELATIVE_TIME_STR "time"
#define UNIX_TIME_STR     "unix_time"
#define INDEXES_STR       "indexes"

#define UNPACKED_BUFFER_SIZE 2048
char unpacked_buffer[UNPACKED_BUFFER_SIZE];

static int decode_schedule_table   (msgpack_object *key, msgpack_object *val, schedule_event_t **t);
static int decode_macs_table       (msgpack_object *key, msgpack_object *val, schedule_t **t);

static int process_map(msgpack_object_map *, schedule_event_t **t);

/* Return true on match of key->via.str.ptr of size key->via.str.size */
static bool name_match(msgpack_object *key, const char *name);

int decode_schedule(size_t len, uint8_t * buf, schedule_t **t) {
    int ret_val = 0;
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_unpack_return ret;
    schedule_t *s;

    if (NULL == t || NULL == buf) {
        return -1;
    }
    
    debug_print("decode_schedule - calling create_schedule\n");
    s = create_schedule();
    *t = s; 
    
    if (NULL == s) {
        return -2;
    }
    
    debug_print("decode_schedule - msgpack_unpacked_init\n");
    msgpack_unpacked_init(&result);
    ret = msgpack_unpack_next(&result, (char *) buf, len, &off);
    
    if (0 == off) {
        destroy_schedule(s);
        *t = NULL;
        return -3;
    }
    
    while (ret == MSGPACK_UNPACK_SUCCESS) {
        debug_print("decode_schedule - MSGPACK_UNPACK_SUCCESS\n");
        msgpack_object obj = result.data;
        if (obj.type == MSGPACK_OBJECT_MAP) {
            debug_print("decode_schedule - MSGPACK_OBJECT_MAP\n");
            msgpack_object_map *map = &obj.via.map;
            msgpack_object_kv* p = map->ptr;
            int size = map->size;
            msgpack_object *key = &p->key;
            msgpack_object *val = &p->val;
            
            while (size-- > 0) {
            if (0 == strncmp(key->via.str.ptr, WEEKLY_SCHEDULE, key->via.str.size)) {
                debug_info("Found %s\n", WEEKLY_SCHEDULE);
                decode_schedule_table(key, val, &s->weekly);
                
                }
            else if (0 == strncmp(key->via.str.ptr, ABSOLUTE_SCHEDULE, key->via.str.size)) {
                debug_info("Found %s\n", ABSOLUTE_SCHEDULE);
                decode_schedule_table(key, val, &s->absolute);
                }
            else  if (0 == strncmp(key->via.str.ptr, MACS, key->via.str.size)) {
                    debug_info("Found %s\n", MACS);
                    if (0 != decode_macs_table(key, val, &s)) {
                        debug_error("decode_schedule():decode_macs_table() failed\n");
                        if (s->macs) {
                            free(s->macs);
                            s->macs = NULL;
                        }
                    }
                } else {
                    debug_error("decode_schedule() can't handle object type\n");
                    // ret_val = -4;
                }
            p++;
            key = &p->key;
            val = &p->val;
            }

        ret = msgpack_unpack_next(&result, (char *) buf, len, &off);
    } else {
            debug_error("Unexpected result in decode_schedule()\n");
            ret_val = -5;
            break;
    }

        if (ret == MSGPACK_UNPACK_CONTINUE) {
            debug_info("All msgpack_object in the buffer is consumed.\n");
        }
        else if (ret == MSGPACK_UNPACK_PARSE_ERROR) {
            debug_error("The data in the buf is invalid format.\n");
            ret_val = -6;
            break;
        }

        msgpack_unpacked_destroy(&result);
    }

    if (0 != ret_val) {
        msgpack_unpacked_destroy(&result);
        destroy_schedule(s);
       *t = NULL;
    }

    return ret_val;
}


int decode_schedule_table (msgpack_object *key, msgpack_object *val, schedule_event_t **t)
{
   (void ) key;
    if (val->type == MSGPACK_OBJECT_ARRAY) {
        msgpack_object *ptr = val->via.array.ptr;
        int count = val->via.array.size; 
        int i;
        schedule_event_t *temp = NULL;
        
        if (count <= 0) {
            return -1;
        }

        if (ptr->type == MSGPACK_OBJECT_MAP) {
            for (i = 0; i < count; i++) {
                if (0 == process_map(&ptr->via.map, &temp)) {
                    insert_event(t, temp);
                }
                ptr++;
           }
        }
    }
    return 0;    
}

int decode_macs_table (msgpack_object *key, msgpack_object *val, schedule_t **t)
{
    uint32_t i;
    uint32_t count;
    msgpack_object *ptr = val->via.array.ptr;
    (void ) key;
    
    count = val->via.array.size;

    if (0 != create_mac_table( *t, count )) {
        debug_error("decode_macs_table(): create_mac_table() failed\n");
        return -2;
    }
    
    for (i =0; i < count;i++) {
        if (ptr->via.str.size < MAC_ADDRESS_SIZE) {
            if (0 != set_mac_index( *t, ptr->via.str.ptr, ptr->via.str.size, i )) {
                debug_error("decode_macs_table(): Invalid MAC address\n");
                return -3;
            }
        } else {
            debug_error("decode_macs_table() Invalid MAC Address Length\n");
            return -4;
        }
        ptr++;
    }
    
    return 0;    
}

int process_map(msgpack_object_map *map, schedule_event_t **t)
{
    uint32_t size = map->size;
    msgpack_object *key = &map->ptr->key;
    msgpack_object *val = &map->ptr->val;
    msgpack_object_kv *kv = map->ptr;
    uint32_t cnt;
    time_t entry_time = 0;
    int ret_val = 0;

    *t = NULL;

    for (cnt = 0;cnt < size; cnt++) {
        if (key->type == MSGPACK_OBJECT_STR && val->type == MSGPACK_OBJECT_POSITIVE_INTEGER
            && (name_match(key, UNIX_TIME_STR) || name_match(key, RELATIVE_TIME_STR))
           )
        {
           // char buf[64];
           // memset(buf, 0, 64);
           // memcpy(buf, key->via.str.ptr, key->via.str.size);
           // debug_info("Key val %s is %d\n", buf, (uint32_t ) val->via.u64);
            
            entry_time = val->via.u64;
            
        } else if (key->type == MSGPACK_OBJECT_STR && val->type == MSGPACK_OBJECT_NIL) {
            *t = create_schedule_event(0);
        } else if (key->type == MSGPACK_OBJECT_STR && val->type == MSGPACK_OBJECT_ARRAY
                  && name_match(key, INDEXES_STR)
                  )
               {
                msgpack_object *ptr = val->via.array.ptr;
                uint32_t array_size = 0;

                *t = create_schedule_event(val->via.array.size);
                for (;array_size < (val->via.array.size); array_size++) {
                        (*t)->block[array_size] = ptr->via.u64;
                        debug_info("Array Element[%d] = %d block[] %d\n",
                                array_size, (uint32_t) ptr->via.u64, 
                                (*t)->block[array_size]);
                        ptr++;
                    }
        } else {
            debug_error("Unexpected Item in msgpack_object_map\n");
            ret_val = -1;
            break;
        }
        
        kv++;
        key = &kv->key;
        val = &kv->val;
    }

    if( NULL != *t ) {
        (*t)->time = entry_time;
    }
    printf("\n");
    return ret_val;
}


static bool name_match(msgpack_object *key, const char *name)
{
  bool result = (0 == strncmp(key->via.str.ptr, name, key->via.str.size));

  return result;
}
