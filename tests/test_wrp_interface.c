/**
 *  Copyright 2017 Comcast Cable Communications Management, LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <wrp-c.h>

#include <CUnit/Basic.h>

#include "test_macros.h"
#include "../src/wrp_interface.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */
/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    size_t pack_status_msgpack_map_rv;
    int process_update_rv;
    size_t process_retrieve_now_rv;
    int process_schedule_data_rv;
    size_t read_file_from_disk_rv;
    int process_is_create_ok_rv;
    int process_delete_rv;
    wrp_msg_t s;
    wrp_msg_t r;
} test_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
int32_t get_max_mac_limit(void)
{
    return 128;
}

static size_t pack_status_msgpack_map_rv = 0;
size_t pack_status_msgpack_map(const char *string, void **binary)
{
    (void) string;
    (void) binary;

    return pack_status_msgpack_map_rv;
}

static int process_update_rv = 0;
int process_update( const char *filename, const char *md5_file,
                    void *payload, size_t payload_size )
{
    (void) filename;
    (void) md5_file;
    (void) payload;
    (void) payload_size;

    return process_update_rv;
}

static size_t process_retrieve_now_rv = 0;
size_t process_retrieve_now( uint8_t **data )
{
    (void) data;

    return process_retrieve_now_rv;
}

static int process_schedule_data_rv = 0;
int process_schedule_data( size_t len, uint8_t *data )
{
    (void) len;
    (void) data;

    return process_schedule_data_rv;
}

static size_t read_file_from_disk_rv = 0;
size_t read_file_from_disk( const char *filename, void **data )
{
    (void) filename;
    (void) data;

    return read_file_from_disk_rv;
}

static int process_is_create_ok_rv = 0;
int process_is_create_ok( const char *filename )
{
    (void) filename;

    return process_is_create_ok_rv;
}

