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
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>

bool malloc_fail = 0;
size_t malloc_failure_limit = UINT_MAX;

void *aker_malloc(size_t size)
{
    if (malloc_fail && (size >= malloc_failure_limit)) {
        return NULL;
    } else {
        return malloc(size);
    }
}

void aker_free   (void *ptr)
{
    free(ptr);
}
