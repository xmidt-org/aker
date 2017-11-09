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
    wrp_msg_t m;
} test_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_process_request_cu_and_ret()
{   
    test_t tests_cu[] =
    {
        {
            .m.msg_type = WRP_MSG_TYPE__CREATE,
            .m.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .m.u.crud.source = "fake-server",
            .m.u.crud.dest = "/parental-control/schedule",
            .m.u.crud.partner_ids = NULL,
            .m.u.crud.headers = NULL,
            .m.u.crud.metadata = NULL,
            .m.u.crud.include_spans = false,
            .m.u.crud.spans.spans = NULL,
            .m.u.crud.spans.count = 0,
            .m.u.crud.payload = "Some binary",
            .m.u.crud.payload_size = 11,
        },
    };

    test_t tests_ret[] =
    {
        {
            .m.msg_type = WRP_MSG_TYPE__RETREIVE,
            .m.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .m.u.crud.source = "/parental-control/schedule",
            .m.u.crud.dest = "fake-server",
            .m.u.crud.partner_ids = NULL,
            .m.u.crud.headers = NULL,
            .m.u.crud.metadata = NULL,
            .m.u.crud.include_spans = false,
            .m.u.crud.spans.spans = NULL,
            .m.u.crud.spans.count = 0,
            .m.u.crud.path = "Some path",
            .m.u.crud.payload = "Some binary",
            .m.u.crud.payload_size = 11,
        },
    };

    size_t t_size = sizeof(tests_cu)/sizeof(test_t);
    ssize_t cu_size, ret_size;
    wrp_msg_t response;
    uint8_t i;

    for( i = 0; i < t_size; i++ ) {
        cu_size = process_message_cu("pcs.bin", "pcs_md5.bin", &tests_cu[i].m);
        CU_ASSERT((size_t)cu_size == tests_cu[i].m.u.crud.payload_size);
    }

    t_size = sizeof(tests_ret)/sizeof(test_t);
    for( i = 0; i < t_size; i++ ) {
        memset(&response, '\0', sizeof(wrp_msg_t));
        ret_size = process_message_ret("pcs.bin", &response);
        CU_ASSERT(0 == memcmp(tests_ret[i].m.u.crud.payload, response.u.crud.payload, response.u.crud.payload_size));
        CU_ASSERT(tests_ret[i].m.u.crud.payload_size == response.u.crud.payload_size);
        free(response.u.crud.payload);
        CU_ASSERT((size_t)ret_size == tests_ret[i].m.u.crud.payload_size);
    }
}

void add_suites( CU_pSuite *suite )
{
    printf("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test 1", test_process_request_cu_and_ret );
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

