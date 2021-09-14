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
#ifndef __AKER_METRICS_H__
#define __AKER_METRICS_H__

#include <stdint.h>
#include <stdarg.h>
#include "time.h"

#if defined(ENABLE_FEATURE_TELEMETRY2_0)
   #include <telemetry_busmessage_sender.h>
#endif

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

typedef struct aker_metrics
{

	uint32_t device_block_count;
	uint32_t window_trans_count;
	uint32_t schedule_set_count;
	uint32_t md5_err_count;
	time_t process_start_time;
	int schedule_enabled;
	char* timezone;
	signed int timezone_offset;

}aker_metrics_t;

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

/**
 *  Sets the device_block_count in the global g_metrics
 *
 *  @param val        the number of devices blocked
 *
 */
void aker_metric_inc_device_block_count( uint32_t val );


/* Sets the window transition count in the global g_metrics incremented by 1 */
void aker_metric_inc_window_trans_count();


/* Sets the schedule set count in the global g_metrics incremented by 1 */
void aker_metric_inc_schedule_set_count();


/* Sets the md5 error count in the global g_metrics incremented by 1 */
void aker_metric_inc_md5_err_count();


/**
 *  Sets the Aker process_start_time in the global g_metrics
 *
 *  @param val        the time_t value at which the Aker process started
 *
 */
void aker_metric_set_process_start_time( time_t val );

/**
 *  Sets the schedule_enabled value in the global g_metrics
 *
 *  @param val        1 for schedule is set and 0 for not set
 *
 */
void aker_metric_set_schedule_enabled( int val );

/**
 *  Sets the timezone value in the global g_metrics
 *
 *  @param val        the TimeZone set in string
 *
 */
void aker_metric_set_tz( const char *val );


/**
 *  Sets the timezone offset value in the global g_metrics
 *
 *  @param val        the timezone offset value
 *
 */
void aker_metric_set_tz_offset( signed int val );


/* Initializes the g_metrics values*/
int init_global_metrics();


/**
 *  Sends the aker metrics values with names into a single
 *  comma separate value via Telemetry event.
 *
 *  @param flag        1 to trigger an event and 0 to not trigger an event
 *
 */
void stringify_metrics(int flag);


/**
 *  Gives the number of devices blocked
 *  comma separate value via Telemetry event.
 *
 *  @param blocked      pass the blocked macs separated by space
 *
 *  @return the total number of devices blocked
 */
int get_blocked_mac_count(const char* blocked);


/* Destroys the g_metrics already intialized. */
void destroy_akermetrics();


#endif

