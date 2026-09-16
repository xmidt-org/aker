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
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>

#include "aker_notification.h"
#include "aker_log.h"
#include "aker_mem.h"
#include "aker_rbus.h"
#include "time.h"

#ifdef ENABLE_FEATURE_TELEMETRY2_0
#include <telemetry_busmessage_sender.h>
#endif

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static char g_timezone[256] = {0};

#ifdef UNIT_TESTING
int g_notification_sent_count[NOTIFY_NON_RECURRING_UNPAUSED + 1] = {0};
#endif

/*----------------------------------------------------------------------------*/
/*                             Helper Functions                               */
/*----------------------------------------------------------------------------*/

/**
 * Format Unix time as ISO8601 UTC string
 * Example: 1784153194 -> "2026-07-15T22:06:34Z"
 */
void format_iso8601_utc(time_t unix_time, char *output)
{
    struct tm *utc_time;
    
    if (!output) {
        debug_error("format_iso8601_utc: NULL output buffer\n");
        return;
    }
    
    utc_time = gmtime(&unix_time);
    if (!utc_time) {
        debug_error("format_iso8601_utc: gmtime() failed\n");
        output[0] = '\0';
        return;
    }
    
    /* Format: YYYY-MM-DDTHH:MM:SSZ */
    strftime(output, 32, "%Y-%m-%dT%H:%M:%SZ", utc_time);
    
    debug_print("format_iso8601_utc: %ld -> %s\n", unix_time, output);
}

/**
 * Calculate UTC offset for timezone at given time
 * Handles DST changes correctly
 */
void calculate_utc_offset(const char *timezone, time_t unix_time, char *output)
{
    struct tm local_time;
    long offset_sec;
    int hours, minutes;
    char sign;
    char *old_tz = NULL;
    char old_tz_buf[256] = {0};
    
    if (!output) {
        debug_error("calculate_utc_offset: NULL output buffer\n");
        return;
    }
    
    if (!timezone) {
        debug_error("calculate_utc_offset: NULL timezone\n");
        strcpy(output, "+00:00");
        return;
    }
    
    /* Save current TZ environment variable */
    old_tz = getenv("TZ");
    if (old_tz) {
        strncpy(old_tz_buf, old_tz, sizeof(old_tz_buf) - 1);
        old_tz_buf[sizeof(old_tz_buf) - 1] = '\0';
    }
    
    /* Set to target timezone */
    setenv("TZ", timezone, 1);
    tzset();
    
    /* Get local time in target timezone */
    local_time = *localtime(&unix_time);
    
    /* Get UTC offset from tm structure
     * tm_gmtoff is available on Linux/BSD/MacOS and gives the
     * offset in seconds from UTC (negative for west of UTC)
     * For example: PDT (UTC-7) gives tm_gmtoff = -25200
     */
    offset_sec = local_time.tm_gmtoff;
    
    /* Restore original TZ */
    if (old_tz_buf[0]) {
        setenv("TZ", old_tz_buf, 1);
    } else {
        unsetenv("TZ");
    }
    tzset();
    
    /* Format as [+/-]HH:MM */
    sign = (offset_sec < 0) ? '-' : '+';
    offset_sec = labs(offset_sec);
    hours = offset_sec / 3600;
    minutes = (offset_sec % 3600) / 60;

    snprintf(output, 16, "%c%02d:%02d", sign, hours, minutes);
    
    debug_info("calculate_utc_offset: %s at %ld -> %s\n", timezone, unix_time, output);
}

/**
 * Initialize notification subsystem
 */
void aker_notification_init(const char *timezone)
{
    if (timezone) {
        strncpy(g_timezone, timezone, sizeof(g_timezone) - 1);
        g_timezone[sizeof(g_timezone) - 1] = '\0';
        debug_info("aker_notification_init: timezone=%s\n", g_timezone);
    } else {
        g_timezone[0] = '\0';
        debug_info("aker_notification_init: timezone=NULL\n");
    }
}

/**
 * Cleanup notification subsystem
 */
void aker_notification_cleanup(void)
{
    g_timezone[0] = '\0';
    debug_info("aker_notification_cleanup: done\n");
}

/*----------------------------------------------------------------------------*/
/*                        Timeline Management Functions                       */
/*----------------------------------------------------------------------------*/

/**
 * Convert weekly time to Unix time for a specific week
 */
static time_t weekly_to_unix_time(time_t weekly_sec, time_t base_time, const char *tz)
{
    struct tm base_tm;
    time_t week_start;
    
    /* Set timezone */
    if (tz) {
        set_unix_time_zone((char*)tz);
    }
    
    /* Get base time in local time */
    if (localtime_r(&base_time, &base_tm) == NULL) {
        return 0;
    }
    
    /* Calculate start of the week (Sunday 00:00:00) */
    base_tm.tm_hour = 0;
    base_tm.tm_min = 0;
    base_tm.tm_sec = 0;
    base_tm.tm_isdst = -1; /* Let mktime determine DST */
    
    /* Go back to Sunday */
    int days_since_sunday = base_tm.tm_wday;
    week_start = mktime(&base_tm) - (days_since_sunday * 86400);
    
    /* Add weekly offset */
    return week_start + weekly_sec;
}

/**
 * Structure for storing event times for timeline building
 */
typedef struct timeline_event {
    time_t event_time;
    bool is_block_start;  /* true = block starts, false = block ends */
    uint32_t *mac_indexes;
    size_t mac_count;
    bool is_absolute;     /* true = from absolute schedule, false = from weekly */
    struct timeline_event *next;
} timeline_event_t;

/**
 * Create a timeline event
 */
static timeline_event_t* create_timeline_event(
    time_t event_time,
    bool is_block_start,
    uint32_t *mac_indexes,
    size_t mac_count,
    bool is_absolute)
{
    timeline_event_t *event;
    
    event = (timeline_event_t*)aker_malloc(sizeof(timeline_event_t));
    if (!event) {
        return NULL;
    }
    
    event->event_time = event_time;
    event->is_block_start = is_block_start;
    event->is_absolute = is_absolute;
    event->mac_count = mac_count;
    event->next = NULL;
    
    if (mac_count > 0) {
        event->mac_indexes = (uint32_t*)aker_malloc(mac_count * sizeof(uint32_t));
        if (!event->mac_indexes) {
            aker_free(event);
            return NULL;
        }
        memcpy(event->mac_indexes, mac_indexes, mac_count * sizeof(uint32_t));
    } else {
        event->mac_indexes = NULL;
    }
    
    return event;
}

/**
 * Free timeline event list
 */
static void free_timeline_events(timeline_event_t *events)
{
    timeline_event_t *current, *next;
    
    current = events;
    while (current) {
        next = current->next;
        if (current->mac_indexes) {
            aker_free(current->mac_indexes);
        }
        aker_free(current);
        current = next;
    }
}

/**
 * Insert event into sorted list (by time)
 */
static timeline_event_t* insert_event_sorted(timeline_event_t *head, timeline_event_t *new_event)
{
    timeline_event_t *current, *prev;
    
    if (!new_event) {
        return head;
    }
    
    /* Insert at head if empty or new event is earliest */
    if (!head || new_event->event_time < head->event_time) {
        new_event->next = head;
        return new_event;
    }

    /* If new event has same time as head, absolute events go first */
    if (new_event->event_time == head->event_time && new_event->is_absolute && !head->is_absolute) {
        new_event->next = head;
        return new_event;
    }

    /* Find insertion point */
    prev = head;
    current = head->next;
    while (current && (current->event_time < new_event->event_time ||
                       (current->event_time == new_event->event_time &&
                        current->is_absolute && !new_event->is_absolute))) {
        prev = current;
        current = current->next;
    }
    
    new_event->next = current;
    prev->next = new_event;
    
    return head;
}

/**
 * Check if a MAC is indefinitely blocked ("Until I Unpause")
 */
bool is_mac_indefinitely_blocked(schedule_t *schedule, uint32_t mac_index)
{
    if (!schedule || !schedule->weekly) {
        return false;
    }
    
    bool found_blocking = false;
    bool found_unblocking = false;
    
    schedule_event_t *event = schedule->weekly;
    while (event) {
        bool mac_in_this_event = false;
        
        /* Check if this MAC is in the blocking list */
        for (size_t i = 0; i < event->block_count; i++) {
            if (event->block[i] == mac_index) {
                found_blocking = true;
                mac_in_this_event = true;
                break;
            }
        }
        
        /* Check if this is an unblock-all event (empty indexes) */
        if (event->block_count == 0) {
            found_unblocking = true;
        }
        /* If event blocks other MACs but NOT this one = implicit unblock */
        else if (!mac_in_this_event) {
            found_unblocking = true;
        }
        
        event = event->next;
    }
    
    /* If MAC is blocked but never unblocked = indefinite block */
    return (found_blocking && !found_unblocking);
}

/**
 * Create a new blocking period
 */
