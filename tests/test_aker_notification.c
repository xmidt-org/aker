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
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <limits.h>

#include <CUnit/Basic.h>

#include "test_macros.h"
#include "../src/schedule.h"
#include "../src/aker_notification.h"
#include "../src/time.h"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* Jan 4, 1970 00:00:00 UTC is a Sunday - used as a fixed, DST-free week anchor
 * so every test computes exact, reproducible expected timestamps. */
static const time_t DAY_BASE = 259200;

/*----------------------------------------------------------------------------*/
/*                                   Helpers                                  */
/*----------------------------------------------------------------------------*/
static schedule_t* build_schedule(const char *tz, size_t mac_count, const char **macs)
{
    schedule_t *s = create_schedule();
    size_t i;

    CU_ASSERT_PTR_NOT_NULL_FATAL(s);
    s->time_zone = strdup(tz);
    CU_ASSERT_EQUAL(create_mac_table(s, mac_count), 0);
    for (i = 0; i < mac_count; i++) {
        CU_ASSERT_EQUAL(set_mac_index(s, macs[i], strlen(macs[i]), (uint32_t) i), 0);
    }

    return s;
}

static void add_weekly_event(schedule_t *s, time_t weekly_sec, uint32_t *block, size_t block_count)
{
    schedule_event_t *e = create_schedule_event(block_count);
    size_t i;

    CU_ASSERT_PTR_NOT_NULL_FATAL(e);
    e->time = weekly_sec;
    for (i = 0; i < block_count; i++) {
        e->block[i] = block[i];
    }
    CU_ASSERT_EQUAL(insert_event(&s->weekly, e), 0);
}

static void add_absolute_event(schedule_t *s, time_t unix_time, uint32_t *block, size_t block_count)
{
    schedule_event_t *e = create_schedule_event(block_count);
    size_t i;

    CU_ASSERT_PTR_NOT_NULL_FATAL(e);
    e->time = unix_time;
    for (i = 0; i < block_count; i++) {
        e->block[i] = block[i];
    }
    CU_ASSERT_EQUAL(insert_event(&s->absolute, e), 0);
}

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_format_iso8601_utc(void)
{
    char out[32] = {0};

    format_iso8601_utc(0, out);
    CU_ASSERT_STRING_EQUAL(out, "1970-01-01T00:00:00Z");

    format_iso8601_utc(DAY_BASE + 36000, out);
    CU_ASSERT_STRING_EQUAL(out, "1970-01-04T10:00:00Z");
}

void test_calculate_utc_offset(void)
{
    char out[16] = {0};

    calculate_utc_offset("UTC", DAY_BASE, out);
    CU_ASSERT_STRING_EQUAL(out, "+00:00");

    /* PST8PDT is a fixed POSIX TZ rule, no external tzdata needed. */
    calculate_utc_offset("PST8PDT", 1720000000 /* Jul 2024, DST active */, out);
    CU_ASSERT_STRING_EQUAL(out, "-07:00");

    calculate_utc_offset("PST8PDT", 1704000000 /* Dec/Jan, DST inactive */, out);
    CU_ASSERT_STRING_EQUAL(out, "-08:00");
}

void test_is_mac_indefinitely_blocked(void)
{
    const char *macs[] = { "11:11:11:11:11:11" };
    schedule_t *s;
    uint32_t block0[] = { 0 };

    /* Blocked, never unblocked -> indefinite. */
    s = build_schedule("UTC", 1, macs);
    add_weekly_event(s, 36000, block0, 1);
    CU_ASSERT_TRUE(is_mac_indefinitely_blocked(s, 0));
    destroy_schedule(s);

    /* Blocked, then unblocked -> not indefinite. */
    s = build_schedule("UTC", 1, macs);
    add_weekly_event(s, 36000, block0, 1);
    add_weekly_event(s, 39600, NULL, 0);
    CU_ASSERT_FALSE(is_mac_indefinitely_blocked(s, 0));
    destroy_schedule(s);
}

/* Regression test for the wrap-around duplicate bug:
 * finalize_schedule() copies the weekly list's last (highest-time) event and
 * re-inserts it shifted back one week, to seed state for days before the
 * first defined event. When build_timeline_from_schedule() expands weekly
 * events across week=1 of its 2-week lookahead, that synthetic event lands
 * back on the exact same timestamp as the real end-of-week event, creating a
 * second weekly node tied with an absolute event at that same timestamp.
 * MAC0's period end must still resolve to ABSOLUTE, not fall through to a
 * plain WEEKLY end because of the extra tied duplicate. */
void test_absolute_weekly_tie_regression(void)
{
    const char *macs[] = { "11:11:11:11:11:11", "22:22:22:22:22:22" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    uint32_t block01[] = { 0, 1 };
    time_t weekly_start = DAY_BASE + 36000;   /* 10:00:00 Sunday */
    time_t weekly_end   = DAY_BASE + 39600;   /* 11:00:00 Sunday */
    time_t abs_start    = DAY_BASE + 35000;   /* absolute pause begins early */
    time_t abs_true_end = DAY_BASE + 40200;   /* true absolute expiry, after weekly end */
    time_t now          = abs_start + 1;

    s = build_schedule("UTC", 2, macs);
    add_weekly_event(s, 36000, block01, 2);
    add_weekly_event(s, 39600, NULL, 0);

    add_absolute_event(s, abs_start, block0, 1);       /* MAC0 only, before weekly */
    add_absolute_event(s, weekly_start, block01, 2);   /* ties weekly start */
    add_absolute_event(s, weekly_end, block0, 1);      /* ties weekly end */
    add_absolute_event(s, abs_true_end, NULL, 0);       /* true absolute end */

    /* Must match decode.c's real pipeline: finalize_schedule() is what
     * creates the wrap-around synthetic event that reproduces the bug. */
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, now, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);

    /* MAC0: the absolute-driven period for this week must resolve to
     * ABSOLUTE/ABSOLUTE despite the wrap-around duplicate tie (the bug).
     * A second, later period for next week's normal recurrence is expected
     * since build_timeline_from_schedule looks ahead MAX_WEEKS_AHEAD weeks. */
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->start_time, abs_start);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->end_time, abs_true_end);
    CU_ASSERT_TRUE(collection->timelines[0].periods->start_is_absolute);
    CU_ASSERT_TRUE(collection->timelines[0].periods->end_is_absolute);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods->next);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->next->start_time, weekly_start + SECONDS_IN_A_WEEK);
    CU_ASSERT_FALSE(collection->timelines[0].periods->next->start_is_absolute);
    CU_ASSERT_PTR_NULL(collection->timelines[0].periods->next->next);

    /* MAC1: unaffected, plain WEEKLY start/end. */
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[1].periods);
    CU_ASSERT_EQUAL(collection->timelines[1].periods->start_time, weekly_start);
    CU_ASSERT_EQUAL(collection->timelines[1].periods->end_time, weekly_end);
    CU_ASSERT_FALSE(collection->timelines[1].periods->start_is_absolute);
    CU_ASSERT_FALSE(collection->timelines[1].periods->end_is_absolute);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

