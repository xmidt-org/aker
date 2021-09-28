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
#include <msgpack.h>
#include <wrp-c/wrp-c.h>
#include <libparodus.h>

#if defined(ENABLE_FEATURE_TELEMETRY2_0)
   #include <telemetry_busmessage_sender.h>
#endif

#include "aker_metrics.h"
#include "aker_log.h"
#include "aker_mem.h"


/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define MAX_TIMEZONE    255
#define MAKE_TOKEN(str)                  \
    {                                    \
        .s = str, .len = sizeof(str) - 1 \
    }

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
struct aker_metrics
{
	time_t snapshot;                /* only used for old samples */

	uint32_t device_block_count;
	uint32_t window_trans_count;
	uint32_t schedule_set_count;
	uint32_t md5_err_count;
	int schedule_enabled;
	char timezone[MAX_TIMEZONE+1];
	long int timezone_offset;
};

struct metric_label {
    const char *s;
    size_t len;
};

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
const struct metric_label METRIC_PS__ = MAKE_TOKEN( "ps" );
const struct metric_label METRIC_ID__ = MAKE_TOKEN( "id" );
const struct metric_label METRIC_ROWS = MAKE_TOKEN( "rows" );
const struct metric_label METRIC_TS__ = MAKE_TOKEN( "ts" );
const struct metric_label METRIC_OFF_ = MAKE_TOKEN( "off" );
const struct metric_label METRIC_DBC_ = MAKE_TOKEN( "dbc" );
const struct metric_label METRIC_WTC_ = MAKE_TOKEN( "wtc" );
const struct metric_label METRIC_SSC_ = MAKE_TOKEN( "ssc" );
const struct metric_label METRIC_MD5_ = MAKE_TOKEN( "md5" );
const struct metric_label METRIC_TZ__ = MAKE_TOKEN( "tz" );

static time_t g_process_start_time;
static const char *g_device_id;
static libpd_instance_t g_libpd;
static struct aker_metrics g_metrics;
pthread_mutex_t aker_metrics_mut=PTHREAD_MUTEX_INITIALIZER;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Internal Functions                             */
/*----------------------------------------------------------------------------*/
static void pack_uint32(msgpack_packer *pk, const struct metric_label *l, uint32_t d)
{
    msgpack_pack_str(pk, l->len );
    msgpack_pack_str_body(pk, l->s, l->len );
    msgpack_pack_unsigned_int(pk, d);
}

static void pack_long__(msgpack_packer *pk, const struct metric_label *l, long d)
{
    msgpack_pack_str(pk, l->len );
    msgpack_pack_str_body(pk, l->s, l->len );
    msgpack_pack_long(pk, d);
}

static void pack_label(msgpack_packer *pk, const struct metric_label *l)
{
    msgpack_pack_str(pk, l->len );
    msgpack_pack_str_body(pk, l->s, l->len );
}

static void pack_string(msgpack_packer *pk, const struct metric_label *l, const char*s)
{
    size_t len = strlen(s);

    pack_label(pk, l);
    msgpack_pack_str(pk, len );
    msgpack_pack_str_body(pk, s, len );
}

static void pack_row_map(msgpack_packer *pk, const struct aker_metrics *m)
{
    msgpack_pack_map(pk, 7);
    pack_long__(pk, &METRIC_TS__, m->snapshot);
    pack_long__(pk, &METRIC_OFF_, m->timezone_offset);
    pack_uint32(pk, &METRIC_DBC_, m->device_block_count);
    pack_uint32(pk, &METRIC_WTC_, m->window_trans_count);
    pack_uint32(pk, &METRIC_SSC_, m->schedule_set_count);
    pack_uint32(pk, &METRIC_MD5_, m->md5_err_count);
    pack_string(pk, &METRIC_TZ__, &m->timezone[0]);
}

static void pack_rows(msgpack_packer *pk, const struct aker_metrics *m, size_t count)
{
    pack_label(pk, &METRIC_ROWS);
    msgpack_pack_array(pk, count);
    for(size_t i = 0; i < count; i++) {
        pack_row_map(pk, &m[i]);
    }
}

