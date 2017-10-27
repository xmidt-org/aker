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
#include <wrp-c.h>

#include <CUnit/Basic.h>

#include "../src/wrp_interface.h"
#include "../src/process_data.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    wrp_msg_t s;
    wrp_msg_t r;
} test_t;

uint8_t decode_buffer[] = {
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

size_t decode_length = sizeof(decode_buffer);
#include "../src/schedule.h"
#include "../src/decode.h"
void decode_test()
{
    schedule_t *t;
    int ret = decode_schedule(decode_length, decode_buffer, &t);
    CU_ASSERT(0 == ret);
}

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static test_t tests_set[] =
{
    {
        .s.msg_type = WRP_MSG_TYPE__REQ,
        .s.u.req.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
        .s.u.req.source = "fake-server",
        .s.u.req.dest = "/iot", //"/parental control/schedule/set",
        .s.u.req.partner_ids = NULL,
        .s.u.req.headers = NULL,
        .s.u.req.metadata = NULL,
        .s.u.req.include_spans = false,
        .s.u.req.spans.spans = NULL,
        .s.u.req.spans.count = 0,
        .s.u.req.payload = "Some binary",
        .s.u.req.payload_size = 11,

        .r.msg_type = WRP_MSG_TYPE__REQ,
        .r.u.req.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
        .r.u.req.source = "/iot", //"/parental control/schedule/set",
        .r.u.req.dest = "fake-server",
        .r.u.req.partner_ids = NULL,
        .r.u.req.headers = NULL,
        .r.u.req.metadata = NULL,
        .r.u.req.include_spans = false,
        .r.u.req.spans.spans = NULL,
        .r.u.req.spans.count = 0,
        .r.u.req.payload = NULL,
        .r.u.req.payload_size = 0,
    },
};

static test_t tests_get[] =
{
    {
        .s.msg_type = WRP_MSG_TYPE__REQ,
        .s.u.req.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
        .s.u.req.source = "fake-server",
        .s.u.req.dest = "/iot", //"/parental control/schedule/get",
        .s.u.req.partner_ids = NULL,
        .s.u.req.headers = NULL,
        .s.u.req.metadata = NULL,
        .s.u.req.include_spans = false,
        .s.u.req.spans.spans = NULL,
        .s.u.req.spans.count = 0,
        .s.u.req.payload = REQ_GET,
        .s.u.req.payload_size = 44,

        .r.msg_type = WRP_MSG_TYPE__REQ,
        .r.u.req.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
        .r.u.req.source = "/iot", //"/parental control/schedule/get",
        .r.u.req.dest = "fake-server",
        .r.u.req.partner_ids = NULL,
        .r.u.req.headers = NULL,
        .r.u.req.metadata = NULL,
        .r.u.req.include_spans = false,
        .r.u.req.spans.spans = NULL,
        .r.u.req.spans.count = 0,
        .r.u.req.payload = "Some binary",
        .r.u.req.payload_size = 11,
    },
};

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_process_request_set()
{
    size_t t_size = sizeof(tests_set)/sizeof(test_t);
    ssize_t set_size;

    for( uint8_t i = 0; i < t_size; i++ ) {
        set_size = process_request_set(&tests_set[i].s);
        CU_ASSERT((size_t)set_size == tests_set[i].s.u.req.payload_size);
    }
}

void test_process_request_get()
{
    size_t t_size = sizeof(tests_set)/sizeof(test_t);
    wrp_msg_t response;
    ssize_t get_size;

    for( uint8_t i = 0; i < t_size; i++ ) {
        memset(&response, '\0', sizeof(wrp_msg_t));
        get_size = process_request_get(&response);
        CU_ASSERT(0 == memcmp(tests_get[i].r.u.req.payload, response.u.req.payload, response.u.req.payload_size));
        CU_ASSERT(tests_get[i].r.u.req.payload_size == response.u.req.payload_size);
        free(response.u.req.payload);
        CU_ASSERT((size_t)get_size == tests_get[i].r.u.req.payload_size);
        free(response.u.req.payload);
    }
}

void add_suites( CU_pSuite *suite )
{
    printf("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Decode Test", decode_test);
    CU_add_test( *suite, "Test 1", test_process_request_set );
    //CU_add_test( *suite, "Test 2", test_process_request_get );
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

