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
    ssize_t rv = -1;

    memset(response, 0, sizeof(wrp_msg_t));
    switch (in_msg->msg_type) {
        case (WRP_MSG_TYPE__CREATE):
        case (WRP_MSG_TYPE__UPDATE):
        {
            crud_msg_t *in_crud  = &(in_msg->u.crud);
            crud_msg_t *out_crud = &(response->u.crud);

            out_crud->status = 400; // default to failed
            /* Not as per WRP spec - Response to Update */
            response->msg_type = in_msg->msg_type;

            out_crud->transaction_uuid = in_crud->transaction_uuid;
            out_crud->source  = in_crud->dest;
            out_crud->dest    = in_crud->source;
            out_crud->path    = in_crud->path;
            if( 0 == strcmp("/parental-control/schedule", in_crud->dest) ) {
                rv = process_message_cu(data_file, md5_file, in_msg);
            } else {
                debug_error("CREATE/UPDATE message destination %s is invalid\n", in_crud->dest);
            }
            if( 0 < rv ) {
                out_crud->status = 200;
            }
        }
        break;

        case (WRP_MSG_TYPE__RETREIVE):
        {
            crud_msg_t *in_crud  = &(in_msg->u.crud);
            crud_msg_t *out_crud = &(response->u.crud);

            out_crud->status = 400; // default to failed
            response->msg_type = WRP_MSG_TYPE__RETREIVE;

            out_crud->transaction_uuid = in_crud->transaction_uuid;
            out_crud->source  = in_crud->dest;
            out_crud->dest    = in_crud->source;
            out_crud->path    = in_crud->path;
            if( (0 == strcmp("/parental-control/schedule", in_crud->dest)) ||
                (0 == strcmp("/parental-control/md5",      in_crud->dest)) )
            {
                rv = process_message_ret(data_file, response);
            } else {
                debug_error("RETRIEVE message destination %s is invalid\n", in_crud->dest);
            }
            if( 0 < rv ) {
                out_crud->status = 200;
            }
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
            free(msg->payload);
        rv = 0;
    }

    return rv;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* None */
