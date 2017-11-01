/**
 * Copyright 2017 Comcast Cable Communications Management, LLC
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
#include <stdint.h>
#include "time.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

/* See time.h for details. */
time_t convert_unix_time_to_weekly(time_t unixtime)
{
    time_t seconds_since_sunday_midnght;
    struct tm ts;

    ts = *localtime(&unixtime);

    seconds_since_sunday_midnght = (ts.tm_wday * 24 * 3600) +
            (ts.tm_hour * 3600) +
            (ts.tm_min * 60) +
            ts.tm_sec;


    return seconds_since_sunday_midnght;
}

/* See time.h for details. */
time_t convert_weekly_time_to_unix(time_t weekly_time)
{
    struct timespec tm;
    time_t current_unix_time = 0, current_weekly_time = 0, unix_time = 0;

    if( 0 == clock_gettime(CLOCK_REALTIME, &tm) ) {
        current_unix_time = tm.tv_sec;

        /* Find current time in seconds since past Saturday 23:59:59 + one second */
        current_weekly_time = convert_unix_time_to_weekly(current_unix_time);

        /* Deduct current weekly time to determine time at Saturday 23:59:59 + one second since Epoch,
         * add weekly time to convert weekly time to seconds since Epoch.
         */
        unix_time = current_unix_time - current_weekly_time + weekly_time;
    }

    return unix_time;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
