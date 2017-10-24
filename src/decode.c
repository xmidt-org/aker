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

static int decode_schedule_table   (msgpack_object *key, msgpack_object *val, schedule_event_t **t);
static int decode_macs_table       (msgpack_object *key, msgpack_object *val, schedule_t **t);

static int process_map(msgpack_object_map *, schedule_event_t **t);

/* DEBUG ONLY */
char test_buffer[] = {
0x83, 0xAF, 0x77, 0x65,   0x65, 0x6B, 0x6C, 0x79,   0x2D, 0x73, 0x63, 0x68,
0x65, 0x64, 0x75, 0x6C,   0x65, 0x93, 0x82, 0xA4,   0x74, 0x69, 0x6D, 0x65,
0x0A, 0xA7, 0x69, 0x6E,   0x64, 0x65, 0x78, 0x65,   0x73, 0x93, 0x00, 0x01,
0x03, 0x82, 0xA4, 0x74,   0x69, 0x6D, 0x65, 0x14,   0xA7, 0x69, 0x6E, 0x64,
0x65, 0x78, 0x65, 0x73,   0x91, 0x00, 0x82, 0xA4,   0x74, 0x69, 0x6D, 0x65,
0x1E, 0xA7, 0x69, 0x6E,   0x64, 0x65, 0x78, 0x65,   0x73, 0x90, 0xA4, 0x6D,
0x61, 0x63, 0x73, 0x94,   0xB1, 0x31, 0x31, 0x3A,   0x32, 0x32, 0x3A, 0x33,
0x33, 0x3A, 0x34, 0x34,   0x3A, 0x35, 0x35, 0x3A,   0x61, 0x61, 0xB1, 0x32,
0x32, 0x3A, 0x33, 0x33,   0x3A, 0x34, 0x34, 0x3A,   0x35, 0x35, 0x3A, 0x36,
0x36, 0x3A, 0x62, 0x62,   0xB1, 0x33, 0x33, 0x3A,   0x34, 0x34, 0x3A, 0x35,
0x35, 0x3A, 0x36, 0x36,   0x3A, 0x37, 0x37, 0x3A,   0x63, 0x63, 0xB1, 0x34,
0x34, 0x3A, 0x35, 0x35,   0x3A, 0x36, 0x36, 0x3A,   0x37, 0x37, 0x3A, 0x38,
0x38, 0x3A, 0x64, 0x64,   0xB1, 0x61, 0x62, 0x73,   0x6F, 0x6C, 0x75, 0x74,
0x65, 0x2D, 0x73, 0x63,   0x68, 0x65, 0x64, 0x75,   0x6C, 0x65, 0x91, 0x82,
0xA9, 0x75, 0x6E, 0x69,   0x78, 0x2D, 0x74, 0x69,   0x6D, 0x65, 0xCE, 0x59,
0xE5, 0x83, 0x17, 0xA7,   0x69, 0x6E, 0x64, 0x65,   0x78, 0x65, 0x73, 0x92,
0x00, 0x02
};
void debug_code(void)
{
    schedule_t *t;
    decode_schedule(sizeof(test_buffer), (uint8_t *) test_buffer, &t);
}

/* END DEBUG ONLY */

int decode_schedule(size_t len, uint8_t * buf, schedule_t **t) {
    /* buf is allocated by client. */
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_unpack_return ret;
    schedule_t *s;
    
    if (NULL == t || NULL == buf) {
        return -1;
    }
    
    s = create_schedule();
    *t = s; 
    
    if (NULL == s) {
        return -1;
    }
    
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
                decode_schedule_table(key, val, &s->weekly);
                
                }
            else if (0 == strncmp(key->via.str.ptr, ABSOLUTE_SCHEDULE, key->via.str.size)) {
                printf("Found %s\n", ABSOLUTE_SCHEDULE);
                decode_schedule_table(key, val, &s->absolute);
                }
            else  if (0 == strncmp(key->via.str.ptr, MACS, key->via.str.size)) {
                printf("Found %s\n", MACS);
                decode_macs_table(key, val, &s);
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
                process_map(&ptr->via.map, &temp);
                ptr++;
                insert_event(t, temp);
           }
        }
    }
    return 0;    
}

int decode_macs_table (msgpack_object *key, msgpack_object *val, schedule_t **t)
{
    uint32_t i;
    msgpack_object *ptr = val->via.array.ptr;
    (void ) key;
    (void ) t;
    
    for (i =0; i < (val->via.array.size);i++) {
        char buf[128];
        memset(buf, 0, 128);
        memcpy(buf, ptr->via.str.ptr, ptr->via.str.size);
        printf("MAC Table index %u is %s\n", i, buf);
        ptr++;
    }
    
    return -1;    
}

int process_map(msgpack_object_map *map, schedule_event_t **t)
{
    uint32_t size = map->size;
    msgpack_object *key = &map->ptr->key;
    msgpack_object *val = &map->ptr->val;
    msgpack_object_kv *kv = map->ptr;
    uint32_t cnt;
    time_t entry_time = 0;
    
    for (cnt = 0;cnt < size; cnt++) {
        if (key->type == MSGPACK_OBJECT_STR && val->type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            char buf[64];
            memset(buf, 0, 64);
             memcpy(buf, key->via.str.ptr, key->via.str.size);
             entry_time = val->via.u64;
             printf("Key val %s is %d\n", buf, (uint32_t ) val->via.u64);
        } else if (key->type == MSGPACK_OBJECT_STR && val->type == MSGPACK_OBJECT_ARRAY) {
            msgpack_object *ptr = val->via.array.ptr;
            uint32_t array_size = 0;
            *t = create_schedule_event(val->via.array.size);
            for (;array_size < (val->via.array.size); array_size++) {
                (*t)->block[array_size] = ptr->via.u64;
                printf("Array Element[%d] = %d block[] %d\n", array_size, (uint32_t) ptr->via.u64, (*t)->block[array_size]);
                ptr++;
            }
            
            printf("\n");
            (void ) ptr;(void ) array_size;
        }
        
        kv++;
        key = &kv->key;
        val = &kv->val;
    }
    
    (*t)->time = entry_time;
    printf("\n");
    (void ) size; (void ) key; (void ) val;
    return 1;
}