static mac_block_period_t* create_block_period(
    time_t start_time,
    time_t end_time,
    uint32_t *blocked_mac_indexes,
    size_t blocked_count,
    size_t total_mac_count,
    bool start_is_absolute,
    bool end_is_absolute)
{
    mac_block_period_t *period;
    
    period = (mac_block_period_t*)aker_malloc(sizeof(mac_block_period_t));
    if (!period) {
        debug_error("create_block_period: Failed to allocate period\n");
        return NULL;
    }
    
    memset(period, 0, sizeof(mac_block_period_t));
    period->start_time = start_time;
    period->end_time = end_time;
    period->blocked_count = blocked_count;
    period->start_is_absolute = start_is_absolute;
    period->end_is_absolute = end_is_absolute;
    period->next = NULL;
    
    /* Allocate and copy blocked MAC indexes */
    if (blocked_count > 0) {
        period->blocked_mac_indexes = (uint32_t*)aker_malloc(blocked_count * sizeof(uint32_t));
        if (!period->blocked_mac_indexes) {
            debug_error("create_block_period: Failed to allocate blocked_mac_indexes\n");
            aker_free(period);
            return NULL;
        }
        memcpy(period->blocked_mac_indexes, blocked_mac_indexes, 
               blocked_count * sizeof(uint32_t));
    }
    
    /* Allocate notification states for ALL MACs */
    period->mac_states = (mac_notification_state_t*)aker_malloc(
        total_mac_count * sizeof(mac_notification_state_t));
    if (!period->mac_states) {
        debug_error("create_block_period: Failed to allocate mac_states\n");
        if (period->blocked_mac_indexes) {
            aker_free(period->blocked_mac_indexes);
        }
        aker_free(period);
        return NULL;
    }
    memset(period->mac_states, 0, total_mac_count * sizeof(mac_notification_state_t));
    
    debug_print("create_block_period: Created period %ld-%ld with %zu MACs\n",
               start_time, end_time, blocked_count);
    
    return period;
}

/**
 * Destroy a block period and its linked list
 */
static void destroy_block_period(mac_block_period_t *period)
{
    mac_block_period_t *current, *next;
    
    current = period;
    while (current) {
        next = current->next;
        
        if (current->blocked_mac_indexes) {
            aker_free(current->blocked_mac_indexes);
        }
        if (current->mac_states) {
            aker_free(current->mac_states);
        }
        aker_free(current);
        
        current = next;
    }
}

/**
 * Destroy timeline collection and free all memory
 */
void destroy_timeline_collection(mac_timeline_collection_t *collection)
{
    if (!collection) {
        return;
    }
    
    if (collection->timelines) {
        for (size_t i = 0; i < collection->mac_count; i++) {
            destroy_block_period(collection->timelines[i].periods);
        }
        aker_free(collection->timelines);
    }
    
    if (collection->time_zone) {
        aker_free(collection->time_zone);
    }
    
    aker_free(collection);
    
    debug_info("destroy_timeline_collection: Cleaned up timeline\n");
}

/**
 * Build periods for a specific MAC from event list
 */
