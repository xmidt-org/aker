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
#ifndef __DECODE_H__
#define __DECODE_H__

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 *  Decodes the MsgPacked structure (bytes) into a new schedule object.
 *
 *  @param len  [in]  the number of bytes to process
 *  @param data [in]  the msgpack bytes to process
 *  @param s    [out] the resulting schedule struture, or untouched on error
 *
 *  @return 0 on success, error otherwise.
 */
int decode_schedule(size_t count, uint8_t *bytes, schedule_t **s);


#endif
