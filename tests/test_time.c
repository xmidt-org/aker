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
#include <stdint.h>

#include <CUnit/Basic.h>
#include "../src/time.h"


void time_test1()
{
    time_t t = convert_unix_time_to_weekly(0);
    CU_ASSERT(0 != t);
    t = get_unix_time();
    CU_ASSERT(0 != t);
    t = convert_unix_time_to_weekly(t);
    CU_ASSERT(0 != t);
}

void test_time_zone()
{
    set_unix_time_zone("no_exist/no_where");

    set_unix_time_zone("America/Boa_Vista");
    set_unix_time_zone("America/Kentucky/Monticello");
    set_unix_time_zone("America/North_Dakota/New_Salem");
    set_unix_time_zone("America/Los_Angeles");
    set_unix_time_zone("US/Arizona");
    set_unix_time_zone("Etc/GMT+0");
}


void add_suites( CU_pSuite *suite )
{
    printf("--------Start of Test Cases Execution For test_time ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Time Test 1", time_test1);
    CU_add_test( *suite, "test_time_zone", test_time_zone);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main( int argc, char *argv[] )
{
    unsigned rv = 1;
    CU_pSuite suite = NULL;
 
    (void ) argc;
    (void ) argv;
    
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

void set_gmtoff(long int timezoneoff)
{
    (void)timezoneoff;
    return;
}
