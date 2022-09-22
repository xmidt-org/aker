 /**
  * Copyright 2021 Comcast Cable Communications Management, LLC
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
#include <wrp-c/wrp-c.h>
#include <libparodus.h>
#include "../src/aker_metrics.h"
#include "../src/aker_metrics.c"

void test_device_block_count()
{
    aker_metric_inc_device_block_count(10);
    CU_ASSERT(10 == g_metrics.device_block_count);

    aker_metric_inc_device_block_count(15);
    CU_ASSERT(25 == g_metrics.device_block_count);

    aker_metric_inc_device_block_count(10);
    CU_ASSERT(35 == g_metrics.device_block_count);

    destroy_akermetrics();
}

void test_window_trans_count()
{
    aker_metric_inc_window_trans_count();
    CU_ASSERT(1 == g_metrics.window_trans_count);

    aker_metric_inc_window_trans_count();
    CU_ASSERT(2 == g_metrics.window_trans_count);

    aker_metric_inc_window_trans_count();
    aker_metric_inc_window_trans_count();
    CU_ASSERT(4 == g_metrics.window_trans_count);

    destroy_akermetrics();
}

void test_schedule_set_count()
{
    aker_metric_inc_schedule_set_count();
    CU_ASSERT(1 == g_metrics.schedule_set_count);

    aker_metric_inc_schedule_set_count();
    CU_ASSERT(2 == g_metrics.schedule_set_count);

    aker_metric_inc_schedule_set_count();
    aker_metric_inc_schedule_set_count();
    CU_ASSERT(4 == g_metrics.schedule_set_count);

    destroy_akermetrics();
}

void test_md5_err_count()
{
    aker_metric_inc_md5_err_count();
    CU_ASSERT(1 == g_metrics.md5_err_count);

    aker_metric_inc_md5_err_count();
    CU_ASSERT(2 == g_metrics.md5_err_count);

    aker_metric_inc_md5_err_count();
    aker_metric_inc_md5_err_count();
    CU_ASSERT(4 == g_metrics.md5_err_count);

    destroy_akermetrics();

}

void test_process_start_time()
{
    aker_metric_set_process_start_time(1631799413);
    CU_ASSERT(1631799413 == g_process_start_time);

    aker_metric_set_process_start_time(1631728753);
    CU_ASSERT(1631728753 == g_process_start_time);

    aker_metric_set_process_start_time(1331791129);
    CU_ASSERT(1331791129 == g_process_start_time);

    destroy_akermetrics();

}

void test_schedule_enabled()
{
    aker_metric_set_schedule_enabled(1);
    CU_ASSERT(1 == g_metrics.schedule_enabled);

    aker_metric_set_schedule_enabled(0);
    CU_ASSERT(0 == g_metrics.schedule_enabled);

    destroy_akermetrics();
}

void test_timezone()
{
    aker_metric_set_tz("NULL");
    CU_ASSERT_STRING_EQUAL(g_metrics.timezone, "NULL");

    aker_metric_set_tz("PST8PDT");
    CU_ASSERT_STRING_EQUAL(g_metrics.timezone, "PST8PDT");

    aker_metric_set_tz("America/New_York");
    CU_ASSERT_STRING_EQUAL(g_metrics.timezone, "America/New_York");

    /* Make sure to check a shorter string in place to ensure the string is
     * terminated properly.  This catches the issue Guru found. */
    aker_metric_set_tz("NULL");
    CU_ASSERT_STRING_EQUAL(g_metrics.timezone, "NULL");

    aker_metric_set_tz(NULL);
    CU_ASSERT_STRING_EQUAL(g_metrics.timezone, "");

    destroy_akermetrics();
}

void test_blocked_mac_count()
{
    int val = 0;

    val = get_blocked_mac_count("7e:55:11:22:33:0a 66:2e:cd:44:45:67");
    CU_ASSERT(2 == val);

    val = get_blocked_mac_count("7e:55:11:22:33:0a");
    CU_ASSERT(1 == val);

    val = get_blocked_mac_count(NULL);
    CU_ASSERT(0 == val);

    val = get_blocked_mac_count("7e:55:11:22:33:0a 66:2e:cd:44:45:67 44:55:7e:8f:dc:21");
    CU_ASSERT(3 == val);

    destroy_akermetrics();
}

