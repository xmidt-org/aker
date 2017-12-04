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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <msgpack.h>

#include "aker_log.h"
#include "process_data.h"
#include "scheduler.h"
#include "aker_md5.h"
#include "time.h"
#include "aker_mem.h"
#include "aker_msgpack.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                                   Variables                                */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See process_data.h for details. */
ssize_t process_create( const char *filename, const char *md5, wrp_msg_t *cu )
{
    ssize_t write_size = 0;
    FILE *file_handle = NULL;

    file_handle = fopen(filename, "r");
    if( NULL != file_handle ) {
        write_size = -1;
    } else {
        write_size = process_update(filename, md5, cu);
        if( 0 > write_size ) write_size--;
    }

    return write_size;
}

/* See process_data.h for details. */
ssize_t process_update( const char *filename, const char *md5, wrp_msg_t *cu )
{
    ssize_t write_size = 0;
    unsigned char result[MD5_SIZE];
    unsigned char *md5_string = NULL;
    time_t process_time = get_unix_time();

    md5_string = compute_byte_stream_md5(cu->u.crud.payload, cu->u.crud.payload_size, result);
    if( NULL != md5_string ) {
        if( 0 == process_schedule_data(cu->u.crud.payload_size, cu->u.crud.payload) )
        {
            FILE *file_handle = NULL;

            file_handle = fopen(filename, "wb");
            if( file_handle ) {
                debug_print("cu->u.crud.payload_size = %d\n", cu->u.crud.payload_size);
                write_size = fwrite(cu->u.crud.payload, sizeof(uint8_t), cu->u.crud.payload_size, 
                                 file_handle);
                if( 0 >= write_size ) {
                    debug_error("Create/Update - failed to write %s\n", md5);
                }
                fclose(file_handle);
            } else {
                debug_error("Create/Update - failed on fopen(%s, \"wb\"\n", filename);
            }

            file_handle = fopen(md5, "wb");
            if (file_handle) {
                size_t cnt = fwrite(md5_string, sizeof(uint8_t), MD5_SIZE * 2, file_handle);
                if (cnt <= 0) {
                    debug_error("Create/Update - failed to write %s\n", md5);
                }
                fclose(file_handle);
            }
        } else {
            debug_error("Create/Update - process data failed\n");
            write_size = -1;
        }
        aker_free(md5_string);
    } else {
        debug_error("Create/Update - compute_byte_stream_md5() failed\n");
        write_size = -2;
    }
    process_time = get_unix_time() - process_time;
    debug_info("Time to process schedule file of size %zu bytes is %ld seconds\n", 
                                    ((0 < write_size) ? write_size : 0), process_time);

    return write_size;
}


/* See process_data.h for details. */
ssize_t process_retrieve_persistent( const char *filename, wrp_msg_t *ret )
{
    uint8_t *data = NULL;
    size_t read_size = 0;

    read_size = read_file_from_disk( filename, &data );
    ret->u.crud.content_type = "application/msgpack";

    ret->u.crud.payload = data;
    ret->u.crud.payload_size = read_size;

    return read_size;
}


/* See process_data.h for details. */
ssize_t process_retrieve_now( wrp_msg_t *ret )
{
    const char cstr_active[] = "active";
    const char cstr_time[] = "time";
    time_t current = 0;
    char *macs = NULL;
    size_t macs_size = 0;
    msgpack_sbuffer sbuf;
    msgpack_packer pk;

    current = get_unix_time();
    macs = get_current_blocked_macs();
    if( macs ) macs_size = strlen(macs);

    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
    msgpack_pack_map(&pk, 2);

    pack_msgpack_string(&pk, cstr_active, strlen(cstr_active));
    pack_msgpack_string(&pk, macs, macs_size);

    pack_msgpack_string(&pk, cstr_time, strlen(cstr_time));
    msgpack_pack_int32(&pk, current);

    ret->u.crud.content_type = "application/msgpack";
    if( sbuf.data ) {
        ret->u.crud.payload = aker_malloc(sizeof(char) * sbuf.size);
        if( ret->u.crud.payload ) {
            memcpy(ret->u.crud.payload, sbuf.data, sbuf.size);
            ret->u.crud.payload_size = sbuf.size;
        }
    }
    if( macs ) aker_free(macs);
    msgpack_sbuffer_destroy(&sbuf);

    return ret->u.crud.payload_size;
}


/* See process_data.h for details. */
size_t read_file_from_disk( const char *filename, uint8_t **data )
{
    FILE *file_handle = NULL;
    size_t read_size;
    int local_errno;
    int32_t file_size;

    errno = 0;
    file_handle = fopen(filename, "rb");
    local_errno = errno;
    if( NULL == file_handle ) {
        debug_error("read_file_from_disk() can't read the file %s err %s\n",
                     filename, strerror(local_errno));
        return 0;
    }

    fseek(file_handle, 0, SEEK_END);
    errno = 0;
    file_size = ftell(file_handle);
    local_errno = errno;
    if (file_size < 0) {
        debug_error("read_file_from_disk() ftell() error on %s err %s\n",
                     filename, strerror(local_errno));
        fclose(file_handle);
        return 0;
    }
    fseek(file_handle, 0, SEEK_SET);

    read_size = 0;
    *data = NULL;
    if( file_size > 0 ) {
        *data = (uint8_t*) aker_malloc(file_size);

        if( NULL != *data ) {
            read_size = fread(*data, sizeof(uint8_t), file_size, file_handle);
        }
    }
    fclose(file_handle);

    return read_size;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* None */
