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
#ifndef __AKER_MD5_H__
#define __AKER_MD5_H__



#ifdef __cplusplus
extern "C" {
#endif
#define MD5_SIZE 16    // 4 uint32_t passed as a char *
extern int compute_file_md5(const char *filename, unsigned char result[MD5_SIZE]); 

extern int compute_byte_stream_md5(uint8_t *data, size_t length,
                                   unsigned char result[MD5_SIZE]); 

#ifdef __cplusplus
}
#endif


#endif

