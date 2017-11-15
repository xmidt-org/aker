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


#include "../src/schedule.h"
#include "../src/scheduler.h"
#include "../src/process_data.h"
#include "test_scheduler.h"

#include "scheduler_data0.h"
#include "scheduler_data1.h"
#include "scheduler_data2.h"
#include "scheduler_data3.h"
#define MAX_WRP_TEST_MSGS 4

static wrp_msg_t wrp_test_msgs[MAX_WRP_TEST_MSGS];
// Tuesday, November 14, 2017 11:57:28 AM PST = 1510689448
static time_t kUnixCurrentTime = 1510689448;
static time_t add_time;

unsigned char *data_payloads[] = {
    scheduler_data0_bin,
    scheduler_data1_bin,
    scheduler_data2_bin,
    scheduler_data3_bin
};

#define TIME_SLOTS 5
uint32_t weekly_minute_offsets[MAX_WRP_TEST_MSGS][TIME_SLOTS] = {
    {10, 20, 30, 40, 35}, {10, 20, 30, 40, 50},
    {11, 23, 25, 43, 19}, {5, 11, 23, 37, 40}
};

pthread_t shceduler_thread_id;
const char *firewall_cmd = " ";
const char *file_name = "scheduler_data.bin";
const char *md5_file  = "md5.md5";

void initialize_wrp_messages(void);

/* Start the scheduler thread without any schedule data */
void test1()
{
    int result;
    result = scheduler_start( &shceduler_thread_id, firewall_cmd );
    CU_ASSERT(0 == result);
}


void test2()
{
    ssize_t result;
    int cnt;
    
    for (cnt = 0; cnt < MAX_WRP_TEST_MSGS; cnt++) {
        result = process_message_cu( file_name, md5_file, &wrp_test_msgs[cnt] );  
        CU_ASSERT(0 < result);
    }

    add_time = -214980;
    result = process_message_cu( file_name, md5_file, &wrp_test_msgs[0] );  
    CU_ASSERT(0 < result);

    add_time = -214500;
    result = process_message_cu( file_name, md5_file, &wrp_test_msgs[1] );  
    CU_ASSERT(0 < result);    

    add_time = -213000;
    result = process_message_cu( file_name, md5_file, &wrp_test_msgs[2] );  
    CU_ASSERT(0 < result);

    add_time = -212900;
    result = process_message_cu( file_name, md5_file, &wrp_test_msgs[3] );
    CU_ASSERT(0 < result);

    add_time = 0; // allow absolute to take effect ?
    result = process_message_cu( file_name, md5_file, &wrp_test_msgs[3] );  
    CU_ASSERT(0 < result);
    
    add_time = -212790;
    result = process_message_cu( file_name, md5_file, &wrp_test_msgs[0] );
    CU_ASSERT(0 < result);
}


void add_suites( CU_pSuite *suite )
{
    printf("--------Start of Test Cases Execution For Scheduler---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Scheduler Test 1", test1);
    CU_add_test( *suite, "Scheduler Test 2", test2);
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
    
    initialize_wrp_messages();
    
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
   //pthread_kill(shceduler_thread_id, SIGTERM);
   return rv;
}

void initialize_wrp_messages(void)
{
    uint32_t data_sizes[] = {
    scheduler_data0_bin_len,
    scheduler_data1_bin_len,
    scheduler_data2_bin_len,
    scheduler_data3_bin_len
    };
    int cnt = 0;

    for (; cnt < MAX_WRP_TEST_MSGS; cnt++) {
        memset(&wrp_test_msgs[cnt], 0, sizeof(wrp_msg_t));
        wrp_test_msgs[cnt].u.crud.payload_size = data_sizes[cnt];
        wrp_test_msgs[cnt].u.crud.payload      = data_payloads[cnt];
    }
    
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

