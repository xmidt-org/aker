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
#ifndef _WRP_INTERFACE_H_
#define _WRP_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif
    
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <wrp-c/wrp-c.h>
    
/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define SCHEDULE_ENDPOINT "/aker/schedule"
#define MD5_ENDPOINT      "/aker/md5"
#define NOW_ENDPOINT      "/aker/now"

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/**
 *  Process an incoming message.
 *
 *  @note The response WRP message needs to be cleaned up by the caller.
 *
 *  @param data_file [in] the data file name
 *  @param md5_file  [in] the md5 file name
 *  @param msg       [in]  incoming WRP data.
 *  @param response  [in]  response WRP message.
 *
 *  @return 0 if success, < 0 otherwise.
 */
int wrp_process(const char *data_file, const char *md5_file,
                wrp_msg_t *msg, wrp_msg_t *response);

/**
 *  Cleanup WRP response returned by wrp_processing.
 *
 *  @param msg      [in]  WRP response structure 
 *
 *  @return 0 if success, < 0 otherwise.
 */
int wrp_cleanup(wrp_msg_t *response);

#ifdef __cplusplus
}
#endif

#endif
