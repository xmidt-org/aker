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

#ifndef __AKER_RBUS_H__
#define __AKER_RBUS_H__

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize RBUS for Aker notification telemetry
 * 
 * Registers the data model:
 *   Device.X_RDK_Aker.NotificationCount - uint32 counter
 * 
 * @return 0 on success, -1 on failure
 */
int aker_rbus_init(void);

/**
 * @brief Uninitialize and cleanup RBUS resources
 */
void aker_rbus_uninit(void);

/**
 * @brief Increment notification counter and publish value change event
 * 
 * This triggers T2 telemetry reports via TriggerCondition.
 * Should be called each time a notification is sent.
 */
void aker_rbus_increment_notification_count(void);

/**
 * @brief Reset notification counter to 0
 * 
 * Should be called when schedule changes to avoid overflow
 * and provide fresh start for new schedule.
 */
void aker_rbus_reset_notification_count(void);

/**
 * @brief Get current notification counter value
 * 
 * @return Current counter value
 */
uint32_t aker_rbus_get_notification_count(void);

#endif /* __AKER_RBUS_H__ */
