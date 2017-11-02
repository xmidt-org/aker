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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>

#include "md5.h"
#include "aker_md5.h"
#include "aker_log.h"
#include "process_data.h"


int compute_file_md5(const char *filename, unsigned char *md5_sig)
{
    size_t size;
    uint8_t *data;
    int ret_val = -1;
    
    size = read_file_from_disk(filename, &data);
    if (size > 0) {
        compute_byte_stream_md5(data, size, md5_sig);
        ret_val = 0;
    }
    
    if (NULL != data) {
        free(data);
    }
    
    return ret_val;
}

int compute_byte_stream_md5(uint8_t *data, size_t size, unsigned char *md5_sig)
{
    MD5_CTX ctx;
    int cnt;
    char debug_buffer[32+1];
    
    MD5_Init(&ctx);
    MD5_Update(&ctx, (const void *) data, (unsigned long) size);
    MD5_Final(md5_sig, &ctx);
    debug_info("MD5 sig: ");
    for (cnt = 0; cnt < 16; cnt++) {
        sprintf(&debug_buffer[cnt * 2], "%02X", md5_sig[cnt]);
    }
    debug_buffer[32] = 0;
    debug_info("%s\n", debug_buffer);    
    
    return 0;
}
