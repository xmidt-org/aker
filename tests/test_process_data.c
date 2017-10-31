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
        .r.u.req.payload = "Some binary",
        .r.u.req.payload_size = 11,
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
        set_size = process_request_set("pcs.bin", &tests_set[i].s);
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
        get_size = process_request_get("pcs.bin", &response);
        CU_ASSERT(0 == memcmp(tests_get[i].r.u.req.payload, response.u.req.payload, response.u.req.payload_size));
        CU_ASSERT(tests_get[i].r.u.req.payload_size == response.u.req.payload_size);
        free(response.u.req.payload);
        CU_ASSERT((size_t)get_size == tests_get[i].r.u.req.payload_size);
    }
}

void add_suites( CU_pSuite *suite )
{
    printf("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test 1", test_process_request_set );
    CU_add_test( *suite, "Test 2", test_process_request_get );
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