const char report_0[] =  "\x83"  /* map(3) */
                            "\xa2""ps"   /*:*/ "\x0c" /* 12 */
                            "\xa2""id"   /*:*/ "\xb0""mac:112233445566"
                            "\xa4""rows" /*:*/
                                "\x91"   /* Array length 1 */
                                    "\x87" /* map(7) */
                                        "\xa2""ts"  /*:*/ "\x14" /* 20 */
                                        "\xa3""off" /*:*/ "\x3c" /* 60 */
                                        "\xa3""dbc" /*:*/ "\x0a" /* 10 */
                                        "\xa3""wtc" /*:*/ "\x01" /* 1 */
                                        "\xa3""ssc" /*:*/ "\x01" /* 1 */
                                        "\xa3""md5" /*:*/ "\x01" /* 1 */
                                        "\xa2""tz"  /*:*/ "\xa3""foo";

const char report_1[] =  "\x83"  /* map(3) */
                            "\xa2""ps"   /*:*/ "\x0c" /* 12 */
                            "\xa2""id"   /*:*/ "\xb0""mac:112233445566"
                            "\xa4""rows" /*:*/
                                "\x92"   /* Array length 2 */
                                    "\x87" /* map(7) */
                                        "\xa2""ts"  /*:*/ "\x28" /* 40 */
                                        "\xa3""off" /*:*/ "\x3c" /* 60 */
                                        "\xa3""dbc" /*:*/ "\x14" /* 20 */
                                        "\xa3""wtc" /*:*/ "\x01" /* 1 */
                                        "\xa3""ssc" /*:*/ "\x01" /* 1 */
                                        "\xa3""md5" /*:*/ "\x01" /* 1 */
                                        "\xa2""tz"  /*:*/ "\xa3""foo"
                                    "\x87" /* map(7) */
                                        "\xa2""ts"  /*:*/ "\x14" /* 20 */
                                        "\xa3""off" /*:*/ "\x3c" /* 60 */
                                        "\xa3""dbc" /*:*/ "\x0a" /* 10 */
                                        "\xa3""wtc" /*:*/ "\x01" /* 1 */
                                        "\xa3""ssc" /*:*/ "\x01" /* 1 */
                                        "\xa3""md5" /*:*/ "\x01" /* 1 */
                                        "\xa2""tz"  /*:*/ "\xa3""foo";

const char report_2[] =  "\x83"  /* map(3) */
                            "\xa2""ps"   /*:*/ "\x0c" /* 12 */
                            "\xa2""id"   /*:*/ "\xb0""mac:112233445566"
                            "\xa4""rows" /*:*/
                                "\x93"   /* Array length 3 */
                                    "\x87" /* map(7) */
                                        "\xa2""ts"  /*:*/ "\x3c" /* 60 */
                                        "\xa3""off" /*:*/ "\x3c" /* 60 */
                                        "\xa3""dbc" /*:*/ "\x1e" /* 30 */
                                        "\xa3""wtc" /*:*/ "\x01" /* 1 */
                                        "\xa3""ssc" /*:*/ "\x01" /* 1 */
                                        "\xa3""md5" /*:*/ "\x01" /* 1 */
                                        "\xa2""tz"  /*:*/ "\xa3""foo"
                                    "\x87" /* map(7) */
                                        "\xa2""ts"  /*:*/ "\x28" /* 40 */
                                        "\xa3""off" /*:*/ "\x3c" /* 60 */
                                        "\xa3""dbc" /*:*/ "\x14" /* 20 */
                                        "\xa3""wtc" /*:*/ "\x01" /* 1 */
                                        "\xa3""ssc" /*:*/ "\x01" /* 1 */
                                        "\xa3""md5" /*:*/ "\x01" /* 1 */
                                        "\xa2""tz"  /*:*/ "\xa3""foo"
                                    "\x87" /* map(7) */
                                        "\xa2""ts"  /*:*/ "\x14" /* 20 */
                                        "\xa3""off" /*:*/ "\x3c" /* 60 */
                                        "\xa3""dbc" /*:*/ "\x0a" /* 10 */
                                        "\xa3""wtc" /*:*/ "\x01" /* 1 */
                                        "\xa3""ssc" /*:*/ "\x01" /* 1 */
                                        "\xa3""md5" /*:*/ "\x01" /* 1 */
                                        "\xa2""tz"  /*:*/ "\xa3""foo";