static void build_and_send_wrp( time_t time )
{
    static int valid_count = 0;
    static struct aker_metrics old[3];
    msgpack_sbuffer sbuf;
    msgpack_packer pk;
    wrp_msg_t msg;
    char source[256];

    memset(&msg, 0, sizeof(wrp_msg_t));

    /* Only send the number of valid rows to prevent bogus metrics data
     * from being sent. */
    valid_count++;
    if( 3 < valid_count ) {
        valid_count = 3;
    }

    /* Record the time of this snapshot so it is consistent going forward. */
    g_metrics.snapshot = time;

    /* Rotate the 3 we're keeping for redundancy */
    memcpy(&old[2], &old[1], sizeof(struct aker_metrics));
    memcpy(&old[1], &old[0], sizeof(struct aker_metrics));
    memcpy(&old[0], &g_metrics, sizeof(struct aker_metrics));

    /* Build the msgpack payload */
    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
    msgpack_pack_map(&pk, 3);

    pack_long__(&pk, &METRIC_PS__, g_process_start_time);
    pack_string(&pk, &METRIC_ID__, g_device_id);
    pack_rows(&pk, old, valid_count);

    /* Build the source since it should be mac:000000000000/aker */
    snprintf( source, 256, "%s/aker", g_device_id );

    /* Populate the rest of the wrp for the event. */
    msg.msg_type = WRP_MSG_TYPE__EVENT;
    msg.u.event.content_type = "application/msgpack";
    msg.u.event.source = source;
    msg.u.event.dest = "event:metrics.aker";
    msg.u.event.payload = sbuf.data;
    msg.u.event.payload_size = sbuf.size;

    /* Send it if we can. */
    if( g_libpd ) {
        libparodus_send(g_libpd, &msg);
    }

    /* Clean up */
    msgpack_sbuffer_destroy(&sbuf);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
void aker_metric_init( const char *device_id, libpd_instance_t libpd )
{
    g_device_id = device_id;
    g_libpd = libpd;
}

/* See aker_metrics.h for details. */
void aker_metric_inc_device_block_count( uint32_t val )
{
	pthread_mutex_lock(&aker_metrics_mut);

	g_metrics.device_block_count += val;

	pthread_mutex_unlock(&aker_metrics_mut);
}

/* See aker_metrics.h for details. */
void aker_metric_inc_window_trans_count()
{
	pthread_mutex_lock(&aker_metrics_mut);

	g_metrics.window_trans_count += 1;

	pthread_mutex_unlock(&aker_metrics_mut);
}

/* See aker_metrics.h for details. */
void aker_metric_inc_schedule_set_count()
{
	pthread_mutex_lock(&aker_metrics_mut);

	g_metrics.schedule_set_count += 1;

	pthread_mutex_unlock(&aker_metrics_mut);
}

/* See aker_metrics.h for details. */
void aker_metric_inc_md5_err_count()
{
	pthread_mutex_lock(&aker_metrics_mut);

	g_metrics.md5_err_count += 1;

	pthread_mutex_unlock(&aker_metrics_mut);
}

/* See aker_metrics.h for details. */
void aker_metric_set_process_start_time( time_t val )
{
	pthread_mutex_lock(&aker_metrics_mut);

	g_process_start_time = val;

	pthread_mutex_unlock(&aker_metrics_mut);
}

/* See aker_metrics.h for details. */
void aker_metric_set_schedule_enabled( int val )
{
	pthread_mutex_lock(&aker_metrics_mut);

	g_metrics.schedule_enabled = val;

	pthread_mutex_unlock(&aker_metrics_mut);
}

/* See aker_metrics.h for details. */
void aker_metric_set_tz( const char *val )
{
	pthread_mutex_lock(&aker_metrics_mut);

	if( val )
	{
        size_t len = strlen(val);
        if( MAX_TIMEZONE < len ) {
            len = MAX_TIMEZONE;
        }
        memcpy(&g_metrics.timezone[0], val, len);
        g_metrics.timezone[len] = '\0';
	} else {
        memset(&g_metrics.timezone[0], 0, MAX_TIMEZONE);
    }

	pthread_mutex_unlock(&aker_metrics_mut);
}

/* See aker_metrics.h for details. */
void aker_metric_set_tz_offset( long int val )
{
	pthread_mutex_lock(&aker_metrics_mut);

	g_metrics.timezone_offset = val;

	pthread_mutex_unlock(&aker_metrics_mut);
}

/* See aker_metrics.h for details. */
void aker_metrics_report_to_log()
{
	char str[512];

	pthread_mutex_lock(&aker_metrics_mut);
    
	snprintf(str, 512, "DeviceBlockCount,%d,"
	                   "WindowTransistionCount,%d,"
	                   "ScheduleSetCount,%d,"
	                   "MD5ErrorCount,%d,"
	                   "ProcessStartTime,%ld,"
	                   "ScheduleEnabled,%d,"
	                   "TimeZone,%s,"
	                   "TimeZoneOffset,%+ld",

	                   g_metrics.device_block_count,
	                   g_metrics.window_trans_count,
	                   g_metrics.schedule_set_count,
	                   g_metrics.md5_err_count,
	                   g_process_start_time,
	                   g_metrics.schedule_enabled,
	                   ('\0' == g_metrics.timezone[0]) ? "NULL" : g_metrics.timezone,
	                   g_metrics.timezone_offset);
	pthread_mutex_unlock(&aker_metrics_mut);

	debug_info("The stringified value is (%s)\n", str);

#if defined(ENABLE_FEATURE_TELEMETRY2_0)
    t2_event_s("Akermetrics", str);
    debug_info("Akermetrics t2 event triggered\n");
#endif
}

void aker_metrics_report(time_t now)
{
	pthread_mutex_lock(&aker_metrics_mut);
	build_and_send_wrp(now);
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

	debug_print("the count is %d\n", count);
	debug_print("The mac after process were %s\n", blocked);
	return count;
}

/* See aker_metrics.h for details. */
void destroy_akermetrics()
{
	pthread_mutex_lock(&aker_metrics_mut);
    memset(&g_metrics, 0, sizeof(struct aker_metrics));
	pthread_mutex_unlock(&aker_metrics_mut);
}
