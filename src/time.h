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
#ifndef __TIME_H__
#define __TIME_H__
#include <time.h>

#define SECONDS_IN_A_WEEK   (24 * 7 * 60 * 60)

/**
 *  Does the conversion from unixtime to the weekly time, dealing with
 *  things like DST changes, etc.
 *
 *  @note The relative/weekly time is seconds since Sunday midnight, i.e. 
 *  Saturday 23:59:59 + one second
 *
 *  @param unixtime the absolute time to convert from
 *
 *  @return the relative time to convert to
 */
time_t convert_unix_time_to_weekly( time_t unixtime );

#endif