static mac_block_period_t* build_periods_for_mac(
    timeline_event_t *events,
    uint32_t mac_index,
    size_t total_mac_count,
    time_t now)
{
    mac_block_period_t *periods_head = NULL;
    mac_block_period_t *periods_tail = NULL;
    timeline_event_t *current;
    time_t block_start = 0;
    bool currently_blocked = false;
    bool start_is_absolute = false;  /* Track if block START is from absolute */
    /* Once an ABSOLUTE event opens a period, only an ABSOLUTE event may close
     * it. A weekly-only event that falls inside that still-open absolute
     * window (e.g. the weekly's own start/end when an absolute pause fully
     * encloses it) must not be treated as a real state transition. */
    bool absolute_controls_period = false;

    if (!events || total_mac_count == 0) {
        debug_error("build_periods_for_mac: Invalid parameters (events=%p, total_mac_count=%zu)\n",
                   (void*)events, total_mac_count);
        return NULL;
    }

    current = events;
    /* Tracks the timestamp of the last absolute event seen so ALL weekly events
     * tied to that same timestamp are skipped, not just the one immediately
     * following it (schedule wrap-around can duplicate a weekly event at the
     * same tie point, breaking a prev_event-only check). */
    time_t last_absolute_time = 0;
    bool has_last_absolute_time = false;
    /* Whether the WEEKLY schedule alone currently blocks this MAC. Maintained
     * for every weekly event even when that event is skipped for period
     * purposes, so an expiring absolute pause can tell if weekly still holds. */
    bool weekly_blocked = false;
    /* Same idea for the absolute list, used to tell a brand-new pause apart
     * from the backend merely carrying this MAC forward while it edits the
     * list (only the former may take control away from an active weekly). */
    bool absolute_blocked = false;
    while (current) {
        bool mac_in_current_list = false;
        bool was_absolute_blocked = absolute_blocked;

        /* Check if this MAC is in the current event's block list */
        if (current->mac_count == 0) {
            /* Empty list = unblock all, so MAC is NOT in block list */
            mac_in_current_list = false;
        } else if (current->mac_indexes) {
            /* Check if MAC is in the list */
            for (size_t i = 0; i < current->mac_count; i++) {
                if (current->mac_indexes[i] == mac_index) {
                    mac_in_current_list = true;
                    break;
                }
            }
        }

        if (current->is_absolute) {
            absolute_blocked = mac_in_current_list;
        } else {
            weekly_blocked = mac_in_current_list;
        }

        /* Skip weekly events if there was an absolute event at the same time (absolute takes precedence) */
        if (!current->is_absolute && has_last_absolute_time &&
            last_absolute_time == current->event_time) {
            debug_print("build_periods_for_mac: MAC %u - Skipping weekly event at %ld, absolute event already processed\n",
                       mac_index, current->event_time);
            current = current->next;
            continue;
        }

        if (current->is_absolute) {
            last_absolute_time = current->event_time;
            has_last_absolute_time = true;
        }

        /* Detect state transitions:
         * - currently_blocked=false, mac_in_list=true → Block START
         * - currently_blocked=true, mac_in_list=false → Block END (unblocked by removal from list)
         */

        /* A weekly-only event crossing through a still-open ABSOLUTE-controlled
         * period (start or end) must be ignored entirely - the absolute window
         * governs exclusively until its own matching event closes it. */
        if (currently_blocked && absolute_controls_period && !current->is_absolute) {
            debug_print("build_periods_for_mac: MAC %u - Ignoring weekly event at %ld, absolute period still open\n",
                       mac_index, current->event_time);
            current = current->next;
            continue;
        }

        /* Process the event */
        if (mac_in_current_list && !currently_blocked) {
            /* MAC just became blocked (either by block_start event or added to list) */
            block_start = current->event_time;
            currently_blocked = true;

            /* Determine if this block start is controlled by absolute or weekly schedule
             * Rule: If absolute event time = weekly event time for this MAC → weekly controls it (backend-generated)
             *       If absolute event time < weekly event time (or no weekly) → absolute controls it (user-initiated)
             */
            start_is_absolute = current->is_absolute;

            if (current->is_absolute) {
                /* Check if there's a weekly event at the same time that also blocks this MAC */
                bool found_matching_weekly = false;
                timeline_event_t *check = current->next;

                /* Look ahead for weekly event at exact same time */
                while (check && check->event_time == current->event_time) {
                    if (!check->is_absolute && check->mac_indexes) {
                        /* This is a weekly event at the same time - check if MAC is in it */
                        for (size_t i = 0; i < check->mac_count; i++) {
                            if (check->mac_indexes[i] == mac_index) {
                                found_matching_weekly = true;
                                break;
                            }
                        }
                        if (found_matching_weekly) break;
                    }
                    check = check->next;
                }

                if (found_matching_weekly) {
                    /* Absolute time = Weekly time for this MAC → Backend added MAC to absolute because of weekly conflict
                     * Treat as weekly start for notification purposes */
                    start_is_absolute = false;
                    debug_print("build_periods_for_mac: MAC %u - Block start at %ld: absolute time = weekly time, treating as WEEKLY start (backend-generated)\n",
                               mac_index, current->event_time);
                } else {
                    /* Absolute time ≠ Weekly time (or no weekly) → True user-initiated pause
                     * Treat as absolute start for notification purposes */
                    start_is_absolute = true;
                    debug_print("build_periods_for_mac: MAC %u - Block start at %ld: absolute time before weekly (or no weekly), treating as ABSOLUTE start (user-initiated)\n",
                               mac_index, current->event_time);
                }
            } else {
                /* Pure weekly event - always treat as weekly start */
                start_is_absolute = false;
                debug_print("build_periods_for_mac: MAC %u - Block start at %ld: WEEKLY event\n",
                           mac_index, current->event_time);
            }

            absolute_controls_period = start_is_absolute;
        } else if (mac_in_current_list && currently_blocked && current->is_absolute) {
            /* An absolute event reaffirms blocking while the period is
             * already open. Resolve who governs from here:
             *  - ties a weekly event for this MAC -> backend hand-off point
             *    (Case 1: absolute expires exactly where weekly takes over),
             *    so weekly governs;
             *  - otherwise, only a MAC *newly* entering the absolute list is a
             *    genuine user pause that may outlive weekly and take control.
             *    A MAC already in the previous absolute list is just being
             *    carried forward while the backend edits membership, so
             *    whoever governs the period keeps it. */
            bool found_matching_weekly = false;
            timeline_event_t *check = current->next;

            while (check && check->event_time == current->event_time) {
                if (!check->is_absolute && check->mac_indexes) {
                    for (size_t i = 0; i < check->mac_count; i++) {
                        if (check->mac_indexes[i] == mac_index) {
                            found_matching_weekly = true;
                            break;
                        }
                    }
                    if (found_matching_weekly) break;
                }
                check = check->next;
            }

            if (found_matching_weekly) {
                absolute_controls_period = false;
                debug_print("build_periods_for_mac: MAC %u - Absolute continuation at %ld: ties weekly, hand off control\n",
                           mac_index, current->event_time);
            } else if (!was_absolute_blocked) {
                absolute_controls_period = true;
                debug_print("build_periods_for_mac: MAC %u - Absolute continuation at %ld: new pause, absolute takes control\n",
                           mac_index, current->event_time);
            } else {
                debug_print("build_periods_for_mac: MAC %u - Absolute continuation at %ld: carried forward, control unchanged\n",
                           mac_index, current->event_time);
            }
        } else if (!mac_in_current_list && currently_blocked) {
            /* MAC was removed from block list (state transition: blocked → unblocked) */

            /* An absolute pause can expire while the weekly window is still
             * blocking this MAC. Weekly events at this exact timestamp sort
             * after absolute ones, so peek before trusting the running state.
             * If weekly still holds, keep the period open and let the weekly
             * end close it (ABSOLUTE start + WEEKLY end). */
            if (current->is_absolute) {
                bool weekly_holds = weekly_blocked;
                timeline_event_t *peek = current->next;

                while (peek && peek->event_time == current->event_time) {
                    if (!peek->is_absolute) {
                        bool in_weekly = false;
                        if (peek->mac_indexes) {
                            for (size_t i = 0; i < peek->mac_count; i++) {
                                if (peek->mac_indexes[i] == mac_index) {
                                    in_weekly = true;
                                    break;
                                }
                            }
                        }
                        weekly_holds = in_weekly;
                        break;
                    }
                    peek = peek->next;
                }

                if (weekly_holds) {
                    debug_print("build_periods_for_mac: MAC %u - Absolute expiry at %ld but weekly still blocking, handing control to weekly\n",
                               mac_index, current->event_time);
                    absolute_controls_period = false;
                    current = current->next;
                    continue;
                }
            }

            /* Create period only if it's in the future or currently active */
            if (current->event_time > now) {
                /* Determine if this block end is controlled by absolute or weekly schedule
                 * Rule: If absolute event time = weekly event time for this MAC → weekly controls it
                 *       If absolute event time ≥ weekly event time → absolute controls it
                 */
                bool end_is_absolute = current->is_absolute;

                if (current->is_absolute && absolute_controls_period) {
                    /* This MAC's period has been genuinely absolute-controlled
                     * (no backend hand-off tie found along the way) - its own
                     * end always resolves as ABSOLUTE, even if a weekly event
                     * happens to unblock at the exact same timestamp. Per AC:
                     * weekly-end == absolute-end for a genuinely absolute
                     * period must still send NON_RECURRING_UNPAUSED, not a
                     * normal weekly ENDED. */
                    end_is_absolute = true;
                    debug_print("build_periods_for_mac: MAC %u - Block end at %ld: absolute-controlled period, treating as ABSOLUTE end regardless of weekly tie\n",
                               mac_index, current->event_time);
                } else if (current->is_absolute) {
                    /* Check if there's a weekly event at the same time that also unblocks this MAC */
                    bool found_matching_weekly = false;
                    timeline_event_t *check = current->next;

                    /* Look ahead for weekly event at exact same time */
                    while (check && check->event_time == current->event_time) {
                        if (!check->is_absolute) {
                            /* This is a weekly event at the same time - check if MAC is NOT in it (meaning unblocked) */
                            bool mac_in_weekly_list = false;
                            if (check->mac_indexes) {
                                for (size_t i = 0; i < check->mac_count; i++) {
                                    if (check->mac_indexes[i] == mac_index) {
                                        mac_in_weekly_list = true;
                                        break;
                                    }
                                }
                            }
                            if (!mac_in_weekly_list) {
                                /* MAC is NOT in weekly block list at this time → weekly also unblocks it */
                                found_matching_weekly = true;
                                break;
                            }
                        }
                        check = check->next;
                    }

                    if (found_matching_weekly) {
                        /* Absolute time = Weekly time for this MAC unblock → Backend removed MAC from absolute because weekly ends
                         * Treat as weekly end for notification purposes */
                        end_is_absolute = false;
                        debug_print("build_periods_for_mac: MAC %u - Block end at %ld: absolute time = weekly time, treating as WEEKLY end\n",
                                   mac_index, current->event_time);
                    } else {
                        /* Absolute time ≠ Weekly time → True user unpause or absolute expiry
                         * Treat as absolute end for notification purposes */
                        end_is_absolute = true;
                        debug_print("build_periods_for_mac: MAC %u - Block end at %ld: absolute expiry, treating as ABSOLUTE end\n",
                                   mac_index, current->event_time);
                    }
                } else {
                    /* Pure weekly event - always treat as weekly end */
                    end_is_absolute = false;
                    debug_print("build_periods_for_mac: MAC %u - Block end at %ld: WEEKLY event\n",
                               mac_index, current->event_time);
                }

                uint32_t blocked_macs[] = { mac_index };
                mac_block_period_t *new_period = create_block_period(
                    block_start,
                    current->event_time,
                    blocked_macs,
                    1,
                    total_mac_count,
                    start_is_absolute,
                    end_is_absolute);

                if (new_period) {
                    /* Log notification strategy based on start/end flags */
                    const char *start_type = start_is_absolute ? "ABSOLUTE" : "WEEKLY";
                    const char *end_type = end_is_absolute ? "ABSOLUTE" : "WEEKLY";
                    debug_print("build_periods_for_mac: MAC %u - Created period [%ld → %ld], start=%s, end=%s\n",
                               mac_index, block_start, current->event_time, start_type, end_type);

                    if (!start_is_absolute && !end_is_absolute) {
                        debug_print("  → Notifications: STARTING_SOON, STARTED, ENDING_SOON, ENDED (all 4)\n");
                    } else if (start_is_absolute && end_is_absolute) {
                        debug_print("  → Notifications: skip STARTING_SOON, STARTED, ENDING_SOON → send NON_RECURRING_UNPAUSED\n");
                    } else if (start_is_absolute && !end_is_absolute) {
                        debug_print("  → Notifications: skip STARTING_SOON, STARTED → send ENDING_SOON, ENDED\n");
                    } else if (!start_is_absolute && end_is_absolute) {
                        debug_print("  → Notifications: STARTING_SOON, STARTED → skip ENDING_SOON → send NON_RECURRING_UNPAUSED\n");
                    }

                    if (!periods_head) {
                        periods_head = new_period;
                        periods_tail = new_period;
                    } else {
                        periods_tail->next = new_period;
                        periods_tail = new_period;
                    }
                }
            }
            currently_blocked = false;
            absolute_controls_period = false;
        }

        current = current->next;
    }

    return periods_head;
}

/**
 * True when an absolute entry merely mirrors a weekly window the backend
 * folded into the absolute array, rather than a pause the user requested.
 * Mirrors the tie rule build_periods_for_mac() uses for start attribution.
 */
static bool absolute_start_ties_weekly(schedule_t *schedule, time_t abs_start, size_t mac_idx)
{
    schedule_event_t *ev;

    if (!schedule) {
        return false;
    }

    ev = schedule->weekly;
    while (ev) {
        if (ev->block_count > 0) {
            for (size_t i = 0; i < ev->block_count; i++) {
                if (ev->block[i] == mac_idx) {
                    if (weekly_to_unix_time(ev->time, abs_start, schedule->time_zone) == abs_start) {
                        return true;
                    }
                    break;
                }
            }
        }
        ev = ev->next;
    }

    return false;
}

/**
 * Log timeline summary for debugging - uses RAW schedule to avoid showing fractured periods
 */
