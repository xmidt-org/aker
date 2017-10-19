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
schedule_t* create_schedule( void );
schedule_event_t* create_schedule_event( size_t block_count );
void insert_event(schedule_event_t **head, schedule_event_t *e );
void prune_expired_events( schedule_t *s, uint32_t unixtime );
void destroy_schedule( schedule_t *s );
char* get_blocked_at_time( schedule_t *s, uint32_t unixtime, uint32_t weekly );
char* convert_index_to_string( schedule_t *s, size_t count, uint32_t *block );
int create_mac_table( schedule_t *s, size_t count );
extern bool validate_mac( const char *mac, size_t len );
bool set_mac_index( schedule_t *s, const char *mac, size_t len, uint32_t index );

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

    (void) rv;
}

void test_mac_validator( void )
{
    CU_ASSERT( true == validate_mac("11:22:33:aa:bb:CC", 17) );
    CU_ASSERT( false == validate_mac("11-22-33-aa-bb-CC", 17) );
    CU_ASSERT( false == validate_mac("zz:22:33:aa:bb:CC", 17) );
    CU_ASSERT( false == validate_mac("11:22:33:aa:bb:CC:12", 20) );
    CU_ASSERT( false == validate_mac("11:22:33:aa:bb", 14) );
    CU_ASSERT( false == validate_mac(NULL, 17) );
}

void test_simple_case( void )
{
    schedule_t *s;
    schedule_event_t *e;
    char *block;
    int rv;

    s = create_schedule();
    CU_ASSERT( NULL != s );

    /* Don't crash */
    block = get_blocked_at_time( s, 1234000, 30 );
    CU_ASSERT( NULL == block );

    /* Don't crash */
    prune_expired_events( s, 1234000 );

    rv = create_mac_table( s, 3 );
    CU_ASSERT( 0 == rv );

    CU_ASSERT( false == set_mac_index(s, NULL, 17, 0) );
    CU_ASSERT( false == set_mac_index(s, "11:22:33:44:55:66", 17, 12) );
    CU_ASSERT( true == set_mac_index(s, "11:22:33:44:55:66", 17, 0) );
    CU_ASSERT( true == set_mac_index(s, "22:33:44:55:66:aa", 17, 1) );
    CU_ASSERT( true == set_mac_index(s, "33:44:55:66:aa:BB", 17, 2) );


    /* Add an entry */
    e = create_schedule_event( 2 );
    CU_ASSERT( NULL != e );
    e->time = 1234000;
    e->block[0] = 2;
    e->block[1] = 1;

    insert_event( &s->absolute, e );

    block = convert_index_to_string( s, e->block_count, e->block );
    CU_ASSERT_STRING_EQUAL("33:44:55:66:aa:BB 22:33:44:55:66:aa", block);

    block = get_blocked_at_time( s, 1234001, 10 );
    CU_ASSERT_STRING_EQUAL("33:44:55:66:aa:BB 22:33:44:55:66:aa", block);

    block = get_blocked_at_time( s, 1234001, 10 );
    CU_ASSERT( NULL == block );

    destroy_schedule( s );
}

void add_suites( CU_pSuite *suite )
{
    printf( "--------Start of Test Cases Execution ---------\n" );
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test decoder", test_decoder );
    CU_add_test( *suite, "Test MAC validator", test_mac_validator );
    CU_add_test( *suite, "Test simple case", test_simple_case );
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