void test_notification_state_progression(void)
{
    const char *macs[] = { "11:11:11:11:11:11" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    time_t weekly_start = DAY_BASE + 36000;
    time_t weekly_end   = DAY_BASE + 39600;
    time_t start_soon    = weekly_start - NOTIFICATION_ADVANCE_TIME_SEC;
    time_t end_soon      = weekly_end - NOTIFICATION_ADVANCE_TIME_SEC;
    mac_notification_state_t *state;

    s = build_schedule("UTC", 1, macs);
    add_weekly_event(s, 36000, block0, 1);
    add_weekly_event(s, 39600, NULL, 0);

    collection = build_timeline_from_schedule(s, DAY_BASE + 30000, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    state = &collection->timelines[0].periods->mac_states[0];

    send_pending_notifications_with_state_check(collection, s, start_soon);
    CU_ASSERT_TRUE(state->starting_soon_sent);
    CU_ASSERT_FALSE(state->started_sent);

    send_pending_notifications_with_state_check(collection, s, weekly_start);
    CU_ASSERT_TRUE(state->started_sent);
    CU_ASSERT_FALSE(state->ending_soon_sent);

    send_pending_notifications_with_state_check(collection, s, end_soon);
    CU_ASSERT_TRUE(state->ending_soon_sent);
    CU_ASSERT_FALSE(state->ended_sent);

    send_pending_notifications_with_state_check(collection, s, weekly_end);
    CU_ASSERT_TRUE(state->ended_sent);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* "Until I Unpause" - MAC blocked with no unblock event anywhere in the
 * weekly schedule must be entirely skipped from timeline building, not just
 * flagged by is_mac_indefinitely_blocked() in isolation. */
void test_indefinite_block_via_timeline(void)
{
    const char *macs[] = { "11:11:11:11:11:11" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };

    s = build_schedule("UTC", 1, macs);
    add_weekly_event(s, 36000, block0, 1);
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, DAY_BASE + 30000, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);
    CU_ASSERT_PTR_NULL(collection->timelines[0].periods);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* "Infinite block when a schedule is there for later period" - one MAC is
 * blocked with no unblock ever (indefinite), while a second MAC in the same
 * schedule has a normal recurring period. The indefinite MAC's timeline must
 * be skipped without affecting the other MAC's timeline. */
void test_infinite_block_mixed_with_normal_mac(void)
{
    const char *macs[] = { "11:11:11:11:11:11", "22:22:22:22:22:22" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block01[] = { 0, 1 };
    uint32_t block0[] = { 0 };
    time_t weekly_start = DAY_BASE + 36000;
    time_t weekly_end   = DAY_BASE + 39600;

    s = build_schedule("UTC", 2, macs);
    add_weekly_event(s, 36000, block01, 2);  /* both MACs blocked */
    add_weekly_event(s, 39600, block0, 1);   /* MAC1 implicitly unblocked, MAC0 stays blocked forever */
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, DAY_BASE + 30000, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);

    CU_ASSERT_PTR_NULL(collection->timelines[0].periods);

    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[1].periods);
    CU_ASSERT_EQUAL(collection->timelines[1].periods->start_time, weekly_start);
    CU_ASSERT_EQUAL(collection->timelines[1].periods->end_time, weekly_end);
    CU_ASSERT_FALSE(collection->timelines[1].periods->start_is_absolute);
    CU_ASSERT_FALSE(collection->timelines[1].periods->end_is_absolute);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* "Pause for 30 min" - a pure absolute pause with no weekly schedule at all
 * must resolve to a single ABSOLUTE/ABSOLUTE period (-> NON_RECURRING_UNPAUSED),
 * with no recurrence. */
void test_pure_absolute_pause_no_weekly(void)
{
    const char *macs[] = { "11:11:11:11:11:11" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    time_t abs_start = DAY_BASE + 10000;
    time_t abs_end   = DAY_BASE + 11800;

    s = build_schedule("UTC", 1, macs);
    add_absolute_event(s, abs_start, block0, 1);
    add_absolute_event(s, abs_end, NULL, 0);

    collection = build_timeline_from_schedule(s, abs_start + 1, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->start_time, abs_start);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->end_time, abs_end);
    CU_ASSERT_TRUE(collection->timelines[0].periods->start_is_absolute);
    CU_ASSERT_TRUE(collection->timelines[0].periods->end_is_absolute);
    CU_ASSERT_PTR_NULL(collection->timelines[0].periods->next);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* is_device_blocked_at() and its two static helpers (is_in_absolute_blocking_
 * period, is_in_weekly_blocking_period) operate directly on the raw schedule
 * (not the built timeline) and were entirely uncovered. Exercise both the
 * absolute and weekly paths, plus both blocked and unblocked outcomes. */
void test_is_device_blocked_at(void)
{
    const char *macs[] = { "11:11:11:11:11:11" };
    schedule_t *s;
    uint32_t block0[] = { 0 };
    time_t weekly_start = DAY_BASE + 36000;
    time_t weekly_end   = DAY_BASE + 39600;
    time_t abs_start    = DAY_BASE + 50000;
    time_t abs_end      = DAY_BASE + 51000;

    s = build_schedule("UTC", 1, macs);
    add_weekly_event(s, 36000, block0, 1);
    add_weekly_event(s, 39600, NULL, 0);
    add_absolute_event(s, abs_start, block0, 1);
    add_absolute_event(s, abs_end, NULL, 0);

    /* Within the weekly window, before any absolute event exists yet. */
    CU_ASSERT_TRUE(is_device_blocked_at(s, 0, weekly_start + 1000));

    /* Between the weekly end and the absolute start - blocked by neither. */
    CU_ASSERT_FALSE(is_device_blocked_at(s, 0, weekly_end + 5000));

    /* Within the absolute pause window. */
    CU_ASSERT_TRUE(is_device_blocked_at(s, 0, abs_start + 500));

    /* After the absolute pause ends and outside the weekly window. */
    CU_ASSERT_FALSE(is_device_blocked_at(s, 0, abs_end + 500));

    destroy_schedule(s);
}

/* Full NON_RECURRING_UNPAUSED notification flow for a pure absolute pause:
 * STARTING_SOON/STARTED/ENDING_SOON must be skipped as "absolute", and the
 * actual expiry must go through the NON_RECURRING_UNPAUSED send path in
 * send_pending_notifications_with_state_check() (and the matching payload
 * branch in send_notification_event()), not just be inspected on the period. */
void test_absolute_pause_full_notification_flow(void)
{
    const char *macs[] = { "11:11:11:11:11:11" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    time_t abs_start = DAY_BASE + 10000;
    time_t abs_end   = DAY_BASE + 11800;
    time_t start_soon = abs_start - NOTIFICATION_ADVANCE_TIME_SEC;
    time_t end_soon   = abs_end - NOTIFICATION_ADVANCE_TIME_SEC;
    mac_notification_state_t *state;

    s = build_schedule("UTC", 1, macs);
    add_absolute_event(s, abs_start, block0, 1);
    add_absolute_event(s, abs_end, NULL, 0);

    collection = build_timeline_from_schedule(s, abs_start + 1, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    state = &collection->timelines[0].periods->mac_states[0];

    send_pending_notifications_with_state_check(collection, s, start_soon);
    CU_ASSERT_TRUE(state->starting_soon_sent);

    send_pending_notifications_with_state_check(collection, s, abs_start);
    CU_ASSERT_TRUE(state->started_sent);

    send_pending_notifications_with_state_check(collection, s, end_soon);
    CU_ASSERT_TRUE(state->ending_soon_sent);
    CU_ASSERT_FALSE(state->ended_sent);

    send_pending_notifications_with_state_check(collection, s, abs_end);
    CU_ASSERT_TRUE(state->ended_sent);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* aker_notification_init()/aker_notification_cleanup() only touch the
 * internal g_timezone buffer, which has no getter - just exercise both
 * branches (timezone given / NULL) plus cleanup without crashing. */
void test_aker_notification_init_and_cleanup(void)
{
    aker_notification_init("America/New_York");
    aker_notification_init(NULL);
    aker_notification_cleanup();
    CU_PASS("aker_notification_init/cleanup did not crash");
}

/* get_next_notification_time() walks periods/states to find the earliest
 * pending notification. Exercise NULL collection, the STARTING_SOON path,
 * later state transitions (ENDING_SOON, ENDED), and the past-period skip. */
void test_get_next_notification_time(void)
{
    const char *macs[] = { "11:11:11:11:11:11" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    time_t weekly_start = DAY_BASE + 36000;
    time_t weekly_end   = DAY_BASE + 39600;
    mac_notification_state_t *state;
    time_t next;

    CU_ASSERT_EQUAL(get_next_notification_time(NULL, 0), INT_MAX);

    s = build_schedule("UTC", 1, macs);
    add_weekly_event(s, 36000, block0, 1);
    add_weekly_event(s, 39600, NULL, 0);

    collection = build_timeline_from_schedule(s, weekly_start - 2000, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    state = &collection->timelines[0].periods->mac_states[0];

    /* Nothing sent yet: earliest pending event is STARTING_SOON. */
    next = get_next_notification_time(collection, weekly_start - 2000);
    CU_ASSERT_EQUAL(next, weekly_start - NOTIFICATION_ADVANCE_TIME_SEC);

    /* STARTING_SOON and STARTED already sent: next pending is ENDING_SOON. */
    state->starting_soon_sent = true;
    state->started_sent = true;
    next = get_next_notification_time(collection, weekly_start - 2000);
    CU_ASSERT_EQUAL(next, weekly_end - NOTIFICATION_ADVANCE_TIME_SEC);

    /* Everything but ENDED sent: next pending is the ENDED time itself. */
    state->ending_soon_sent = true;
    next = get_next_notification_time(collection, weekly_start - 2000);
    CU_ASSERT_EQUAL(next, weekly_end);

    /* now moves past ALL periods - weeks_ahead=MAX_WEEKS_AHEAD builds a
     * second period next week too, so both must be behind "now" for nothing
     * to remain pending. */
    next = get_next_notification_time(collection, weekly_end + SECONDS_IN_A_WEEK + 1);
    CU_ASSERT_EQUAL(next, INT_MAX);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Absolute-start periods skip STARTING_SOON/STARTED regardless of sent state,
 * and a period shorter than the advance window (skip_soon) also skips
 * ENDING_SOON - only ENDED should remain pending. */
void test_get_next_notification_time_absolute_and_short_period(void)
{
    const char *macs[] = { "11:11:11:11:11:11" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    time_t abs_start = DAY_BASE + 10000;
    time_t abs_end   = DAY_BASE + 10500; /* 500s: shorter than the 900s advance window */
    time_t next;

    s = build_schedule("UTC", 1, macs);
    add_absolute_event(s, abs_start, block0, 1);
    add_absolute_event(s, abs_end, NULL, 0);

    collection = build_timeline_from_schedule(s, abs_start + 1, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);

    next = get_next_notification_time(collection, abs_start + 1);
    CU_ASSERT_EQUAL(next, abs_end);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Mirrors scheduler.c's "structure changed -> destroy + rebuild" flow: after
 * the schedule is mutated (a new absolute pause added), recalculating the
 * timeline from the SAME schedule pointer must reflect the new event, not
 * stale data from the first build. */
void test_timeline_recalculation_on_schedule_change(void)
{
    const char *macs[] = { "11:11:11:11:11:11" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    time_t weekly_start = DAY_BASE + 36000;
    time_t abs_start;
    time_t abs_end;

    s = build_schedule("UTC", 1, macs);
    add_weekly_event(s, 36000, block0, 1);
    add_weekly_event(s, 39600, NULL, 0);

    /* First build: pure weekly schedule. */
    collection = build_timeline_from_schedule(s, weekly_start - 2000, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->start_time, weekly_start);
    CU_ASSERT_FALSE(collection->timelines[0].periods->start_is_absolute);
    destroy_timeline_collection(collection);

    /* Schedule structure changes: a new absolute pause is added, starting
     * before the weekly window. */
    abs_start = weekly_start - 1000;
    abs_end   = weekly_start - 500;
    add_absolute_event(s, abs_start, block0, 1);
    add_absolute_event(s, abs_end, NULL, 0);

    /* Recalculate from the same schedule pointer - must reflect the newly
     * added absolute event, not the stale first-build result. */
    collection = build_timeline_from_schedule(s, weekly_start - 2000, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->start_time, abs_start);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->end_time, abs_end);
    CU_ASSERT_TRUE(collection->timelines[0].periods->start_is_absolute);
    CU_ASSERT_TRUE(collection->timelines[0].periods->end_is_absolute);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Regression test for a real field report (RDKB-65401): an absolute pause
 * fully ENCLOSES a weekly window (starts before weekly start, ends after
 * weekly end) for the SAME MACs, with NO backend-injected tie events at the
 * weekly's own boundaries (unlike Cases 1/2/3 below). Previously the
 * weekly's own end event prematurely closed the period at the weekly end
 * time (treating it as a WEEKLY end), silently dropping the true absolute
 * end and causing DOWNTIME_ENDING_SOON/DOWNTIME_ENDED to be sent instead of
 * NON_RECURRING_UNPAUSED. */
void test_absolute_fully_encloses_weekly_no_tie_events(void)
{
    const char *macs[] = { "11:11:11:11:11:11", "22:22:22:22:22:22" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block01[] = { 0, 1 };
    time_t weekly_start = DAY_BASE + 7200;
    time_t weekly_end   = DAY_BASE + 8160;
    time_t abs_start    = weekly_start - 1320;
    time_t abs_end      = weekly_end + 1320;

    s = build_schedule("UTC", 2, macs);
    add_weekly_event(s, 7200, block01, 2);
    add_weekly_event(s, 8160, NULL, 0);
    add_absolute_event(s, abs_start, block01, 2);
    add_absolute_event(s, abs_end, NULL, 0);

    collection = build_timeline_from_schedule(s, abs_start + 1, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);

    for (size_t mac = 0; mac < 2; mac++) {
        CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[mac].periods);
        CU_ASSERT_EQUAL(collection->timelines[mac].periods->start_time, abs_start);
        CU_ASSERT_EQUAL(collection->timelines[mac].periods->end_time, abs_end);
        CU_ASSERT_TRUE(collection->timelines[mac].periods->start_is_absolute);
        CU_ASSERT_TRUE(collection->timelines[mac].periods->end_is_absolute);
    }

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Corner case per AC: a genuinely absolute-controlled MAC (absolute start has
 * no weekly tie) whose absolute END happens to fall on the EXACT SAME
 * timestamp as the weekly's own end. Must still resolve as ABSOLUTE end
 * (skip ENDING_SOON/ENDED, send NON_RECURRING_UNPAUSED) - a coincidental
 * timestamp match must not misclassify it as a normal weekly end. */
void test_absolute_end_exactly_ties_weekly_end_stays_absolute(void)
{
    const char *macs[] = { "11:11:11:11:11:11" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    time_t weekly_start = DAY_BASE + 36000;
    time_t weekly_end   = DAY_BASE + 39600;
    time_t abs_start     = weekly_start - 1000;   /* before weekly start, no tie */
    time_t abs_end       = weekly_end;             /* exactly ties weekly's own end */

    s = build_schedule("UTC", 1, macs);
    add_weekly_event(s, 36000, block0, 1);
    add_weekly_event(s, 39600, NULL, 0);
    add_absolute_event(s, abs_start, block0, 1);
    add_absolute_event(s, abs_end, NULL, 0);

    collection = build_timeline_from_schedule(s, abs_start + 1, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->start_time, abs_start);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->end_time, abs_end);
    CU_ASSERT_TRUE(collection->timelines[0].periods->start_is_absolute);
    CU_ASSERT_TRUE(collection->timelines[0].periods->end_is_absolute);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* ---------------------------------------------------------------------------
 * AC rule matrix. Each period resolves to one of four (start,end) shapes:
 *   R1 ABS/ABS     -> skip all weekly, send NON_RECURRING_UNPAUSED
 *   R2 ABS/WEEKLY  -> skip STARTING_SOON+STARTED, send ENDING_SOON+ENDED
 *   R3 WEEKLY/WEEK -> all 4 weekly notifications
 *   R4 WEEKLY/ABS  -> STARTING_SOON+STARTED, skip ENDING_SOON, NON_RECURRING_UNPAUSED
 *   R5 absolute end tying weekly end wins for a genuinely absolute-held period
 * --------------------------------------------------------------------------- */

/* R1, single MAC: pure user pause, no weekly at all. */
void test_rule1_abs_abs_single_mac(void)
{
    const char *macs[] = { "11:11:11:11:11:11" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    time_t abs_start = DAY_BASE + 20000;
    time_t abs_end   = DAY_BASE + 23600;

    s = build_schedule("UTC", 1, macs);
    add_absolute_event(s, abs_start, block0, 1);
    add_absolute_event(s, abs_end, NULL, 0);

    collection = build_timeline_from_schedule(s, abs_start - 100, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    CU_ASSERT_TRUE(collection->timelines[0].periods->start_is_absolute);
    CU_ASSERT_TRUE(collection->timelines[0].periods->end_is_absolute);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* R2, single MAC: absolute starts early, hands off at the weekly start tie,
 * so the weekly's own end closes the period. */
void test_rule2_abs_start_weekly_end_single_mac(void)
{
    const char *macs[] = { "11:11:11:11:11:11" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    time_t weekly_start = DAY_BASE + 36000;
    time_t weekly_end   = DAY_BASE + 39600;
    time_t abs_start    = weekly_start - 1500;

    s = build_schedule("UTC", 1, macs);
    add_weekly_event(s, 36000, block0, 1);
    add_weekly_event(s, 39600, NULL, 0);
    add_absolute_event(s, abs_start, block0, 1);
    add_absolute_event(s, weekly_start, block0, 1);  /* hand-off tie at weekly start */

    collection = build_timeline_from_schedule(s, abs_start + 1, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->start_time, abs_start);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->end_time, weekly_end);
    CU_ASSERT_TRUE(collection->timelines[0].periods->start_is_absolute);
    CU_ASSERT_FALSE(collection->timelines[0].periods->end_is_absolute);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* R3, two MACs sharing one weekly profile: plain recurring, all 4 for both. */
void test_rule3_weekly_weekly_two_macs(void)
{
    const char *macs[] = { "11:11:11:11:11:11", "22:22:22:22:22:22" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block01[] = { 0, 1 };
    time_t weekly_start = DAY_BASE + 36000;
    time_t weekly_end   = DAY_BASE + 39600;
    size_t mac;

    s = build_schedule("UTC", 2, macs);
    add_weekly_event(s, 36000, block01, 2);
    add_weekly_event(s, 39600, NULL, 0);

    collection = build_timeline_from_schedule(s, weekly_start - 2000, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);

    for (mac = 0; mac < 2; mac++) {
        CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[mac].periods);
        CU_ASSERT_EQUAL(collection->timelines[mac].periods->start_time, weekly_start);
        CU_ASSERT_EQUAL(collection->timelines[mac].periods->end_time, weekly_end);
        CU_ASSERT_FALSE(collection->timelines[mac].periods->start_is_absolute);
        CU_ASSERT_FALSE(collection->timelines[mac].periods->end_is_absolute);
    }

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* R4, single MAC: weekly starts normally, then a user pause extends past the
 * weekly end, so the period closes on the absolute expiry. */
void test_rule4_weekly_start_abs_end_single_mac(void)
{
    const char *macs[] = { "11:11:11:11:11:11" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    time_t weekly_start = DAY_BASE + 36000;
    time_t weekly_end   = DAY_BASE + 39600;
    time_t abs_end      = weekly_end + 1800;

    s = build_schedule("UTC", 1, macs);
    add_weekly_event(s, 36000, block0, 1);
    add_weekly_event(s, 39600, NULL, 0);
    add_absolute_event(s, weekly_start + 600, block0, 1);  /* pause asserted mid-weekly */
    add_absolute_event(s, abs_end, NULL, 0);

    collection = build_timeline_from_schedule(s, weekly_start - 2000, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->start_time, weekly_start);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->end_time, abs_end);
    CU_ASSERT_FALSE(collection->timelines[0].periods->start_is_absolute);
    CU_ASSERT_TRUE(collection->timelines[0].periods->end_is_absolute);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* R5 with a WEEKLY start: absolute end lands exactly on the weekly end, and
 * the MAC is genuinely under an absolute pause - absolute must win, giving
 * the R4 shape (WEEKLY start, ABSOLUTE end). */
void test_rule5_abs_end_ties_weekly_end_after_weekly_start(void)
{
    const char *macs[] = { "11:11:11:11:11:11" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    time_t weekly_start = DAY_BASE + 36000;
    time_t weekly_end   = DAY_BASE + 39600;

    s = build_schedule("UTC", 1, macs);
    add_weekly_event(s, 36000, block0, 1);
    add_weekly_event(s, 39600, NULL, 0);
    add_absolute_event(s, weekly_start + 900, block0, 1); /* genuine pause, no tie */
    add_absolute_event(s, weekly_end, NULL, 0);           /* ends exactly at weekly end */

    collection = build_timeline_from_schedule(s, weekly_start - 2000, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->start_time, weekly_start);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->end_time, weekly_end);
    CU_ASSERT_FALSE(collection->timelines[0].periods->start_is_absolute);
    CU_ASSERT_TRUE(collection->timelines[0].periods->end_is_absolute);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* R5 must NOT fire for a bystander MAC: MAC1 is only in the weekly profile,
 * MAC0 holds the absolute pause. MAC1's weekly end coincides with MAC0's
 * absolute event timestamp but MAC1 stays a pure WEEKLY/WEEKLY period. */
void test_rule5_does_not_leak_to_weekly_only_mac(void)
{
    const char *macs[] = { "11:11:11:11:11:11", "22:22:22:22:22:22" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    uint32_t block01[] = { 0, 1 };
    time_t weekly_start = DAY_BASE + 36000;
    time_t weekly_end   = DAY_BASE + 39600;
    time_t abs_start    = weekly_start - 1200;
    time_t abs_end      = weekly_end + 300;

    s = build_schedule("UTC", 2, macs);
    add_weekly_event(s, 36000, block01, 2);
    add_weekly_event(s, 39600, NULL, 0);
    add_absolute_event(s, abs_start, block0, 1);      /* MAC0 only */
    add_absolute_event(s, weekly_start, block01, 2);  /* backend pad at weekly start */
    add_absolute_event(s, weekly_end, block0, 1);     /* MAC1 released, MAC0 held */
    add_absolute_event(s, abs_end, NULL, 0);

    collection = build_timeline_from_schedule(s, abs_start + 1, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);

    /* MAC0: genuine absolute pause on both ends. */
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->start_time, abs_start);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->end_time, abs_end);
    CU_ASSERT_TRUE(collection->timelines[0].periods->start_is_absolute);
    CU_ASSERT_TRUE(collection->timelines[0].periods->end_is_absolute);

    /* MAC1: untouched by the pause - normal weekly on both ends. */
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[1].periods);
    CU_ASSERT_EQUAL(collection->timelines[1].periods->start_time, weekly_start);
    CU_ASSERT_EQUAL(collection->timelines[1].periods->end_time, weekly_end);
    CU_ASSERT_FALSE(collection->timelines[1].periods->start_is_absolute);
    CU_ASSERT_FALSE(collection->timelines[1].periods->end_is_absolute);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Two overlapping profiles plus a pause: MAC0+MAC1 on profile A, MAC2 on a
 * later profile B, MAC3 holding a solo absolute pause. Each MAC must resolve
 * independently to its own rule shape. */
void test_rules_mixed_two_profiles_and_absolute_mac(void)
{
    const char *macs[] = { "11:11:11:11:11:11", "22:22:22:22:22:22",
                           "33:33:33:33:33:33", "44:44:44:44:44:44" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t blockA[]  = { 0, 1 };
    uint32_t blockAB[] = { 0, 1, 2 };
    uint32_t blockB[]  = { 2 };
    uint32_t block3[]  = { 3 };
    time_t a_start = DAY_BASE + 36000;
    time_t b_start = DAY_BASE + 37200;
    time_t a_end   = DAY_BASE + 39600;
    time_t b_end   = DAY_BASE + 41400;
    time_t abs_start = DAY_BASE + 34800;
    time_t abs_end   = DAY_BASE + 36600;

    s = build_schedule("UTC", 4, macs);
    add_weekly_event(s, 36000, blockA, 2);   /* profile A starts */
    add_weekly_event(s, 37200, blockAB, 3);  /* profile B adds MAC2 */
    add_weekly_event(s, 39600, blockB, 1);   /* profile A ends, MAC2 stays */
    add_weekly_event(s, 41400, NULL, 0);     /* profile B ends */
    add_absolute_event(s, abs_start, block3, 1);
    add_absolute_event(s, abs_end, NULL, 0);

    collection = build_timeline_from_schedule(s, abs_start + 1, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);

    /* MAC0/MAC1 - profile A, pure weekly (R3). */
    for (size_t mac = 0; mac < 2; mac++) {
        CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[mac].periods);
        CU_ASSERT_EQUAL(collection->timelines[mac].periods->start_time, a_start);
        CU_ASSERT_EQUAL(collection->timelines[mac].periods->end_time, a_end);
        CU_ASSERT_FALSE(collection->timelines[mac].periods->start_is_absolute);
        CU_ASSERT_FALSE(collection->timelines[mac].periods->end_is_absolute);
    }

    /* MAC2 - profile B, pure weekly (R3) on its own later window. */
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[2].periods);
    CU_ASSERT_EQUAL(collection->timelines[2].periods->start_time, b_start);
    CU_ASSERT_EQUAL(collection->timelines[2].periods->end_time, b_end);
    CU_ASSERT_FALSE(collection->timelines[2].periods->start_is_absolute);
    CU_ASSERT_FALSE(collection->timelines[2].periods->end_is_absolute);

    /* MAC3 - solo pause, absolute on both ends (R1). */
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[3].periods);
    CU_ASSERT_EQUAL(collection->timelines[3].periods->start_time, abs_start);
    CU_ASSERT_EQUAL(collection->timelines[3].periods->end_time, abs_end);
    CU_ASSERT_TRUE(collection->timelines[3].periods->start_is_absolute);
    CU_ASSERT_TRUE(collection->timelines[3].periods->end_is_absolute);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Absolute pause expires while the weekly window is still blocking: the
 * period must stay open and close on the WEEKLY end, not on the absolute
 * expiry. Real shape: pause 9:00-9:30, weekly 9:20-10:00, both MACs in both.
 * Expected per AC: skip STARTING_SOON/STARTED, send ENDING_SOON + ENDED. */
void test_abs_expires_while_weekly_active_two_macs(void)
{
    const char *macs[] = { "11:11:11:11:11:11", "22:22:22:22:22:22" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block01[] = { 0, 1 };
    time_t abs_start    = DAY_BASE + 32400;  /* 09:00 */
    time_t weekly_start = DAY_BASE + 33600;  /* 09:20 */
    time_t abs_end      = DAY_BASE + 34200;  /* 09:30 */
    time_t weekly_end   = DAY_BASE + 36000;  /* 10:00 */
    size_t mac;

    s = build_schedule("UTC", 2, macs);
    add_weekly_event(s, 33600, block01, 2);
    add_weekly_event(s, 36000, NULL, 0);
    add_absolute_event(s, abs_start, block01, 2);
    add_absolute_event(s, abs_end, NULL, 0);
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, abs_start - 60, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);

    for (mac = 0; mac < 2; mac++) {
        CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[mac].periods);
        CU_ASSERT_EQUAL(collection->timelines[mac].periods->start_time, abs_start);
        CU_ASSERT_EQUAL(collection->timelines[mac].periods->end_time, weekly_end);
        CU_ASSERT_TRUE(collection->timelines[mac].periods->start_is_absolute);
        CU_ASSERT_FALSE(collection->timelines[mac].periods->end_is_absolute);
    }
    (void) weekly_start;

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Near-miss ordering: absolute ends one second BEFORE the weekly end. Weekly
 * still holds for that second, so the period closes WEEKLY at the later time. */
void test_abs_end_one_second_before_weekly_end(void)
{
    const char *macs[] = { "11:11:11:11:11:11" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    time_t weekly_start = DAY_BASE + 36000;
    time_t weekly_end   = DAY_BASE + 39600;
    time_t abs_start    = weekly_start - 1200;
    time_t abs_end      = weekly_end - 1;

    s = build_schedule("UTC", 1, macs);
    add_weekly_event(s, 36000, block0, 1);
    add_weekly_event(s, 39600, NULL, 0);
    add_absolute_event(s, abs_start, block0, 1);
    add_absolute_event(s, abs_end, NULL, 0);
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, abs_start - 60, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->start_time, abs_start);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->end_time, weekly_end);
    CU_ASSERT_TRUE(collection->timelines[0].periods->start_is_absolute);
    CU_ASSERT_FALSE(collection->timelines[0].periods->end_is_absolute);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Near-miss ordering: absolute ends one second AFTER the weekly end. The pause
 * outlives weekly, so the period closes ABSOLUTE at the later time. */
void test_abs_end_one_second_after_weekly_end(void)
{
    const char *macs[] = { "11:11:11:11:11:11" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    time_t weekly_start = DAY_BASE + 36000;
    time_t weekly_end   = DAY_BASE + 39600;
    time_t abs_start    = weekly_start - 1200;
    time_t abs_end      = weekly_end + 1;

    s = build_schedule("UTC", 1, macs);
    add_weekly_event(s, 36000, block0, 1);
    add_weekly_event(s, 39600, NULL, 0);
    add_absolute_event(s, abs_start, block0, 1);
    add_absolute_event(s, abs_end, NULL, 0);
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, abs_start - 60, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->start_time, abs_start);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->end_time, abs_end);
    CU_ASSERT_TRUE(collection->timelines[0].periods->start_is_absolute);
    CU_ASSERT_TRUE(collection->timelines[0].periods->end_is_absolute);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Asymmetric membership: only MAC0 is paused (09:00-09:30) but the weekly
 * window (09:20-10:00) covers MAC0 and MAC1. MAC0 hands off to weekly at the
 * pause expiry; MAC1 must be completely untouched by the absolute events. */
void test_abs_mac0_only_weekly_both_macs(void)
{
    const char *macs[] = { "11:11:11:11:11:11", "22:22:22:22:22:22" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[]  = { 0 };
    uint32_t block01[] = { 0, 1 };
    time_t abs_start    = DAY_BASE + 32400;  /* 09:00 */
    time_t weekly_start = DAY_BASE + 33600;  /* 09:20 */
    time_t abs_end      = DAY_BASE + 34200;  /* 09:30 */
    time_t weekly_end   = DAY_BASE + 36000;  /* 10:00 */

    s = build_schedule("UTC", 2, macs);
    add_weekly_event(s, 33600, block01, 2);
    add_weekly_event(s, 36000, NULL, 0);
    add_absolute_event(s, abs_start, block0, 1);
    add_absolute_event(s, abs_end, NULL, 0);
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, abs_start - 60, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);

    /* MAC0: paused, then weekly keeps blocking -> ABSOLUTE start, WEEKLY end */
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->start_time, abs_start);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->end_time, weekly_end);
    CU_ASSERT_TRUE(collection->timelines[0].periods->start_is_absolute);
    CU_ASSERT_FALSE(collection->timelines[0].periods->end_is_absolute);

    /* MAC1: weekly only - the pause it never joined must not clip its period */
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[1].periods);
    CU_ASSERT_EQUAL(collection->timelines[1].periods->start_time, weekly_start);
    CU_ASSERT_EQUAL(collection->timelines[1].periods->end_time, weekly_end);
    CU_ASSERT_FALSE(collection->timelines[1].periods->start_is_absolute);
    CU_ASSERT_FALSE(collection->timelines[1].periods->end_is_absolute);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Absolute pause 21:00-23:00 fully encloses weekly 21:30-22:00 for both MACs.
 * Weekly has already ended when the pause expires, so the whole span stays a
 * single ABSOLUTE/ABSOLUTE period (NON_RECURRING_UNPAUSED only). */
void test_absolute_encloses_weekly_both_macs(void)
{
    const char *macs[] = { "11:11:11:11:11:11", "22:22:22:22:22:22" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block01[] = { 0, 1 };
    time_t abs_start = DAY_BASE + 75600;  /* 21:00 */
    time_t abs_end   = DAY_BASE + 82800;  /* 23:00 */
    size_t mac;

    s = build_schedule("UTC", 2, macs);
    add_weekly_event(s, 77400, block01, 2);   /* 21:30 */
    add_weekly_event(s, 79200, NULL, 0);      /* 22:00 */
    add_absolute_event(s, abs_start, block01, 2);
    add_absolute_event(s, abs_end, NULL, 0);
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, abs_start - 60, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);

    for (mac = 0; mac < 2; mac++) {
        CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[mac].periods);
        CU_ASSERT_EQUAL(collection->timelines[mac].periods->start_time, abs_start);
        CU_ASSERT_EQUAL(collection->timelines[mac].periods->end_time, abs_end);
        CU_ASSERT_TRUE(collection->timelines[mac].periods->start_is_absolute);
        CU_ASSERT_TRUE(collection->timelines[mac].periods->end_is_absolute);
    }

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Same enclosing pause 21:00-23:00, but only MAC0 is paused. MAC0 collapses to
 * one absolute period; MAC1 keeps its plain weekly 21:30-22:00 period. */
void test_absolute_encloses_weekly_abs_mac0_only(void)
{
    const char *macs[] = { "11:11:11:11:11:11", "22:22:22:22:22:22" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[]  = { 0 };
    uint32_t block01[] = { 0, 1 };
    time_t abs_start    = DAY_BASE + 75600;  /* 21:00 */
    time_t weekly_start = DAY_BASE + 77400;  /* 21:30 */
    time_t weekly_end   = DAY_BASE + 79200;  /* 22:00 */
    time_t abs_end      = DAY_BASE + 82800;  /* 23:00 */

    s = build_schedule("UTC", 2, macs);
    add_weekly_event(s, 77400, block01, 2);
    add_weekly_event(s, 79200, NULL, 0);
    add_absolute_event(s, abs_start, block0, 1);
    add_absolute_event(s, abs_end, NULL, 0);
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, abs_start - 60, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);

    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->start_time, abs_start);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->end_time, abs_end);
    CU_ASSERT_TRUE(collection->timelines[0].periods->start_is_absolute);
    CU_ASSERT_TRUE(collection->timelines[0].periods->end_is_absolute);

    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[1].periods);
    CU_ASSERT_EQUAL(collection->timelines[1].periods->start_time, weekly_start);
    CU_ASSERT_EQUAL(collection->timelines[1].periods->end_time, weekly_end);
    CU_ASSERT_FALSE(collection->timelines[1].periods->start_is_absolute);
    CU_ASSERT_FALSE(collection->timelines[1].periods->end_is_absolute);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/*----------------------------------------------------------------------------*/
/*                          Dispatch-level helpers                            */
/*----------------------------------------------------------------------------*/
extern int g_notification_sent_count[NOTIFY_NON_RECURRING_UNPAUSED + 1];

static void reset_sent_counts(void)
{
    size_t i;
    for (i = 0; i <= (size_t) NOTIFY_NON_RECURRING_UNPAUSED; i++) {
        g_notification_sent_count[i] = 0;
    }
}

/* Drives the dispatcher through every notification instant of a period. */
static void run_full_dispatch(mac_timeline_collection_t *collection, schedule_t *s,
                              time_t start, time_t end)
{
    send_pending_notifications_with_state_check(collection, s, start - NOTIFICATION_ADVANCE_TIME_SEC);
    send_pending_notifications_with_state_check(collection, s, start);
    send_pending_notifications_with_state_check(collection, s, end - NOTIFICATION_ADVANCE_TIME_SEC);
    send_pending_notifications_with_state_check(collection, s, end);
}

/* Rule 3 dispatch: a plain weekly window must emit all four notifications. */
void test_dispatch_rule3_weekly_sends_all_four(void)
{
    const char *macs[] = { "11:11:11:11:11:11" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    time_t weekly_start = DAY_BASE + 36000;
    time_t weekly_end   = DAY_BASE + 39600;

    s = build_schedule("UTC", 1, macs);
    add_weekly_event(s, 36000, block0, 1);
    add_weekly_event(s, 39600, NULL, 0);
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, weekly_start - 2000, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);

    reset_sent_counts();
    run_full_dispatch(collection, s, weekly_start, weekly_end);

    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_STARTING_SOON], 1);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_STARTED], 1);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_ENDING_SOON], 1);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_ENDED], 1);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_NON_RECURRING_UNPAUSED], 0);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Rule 1 dispatch: a pure pause emits only NON_RECURRING_UNPAUSED. */
void test_dispatch_rule1_absolute_only_unpause(void)
{
    const char *macs[] = { "11:11:11:11:11:11" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    time_t abs_start = DAY_BASE + 36000;
    time_t abs_end   = DAY_BASE + 41400;

    s = build_schedule("UTC", 1, macs);
    add_absolute_event(s, abs_start, block0, 1);
    add_absolute_event(s, abs_end, NULL, 0);
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, abs_start - 2000, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);

    reset_sent_counts();
    run_full_dispatch(collection, s, abs_start, abs_end);

    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_STARTING_SOON], 0);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_STARTED], 0);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_ENDING_SOON], 0);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_ENDED], 0);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_NON_RECURRING_UNPAUSED], 1);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Rule 2 dispatch, the 9:00-9:30 pause under a 9:20-10:00 weekly window:
 * start notifications suppressed, end notifications delivered. */
