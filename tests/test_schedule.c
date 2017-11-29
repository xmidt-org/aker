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
#include <stdlib.h>
#include <limits.h>

#include <CUnit/Basic.h>

#include "../src/schedule.h"
#include "../src/decode.h"
#include "../src/time.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct schedule_input {
    time_t   time;
    uint8_t  block_count;
    uint32_t block[2];
} input_t;

typedef struct get_blocked_at_time_test {
    time_t   unixtime;
    char     *macs;
    time_t   next_unixtime;
} block_test_t;

typedef struct schedule_test {
    char         **macs;
    size_t       macs_size;
    input_t      *absolute;
    size_t       absolute_size;
    input_t      *weekly;
    size_t       weekly_size;
    block_test_t *block_test;
    size_t       block_test_size;
} schedule_test_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
int __validate_mac( const char *mac, size_t len );


/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
void test_decoder( void )
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
    destroy_schedule(s);

    (void) rv;
}


time_t convert_unix_time_to_weekly( time_t unixtime )
{
    //printf( "unix: %ld -> rel: %ld\n", unixtime, (unixtime - 1234000 + 11) );
    return unixtime - 1234000 + 11;
}


time_t get_unix_time(void)
{
    struct timespec tm;
    time_t unix_time = 0;

    clock_gettime(CLOCK_REALTIME, &tm);
    unix_time = tm.tv_sec; // ignore tm.tv_nsec

    return unix_time;
}

void test_mac_validator( void )
{
    CU_ASSERT( 0 == __validate_mac("11:22:33:aa:bb:CC", 17) );
    CU_ASSERT( 0 != __validate_mac("11-22-33-aa-bb-CC", 17) );
    CU_ASSERT( 0 != __validate_mac("zz:22:33:aa:bb:CC", 17) );
    CU_ASSERT( 0 != __validate_mac("11:22:33:aa:bb:CC:12", 20) );
    CU_ASSERT( 0 != __validate_mac("11:22:33:aa:bb", 14) );
    CU_ASSERT( 0 != __validate_mac(NULL, 17) );
}

void run_schedule_test( schedule_test_t *t )
{
    schedule_t *s;
    schedule_event_t *e;
    char *block;
    int rv;
    uint8_t i, j;
    time_t next_unixtime;

    CU_ASSERT( NULL != t );

    s = create_schedule();
    CU_ASSERT( NULL != s );

    rv = create_mac_table( s, 5 );
    CU_ASSERT( 0 == rv );

    for( i = 0; i < t->macs_size/sizeof(char *); i++ ) {
        CU_ASSERT( 0 == set_mac_index(s, t->macs[i], 17, i) );
    }

    /* Add absolute entries */
    for( i = 0; (NULL != t->absolute) && (i < t->absolute_size/sizeof(input_t)); i++ ) {
        e = create_schedule_event( t->absolute[i].block_count );
        CU_ASSERT( NULL != e );
        e->time = t->absolute[i].time;
        for( j = 0; j < t->absolute[i].block_count; j++) {
            e->block[j] = t->absolute[i].block[j];
        }
        insert_event( &s->absolute, e );
    }

    /* Add weekly entry */
    for( i = 0; (NULL != t->weekly) && (i < t->weekly_size/sizeof(input_t)); i++ ) {
        e = create_schedule_event( t->weekly[i].block_count );
        CU_ASSERT( NULL != e );
        e->time = t->weekly[i].time;
        for( j = 0; j < t->weekly[i].block_count; j++) {
            e->block[j] = t->weekly[i].block[j];
        }
        insert_event( &s->weekly, e );
    }

    finalize_schedule( s );

    print_schedule( s );

    for( i = 0; i < t->block_test_size/sizeof(block_test_t); i++ ) {
        block = get_blocked_at_time( s, t->block_test[i].unixtime );
        if( NULL != t->block_test[i].macs && NULL != block ) {
            CU_ASSERT_STRING_EQUAL(t->block_test[i].macs, block);
        }
        else if( NULL == t->block_test[i].macs && NULL == block ) {
            CU_PASS("Both expected and observed are NULL.");
        }
        else if( (NULL != t->block_test[i].macs && NULL == block) ||
                 (NULL == t->block_test[i].macs && NULL != block) )
        {
            CU_FAIL("Both expected and observed are NOT EQUAL! One is NULL and the other NOT NULL!!");
        }
        next_unixtime = get_next_unixtime(s, t->block_test[i].unixtime);
        CU_ASSERT(t->block_test[i].next_unixtime == next_unixtime);
        if( NULL != block ) free(block);
    }

    destroy_schedule( s );
}

