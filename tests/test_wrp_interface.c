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
static test_t tests[] = {
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
        .r.u.req.payload = "Some other binary",
        .r.u.req.payload_size = 16,
    },
};
static uint8_t i;

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
int process_message_cu( wrp_msg_t *msg, wrp_msg_t *object )
{
    (void) msg; (void) object;
    return 0;
}

ssize_t process_message_ret( wrp_msg_t *msg, void **data )
{
    (void) msg; (void) data;
    return 0;
}

ssize_t process_request_set( wrp_msg_t *msg )
{
    (void) msg;
    return 0;
}

ssize_t process_request_get( wrp_msg_t *resp )
{
    resp->u.req.payload = malloc(tests[i].r.u.req.payload_size);
    memcpy(resp->u.req.payload, tests[i].r.u.req.payload, tests[i].r.u.req.payload_size);
    resp->u.req.payload_size = tests[i].r.u.req.payload_size;
    return resp->u.req.payload_size;
}

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_wrp_processing()
{
    size_t t_size = sizeof(tests)/sizeof(test_t);

    for( i = 0; i < t_size; i++ ) {
        wrp_msg_t msg;
        int rv = wrp_process(&(tests[i].s), &msg);
        CU_ASSERT(0 == rv);
        CU_ASSERT(tests[i].r.msg_type == msg.msg_type);
        CU_ASSERT_STRING_EQUAL(tests[i].r.u.req.transaction_uuid, msg.u.req.transaction_uuid);
        CU_ASSERT_STRING_EQUAL(tests[i].r.u.req.source, msg.u.req.source);
        CU_ASSERT_STRING_EQUAL(tests[i].r.u.req.dest, msg.u.req.dest);
        CU_ASSERT(0 == memcmp(tests[i].r.u.req.payload, msg.u.req.payload, msg.u.req.payload_size));
        CU_ASSERT(tests[i].r.u.req.payload_size == msg.u.req.payload_size);
        wrp_cleanup(&msg);
    }
}

void add_suites( CU_pSuite *suite )
{
    printf("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test 1", test_wrp_processing );
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
