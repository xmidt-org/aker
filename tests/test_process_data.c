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


void test_process_data()
{
    uint8_t *data = NULL;
    uint8_t *test_vector = NULL;
    size_t len, data_size = 0;
    int rv;

    data_size = get_data(&test_vector);

    /* No file names */
    rv = process_update(NULL, NULL, test_vector, data_size);
    CU_ASSERT( 0 != rv );

    /* Empty payload */
    rv = process_update("pcs.bin", "pcs_md5.bin", NULL, 0);
    CU_ASSERT( rv != 0 );

    /* Normal payload */
    rv = process_update("pcs.bin", "pcs_md5.bin", test_vector, data_size);
    CU_ASSERT( rv == 0 );

    len = read_file_from_disk("pcs.bin", &data);
    CU_ASSERT(data_size == len);
    CU_ASSERT(0 == memcmp(test_vector, data, len));

    if( NULL != data ) {
        free(data);
    }
}

void add_suites( CU_pSuite *suite )
{
    printf("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test 1", test_process_data );
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
