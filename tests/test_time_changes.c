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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <signal.h>

#include <CUnit/Basic.h>
#include <wrp-c/wrp-c.h>

#include "../src/aker_mem.h"
#include "../src/schedule.h"
#include "../src/time.h"
#include "mem_wrapper.h"

/* The test schedule:
 *
 * A '.' indicates the end time for a particular window.
 *
 * Clock Time     Offset   Mac index block list
 * 12:00:00 AM         0
 * 12:15:00 PM       900
 * 12:30:00 PM      1800
 * 12:45:00 PM      2700   0   1
 * 01:00:00 AM      3600   0   1
 * 01:15:00 AM      4500   0   1
 * 01:30:00 AM      5400   0   .   2
 * 01:45:00 AM      6300   0       2
 * 02:00:00 AM      7200   0       2       4
 * 02:15:00 AM      8100   0       .   3   4
 * 02:30:00 AM      9000   0           3   4   5
 * 02:45:00 AM      9900   0           .   4   5
 * 03:00:00 AM     10800   0               .   5
 * 03:15:00 AM     11700   .                   .
 * 03:30:00 AM     12600
 *
 * Expected - Spring Forward
 * 12:00:00 AM         0
 * 12:15:00 PM       900
 * 12:30:00 PM      1800
 * 12:45:00 PM      2700   0   1
 * 01:00:00 AM      3600   0   1
 * 01:15:00 AM      4500   0   1
 * 01:30:00 AM      5400   0   .   2
 * 01:45:00 AM      6300   0       2
 * 01:59:59 AM      7199   0       2        
 * 03:00:00 AM     10800   0               .   5
 * 03:15:00 AM     11700   .                   .
 * 03:30:00 AM     12600
 *
 * Expected - Fall Back
 * 12:00:00 AM         0
 * 12:15:00 PM       900
 * 12:30:00 PM      1800
 * 12:45:00 PM      2700   0   1
 * 01:00:00 AM      3600   0   1
 * 01:15:00 AM      4500   0   1
 * 01:30:00 AM      5400   0   .   2
 * 01:45:00 AM      6300   0       2
 * 01:59:59 AM      7199   0       2
 * 01:00:00 AM      3600   0   1
 * 01:30:00 AM      5400   0   .   2
 * 01:45:00 AM      6300   0       2
 * 02:00:00 AM      7200   0       2       4
 * 02:15:00 AM      8100   0       .   3   4
 * 02:30:00 AM      9000   0           3   4   5
 * 02:45:00 AM      9900   0           .   4   5
 * 03:00:00 AM     10800   0               .   5
 * 03:15:00 AM     11700   .                   .
 * 03:30:00 AM     12600
 *
 * {
 *    "time_zone": "America/New_York",
 *
 *    "weekly": [
 *        { "time":  2700, "indexes": [0, 1] },
 *        { "time":  5400, "indexes": [0, 2] },
 *        { "time":  7200, "indexes": [0, 2, 4] },
 *        { "time":  8100, "indexes": [0, 3, 4] },
 *        { "time":  9000, "indexes": [0, 3, 4, 5] },
 *        { "time":  9900, "indexes": [0, 4, 5] },
 *        { "time": 10800, "indexes": [0, 5] },
 *        { "time": 11700, "indexes": null }
 *    ],
 *
 *    "macs": [
 *        "00:00:00:00:00:00",
 *        "11:11:11:11:11:11",
 *        "22:22:22:22:22:22",
 *        "33:33:33:33:33:33",
 *        "44:44:44:44:44:44",
 *        "55:55:55:55:55:55"
 *    ]
 * }
 */
