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

#include "test_macros.h"
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
/* None */

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
ssize_t process_create( const char *filename, const char *md5_file, wrp_msg_t *msg )
{
    (void) filename; (void) md5_file; (void) msg;
    return 0;
}

ssize_t process_update( const char *filename, const char *md5_file, wrp_msg_t *msg )
{
    (void) filename; (void) md5_file; (void) msg;
    return 0;
}

ssize_t process_retrieve_persistent( const char *filename, wrp_msg_t *msg )
{
    (void) filename; (void) msg;
    return 0;
}

ssize_t process_retrieve_now( wrp_msg_t *msg )
{
    (void) msg;
    return 0;
}

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_process_wrp()
{
    test_t tests[] = {
        {
            .s.msg_type = WRP_MSG_TYPE__CREATE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = SCHEDULE_ENDPOINT,
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = "Some binary",
            .s.u.crud.payload_size = 11,

            .r.msg_type = WRP_MSG_TYPE__CREATE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = SCHEDULE_ENDPOINT,
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 201,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = NULL,
            .r.u.crud.payload_size = 0,
        },

        {
            .s.msg_type = WRP_MSG_TYPE__CREATE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = SCHEDULE_ENDPOINT,
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = "Some binary",
            .s.u.crud.payload_size = 11,

            .r.msg_type = WRP_MSG_TYPE__CREATE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = SCHEDULE_ENDPOINT,
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 422,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = NULL,
            .r.u.crud.payload_size = 0,
        },

        {
            .s.msg_type = WRP_MSG_TYPE__CREATE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = SCHEDULE_ENDPOINT,
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = "Some binary",
            .s.u.crud.payload_size = 11,

            .r.msg_type = WRP_MSG_TYPE__CREATE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = SCHEDULE_ENDPOINT,
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 400,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = NULL,
            .r.u.crud.payload_size = 0,
        },

        {
            .s.msg_type = WRP_MSG_TYPE__UPDATE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = SCHEDULE_ENDPOINT,
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.spans.count = 0,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = "Some binary",
            .s.u.crud.payload_size = 11,

            .r.msg_type = WRP_MSG_TYPE__UPDATE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = SCHEDULE_ENDPOINT,
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 200,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = NULL,
            .r.u.crud.payload_size = 0,
        },

        {
            .s.msg_type = WRP_MSG_TYPE__RETREIVE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = PERSISTENT_SCHEDULE_ENDPOINT,
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.spans.count = 0,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = NULL,
            .s.u.crud.payload_size = 0,

            .r.msg_type = WRP_MSG_TYPE__RETREIVE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = PERSISTENT_SCHEDULE_ENDPOINT,
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 204,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = "Some other binary",
            .r.u.crud.payload_size = 16,
        },

        {
            .s.msg_type = WRP_MSG_TYPE__RETREIVE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = PERSISTENT_MD5_ENDPOINT,
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.spans.count = 0,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = NULL,
            .s.u.crud.payload_size = 0,

            .r.msg_type = WRP_MSG_TYPE__RETREIVE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = PERSISTENT_MD5_ENDPOINT,
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 200,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = "Some other binary",
            .r.u.crud.payload_size = 16,
        },

        {
            .s.msg_type = WRP_MSG_TYPE__RETREIVE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = NOW_ENDPOINT,
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.spans.count = 0,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = NULL,
            .s.u.crud.payload_size = 0,

            .r.msg_type = WRP_MSG_TYPE__RETREIVE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = NOW_ENDPOINT,
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 200,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = "Some other binary",
            .r.u.crud.payload_size = 16,
        },

        {
            .s.msg_type = WRP_MSG_TYPE__DELETE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = NOW_ENDPOINT,
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.spans.count = 0,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = NULL,
            .s.u.crud.payload_size = 0,

            .r.msg_type = WRP_MSG_TYPE__DELETE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = NOW_ENDPOINT,
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 405,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = NULL,
            .r.u.crud.payload_size = 0,
        },

        {
            .s.msg_type = WRP_MSG_TYPE__REQ,
            .s.u.req.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.req.source = "fake-server",
            .s.u.req.dest = PERSISTENT_MD5_ENDPOINT,
            .s.u.req.partner_ids = NULL,
            .s.u.req.headers = NULL,
            .s.u.req.metadata = NULL,
            .s.u.req.include_spans = false,
            .s.u.req.spans.spans = NULL,
            .s.u.req.spans.count = 0,
            .s.u.req.payload = NULL,
            .s.u.req.payload_size = 0,

            .r.msg_type = WRP_MSG_TYPE__REQ,
            .r.u.req.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.req.source = PERSISTENT_MD5_ENDPOINT,
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
    size_t t_size = sizeof(tests)/sizeof(test_t);
    uint8_t i;

    for( i = 0; i < t_size; i++ ) {
        wrp_msg_t msg;
        int rv = process_wrp("data", "md5", &(tests[i].s), &msg);
        CU_ASSERT(0 <= rv);
        CU_ASSERT(tests[i].r.msg_type == msg.msg_type);
        if( WRP_MSG_TYPE__REQ == msg.msg_type ) {
            CU_ASSERT_STRING_EQUAL(tests[i].r.u.req.transaction_uuid, msg.u.req.transaction_uuid);
            CU_ASSERT_STRING_EQUAL(tests[i].r.u.req.source, msg.u.req.source);
            CU_ASSERT_STRING_EQUAL(tests[i].r.u.req.dest, msg.u.req.dest);
        } else {
            CU_ASSERT_STRING_EQUAL(tests[i].r.u.crud.transaction_uuid, msg.u.crud.transaction_uuid);
            CU_ASSERT_STRING_EQUAL(tests[i].r.u.crud.source, msg.u.crud.source);
            CU_ASSERT_STRING_EQUAL(tests[i].r.u.crud.dest, msg.u.crud.dest);
            CU_ASSERT(0 == strcmp(tests[i].r.u.crud.path, msg.u.crud.path));
        }
        cleanup_wrp(&msg);
    }
}

void add_suites( CU_pSuite *suite )
{
    printf("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test 1", test_process_wrp );
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
