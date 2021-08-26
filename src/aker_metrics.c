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

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "aker_metrics.h"
#include "aker_log.h"

static aker_metrics_t *g_metrics = NULL;
static bool intialised_flag = false;

/*aker_metrics_t* get_global_metrics(void)
{
	aker_metrics_t* tmp = NULL;
	tmp = g_metrics;
	return tmp;
}*/

int init_global_metrics()
{
	aker_metrics_t* metrics;
	metrics = (aker_metrics_t*)malloc(sizeof(aker_metrics_t));
	
	if(metrics)
	{
		memset(metrics, 0, sizeof(aker_metrics_t));

		metrics->device_block_count = 0;
		metrics->windows_transistion_count = 0;
		metrics->schedule_set_count = 0;
		metrics->md5_error_count = 0;
		metrics->process_start_time = 0;
		metrics->schedule_enabled = 0;
		metrics->timezone = NULL;
		intialised_flag = true;

	}
	else
	{
		debug_error("malloc failed in metrics\n");
		return 0;
	}

	if(g_metrics == NULL)
	{
		g_metrics = metrics;
	}

	return 1;
}
int set_aker_metrics(int metrics,int num, ... )
{
	
	aker_metrics_t * tmp = NULL;

	if(!intialised_flag)
	{
		init_global_metrics();
	}

	tmp = g_metrics;

	va_list valist;
	va_start(valist, num);

	switch(metrics)
	{
		case 0:
			debug_info("Before changing tmp->device_block_count %d\n", tmp->device_block_count);
			tmp->device_block_count += va_arg(valist, uint32_t);
			va_end(valist);
			debug_info("After changing tmp->device_block_count %d\n", tmp->device_block_count);
			break;

		case 1:
			debug_info("Before changing tmp->windows_transistion_count %d\n", tmp->windows_transistion_count);
			tmp->windows_transistion_count += va_arg(valist, uint32_t);
			va_end(valist);
			debug_info("After changing tmp->windows_transistion_count %d\n", tmp->windows_transistion_count);
			break;

		case 2:
			debug_info("Before changing tmp->schedule_set_count %d\n", tmp->schedule_set_count);
			tmp->schedule_set_count += va_arg(valist, uint32_t);
			va_end(valist);
			debug_info("After changing tmp->schedule_set_count %d\n", tmp->schedule_set_count);
			break;

		case 3:
			debug_info("Before changing tmp->md5_error_count %d\n", tmp->md5_error_count);
			tmp->md5_error_count += va_arg(valist, uint32_t);
			va_end(valist);
			debug_info("After changing tmp->md5_error_count %d\n", tmp->md5_error_count);
			break;

		case 4:
			debug_info("Before changing tmp->process_start_time %ld\n", (long)tmp->process_start_time);
			tmp->process_start_time = va_arg(valist, time_t);
			va_end(valist);
			debug_info("After changing tmp->process_start_time %ld\n", (long)tmp->process_start_time);
			break;

		case 5:
			debug_info("Before changing tmp->schedule_enabled %d\n", tmp->schedule_enabled);
			tmp->schedule_enabled = va_arg(valist, int);
			va_end(valist);
			debug_info("After changing tmp->schedule_enabled %d\n", tmp->schedule_enabled);
			break;

		case 6:
			if(tmp->timezone != NULL)
			{
				debug_info("Before changing tmp->timezone %d\n", tmp->timezone);
				tmp->timezone = NULL;
				tmp->timezone = strdup(va_arg(valist, char*));
				va_end(valist);
				debug_info("After changing tmp->timezone %d\n", tmp->timezone);
			}
			else
			{
				debug_info("Inside timezone NULL\n");
				tmp->timezone = strdup(va_arg(valist, char*));
				va_end(valist);
				debug_info("After changing tmp->timezone %s\n", tmp->timezone);
			}
			break;

		default:
			debug_error("The mentioned metrics is not found\n");
			return 0;
	}
	return 1;	
}

int stringify_metrics()
{
	char str[512];
	aker_metrics_t * tmp = NULL;
	tmp = g_metrics;

	if(g_metrics != NULL)
	{
		snprintf(str, 512, "%s%d%s%d%s%d%s%d%s%ld%s%d%s%s", "DeviceBlockCount,", tmp->device_block_count,
                                                            ",WindowTransistionCount,", tmp->windows_transistion_count,
                                                            ",ScheduleSetCount,", tmp->schedule_set_count,
                                                            ",MD5ErrorCount,", tmp->md5_error_count,
                                                            ",ProcessStartTime,", tmp->process_start_time,
                                                            ",ScheduleEnabled,", tmp->schedule_enabled,
                                                            ",TimeZone,", tmp->timezone);

		debug_info("The stringified valued is (%s)\n", str);
	}
	else
	{
		debug_error("The g_metrics is NULL\n");
		return 0;
	}

	return 1;
}