schedule_t* build_schedule()
{
    schedule_t *s;
    schedule_event_t *e;

    s = create_schedule();

    e = create_schedule_event( 2 );
    e->time = 2700;
    e->block[0] = 0;
    e->block[1] = 1;
    insert_event( &s->weekly, e );

    e = create_schedule_event( 2 );
    e->time = 5400;
    e->block[0] = 0;
    e->block[1] = 2;
    insert_event( &s->weekly, e );

    e = create_schedule_event( 3 );
    e->time = 7200;
    e->block[0] = 0;
    e->block[1] = 2;
    e->block[2] = 4;
    insert_event( &s->weekly, e );

    e = create_schedule_event( 3 );
    e->time = 8100;
    e->block[0] = 0;
    e->block[1] = 3;
    e->block[2] = 4;
    insert_event( &s->weekly, e );

    e = create_schedule_event( 4 );
    e->time = 9000;
    e->block[0] = 0;
    e->block[1] = 3;
    e->block[2] = 4;
    e->block[3] = 5;
    insert_event( &s->weekly, e );

    e = create_schedule_event( 3 );
    e->time = 9900;
    e->block[0] = 0;
    e->block[1] = 4;
    e->block[2] = 5;
    insert_event( &s->weekly, e );

    e = create_schedule_event( 2 );
    e->time = 10800;
    e->block[0] = 0;
    e->block[1] = 5;
    insert_event( &s->weekly, e );

    e = create_schedule_event( 0 );
    e->time = 11700;
    insert_event( &s->weekly, e );

    create_mac_table( s, 6 );
    set_mac_index( s, "00:00:00:00:00:00", 17, 0 );
    set_mac_index( s, "11:11:11:11:11:11", 17, 1 );
    set_mac_index( s, "22:22:22:22:22:22", 17, 2 );
    set_mac_index( s, "33:33:33:33:33:33", 17, 3 );
    set_mac_index( s, "44:44:44:44:44:44", 17, 4 );
    set_mac_index( s, "55:55:55:55:55:55", 17, 5 );

    finalize_schedule( s );
    return s;
}

size_t get_max_mac_limit()
{
    return 10;
}

void test_spring()
{
    /* March 11, 2018 TZ=America/New_York *///from 12:00:01 AM to 4:00 AM - 14,399s*/
    time_t start_unix = 1520744401;  /* 12:00:01 AM EST */
    time_t end_unix   = 1520755200;  /* 04:00:00 AM EDT - 3h later */
    time_t t;
    schedule_t *s;

    set_unix_time_zone( "America/New_York" );
    s = build_schedule();
    print_schedule( s );

    for( t = start_unix; t < end_unix; t++ ) {
        char *next;
        time_t until;

        next = get_blocked_at_time( s, t );
        until = get_next_unixtime( s, t );
        if( t < 1520747100 ) {
            CU_ASSERT( NULL == next );
            CU_ASSERT( 1520747100 == until );
        }
        if( (1520747100 <= t) && (t < 1520749800) ) {
            CU_ASSERT_STRING_EQUAL(next, "00:00:00:00:00:00 11:11:11:11:11:11");
            CU_ASSERT( 1520749800 == until );
        }
        if( (1520749800 <= t) && (t < 1520751600) ) {
            CU_ASSERT_STRING_EQUAL(next, "00:00:00:00:00:00 22:22:22:22:22:22");
            CU_ASSERT( 1520751600 == until );
        }
        if( (1520751600 <= t) && (t < 1520752500) ) {
            CU_ASSERT_STRING_EQUAL(next, "00:00:00:00:00:00 55:55:55:55:55:55");
            CU_ASSERT( 1520752500 == until );
        }
        if( 1520752500 <= t ) {
            CU_ASSERT( NULL == next );
            // Next Sunday at 00:45 AM.
            CU_ASSERT( 1521348300 == until );
        }

        if( NULL != next ) {
            aker_free( next );
            next = NULL;
        }
    }

    destroy_schedule( s );
    s = NULL;
}

