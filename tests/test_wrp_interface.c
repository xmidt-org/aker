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
/* None */

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
ssize_t process_message_cu( const char *filename, const char *md5, wrp_msg_t *cu )
{
    (void) filename; (void) md5; (void) cu;
    return 1;
}

ssize_t process_message_ret( const char *filename, wrp_msg_t *ret )
{
    (void) filename; (void) ret;
    return 1;
}

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_wrp_processing()
{
    test_t tests[] = {
        {
            .s.msg_type = WRP_MSG_TYPE__CREATE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = "/parental-control/schedule",
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.spans.count = 0,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = "Some binary",
            .s.u.crud.payload_size = 11,

            .r.msg_type = WRP_MSG_TYPE__CREATE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = "/parental-control/schedule",
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = NULL,
            .r.u.crud.payload_size = 0,
        },

        {
            .s.msg_type = WRP_MSG_TYPE__RETREIVE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = "/parental-control/schedule",
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
            .r.u.crud.source = "/parental-control/schedule",
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = "Some other binary",
            .r.u.crud.payload_size = 16,
        },
    };
    size_t t_size = sizeof(tests)/sizeof(test_t);
    uint8_t i;

    for( i = 0; i < t_size; i++ ) {
        wrp_msg_t msg;
        int rv = wrp_process("data", "md5", &(tests[i].s), &msg);
        CU_ASSERT(0 < rv);
        CU_ASSERT(tests[i].r.msg_type == msg.msg_type);
        CU_ASSERT_STRING_EQUAL(tests[i].r.u.crud.transaction_uuid, msg.u.crud.transaction_uuid);
        CU_ASSERT_STRING_EQUAL(tests[i].r.u.crud.source, msg.u.crud.source);
        CU_ASSERT_STRING_EQUAL(tests[i].r.u.crud.dest, msg.u.crud.dest);
        CU_ASSERT(0 == strcmp(tests[i].r.u.crud.path, msg.u.crud.path));
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
