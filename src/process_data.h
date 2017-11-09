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
#ifndef _PROCESS_DATA_H_
#define _PROCESS_DATA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <wrp-c/wrp-c.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/**
 * @brief Processes wrp CRUD message for Create and Update.
 *
 * @param filename to write payload into
 * @param md5_file
 * @param cu       CRUD message
 *
 * @return size of payload, <0 otherwise.
 */
ssize_t process_message_cu( const char *filename, const char *md5_file, wrp_msg_t *cu );

/**
 * @brief Returns data through the wrp CRUD message.
 * 
 * @note return data buffer needs to be free()-ed by caller.
 *
 * @param filename to read payload from
 * @param ret CRUD message
 *
 * @return size of data retrieved, <0 otherwise.
 */
ssize_t process_message_ret( const char *filename, wrp_msg_t *ret );

/**
 * @brief reads the file.
 * 
 * @note Locks/UnLocks the mutex for acccess.
 *
 * @param pointer to be allocated by read_file_from_disk()
 *
 * @return size of the file 
 * 
 */
size_t read_file_from_disk( const char *filename, uint8_t **data );

#ifdef __cplusplus
}
#endif

#endif