void test_fall()
{
    /* Nov 4, 2018 TZ=America/New_York *///from 12:00:01 AM to 4:00 AM - 17,999s*/
    time_t start_unix = 1541304001;  /* 12:00:01 AM EDT */
    time_t end_unix   = 1541322000;  /* 04:00:00 AM EST - 5h later */
    time_t t;
    schedule_t *s;

    set_unix_time_zone( "America/New_York" );
    s = build_schedule();
    print_schedule( s );

    for( t = start_unix; t < end_unix; t++ ) {
        char *next;
        time_t until;

        next = get_blocked_at_time( s, t );
        until = get_next_unixtime( s, t );
        if( t < 1541306700 ) {
            CU_ASSERT( NULL == next );
            CU_ASSERT( 1541306700 == until );
        }
        if( (1541306700 <= t) && (t < 1541309400) ) {
            CU_ASSERT_STRING_EQUAL(next, "00:00:00:00:00:00 11:11:11:11:11:11");
            CU_ASSERT( 1541309400 == until );
        }
        if( (1541309400 <= t) && (t < 1541311200) ) {
            CU_ASSERT_STRING_EQUAL(next, "00:00:00:00:00:00 22:22:22:22:22:22");
            CU_ASSERT( 1541311200 == until );
        }
        if( (1541311200 <= t) && (t < 1541313000) ) {
            CU_ASSERT_STRING_EQUAL(next, "00:00:00:00:00:00 11:11:11:11:11:11");
            CU_ASSERT( 1541313000 == until );
        }
        if( (1541313000 <= t) && (t < 1541314800) ) {
            CU_ASSERT_STRING_EQUAL(next, "00:00:00:00:00:00 22:22:22:22:22:22");
            CU_ASSERT( 1541314800 == until );
        }
        if( (1541314800 <= t) && (t < 1541315700) ) {
            CU_ASSERT_STRING_EQUAL(next, "00:00:00:00:00:00 22:22:22:22:22:22 44:44:44:44:44:44");
            CU_ASSERT( 1541315700 == until );
        }
        if( (1541315700 <= t) && (t < 1541316600) ) {
            CU_ASSERT_STRING_EQUAL(next, "00:00:00:00:00:00 33:33:33:33:33:33 44:44:44:44:44:44");
            CU_ASSERT( 1541316600 == until );
        }
        if( (1541316600 <= t) && (t < 1541317500) ) {
            CU_ASSERT_STRING_EQUAL(next, "00:00:00:00:00:00 33:33:33:33:33:33 44:44:44:44:44:44 55:55:55:55:55:55");
            CU_ASSERT( 1541317500 == until );
        }
        if( (1541317500 <= t) && (t < 1541318400) ) {
            CU_ASSERT_STRING_EQUAL(next, "00:00:00:00:00:00 44:44:44:44:44:44 55:55:55:55:55:55");
            CU_ASSERT( 1541318400 == until );
        }
        if( (1541318400 <= t) && (t < 1541319300) ) {
            CU_ASSERT_STRING_EQUAL(next, "00:00:00:00:00:00 55:55:55:55:55:55");
            CU_ASSERT( 1541319300 == until );
        }
        if( 1541319300 <= t ) {
            CU_ASSERT( NULL == next );
            // Next Sunday at 00:45 AM.
            CU_ASSERT( 1541915100 == until );
        }

        if( NULL != next ) {
            aker_free( next );
            next = NULL;
        }

/*
        if( (NULL == next && NULL != last) ||
            (NULL != next && NULL == last) ||
            (NULL != next && NULL != last && 0 != strcmp(next, last)) )
        {
            struct tm ts;

            ts = *localtime(&t);

            printf( "%ld -> %d-%02d-%02d %02d:%02d:%02d %s\n", t,
                    (ts.tm_year+1900), (ts.tm_mon+1), ts.tm_mday,
                    ts.tm_hour, ts.tm_min, ts.tm_sec,
                    next );

            if( NULL != last ) {
                aker_free( last );
            }
            last = next;
        }
*/
    }
}

void add_suites( CU_pSuite *suite )
{
    printf("--------Start of Test Cases Execution For Scheduler---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Scheduler Test Spring Forward", test_spring);
    CU_add_test( *suite, "Scheduler Test Fall Back", test_fall);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main( int argc, char *argv[] )
{
    unsigned rv = 1;
    CU_pSuite suite = NULL;
 
    (void) argc;
    (void) argv;
    
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
