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
#ifndef __AKER_NOTIFICATION_H__
#define __AKER_NOTIFICATION_H__

#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include "schedule.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define NOTIFICATION_ADVANCE_TIME_SEC     900   /* 15 minutes before event */
#define NOTIFICATION_LATE_THRESHOLD_SEC   5     /* Max seconds late to still send notification */
#define SCHEDULED_TIME_TOLERANCE_SEC      60    /* Tolerance for timeline comparison */
#define MAX_WEEKS_AHEAD                   2     /* Build timeline 2 weeks ahead */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

/**
 * Notification types as per acceptance criteria
 */
typedef enum {
    NOTIFY_DOWNTIME_STARTING_SOON,    /* 15 min before blocking starts */
    NOTIFY_DOWNTIME_STARTED,          /* When blocking starts */
    NOTIFY_DOWNTIME_ENDING_SOON,      /* 15 min before blocking ends */
    NOTIFY_DOWNTIME_ENDED,            /* When blocking ends */
    NOTIFY_NON_RECURRING_UNPAUSED     /* When temporary pause expires naturally */
} notification_type_t;

/**
 * Absolute unblock event classification
 */
typedef enum {
    UNBLOCK_UNKNOWN,                /* Cannot determine type */
    UNBLOCK_TOO_OLD,                /* Event older than tolerance window */
    UNBLOCK_RECENT_ABSOLUTE,        /* Recent absolute unblock (basic check) */
    UNBLOCK_NATURAL_EXPIRY,         /* Pause expired at scheduled time (send notification) */
    UNBLOCK_NATURAL_NO_NOTIFY,      /* Natural expiry but device remains blocked */
    UNBLOCK_MANUAL_EARLY,           /* Manual wakeup before scheduled end (no notification) */
    UNBLOCK_TYPE_WEEKLY_DOWNTIME_END,      /* Weekly schedule ended normally */
    UNBLOCK_TYPE_MANUAL_WAKEUP_WEEKLY,     /* User woke up during weekly downtime */
    UNBLOCK_TYPE_PAUSE_EXPIRED,            /* Temporary pause expired naturally */
    UNBLOCK_TYPE_MANUAL_WAKEUP_PAUSE,      /* User woke up early from pause */
    UNBLOCK_TYPE_REDUNDANT                 /* Already unblocked */
} unblock_type_t;

/**
 * Per-MAC notification state for a blocking period
 * Tracks which notifications have been sent to avoid duplicates
 */
typedef struct {
    bool starting_soon_sent;  /* DOWNTIME_STARTING_SOON sent */
    bool started_sent;        /* DOWNTIME_STARTED sent */
    bool ending_soon_sent;    /* DOWNTIME_ENDING_SOON sent */
    bool ended_sent;          /* DOWNTIME_ENDED sent */
} mac_notification_state_t;

/**
 * A single blocking period with notification tracking
 */
typedef struct mac_block_period {
    time_t start_time;                      /* When blocking starts */
    time_t end_time;                        /* When blocking ends */
    
    uint32_t *blocked_mac_indexes;          /* Which MACs are blocked (array) */
    size_t blocked_count;                   /* Number of blocked MACs */
    
    mac_notification_state_t *mac_states;   /* Notification state per MAC (array) */
    
    bool start_is_absolute;                 /* true = started by absolute, false = started by weekly */
    bool end_is_absolute;                   /* true = ends by absolute, false = ends by weekly */

    struct mac_block_period *next;          /* Next period in linked list */
} mac_block_period_t;

/**
 * Timeline for a single MAC address
 */
typedef struct {
    char mac_address[MAC_ADDRESS_SIZE];     /* MAC address string */
    uint32_t mac_index;                     /* Index in schedule->macs array */
    mac_block_period_t *periods;            /* Linked list of blocking periods */
} mac_timeline_t;

/**
 * Collection of timelines for all MACs
 */
typedef struct {
    mac_timeline_t *timelines;              /* Array of timelines */
    size_t mac_count;                       /* Number of MACs */
    char *time_zone;                        /* Timezone string (copy) */
    time_t created_at;                      /* When timeline was built */
} mac_timeline_collection_t;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

/**
 * Initialize notification subsystem
 * 
 * @param timezone  The timezone string (e.g., "PST8PDT")
 */
void aker_notification_init(const char *timezone);

/**
 * Build MAC-specific timeline from schedule
 * 
 * @param schedule      The schedule to process
 * @param now           Current Unix time
 * @param weeks_ahead   How many weeks ahead to build
 * 
 * @return Timeline collection, or NULL on error
 */
mac_timeline_collection_t* build_timeline_from_schedule(
    schedule_t *schedule,
    time_t now,
    int weeks_ahead
);

/**
 * Destroy timeline collection and free memory
 * 
 * @param collection  The timeline to destroy
 */
void destroy_timeline_collection(mac_timeline_collection_t *collection);

/**
 * Check if a device is blocked at a specific time
 * Considers both weekly and absolute schedules (absolute takes precedence)
 * 
 * @param schedule    The current schedule
 * @param mac_index   Index of MAC in schedule->macs array
 * @param check_time  Time to check
 * 
 * @return true if blocked, false otherwise
 */
bool is_device_blocked_at(
    schedule_t *schedule,
    uint32_t mac_index,
    time_t check_time
);

/**
 * Check if a MAC is indefinitely blocked ("Until I Unpause")
 * 
 * @param schedule   The current schedule
 * @param mac_index  Index of MAC in schedule->macs array
 * 
 * @return true if indefinitely blocked, false otherwise
 */
bool is_mac_indefinitely_blocked(
    schedule_t *schedule,
    uint32_t mac_index
);

/**
 * Get next notification time from timeline
 * 
 * @param collection  The timeline collection
 * @param now         Current Unix time
 * 
 * @return Next notification time, or INT_MAX if none
 */
time_t get_next_notification_time(
    mac_timeline_collection_t *collection,
    time_t now
);

/**
 * Send pending notifications with state-change checking
 * This version performs actual notification sending with state verification
 * 
 * @param collection  The timeline collection
 * @param schedule    The current schedule (for state checking)
 * @param now         Current Unix time
 */
void send_pending_notifications_with_state_check(
    mac_timeline_collection_t *collection,
    schedule_t *schedule,
    time_t now
);

/**
 * Send notification event via T2 telemetry
 * 
 * @param type                  Notification type
 * @param scheduled_time        Start/end time for event
 * @param mac_indexes           Array of MAC indexes
 * @param mac_count             Number of MACs
 * @param timezone              Timezone string
 * @param schedule              Schedule (for MAC address lookup)
 */
void send_notification_event(
    notification_type_t type,
    time_t scheduled_time,
    uint32_t *mac_indexes,
    size_t mac_count,
    const char *timezone,
    schedule_t *schedule
);

/**
 * Format Unix time as ISO8601 UTC string
 * 
 * @param unix_time  Unix timestamp
 * @param output     Buffer for output (min 32 bytes)
 * 
 * Example: "2026-06-06T03:00:00Z"
 */
void format_iso8601_utc(time_t unix_time, char *output);

/**
 * Calculate UTC offset for timezone at given time
 * 
 * @param timezone   Timezone string (e.g., "PST8PDT")
 * @param unix_time  Unix timestamp
 * @param output     Buffer for output (min 8 bytes)
 * 
 * Example: "-07:00" or "+00:00"
 */
void calculate_utc_offset(const char *timezone, time_t unix_time, char *output);

/**
 * Cleanup notification subsystem
 */
void aker_notification_cleanup(void);

#endif /* __AKER_NOTIFICATION_H__ */
