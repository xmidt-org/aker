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
#include <unistd.h>

#include <CUnit/Basic.h>

#include "mem_wrapper.h"
#include "../src/schedule.h"
#include "../src/aker_mem.h"
#include "../src/scheduler.h"
#include "../src/process_data.h"
#include "test_scheduler.h"

static int report_count;

pthread_t scheduler_thread_id;

const char data0[] = "\x84"
                        "\xab""report_rate" /* : */ "\xcd\x00\x03"
                        "\xa6""weekly"      /* : */ "\x93"
                            "\x82"
                                "\xa4""time"    /* : */ "\x0a"
                                "\xa7""indexes" /* : */
                                    "\x93"      /* [ */ "\x00" "\x01" "\x03" /* ] */
                            "\x82"
                                "\xa4""time"    /* : */ "\x14"
                                "\xa7""indexes" /* : */
                                    "\x91"      /* [ */ "\x00" /* ] */
                            "\x82"
                                "\xa4""time"    /* : */ "\x28"
                                "\xa7""indexes" /* : */
                                    "\x90"      /* [ ] */
                        "\xa4""macs"
                            "\x94"
                                "\xb1""11:22:33:44:55:aa"
                                "\xb1""22:33:44:55:66:bb"
                                "\xb1""33:44:55:66:77:cc"
                                "\xb1""44:55:66:77:88:dd"
                        "\xa7""index"
                            "\x92"  "\x00" "\x02";

const char data1[] = "\x84"
                        "\xab""report_rate" /* : */ "\xcd\x00\x00"
                        "\xa6""weekly"      /* : */ "\x93"
                            "\x82"
                                "\xa4""time"    /* : */ "\x0a"
                                "\xa7""indexes" /* : */
                                    "\x93"      /* [ */ "\x00" "\x01" "\x03" /* ] */
                            "\x82"
                                "\xa4""time"    /* : */ "\x14"
                                "\xa7""indexes" /* : */
                                    "\x91"      /* [ */ "\x00" /* ] */
                            "\x82"
                                "\xa4""time"    /* : */ "\x28"
                                "\xa7""indexes" /* : */
                                    "\x90"      /* [ ] */
                        "\xa4""macs"
                            "\x94"
                                "\xb1""11:22:33:44:55:aa"
                                "\xb1""22:33:44:55:66:bb"
                                "\xb1""33:44:55:66:77:cc"
                                "\xb1""44:55:66:77:88:dd"
                        "\xa7""index"
                            "\x92"  "\x00" "\x02";

/* Start the scheduler thread without any schedule data */
void test1()
{
    int result;
    result = scheduler_start( &scheduler_thread_id, " " );
    CU_ASSERT(0 == result);

    result = process_update( "scheduler_data.bin", "md5.md5", (void*) data0, sizeof(data0)-1 );
    CU_ASSERT(0 == result);

    sleep(15);

    result = process_update( "scheduler_data.bin", "md5.md5", (void*) data1, sizeof(data1)-1 );
    CU_ASSERT(0 == result);

    CU_ASSERT(4 <= report_count);
    CU_ASSERT(report_count <= 6);

    report_count = 0;

    sleep(5);

    CU_ASSERT(0 == report_count);

    terminate_scheduler_thread();
}

void add_suites( CU_pSuite *suite )
{
    printf("--------Start of Test Cases Execution For Scheduler---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Scheduler Test 1", test1);
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
   //sleep(10);
   //terminate_scheduler_thread();
   //pthread_kill(scheduler_thread_id, SIGTERM);
   return rv;
}

int32_t get_max_mac_limit()
{
    return 100;
}

void aker_metrics_report_to_log(void)
{
}

void aker_metrics_report(time_t time)
{
    printf( "report: %ld\n", time);
    report_count++;
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
