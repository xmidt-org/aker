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
#include "../src/process_data.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* None */

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
uint8_t get_data(uint8_t **data)
{
    ssize_t data_size = 0;

    const char some_binary_file[] = "../../tests/some.bin";
    FILE *file_handle = fopen(some_binary_file, "rb");
    if( NULL != file_handle ) {
        int32_t file_size;

        fseek(file_handle, 0, SEEK_END);
        file_size = ftell(file_handle);
        if (file_size < 0) {
            printf("read_file_from_disk() ftell() error on %s\n", some_binary_file);
            fclose(file_handle);
        } else {
            fseek(file_handle, 0, SEEK_SET);

            if( file_size > 0 ) { 
                *data = (uint8_t*) malloc(file_size);

                if( NULL != *data ) {
                    data_size = fread(*data, sizeof(uint8_t), file_size, file_handle);
                }
            }
            fclose(file_handle);
        }
    }
    return data_size;
}


void test_process_cu_and_ret()
{
    wrp_msg_t tests_cu[] =
    {
        {
            .msg_type = WRP_MSG_TYPE__CREATE,
            .u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .u.crud.source = "fake-server",
            .u.crud.dest = SCHEDULE_ENDPOINT,
            .u.crud.partner_ids = NULL,
            .u.crud.headers = NULL,
            .u.crud.metadata = NULL,
            .u.crud.include_spans = false,
            .u.crud.spans.spans = NULL,
            .u.crud.spans.count = 0,
            .u.crud.path = "Some path",
        },
    };

    wrp_msg_t tests_ret[] =
    {
        {
            .msg_type = WRP_MSG_TYPE__RETREIVE,
            .u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .u.crud.source = SCHEDULE_ENDPOINT,
            .u.crud.dest = "fake-server",
            .u.crud.partner_ids = NULL,
            .u.crud.headers = NULL,
            .u.crud.metadata = NULL,
            .u.crud.include_spans = false,
            .u.crud.spans.spans = NULL,
            .u.crud.spans.count = 0,
            .u.crud.path = "Some path",
        },
    };

    size_t t_size = sizeof(tests_cu)/sizeof(wrp_msg_t);
    ssize_t cu_size = 0, ret_size = 0, data_size = 0;
    wrp_msg_t response;
    uint8_t i, *data = NULL;

    data_size = get_data(&data);
    for( i = 0; i < t_size; i++ ) {
        tests_cu[i].u.crud.payload = data;
        tests_cu[i].u.crud.payload_size = data_size;
        cu_size = process_message_cu("pcs.bin", "pcs_md5.bin", &tests_cu[i]);
        CU_ASSERT((size_t)cu_size == tests_cu[i].u.crud.payload_size);
    }

    t_size = sizeof(tests_ret)/sizeof(wrp_msg_t);
    for( i = 0; i < t_size; i++ ) {
        tests_ret[i].u.crud.payload = data;
        tests_ret[i].u.crud.payload_size = data_size;
        memset(&response, '\0', sizeof(wrp_msg_t));
        ret_size = process_message_ret_all("pcs.bin", &response);
        CU_ASSERT(0 == memcmp(tests_ret[i].u.crud.payload, response.u.crud.payload, response.u.crud.payload_size));
        CU_ASSERT(tests_ret[i].u.crud.payload_size == response.u.crud.payload_size);
        free(response.u.crud.payload);
        CU_ASSERT((size_t)ret_size == tests_ret[i].u.crud.payload_size);
    }
    if( NULL != data ) {
        free(data);
    }
}

void test_null_file()
{
    wrp_msg_t test_cu =
    {
        .msg_type = WRP_MSG_TYPE__CREATE,
        .u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
        .u.crud.source = "fake-server",
        .u.crud.dest = SCHEDULE_ENDPOINT,
        .u.crud.partner_ids = NULL,
        .u.crud.headers = NULL,
        .u.crud.metadata = NULL,
        .u.crud.include_spans = false,
        .u.crud.spans.spans = NULL,
        .u.crud.spans.count = 0,
        .u.crud.path = "Some path",
    };
    uint8_t *data = NULL;

    test_cu.u.crud.payload_size = get_data(&data);
    test_cu.u.crud.payload = data;
    ssize_t cu_size = process_message_cu(NULL, NULL, &test_cu);
    CU_ASSERT(0 <= cu_size);

    test_cu.u.crud.payload_size = 0;
    test_cu.u.crud.payload = NULL;
    cu_size = process_message_cu("pcs.bin", "pcs.bin.md5", &test_cu);
    CU_ASSERT(-1 == cu_size);

    if( NULL != data ) {
        free(data);
    }
}

void add_suites( CU_pSuite *suite )
{
    printf("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test 1", test_process_cu_and_ret );
    CU_add_test( *suite, "Test null file", test_null_file );
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

int32_t get_max_mac_limit(void)
{
    return 2048;
}