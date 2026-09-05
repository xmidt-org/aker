/**
 *  Copyright 2026 Comcast Cable Communications Management, LLC
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
#include <stdio.h>

#include <CUnit/Basic.h>
#include <rbus.h>

#include "test_macros.h"
#include "../src/aker_rbus.h"

/* Not part of the public API (aker_rbus.h): declared non-static in aker_rbus.c
 * so these RBUS data-model callbacks can be exercised without a live broker. */
rbusError_t notification_count_get_handler(
    rbusHandle_t handle,
    rbusProperty_t property,
    rbusGetHandlerOptions_t* opts);

rbusError_t notification_count_set_handler(
    rbusHandle_t handle,
    rbusProperty_t property,
    rbusSetHandlerOptions_t* opts);

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

/* aker_rbus_init() is intentionally never called here - it opens a real RBUS
 * connection. The counter functions must work purely on local state without
 * an active RBUS handle, since scheduler code can call them before/without
 * RBUS being available. */
void test_notification_count_reset_and_get(void)
{
    aker_rbus_reset_notification_count();
    CU_ASSERT_EQUAL(aker_rbus_get_notification_count(), 0);
}

void test_notification_count_increment(void)
{
    uint32_t before;

    aker_rbus_reset_notification_count();
    before = aker_rbus_get_notification_count();

    aker_rbus_increment_notification_count();
    CU_ASSERT_EQUAL(aker_rbus_get_notification_count(), before + 1);

    aker_rbus_increment_notification_count();
    aker_rbus_increment_notification_count();
    CU_ASSERT_EQUAL(aker_rbus_get_notification_count(), before + 3);

    aker_rbus_reset_notification_count();
    CU_ASSERT_EQUAL(aker_rbus_get_notification_count(), 0);
}

/* aker_rbus_uninit() guards on the "never initialized" case with no RBUS I/O
 * at all (no rbus_close call), so it is safe to exercise directly here. This
 * only covers the early-return guard clause, not the rbus_close() path, which
 * requires a live handle from a successful aker_rbus_init(). */
void test_uninit_when_not_initialized(void)
{
    aker_rbus_uninit();
    aker_rbus_uninit();
}

/* rbusValue_t/rbusProperty_t are pure in-memory constructs (no broker needed),
 * so the GET handler's mutex-protected read can be exercised directly. */
void test_get_handler_returns_current_count(void)
{
    rbusValue_t value;
    rbusProperty_t property;
    rbusError_t rc;

    aker_rbus_reset_notification_count();
    aker_rbus_increment_notification_count();
    aker_rbus_increment_notification_count();

    rbusValue_Init(&value);
    rbusProperty_Init(&property, "Device.X_RDK_Aker.NotificationCount", value);
    rbusValue_Release(value);

    rc = notification_count_get_handler(NULL, property, NULL);
    CU_ASSERT_EQUAL(rc, RBUS_ERROR_SUCCESS);
    CU_ASSERT_EQUAL(rbusValue_GetUInt32(rbusProperty_GetValue(property)), 2);

    rbusProperty_Release(property);
}

/* SET is only allowed for reset-to-0 (administrative); any other value must
 * be rejected and must not change the counter. */
void test_set_handler_rejects_nonzero(void)
{
    rbusValue_t value;
    rbusProperty_t property;
    rbusError_t rc;

    aker_rbus_reset_notification_count();
    aker_rbus_increment_notification_count();

    rbusValue_Init(&value);
    rbusValue_SetUInt32(value, 5);
    rbusProperty_Init(&property, "Device.X_RDK_Aker.NotificationCount", value);
    rbusValue_Release(value);

    rc = notification_count_set_handler(NULL, property, NULL);
    CU_ASSERT_EQUAL(rc, RBUS_ERROR_INVALID_INPUT);
    CU_ASSERT_EQUAL(aker_rbus_get_notification_count(), 1);

    rbusProperty_Release(property);
}

void test_set_handler_allows_zero_reset(void)
{
    rbusValue_t value;
    rbusProperty_t property;
    rbusError_t rc;

    aker_rbus_reset_notification_count();
    aker_rbus_increment_notification_count();

    rbusValue_Init(&value);
    rbusValue_SetUInt32(value, 0);
    rbusProperty_Init(&property, "Device.X_RDK_Aker.NotificationCount", value);
    rbusValue_Release(value);

    rc = notification_count_set_handler(NULL, property, NULL);
    CU_ASSERT_EQUAL(rc, RBUS_ERROR_SUCCESS);
    CU_ASSERT_EQUAL(aker_rbus_get_notification_count(), 0);

    rbusProperty_Release(property);
}

void add_suites( CU_pSuite *suite )
{
    printf("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test notification count reset/get", test_notification_count_reset_and_get );
    CU_add_test( *suite, "Test notification count increment", test_notification_count_increment );
    CU_add_test( *suite, "Test uninit when not initialized is a safe no-op", test_uninit_when_not_initialized );
    CU_add_test( *suite, "Test GET handler returns current count", test_get_handler_returns_current_count );
    CU_add_test( *suite, "Test SET handler rejects non-zero value", test_set_handler_rejects_nonzero );
    CU_add_test( *suite, "Test SET handler allows reset to zero", test_set_handler_allows_zero_reset );
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
