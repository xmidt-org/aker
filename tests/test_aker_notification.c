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