void test_dispatch_rule2_abs_start_weekly_end(void)
{
    const char *macs[] = { "11:11:11:11:11:11", "22:22:22:22:22:22" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block01[] = { 0, 1 };
    time_t abs_start  = DAY_BASE + 32400;
    time_t abs_end    = DAY_BASE + 34200;
    time_t weekly_end = DAY_BASE + 36000;

    s = build_schedule("UTC", 2, macs);
    add_weekly_event(s, 33600, block01, 2);
    add_weekly_event(s, 36000, NULL, 0);
    add_absolute_event(s, abs_start, block01, 2);
    add_absolute_event(s, abs_end, NULL, 0);
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, abs_start - 60, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);

    reset_sent_counts();
    run_full_dispatch(collection, s, abs_start, weekly_end);

    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_STARTING_SOON], 0);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_STARTED], 0);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_ENDING_SOON], 1);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_ENDED], 1);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_NON_RECURRING_UNPAUSED], 0);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Rule 4 dispatch: weekly start is announced, but the pause outliving the
 * window replaces ENDING_SOON/ENDED with NON_RECURRING_UNPAUSED. */
void test_dispatch_rule4_weekly_start_abs_end(void)
{
    const char *macs[] = { "11:11:11:11:11:11" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    time_t weekly_start = DAY_BASE + 36000;
    time_t abs_end      = DAY_BASE + 41400;

    s = build_schedule("UTC", 1, macs);
    add_weekly_event(s, 36000, block0, 1);
    add_weekly_event(s, 39600, NULL, 0);
    add_absolute_event(s, weekly_start + 600, block0, 1);
    add_absolute_event(s, abs_end, NULL, 0);
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, weekly_start - 2000, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);

    reset_sent_counts();
    run_full_dispatch(collection, s, weekly_start, abs_end);

    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_STARTING_SOON], 1);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_STARTED], 1);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_ENDING_SOON], 0);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_ENDED], 0);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_NON_RECURRING_UNPAUSED], 1);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Two MACs on different weekly windows must each get their own batch, so the
 * dispatcher flushes the pending batch when the scheduled time changes. */
