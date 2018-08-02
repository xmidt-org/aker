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
#include <unistd.h>

#include "aker_log.h"
#include "process_data.h"
#include "scheduler.h"
#include "aker_md5.h"
#include "aker_msgpack.h"
#include "time.h"
#include "aker_mem.h"

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
int process_is_create_ok( const char *filename )
{
    if( (NULL != filename) && (0 == access(filename, F_OK)) ) {
        return -1;
    }

    return 0;
}

/* See process_data.h for details. */
int process_update( const char *filename, const char *md5, 
                        void *payload, size_t payload_size )
{
    int rv;
    unsigned char result[MD5_SIZE];
    unsigned char *md5_string = NULL;
    time_t process_time;

    process_time = get_unix_time();
    rv = 0;

    md5_string = compute_byte_stream_md5(payload, payload_size, result);
    if( (NULL != md5_string) && (0 < payload_size) ) {
        if( 0 == process_schedule_data(payload_size, payload) ) {
            FILE *fh = NULL;

            fh = fopen(filename, "wb");
            if( fh ) {
                debug_print("payload_size = %d\n", payload_size);
                if( payload_size == fwrite(payload, sizeof(uint8_t), payload_size, fh) ) {
                    rv += 1;
                } else {
                    debug_error("Create/Update - failed to write %s\n", md5);
                }
                fclose(fh);
            } else {
                debug_error("Create/Update - failed on fopen(%s, \"wb\"\n", filename);
            }

            fh = fopen(md5, "wb");
            if (fh) {
                if( 0 < fwrite(md5_string, sizeof(uint8_t), MD5_SIZE * 2, fh) ) {
                    rv += 2;
                } else {
                    debug_error("Create/Update - failed to write %s\n", md5);
                }
                fclose(fh);
            }

            /* If we wrote both files successfully, rv is 3 - then it's a success. */
            if( 3 == rv ) {
                rv = 0;
            } else {
                rv = -1;
            }
        } else {
            debug_error("Create/Update - process data failed\n");
            rv = -2;
        }
    } else {
        debug_error("Create/Update - compute_byte_stream_md5() failed\n");
        rv = -3;
    }

    if( NULL != md5_string ) {
        aker_free(md5_string);
    }

    process_time = get_unix_time() - process_time;
    debug_info("Time to process schedule file of size %zu bytes is %ld seconds\n", 
                                    ((0 < rv) ? rv : 0), process_time);

    return rv;
}


/* See process_data.h for details. */
size_t process_retrieve_now( uint8_t **data )
{
    time_t current;
    char *macs;
    size_t rv;

    current = get_unix_time();
    macs = strdup(get_current_blocked_macs());

    rv = pack_now_msg (macs, current, (void**) data);

    if (macs) {
        aker_free(macs);
    }

    return rv;
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

/* See process_data.h for details. */
int process_delete( const char *filename, const char *md5_file )
{
    int rv;

    rv = process_schedule_data(0, NULL);

    /* We don't care if these have errors, just try to delete the files. */
    if( NULL != filename ) {
        (void) remove(filename);
    }
    if( NULL != md5_file ) {
        (void) remove(md5_file);
    }

    return rv;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* None */
