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


unsigned char *compute_file_md5(const char *filename, unsigned char *md5_sig)
{
    size_t size;
    uint8_t *data;
    unsigned char *md5_string = NULL;
    
    size = read_file_from_disk(filename, &data);
    if (size > 0) {
        md5_string = compute_byte_stream_md5(data, size, md5_sig);
    }
    
    if (NULL != data) {
        free(data);
    }
    
    return md5_string;
}

unsigned char * compute_byte_stream_md5(uint8_t *data, size_t size, unsigned char *md5_sig)
{
    MD5_CTX ctx;
    int cnt;
    unsigned char *md5_string = NULL;
    
    MD5_Init(&ctx);
    MD5_Update(&ctx, (const void *) data, (unsigned long) size);
    MD5_Final(md5_sig, &ctx);
    md5_string = (unsigned char *) malloc(sizeof(unsigned char) * (MD5_SIZE * 2) + 1);
    if (NULL == md5_string) {
        debug_error("compute_byte_stream_md5()->malloc() failed\n");
        return md5_string;
    }
    debug_info("MD5 sig: ");
    for (cnt = 0; cnt < MD5_SIZE; cnt++) {
        sprintf((char *) &md5_string[cnt * 2], "%02x", md5_sig[cnt]);
    }
    md5_string[MD5_SIZE * 2] = 0;
    debug_info("%s\n", md5_string);    
    
    return md5_string;
}


int verify_md5_signatures(const char *data_file, const char *md5_file)
{
    unsigned char data_sig[MD5_SIZE];
    unsigned char *md5_sig = NULL;
    int ret_val = 0;
    unsigned char *data_md5;

    memset(data_sig, 0, MD5_SIZE);
    if ( NULL == (data_md5 = compute_file_md5(data_file, data_sig)))
    {
        printf("verify_md5_signatures() Error computing md5 for %s\n",
                    data_file);
        return -1;
    }

    if (0 < read_file_from_disk( md5_file, &md5_sig ))
    {
        if (0 != memcmp(md5_sig, data_md5, MD5_SIZE * 2)) {
            printf("verify_md5_signatures sig mismatch\n");
            ret_val = -2;
        }
        else {
            printf("verify_md5_signatures() data and md5 signature verified\n");
        }
    } else {
        printf("verify_md5_signatures failed to read md5_file\n");
        ret_val = -3;
    }

    if (md5_sig) {
        free(md5_sig);
    }

    if (data_md5) {
        free(data_md5);
    }
    
    return ret_val;
}

