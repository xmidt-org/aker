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
ssize_t wrp_processing(wrp_msg_t *msg, void **message)
{
    wrp_msg_t *in_msg = msg;
    wrp_msg_t response;
    ssize_t   message_size;

    memset(&response, 0, sizeof(wrp_msg_t));

    switch (in_msg->msg_type) {
        case (WRP_MSG_TYPE__CREATE): 
        case (WRP_MSG_TYPE__UPDATE):
        {
            crud_msg_t *out_crud = &(response.u.crud);

            out_crud->status = 400; // default to failed
            /* Not as per WRP spec - Response to Update */
            response.msg_type = in_msg->msg_type;

            if( 0 == process_message_cu(in_msg, &response) ) {
                out_crud->status =200;
            }
        }
        break;
        
        case (WRP_MSG_TYPE__RETREIVE):
        {
            crud_msg_t *in_crud  = &(in_msg->u.crud);
            crud_msg_t *out_crud = &(response.u.crud);

            out_crud->status = 400; // default to failed
            response.msg_type = WRP_MSG_TYPE__RETREIVE;

            out_crud->transaction_uuid = strdup(in_crud->transaction_uuid);
            out_crud->source  = strdup(in_crud->dest);
            out_crud->dest    = strdup(in_crud->source);
            out_crud->headers = NULL;
            out_crud->metadata = NULL;
            out_crud->include_spans = false;
            out_crud->spans.spans = NULL;
            out_crud->spans.count = 0;
            out_crud->rdr     = 0;
            out_crud->path    = strdup(in_crud->path);
            /* TODO: once payload type is resolved */
            // process_message_ret(in_msg, &(out_crud->payload)); 
            if( NULL != out_crud->payload ) {
                out_crud->status = 200;
            }
        }
        break;

        case (WRP_MSG_TYPE__REQ):
        {
            req_msg_t *req = &(in_msg->u.req);
            req_msg_t *resp = &(response.u.req);

            response.msg_type = WRP_MSG_TYPE__REQ;
            
            resp->transaction_uuid = strdup(req->transaction_uuid);
            resp->source = strdup(req->dest);
            resp->dest   = strdup(req->source);
            resp->partner_ids = req->partner_ids;
            resp->headers = req->headers;
            resp->content_type = NULL;
            resp->include_spans = req->include_spans;
            resp->spans.spans = req->spans.spans;
            resp->spans.count = req->spans.count;
            resp->payload = NULL;
            resp->payload_size = 0;
            if( 0 == strcmp(SET_DEST, req->dest) ) {
                process_request_set(in_msg, &response);
            } else if( 0 == strcmp(GET_DEST, req->dest) ) {
                process_request_get(in_msg, &response);
            } else {
                debug_error("Request-Response message destination %s is invalid\n", req->dest);
                break;
            }
        }
            
        default:
            debug_info("Message of type %d not handled\n", in_msg->msg_type);
        break;
    }

    message_size = wrp_struct_to(&response, WRP_BYTES, message);    
    /* TODO: Handle CRUD type after fix to handle binary payload. */
    /* Request-Response WRP is temporary. */
    if( WRP_MSG_TYPE__REQ == response.msg_type )
    {
        req_msg_t *ret = &(response.u.req);
        if (ret->transaction_uuid)
            free(ret->transaction_uuid);
        if (ret->dest)             
            free(ret->dest);
        if (ret->payload )         
            free(ret->payload);
        if (ret->source)           
            free(ret->source);
    }
     
    if (in_msg) {
        wrp_free_struct(in_msg);
    }

    /* Return message_size or object size */
    return message_size;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* None */
