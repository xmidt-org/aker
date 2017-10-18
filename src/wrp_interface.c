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

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct wrp_crud_msg crud_msg_t;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static int process_message_cu( crud_msg_t *msg, uint8_t **object );
static uint8_t *process_message_ret( crud_msg_t *msg );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
ssize_t wrp_to_object(wrp_msg_t *msg, uint8_t **object)
{
    wrp_msg_t *in_msg = msg;
    wrp_msg_t response;
    /* ssize_t   message_size; */

    memset(&response, 0, sizeof(wrp_msg_t));

    switch (in_msg->msg_type) {
        case (WRP_MSG_TYPE__CREATE): 
        case (WRP_MSG_TYPE__UPDATE):
        {
            crud_msg_t *in_crud  = &(in_msg->u.crud);
            crud_msg_t *out_crud = &(response.u.crud);

            out_crud->status = 400; // default to failed
            /* Not as per WRP spec - Response to Update */
            response.msg_type = in_msg->msg_type;

            if( 0 == process_message_cu(in_crud, object) ) {
                out_crud->status =200;
            }
        }
        break;
        
        case (WRP_MSG_TYPE__RETREIVE):
        {
            crud_msg_t *in_crud  = &(in_msg->u.crud);
            crud_msg_t *out_crud = &(response.u.crud);

            out_crud->status = 400; // default to failed
            response.msg_type = in_msg->msg_type;

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
            // out_crud->payload = process_message_ret(in_crud);
            process_message_ret(in_crud); 
            if( NULL != out_crud->payload ) {
                out_crud->status = 200;
            }
        }
        break;
            
        default:
            debug_info("Message of type %d not handled\n", in_msg->msg_type);
        break;
    }

    /* TODO: Handle response */
/*
    message_size = wrp_struct_to(&response, WRP_BYTES, message);    
    if( WRP_MSG_TYPE__RETREIVE == response.msg_type ) {
        ret_msg_t *ret = &(response.u.crud);
        if (ret->transaction_uuid)
            free(ret->transaction_uuid);
        if (ret->dest)             
            free(ret->dest);
        if (ret->path)             
            free(ret->path);
        if (ret->payload )         
            free(ret->payload);
        if (ret->source)           
            free(ret->source);
    }
 */    
    if (in_msg) {
        wrp_free_struct(in_msg);
    }

    /* Return message_size or object size */
    return 0;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/**
 * @brief Processes wrp CRUD message for Create and Update.
 *
 * @param[in]  wrp CRUD message.
 * @param[out] msgpack object
 */
int process_message_cu( crud_msg_t *msg, uint8_t **object )
{
    if( 0 != strcmp("/parental control/schedule", msg->dest) ) {
        return -1;
    }

    /* Process msg */
    
    return 0;
}

/**
 * @brief Returns a JSON object for the service the wrp CRUD message.
 * 
 * @note return JSON object buffer needs to be free()-ed by caller.
 *
 * @param[in] wrp CRUD message.
 *
 * @return buffer of the object retrieved.
 */
uint8_t *process_message_ret( crud_msg_t *msg )
{
    if( 0 != strcmp("/parental control/schedule", msg->dest) &&
        0 != strcmp("/parental control/md5", msg->dest) ) 
    {
        return NULL;
    }

    /* Retrieve schedule info and return. */

    return NULL;
}
