/**
 *  Copyright 2017 Comcast Cable Communications Management, LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include <CUnit/Basic.h>

#include "../src/process_data.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    char      *macs;
    time_t    ts;
    size_t    msgpack_size;
    uint8_t   msgpack[255];
} test_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static test_t tests_now[] =
{
    { /* Index 0 */
        .macs = "33:44:55:66:aa:BB 22:33:44:55:66:aa",
        .ts = 1233999,
        .msgpack_size = 55,
        .msgpack = {
            0x82, /* 2 name value pairs */

            0xa6, /* "active" */
            'a', 'c', 't', 'i', 'v', 'e',
            0xd9, 0x23, /* "33:44:55:66:aa:BB 22:33:44:55:66:aa" */
            '3', '3', ':', '4', '4', ':', '5', '5', ':', '6', '6', ':', 'a', 'a', ':', 'B', 'B', ' ', 
            '2', '2', ':', '3', '3', ':', '4', '4', ':', '5', '5', ':', '6', '6', ':', 'a', 'a',

            0xa4, /* "time" */
            't', 'i', 'm', 'e',
            0xce, /* 1233999 */
            0x00, 0x12, 0xd4, 0x4f,
        },
    },

    { /* Index 1 */
        .macs = "33:44:55:66:aa:BB",
        .ts = 1234000,
        .msgpack_size = 36,
        .msgpack = {
            0x82, /* 2 name value pairs */

            0xa6, /* "active" */
            'a', 'c', 't', 'i', 'v', 'e',
            0xb1, /* "33:44:55:66:aa:BB" */
            '3', '3', ':', '4', '4', ':', '5', '5', ':', '6', '6', ':', 'a', 'a', ':', 'B', 'B',

            0xa4, /* "time" */
            't', 'i', 'm', 'e',
            0xce, /* 12340000 */
            0x00, 0x12, 0xd4, 0x50,
        },      
    },

    { /* Index 2 */
        .macs = "11:22:33:44:55:66 22:33:44:55:66:aa",
        .ts = 1234001,
        .msgpack_size = 55,
        .msgpack = {
            0x82, /* 2 name value pairs */

            0xa6, /* "active" */
            'a', 'c', 't', 'i', 'v', 'e',
            0xd9, 0x23, /* "11:22:33:44:55:66 22:33:44:55:66:aa" */
            '1', '1', ':', '2', '2', ':', '3', '3', ':', '4', '4', ':', '5', '5', ':', '6', '6', ' ', 
            '2', '2', ':', '3', '3', ':', '4', '4', ':', '5', '5', ':', '6', '6', ':', 'a', 'a',

            0xa4, /* "time" */
            't', 'i', 'm', 'e',
            0xce, /* 12340001 */
            0x00, 0x12, 0xd4, 0x51,
        },
    },

    { /* Index 3 */
        .macs = "11:22:33:44:55:66 33:44:55:66:aa:BB 22:33:44:55:66:aa",
        .ts = 1234010,
        .msgpack_size = 73,
        .msgpack = {
            0x82, /* 2 name value pairs */

            0xa6, /* "active" */
            'a', 'c', 't', 'i', 'v', 'e',
            0xd9, 0x35, /* "11:22:33:44:55:66 33:44:55:66:aa:BB 22:33:44:55:66:aa" */
            '1', '1', ':', '2', '2', ':', '3', '3', ':', '4', '4', ':', '5', '5', ':', '6', '6', ' ', 
            '3', '3', ':', '4', '4', ':', '5', '5', ':', '6', '6', ':', 'a', 'a', ':', 'B', 'B', ' ',
            '2', '2', ':', '3', '3', ':', '4', '4', ':', '5', '5', ':', '6', '6', ':', 'a', 'a',

            0xa4, /* "time" */
            't', 'i', 'm', 'e',
            0xce, /* 12340010 */
            0x00, 0x12, 0xd4, 0x5a,
        },
    },
};

static uint8_t i;

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
#define MD5_SIZE 1

time_t get_unix_time(void)
{
    return tests_now[i].ts;
}

char *get_current_blocked_macs( void )
{
    return strdup(tests_now[i].macs);
}

unsigned char *compute_byte_stream_md5(uint8_t *data, size_t length,
                                   unsigned char result[MD5_SIZE])
{
    (void) data; (void) length; (void) result;
    return NULL;
}

int process_schedule_data( size_t len, uint8_t *data )
{
    (void) len; (void) data;
    return 1;
}

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_process_ret_now()
{   
    size_t t_size = sizeof(tests_now)/sizeof(test_t);
    size_t ret_size = 0;
    uint8_t *data = NULL;

    for( i = 0; i < t_size; i++ ) {
        ret_size = process_retrieve_now(&data);
        CU_ASSERT(ret_size == tests_now[i].msgpack_size);
        CU_ASSERT(0 == memcmp(data, tests_now[i].msgpack, tests_now[i].msgpack_size));
        free(data);
        data = NULL;
    }
}

void add_suites( CU_pSuite *suite )
{
    printf("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test 1", test_process_ret_now );
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main( void )
{
    unsigned rv = 1;
    CU_pSuite suite = NULL;

    if( CUE_SUCCESS == CU_initialize_registry() ) {
        add_suites( &suite );

        if( NULL != suite ) {
            CU_basic_set_mode( CU_BRM_VERBOSE );
            CU_basic_run_tests();
            printf( "\n" );
            CU_basic_show_failures( CU_get_failure_list() );
            printf( "\n\n" );
            rv = CU_get_number_of_tests_failed();
        }

        CU_cleanup_registry();

    }

    return rv;
}

