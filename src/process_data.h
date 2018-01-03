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

#include <stdint.h>
#include <stdlib.h>

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
 * @brief Returns if it is ok to create the file or not.
 *
 * @param filename the name of the data file to check for
 *
 * @return 0 if ok, otherwise it is not ok
 */
int process_is_create_ok( const char *filename );

/**
 * @brief Processes wrp CRUD message for Update.
 *
 * @param filename     to write data payload into
 * @param md5_file     to write the MD5 checksum into
 * @param payload      the data to consume
 * @param payload_size the length of the data in bytes
 *
 * @return 0 if successful, error otherwise
 */
int process_update( const char *filename, const char *md5_file,
                    void *payload, size_t payload_size );

/**
 * @brief Returns list of the currently blocked MAC IDs through the wrp CRUD message.
 * 
 * @note return data buffer needs to be free()-ed by caller.
 *
 * @param msg CRUD message
 *
 * @return size of data retrieved, <0 otherwise.
 */
size_t process_retrieve_now( uint8_t **data );

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

/**
 * @brief Deletes the files during the delete operation.
 *
 * @param filename the data file to delete
 * @param md5_file the md5 file to delete
 *
 * @return 0 if successful, error otherwise
 */
int process_delete( const char *filename, const char *md5_file );

#ifdef __cplusplus
}
#endif

#endif

