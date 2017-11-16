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
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>

#include <CUnit/Basic.h>


#include "../src/md5.h"
#include "../src/aker_md5.h"
#include "../src/process_data.h"
#include "../src/aker_mem.h"

#define MD5_SIG_FILE "md5_sig.out"

static char *test_file_name;
static unsigned char md5_sig_1[MD5_SIZE];
static unsigned char md5_sig_2[MD5_SIZE];


void md5_test1()
{
    unsigned char *result;
    
    result = compute_file_md5(test_file_name, &md5_sig_1[0]);
    
    CU_ASSERT(NULL != result);
    
    if (result) {
        aker_free(result);
    }

    result = compute_file_md5("_no_exist_", &md5_sig_1[0]);
    CU_ASSERT(NULL == result);

}

void md5_test2()
{
    char buffer[256];
    size_t size;
    uint8_t *data;
    int cnt;

    
    sprintf(buffer, "md5sum -b %s > %s", test_file_name, MD5_SIG_FILE);
    if (0 == system(buffer)) {
     size = read_file_from_disk(MD5_SIG_FILE, &data);
     CU_ASSERT(size > 0);
     memset(buffer, 0, 256);
     
     for (cnt = 0; cnt < MD5_SIZE; cnt++) {
        sprintf(&buffer[cnt * 2], "%02x", md5_sig_1[cnt]);
     }    
    //  printf("\nmd5_sig1 %s\n", buffer);
    //  printf("\nmd5sum   %s\n", data);
     
     CU_ASSERT(0 == strncmp(buffer, (char *) data, MD5_SIZE << 1));
      
     aker_free(data);

     cnt = verify_md5_signatures("_no_exist_", MD5_SIG_FILE);
     CU_ASSERT(0 != cnt);

     cnt = verify_md5_signatures(test_file_name, "_no_exist_");
     CU_ASSERT(0 != cnt);

    }
}

void md5_test3()
{
    size_t size;
    uint8_t *data;

    CU_ASSERT(0 == verify_md5_signatures(test_file_name, MD5_SIG_FILE));

    size = read_file_from_disk(MD5_SIG_FILE, &data);
    data[0] = ~data[0];
    FILE *bad_file = fopen(MD5_SIG_FILE, "w");
    fwrite(data, sizeof(uint8_t), size, bad_file);
    fclose(bad_file);
    aker_free(data);

    CU_ASSERT(0 != verify_md5_signatures(test_file_name, MD5_SIG_FILE));

}

void md5_test4()
{
    size_t size;
    uint8_t *data;
    unsigned char *result;
    
    size = read_file_from_disk(test_file_name, &data);
    if (size > 0) {
        result = compute_byte_stream_md5(data, size, &md5_sig_2[0]);
        CU_ASSERT(NULL != result);
        if (result) {
            aker_free(result);
        }
        CU_ASSERT(0 == memcmp(md5_sig_1, md5_sig_2, MD5_SIZE));
        data[size >> 1] = ~data[size >> 1];

        result = compute_byte_stream_md5(data, size, &md5_sig_2[0]);
        CU_ASSERT(NULL != result);
        if (result) {
            aker_free(result);
        }
        CU_ASSERT(0 != memcmp(md5_sig_1, md5_sig_2, MD5_SIZE));
    }

    if (NULL != data) {
        aker_free(data);
    }
    
}

void add_suites( CU_pSuite *suite )
{
    printf("--------Start of Test Cases Execution For MD5---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "MD5 Test 1", md5_test1);
    CU_add_test( *suite, "MD5 Test 2", md5_test2);
    CU_add_test( *suite, "MD5 Test 3", md5_test3);
    CU_add_test( *suite, "MD5 Test 4", md5_test4);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main( int argc, char *argv[] )
{
    unsigned rv = 1;
    CU_pSuite suite = NULL;
 
    (void ) argc;
    test_file_name = argv[0];
    
    if( CUE_SUCCESS == CU_initialize_registry() ) {
        add_suites( &suite );

        if( NULL != suite ) {
            CU_basic_set_mode( CU_BRM_VERBOSE );
            CU_basic_run_tests();
            printf( "\n" );
            CU_basic_show_failures( CU_get_failure_list() );
            printf( "\n\n" );
            rv = CU_get_number_of_tests_failed();
        }

        CU_cleanup_registry();

    }

    return rv;
}

