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
#include "aker_mem.h"

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
int wrp_process(const char *data_file, const char *md5_file,
                wrp_msg_t *msg, wrp_msg_t *response)
{
    wrp_msg_t *in_msg = msg;
    int rv = -1;

    switch (in_msg->msg_type) {
        case (WRP_MSG_TYPE__CREATE):
        case (WRP_MSG_TYPE__UPDATE):
        {
            crud_msg_t *in_crud  = &(in_msg->u.crud);
            crud_msg_t *out_crud = &(response->u.crud);
            ssize_t cu_size = 0;

            out_crud->status = 400; // default to failed
            /* Not as per WRP spec - Response to Update */
            response->msg_type = in_msg->msg_type;

            out_crud->transaction_uuid = in_crud->transaction_uuid;
            out_crud->source  = in_crud->dest;
            out_crud->dest    = in_crud->source;
            out_crud->path    = in_crud->path;
            char *service = wrp_get_msg_dest_element(WRP_ID_ELEMENT__SERVICE, msg);
            if (service && 0 == strcmp(SERVICE_AKER, service))
            {
                char *schedule = wrp_get_msg_dest_element(WRP_ID_ELEMENT__APPLICATION, msg);
                if( schedule && 0 == strcmp(APP_SCHEDULE, schedule) ) {
                    cu_size = process_message_cu(data_file, md5_file, in_msg);
                } else {
                    debug_error("CREATE/UPDATE message destination %s is invalid\n", in_crud->dest);
                }
                if( 0 <= cu_size ) {
                    out_crud->status = 200;
                }
                if (schedule) {
                    free(schedule);
                }
            }
            in_crud->transaction_uuid = NULL;
            in_crud->source = NULL;
            in_crud->dest   = NULL;
            in_crud->path   = NULL;
            if (service) {
                free(service);
            }
            rv = 0;
        }
        break;

        case (WRP_MSG_TYPE__RETREIVE):
        {
            crud_msg_t *in_crud  = &(in_msg->u.crud);
            crud_msg_t *out_crud = &(response->u.crud);
            ssize_t ret_size = 0;

            out_crud->status = 400; // default to failed
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
                    ret_size = process_message_ret_all(data_file, response);
                } else if(schedule && 0 == strcmp(APP_SCHEDULE_MD5, schedule) ) {
                    ret_size = process_message_ret_all(md5_file, response);
                } else if(schedule && 0 == strcmp(APP_SCHEDULE_END, schedule) ) {
                    ret_size = process_message_ret_now(response);
                } else if(schedule && 0 == strcmp(APP_SCHEDULE, schedule) ) {
                    /* TODO */
                    debug_error("RETRIEVE /aker/schedule not supported yet.");
                } else {
                    debug_error("RETRIEVE message destination %s is invalid\n", in_crud->dest);
                }
                if (schedule) {
                    free(schedule);
                }
                if( 0 <= ret_size ) {
                    out_crud->status = 200;
                }
            }
            in_crud->transaction_uuid = NULL;
            in_crud->source = NULL;
            in_crud->dest   = NULL;
            in_crud->path   = NULL;
            if (service) {
                free(service);
            }
            rv = 0;
        }
        break;

        case (WRP_MSG_TYPE__DELETE):
        {
            crud_msg_t *in_crud  = &(in_msg->u.crud);
            crud_msg_t *out_crud = &(response->u.crud);

            out_crud->status = 400; // default to failed
            response->msg_type = WRP_MSG_TYPE__DELETE;

            out_crud->transaction_uuid = in_crud->transaction_uuid;
            out_crud->source  = in_crud->dest;
            out_crud->dest    = in_crud->source;
            out_crud->path    = in_crud->path;

            in_crud->transaction_uuid = NULL;
            in_crud->source = NULL;
            in_crud->dest   = NULL;
            in_crud->path   = NULL;

            rv = 0;
        }
        break;
            
        case (WRP_MSG_TYPE__REQ):
        {
            req_msg_t *in_req  = &(in_msg->u.req);
            req_msg_t *out_req = &(response->u.req);

            response->msg_type = WRP_MSG_TYPE__REQ;

            out_req->transaction_uuid = in_req->transaction_uuid;
            out_req->source  = in_req->dest;
            out_req->dest    = in_req->source;

            in_req->transaction_uuid = NULL;
            in_req->source = NULL;
            in_req->dest   = NULL;

            rv = 0;
        }
        break;

        default:
            debug_info("Message of type %d not handled\n", in_msg->msg_type);
        break;
    }

    return rv;
}

int wrp_cleanup(wrp_msg_t *message)
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
