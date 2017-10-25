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
/*** Temporary until CRUD WRP messages can support binary payload ***/
/*
#define SET_DEST  "/parental control/schedule/set"
#define GET_DEST  "/parental control/schedule/get"
*/
#define REQ_DEST  "/iot"
#define REQ_GET   "\"command\":\"GET\""

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
int wrp_process(wrp_msg_t *msg, wrp_msg_t *response)
{
    wrp_msg_t *in_msg = msg;

    switch (in_msg->msg_type) {
        case (WRP_MSG_TYPE__CREATE): 
        case (WRP_MSG_TYPE__UPDATE):
        {
            crud_msg_t *out_crud = &(response->u.crud);

            out_crud->status = 400; // default to failed
            /* Not as per WRP spec - Response to Update */
            response->msg_type = in_msg->msg_type;

            if( 0 == process_message_cu(in_msg, response) ) {
                out_crud->status =200;
            }
        }
        break;
        
        case (WRP_MSG_TYPE__RETREIVE):
        {
            crud_msg_t *in_crud  = &(in_msg->u.crud);
            crud_msg_t *out_crud = &(response->u.crud);

            out_crud->status = 400; // default to failed
            response->msg_type = WRP_MSG_TYPE__RETREIVE;

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
            req_msg_t *resp = &(response->u.req);

            response->msg_type = WRP_MSG_TYPE__REQ;
            
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
            if( NULL != strstr(req->dest, REQ_DEST) ) {
                if( (NULL != req->payload) &&
                    (NULL != strstr(req->payload, REQ_GET)) )
                {
                    process_request_get(response);
                }
                else {
                    process_request_set(in_msg);
                }
            }
            else {
                 debug_error("Request-Response message destination %s is invalid\n", req->dest);
            }
        }
        break;
            
        default:
            debug_info("Message of type %d not handled\n", in_msg->msg_type);
        break;
    }

    return 0;
}

int wrp_cleanup(wrp_msg_t *message)
{
    int rv = -1;

    if( WRP_MSG_TYPE__REQ == message->msg_type ) {
        req_msg_t *msg = &(message->u.req);
        if( msg->transaction_uuid )
            free(msg->transaction_uuid);
        if( msg->source )          
            free(msg->source);
        if( msg->dest )             
            free(msg->dest);
        if( msg->payload )         
            free(msg->payload);
        rv = 0;
    }

    return rv;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* None */
