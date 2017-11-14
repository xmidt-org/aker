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
#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

/**
 *  Starts the scheduler thread
 *
 *  @param thread       if not NULL the thread id is returned here, ignored otherwise
 *  @param firewall_cmd the firewall command to execute via system()
 *
 *  @return the result of thread creation
 */
int scheduler_start( pthread_t *thread, const char *firewall_cmd );

/**
 *  Sends in data to make a new schedule and replace any existing ones.
 *
 *  @param len  the length of the data in bytes
 *  @param data the schedule msgpack data
 *
 *  @return 0 on success, error from decoding the data otherwise
 */
int process_schedule_data( size_t len, uint8_t *data );

/**
 *  Retreives data generated the last time the scheduler was run.
 *
 *  @note Makes new copy of string and returns, caller to free returned string
 *
 *  @return the string with the list of blocked addresses (may be NULL and valid)
 */
char *get_current_blocked_macs( void );

#endif