void test_simple_case( void )
{
    char *mac_id[] = { "11:22:33:44:55:66", "22:33:44:55:66:aa", "33:44:55:66:aa:BB", };

    input_t absolute[] = { 
        { .time = 1234000, .block_count = 2, .block = { 2, 1 }, },
        { .time = 1234010, .block_count = 1, .block = { 2 }, },
    };
    
    input_t weekly[] = {
        { .time = 23, .block_count = 1, .block = { 0 }, },
        { .time = 24, .block_count = 1, .block = { 1 }, },
    };
    
    block_test_t b_test[] = {
        { .unixtime = 1233999, .macs = "22:33:44:55:66:aa",                   .next_unixtime = 1234000, },
        { .unixtime = 1234000, .macs = "33:44:55:66:aa:BB 22:33:44:55:66:aa", .next_unixtime = 1234010, },
        { .unixtime = 1234001, .macs = "33:44:55:66:aa:BB 22:33:44:55:66:aa", .next_unixtime = 1234010, },
        { .unixtime = 1234010, .macs = "33:44:55:66:aa:BB",                   .next_unixtime = 1234012, },
        { .unixtime = 1234011, .macs = "33:44:55:66:aa:BB",                   .next_unixtime = 1234012, },
        { .unixtime = 1234012, .macs = "11:22:33:44:55:66",                   .next_unixtime = 1234013, },
    };
    
    schedule_test_t test = { .macs = mac_id,       .macs_size = sizeof(mac_id),
                             .absolute = absolute, .absolute_size = sizeof(absolute),
                             .weekly = weekly,     .weekly_size = sizeof(weekly),
                             .block_test = b_test, .block_test_size = sizeof(b_test),
    };
    
    run_schedule_test(&test);
}

void test_another_usecase( void )
{   
    char *mac_id[] = { "11:22:33:44:55:66", "22:33:44:55:66:aa", "33:44:55:66:aa:BB", "44:55:66:aa:BB:cc", "55:66:aa:BB:cc:DD", };

    input_t absolute[] = { 
        { .time = 0,       .block_count = 2, .block = { 2, 1 }, },
        { .time = 1234010, .block_count = 1, .block = { 2 }, },
    };

    input_t weekly[] = {
        { .time = 23, .block_count = 1, .block = { 0 }, },
        { .time = 24, .block_count = 1, .block = { 1 }, },
    };

    block_test_t b_test[] = {
        { .unixtime = 1233999, .macs = "33:44:55:66:aa:BB 22:33:44:55:66:aa", .next_unixtime = 1234010, },
        { .unixtime = 1234000, .macs = "33:44:55:66:aa:BB 22:33:44:55:66:aa", .next_unixtime = 1234010, },
        { .unixtime = 1234001, .macs = "33:44:55:66:aa:BB 22:33:44:55:66:aa", .next_unixtime = 1234010, },
        { .unixtime = 1234010, .macs = "33:44:55:66:aa:BB",                   .next_unixtime = 1234012, },
        { .unixtime = 1234011, .macs = "33:44:55:66:aa:BB",                   .next_unixtime = 1234012, },
        { .unixtime = 1234012, .macs = "11:22:33:44:55:66",                   .next_unixtime = 1234013, },
    };

    schedule_test_t test = { .macs = mac_id,       .macs_size = sizeof(mac_id),
                             .absolute = absolute, .absolute_size = sizeof(absolute),
                             .weekly = weekly,     .weekly_size = sizeof(weekly),
                             .block_test = b_test, .block_test_size = sizeof(b_test),
    };

    run_schedule_test(&test);
}

void test_no_schedule( void )
{
    char *mac_id[] = { "11:22:33:44:55:66", "22:33:44:55:66:aa", "33:44:55:66:aa:BB", "44:55:66:aa:BB:cc", "55:66:aa:BB:cc:DD", };

    block_test_t b_test[] = {
        { .unixtime = 1233999, .macs = NULL, .next_unixtime = INT_MAX, },
        { .unixtime = 1234000, .macs = NULL, .next_unixtime = INT_MAX, },
        { .unixtime = 1234001, .macs = NULL, .next_unixtime = INT_MAX, },
        { .unixtime = 1234010, .macs = NULL, .next_unixtime = INT_MAX, },
        { .unixtime = 1234011, .macs = NULL, .next_unixtime = INT_MAX, },
        { .unixtime = 1234012, .macs = NULL, .next_unixtime = INT_MAX, },
    };

    schedule_test_t test = { .macs = mac_id,       .macs_size = sizeof(mac_id),
                             .absolute = NULL,     .absolute_size = 0,
                             .weekly = NULL,       .weekly_size = 0,
                             .block_test = b_test, .block_test_size = sizeof(b_test),
    };

    run_schedule_test(&test);
}