static int process_delete_rv = 0;
int process_delete( const char *filename, const char *md5_file )
{
    (void) filename;
    (void) md5_file;

    return process_delete_rv;
}

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_process_wrp()
{
    test_t tests[] = {
        {   // 0
            .pack_status_msgpack_map_rv = 0,
            .process_update_rv = 0,
            .process_retrieve_now_rv = 0,
            .process_schedule_data_rv = 0,
            .read_file_from_disk_rv = 0,
            .process_is_create_ok_rv = 0,
            .process_delete_rv = 0,

            .s.msg_type = WRP_MSG_TYPE__CREATE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = "mac:112233445566/aker/schedule",
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = "Some binary",
            .s.u.crud.payload_size = 11,

            .r.msg_type = WRP_MSG_TYPE__CREATE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = "mac:112233445566/aker/schedule",
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 201,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = NULL,
            .r.u.crud.payload_size = 0,
        },

        {   // 1
            .pack_status_msgpack_map_rv = 0,
            .process_update_rv = 0,
            .process_retrieve_now_rv = 0,
            .process_schedule_data_rv = 0,
            .read_file_from_disk_rv = 0,
            .process_is_create_ok_rv = -1,
            .process_delete_rv = 0,

            .s.msg_type = WRP_MSG_TYPE__CREATE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = "mac:112233445566/aker/schedule",
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = "Some binary",
            .s.u.crud.payload_size = 11,

            .r.msg_type = WRP_MSG_TYPE__CREATE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = "mac:112233445566/aker/schedule",
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 409,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = NULL,
            .r.u.crud.payload_size = 0,
        },

        {   // 2
            .pack_status_msgpack_map_rv = 0,
            .process_update_rv = -1,
            .process_retrieve_now_rv = 0,
            .process_schedule_data_rv = 0,
            .read_file_from_disk_rv = 0,
            .process_is_create_ok_rv = 0,
            .process_delete_rv = 0,

            .s.msg_type = WRP_MSG_TYPE__CREATE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = "mac:112233445566/aker/schedule",
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = "Some binary",
            .s.u.crud.payload_size = 11,

            .r.msg_type = WRP_MSG_TYPE__CREATE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = "mac:112233445566/aker/schedule",
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 533,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = NULL,
            .r.u.crud.payload_size = 0,
        },

        {   // 3
            .pack_status_msgpack_map_rv = 0,
            .process_update_rv = -1,
            .process_retrieve_now_rv = 0,
            .process_schedule_data_rv = 0,
            .read_file_from_disk_rv = 0,
            .process_is_create_ok_rv = 0,
            .process_delete_rv = 0,

            .s.msg_type = WRP_MSG_TYPE__CREATE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = "mac:112233445566/aker/now",
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = "Some binary",
            .s.u.crud.payload_size = 11,

            .r.msg_type = WRP_MSG_TYPE__CREATE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = "mac:112233445566/aker/now",
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 405,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = NULL,
            .r.u.crud.payload_size = 0,
        },

        {   // 4
            .pack_status_msgpack_map_rv = 0,
            .process_update_rv = 0,
            .process_retrieve_now_rv = 0,
            .process_schedule_data_rv = 0,
            .read_file_from_disk_rv = 0,
            .process_is_create_ok_rv = 0,
            .process_delete_rv = 0,

            .s.msg_type = WRP_MSG_TYPE__CREATE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = "mac:112233445566/aker/schedule/invalid",
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = "Some binary",
            .s.u.crud.payload_size = 11,

            .r.msg_type = WRP_MSG_TYPE__CREATE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = "mac:112233445566/aker/schedule/invalid",
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 400,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = NULL,
            .r.u.crud.payload_size = 0,
        },

        {   // 5
            .pack_status_msgpack_map_rv = 0,
            .process_update_rv = 0,
            .process_retrieve_now_rv = 0,
            .process_schedule_data_rv = 0,
            .read_file_from_disk_rv = 0,
            .process_is_create_ok_rv = 0,
            .process_delete_rv = 0,

            .s.msg_type = WRP_MSG_TYPE__UPDATE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = "mac:112233445566/aker/schedule",
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.spans.count = 0,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = "Some binary",
            .s.u.crud.payload_size = 11,

            .r.msg_type = WRP_MSG_TYPE__UPDATE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = "mac:112233445566/aker/schedule",
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 201,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = NULL,
            .r.u.crud.payload_size = 0,
        },

        {   // 6
            .pack_status_msgpack_map_rv = 0,
            .process_update_rv = -1,
            .process_retrieve_now_rv = 0,
            .process_schedule_data_rv = 0,
            .read_file_from_disk_rv = 0,
            .process_is_create_ok_rv = 0,
            .process_delete_rv = 0,

            .s.msg_type = WRP_MSG_TYPE__UPDATE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = "mac:112233445566/aker/schedule",
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.spans.count = 0,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = "Some binary",
            .s.u.crud.payload_size = 11,

            .r.msg_type = WRP_MSG_TYPE__UPDATE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = "mac:112233445566/aker/schedule",
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 534,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = NULL,
            .r.u.crud.payload_size = 0,
        },

        {   // 7
            .pack_status_msgpack_map_rv = 0,
            .process_update_rv = 0,
            .process_retrieve_now_rv = 0,
            .process_schedule_data_rv = 0,
            .read_file_from_disk_rv = 0,
            .process_is_create_ok_rv = 0,
            .process_delete_rv = 0,

            .s.msg_type = WRP_MSG_TYPE__UPDATE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = "mac:112233445566/aker/now",
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.spans.count = 0,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = "Some binary",
            .s.u.crud.payload_size = 11,

            .r.msg_type = WRP_MSG_TYPE__UPDATE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = "mac:112233445566/aker/now",
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 405,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = NULL,
            .r.u.crud.payload_size = 0,
        },

        {   // 8
            .pack_status_msgpack_map_rv = 0,
            .process_update_rv = 0,
            .process_retrieve_now_rv = 0,
            .process_schedule_data_rv = 0,
            .read_file_from_disk_rv = 0,
            .process_is_create_ok_rv = 0,
            .process_delete_rv = 0,

            .s.msg_type = WRP_MSG_TYPE__UPDATE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = "mac:112233445566/aker/now/invalid",
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.spans.count = 0,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = "Some binary",
            .s.u.crud.payload_size = 11,

            .r.msg_type = WRP_MSG_TYPE__UPDATE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = "mac:112233445566/aker/now/invalid",
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 400,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = NULL,
            .r.u.crud.payload_size = 0,
        },

        {   // 9
            .pack_status_msgpack_map_rv = 0,
            .process_update_rv = 0,
            .process_retrieve_now_rv = 0,
            .process_schedule_data_rv = 0,
            .read_file_from_disk_rv = 16,
            .process_is_create_ok_rv = 0,
            .process_delete_rv = 0,

            .s.msg_type = WRP_MSG_TYPE__RETREIVE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = "mac:112233445566/aker/schedule",
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.spans.count = 0,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = NULL,
            .s.u.crud.payload_size = 0,

            .r.msg_type = WRP_MSG_TYPE__RETREIVE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = "mac:112233445566/aker/schedule",
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 200,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = "Some other binary",
            .r.u.crud.payload_size = 16,
        },

        {   // 10
            .pack_status_msgpack_map_rv = 0,
            .process_update_rv = 0,
            .process_retrieve_now_rv = 0,
            .process_schedule_data_rv = 0,
            .read_file_from_disk_rv = 0,
            .process_is_create_ok_rv = 0,
            .process_delete_rv = 0,

            .s.msg_type = WRP_MSG_TYPE__RETREIVE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = "mac:112233445566/aker/schedule",
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.spans.count = 0,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = NULL,
            .s.u.crud.payload_size = 0,

            .r.msg_type = WRP_MSG_TYPE__RETREIVE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = "mac:112233445566/aker/schedule",
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 404,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = NULL,
            .r.u.crud.payload_size = 0,
        },


        {   // 11
            .pack_status_msgpack_map_rv = 0,
            .process_update_rv = 0,
            .process_retrieve_now_rv = 0,
            .process_schedule_data_rv = 0,
            .read_file_from_disk_rv = 0,
            .process_is_create_ok_rv = 0,
            .process_delete_rv = 0,

            .s.msg_type = WRP_MSG_TYPE__RETREIVE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = "mac:112233445566/aker/schedule/invalid",
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.spans.count = 0,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = NULL,
            .s.u.crud.payload_size = 0,

            .r.msg_type = WRP_MSG_TYPE__RETREIVE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = "mac:112233445566/aker/schedule/invalid",
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 400,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = "Some other binary",
            .r.u.crud.payload_size = 16,
        },

        {   // 12
            .pack_status_msgpack_map_rv = 0,
            .process_update_rv = 0,
            .process_retrieve_now_rv = 16,
            .process_schedule_data_rv = 0,
            .read_file_from_disk_rv = 0,
            .process_is_create_ok_rv = 0,
            .process_delete_rv = 0,

            .s.msg_type = WRP_MSG_TYPE__RETREIVE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = "mac:112233445566/aker/now",
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.spans.count = 0,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = NULL,
            .s.u.crud.payload_size = 0,

            .r.msg_type = WRP_MSG_TYPE__RETREIVE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = "mac:112233445566/aker/now",
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 200,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = "Some other binary",
            .r.u.crud.payload_size = 16,
        },

        {   // 13
            .pack_status_msgpack_map_rv = 0,
            .process_update_rv = 0,
            .process_retrieve_now_rv = 0,
            .process_schedule_data_rv = 0,
            .read_file_from_disk_rv = 0,
            .process_is_create_ok_rv = 0,
            .process_delete_rv = 0,

            .s.msg_type = WRP_MSG_TYPE__DELETE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = "mac:112233445566/aker/now",
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.spans.count = 0,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = NULL,
            .s.u.crud.payload_size = 0,

            .r.msg_type = WRP_MSG_TYPE__DELETE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = "mac:112233445566/aker/now",
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 405,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = NULL,
            .r.u.crud.payload_size = 0,
        },

        {   // 14
            .pack_status_msgpack_map_rv = 0,
            .process_update_rv = 0,
            .process_retrieve_now_rv = 0,
            .process_schedule_data_rv = 0,
            .read_file_from_disk_rv = 0,
            .process_is_create_ok_rv = 0,
            .process_delete_rv = 0,

            .s.msg_type = WRP_MSG_TYPE__DELETE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = "mac:112233445566/aker/schedule",
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.spans.count = 0,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = NULL,
            .s.u.crud.payload_size = 0,

            .r.msg_type = WRP_MSG_TYPE__DELETE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = "mac:112233445566/aker/schedule",
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 200,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = NULL,
            .r.u.crud.payload_size = 0,
        },

        {   // 15
            .pack_status_msgpack_map_rv = 0,
            .process_update_rv = 0,
            .process_retrieve_now_rv = 0,
            .process_schedule_data_rv = -1,
            .read_file_from_disk_rv = 0,
            .process_is_create_ok_rv = 0,
            .process_delete_rv = -1,

            .s.msg_type = WRP_MSG_TYPE__DELETE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server",
            .s.u.crud.dest = "mac:112233445566/aker/schedule",
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.spans.count = 0,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = NULL,
            .s.u.crud.payload_size = 0,

            .r.msg_type = WRP_MSG_TYPE__DELETE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = "mac:112233445566/aker/schedule",
            .r.u.crud.dest = "fake-server",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.status = 535,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = NULL,
            .r.u.crud.payload_size = 0,
        },
    };
    size_t t_size = sizeof(tests)/sizeof(test_t);
    uint8_t i;

    for( i = 0; i < t_size; i++ ) {
        wrp_msg_t msg;

        pack_status_msgpack_map_rv = tests[i].pack_status_msgpack_map_rv;
        process_update_rv = tests[i].process_update_rv;
        process_retrieve_now_rv = tests[i].process_retrieve_now_rv;
        process_schedule_data_rv = tests[i].process_schedule_data_rv;
        read_file_from_disk_rv = tests[i].read_file_from_disk_rv;
        process_is_create_ok_rv = tests[i].process_is_create_ok_rv;
        process_delete_rv = tests[i].process_delete_rv;

        int rv = process_wrp("data", "md5", &(tests[i].s), &msg);
        CU_ASSERT(0 <= rv);
        CU_ASSERT(tests[i].r.msg_type == msg.msg_type);
        if( WRP_MSG_TYPE__REQ == msg.msg_type ) {
            CU_ASSERT_STRING_EQUAL(tests[i].r.u.req.transaction_uuid, msg.u.req.transaction_uuid);
            CU_ASSERT_STRING_EQUAL(tests[i].r.u.req.source, msg.u.req.source);
            CU_ASSERT_STRING_EQUAL(tests[i].r.u.req.dest, msg.u.req.dest);
        } else {
            CU_ASSERT_STRING_EQUAL(tests[i].r.u.crud.transaction_uuid, msg.u.crud.transaction_uuid);
            CU_ASSERT_STRING_EQUAL(tests[i].r.u.crud.source, msg.u.crud.source);
            CU_ASSERT_STRING_EQUAL(tests[i].r.u.crud.dest, msg.u.crud.dest);
            if( tests[i].r.u.crud.status !=  msg.u.crud.status ) {
                printf( "\nTest: %d Expected: %d, Got: %d\n", i, tests[i].r.u.crud.status, msg.u.crud.status );
            }
            CU_ASSERT_EQUAL(tests[i].r.u.crud.status, msg.u.crud.status);
            CU_ASSERT(0 == strcmp(tests[i].r.u.crud.path, msg.u.crud.path));
        }
        // TODO Fix the leak! cleanup_wrp(&msg);
    }
}

void add_suites( CU_pSuite *suite )
{
    printf("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test 1", test_process_wrp );
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main( void )
{
    unsigned rv = 1;
    CU_pSuite suite = NULL;

    if( CUE_SUCCESS == CU_initialize_registry() ) {
        add_suites( &suite );

        if( NULL != suite ) {
            CU_basic_set_mode( CU_BRM_VERBOSE );
            CU_basic_run_tests();
            printf( "\n" );
            CU_basic_show_failures( CU_get_failure_list() );
            printf( "\n\n" );
            rv = CU_get_number_of_tests_failed();
        }

        CU_cleanup_registry();

    }

    return rv;
}