void test_dispatch_batches_flush_on_time_change(void)
{
    const char *macs[] = { "11:11:11:11:11:11", "22:22:22:22:22:22" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    uint32_t block01[] = { 0, 1 };
    time_t mac0_start = DAY_BASE + 36000;
    time_t mac1_start = DAY_BASE + 37800;
    time_t both_end   = DAY_BASE + 41400;

    s = build_schedule("UTC", 2, macs);
    add_weekly_event(s, 36000, block0, 1);    /* MAC0 starts */
    add_weekly_event(s, 37800, block01, 2);   /* MAC1 joins later */
    add_weekly_event(s, 41400, NULL, 0);      /* both end together */
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, mac0_start - 2000, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);

    reset_sent_counts();
    send_pending_notifications_with_state_check(collection, s, mac0_start - NOTIFICATION_ADVANCE_TIME_SEC);
    send_pending_notifications_with_state_check(collection, s, mac0_start);
    send_pending_notifications_with_state_check(collection, s, mac1_start - NOTIFICATION_ADVANCE_TIME_SEC);
    send_pending_notifications_with_state_check(collection, s, mac1_start);
    send_pending_notifications_with_state_check(collection, s, both_end - NOTIFICATION_ADVANCE_TIME_SEC);
    send_pending_notifications_with_state_check(collection, s, both_end);

    CU_ASSERT_TRUE(g_notification_sent_count[NOTIFY_DOWNTIME_STARTING_SOON] >= 2);
    CU_ASSERT_TRUE(g_notification_sent_count[NOTIFY_DOWNTIME_STARTED] >= 2);
    CU_ASSERT_TRUE(g_notification_sent_count[NOTIFY_DOWNTIME_ENDED] >= 1);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Arriving far outside the +/-5s window must suppress the notification while
 * still marking it handled, so it is never retried. */
void test_dispatch_late_arrival_is_suppressed(void)
{
    const char *macs[] = { "11:11:11:11:11:11" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    mac_notification_state_t *state;
    time_t weekly_start = DAY_BASE + 36000;
    time_t weekly_end   = DAY_BASE + 39600;

    s = build_schedule("UTC", 1, macs);
    add_weekly_event(s, 36000, block0, 1);
    add_weekly_event(s, 39600, NULL, 0);
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, weekly_start - 2000, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    state = &collection->timelines[0].periods->mac_states[0];

    reset_sent_counts();
    /* 200s past each window - far beyond NOTIFICATION_LATE_THRESHOLD_SEC */
    send_pending_notifications_with_state_check(collection, s,
        weekly_start - NOTIFICATION_ADVANCE_TIME_SEC + 200);
    send_pending_notifications_with_state_check(collection, s, weekly_start + 200);
    send_pending_notifications_with_state_check(collection, s,
        weekly_end - NOTIFICATION_ADVANCE_TIME_SEC + 200);
    send_pending_notifications_with_state_check(collection, s, weekly_end + 200);

    CU_ASSERT_TRUE(state->starting_soon_sent);
    CU_ASSERT_TRUE(state->started_sent);
    CU_ASSERT_TRUE(state->ending_soon_sent);
    /* A period already in the past is skipped wholesale before the ENDED
     * check, so ENDED stays unmarked rather than being flagged late. */
    CU_ASSERT_FALSE(state->ended_sent);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_STARTING_SOON], 0);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_STARTED], 0);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_ENDING_SOON], 0);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_ENDED], 0);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* A window shorter than the 15-minute advance window must skip the "soon"
 * notifications entirely rather than firing them at a nonsensical time. */
