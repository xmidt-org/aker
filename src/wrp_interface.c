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
/* None */

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int process_wrp(const char *data_file, const char *md5_file,
                wrp_msg_t *msg, wrp_msg_t *response)
{
    wrp_msg_t *in_msg = msg;
    char status_str[100] = {0};
    int rv = 0;
    char *service, *endpoint;


    service = wrp_get_msg_dest_element(WRP_ID_ELEMENT__SERVICE, msg);
    endpoint = wrp_get_msg_dest_element(WRP_ID_ELEMENT__APPLICATION, msg);

    switch (in_msg->msg_type) {
        case (WRP_MSG_TYPE__CREATE):
        {
            crud_msg_t *in_crud  = &(in_msg->u.crud);
            crud_msg_t *out_crud = &(response->u.crud);
            ssize_t ssize = 0;

            out_crud->status = 201; // default to success
            snprintf(status_str, sizeof(status_str), "Success.");
            response->msg_type = in_msg->msg_type;

            out_crud->transaction_uuid = in_crud->transaction_uuid;
            out_crud->source  = in_crud->dest;
            out_crud->dest    = in_crud->source;
            out_crud->path    = in_crud->path;
            if( (NULL != service) && (0 == strcmp(SERVICE_AKER, service)) )
            {
                if( (NULL != endpoint) && (0 == strcmp(APP_SCHEDULE, endpoint)) ) 
                {
                    if( -1 == access(data_file, F_OK) )
                    {
                        snprintf(status_str, sizeof(status_str), "Schedule already present.");
                        out_crud->status = 422;
                    } else {
                        ssize = process_update(data_file, md5_file, in_msg);
                        if( 0 > ssize ) {
                            snprintf(status_str, sizeof(status_str), "Unable to create schedule.");
                            out_crud->status = 400;
                        }
                    }
                } else {
                    snprintf(status_str, sizeof(status_str), "Not allowed.");
                    out_crud->status = 405;
                }
                if (endpoint) {
                    free(endpoint);
                }
            } else {
                snprintf(status_str, sizeof(status_str), "Not found.");
                out_crud->status = 404;
            }
            out_crud->payload_size = pack_status_msgpack_map(status_str, &out_crud->payload);

            in_crud->transaction_uuid = NULL;
            in_crud->source = NULL;
            in_crud->dest   = NULL;
            in_crud->path   = NULL;
        }
        break;

        case (WRP_MSG_TYPE__UPDATE):
        {
            crud_msg_t *in_crud  = &(in_msg->u.crud);
            crud_msg_t *out_crud = &(response->u.crud);
            ssize_t ssize = 0;

            out_crud->status = 200; // default to success
            snprintf(status_str, sizeof(status_str), "Success.");
            response->msg_type = in_msg->msg_type;

            out_crud->transaction_uuid = in_crud->transaction_uuid;
            out_crud->source  = in_crud->dest;
            out_crud->dest    = in_crud->source;
            out_crud->path    = in_crud->path;
            if( (NULL != service) && (0 == strcmp(SERVICE_AKER, service)) )
            {
                if( (NULL != endpoint) && (0 == strcmp(APP_SCHEDULE, endpoint)) ) 
                {
                    ssize = process_update(data_file, md5_file, in_msg);
                    if( 0 > ssize ) {
                        snprintf(status_str, sizeof(status_str), "Unable to update schedule.");
                        out_crud->status = 400;
                    }
                } else {
                    snprintf(status_str, sizeof(status_str), "Not allowed.");
                    out_crud->status = 405;
                }
                if (endpoint) {
                    free(endpoint);
                }
            } else {
                snprintf(status_str, sizeof(status_str), "Not found.");
                out_crud->status = 404;
            }
            out_crud->payload_size = pack_status_msgpack_map(status_str, &out_crud->payload);

            in_crud->transaction_uuid = NULL;
            in_crud->source = NULL;
            in_crud->dest   = NULL;
            in_crud->path   = NULL;
        }
        break;

        case (WRP_MSG_TYPE__RETREIVE):
        {
            crud_msg_t *in_crud  = &(in_msg->u.crud);
            crud_msg_t *out_crud = &(response->u.crud);
            ssize_t ret_size = 0;

            response->msg_type = WRP_MSG_TYPE__RETREIVE;

            out_crud->transaction_uuid = in_crud->transaction_uuid;
            out_crud->source  = in_crud->dest;
            out_crud->dest    = in_crud->source;
            out_crud->path    = in_crud->path;
            if (service && 0 == strcmp(SERVICE_AKER, service))
            {            
                if( endpoint && 0 == strcmp(APP_SCHEDULE, endpoint) ) {
                    ret_size = process_retrieve_persistent(data_file, response);
                    out_crud->status = ((0 == ret_size) ? 204 : 200);
                } else if(endpoint && 0 == strcmp(APP_SCHEDULE_END, endpoint) ) {
                    ret_size = process_retrieve_now(response);
                    out_crud->status = ((0 == ret_size) ? 204 : 200);
                } else {
                    snprintf(status_str, sizeof(status_str), "Not allowed.");
                    out_crud->status = 405;
                }
            } else {
                snprintf(status_str, sizeof(status_str), "Not found.");
                out_crud->status = 404;
            }
            out_crud->payload_size = pack_status_msgpack_map(status_str, &out_crud->payload);

            in_crud->transaction_uuid = NULL;
            in_crud->source = NULL;
            in_crud->dest   = NULL;
            in_crud->path   = NULL;
        }
        break;

        case (WRP_MSG_TYPE__DELETE):
        {
            crud_msg_t *in_crud  = &(in_msg->u.crud);
            crud_msg_t *out_crud = &(response->u.crud);

            out_crud->status = 405; // default to not allowed
            snprintf(status_str, sizeof(status_str), "Not allowed.");

            if (service && 0 == strcmp(SERVICE_AKER, service)) {
                if( endpoint && 0 == strcmp(APP_SCHEDULE, endpoint) ) {
                    int success[3];
                    success[0] = process_schedule_data( 0, NULL );
                    success[1] = 0;
                    success[2] = 0;
                    if( NULL != data_file ) {
                        success[1] = remove( data_file );
                    }
                    if( NULL != md5_file ) {
                        success[2] = remove( md5_file );
                    }

                    if( (0 == success[0]) && (0 == success[1]) && (0 == success[2]) )
                    {
                        out_crud->status = 200;
                        snprintf(status_str, sizeof(status_str), "Success.");
                    } else {
                        out_crud->status = 500;
                        snprintf(status_str, sizeof(status_str), "Problem deleting (%d|%d|%d).",
                                 success[0], success[1], success[2]);
                    }
                }
            }
            response->msg_type = WRP_MSG_TYPE__DELETE;

            out_crud->transaction_uuid = in_crud->transaction_uuid;
            out_crud->source  = in_crud->dest;
            out_crud->dest    = in_crud->source;
            out_crud->path    = in_crud->path;
            out_crud->payload_size = pack_status_msgpack_map(status_str, &out_crud->payload);
  
            in_crud->transaction_uuid = NULL;
            in_crud->source = NULL;
            in_crud->dest   = NULL;
            in_crud->path   = NULL;
        }
        break;
            
        case (WRP_MSG_TYPE__REQ):
        {
            req_msg_t *in_req  = &(in_msg->u.req);
            req_msg_t *out_req = &(response->u.req);

            snprintf(status_str, sizeof(status_str), "Not allowed.");
            response->msg_type = WRP_MSG_TYPE__REQ;

            out_req->transaction_uuid = in_req->transaction_uuid;
            out_req->source  = in_req->dest;
            out_req->dest    = in_req->source;
            out_req->payload_size = pack_status_msgpack_map(status_str, &out_req->payload);

            in_req->transaction_uuid = NULL;
            in_req->source = NULL;
            in_req->dest   = NULL;
        }
        break;

        default:
            debug_info("Message of type %d not handled\n", in_msg->msg_type);
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
/* None */
