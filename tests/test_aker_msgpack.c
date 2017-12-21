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
#include "../src/aker_msgpack.h"
#include "../src/aker_mem.h"

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
void test_pack_status_msg()
{
    uint8_t expected[] = { 0x81,
                                 0xa7, 'm', 'e', 's', 's', 'a', 'g', 'e',
                                 0xaf, 'I', ' ', 'a', 'm', ' ', 'a', ' ',
                                 'm', 'e', 's', 's', 'a', 'g', 'e', '.' };

    size_t len;
    uint8_t *buf;

    buf = NULL;
    len = pack_status_msg( "I am a message.", (void**) &buf );
    CU_ASSERT( len = sizeof(expected)/sizeof(uint8_t) );
    CU_ASSERT( NULL != buf );
    CU_ASSERT( 0 == memcmp(expected, buf, len) );
    aker_free(buf);
}

void test_pack_now_msg()
{
    // time: 1513822552
    uint8_t expected0[] = { 0x82,
                                 0xa6, 'a', 'c', 't', 'i', 'v', 'e',
                                 0xa0,
                                 0xa4, 't', 'i', 'm', 'e',
                                 0xce, 0x5a, 0x3b, 0x19, 0x58 };

    // time: 1513822553
    uint8_t expected1[] = { 0x82,
                                 0xa6, 'a', 'c', 't', 'i', 'v', 'e',
                                 0xb1, '1', '1', ':', '2', '2', ':', '3', '3', ':', '4', '4', ':', '5', '5', ':', '6', '6',
                                 0xa4, 't', 'i', 'm', 'e',
                                 0xce, 0x5a, 0x3b, 0x19, 0x59 };

    size_t len;
    uint8_t *buf;

    buf = NULL;

    len = pack_now_msg( NULL, 1513822552, (void**) &buf );
    CU_ASSERT( len = sizeof(expected0)/sizeof(uint8_t) );
    CU_ASSERT( NULL != buf );
    CU_ASSERT( 0 == memcmp(expected0, buf, len) );
    aker_free(buf);
    buf = NULL;

    len = pack_now_msg( "11:22:33:44:55:66", 1513822553, (void**) &buf );
    CU_ASSERT( len = sizeof(expected1)/sizeof(uint8_t) );
    CU_ASSERT( NULL != buf );
    CU_ASSERT( 0 == memcmp(expected1, buf, len) );
    aker_free(buf);
    buf = NULL;
}


void add_suites( CU_pSuite *suite )
{
    printf("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test pack_status_msg", test_pack_status_msg );
    CU_add_test( *suite, "Test pack_now_msg", test_pack_now_msg );
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