void test_dispatch_short_period_skips_soon(void)
{
    const char *macs[] = { "11:11:11:11:11:11" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    mac_notification_state_t *state;
    time_t weekly_start = DAY_BASE + 36000;
    time_t weekly_end   = DAY_BASE + 36300;  /* 5 minutes only */

    s = build_schedule("UTC", 1, macs);
    add_weekly_event(s, 36000, block0, 1);
    add_weekly_event(s, 36300, NULL, 0);
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, weekly_start - 2000, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    state = &collection->timelines[0].periods->mac_states[0];

    reset_sent_counts();
    run_full_dispatch(collection, s, weekly_start, weekly_end);

    CU_ASSERT_FALSE(state->starting_soon_sent);
    CU_ASSERT_FALSE(state->ending_soon_sent);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_STARTING_SOON], 0);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_ENDING_SOON], 0);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_ENDED], 1);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Guard clauses must not crash or dispatch anything. */
void test_dispatch_null_inputs(void)
{
    const char *macs[] = { "11:11:11:11:11:11" };
    schedule_t *s = build_schedule("UTC", 1, macs);

    reset_sent_counts();
    send_pending_notifications_with_state_check(NULL, s, DAY_BASE);
    send_pending_notifications_with_state_check(NULL, NULL, DAY_BASE);
    CU_ASSERT_EQUAL(g_notification_sent_count[NOTIFY_DOWNTIME_STARTED], 0);

    destroy_schedule(s);
}

