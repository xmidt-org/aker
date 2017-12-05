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
#include <stddef.h>

#include "aker_mem.h"
#include "aker_msgpack.h"

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

/* See aker_msgpack.h for details. */
void pack_msgpack_string( msgpack_packer *pk, const void *string, size_t size )
{
    msgpack_pack_str( pk, size );
    msgpack_pack_str_body( pk, string, size );
}

/* See aker_msgpack.h for details. */
size_t pack_status_msgpack_map(const char *string, void **binary)
{
    const char cstr_message[] = "message";
    size_t binary_size = 0;
    msgpack_sbuffer sbuf;
    msgpack_packer pk;

    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
    msgpack_pack_map(&pk, 2);

    pack_msgpack_string(&pk, cstr_message, strlen(cstr_message));
    pack_msgpack_string(&pk, string, strlen(string));

    if( NULL != sbuf.data ) {
        *binary = aker_malloc(sizeof(char) * sbuf.size);
        if( NULL != *binary ) {
            memcpy(*binary, sbuf.data, sbuf.size);
            binary_size = sbuf.size;
        }
    }
    msgpack_sbuffer_destroy(&sbuf);

    return binary_size;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