void test_only_one_absolute( void )
{
    char *mac_id[] = { "11:22:33:44:55:66", "22:33:44:55:66:aa", "33:44:55:66:aa:BB", "44:55:66:aa:BB:cc", "55:66:aa:BB:cc:DD", };

    input_t absolute1[] = {
        { .time = 0,       .block_count = 2, .block = { 2, 1 }, },
    };

    input_t absolute2[] = {
        { .time = 1234010, .block_count = 1, .block = { 2 }, },
    };

    block_test_t b_test1[] = {
        { .unixtime = 1233999, .macs = "33:44:55:66:aa:BB 22:33:44:55:66:aa", .next_unixtime = INT_MAX, },
        { .unixtime = 1234000, .macs = "33:44:55:66:aa:BB 22:33:44:55:66:aa", .next_unixtime = INT_MAX, },
        { .unixtime = 1234001, .macs = "33:44:55:66:aa:BB 22:33:44:55:66:aa", .next_unixtime = INT_MAX, },
        { .unixtime = 1234010, .macs = "33:44:55:66:aa:BB 22:33:44:55:66:aa", .next_unixtime = INT_MAX, },
        { .unixtime = 1234011, .macs = "33:44:55:66:aa:BB 22:33:44:55:66:aa", .next_unixtime = INT_MAX, },
        { .unixtime = 1234012, .macs = "33:44:55:66:aa:BB 22:33:44:55:66:aa", .next_unixtime = INT_MAX, },
    };

    block_test_t b_test2[] = {
        { .unixtime = 1233999, .macs = NULL,                .next_unixtime = 1234010, },
        { .unixtime = 1234000, .macs = NULL,                .next_unixtime = 1234010, },
        { .unixtime = 1234001, .macs = NULL,                .next_unixtime = 1234010, },
        { .unixtime = 1234010, .macs = "33:44:55:66:aa:BB", .next_unixtime = INT_MAX, },
        { .unixtime = 1234011, .macs = "33:44:55:66:aa:BB", .next_unixtime = INT_MAX, },
        { .unixtime = 1234012, .macs = "33:44:55:66:aa:BB", .next_unixtime = INT_MAX, },
    };

    schedule_test_t test = { .macs = mac_id,        .macs_size = sizeof(mac_id),
                             .absolute = absolute1, .absolute_size = sizeof(absolute1),
                             .weekly = NULL,        .weekly_size = 0,
                             .block_test = b_test1, .block_test_size = sizeof(b_test1),
    };

    run_schedule_test(&test);

    test.absolute = absolute2;
    test.absolute_size = sizeof(absolute2);
    test.block_test = b_test2;
    test.block_test_size = sizeof(b_test2);

    run_schedule_test(&test);
}

void test_only_one_weekly( void )
{
    #define WEEKLY_23_NEXT_WEEK 1234012 + SECONDS_IN_A_WEEK

    char *mac_id[] = { "11:22:33:44:55:66", "22:33:44:55:66:aa", "33:44:55:66:aa:BB", "44:55:66:aa:BB:cc", "55:66:aa:BB:cc:DD", };

    input_t weekly[] = {
        { .time = 23, .block_count = 1, .block = { 0 }, },
    };

    block_test_t b_test[] = {
        { .unixtime = 1233999, .macs = "11:22:33:44:55:66", .next_unixtime = 1234012, },
        { .unixtime = 1234000, .macs = "11:22:33:44:55:66", .next_unixtime = 1234012, },
        { .unixtime = 1234001, .macs = "11:22:33:44:55:66", .next_unixtime = 1234012, },
        { .unixtime = 1234010, .macs = "11:22:33:44:55:66", .next_unixtime = 1234012, },
        { .unixtime = 1234011, .macs = "11:22:33:44:55:66", .next_unixtime = 1234012, },
        { .unixtime = 1234012, .macs = "11:22:33:44:55:66", .next_unixtime = WEEKLY_23_NEXT_WEEK, },
        { .unixtime = 1234013, .macs = "11:22:33:44:55:66", .next_unixtime = WEEKLY_23_NEXT_WEEK, },
    };

    schedule_test_t test = { .macs = mac_id,       .macs_size = sizeof(mac_id),
                             .absolute = NULL,     .absolute_size = 0,
                             .weekly = weekly,     .weekly_size = sizeof(weekly),
                             .block_test = b_test, .block_test_size = sizeof(b_test),
    };

    run_schedule_test(&test);
}

void add_suites( CU_pSuite *suite )
{
    printf( "--------Start of Test Cases Execution ---------\n" );
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test decoder", test_decoder );
    CU_add_test( *suite, "Test MAC validator", test_mac_validator );
    CU_add_test( *suite, "Test simple case", test_simple_case );
    CU_add_test( *suite, "Test another usecase", test_another_usecase);
    CU_add_test( *suite, "Test no schedule", test_no_schedule);
    CU_add_test( *suite, "Test only one absolute event", test_only_one_absolute);
    CU_add_test( *suite, "Test only one weekly event", test_only_one_weekly);
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