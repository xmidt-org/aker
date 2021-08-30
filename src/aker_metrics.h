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

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

#define DBC	0		//for setting device_block_count
#define WTC	1		//for setting windows_transistion_count
#define SSC	2		//for setting schedule_set_count
#define MEC	3		//for setting md5_error_count
#define PST	4		//for setting process_start_time
#define SE	5		//for setting schedule_enabled
#define TZ	6		//for setting timezone

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

typedef struct aker_metrics
{

	uint32_t device_block_count;
	uint32_t windows_transistion_count;
	uint32_t schedule_set_count;
	uint32_t md5_error_count;
	time_t process_start_time;
	int schedule_enabled;
	char* timezone;

}aker_metrics_t;

int init_global_metrics();

int stringify_metrics();

int set_aker_metrics(int metrics,int num, ... );

int get_blocked_mac_count(char* blocked);
#endif

