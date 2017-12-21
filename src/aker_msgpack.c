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
#include <time.h>
#include <msgpack.h>

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
void pack_msgpack_string( msgpack_packer *pk, const void *string, size_t size );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

/* See aker_msgpack.h for details. */
size_t pack_status_msg(const char *string, void **binary)
{
    const char cstr_message[] = "message";
    size_t binary_size = 0;
    msgpack_sbuffer sbuf;
    msgpack_packer pk;

    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
    msgpack_pack_map(&pk, 1);

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

/* See aker_msgpack.h for details. */
size_t pack_now_msg( const char *active, time_t time, void **binary )
{
    const char cstr_active[] = "active";
    const char cstr_time[] = "time";
    size_t len;
    size_t active_len = 0;
    msgpack_sbuffer sbuf;
    msgpack_packer pk;

    if( active ) {
        active_len = strlen(active);
    }

    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
    msgpack_pack_map(&pk, 2);

    pack_msgpack_string(&pk, cstr_active, strlen(cstr_active));
    pack_msgpack_string(&pk, active, active_len);

    pack_msgpack_string(&pk, cstr_time, strlen(cstr_time));
    msgpack_pack_int32(&pk, time);

    len = 0;
    if( sbuf.data ) {
        *binary = aker_malloc(sizeof(char) * sbuf.size);
        if( NULL != *binary ) {
            memcpy(*binary, sbuf.data, sbuf.size);
            len = sbuf.size;
        }
    }
    msgpack_sbuffer_destroy(&sbuf);

    return len;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

/**
 *  Packs string into msgpack object
 *
 *  @param pk     msgpack object
 *  @param string string to be packed
 *  @param size   string size
 */
void pack_msgpack_string( msgpack_packer *pk, const void *string, size_t size )
{
    msgpack_pack_str( pk, size );
    msgpack_pack_str_body( pk, string, size );
}
