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
#ifndef __AKER_MSGPACK_H__
#define __AKER_MSGPACK_H__

#include <stddef.h>

/**
 *  Packs string into msgpack 
 *
 *  @param string [in]  string to be packed
 *  @param binary [out] string size
 */
size_t pack_status_msg(const char *string, void **binary);

/**
 *  Packs the actively blocked devices and the time into msgpack payload.
 *
 *  @param active [in]  the list of blocked devices
 *  @param time   [in]  the time value
 *  @param binary [out] the pointer to assign the allocated byte array
 *
 *  @returns the length of the allocated byte array
 */
size_t pack_now_msg( const char *active, time_t time, void **binary );
#endif
