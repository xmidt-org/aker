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
        .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
        .s.u.crud.source = "fake-server",
        .s.u.crud.dest = "seshat",
        .s.u.crud.headers = NULL,
        .s.u.crud.metadata = NULL,
        .s.u.crud.include_spans = false,
        .s.u.crud.spans.spans = NULL,
        .s.u.crud.spans.count = 0,
        .s.u.crud.status = 1,
        .s.u.crud.rdr = 0,
        .s.u.crud.payload = "service1",

        .r.msg_type = WRP_MSG_TYPE__REQ,
        .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
        .r.u.crud.source = "seshat",
        .r.u.crud.dest = "fake-server",
        .r.u.crud.headers = NULL,
        .r.u.crud.metadata = NULL,
        .r.u.crud.include_spans = false,
        .r.u.crud.spans.spans = NULL,
        .r.u.crud.spans.count = 0,
        .r.u.crud.status = 200,
        .r.u.crud.rdr = 0,
        .r.u.crud.payload = "{\"service1\":{\"url\":\"url1\"}}",
    },
};
static size_t i;

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
int ji_add_entry( const char *entry, const char *value )
{
    (void) entry; (void) value;
    return EXIT_SUCCESS;
}

int ji_retrieve_entry( const char *entry, char **object )
{
    (void) entry; 
    *object = strdup(tests[i].r.u.crud.payload);
    return EXIT_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_wi_create_response_to_message_ret()
{
    size_t t_size = sizeof(tests)/sizeof(test_t);

    for( i = 0; i < t_size; i++ ) {
        void *data;
        void *bytes;
        wrp_msg_t *msg;
        ssize_t data_s = wrp_struct_to(&tests[i].s, WRP_BYTES, &data);
        ssize_t bytes_s = wi_create_response_to_message(data, data_s, &bytes);
        CU_ASSERT(0 < bytes_s);
        ssize_t msg_s = wrp_to_struct(bytes, bytes_s, WRP_BYTES, &msg);
        CU_ASSERT(0 < msg_s);
        CU_ASSERT(tests[i].r.msg_type == msg->msg_type);
        CU_ASSERT_STRING_EQUAL(tests[i].r.u.crud.transaction_uuid, msg->u.crud.transaction_uuid);
        CU_ASSERT_STRING_EQUAL(tests[i].r.u.crud.source, msg->u.crud.source);
        CU_ASSERT_STRING_EQUAL(tests[i].r.u.crud.dest, msg->u.crud.dest);
        CU_ASSERT(tests[i].r.u.crud.status == msg->u.crud.status);
        CU_ASSERT(tests[i].r.u.crud.rdr == msg->u.crud.rdr);
        CU_ASSERT_STRING_EQUAL(tests[i].r.u.crud.path, msg->u.crud.path);
        CU_ASSERT_STRING_EQUAL(tests[i].r.u.crud.payload, msg->u.crud.payload);
        wrp_free_struct(msg);
        free(bytes);
    }
}

void add_suites( CU_pSuite *suite )
{
    printf("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test 1", test_wi_create_response_to_message_ret );
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