const char report_3[] =  "\x83"  /* map(3) */
                            "\xa2""ps"   /*:*/ "\x0c" /* 12 */
                            "\xa2""id"   /*:*/ "\xb0""mac:112233445566"
                            "\xa4""rows" /*:*/
                                "\x93"   /* Array length 3 */
                                    "\x87" /* map(7) */
                                        "\xa2""ts"  /*:*/ "\x50" /* 80 */
                                        "\xa3""off" /*:*/ "\x3c" /* 60 */
                                        "\xa3""dbc" /*:*/ "\x28" /* 40 */
                                        "\xa3""wtc" /*:*/ "\x01" /* 1 */
                                        "\xa3""ssc" /*:*/ "\x01" /* 1 */
                                        "\xa3""md5" /*:*/ "\x01" /* 1 */
                                        "\xa2""tz"  /*:*/ "\xa3""bar"
                                    "\x87" /* map(7) */
                                        "\xa2""ts"  /*:*/ "\x3c" /* 60 */
                                        "\xa3""off" /*:*/ "\x3c" /* 60 */
                                        "\xa3""dbc" /*:*/ "\x1e" /* 30 */
                                        "\xa3""wtc" /*:*/ "\x01" /* 1 */
                                        "\xa3""ssc" /*:*/ "\x01" /* 1 */
                                        "\xa3""md5" /*:*/ "\x01" /* 1 */
                                        "\xa2""tz"  /*:*/ "\xa3""foo"
                                    "\x87" /* map(7) */
                                        "\xa2""ts"  /*:*/ "\x28" /* 40 */
                                        "\xa3""off" /*:*/ "\x3c" /* 60 */
                                        "\xa3""dbc" /*:*/ "\x14" /* 20 */
                                        "\xa3""wtc" /*:*/ "\x01" /* 1 */
                                        "\xa3""ssc" /*:*/ "\x01" /* 1 */
                                        "\xa3""md5" /*:*/ "\x01" /* 1 */
                                        "\xa2""tz"  /*:*/ "\xa3""foo";

const char *__check = NULL;
size_t __check_len = 0;
int libparodus_send( libpd_instance_t instance, wrp_msg_t *msg )
{
    (void) instance;

    CU_ASSERT(WRP_MSG_TYPE__EVENT == msg->msg_type);
    CU_ASSERT_STRING_EQUAL("event:metrics.aker", msg->u.event.dest);
    CU_ASSERT_STRING_EQUAL("mac:112233445566/aker", msg->u.event.source);
    CU_ASSERT_STRING_EQUAL("application/msgpack", msg->u.event.content_type);

    if( NULL != __check ) {
        CU_ASSERT( msg->u.event.payload_size == __check_len );
        CU_ASSERT( 0 == memcmp(__check, msg->u.event.payload, __check_len) );
    }

    return 0;
}

void test_complete()
{
    int ignored;

    aker_metric_init( "mac:112233445566", (libpd_instance_t) &ignored);

    aker_metric_set_process_start_time( 12 );
    aker_metric_inc_device_block_count( 10 );
    aker_metric_inc_window_trans_count();
    aker_metric_inc_schedule_set_count();
    aker_metric_inc_md5_err_count();
    aker_metric_set_tz( "foo" );

    __check = report_0;
    __check_len = sizeof(report_0) - 1;
    aker_metrics_report( 20 );

    __check = report_1;
    __check_len = sizeof(report_1) - 1;
    aker_metric_inc_device_block_count( 10 ); /* 20 */
    aker_metrics_report( 40 );

    __check = report_2;
    __check_len = sizeof(report_2) - 1;
    aker_metric_inc_device_block_count( 10 ); /* 30 */
    aker_metrics_report( 60 );

    __check = report_3;
    __check_len = sizeof(report_3) - 1;
    aker_metric_set_tz( "bar" );
    aker_metric_inc_device_block_count( 10 ); /* 40 */
    aker_metrics_report( 80 );
}



void add_suites( CU_pSuite *suite )
{
    printf("--------Start of Test Cases Execution For test_time ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "test_device_block_count", test_device_block_count);
    CU_add_test( *suite, "test_window_trans_count", test_window_trans_count);
    CU_add_test( *suite, "test_schedule_set_count", test_schedule_set_count);
    CU_add_test( *suite, "test_md5_err_count", test_md5_err_count);
    CU_add_test( *suite, "test_process_start_time", test_process_start_time);
    CU_add_test( *suite, "test_schedule_enabled", test_schedule_enabled);
    CU_add_test( *suite, "test_timezone", test_timezone);
    CU_add_test( *suite, "test_blocked_mac_count", test_blocked_mac_count);
    CU_add_test( *suite, "test_complete", test_complete);
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

