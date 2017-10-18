/**
 * Copyright 2017 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>

#include <CUnit/Basic.h>

#include "../src/schedule.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
void test_normal( void )
{
    uint8_t data[] = {
        0x83,
            0xaf, 'w', 'e', 'e', 'k', 'l', 'y', '-', 's', 'c', 'h', 'e', 'd', 'u', 'l', 'e',
            0x93,
                0x82,
                    0xa4, 't', 'i', 'm', 'e',
                    0x0a,
                    0xa7, 'i', 'n', 'd', 'e', 'x', 'e', 's',
                    0x93,
                    0x00, 0x01, 0x03,
                0x82,
                    0xa4, 't', 'i', 'm', 'e',
                    0x14,
                    0xa7, 'i', 'n', 'd', 'e', 'x', 'e', 's',
                    0x91,
                    0x00,
                0x82,
                    0xa4, 't', 'i', 'm', 'e',
                    0x14,
                    0xa7, 'i', 'n', 'd', 'e', 'x', 'e', 's',
                    0x90,

            0xa4, 'm', 'a', 'c', 's',
            0x94,
                0xb1, '1', '1', ':', '2', '2', ':', '3', '3', ':', '4', '4', ':', '5', '5', ':', 'a', 'a',
                0xb1, '2', '2', ':', '3', '3', ':', '4', '4', ':', '5', '5', ':', '6', '6', ':', 'b', 'b',
                0xb1, '3', '3', ':', '4', '4', ':', '5', '5', ':', '6', '6', ':', '7', '7', ':', 'c', 'c',
                0xb1, '4', '4', ':', '5', '5', ':', '6', '6', ':', '7', '7', ':', '8', '8', ':', 'd', 'd',

            0xb1, 'a', 'b', 's', 'o', 'l', 'u', 't', 'e', '-', 's', 'c', 'h', 'e', 'd', 'u', 'l', 'e',
            0x91,
                0x82,
                    0xa9, 'u', 'n', 'i', 'x', '-', 't', 'i', 'm', 'e',
                    0xce, 0x59, 0xe5, 0x83, 0x17,

                    0xa7, 'i', 'n', 'd', 'e', 'x', 'e', 's',
                    0x92,
                    0x00, 0x02 };

    size_t len = sizeof(data)/sizeof(uint8_t);
    schedule_t *s;
    int rv;

    rv = decode_schedule( len, data, &s );

    (void) rv;
}

void add_suites( CU_pSuite *suite )
{
    printf( "--------Start of Test Cases Execution ---------\n" );
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test 1", test_normal );
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
