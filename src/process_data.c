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

#include "aker_log.h"
#include "process_data.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define FILE_NAME "pcs.bin"

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

ssize_t process_request_set( wrp_msg_t *req )
{
    FILE *file_handle = NULL;
    size_t write_size = 0;

    file_handle = fopen(FILE_NAME, "wb");
    if( NULL == file_handle ) {
        return -1;
    }

    write_size = fwrite(req->u.req.payload, sizeof(uint8_t), req->u.req.payload_size, file_handle);
    fclose(file_handle);

    /* TODO: Pass off payload to decoder */

    return write_size;
}

ssize_t process_request_get( wrp_msg_t *resp )
{
    FILE *file_handle = NULL;
    size_t file_size = 0, read_size = 0;

    file_handle = fopen(FILE_NAME, "rb");
    if( NULL == file_handle ) {
        return -1;
    }

    resp->u.req.content_type = "application/msgpack";

    fseek(file_handle, 0, SEEK_END);
    file_size = ftell(file_handle);
    fseek(file_handle, 0, SEEK_SET);

    resp->u.req.payload = (uint8_t *)malloc(file_size);
    read_size = fread(resp->u.req.payload, 1, file_size, file_handle);
    fclose(file_handle);

    resp->u.req.payload_size = read_size;

    return read_size;
}

