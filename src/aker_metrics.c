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
#include <pthread.h>
#include <time.h>

#include "aker_metrics.h"
#include "aker_log.h"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

static aker_metrics_t *g_metrics = NULL;
pthread_mutex_t aker_metrics_mut=PTHREAD_MUTEX_INITIALIZER;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Internal Functions                             */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

/* See aker_metrics.h for details. */
int init_global_metrics()
{
	g_metrics = calloc(1, sizeof(aker_metrics_t));
	
	if(g_metrics)
	{
		g_metrics->timezone = strdup("NULL");

	}
	else
	{
		debug_error("malloc failed in metrics\n");
		return 1;
	}

	return 0;
}

/* See aker_metrics.h for details. */
void aker_metric_inc_device_block_count( uint32_t val )
{
	pthread_mutex_lock(&aker_metrics_mut);

        g_metrics->device_block_count += val;

	pthread_mutex_unlock(&aker_metrics_mut);
}

/* See aker_metrics.h for details. */
void aker_metric_inc_window_trans_count()
{
	pthread_mutex_lock(&aker_metrics_mut);

	g_metrics->window_trans_count += 1;

	pthread_mutex_unlock(&aker_metrics_mut);
}

/* See aker_metrics.h for details. */
void aker_metric_inc_schedule_set_count()
{
	pthread_mutex_lock(&aker_metrics_mut);

	g_metrics->schedule_set_count += 1;

	pthread_mutex_unlock(&aker_metrics_mut);
}

/* See aker_metrics.h for details. */
void aker_metric_inc_md5_err_count()
{
	pthread_mutex_lock(&aker_metrics_mut);

	g_metrics->md5_err_count += 1;

	pthread_mutex_unlock(&aker_metrics_mut);
}

/* See aker_metrics.h for details. */
void aker_metric_set_process_start_time( time_t val )
{
	pthread_mutex_lock(&aker_metrics_mut);

	g_metrics->process_start_time = val;

	pthread_mutex_unlock(&aker_metrics_mut);
}

/* See aker_metrics.h for details. */
void aker_metric_set_schedule_enabled( int val )
{
	pthread_mutex_lock(&aker_metrics_mut);

	g_metrics->schedule_enabled = val;

	pthread_mutex_unlock(&aker_metrics_mut);
}

/* See aker_metrics.h for details. */
void aker_metric_set_tz( const char *val )
{
	pthread_mutex_lock(&aker_metrics_mut);

	if( g_metrics->timezone )
	{
		free(g_metrics->timezone);
	}

	g_metrics->timezone = strdup(val);

	pthread_mutex_unlock(&aker_metrics_mut);
}

/* See aker_metrics.h for details. */
void aker_metric_set_tz_offset( long int val )
{
	pthread_mutex_lock(&aker_metrics_mut);

	g_metrics->timezone_offset = val;

	pthread_mutex_unlock(&aker_metrics_mut);
}

/* See aker_metrics.h for details. */
void stringify_metrics(int flag)
{
	pthread_mutex_lock(&aker_metrics_mut);

	char str[512];
	aker_metrics_t * tmp = NULL;
	tmp = g_metrics;

	if(g_metrics != NULL)
	{
		snprintf(str, 512, "DeviceBlockCount,%d,"
                   "WindowTransistionCount,%d,"
                   "ScheduleSetCount,%d,"
                   "MD5ErrorCount,%d,"
                   "ProcessStartTime,%ld,"
                   "ScheduleEnabled,%d,"
                   "TimeZone,%s,"
                   "TimeZoneOffset,%+ld",

                   tmp->device_block_count,
                   tmp->window_trans_count,
                   tmp->schedule_set_count,
                   tmp->md5_err_count,
                   tmp->process_start_time,
                   tmp->schedule_enabled,
                   tmp->timezone,
                   tmp->timezone_offset);


		debug_info("The stringified valued is (%s)\n", str);

		if(flag)
		{

	#if defined(ENABLE_FEATURE_TELEMETRY2_0)
			t2_event_s("akermetrics", str);
			debug_info("akermetrics t2 event triggered\n");
	#endif
		}

	}
	else
	{
		debug_error("The g_metrics is NULL\n");
	}

	pthread_mutex_unlock(&aker_metrics_mut);
}

/* See aker_metrics.h for details. */
int get_blocked_mac_count(const char* blocked)
{
	int count = 0;
	const char *p = blocked;

	if(NULL != p)
	{
		/* If there are characters, then there there is 1+the number of spaces,
		* otherwise there are 0 blocked devices. */
		count = 1;

		while(*p)
		{
			if(' ' == *p)
			{
				count++;
			}
			p++;
		}
	}

	debug_info("the count is %d\n", count);
	debug_info("The mac after process were %s\n", blocked);
	return count;
}

/* See aker_metrics.h for details. */
void destroy_akermetrics()
{
	pthread_mutex_lock(&aker_metrics_mut);

	if( NULL != g_metrics )
	{
		if( NULL != g_metrics->timezone )
		{
			free( g_metrics->timezone );
		}

		free( g_metrics );
		g_metrics = NULL;
	}

	pthread_mutex_unlock(&aker_metrics_mut);
}