/* The absolute-schedule summary log only formats windows that end after the
 * real wall clock, so this anchors to time(NULL) instead of DAY_BASE. Two MACs
 * share one window (exercising the grouped, comma-separated MAC list) and a
 * third has its own, producing two distinct patterns. */
void test_log_summary_groups_absolute_macs(void)
{
    const char *macs[] = { "11:11:11:11:11:11", "22:22:22:22:22:22", "33:33:33:33:33:33" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block01[] = { 0, 1 };
    uint32_t block2[]  = { 2 };
    time_t base = time(NULL) + 3600;

    s = build_schedule("UTC", 3, macs);
    add_absolute_event(s, base, block01, 2);
    add_absolute_event(s, base + 3600, block2, 1);
    add_absolute_event(s, base + 7200, NULL, 0);
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, time(NULL), MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);

    /* MAC0 and MAC1 share the first window. */
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->start_time, base);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->end_time, base + 3600);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[1].periods);
    CU_ASSERT_EQUAL(collection->timelines[1].periods->start_time, base);
    CU_ASSERT_EQUAL(collection->timelines[1].periods->end_time, base + 3600);

    /* MAC2 has its own, later window. */
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[2].periods);
    CU_ASSERT_EQUAL(collection->timelines[2].periods->start_time, base + 3600);
    CU_ASSERT_EQUAL(collection->timelines[2].periods->end_time, base + 7200);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Regression guard for the bounded MAC-list formatting (CWE-787): enough MACs
 * in one absolute window to overflow the 256-byte buffer must truncate safely
 * rather than run off the end. Run under valgrind to make this meaningful. */
void test_log_summary_truncates_long_mac_list(void)
{
    enum { MANY = 40 };
    char mac_buf[MANY][18];
    const char *macs[MANY];
    uint32_t block_all[MANY];
    schedule_t *s;
    mac_timeline_collection_t *collection;
    time_t base = time(NULL) + 3600;
    size_t i;

    for (i = 0; i < MANY; i++) {
        snprintf(mac_buf[i], sizeof(mac_buf[i]), "aa:bb:cc:dd:ee:%02x", (unsigned) i);
        macs[i] = mac_buf[i];
        block_all[i] = (uint32_t) i;
    }

    s = build_schedule("UTC", MANY, macs);
    add_absolute_event(s, base, block_all, MANY);
    add_absolute_event(s, base + 3600, NULL, 0);
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, time(NULL), MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);
    for (i = 0; i < MANY; i++) {
        CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[i].periods);
        CU_ASSERT_EQUAL(collection->timelines[i].periods->start_time, base);
        CU_ASSERT_EQUAL(collection->timelines[i].periods->end_time, base + 3600);
    }

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Field case: a user pauses MAC1 only, and the cloud folds the overlapping
 * weekly window into the absolute array by adding MAC0 at the weekly start.
 * MAC0's entry ties the weekly start, so it must be attributed to WEEKLY and
 * must not be reported as an absolute pause in the summary log. */
void test_cloud_adjusted_absolute_mac_not_user_pause(void)
{
    const char *macs[] = { "40:d1:60:41:5d:14", "aa:1f:3d:47:0b:30" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block1[]  = { 1 };
    uint32_t block01[] = { 0, 1 };
    time_t now_ref = time(NULL) + 600;
    time_t weekly_start, pause_start, window_end;
    time_t week_start;

    /* Sunday 00:00 UTC of the current week. Jan 1 1970 was a Thursday, hence
     * the 4-day shift. Keeping this in UTC (and using a UTC schedule) makes
     * the weekly offsets independent of the build machine's timezone. */
    week_start = ((now_ref + 4 * 86400) / SECONDS_IN_A_WEEK) * SECONDS_IN_A_WEEK - 4 * 86400;

    weekly_start = now_ref + 1020;          /* weekly window opens shortly */
    pause_start  = now_ref;                 /* user pause starts earlier */
    window_end   = weekly_start + 2040;

    s = build_schedule("UTC", 2, macs);
    add_weekly_event(s, weekly_start - week_start, block01, 2);
    add_weekly_event(s, window_end - week_start, NULL, 0);

    add_absolute_event(s, pause_start, block1, 1);    /* genuine pause, MAC1 */
    add_absolute_event(s, weekly_start, block01, 2);  /* cloud folds MAC0 in */
    add_absolute_event(s, window_end, NULL, 0);
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, now_ref - 60, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);

    /* MAC0 joined only because of the weekly window -> WEEKLY start. */
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->start_time, weekly_start);
    CU_ASSERT_FALSE(collection->timelines[0].periods->start_is_absolute);

    /* MAC1 is the real pause -> ABSOLUTE start. */
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[1].periods);
    CU_ASSERT_EQUAL(collection->timelines[1].periods->start_time, pause_start);
    CU_ASSERT_TRUE(collection->timelines[1].periods->start_is_absolute);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Field case: two genuine user pauses triggered 8 seconds apart, both fully
 * enclosing a later weekly window. Neither start ties a weekly event, so both
 * must stay ABSOLUTE/ABSOLUTE and both must survive the summary-log filter
 * that hides cloud-folded entries. */
void test_two_staggered_user_pauses_enclosing_weekly(void)
{
    const char *macs[] = { "40:d1:60:41:5d:14", "aa:1f:3d:47:0b:30" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block1[]  = { 1 };
    uint32_t block0[]  = { 0 };
    uint32_t block01[] = { 0, 1 };
    time_t now_ref = time(NULL) + 600;
    time_t week_start;
    time_t mac1_start, mac0_start, weekly_start, weekly_end, mac1_end, mac0_end;

    week_start = ((now_ref + 4 * 86400) / SECONDS_IN_A_WEEK) * SECONDS_IN_A_WEEK - 4 * 86400;

    mac1_start   = now_ref;             /* first pause */
    mac0_start   = now_ref + 8;         /* second pause, 8s later */
    weekly_start = now_ref + 1548;      /* weekly opens inside both pauses */
    weekly_end   = weekly_start + 960;  /* 16-minute window */
    mac1_end     = now_ref + 3600;
    mac0_end     = now_ref + 3608;

    s = build_schedule("UTC", 2, macs);
    add_weekly_event(s, weekly_start - week_start, block01, 2);
    add_weekly_event(s, weekly_end - week_start, NULL, 0);

    add_absolute_event(s, mac1_start, block1, 1);
    add_absolute_event(s, mac0_start, block01, 2);
    add_absolute_event(s, mac1_end, block0, 1);   /* MAC1 released, MAC0 stays */
    add_absolute_event(s, mac0_end, NULL, 0);
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, now_ref - 60, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);

    /* MAC0: own pause, weekly swallowed -> ABSOLUTE/ABSOLUTE. */
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->start_time, mac0_start);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->end_time, mac0_end);
    CU_ASSERT_TRUE(collection->timelines[0].periods->start_is_absolute);
    CU_ASSERT_TRUE(collection->timelines[0].periods->end_is_absolute);

    /* MAC1: its own, 8-second-offset pause, independently resolved. */
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[1].periods);
    CU_ASSERT_EQUAL(collection->timelines[1].periods->start_time, mac1_start);
    CU_ASSERT_EQUAL(collection->timelines[1].periods->end_time, mac1_end);
    CU_ASSERT_TRUE(collection->timelines[1].periods->start_is_absolute);
    CU_ASSERT_TRUE(collection->timelines[1].periods->end_is_absolute);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Rule 6 support: repeated rebuilds from the same schedule must stay stable
 * and self-consistent (run under valgrind in CI to catch leaks). */