static void log_timeline_summary(mac_timeline_collection_t *collection, schedule_t *schedule)
{
    if (!collection || !collection->timelines || !schedule) {
        debug_error("log_timeline_summary: Invalid parameters (collection=%p, schedule=%p)\n",
                   (void*)collection, (void*)schedule);
        return;
    }

    if (!schedule->macs || schedule->mac_count == 0) {
        debug_error("log_timeline_summary: Invalid schedule MAC data\n");
        return;
    }

    /* Set timezone for localtime conversions */
    if (schedule->time_zone) {
        set_unix_time_zone((char*)schedule->time_zone);
    }

    /* Process WEEKLY schedule - group by time-of-day pattern */
    typedef struct {
        int start_hour, start_min, start_sec;
        int end_hour, end_min, end_sec;
        time_t duration;     /* start -> end, already unwrapped across midnight */
        bool days[7];
        uint32_t macs[256];  /* Max MACs per pattern */
        size_t mac_count;
    } weekly_pattern_t;

    weekly_pattern_t weekly_patterns[50];
    int weekly_count = 0;
    memset(weekly_patterns, 0, sizeof(weekly_patterns));

    /* Track blocking state for each MAC across weekly events */
    bool mac_blocked[256] = {false};
    time_t block_start_time[256] = {0};

    schedule_event_t *weekly_event = schedule->weekly;
    while (weekly_event) {
        /* Check each MAC to see if blocking starts or ends */
        for (size_t mac_idx = 0; mac_idx < schedule->mac_count && mac_idx < 256; mac_idx++) {
            bool is_in_event = false;
            if (weekly_event->block_count > 0) {
                for (size_t i = 0; i < weekly_event->block_count; i++) {
                    if (weekly_event->block[i] == mac_idx) {
                        is_in_event = true;
                        break;
                    }
                }
            }

            if (is_in_event && !mac_blocked[mac_idx]) {
                /* Blocking starts for this MAC */
                mac_blocked[mac_idx] = true;
                block_start_time[mac_idx] = weekly_event->time;
            } else if (!is_in_event && mac_blocked[mac_idx]) {
                /* Blocking ends - create pattern entry */
                time_t start_seconds = block_start_time[mac_idx];
                time_t end_seconds = weekly_event->time;

                /* Convert seconds-from-Sunday to hour/minute/day */
                int start_day = start_seconds / 86400;
                int start_hour = (start_seconds % 86400) / 3600;
                int start_min = (start_seconds % 3600) / 60;
                int start_sec = start_seconds % 60;

                int end_hour = (end_seconds % 86400) / 3600;
                int end_min = (end_seconds % 3600) / 60;
                int end_sec = end_seconds % 60;

                time_t duration = end_seconds - start_seconds;
                if (duration < 0) {
                    duration += SECONDS_IN_A_WEEK;
                }

                /* Validate day range */
                if (start_day >= 0 && start_day <= 6) {
                    /* Find existing pattern or create new */
                    int found = -1;
                    for (int i = 0; i < weekly_count; i++) {
                        if (weekly_patterns[i].start_hour == start_hour &&
                            weekly_patterns[i].start_min == start_min &&
                            weekly_patterns[i].start_sec == start_sec &&
                            weekly_patterns[i].end_hour == end_hour &&
                            weekly_patterns[i].end_min == end_min &&
                            weekly_patterns[i].end_sec == end_sec) {
                            found = i;
                            break;
                        }
                    }

                    if (found >= 0) {
                        /* Add MAC if not already there */
                        bool mac_exists = false;
                        for (size_t m = 0; m < weekly_patterns[found].mac_count; m++) {
                            if (weekly_patterns[found].macs[m] == mac_idx) {
                                mac_exists = true;
                                break;
                            }
                        }
                        if (!mac_exists && weekly_patterns[found].mac_count < 256) {
                            weekly_patterns[found].macs[weekly_patterns[found].mac_count++] = mac_idx;
                        }
                        weekly_patterns[found].days[start_day] = true;
                    } else if (weekly_count < 50) {
                        /* New pattern */
                        weekly_patterns[weekly_count].start_hour = start_hour;
                        weekly_patterns[weekly_count].start_min = start_min;
                        weekly_patterns[weekly_count].start_sec = start_sec;
                        weekly_patterns[weekly_count].end_hour = end_hour;
                        weekly_patterns[weekly_count].end_min = end_min;
                        weekly_patterns[weekly_count].end_sec = end_sec;
                        weekly_patterns[weekly_count].duration = duration;
                        weekly_patterns[weekly_count].days[start_day] = true;
                        weekly_patterns[weekly_count].macs[0] = mac_idx;
                        weekly_patterns[weekly_count].mac_count = 1;
                        weekly_count++;
                    }
                }  /* End validation check */

                mac_blocked[mac_idx] = false;
            }
        }
        weekly_event = weekly_event->next;
    }

    /* Log weekly patterns */
    for (int i = 0; i < weekly_count; i++) {
        /* Build MAC list - bounded append, a pattern can hold up to 256 MACs */
        char mac_list[256] = {0};
        size_t mac_list_len = 0;
        for (size_t m = 0; m < weekly_patterns[i].mac_count; m++) {
            int written = snprintf(mac_list + mac_list_len, sizeof(mac_list) - mac_list_len,
                                   "%sMAC%u", (m > 0) ? ", " : "", weekly_patterns[i].macs[m]);
            if (written < 0 || (size_t)written >= sizeof(mac_list) - mac_list_len) {
                mac_list_len = sizeof(mac_list) - 1;
                break;
            }
            mac_list_len += (size_t)written;
        }

        /* Build days string */
        const char *day_names[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        char days_str[128] = {0};
        int day_count = 0;
        for (int d = 0; d < 7; d++) if (weekly_patterns[i].days[d]) day_count++;

        int idx = 0;
        for (int d = 0; d < 7; d++) {
            if (weekly_patterns[i].days[d]) {
                if (idx > 0) strcat(days_str, (idx == day_count - 1) ? " and " : ", ");
                strcat(days_str, day_names[d]);
                idx++;
            }
        }

        /* Format times */
        int start_hour_12 = weekly_patterns[i].start_hour % 12;
        if (start_hour_12 == 0) start_hour_12 = 12;
        int end_hour_12 = weekly_patterns[i].end_hour % 12;
        if (end_hour_12 == 0) end_hour_12 = 12;

        /* Convert local time-of-day to an approximate UTC time-of-day */
        /* A weekly window recurs, so anchor the UTC stamp to the next upcoming
         * occurrence among this pattern's days - that yields a real calendar
         * date and exact DST handling, unlike a fixed offset. */
        time_t now_ref = time(NULL);
        time_t next_start = 0;
        for (int d = 0; d < 7; d++) {
            if (!weekly_patterns[i].days[d]) {
                continue;
            }
            time_t weekly_sec = (time_t)d * 86400
                              + weekly_patterns[i].start_hour * 3600
                              + weekly_patterns[i].start_min * 60
                              + weekly_patterns[i].start_sec;
            time_t candidate = weekly_to_unix_time(weekly_sec, now_ref, schedule->time_zone);
            if (candidate < now_ref) {
                candidate += SECONDS_IN_A_WEEK;
            }
            if (next_start == 0 || candidate < next_start) {
                next_start = candidate;
            }
        }

        char start_utc_str[32] = "unknown";
        char end_utc_str[32] = "unknown";
        if (next_start > 0) {
            time_t next_end = next_start + weekly_patterns[i].duration;
            struct tm start_gmt, end_gmt;
            if (gmtime_r(&next_start, &start_gmt) && gmtime_r(&next_end, &end_gmt)) {
                strftime(start_utc_str, sizeof(start_utc_str), "%Y-%m-%d %H:%M:%S", &start_gmt);
                strftime(end_utc_str, sizeof(end_utc_str), "%Y-%m-%d %H:%M:%S", &end_gmt);
            }
        }

        debug_info("Received weekly schedule - %d:%02d:%02d %s to %d:%02d:%02d %s to block %s on %s - next UTC: %s to %s\n",
                   start_hour_12, weekly_patterns[i].start_min, weekly_patterns[i].start_sec,
                   (weekly_patterns[i].start_hour < 12) ? "AM" : "PM",
                   end_hour_12, weekly_patterns[i].end_min, weekly_patterns[i].end_sec,
                   (weekly_patterns[i].end_hour < 12) ? "AM" : "PM",
                   mac_list, days_str,
                   start_utc_str, end_utc_str);
    }

    /* Process ABSOLUTE schedule - group MACs sharing the same start/end time
     * into one log line, mirroring the WEEKLY pattern grouping above (was
     * previously hardcoded to only track/log the first blocked MAC, which
     * silently hid other MACs sharing the same absolute window). */
    typedef struct {
        time_t start;
        time_t end;
        uint32_t macs[256];
        size_t mac_count;
    } abs_pattern_t;

    abs_pattern_t abs_patterns[50];
    int abs_pattern_count = 0;
    memset(abs_patterns, 0, sizeof(abs_patterns));

    bool abs_mac_blocked[256] = {false};
    time_t abs_block_start[256] = {0};
    time_t now_time = time(NULL);

    schedule_event_t *abs_event = schedule->absolute;
    while (abs_event) {
        /* Check for blocking start/end */
        for (size_t mac_idx = 0; mac_idx < schedule->mac_count && mac_idx < 256; mac_idx++) {
            bool is_in_event = false;
            if (abs_event->block_count > 0) {
                for (size_t i = 0; i < abs_event->block_count; i++) {
                    if (abs_event->block[i] == mac_idx) {
                        is_in_event = true;
                        break;
                    }
                }
            }

            if (is_in_event && !abs_mac_blocked[mac_idx]) {
                /* Absolute blocking starts */
                abs_mac_blocked[mac_idx] = true;
                abs_block_start[mac_idx] = abs_event->time;
            } else if (!is_in_event && abs_mac_blocked[mac_idx]) {
                /* Absolute blocking ends - group into a pattern by (start, end), skip expired */
                time_t start_t = abs_block_start[mac_idx];
                time_t end_t = abs_event->time;

                if (end_t > now_time && abs_pattern_count < 50 &&
                    !absolute_start_ties_weekly(schedule, start_t, mac_idx)) {
                    int found = -1;
                    for (int i = 0; i < abs_pattern_count; i++) {
                        if (abs_patterns[i].start == start_t && abs_patterns[i].end == end_t) {
                            found = i;
                            break;
                        }
                    }
                    if (found == -1) {
                        found = abs_pattern_count++;
                        abs_patterns[found].start = start_t;
                        abs_patterns[found].end = end_t;
                        abs_patterns[found].mac_count = 0;
                    }
                    if (abs_patterns[found].mac_count < 256) {
                        abs_patterns[found].macs[abs_patterns[found].mac_count++] = (uint32_t)mac_idx;
                    }
                }
                abs_mac_blocked[mac_idx] = false;
            }
        }
        abs_event = abs_event->next;
    }

    /* Log absolute patterns */
    for (int i = 0; i < abs_pattern_count; i++) {
        struct tm start_tm, end_tm;
        if (localtime_r(&abs_patterns[i].start, &start_tm) &&
            localtime_r(&abs_patterns[i].end, &end_tm)) {
            char mac_list[256] = {0};
            size_t mac_list_len = 0;
            for (size_t m = 0; m < abs_patterns[i].mac_count; m++) {
                int written = snprintf(mac_list + mac_list_len, sizeof(mac_list) - mac_list_len,
                                       "%sMAC%u", (m > 0) ? ", " : "", abs_patterns[i].macs[m]);
                if (written < 0 || (size_t)written >= sizeof(mac_list) - mac_list_len) {
                    mac_list_len = sizeof(mac_list) - 1;
                    break;
                }
                mac_list_len += (size_t)written;
            }

            int start_hour_12 = start_tm.tm_hour % 12;
            if (start_hour_12 == 0) start_hour_12 = 12;
            int end_hour_12 = end_tm.tm_hour % 12;
            if (end_hour_12 == 0) end_hour_12 = 12;

            /* Absolute events have real unix timestamps, so UTC is exact (no
             * DST approximation needed, unlike the weekly section above). */
            struct tm start_gmt, end_gmt;
            char start_utc_str[32] = {0};
            char end_utc_str[32] = {0};
            if (gmtime_r(&abs_patterns[i].start, &start_gmt) && gmtime_r(&abs_patterns[i].end, &end_gmt)) {
                strftime(start_utc_str, sizeof(start_utc_str), "%Y-%m-%d %H:%M:%S", &start_gmt);
                strftime(end_utc_str, sizeof(end_utc_str), "%Y-%m-%d %H:%M:%S", &end_gmt);
            }

            debug_info("Received absolute schedule - %d:%02d %s to %d:%02d %s to block %s - UTC: %s to %s\n",
                       start_hour_12, start_tm.tm_min,
                       (start_tm.tm_hour < 12) ? "AM" : "PM",
                       end_hour_12, end_tm.tm_min,
                       (end_tm.tm_hour < 12) ? "AM" : "PM",
                       mac_list, start_utc_str, end_utc_str);
        }
    }
}

/**
 * Build MAC-specific timeline from schedule
 */
mac_timeline_collection_t* build_timeline_from_schedule(
    schedule_t *schedule,
    time_t now,
    int weeks_ahead)
{
    mac_timeline_collection_t *collection;
    timeline_event_t *all_events = NULL;
    schedule_event_t *sched_event;
    time_t future_limit;
    
    if (!schedule || weeks_ahead < 1) {
        debug_error("build_timeline_from_schedule: Invalid parameters\n");
        return NULL;
    }
    
    debug_print("build_timeline_from_schedule: Building timeline for %zu MACs, %d weeks ahead\n",
               schedule->mac_count, weeks_ahead);
    
    /* Calculate future limit */
    future_limit = now + (weeks_ahead * 7 * 86400);
    
    /* Allocate collection */
    collection = (mac_timeline_collection_t*)aker_malloc(sizeof(mac_timeline_collection_t));
    if (!collection) {
        debug_error("build_timeline_from_schedule: Failed to allocate collection\n");
        return NULL;
    }
    memset(collection, 0, sizeof(mac_timeline_collection_t));
    
    collection->mac_count = schedule->mac_count;
    collection->created_at = now;
    
    /* Copy timezone */
    if (schedule->time_zone) {
        collection->time_zone = strdup(schedule->time_zone);
        if (!collection->time_zone) {
            debug_error("build_timeline_from_schedule: Failed to duplicate timezone string\n");
            aker_free(collection);
            return NULL;
        }
    }
    
    /* Step 1: Expand weekly events into concrete Unix timestamps */
    if (schedule->weekly) {
        debug_print("build_timeline_from_schedule: Expanding weekly events\n");
        
        for (int week = 0; week < weeks_ahead; week++) {
            time_t week_base = now + (week * 7 * 86400);
            
            sched_event = schedule->weekly;
            while (sched_event) {
                time_t event_time = weekly_to_unix_time(
                    sched_event->time,
                    week_base,
                    schedule->time_zone);

                /* For current week (week 0), include all events from current week start
                 * to capture currently active blocks that started earlier this week
                 * For future weeks, only include future events */
                bool include_event = false;
                if (week == 0) {
                    /* Calculate start of current week (last Sunday midnight) */
                    struct tm now_tm;
                    if (schedule->time_zone) {
                        set_unix_time_zone((char*)schedule->time_zone);
                    }
                    if (localtime_r(&now, &now_tm) != NULL) {
                        now_tm.tm_hour = 0;
                        now_tm.tm_min = 0;
                        now_tm.tm_sec = 0;
                        now_tm.tm_isdst = -1;
                        int days_since_sunday = now_tm.tm_wday;
                        time_t week_start = mktime(&now_tm) - (days_since_sunday * 86400);

                        /* Include events from this week's start to capture active periods */
                        if (event_time >= week_start && event_time <= future_limit) {
                            include_event = true;
                        }
                    }
                } else {
                    /* Future weeks: only include future events */
                    if (event_time > now && event_time <= future_limit) {
                        include_event = true;
                    }
                }

                if (include_event) {
                    bool is_block_start = (sched_event->block_count > 0);
                    timeline_event_t *new_event = create_timeline_event(
                        event_time,
                        is_block_start,
                        sched_event->block,
                        sched_event->block_count,
                        false);  /* is_absolute - Weekly events */
                    
                    if (new_event) {
                        all_events = insert_event_sorted(all_events, new_event);
                    }
                }
                
                sched_event = sched_event->next;
            }
        }
    }
    
    /* Step 2: Add absolute events (include recent past to handle network latency) */
    /* Backend uses state-replacement model: each absolute event defines the NEW blocking state.
     * No need for implicit unblock detection - build_periods_for_mac() detects unblocks automatically
     * when a MAC is removed from the block list. */
    if (schedule->absolute) {
        debug_print("build_timeline_from_schedule: Adding absolute events\n");
        
        sched_event = schedule->absolute;
        while (sched_event) {
            /* Include events from past SCHEDULED_TIME_TOLERANCE_SEC to catch events that just happened
             * due to network latency between cloud schedule creation and device receipt */
            if (sched_event->time >= (now - SCHEDULED_TIME_TOLERANCE_SEC) && sched_event->time <= future_limit) {

                /* Add the absolute event - each event defines which MACs are blocked at that time */
                bool is_block_start = (sched_event->block_count > 0);
                timeline_event_t *new_event = create_timeline_event(
                    sched_event->time,
                    is_block_start,
                    sched_event->block,
                    sched_event->block_count,
                    true);  /* is_absolute - From absolute schedule */

                if (new_event) {
                    all_events = insert_event_sorted(all_events, new_event);
                }
            }

            sched_event = sched_event->next;
        }
    }

    /* Allocate timelines array */
    collection->timelines = (mac_timeline_t*)aker_malloc(
        schedule->mac_count * sizeof(mac_timeline_t));
    if (!collection->timelines) {
        debug_error("build_timeline_from_schedule: Failed to allocate timelines\n");
        free_timeline_events(all_events);
        destroy_timeline_collection(collection);
        return NULL;
    }
    memset(collection->timelines, 0, schedule->mac_count * sizeof(mac_timeline_t));

    /* Step 3: Build periods for each MAC from event list */
    for (size_t i = 0; i < schedule->mac_count; i++) {
        collection->timelines[i].mac_index = i;
        strncpy(collection->timelines[i].mac_address, 
                schedule->macs[i].mac, 
                MAC_ADDRESS_SIZE - 1);
        collection->timelines[i].mac_address[MAC_ADDRESS_SIZE - 1] = '\0';

        /* Skip indefinitely blocked MACs */
        if (is_mac_indefinitely_blocked(schedule, i)) {
            debug_info("build_timeline_from_schedule: MAC %u (%s) indefinitely blocked, skip timeline\n",
                       i, collection->timelines[i].mac_address);
            collection->timelines[i].periods = NULL;
            continue;
        }

        /* Build periods for this MAC */
        collection->timelines[i].periods = build_periods_for_mac(
            all_events,
            i,
            schedule->mac_count,
            now);
    }

    /* Cleanup */
    free_timeline_events(all_events);

    debug_print("build_timeline_from_schedule: Timeline built successfully\n");

    /* Log timeline summary for debugging */
    log_timeline_summary(collection, schedule);

    return collection;
}

/*----------------------------------------------------------------------------*/
/*                      State Checking Helper Functions                      */
/*----------------------------------------------------------------------------*/

/**
 * Check if a specific MAC is in a weekly blocking period at given time
 */
static bool is_in_weekly_blocking_period(
    schedule_t *schedule,
    uint32_t mac_index,
    time_t check_time)
{
    schedule_event_t *event;
    time_t weekly_time;
    bool currently_blocked = false;
    
    if (!schedule || !schedule->weekly) {
        return false;
    }
    
    /* Convert check_time to weekly time */
    weekly_time = convert_unix_time_to_weekly(check_time);
    
    /* Walk through weekly schedule in order */
    event = schedule->weekly;
    while (event) {
        if (event->time > weekly_time) {
            break;  /* Haven't reached this event yet */
        }
        
        /* Check if this event affects our MAC */
        if (event->block_count == 0) {
            /* Unblock all */
            currently_blocked = false;
        } else {
            /* Check if MAC is in block list */
            for (size_t i = 0; i < event->block_count; i++) {
                if (event->block[i] == mac_index) {
                    currently_blocked = true;
                    break;
                }
            }
        }
        
        event = event->next;
    }
    
    return currently_blocked;
}

/**
 * Check if a specific MAC is in an absolute blocking period at given time
 */
static bool is_in_absolute_blocking_period(
    schedule_t *schedule,
    uint32_t mac_index,
    time_t check_time)
{
    schedule_event_t *event;
    bool currently_blocked = false;
    
    if (!schedule || !schedule->absolute) {
        return false;
    }
    
    /* Walk through absolute schedule in order */
    event = schedule->absolute;
    while (event) {
        if (event->time > check_time) {
            break;  /* Haven't reached this event yet */
        }
        
        /* Check if this event affects our MAC */
        if (event->block_count == 0) {
            /* Unblock */
            currently_blocked = false;
        } else {
            /* Check if MAC is in block list */
            for (size_t i = 0; i < event->block_count; i++) {
                if (event->block[i] == mac_index) {
                    currently_blocked = true;
                    break;
                }
            }
        }
        
        event = event->next;
    }
    
    return currently_blocked;
}

/**
 * Check if device is blocked at specific time (considers both weekly and absolute)
 * Absolute schedule takes precedence over weekly.
 */
bool is_device_blocked_at(
    schedule_t *schedule,
    uint32_t mac_index,
    time_t check_time)
{
    if (is_in_absolute_blocking_period(schedule, mac_index, check_time)) {
        return true;
    }
    
    if (is_in_weekly_blocking_period(schedule, mac_index, check_time)) {
        return true;
    }
    
    return false;
}

/**
 * Check if MAC is blocked at specific time using timeline periods (not raw schedule)
 * This correctly handles merged absolute+weekly periods.
 */
static bool is_mac_blocked_in_timeline(
    mac_timeline_collection_t *collection,
    uint32_t mac_index,
    time_t check_time)
{
    if (!collection || !collection->timelines || mac_index >= collection->mac_count) {
        return false;
    }

    mac_block_period_t *period = collection->timelines[mac_index].periods;

    while (period) {
        /* Check if check_time falls within this period [start, end) */
        if (check_time >= period->start_time && check_time < period->end_time) {
            return true;
        }

        /* Periods are sorted by time, so we can stop if we're past check_time */
        if (period->start_time > check_time) {
            break;
        }

        period = period->next;
    }

    return false;
}

/*----------------------------------------------------------------------------*/
/*                   Stub Functions (Phases 3-5)                              */
/*----------------------------------------------------------------------------*/

/**
 * Convert notification type to event name string
 */
static const char* get_event_type_string(notification_type_t type)
{
    switch (type) {
        case NOTIFY_DOWNTIME_STARTING_SOON:
            return "DOWNTIME_STARTING_SOON";
        case NOTIFY_DOWNTIME_STARTED:
            return "DOWNTIME_STARTED";
        case NOTIFY_DOWNTIME_ENDING_SOON:
            return "DOWNTIME_ENDING_SOON";
        case NOTIFY_DOWNTIME_ENDED:
            return "DOWNTIME_ENDED";
        case NOTIFY_NON_RECURRING_UNPAUSED:
            return "NON_RECURRING_UNPAUSED";
        default:
            return "UNKNOWN";
    }
}

#ifdef ENABLE_FEATURE_TELEMETRY2_0
/**
 * Get T2 telemetry marker name for notification type
 *
 * All notification types use the same marker name.
 * The eventType field in the JSON payload differentiates between notification types:
 * - DOWNTIME_STARTING_SOON
 * - DOWNTIME_STARTED
 * - DOWNTIME_ENDING_SOON
 * - DOWNTIME_ENDED
 * - NON_RECURRING_UNPAUSED
 */
static const char* get_t2_marker_name(notification_type_t type)
{
    (void)type;  /* Unused - all notifications use same marker */
    return "Aker_Notification";
}
#endif

/**
 * Build JSON array of MAC addresses
 */
static int build_mac_array(
    char *buffer,
    size_t buffer_size,
    uint32_t *mac_indexes,
    size_t mac_count,
    schedule_t *schedule)
{
    size_t offset = 0;
    int ret;

    if (!buffer || buffer_size == 0 || !mac_indexes || mac_count == 0 || !schedule) {
        debug_error("build_mac_array: Invalid parameters\n");
        return -1;
    }

    if (!schedule->macs || schedule->mac_count == 0) {
        debug_error("build_mac_array: Invalid schedule MAC data\n");
        return -1;
    }

    ret = snprintf(buffer + offset, buffer_size - offset, "[");
    if (ret < 0 || (size_t)ret >= buffer_size - offset) {
        debug_error("build_mac_array: Buffer too small for opening bracket\n");
        return -1;
    }
    offset += ret;

    for (size_t i = 0; i < mac_count; i++) {
        if (mac_indexes[i] >= schedule->mac_count) {
            debug_error("build_mac_array: Invalid MAC index %u (max=%zu)\n",
                       mac_indexes[i], schedule->mac_count);
            continue;
        }

        ret = snprintf(buffer + offset, buffer_size - offset, 
                      "%s\"%s\"",
                      (i > 0) ? "," : "",
                      schedule->macs[mac_indexes[i]].mac);
        if (ret < 0 || (size_t)ret >= buffer_size - offset) {
            return -1;
        }
        offset += ret;
    }

    ret = snprintf(buffer + offset, buffer_size - offset, "]");
    if (ret < 0 || (size_t)ret >= buffer_size - offset) {
        return -1;
    }
    offset += ret;

    return (int)offset;
}

/**
 * Send notification event via T2 telemetry
 */
void send_notification_event(
    notification_type_t type,
    time_t scheduled_time,
    uint32_t *mac_indexes,
    size_t mac_count,
    const char *timezone,
    schedule_t *schedule)
{
    char json_payload[4096];
    char iso_timestamp[32];
    char iso_scheduled[32];
    char utc_offset[16];
    char mac_array[2048];
    time_t now = time(NULL);
    const char *event_type_str;
    int ret;

    if (!mac_indexes || mac_count == 0 || !schedule) {
        debug_error("send_notification_event: Invalid parameters\n");
        return;
    }

    event_type_str = get_event_type_string(type);

    /* Format timestamps */
    format_iso8601_utc(now, iso_timestamp);
    format_iso8601_utc(scheduled_time, iso_scheduled);
    calculate_utc_offset(timezone, now, utc_offset);

    /* Build MAC address array */
    ret = build_mac_array(mac_array, sizeof(mac_array), mac_indexes, mac_count, schedule);
    if (ret < 0) {
        debug_error("send_notification_event: Failed to build MAC array\n");
        return;
    }

    /* Build JSON payload based on notification type */
    switch (type) {
        case NOTIFY_DOWNTIME_STARTING_SOON:
        case NOTIFY_DOWNTIME_STARTED:
            ret = snprintf(json_payload, sizeof(json_payload),
                "{\"eventType\":\"%s\","
                "\"timestamp\":\"%s\","
                "\"timeZone\":\"%s\","
                "\"utcOffset\":\"%s\","
                "\"scheduledStartTime\":\"%s\","
                "\"affectedMacs\":%s}",
                event_type_str,
                iso_timestamp,
                timezone ? timezone : "UTC",
                utc_offset,
                iso_scheduled,
                mac_array);
            break;
            
        case NOTIFY_DOWNTIME_ENDING_SOON:
        case NOTIFY_DOWNTIME_ENDED:
            ret = snprintf(json_payload, sizeof(json_payload),
                "{\"eventType\":\"%s\","
                "\"timestamp\":\"%s\","
                "\"timeZone\":\"%s\","
                "\"utcOffset\":\"%s\","
                "\"scheduledEndTime\":\"%s\","
                "\"affectedMacs\":%s}",
                event_type_str,
                iso_timestamp,
                timezone ? timezone : "UTC",
                utc_offset,
                iso_scheduled,
                mac_array);
            break;
            
        case NOTIFY_NON_RECURRING_UNPAUSED:
            ret = snprintf(json_payload, sizeof(json_payload),
                "{\"eventType\":\"%s\","
                "\"timestamp\":\"%s\","
                "\"timeZone\":\"%s\","
                "\"utcOffset\":\"%s\","
                "\"pauseUntilTime\":\"%s\","
                "\"affectedMacs\":%s}",
                event_type_str,
                iso_timestamp,
                timezone ? timezone : "UTC",
                utc_offset,
                iso_scheduled,
                mac_array);
            break;
            
        default:
            debug_error("send_notification_event: Unknown notification type %d\n", type);
            return;
    }

    if (ret < 0 || (size_t)ret >= sizeof(json_payload)) {
        debug_error("send_notification_event: JSON payload too large\n");
        return;
    }

    debug_info("send_notification_event: Sending %s for %zu MACs\n", 
               event_type_str, mac_count);
    debug_info("send_notification_event: Payload: %s\n", json_payload);

#ifdef UNIT_TESTING
    /* Lets tests assert what was actually dispatched; the per-period "sent"
     * flags cannot, since they are also set when a notification is skipped. */
    if (type >= 0 && type <= NOTIFY_NON_RECURRING_UNPAUSED) {
        g_notification_sent_count[type]++;
    }
#endif

#ifdef ENABLE_FEATURE_TELEMETRY2_0
    const char *t2_marker = get_t2_marker_name(type);
    t2_event_s(t2_marker, json_payload);
    debug_info("send_notification_event: T2 event sent: %s\n", t2_marker);

    /* Increment RBUS notification counter to trigger telemetry */
    aker_rbus_increment_notification_count();
#else
    debug_info("send_notification_event: T2 telemetry disabled, payload not sent\n");
#endif
}

time_t get_next_notification_time(
    mac_timeline_collection_t *collection,
    time_t now)
{
    time_t next_time = INT_MAX;
    
    if (!collection || !collection->timelines) {
        return INT_MAX;
    }
    
    /* Walk through all MAC timelines */
    for (size_t mac_idx = 0; mac_idx < collection->mac_count; mac_idx++) {
        mac_block_period_t *period = collection->timelines[mac_idx].periods;

        while (period) {
            /* Validate mac_states array exists */
            if (!period->mac_states) {
                debug_error("get_next_notification_time: NULL mac_states for MAC %zu\n", mac_idx);
                period = period->next;
                continue;
            }

            mac_notification_state_t *state = &period->mac_states[mac_idx];

            /* Check if period is in the future */
            if (period->end_time <= now) {
                period = period->next;
                continue;
            }

            /* Calculate notification times for this period */
            time_t start_soon_time = period->start_time - NOTIFICATION_ADVANCE_TIME_SEC;
            time_t end_soon_time = period->end_time - NOTIFICATION_ADVANCE_TIME_SEC;

            /* Skip "SOON" notifications if period is too short */
            bool skip_soon = (period->end_time - period->start_time) < NOTIFICATION_ADVANCE_TIME_SEC;

            /* Check STARTING_SOON (skip for absolute starts - user already knows) */
            if (!skip_soon && !state->starting_soon_sent && !period->start_is_absolute && start_soon_time >= now) {
                if (start_soon_time < next_time) {
                    next_time = start_soon_time;
                }
            }

            /* Check STARTED (skip for absolute starts - user already knows) */
            if (!state->started_sent && !period->start_is_absolute && period->start_time >= now) {
                if (period->start_time < next_time) {
                    next_time = period->start_time;
                }
            }

            /* Check ENDING_SOON */
            if (!skip_soon && !state->ending_soon_sent && end_soon_time >= now) {
                if (end_soon_time < next_time) {
                    next_time = end_soon_time;
                }
            }

            /* Check ENDED */
            if (!state->ended_sent && period->end_time >= now) {
                if (period->end_time < next_time) {
                    next_time = period->end_time;
                }
            }

            period = period->next;
        }
    }

    if (next_time == INT_MAX) {
        debug_print("get_next_notification_time: No pending notifications\n");
    } else {
        char next_time_iso[32];
        format_iso8601_utc(next_time, next_time_iso);
        debug_info("Next notification triggers at %s (unix:%ld, in %ld seconds)\n",
                   next_time_iso, next_time, next_time - now);
    }

    return next_time;
}

/**
 * Helper to batch MACs with same notification time
 */
typedef struct mac_batch {
    uint32_t mac_indexes[256];  /* Batch up to 256 MACs */
    size_t count;
} mac_batch_t;

/**
 * Send pending notifications with state-change checking and actual sending
 * This is called from scheduler integration
 */
void send_pending_notifications_with_state_check(
    mac_timeline_collection_t *collection,
    schedule_t *schedule,
    time_t now)
{
    if (!collection || !collection->timelines || !schedule) {
        return;
    }

    debug_print("send_pending_notifications_with_state_check: Checking for notifications at %ld\n", now);

    /* Batch notifications by type and time - track scheduled time per batch type */
    mac_batch_t starting_soon_batch = {.count = 0};
    mac_batch_t started_batch = {.count = 0};
    mac_batch_t ending_soon_batch = {.count = 0};
    mac_batch_t ended_batch = {.count = 0};
    mac_batch_t non_recurring_batch = {.count = 0};  /* For absolute schedule expiry */

    time_t starting_soon_time = 0;
    time_t started_time = 0;
    time_t ending_soon_time = 0;
    time_t ended_time = 0;
    time_t non_recurring_time = 0;

    /* Walk through all MAC timelines */
    for (size_t mac_idx = 0; mac_idx < collection->mac_count; mac_idx++) {
        mac_block_period_t *period = collection->timelines[mac_idx].periods;

        while (period) {
            /* Validate mac_states array exists */
            if (!period->mac_states) {
                debug_error("send_pending_notifications_with_state_check: NULL mac_states for MAC %zu\n", mac_idx);
                period = period->next;
                continue;
            }

            /* mac_states is sized for all MACs (total_mac_count), so mac_idx access is safe */
            mac_notification_state_t *state = &period->mac_states[mac_idx];

            /* Skip past periods (but include exact end time for ENDED notification) */
            if (period->end_time < now) {
                period = period->next;
                continue;
            }

            /* Calculate notification times */
            time_t start_soon_time = period->start_time - NOTIFICATION_ADVANCE_TIME_SEC;
            time_t end_soon_time = period->end_time - NOTIFICATION_ADVANCE_TIME_SEC;
            bool skip_soon = (period->end_time - period->start_time) < NOTIFICATION_ADVANCE_TIME_SEC;

            /* Check and batch STARTING_SOON with tight time window (±5 seconds) */
            if (!skip_soon && !state->starting_soon_sent) {
                /* Tight time window: only send if within 5 seconds of notification time */
                time_t time_diff = (now >= start_soon_time) ? (now - start_soon_time) : (start_soon_time - now);

                if (now < start_soon_time) {
                    /* Too early, skip silently (will check again on next scheduler wake) */
                } else if (time_diff > NOTIFICATION_LATE_THRESHOLD_SEC) {
                    state->starting_soon_sent = true;
                    debug_info("send_pending_notifications_with_state_check: Skip late STARTING_SOON for MAC %u (%ld sec past window)\n", mac_idx, time_diff);
                } else if (period->start_is_absolute) {
                    /* Skip if block starts from absolute (user pressed pause - they know!) */
                    state->starting_soon_sent = true;
                    debug_info("send_pending_notifications_with_state_check: Skip STARTING_SOON for MAC %u (absolute start)\n", mac_idx);
                } else {
                    /* Verify device will actually become blocked at start time */
                    bool will_be_blocked = is_mac_blocked_in_timeline(collection, mac_idx, period->start_time);
                    bool currently_blocked = is_mac_blocked_in_timeline(collection, mac_idx, now);

                    if (will_be_blocked && !currently_blocked) {
                        /* If scheduled time changed and batch not empty, send current batch first */
                        if (starting_soon_batch.count > 0 && starting_soon_time != period->start_time) {
                            debug_print("send_pending_notifications_with_state_check: Sending STARTING_SOON for %zu MACs (time changed)\n",
                                       starting_soon_batch.count);
                            send_notification_event(
                                NOTIFY_DOWNTIME_STARTING_SOON,
                                starting_soon_time,
                                starting_soon_batch.mac_indexes,
                                starting_soon_batch.count,
                                collection->time_zone,
                                schedule);
                            starting_soon_batch.count = 0;  /* Reset batch */
                        }

                        if (starting_soon_batch.count < 256) {
                            starting_soon_batch.mac_indexes[starting_soon_batch.count++] = mac_idx;
                            starting_soon_time = period->start_time;
                            state->starting_soon_sent = true;
                        }
                    } else {
                        /* Skip notification but mark as sent to avoid retry */
                        state->starting_soon_sent = true;
                        debug_info("send_pending_notifications_with_state_check: Skip STARTING_SOON for MAC %u (no state change)\n", mac_idx);
                    }
                }
            }

            /* Check and batch STARTED with tight time window (±5 seconds) */
            if (!state->started_sent) {
                time_t time_diff = (now >= period->start_time) ? (now - period->start_time) : (period->start_time - now);

                if (now < period->start_time) {
                    /* Too early */
                } else if (time_diff > NOTIFICATION_LATE_THRESHOLD_SEC) {
                    state->started_sent = true;
                    debug_info("send_pending_notifications_with_state_check: Skip late STARTED for MAC %u (%ld sec past window)\n", mac_idx, time_diff);
                } else if (period->start_is_absolute) {
                    /* Skip if block starts from absolute (user pressed pause - they know!) */
                    state->started_sent = true;
                    debug_info("send_pending_notifications_with_state_check: Skip STARTED for MAC %u (absolute start)\n", mac_idx);
                } else {
                    /* Within time window, check state */
                    bool arrived_late = false;

                    if (!arrived_late) {
                        /* Verify device actually became blocked */
                        bool is_blocked = is_mac_blocked_in_timeline(collection, mac_idx, now);

                        if (is_blocked) {
                            /* If scheduled time changed and batch not empty, send current batch first */
                            if (started_batch.count > 0 && started_time != period->start_time) {
                                debug_print("send_pending_notifications_with_state_check: Sending STARTED for %zu MACs (time changed)\n",
                                           started_batch.count);
                                send_notification_event(
                                    NOTIFY_DOWNTIME_STARTED,
                                    started_time,
                                    started_batch.mac_indexes,
                                    started_batch.count,
                                    collection->time_zone,
                                    schedule);
                                started_batch.count = 0;  /* Reset batch */
                            }

                            if (started_batch.count < 256) {
                                started_batch.mac_indexes[started_batch.count++] = mac_idx;
                                started_time = period->start_time;
                                state->started_sent = true;
                            }
                        } else {
                            state->started_sent = true;
                            debug_info("send_pending_notifications_with_state_check: Skip STARTED for MAC %u (not blocked)\n", mac_idx);
                        }
                    } else {
                        state->started_sent = true;
                        debug_info("send_pending_notifications_with_state_check: Skip late STARTED for MAC %u\n", mac_idx);
                    }
                }
            }

            /* Check and batch ENDING_SOON with tight time window (±5 seconds) */
            if (!skip_soon && !state->ending_soon_sent) {
                time_t time_diff = (now >= end_soon_time) ? (now - end_soon_time) : (end_soon_time - now);

                if (now < end_soon_time) {
                    /* Too early */
                } else if (time_diff > NOTIFICATION_LATE_THRESHOLD_SEC) {
                    state->ending_soon_sent = true;
                    debug_info("send_pending_notifications_with_state_check: Skip late ENDING_SOON for MAC %u (%ld sec past window)\n", mac_idx, time_diff);
                } else if (period->end_is_absolute) {
                    /* Skip if block ends by absolute (will send NON_RECURRING_UNPAUSED instead) */
                    state->ending_soon_sent = true;
                    debug_info("send_pending_notifications_with_state_check: Skip ENDING_SOON for MAC %u (absolute end)\n", mac_idx);
                } else {
                    /* Verify device will actually become unblocked at end time */
                    bool currently_blocked = is_mac_blocked_in_timeline(collection, mac_idx, now);
                    bool will_be_blocked = is_mac_blocked_in_timeline(collection, mac_idx, period->end_time);

                    if (currently_blocked && !will_be_blocked) {
                        /* If scheduled time changed and batch not empty, send current batch first */
                        if (ending_soon_batch.count > 0 && ending_soon_time != period->end_time) {
                            debug_print("send_pending_notifications_with_state_check: Sending ENDING_SOON for %zu MACs (time changed)\n",
                                       ending_soon_batch.count);
                            send_notification_event(
                                NOTIFY_DOWNTIME_ENDING_SOON,
                                ending_soon_time,
                                ending_soon_batch.mac_indexes,
                                ending_soon_batch.count,
                                collection->time_zone,
                                schedule);
                            ending_soon_batch.count = 0;  /* Reset batch */
                        }

                        if (ending_soon_batch.count < 256) {
                            ending_soon_batch.mac_indexes[ending_soon_batch.count++] = mac_idx;
                            ending_soon_time = period->end_time;
                            state->ending_soon_sent = true;
                        }
                    } else {
                        state->ending_soon_sent = true;
                        debug_info("send_pending_notifications_with_state_check: Skip ENDING_SOON for MAC %u (no state change)\n", mac_idx);
                    }
                }
            }

            /* Check and batch ENDED or NON_RECURRING_UNPAUSED with tight time window (±5 seconds) */
            if (!state->ended_sent) {
                time_t time_diff = (now >= period->end_time) ? (now - period->end_time) : (period->end_time - now);

                if (now < period->end_time) {
                    /* Too early */
                } else if (time_diff > NOTIFICATION_LATE_THRESHOLD_SEC) {
                    state->ended_sent = true;
                    debug_info("send_pending_notifications_with_state_check: Skip late ENDED for MAC %u (%ld sec past window)\n", mac_idx, time_diff);
                } else if (period->end_is_absolute) {
                    /* If period ends with absolute, send NON_RECURRING_UNPAUSED */
                    /* Absolute pause expiry - send NON_RECURRING_UNPAUSED */
                    bool was_blocked = is_mac_blocked_in_timeline(collection, mac_idx, period->end_time - 1);
                    bool is_blocked = is_mac_blocked_in_timeline(collection, mac_idx, now);

                    if (was_blocked && !is_blocked) {
                            /* If scheduled time changed and batch not empty, send current batch first */
                            if (non_recurring_batch.count > 0 && non_recurring_time != period->end_time) {
                                debug_print("send_pending_notifications_with_state_check: Sending NON_RECURRING_UNPAUSED for %zu MACs (time changed)\n",
                                           non_recurring_batch.count);
                                send_notification_event(
                                    NOTIFY_NON_RECURRING_UNPAUSED,
                                    non_recurring_time,
                                    non_recurring_batch.mac_indexes,
                                    non_recurring_batch.count,
                                    collection->time_zone,
                                    schedule);
                                non_recurring_batch.count = 0;  /* Reset batch */
                            }

                        /* Send NON_RECURRING_UNPAUSED for natural absolute expiry */
                        if (non_recurring_batch.count < 256) {
                            non_recurring_batch.mac_indexes[non_recurring_batch.count++] = mac_idx;
                            non_recurring_time = period->end_time;
                            state->ended_sent = true;
                        }
                    } else {
                        state->ended_sent = true;
                        debug_info("send_pending_notifications_with_state_check: Skip NON_RECURRING for MAC %u (still blocked)\\n", mac_idx);
                    }
                } else {
                    /* Weekly downtime end - send ENDED */
                    bool was_blocked = is_mac_blocked_in_timeline(collection, mac_idx, period->end_time - 1);
                    bool is_blocked = is_mac_blocked_in_timeline(collection, mac_idx, now);

                    if (was_blocked && !is_blocked) {
                        /* If scheduled time changed and batch not empty, send current batch first */
                        if (ended_batch.count > 0 && ended_time != period->end_time) {
                            debug_print("send_pending_notifications_with_state_check: Sending ENDED for %zu MACs (time changed)\n",
                                       ended_batch.count);
                            send_notification_event(
                                NOTIFY_DOWNTIME_ENDED,
                                ended_time,
                                ended_batch.mac_indexes,
                                ended_batch.count,
                                collection->time_zone,
                                schedule);
                            ended_batch.count = 0;  /* Reset batch */
                        }

                        if (ended_batch.count < 256) {
                            ended_batch.mac_indexes[ended_batch.count++] = mac_idx;
                            ended_time = period->end_time;
                            state->ended_sent = true;
                        }
                    } else {
                        state->ended_sent = true;
                        debug_info("send_pending_notifications_with_state_check: Skip ENDED for MAC %u (no state change)\\n", mac_idx);
                    }
                }
            }

            period = period->next;
        }
    }

    /* Send remaining batched notifications (already cached when added to batch) */
    if (starting_soon_batch.count > 0) {
        debug_print("send_pending_notifications_with_state_check: Sending STARTING_SOON for %zu MACs\n",
                   starting_soon_batch.count);
        send_notification_event(
            NOTIFY_DOWNTIME_STARTING_SOON,
            starting_soon_time,
            starting_soon_batch.mac_indexes,
            starting_soon_batch.count,
            collection->time_zone,
            schedule);
    }

    if (started_batch.count > 0) {
        debug_print("send_pending_notifications_with_state_check: Sending STARTED for %zu MACs\n",
                   started_batch.count);
        send_notification_event(
            NOTIFY_DOWNTIME_STARTED,
            started_time,
            started_batch.mac_indexes,
            started_batch.count,
            collection->time_zone,
            schedule);
    }

    if (ending_soon_batch.count > 0) {
        debug_print("send_pending_notifications_with_state_check: Sending ENDING_SOON for %zu MACs\n",
                   ending_soon_batch.count);
        send_notification_event(
            NOTIFY_DOWNTIME_ENDING_SOON,
            ending_soon_time,
            ending_soon_batch.mac_indexes,
            ending_soon_batch.count,
            collection->time_zone,
            schedule);
    }

    if (ended_batch.count > 0) {
        debug_print("send_pending_notifications_with_state_check: Sending ENDED for %zu MACs\n",
                   ended_batch.count);
        send_notification_event(
            NOTIFY_DOWNTIME_ENDED,
            ended_time,
            ended_batch.mac_indexes,
            ended_batch.count,
            collection->time_zone,
            schedule);
    }

    if (non_recurring_batch.count > 0) {
        debug_print("send_pending_notifications_with_state_check: Sending NON_RECURRING_UNPAUSED for %zu MACs\n",
                   non_recurring_batch.count);
        send_notification_event(
            NOTIFY_NON_RECURRING_UNPAUSED,
            non_recurring_time,
            non_recurring_batch.mac_indexes,
            non_recurring_batch.count,
            collection->time_zone,
            schedule);
    }
}
