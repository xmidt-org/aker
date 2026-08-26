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
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

// can't include time.c 
int set_unix_time_zone (const char *time_zone)
{
   int rv = 0;

   setenv("TZ", time_zone, 1);
   tzset();

   return rv;
}

// Stub for T2 telemetry function (used in aker_notification.c and aker_metrics.c)
void t2_event_s(const char *marker, const char *value)
{
    // Stub implementation for tests - just log to console
    printf("[T2_STUB] Marker: %s, Value: %s\n", marker ? marker : "NULL", value ? value : "NULL");
}