void test_rule6_repeated_rebuild_is_stable(void)
{
    const char *macs[] = { "11:11:11:11:11:11", "22:22:22:22:22:22" };
    schedule_t *s;
    uint32_t block0[] = { 0 };
    uint32_t block01[] = { 0, 1 };
    time_t weekly_start = DAY_BASE + 36000;
    time_t abs_start = weekly_start - 1200;
    time_t abs_end   = weekly_start + 600;
    int i;

    s = build_schedule("UTC", 2, macs);
    add_weekly_event(s, 36000, block01, 2);
    add_weekly_event(s, 39600, NULL, 0);
    add_absolute_event(s, abs_start, block0, 1);
    add_absolute_event(s, abs_end, NULL, 0);

    for (i = 0; i < 12; i++) {
        /* Advance a simulated day each pass, mirroring the scheduler's
         * 10-day staleness rebuild without waiting real time. */
        time_t now = abs_start + ((time_t) i * 86400);
        mac_timeline_collection_t *collection =
            build_timeline_from_schedule(s, now, MAX_WEEKS_AHEAD);

        CU_ASSERT_PTR_NOT_NULL_FATAL(collection);
        CU_ASSERT_EQUAL(collection->mac_count, 2);
        CU_ASSERT_PTR_NOT_NULL(collection->timelines);
        /* Weekly recurrence must still be projected on every rebuild. */
        CU_ASSERT_PTR_NOT_NULL(collection->timelines[1].periods);

        destroy_timeline_collection(collection);
    }

    destroy_schedule(s);
}

/* Case 1 (Akerlogs "Overlap Scenario"): absolute block end TIME TIES the
 * weekly START, with no further absolute event near the weekly end. MAC0
 * must transition seamlessly from ABSOLUTE start into a natural WEEKLY end. */
void test_absolute_end_ties_weekly_start_case1(void)
{
    const char *macs[] = { "11:11:11:11:11:11", "22:22:22:22:22:22" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    uint32_t block01[] = { 0, 1 };
    time_t weekly_start = DAY_BASE + 36000;
    time_t weekly_end   = DAY_BASE + 39600;
    time_t abs_start    = DAY_BASE + 34500;

    s = build_schedule("UTC", 2, macs);
    add_weekly_event(s, 36000, block01, 2);
    add_weekly_event(s, 39600, NULL, 0);

    add_absolute_event(s, abs_start, block0, 1);        /* MAC0 only, before weekly */
    add_absolute_event(s, weekly_start, block01, 2);    /* ties weekly start; no later absolute event */
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, abs_start + 1, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);

    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->start_time, abs_start);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->end_time, weekly_end);
    CU_ASSERT_TRUE(collection->timelines[0].periods->start_is_absolute);
    CU_ASSERT_FALSE(collection->timelines[0].periods->end_is_absolute);

    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[1].periods);
    CU_ASSERT_EQUAL(collection->timelines[1].periods->start_time, weekly_start);
    CU_ASSERT_EQUAL(collection->timelines[1].periods->end_time, weekly_end);
    CU_ASSERT_FALSE(collection->timelines[1].periods->start_is_absolute);
    CU_ASSERT_FALSE(collection->timelines[1].periods->end_is_absolute);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Case 2 (Akerlogs "Overlap Scenario"): absolute end ties the weekly END, but
 * on a weekday that is NOT the schedule's last (highest-time) entry, so the
 * finalize_schedule() wrap-around duplicate lands on a different day and
 * does not interfere here. Confirms the "clean", non-duplicate tie path
 * still resolves correctly (no regression from the wrap-around fix). */
void test_absolute_end_ties_weekly_end_case2(void)
{
    const char *macs[] = { "11:11:11:11:11:11", "22:22:22:22:22:22" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    uint32_t block01[] = { 0, 1 };
    time_t mon_start = DAY_BASE + 90000;   /* Monday - not the schedule's last entry */
    time_t mon_end   = DAY_BASE + 93600;
    time_t tue_start = DAY_BASE + 176400;  /* Tuesday - the last entry */
    time_t tue_end   = DAY_BASE + 180000;
    time_t abs_start = DAY_BASE + 89000;
    time_t abs_true_end = DAY_BASE + 94200;

    s = build_schedule("UTC", 2, macs);
    add_weekly_event(s, 90000, block01, 2);
    add_weekly_event(s, 93600, NULL, 0);
    add_weekly_event(s, 176400, block01, 2);
    add_weekly_event(s, 180000, NULL, 0);

    add_absolute_event(s, abs_start, block0, 1);       /* MAC0 only, before Monday's weekly start */
    add_absolute_event(s, mon_start, block01, 2);      /* ties Monday's weekly start */
    add_absolute_event(s, mon_end, block0, 1);         /* ties Monday's weekly end */
    add_absolute_event(s, abs_true_end, NULL, 0);       /* true absolute end, after Monday's weekly end */
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, abs_start + 1, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);

    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->start_time, abs_start);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->end_time, abs_true_end);
    CU_ASSERT_TRUE(collection->timelines[0].periods->start_is_absolute);
    CU_ASSERT_TRUE(collection->timelines[0].periods->end_is_absolute);

    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[1].periods);
    CU_ASSERT_EQUAL(collection->timelines[1].periods->start_time, mon_start);
    CU_ASSERT_EQUAL(collection->timelines[1].periods->end_time, mon_end);
    CU_ASSERT_FALSE(collection->timelines[1].periods->start_is_absolute);
    CU_ASSERT_FALSE(collection->timelines[1].periods->end_is_absolute);

    /* Tuesday's pair (the schedule's last entry) is untouched by the absolute
     * schedule and must still produce a normal recurring period for both MACs. */
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods->next);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->next->start_time, tue_start);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->next->end_time, tue_end);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Case 4 (Akerlogs): a brand-new MAC (index 0) gets a solo absolute pause
 * that ties an existing weekly schedule's start time for two OTHER MACs
 * (indexes 1,2). MAC0 has no weekly membership at all, so its absolute
 * period must resolve as a pure ABSOLUTE/ABSOLUTE pause, while MAC1/MAC2's
 * absolute entries at the same tie point must resolve as WEEKLY (backend-
 * generated), since a weekly event exists at that exact timestamp. */
void test_case4_absolute_out_of_profile_ties_weekly_start(void)
{
    const char *macs[] = { "11:11:11:11:11:11", "22:22:22:22:22:22", "33:33:33:33:33:33" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block0[] = { 0 };
    uint32_t block12[] = { 1, 2 };
    uint32_t block012[] = { 0, 1, 2 };
    time_t weekly_start = DAY_BASE + 36000;
    time_t weekly_end   = DAY_BASE + 39600;
    time_t abs1 = weekly_start - 900;  /* MAC0 solo pause starts */
    time_t abs3 = weekly_start + 900;  /* MAC0 solo pause naturally expires */

    s = build_schedule("UTC", 3, macs);
    add_weekly_event(s, 36000, block12, 2);
    add_weekly_event(s, 39600, NULL, 0);

    add_absolute_event(s, abs1, block0, 1);           /* MAC0 only, before weekly */
    add_absolute_event(s, weekly_start, block012, 3); /* ties weekly start for MAC1/MAC2 */
    add_absolute_event(s, abs3, block12, 2);          /* MAC0 removed, MAC1/MAC2 remain */
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, abs1 + 1, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);

    /* MAC0: pure absolute pause, unrelated to the weekly schedule. */
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->start_time, abs1);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->end_time, abs3);
    CU_ASSERT_TRUE(collection->timelines[0].periods->start_is_absolute);
    CU_ASSERT_TRUE(collection->timelines[0].periods->end_is_absolute);

    /* MAC1 and MAC2: backend-generated absolute entry ties the weekly start,
     * so it must be attributed as WEEKLY, ending at the weekly's natural end. */
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[1].periods);
    CU_ASSERT_EQUAL(collection->timelines[1].periods->start_time, weekly_start);
    CU_ASSERT_EQUAL(collection->timelines[1].periods->end_time, weekly_end);
    CU_ASSERT_FALSE(collection->timelines[1].periods->start_is_absolute);
    CU_ASSERT_FALSE(collection->timelines[1].periods->end_is_absolute);

    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[2].periods);
    CU_ASSERT_EQUAL(collection->timelines[2].periods->start_time, weekly_start);
    CU_ASSERT_EQUAL(collection->timelines[2].periods->end_time, weekly_end);
    CU_ASSERT_FALSE(collection->timelines[2].periods->start_is_absolute);
    CU_ASSERT_FALSE(collection->timelines[2].periods->end_is_absolute);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

/* Case 5 (Akerlogs): two overlapping weekly windows for different MAC
 * subsets (MAC0 alone starts/ends slightly later than MAC1/MAC2), plus a
 * 4th MAC (index 3) with its own solo absolute pause that ties BOTH weekly
 * start times and naturally expires mid-way through both windows. Confirms
 * each MAC resolves independently: MAC0/MAC1/MAC2 stay WEEKLY-attributed,
 * MAC3 resolves as a pure ABSOLUTE/ABSOLUTE pause. */
