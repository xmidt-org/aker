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

/* The test message
{
    "weekly-schedule": [
        { "time": 10, "indexes": [0, 1, 3] },
        { "time": 20, "indexes": [0] },
        { "time": 30, "indexes": [] }
    ],

    "macs": [
        "11:22:33:44:55:aa",
        "22:33:44:55:66:bb",
        "33:44:55:66:77:cc",
        "44:55:66:77:88:dd"
    ],

    "absolute-schedule": [
        { "unix-time": 1508213527, "indexes": [0, 2] }
    ]
}
*/

#define WEEKLY_SCHEDULE   "weekly-schedule"
#define MACS              "macs"
#define ABSOLUTE_SCHEDULE "absolute-schedule"

#define UNPACKED_BUFFER_SIZE 2048
char unpacked_buffer[UNPACKED_BUFFER_SIZE];

static int decode_schedule_table   (msgpack_object *key, msgpack_object *val);
static int decode_macs_table       (msgpack_object *key, msgpack_object *val);

static void process_map(msgpack_object_map *);

int decode_schedule(size_t len, uint8_t * buf, schedule_t **t) {
    /* buf is allocated by client. */
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_unpack_return ret;
    
    (void ) t; // Temp Code  !!!
    
    msgpack_unpacked_init(&result);
    ret = msgpack_unpack_next(&result, (char *) buf, len, &off);
    while (ret == MSGPACK_UNPACK_SUCCESS) {
        msgpack_object obj = result.data;
        if (obj.type == MSGPACK_OBJECT_MAP) {
            msgpack_object_map *map = &obj.via.map;
            msgpack_object_kv* p = map->ptr;
            int size = map->size;
            msgpack_object *key = &p->key;
            msgpack_object *val = &p->val;
            
            while (size-- > 0) {
            if (0 == strncmp(key->via.str.ptr, WEEKLY_SCHEDULE, key->via.str.size)) {
                printf("Found %s\n", WEEKLY_SCHEDULE);
                decode_schedule_table(key, val);
                }
            else if (0 == strncmp(key->via.str.ptr, ABSOLUTE_SCHEDULE, key->via.str.size)) {
                printf("Found %s\n", ABSOLUTE_SCHEDULE);
                decode_schedule_table(key, val);
                }
            else  if (0 == strncmp(key->via.str.ptr, MACS, key->via.str.size)) {
                printf("Found %s\n", MACS);
                decode_macs_table(key, val);
                } else {
                // zztop handle_decode_error();
                }
            p++;
            key = &p->key;
            val = &p->val;
            }

        ret = msgpack_unpack_next(&result, (char *) buf, len, &off);
    }
        msgpack_unpacked_destroy(&result);

        if (ret == MSGPACK_UNPACK_CONTINUE) {
            printf("All msgpack_object in the buffer is consumed.\n");
        }
        else if (ret == MSGPACK_UNPACK_PARSE_ERROR) {
            printf("The data in the buf is invalid format.\n");
        }
    }
    return 0;
}


int decode_schedule_table (msgpack_object *key, msgpack_object *val)
{
    (void ) key;
    
    if (val->type == MSGPACK_OBJECT_ARRAY) {
       msgpack_object *ptr = val->via.array.ptr;
       int count = val->via.array.size; 
        for (;count > 0; count--) {
            if (ptr->type == MSGPACK_OBJECT_MAP) {
                
                for (;count > 0; count--) {
                    process_map(&ptr->via.map);
                    ptr++;
                }
            }
        }
    }
    
    return -1;    
}

int decode_macs_table (msgpack_object *key, msgpack_object *val)
{
    uint32_t i;
    msgpack_object *ptr = val->via.array.ptr;
    (void ) key;
    
    for (i =0; i < (val->via.array.size);i++) {
        char buf[128];
        memset(buf, 0, 128);
        memcpy(buf, ptr->via.str.ptr, ptr->via.str.size);
        printf("MAC Table index %d is %s\n", i, buf);
        ptr++;
    }
    
    return -1;    
}

void process_map(msgpack_object_map *map)
{
    uint32_t size = map->size;
    msgpack_object *key = &map->ptr->key;
    msgpack_object *val = &map->ptr->val;
    msgpack_object_kv *kv = map->ptr;
    uint32_t cnt;
    
    for (cnt = 0;cnt < size; cnt++) {
        if (key->type == MSGPACK_OBJECT_STR && val->type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            char buf[64];
            memset(buf, 0, 64);
            memcpy(buf, key->via.str.ptr, key->via.str.size);
            printf("Key val %s is %d\n", buf, (uint32_t ) val->via.u64);
        } else if (key->type == MSGPACK_OBJECT_STR && val->type == MSGPACK_OBJECT_ARRAY) {
            msgpack_object *ptr = val->via.array.ptr;
            uint32_t array_size = 0;
            
            for (;array_size < (val->via.array.size); array_size++) {
                printf("Array Element[%d] = %d\n", array_size, (uint32_t) ptr->via.u64);
                ptr++;
            }
            
            printf("\n");
            (void ) ptr;(void ) array_size;
        }
        kv++;
        key = &kv->key;
        val = &kv->val;
    }
    printf("\n");
    (void ) size; (void ) key; (void ) val;
}
