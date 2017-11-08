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

#include "aker_log.h"
#include "process_data.h"
#include "schedule.h"
#include "scheduler.h"
#include "aker_md5.h"


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
int process_message_cu( wrp_msg_t *msg, wrp_msg_t *object )
{
    if( 0 != strcmp("/parental control/schedule", msg->u.crud.dest) ) {
        return -1;
    }

    /* TODO: Process msg */
    (void) object;

    return 0;
}

ssize_t process_message_ret( wrp_msg_t *msg, void **data )
{
    if( 0 != strcmp("/parental control/schedule", msg->u.crud.dest) &&
        0 != strcmp("/parental control/md5", msg->u.crud.dest) )
    {   
        return -1;
    }

    /* TODO: Retrieve schedule info and return. */
    (void) data;

    return 0;
}

ssize_t process_request_set( const char *filename, wrp_msg_t *req, const char *md5 )
{
    FILE *file_handle = NULL;
    size_t write_size = 0;
    unsigned char result[MD5_SIZE];
    unsigned char *md5_string = NULL;

    file_handle = fopen(filename, "wb");
    if( NULL == file_handle ) {
        debug_info("process_request_set() Failed on fopen(%s, \"wb\"\n", filename);
        return -1;
    }
    debug_print("req->u.req.payload_size = %d\n", req->u.req.payload_size);
    write_size = fwrite(req->u.req.payload, sizeof(uint8_t), req->u.req.payload_size, file_handle);
    fclose(file_handle);
    if (NULL != (md5_string = compute_byte_stream_md5(req->u.req.payload, req->u.req.payload_size, result)))
    {
        file_handle = fopen(md5, "wb");
        if (file_handle) {
            size_t cnt = fwrite(md5_string, sizeof(uint8_t), MD5_SIZE * 2, file_handle);
            if (cnt <= 0) {
                debug_error("process_request_set failed to write %s\n", md5);
            }
            fclose(file_handle);
        }
        free(md5_string);
        process_schedule_data(req->u.req.payload_size, req->u.req.payload);
    } else {
        debug_error("process_request_set()->compute_byte_stream_md5() Failed\n");
    }

    return write_size;
}

size_t process_request_get( const char *filename, wrp_msg_t *resp )
{
    uint8_t *data = NULL;
    size_t read_size = 0;

    read_size = read_file_from_disk( filename, &data );
    resp->u.req.content_type = "application/msgpack";

    resp->u.req.payload = data;
    resp->u.req.payload_size = read_size;

    return read_size;
}

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
        *data = (uint8_t*) malloc(file_size);

        if( NULL != *data ) {
            read_size = fread(*data, sizeof(uint8_t), file_size, file_handle);
        }
    }
    fclose(file_handle);

    return read_size;
}
