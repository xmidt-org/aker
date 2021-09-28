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

#include "mem_wrapper.h"
#include "../src/schedule.h"
#include "../src/aker_mem.h"
#include "../src/scheduler.h"
#include "../src/process_data.h"
#include "test_scheduler.h"

#include "scheduler_data0.h"
#include "scheduler_data1.h"
#include "scheduler_data2.h"
#include "scheduler_data3.h"
#define MAX_WRP_TEST_MSGS 4

// Tuesday, November 14, 2017 11:57:28 AM PST = 1510689448
static time_t kUnixCurrentTime = 1510689448;
static time_t add_time;

unsigned char *data_payloads[] = {
    scheduler_data0_bin,
    scheduler_data1_bin,
    scheduler_data2_bin,
    scheduler_data3_bin
};

uint32_t data_sizes[] = {
    scheduler_data0_bin_len,
    scheduler_data1_bin_len,
    scheduler_data2_bin_len,
    scheduler_data3_bin_len
};

pthread_t scheduler_thread_id;
const char *firewall_cmd = " ";
const char *file_name = "scheduler_data.bin";
const char *md5_file  = "md5.md5";

/* Start the scheduler thread without any schedule data */
void test1()
{
    int result;
    result = scheduler_start( &scheduler_thread_id, firewall_cmd );
    CU_ASSERT(0 == result);
}


void test2()
{
    int result;
    int cnt;
    
    for (cnt = 0; cnt < MAX_WRP_TEST_MSGS; cnt++) {
        result = process_update( file_name, md5_file, data_payloads[cnt], data_sizes[cnt] );
        printf( "got: %d\n", result );
        CU_ASSERT(0 == result);
    }
    malloc_fail = true;
    malloc_failure_limit = 32;
     for (cnt = 0; cnt < MAX_WRP_TEST_MSGS; cnt++) {
        result = process_update( file_name, md5_file, data_payloads[cnt], data_sizes[cnt] );
        CU_ASSERT(0 >= result);
    }

    malloc_fail = false;

    add_time = -214980;
    result = process_update( file_name, md5_file, data_payloads[0], data_sizes[0] );
    CU_ASSERT(0 == result);

    add_time = -214500;
    result = process_update( file_name, md5_file, data_payloads[1], data_sizes[1] );
    CU_ASSERT(0 == result);    

    add_time = -213000;
    result = process_update( file_name, md5_file, data_payloads[2], data_sizes[2] );
    CU_ASSERT(0 == result);

    add_time = -212900;
    result = process_update( file_name, md5_file, data_payloads[3], data_sizes[3] );
    CU_ASSERT(0 == result);

    add_time = -212800;
    result = process_delete( file_name, md5_file );
    CU_ASSERT(0 == result);

    add_time = 0; // allow absolute to take effect ?
    result = process_update( file_name, md5_file, data_payloads[3], data_sizes[3] );
    CU_ASSERT(0 == result);
    
    add_time = -212790;
    result = process_update( file_name, md5_file, data_payloads[0], data_sizes[0] );
    CU_ASSERT(0 == result);
}

void test3()
{
    // Just cover process_message_ret_now()->get_current_blocked_macs()
    uint8_t *data;
    ssize_t cnt;

    cnt = process_retrieve_now(&data);

    CU_ASSERT(cnt >= 0);
    CU_ASSERT(data != NULL);
    if( NULL != data ) {
        aker_free( data );
    }
}

void add_suites( CU_pSuite *suite )
{
    printf("--------Start of Test Cases Execution For Scheduler---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Scheduler Test 1", test1);
    CU_add_test( *suite, "Scheduler Test 2", test2);
    CU_add_test( *suite, "Scheduler Test 3", test3);
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

   sleep(1);
   terminate_scheduler_thread();
   //pthread_kill(scheduler_thread_id, SIGTERM);
   return rv;
}


void aker_metrics_report_to_log(void)
{
}


void aker_metrics_report(time_t time)
{
    (void) time;
}

void aker_metric_set_schedule_enabled(int val)
{
    (void) val;
}

void aker_metric_set_tz(const char *val)
{
    (void) val;
}

void aker_metric_set_tz_offset(long int val)
{
    (void) val;
}

void aker_metric_inc_device_block_count(uint32_t val)
{
    (void) val;
}

void aker_metric_inc_window_trans_count()
{
}

void aker_metric_inc_schedule_set_count()
{
}

void destroy_akermetrics()
{
}

int get_blocked_mac_count(const char* blocked)
{
    (void) blocked;
    return 0;
}

time_t convert_unix_time_to_weekly(time_t unixtime)
{
    time_t seconds_since_sunday_midnght;
    time_t t = unixtime;
    struct tm ts;

    ts = *localtime(&t);

    seconds_since_sunday_midnght = (ts.tm_wday * 24 * 3600) +
            (ts.tm_hour * 3600) +
            (ts.tm_min * 60) +
            ts.tm_sec;


    return seconds_since_sunday_midnght;
}

time_t get_unix_time(void)
{
    return add_time + kUnixCurrentTime;
}

int32_t get_max_mac_limit(void)
{
    return 2048;
}
