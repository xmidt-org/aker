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
#include <msgpack.h>

#include "aker_log.h"
#include "wrp_interface.h"
#include "process_data.h"
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

    switch (in_msg->msg_type) {
        case (WRP_MSG_TYPE__CREATE):
        {
            crud_msg_t *in_crud  = &(in_msg->u.crud);
            crud_msg_t *out_crud = &(response->u.crud);
            ssize_t ssize = 0;

            out_crud->status = 201; // default to success
            snprintf(status_str, sizeof(status_str), "Success - schedule CREATE-ed.\n");
            response->msg_type = in_msg->msg_type;

            out_crud->transaction_uuid = in_crud->transaction_uuid;
            out_crud->source  = in_crud->dest;
            out_crud->dest    = in_crud->source;
            out_crud->path    = in_crud->path;
            char *service = wrp_get_msg_dest_element(WRP_ID_ELEMENT__SERVICE, msg);
            if( (NULL != service) && (0 == strcmp(SERVICE_AKER, service)) )
            {
                char *schedule = wrp_get_msg_dest_element(WRP_ID_ELEMENT__APPLICATION, msg);
                if( (NULL != schedule) && (0 == strcmp(APP_SCHEDULE, schedule)) ) 
                {
                    if( -1 == access(data_file, F_OK) )
                    {
                        snprintf(status_str, sizeof(status_str), "Conflict - schedule already CREATE-ed.\n");
                        out_crud->status = 422;
                    } else {
                        ssize = process_update(data_file, md5_file, in_msg);
                        if( 0 > ssize ) {
                            snprintf(status_str, sizeof(status_str), "Error - schedule not CREATE-ed.\n");
                            out_crud->status = 400;
                        }
                    }
                } else {
                    snprintf(status_str, sizeof(status_str), "Not allowed - endpoint(%s)\n", in_crud->dest);
                    out_crud->status = 405;
                }
                if (schedule) {
                    free(schedule);
                }
            } else {
                snprintf(status_str, sizeof(status_str), "Not found - destination(%s)\n", in_crud->dest);
                out_crud->status = 404;
            }
            out_crud->payload_size = pack_status_msgpack_map(status_str, &out_crud->payload);

            in_crud->transaction_uuid = NULL;
            in_crud->source = NULL;
            in_crud->dest   = NULL;
            in_crud->path   = NULL;
            if (service) {
                free(service);
            }
        }
        break;

        case (WRP_MSG_TYPE__UPDATE):
        {
            crud_msg_t *in_crud  = &(in_msg->u.crud);
            crud_msg_t *out_crud = &(response->u.crud);
            ssize_t ssize = 0;

            out_crud->status = 200; // default to success
            snprintf(status_str, sizeof(status_str), "Success - schedule UPDATE-ed.\n");
            response->msg_type = in_msg->msg_type;

            out_crud->transaction_uuid = in_crud->transaction_uuid;
            out_crud->source  = in_crud->dest;
            out_crud->dest    = in_crud->source;
            out_crud->path    = in_crud->path;
            char *service = wrp_get_msg_dest_element(WRP_ID_ELEMENT__SERVICE, msg);
            if( (NULL != service) && (0 == strcmp(SERVICE_AKER, service)) )
            {
                char *schedule = wrp_get_msg_dest_element(WRP_ID_ELEMENT__APPLICATION, msg);
                if( (NULL != schedule) && (0 == strcmp(APP_SCHEDULE, schedule)) ) 
                {
                    ssize = process_update(data_file, md5_file, in_msg);
                    if( 0 > ssize ) {
                        snprintf(status_str, sizeof(status_str), "Error - schedule not CREATE-ed/UPDATE-ed.\n");
                        out_crud->status = 400;
                    }
                } else {
                    snprintf(status_str, sizeof(status_str), "Not allowed - endpoint(%s)\n", in_crud->dest);
                    out_crud->status = 405;
                }
                if (schedule) {
                    free(schedule);
                }
            } else {
                snprintf(status_str, sizeof(status_str), "Not found - destination(%s)\n", in_crud->dest);
                out_crud->status = 404;
            }
            out_crud->payload_size = pack_status_msgpack_map(status_str, &out_crud->payload);

            in_crud->transaction_uuid = NULL;
            in_crud->source = NULL;
            in_crud->dest   = NULL;
            in_crud->path   = NULL;
            if (service) {
                free(service);
            }
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
            char *service = wrp_get_msg_dest_element(WRP_ID_ELEMENT__SERVICE, msg);
            if (service && 0 == strcmp(SERVICE_AKER, service))
            {            
                char *schedule = wrp_get_msg_dest_element(WRP_ID_ELEMENT__APPLICATION, msg);    
                    
                if( schedule && 0 == strcmp(APP_SCHEDULE_PERSIST, schedule) ) {
                    ret_size = process_retrieve_persistent(data_file, response);
                    out_crud->status = ((0 == ret_size) ? 204 : 200);
                } else if(schedule && 0 == strcmp(APP_SCHEDULE_MD5, schedule) ) {
                    ret_size = process_retrieve_persistent(md5_file, response);
                    out_crud->status = ((0 == ret_size) ? 204 : 200);
                } else if(schedule && 0 == strcmp(APP_SCHEDULE_END, schedule) ) {
                    ret_size = process_retrieve_now(response);
                    out_crud->status = ((0 == ret_size) ? 204 : 200);
                } else if(schedule && 0 == strcmp(APP_SCHEDULE, schedule) ) {
                    /* TODO */
                    snprintf(status_str, sizeof(status_str), "Not allowed - /aker/schedule not supported yet.\n");
                    out_crud->status = 405;
                } else {
                    snprintf(status_str, sizeof(status_str), "Not allowed - endpoint(%s)\n", in_crud->dest);
                    out_crud->status = 405;
                }
                if (schedule) {
                    free(schedule);
                }
            } else {
                snprintf(status_str, sizeof(status_str), "Not found - destination(%s)\n", in_crud->dest);
                out_crud->status = 404;
            }
            out_crud->payload_size = pack_status_msgpack_map(status_str, &out_crud->payload);

            in_crud->transaction_uuid = NULL;
            in_crud->source = NULL;
            in_crud->dest   = NULL;
            in_crud->path   = NULL;
            if (service) {
                free(service);
            }
        }
        break;

        case (WRP_MSG_TYPE__DELETE):
        {
            crud_msg_t *in_crud  = &(in_msg->u.crud);
            crud_msg_t *out_crud = &(response->u.crud);

            out_crud->status = 405; // default to not allowed
            snprintf(status_str, sizeof(status_str), "Not allowed - endpoint(%s)\n", in_crud->dest);
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

            snprintf(status_str, sizeof(status_str), "Not allowed - endpoint(%s)\n", in_req->dest);
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