void test_case5_two_overlapping_weekly_windows_with_absolute_mac(void)
{
    const char *macs[] = { "11:11:11:11:11:11", "22:22:22:22:22:22", "33:33:33:33:33:33", "44:44:44:44:44:44" };
    schedule_t *s;
    mac_timeline_collection_t *collection;
    uint32_t block3[] = { 3 };
    uint32_t block123[] = { 1, 2, 3 };
    uint32_t block0123[] = { 0, 1, 2, 3 };
    uint32_t block012[] = { 0, 1, 2 };
    uint32_t block0[] = { 0 };
    uint32_t block12[] = { 1, 2 };
    time_t mac12_start = DAY_BASE + 36000;
    time_t mac12_end   = DAY_BASE + 38520;  /* MAC1/MAC2 removed here; MAC0 stays blocked */
    time_t mac0_start   = DAY_BASE + 36300;
    time_t mac0_end     = DAY_BASE + 38880;  /* MAC0's own, later weekly end */
    time_t abs1 = mac12_start - 900;         /* MAC3 solo pause starts */
    time_t abs4 = abs1 + 1800;               /* MAC3 solo pause naturally expires */

    s = build_schedule("UTC", 4, macs);
    add_weekly_event(s, 36000, block12, 2);     /* MAC1, MAC2 weekly start */
    add_weekly_event(s, 36300, block012, 3);    /* MAC0 added */
    add_weekly_event(s, 38520, block0, 1);      /* MAC1, MAC2 implicitly removed; MAC0 stays */
    add_weekly_event(s, 38880, NULL, 0);        /* MAC0's own weekly end */

    add_absolute_event(s, abs1, block3, 1);              /* MAC3 only */
    add_absolute_event(s, mac12_start, block123, 3);     /* ties MAC1/MAC2 weekly start */
    add_absolute_event(s, mac0_start, block0123, 4);     /* ties MAC0 weekly start */
    add_absolute_event(s, abs4, block012, 3);            /* MAC3 removed, others remain */
    CU_ASSERT_TRUE(finalize_schedule(s) <= 0);

    collection = build_timeline_from_schedule(s, abs1 + 1, MAX_WEEKS_AHEAD);
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection);

    /* MAC0: own weekly window, tied to absolute start but ending at its own,
     * later natural weekly end (not MAC1/MAC2's earlier end). */
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[0].periods);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->start_time, mac0_start);
    CU_ASSERT_EQUAL(collection->timelines[0].periods->end_time, mac0_end);
    CU_ASSERT_FALSE(collection->timelines[0].periods->start_is_absolute);
    CU_ASSERT_FALSE(collection->timelines[0].periods->end_is_absolute);

    /* MAC1 and MAC2: their own, earlier weekly window. */
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[1].periods);
    CU_ASSERT_EQUAL(collection->timelines[1].periods->start_time, mac12_start);
    CU_ASSERT_EQUAL(collection->timelines[1].periods->end_time, mac12_end);
    CU_ASSERT_FALSE(collection->timelines[1].periods->start_is_absolute);
    CU_ASSERT_FALSE(collection->timelines[1].periods->end_is_absolute);

    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[2].periods);
    CU_ASSERT_EQUAL(collection->timelines[2].periods->start_time, mac12_start);
    CU_ASSERT_EQUAL(collection->timelines[2].periods->end_time, mac12_end);

    /* MAC3: pure absolute pause, unrelated to either weekly window. */
    CU_ASSERT_PTR_NOT_NULL_FATAL(collection->timelines[3].periods);
    CU_ASSERT_EQUAL(collection->timelines[3].periods->start_time, abs1);
    CU_ASSERT_EQUAL(collection->timelines[3].periods->end_time, abs4);
    CU_ASSERT_TRUE(collection->timelines[3].periods->start_is_absolute);
    CU_ASSERT_TRUE(collection->timelines[3].periods->end_is_absolute);

    destroy_timeline_collection(collection);
    destroy_schedule(s);
}

void add_suites( CU_pSuite *suite )
{
    printf("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test format_iso8601_utc", test_format_iso8601_utc );
    CU_add_test( *suite, "Test calculate_utc_offset", test_calculate_utc_offset );
    CU_add_test( *suite, "Test is_mac_indefinitely_blocked", test_is_mac_indefinitely_blocked );
    CU_add_test( *suite, "Test absolute/weekly tie wrap-around regression", test_absolute_weekly_tie_regression );
    CU_add_test( *suite, "Test notification state progression", test_notification_state_progression );
    CU_add_test( *suite, "Test indefinite block skips timeline", test_indefinite_block_via_timeline );
    CU_add_test( *suite, "Test infinite block for one MAC, normal for another", test_infinite_block_mixed_with_normal_mac );
    CU_add_test( *suite, "Test pure absolute pause with no weekly schedule", test_pure_absolute_pause_no_weekly );
    CU_add_test( *suite, "Test Case 1: absolute end ties weekly start", test_absolute_end_ties_weekly_start_case1 );
    CU_add_test( *suite, "Test Case 2: absolute end ties weekly end (non-wraparound day)", test_absolute_end_ties_weekly_end_case2 );
    CU_add_test( *suite, "Test Case 4: out-of-profile absolute ties weekly start", test_case4_absolute_out_of_profile_ties_weekly_start );
    CU_add_test( *suite, "Test Case 5: two overlapping weekly windows with absolute MAC", test_case5_two_overlapping_weekly_windows_with_absolute_mac );
    CU_add_test( *suite, "Test is_device_blocked_at", test_is_device_blocked_at );
    CU_add_test( *suite, "Test absolute pause full notification flow", test_absolute_pause_full_notification_flow );
    CU_add_test( *suite, "Test aker_notification_init/cleanup", test_aker_notification_init_and_cleanup );
    CU_add_test( *suite, "Test get_next_notification_time", test_get_next_notification_time );
    CU_add_test( *suite, "Test get_next_notification_time absolute/short period", test_get_next_notification_time_absolute_and_short_period );
    CU_add_test( *suite, "Test timeline recalculation on schedule change", test_timeline_recalculation_on_schedule_change );
    CU_add_test( *suite, "Test absolute fully encloses weekly with no tie events (RDKB-65401)", test_absolute_fully_encloses_weekly_no_tie_events );
    CU_add_test( *suite, "Test absolute end exactly ties weekly end stays absolute", test_absolute_end_exactly_ties_weekly_end_stays_absolute );
    CU_add_test( *suite, "Rule 1: ABS start + ABS end (single MAC)", test_rule1_abs_abs_single_mac );
    CU_add_test( *suite, "Rule 2: ABS start + WEEKLY end (single MAC)", test_rule2_abs_start_weekly_end_single_mac );
    CU_add_test( *suite, "Rule 3: WEEKLY start + WEEKLY end (two MACs)", test_rule3_weekly_weekly_two_macs );
    CU_add_test( *suite, "Rule 4: WEEKLY start + ABS end (single MAC)", test_rule4_weekly_start_abs_end_single_mac );
    CU_add_test( *suite, "Rule 5: ABS end ties weekly end after weekly start", test_rule5_abs_end_ties_weekly_end_after_weekly_start );
    CU_add_test( *suite, "Rule 5: does not leak to weekly-only MAC", test_rule5_does_not_leak_to_weekly_only_mac );
    CU_add_test( *suite, "Rules: mixed two profiles + absolute MAC", test_rules_mixed_two_profiles_and_absolute_mac );
    CU_add_test( *suite, "ABS expires while weekly active (two MACs)", test_abs_expires_while_weekly_active_two_macs );
    CU_add_test( *suite, "ABS end 1s before weekly end -> WEEKLY end", test_abs_end_one_second_before_weekly_end );
    CU_add_test( *suite, "ABS end 1s after weekly end -> ABSOLUTE end", test_abs_end_one_second_after_weekly_end );
    CU_add_test( *suite, "ABS on MAC0 only, weekly on both MACs", test_abs_mac0_only_weekly_both_macs );
    CU_add_test( *suite, "ABS 21-23 encloses weekly 21:30-22 (both MACs)", test_absolute_encloses_weekly_both_macs );
    CU_add_test( *suite, "ABS 21-23 encloses weekly, MAC0 paused only", test_absolute_encloses_weekly_abs_mac0_only );
    CU_add_test( *suite, "Dispatch R3: weekly sends all four", test_dispatch_rule3_weekly_sends_all_four );
    CU_add_test( *suite, "Dispatch R1: absolute only sends unpause", test_dispatch_rule1_absolute_only_unpause );
    CU_add_test( *suite, "Dispatch R2: ABS start + WEEKLY end", test_dispatch_rule2_abs_start_weekly_end );
    CU_add_test( *suite, "Dispatch R4: WEEKLY start + ABS end", test_dispatch_rule4_weekly_start_abs_end );
    CU_add_test( *suite, "Dispatch: batch flush on time change", test_dispatch_batches_flush_on_time_change );
    CU_add_test( *suite, "Dispatch: late arrival suppressed", test_dispatch_late_arrival_is_suppressed );
    CU_add_test( *suite, "Dispatch: short period skips soon", test_dispatch_short_period_skips_soon );
    CU_add_test( *suite, "Dispatch: NULL inputs", test_dispatch_null_inputs );
    CU_add_test( *suite, "Log summary groups absolute MACs", test_log_summary_groups_absolute_macs );
    CU_add_test( *suite, "Log summary truncates long MAC list", test_log_summary_truncates_long_mac_list );
    CU_add_test( *suite, "Cloud-adjusted absolute MAC is not a user pause", test_cloud_adjusted_absolute_mac_not_user_pause );
    CU_add_test( *suite, "Two staggered user pauses enclosing weekly", test_two_staggered_user_pauses_enclosing_weekly );
    CU_add_test( *suite, "Rule 6: repeated rebuild is stable", test_rule6_repeated_rebuild_is_stable );
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

time_t convert_unix_time_to_weekly( time_t unixtime )
{
    time_t seconds_since_sunday_midnight;
    time_t t = unixtime;
    struct tm ts;

    ts = *localtime(&t);

    seconds_since_sunday_midnight = (ts.tm_wday * 24 * 3600) +
            (ts.tm_hour * 3600) +
            (ts.tm_min * 60) +
            ts.tm_sec;

    return seconds_since_sunday_midnight;
}

time_t get_unix_time(void)
{
    struct timespec tm;
    time_t unix_time = 0;

    clock_gettime(CLOCK_REALTIME, &tm);
    unix_time = tm.tv_sec;

    return unix_time;
}

int32_t get_max_mac_limit(void)
{
    return 2048;
}
