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
#include "time.h"


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
ssize_t process_message_cu( const char *filename, const char *md5, wrp_msg_t *cu )
{
    FILE *file_handle = NULL;
    size_t write_size = 0;
    unsigned char result[MD5_SIZE];
    unsigned char *md5_string = NULL;
    time_t process_time = 0;

    file_handle = fopen(filename, "wb");
    if( NULL == file_handle ) {
        debug_error("process_message_cu() Failed on fopen(%s, \"wb\"\n", filename);
        return -1;
    }
    debug_print("cu->u.crud.payload_size = %d\n", cu->u.crud.payload_size);
    write_size = fwrite(cu->u.crud.payload, sizeof(uint8_t), cu->u.crud.payload_size, file_handle);
    fclose(file_handle);
    if (NULL != (md5_string = compute_byte_stream_md5(cu->u.crud.payload, cu->u.crud.payload_size, result)))
    {
        process_time = get_unix_time();
        if (0 == process_schedule_data(cu->u.crud.payload_size, cu->u.crud.payload))  {
            file_handle = fopen(md5, "wb");
            if (file_handle) {
                size_t cnt = fwrite(md5_string, sizeof(uint8_t), MD5_SIZE * 2, file_handle);
                if (cnt <= 0) {
                    debug_error("process_message_cu failed to write %s\n", md5);
                }
                fclose(file_handle);
            }
        }
        free(md5_string);
        process_time = get_unix_time() - process_time;
        debug_info("Time to process schedule file of size %zu bytes is %ld seconds\n", write_size, process_time);

    } else {
        debug_error("process_message_cu()->compute_byte_stream_md5() Failed\n");
    }

    return write_size;
}

ssize_t process_message_ret( const char *filename, wrp_msg_t *ret )
{
    uint8_t *data = NULL;
    size_t read_size = 0;

    read_size = read_file_from_disk( filename, &data );
    ret->u.crud.content_type = "application/msgpack";

    ret->u.crud.payload = data;
    ret->u.crud.payload_size = read_size;

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
