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

#include "aker_log.h"
#include "wrp_interface.h"
#include "process_data.h"
#include "scheduler.h"
#include "aker_mem.h"
#include "aker_msgpack.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct wrp_crud_msg crud_msg_t;
typedef struct wrp_req_msg  req_msg_t;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
void process_crud(const char *data_file, const char *md5_file,
                  const char *service, const char *endpoint,
                  wrp_msg_t *in, wrp_msg_t *response);

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int process_wrp(const char *data_file, const char *md5_file,
                wrp_msg_t *msg, wrp_msg_t *response)
{
    int rv = 0;
    char *service, *endpoint;

    service = wrp_get_msg_dest_element(WRP_ID_ELEMENT__SERVICE, msg);
    endpoint = wrp_get_msg_dest_element(WRP_ID_ELEMENT__APPLICATION, msg);

    switch (msg->msg_type) {
        case WRP_MSG_TYPE__CREATE:
        case WRP_MSG_TYPE__RETREIVE:
        case WRP_MSG_TYPE__UPDATE:
        case WRP_MSG_TYPE__DELETE:
            debug_info("Received CRUD message, type = %d\n", msg-> msg_type);
            process_crud(data_file, md5_file, service, endpoint, msg, response);
            break;

        default:
            debug_info("Message not handled, type = %d\n", msg->msg_type);
            rv = -1;
            break;
    }

    if (endpoint) {
        free(endpoint);
    }
    if (service) {
        free(service);
    }

    return rv;
}

int cleanup_wrp(wrp_msg_t *message)
{
    int rv = -1;

    if( WRP_MSG_TYPE__RETREIVE == message->msg_type ) {
        crud_msg_t *msg = &(message->u.crud);
        if( msg->payload )
            aker_free(msg->payload);
        rv = 0;
    }

    return rv;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
void process_crud(const char *data_file, const char *md5_file,
                  const char *service, const char *endpoint,
                  wrp_msg_t *in, wrp_msg_t *response)
{
    int tmp;
    int payload_valid;
    crud_msg_t *crud_in = &(in->u.crud);
    crud_msg_t *crud_out = &(response->u.crud);

    /* Response struct has been initialized to 0. */

    payload_valid = 0;
    crud_out->status = 400;
    response->msg_type = in->msg_type;

    crud_out->content_type     = "application/msgpack";
    crud_out->transaction_uuid = crud_in->transaction_uuid;
    crud_out->source           = crud_in->dest;
    crud_out->dest             = crud_in->source;
    crud_out->path             = crud_in->path;

    if( (NULL != service) &&
        (NULL != endpoint) &&
        (0 == strcmp(SERVICE_AKER, service)) )
    {
        switch (in->msg_type) {
            case WRP_MSG_TYPE__CREATE:
                if( 0 == strcmp(APP_SCHEDULE, endpoint) ) {
                    if( 0 == process_is_create_ok(data_file) ) {
                        tmp = process_update(data_file, md5_file,
                                                crud_in->payload, crud_in->payload_size );
                        crud_out->status = ((0 == tmp) ? 201 : 533);
                    } else {
                        crud_out->status = 409;
                    }
                } else if( 0 == strcmp(APP_SCHEDULE_END, endpoint) ) {
                    crud_out->status = 405;
                }
                break;
                
            case WRP_MSG_TYPE__RETREIVE:
                if( 0 == strcmp(APP_SCHEDULE, endpoint) ) {
                    crud_out->status = 200;
                    crud_out->payload_size = read_file_from_disk(data_file,
                                                (uint8_t**) &(crud_out->payload));
                } else if( 0 == strcmp(APP_SCHEDULE_END, endpoint) ) {
                    crud_out->status = 200;
                    crud_out->payload_size = process_retrieve_now((uint8_t**) &(crud_out->payload));
                }

                if( 200 == crud_out->status ) {
                    if( 0 == crud_out->payload_size ) {
                        crud_out->payload = NULL;
                        crud_out->status = 404;
                    }
                    payload_valid = 1;
                }
                break;

            case WRP_MSG_TYPE__UPDATE:
                if( 0 == strcmp(APP_SCHEDULE, endpoint) ) {
                    tmp = process_update(data_file, md5_file, crud_in->payload,
                                            crud_in->payload_size );
                    crud_out->status = ((0 == tmp) ? 201 : 534);
                } else if( 0 == strcmp(APP_SCHEDULE_END, endpoint) ) {
                    crud_out->status = 405;
                }
                break;

            case WRP_MSG_TYPE__DELETE:
                if( 0 == strcmp(APP_SCHEDULE, endpoint) ) {
                    crud_out->status = 200;
                    if( 0 != process_delete(data_file, md5_file) ) {
                        crud_out->status = 535;
                    }
                } else if( 0 == strcmp(APP_SCHEDULE_END, endpoint) ) {
                    crud_out->status = 405;
                }
                break;

            default:
                break;
        }
    }

    crud_in->transaction_uuid = NULL;
    crud_in->source = NULL;
    crud_in->dest   = NULL;
    crud_in->path   = NULL;

    if( 0 == payload_valid ) {
        char *text;
        text = "Unknown Error";

        switch( crud_out->status ) {
            case 200: text = "Success";                     break;
            case 201: text = "Created";                     break;
            case 400: text = "Bad Request";                 break;
            case 404: text = "Not Found";                   break;
            case 405: text = "Method Not allowed";          break;
            case 409: text = "Schedule already present";    break;
            case 533: text = "Unable to create schedule";   break;
            case 534: text = "Unable to update schedule";   break;
            case 535: text = "Unable to delete schedule";   break;
        }
        crud_out->payload_size = pack_status_msg(text, &crud_out->payload);
    }
}
