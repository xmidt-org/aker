/**
 * Copyright 2026 Comcast Cable Communications Management, LLC
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

#include "aker_rbus.h"
#include "aker_log.h"
#include <rbus.h>
#include <pthread.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define AKER_RBUS_COMPONENT_NAME "Aker"
#define AKER_NOTIFICATION_COUNT_DM "Device.X_RDK_Aker.NotificationCount"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static rbusHandle_t g_rbus_handle = NULL;
static uint32_t g_notification_count = 0;
static pthread_mutex_t g_count_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_rbus_initialized = false;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static rbusError_t notification_count_get_handler(
    rbusHandle_t handle,
    rbusProperty_t property,
    rbusGetHandlerOptions_t* opts);

static rbusError_t notification_count_set_handler(
    rbusHandle_t handle,
    rbusProperty_t property,
    rbusSetHandlerOptions_t* opts);

/*----------------------------------------------------------------------------*/
/*                             Internal Functions                             */
/*----------------------------------------------------------------------------*/

/**
 * @brief RBUS GET handler for NotificationCount property
 */
static rbusError_t notification_count_get_handler(
    rbusHandle_t handle,
    rbusProperty_t property,
    rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;

    rbusValue_t value;
    uint32_t count;

    pthread_mutex_lock(&g_count_mutex);
    count = g_notification_count;
    pthread_mutex_unlock(&g_count_mutex);

    rbusValue_Init(&value);
    rbusValue_SetUInt32(value, count);
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);

    debug_print("aker_rbus: GET NotificationCount=%u\n", count);
    return RBUS_ERROR_SUCCESS;
}

/**
 * @brief RBUS SET handler for NotificationCount property
 * 
 * Note: Setting is only allowed for reset to 0 (administrative purposes)
 */
static rbusError_t notification_count_set_handler(
    rbusHandle_t handle,
    rbusProperty_t property,
    rbusSetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;

    rbusValue_t value = rbusProperty_GetValue(property);
    uint32_t new_value = rbusValue_GetUInt32(value);

    /* Only allow setting to 0 (reset) */
    if (new_value != 0)
    {
        debug_error("aker_rbus: NotificationCount can only be set to 0 (reset)\n");
        return RBUS_ERROR_INVALID_INPUT;
    }

    pthread_mutex_lock(&g_count_mutex);
    g_notification_count = 0;
    pthread_mutex_unlock(&g_count_mutex);

    debug_info("aker_rbus: NotificationCount reset to 0 via SET\n");
    return RBUS_ERROR_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int aker_rbus_init(void)
{
    rbusError_t rc;

    if (g_rbus_initialized)
    {
        debug_info("aker_rbus: Already initialized\n");
        return 0;
    }

    /* Register data elements */
    rbusDataElement_t dataElements[] = {
        {
            AKER_NOTIFICATION_COUNT_DM,
            RBUS_ELEMENT_TYPE_PROPERTY,
            {
                notification_count_get_handler,  /* getHandler */
                notification_count_set_handler,  /* setHandler */
                NULL,                            /* tableAddRowHandler */
                NULL,                            /* tableRemoveRowHandler */
                NULL,                            /* eventSubHandler */
                NULL                             /* methodHandler */
            }
        }
    };

    /* Open RBUS connection */
    rc = rbus_open(&g_rbus_handle, AKER_RBUS_COMPONENT_NAME);
    if (rc != RBUS_ERROR_SUCCESS)
    {
        debug_error("aker_rbus_init: rbus_open failed: %d\n", rc);
        return -1;
    }

    /* Register data model elements */
    rc = rbus_regDataElements(g_rbus_handle, 1, dataElements);
    if (rc != RBUS_ERROR_SUCCESS)
    {
        debug_error("aker_rbus_init: rbus_regDataElements failed: %d\n", rc);
        rbus_close(g_rbus_handle);
        g_rbus_handle = NULL;
        return -1;
    }

    g_rbus_initialized = true;
    debug_info("aker_rbus_init: RBUS initialized successfully\n");
    debug_info("aker_rbus_init: Registered data model: %s\n", AKER_NOTIFICATION_COUNT_DM);

    return 0;
}

void aker_rbus_uninit(void)
{
    if (!g_rbus_initialized)
    {
        return;
    }

    if (g_rbus_handle)
    {
        rbus_close(g_rbus_handle);
        g_rbus_handle = NULL;
    }

    g_rbus_initialized = false;
    debug_info("aker_rbus_uninit: RBUS uninitialized\n");
}

void aker_rbus_increment_notification_count(void)
{
    uint32_t new_count;

    pthread_mutex_lock(&g_count_mutex);

    /* Increment with overflow protection */
    if (g_notification_count == UINT32_MAX)
    {
        g_notification_count = 0;  /* Wrap around on overflow */
        debug_info("aker_rbus: NotificationCount wrapped around from UINT32_MAX to 0\n");
    }
    else
    {
        g_notification_count++;
    }

    new_count = g_notification_count;
    pthread_mutex_unlock(&g_count_mutex);

    debug_print("aker_rbus: NotificationCount incremented to %u\n", new_count);
}

void aker_rbus_reset_notification_count(void)
{
    pthread_mutex_lock(&g_count_mutex);
    g_notification_count = 0;
    pthread_mutex_unlock(&g_count_mutex);

    debug_info("aker_rbus: NotificationCount reset to 0\n");
}

uint32_t aker_rbus_get_notification_count(void)
{
    uint32_t count;

    pthread_mutex_lock(&g_count_mutex);
    count = g_notification_count;
    pthread_mutex_unlock(&g_count_mutex);

    return count;
}
