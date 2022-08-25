/**
 *  Copyright 2017 Comcast Cable Communications Management, LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <CUnit/Basic.h>

#include "../src/aker_mem.h"
#include "../src/schedule.h"
#include "../src/decode.h"
#include "../src/time.h"
#include "mem_wrapper.h"

#include "tz1.h"
#include "tz2.h"

uint8_t decode_buffer[] = {
0x84, 0xA6, 0x77, 0x65, 0x65, 0x6B, 0x6C, 0x79, 0x93, 0x82, 0xA4, 0x74,
0x69, 0x6D, 0x65, 0x0A, 0xA7, 0x69, 0x6E, 0x64, 0x65, 0x78, 0x65, 0x73,
0x93, 0x00, 0x01, 0x03, 0x82, 0xA4, 0x74, 0x69, 0x6D, 0x65, 0x14, 0xA7,
0x69, 0x6E, 0x64, 0x65, 0x78, 0x65, 0x73, 0x91, 0x00, 0x82, 0xA4, 0x74,
0x69, 0x6D, 0x65, 0x1E, 0xA7, 0x69, 0x6E, 0x64, 0x65, 0x78, 0x65, 0x73,
0xC0, 0xA4, 0x6D, 0x61, 0x63, 0x73, 0x94, 0xB1, 0x31, 0x31, 0x3A, 0x32,
0x32, 0x3A, 0x33, 0x33, 0x3A, 0x34, 0x34, 0x3A, 0x35, 0x35, 0x3A, 0x61,
0x61, 0xB1, 0x32, 0x32, 0x3A, 0x33, 0x33, 0x3A, 0x34, 0x34, 0x3A, 0x35,
0x35, 0x3A, 0x36, 0x36, 0x3A, 0x62, 0x62, 0xB1, 0x33, 0x33, 0x3A, 0x34,
0x34, 0x3A, 0x35, 0x35, 0x3A, 0x36, 0x36, 0x3A, 0x37, 0x37, 0x3A, 0x63,
0x63, 0xB1, 0x34, 0x34, 0x3A, 0x35, 0x35, 0x3A, 0x36, 0x36, 0x3A, 0x37,
0x37, 0x3A, 0x38, 0x38, 0x3A, 0x64, 0x64, 0xA8, 0x61, 0x62, 0x73, 0x6F,
0x6C, 0x75, 0x74, 0x65, 0x91, 0x82, 0xA9, 0x75, 0x6E, 0x69, 0x78, 0x5F,
0x74, 0x69, 0x6D, 0x65, 0xCE, 0x59, 0xE5, 0x83, 0x17, 0xA7, 0x69, 0x6E,
0x64, 0x65, 0x78, 0x65, 0x73, 0x92, 0x00, 0x02, 0xAB, 0x72, 0x65, 0x70,
0x6F, 0x72, 0x74, 0x5F, 0x72, 0x61, 0x74, 0x65, 0xCE, 0x00, 0x01, 0x51,
0x80
};

size_t decode_length = sizeof(decode_buffer);

uint8_t decode_buffer2[] = {
131, 166, 119, 101, 101, 107, 108, 121, 148, 130, 164, 116, 105, 109, 101, 10,
167, 105, 110, 100, 101, 120, 101, 115, 148, 0, 1, 5, 3, 130, 164, 116, 105,
109, 101, 20, 167, 105, 110, 100, 101, 120, 101, 115, 145, 0, 130, 164, 116,
105, 109, 101, 30, 167, 105, 110, 100, 101, 120, 101, 115, 147, 1, 5, 2, 130,
164, 116, 105, 109, 101, 205, 1, 45, 167, 105, 110, 100, 101, 120, 101, 115,
147, 0, 3, 7, 164, 109, 97, 99, 115, 152, 177, 49, 49, 58, 50, 50, 58, 51, 51,
58, 52, 52, 58, 53, 53, 58, 97, 97, 177, 50, 50, 58, 51, 51, 58, 52, 52, 58,
53, 53, 58, 54, 54, 58, 98, 98, 177, 51, 51, 58, 52, 52, 58, 53, 53, 58, 54,
54, 58, 55, 55, 58, 99, 99, 177, 52, 52, 58, 53, 53, 58, 54, 54, 58, 55, 55,
58, 56, 56, 58, 100, 100, 177, 49, 49, 58, 50, 50, 58, 51, 51, 58, 52, 52, 58,
53, 53, 58, 97, 48, 177, 50, 50, 58, 51, 51, 58, 52, 52, 58, 53, 53, 58, 54,
54, 58, 98, 48, 177, 51, 51, 58, 52, 52, 58, 53, 53, 58, 54, 54, 58, 55, 55,
58, 99, 48, 177, 52, 52, 58, 53, 53, 58, 54, 54, 58, 55, 55, 58, 56, 56, 58,
100, 57, 168, 97, 98, 115, 111, 108, 117, 116, 101, 145, 130, 169, 117, 110,
105, 120, 95, 116, 105, 109, 101, 206, 89, 229, 131, 23, 167, 105, 110, 100,
101, 120, 101, 115, 146, 0, 2
};
size_t decode_length2 = sizeof(decode_buffer2);

uint8_t decode_buffer_corrupted [] = {
131, 185, 119, 101, 101, 107, 108, 121, 45, 115, 99, 104, 101, 100, 117, 108,
101, 148, 130, 164, 116, 105, 109, 101, 10, 167, 105, 110, 100, 101, 120, 101,
115, 148, 0, 1, 5, 3, 130, 164, 116, 105, 109, 101, 20, 167, 105, 110, 100, 101,
120, 101, 115, 145, 0, 130, 164, 116, 105, 109, 101, 30, 167, 105, 110, 100, 101,
120, 101, 115, 147, 1, 5, 2, 130, 164, 116, 105, 109, 101, 205, 1, 45, 167, 105,
110, 100, 101, 120, 101, 115, 147, 0, 3, 7, 164, 109, 97, 99, 115, 152, 177, 49,
49, 58, 50, 50, 58, 51, 51, 58, 52, 52, 58, 53, 53, 58, 97, 97, 177, 50, 50, 58,
51, 51, 58, 52, 52, 58, 53, 53, 58, 54, 54, 58, 98, 98, 177, 51, 51, 58, 52, 52,
58, 53, 53, 58, 54, 54, 58, 55, 55, 58, 99, 99, 177, 52, 52, 58, 53, 53, 58, 54,
54, 58, 55, 55, 58, 56, 56, 58, 100, 100, 177, 49, 49, 58, 50, 50, 58, 51, 51,
58, 52, 52, 58, 53, 53, 58, 97, 48, 177, 50, 50, 58, 51, 51, 58, 52, 52, 58, 53,
53, 58, 54, 54, 58, 98, 48, 177, 51, 51, 58, 52, 52, 58, 53, 53, 58, 54, 54, 58,
55, 55, 58, 99, 48, 177, 52, 52, 58, 53, 53, 58, 54, 54, 58, 55, 55, 58, 56, 56,
58, 100, 57, 177, 97, 98, 115, 111, 108, 117, 116, 101, 45, 115, 99, 104, 101,
100, 117, 108, 101, 145, 130, 169, 117, 110, 105, 120, 95, 116, 105, 109, 101,
206, 89, 229, 131, 23, 167, 105, 110, 100, 101, 120, 101, 115, 146, 0, 2
};
size_t buffer_corrupt_length = sizeof(decode_buffer_corrupted);

/* Invalid msgpack */
uint8_t decode_buffer_3[] = {
0x00, 0xAF, 0x77, 0x65,   0x65, 0x6B, 0x6C, 0x79,   0x5F, 0x73, 0x63, 0x68,
0x65, 0x64, 0x75, 0x6C,   0x65, 0x93, 0x82, 0xA4,   0x74, 0x69, 0x6D, 0x65,
0x0A, 0xA7, 0x69, 0x6E,   0x64, 0x65, 0x78, 0x65,   0x73, 0x93, 0x00, 0x01,
0x03, 0x82, 0xA4, 0x74,   0x69, 0x6D, 0x65, 0x14,   0xA7, 0x69, 0x6E, 0x64,
0x65, 0x78, 0x65, 0x73,   0x91, 0x00, 0x82, 0xA4,   0x74, 0x69, 0x6D, 0x65,
0x1E, 0xA7, 0x69, 0x6E,   0x64, 0x65, 0x78, 0x65,   0x73, 0x90, 0xA4, 0x6D,
0x61, 0x63, 0x73, 0x94,   0xB1, 0x31, 0x31, 0x3A,   0x32, 0x32, 0x3A, 0x33,
0x33, 0x3A, 0x34, 0x34,   0x3A, 0x35, 0x35, 0x3A,   0x61, 0x61, 0xB1, 0x32,
0x32, 0x3A, 0x33, 0x33,   0x3A, 0x34, 0x34, 0x3A,   0x35, 0x35, 0x3A, 0x36,
0x36, 0x3A, 0x62, 0x62,   0xB1, 0x33, 0x33, 0x3A,   0x34, 0x34, 0x3A, 0x35,
0x35, 0x3A, 0x36, 0x36,   0x3A, 0x37, 0x37, 0x3A,   0x63, 0x63, 0xB1, 0x34,
0x34, 0x3A, 0x35, 0x35,   0x3A, 0x36, 0x36, 0x3A,   0x37, 0x37, 0x3A, 0x38,
0x38, 0x3A, 0x64, 0x64,   0xB1, 0x61, 0x62, 0x73,   0x6F, 0x6C, 0x75, 0x74,
0x65, 0x5F, 0x73, 0x63,   0x68, 0x65, 0x64, 0x75,   0x6C, 0x65, 0x91, 0x82,
0xA9, 0x75, 0x6E, 0x69,   0x78, 0x5F, 0x74, 0x69,   0x6D, 0x65, 0xCE, 0x59,
0xE5, 0x83, 0x17, 0xA7,   0x69, 0x6E, 0x64, 0x65,   0x78, 0x65, 0x73, 0x92,
0x00, 0x02
};
size_t decode_length_3 = sizeof(decode_buffer_3);

char decode_buffer_4[] = "\x85"
                            "\xa6""weekly"
                                "\x95"
                                    "\x82"
                                        "\xa4""time" /*: */ "\x0a"
                                        "\xa7""indexes"
                                            "\x93" /* [ */ "\x00" "\x01" "\x03" /* ] */
                                    "\x82"
                                        "\xa4""time" /*: */ "\x14"
                                        "\xa7""indexes"
                                            "\x92" /* [ */ "\x00" "\x04" /* ] */
                                    "\x82"
                                        "\xa4""time" /*: */ "\x1e"
                                        "\xa7""indexes" "\xc0"
                                    "\x82"
                                        "\xa4""time" /*: */ "\x28"
                                        "\xa7""indexes"
                                            "\x91" /* [ */ "\x00" /* ] */
                                    "\x82"
                                        "\xa4""time" /*: */ "\x32"
                                        "\xa7""indexes"
                                            "\x93" /* [ */ "\x03" "\x05" "\xcd\x03\x8f" /* ] */
                            "\xa4""macs"
                                "\x9a"
                                    "\xb1""11:22:33:44:55:aa"
                                    "\xb1""22:33:44:55:66:bb"
                                    "\xb1""33:44:55:66:77:cc"
                                    "\xb1""44:55:66:77:88:dd"
                                    "\xb1""55:33:44:55:66:bb"
                                    "\xb1""66:44:55:66:77:cc"
                                    "\xb1""77:55:66:77:88:dd"
                                    "\xb1""88:33:44:55:66:bb"
                                    "\xb1""99:44:55:66:77:cc"
                                    "\xb1""00:55:66:77:88:dd"
                            "\xa8""absolute"
                                "\x91"
                                    "\x82"
                                        "\xa9""unix_time" /* : */ "\xce""\x5a\x0b\x4a\xa8"
                                        "\xa7""indexes"   /* : */ "\x93" /* [ */ "\x00" "\x02" "\x09" /* ] */
                            "\xab""report_rate" /* : */ "\xcd""\x0e\x10"
                            "\xa6""ignore"      /* : */ "\xae""this parameter";
size_t decode_length_4 = sizeof(decode_buffer_4) - 1;

char decode_infinite_loop[] = "\x85"
                            "\xa6""weekly"
                                "\x95"
                                    "\x82"
                                        "\xa4""time" /*: */ "\x0a"
                                        "\xa7""indexes"
                                            "\x93" /* [ */ "\x00" "\x01" "\x03" /* ] */
                                    "\x82"
                                        "\xa4""time" /*: */ "\x14"
                                        "\xa7""indexes"
                                            "\x92" /* [ */ "\x00" "\x04" /* ] */
                                    "\x82"
                                        "\xa4""time" /*: */ "\x1e"
                                        "\xa7""indexes" "\xc0"
                                    "\x82"
                                        "\xa4""time" /*: */ "\x28"
                                        "\xa7""indexes"
                                            "\x91" /* [ */ "\x00" /* ] */
                                    "\x82"
                                        "\xa4""time" /*: */ "\x0a"
                                        "\xa7""indexes"
                                            "\x93" /* [ */ "\x03" "\x05" "\xcd\x03\x8f" /* ] */
                            "\xa4""macs"
                                "\x9a"
                                    "\xb1""11:22:33:44:55:aa"
                                    "\xb1""22:33:44:55:66:bb"
                                    "\xb1""33:44:55:66:77:cc"
                                    "\xb1""44:55:66:77:88:dd"
                                    "\xb1""55:33:44:55:66:bb"
                                    "\xb1""66:44:55:66:77:cc"
                                    "\xb1""77:55:66:77:88:dd"
                                    "\xb1""88:33:44:55:66:bb"
                                    "\xb1""99:44:55:66:77:cc"
                                    "\xb1""00:55:66:77:88:dd"
                            "\xa8""absolute"
                                "\x91"
                                    "\x82"
                                        "\xa9""unix_time" /* : */ "\xce""\x5a\x0b\x4a\xa8"
                                        "\xa7""indexes"   /* : */ "\x93" /* [ */ "\x00" "\x02" "\x09" /* ] */
                            "\xab""report_rate" /* : */ "\xcd""\x0e\x10"
                            "\xa6""ignore"      /* : */ "\xae""this parameter";
size_t decode_length_infinite_loop = sizeof(decode_infinite_loop) - 1;

unsigned char scheduler_data4_bin[] = {
  0x85, 0xa6, 0x77, 0x65, 0x65, 0x6b, 0x6c, 0x79, 0x98, 0x82, 0xa4, 0x74,
  0x69, 0x6d, 0x65, 0xce, 0x00, 0x01, 0x51, 0x80, 0xa7, 0x69, 0x6e, 0x64,
  0x65, 0x78, 0x65, 0x73, 0x92, 0x00, 0x01, 0x82, 0xa4, 0x74, 0x69, 0x6d,
  0x65, 0xce, 0x00, 0x01, 0xc2, 0x00, 0xa7, 0x69, 0x6e, 0x64, 0x65, 0x78,
  0x65, 0x73, 0x90, 0x82, 0xa4, 0x74, 0x69, 0x6d, 0x65, 0xce, 0x00, 0x02,
  0xa3, 0x00, 0xa7, 0x69, 0x6e, 0x64, 0x65, 0x78, 0x65, 0x73, 0x92, 0x00,
  0x01, 0x82, 0xa4, 0x74, 0x69, 0x6d, 0x65, 0xce, 0x00, 0x03, 0x13, 0x80,
  0xa7, 0x69, 0x6e, 0x64, 0x65, 0x78, 0x65, 0x73, 0x90, 0x82, 0xa4, 0x74,
  0x69, 0x6d, 0x65, 0xce, 0x00, 0x03, 0xf4, 0x80, 0xa7, 0x69, 0x6e, 0x64,
  0x65, 0x78, 0x65, 0x73, 0x92, 0x00, 0x01, 0x82, 0xa4, 0x74, 0x69, 0x6d,
  0x65, 0xce, 0x00, 0x04, 0x65, 0x00, 0xa7, 0x69, 0x6e, 0x64, 0x65, 0x78,
  0x65, 0x73, 0x90, 0x82, 0xa4, 0x74, 0x69, 0x6d, 0x65, 0xce, 0x00, 0x05,
  0x46, 0x00, 0xa7, 0x69, 0x6e, 0x64, 0x65, 0x78, 0x65, 0x73, 0x92, 0x00,
  0x01, 0x82, 0xa4, 0x74, 0x69, 0x6d, 0x65, 0xce, 0x00, 0x07, 0x4d, 0x24,
  0xa7, 0x69, 0x6e, 0x64, 0x65, 0x78, 0x65, 0x73, 0x90, 0xa4, 0x6d, 0x61,
  0x63, 0x73, 0x92, 0xb1, 0x64, 0x36, 0x3a, 0x30, 0x62, 0x3a, 0x36, 0x38,
  0x3a, 0x34, 0x66, 0x3a, 0x31, 0x35, 0x3a, 0x61, 0x30, 0xb1, 0x63, 0x34,
  0x3a, 0x38, 0x34, 0x3a, 0x36, 0x36, 0x3a, 0x32, 0x39, 0x3a, 0x30, 0x66,
  0x3a, 0x39, 0x64, 0xa9, 0x74, 0x69, 0x6d, 0x65, 0x5f, 0x7a, 0x6f, 0x6e,
  0x65, 0xa7, 0x50, 0x53, 0x54, 0x38, 0x50, 0x44, 0x54, 0xa8, 0x61, 0x62,
  0x73, 0x6f, 0x6c, 0x75, 0x74, 0x65, 0x90, 0xab, 0x72, 0x65, 0x70, 0x6f,
  0x72, 0x74, 0x5f, 0x72, 0x61, 0x74, 0x65, 0x78
};
#define scheduler_data4_bin_len 260

#include "test1.h" /* Empty Arrays */
#include "test2.h" /* NULL Index */
#include "test3.h" /* Missing Arrays */
#include "test4.h" /* Missing Everything i.e. "{ }" */
#include "unknown.h" /* name:value pairs that are unknown to decode */

void test_schedule(schedule_t *s)
{
    /* December 31, 2017 from 12:00:01 AM to 12:05 AM*/
    time_t start_unix = 1514707200;  /* 12:00:00 AM */
    time_t end_unix   = 1514707500;  /* 12:05:00 AM - 5 minutes later */
    time_t t;

    CU_ASSERT(86400 == s->report_rate_s);

    set_unix_time_zone( "PST8PDT" );

    for( t = start_unix; t < end_unix; t++ ) {
        char *macs = NULL;
        time_t next;

        macs = get_blocked_at_time( s, t );
        next = get_next_unixtime( s, t );
        if( t < 1514707210 ) {
            CU_ASSERT( NULL == macs );
            CU_ASSERT( 1514707210 == next );
        }
        if( (1514707210 <= t) && (t < 1514707220) ) {
            CU_ASSERT_STRING_EQUAL(macs, "11:22:33:44:55:aa 22:33:44:55:66:bb 44:55:66:77:88:dd");
            CU_ASSERT( 1514707220 == next );
        }
        if( (1514707220 <= t) && (t < 1514707230) ) {
            CU_ASSERT_STRING_EQUAL(macs, "11:22:33:44:55:aa");
            CU_ASSERT( 1514707230 == next );
        }
        if( 1514707230 <= t ) {
            CU_ASSERT( NULL == macs );
            // Next Sunday at 12:00:10 AM.
            CU_ASSERT( 1515312010 == next );
        }

        if( NULL != macs ) {
            aker_free( macs );
            macs = NULL;
        }

    }
}

void test_schedule_2(schedule_t *s)
{
    /* December 31, 2017 from 12:00:01 AM to 12:05 AM*/
    time_t start_unix = 1514707200;  /* 12:00:01 AM */
    time_t end_unix   = 1514707500;  /* 12:05:00 AM - 5 minutes later */
    time_t t;

    set_unix_time_zone( "PST8PDT" );

    for( t = start_unix; t < end_unix; t++ ) {
        char *macs = NULL;
        time_t next;

        macs = get_blocked_at_time( s, t );
        next = get_next_unixtime( s, t );
        if( t < 1514707210 ) {
            CU_ASSERT_STRING_EQUAL(macs, "11:22:33:44:55:aa 44:55:66:77:88:dd 44:55:66:77:88:d9");
            CU_ASSERT( 1514707210 == next );
        }
        if( (1514707210 <= t) && (t < 1514707220) ) {
            CU_ASSERT_STRING_EQUAL(macs, "11:22:33:44:55:aa 22:33:44:55:66:bb 22:33:44:55:66:b0 44:55:66:77:88:dd");
            CU_ASSERT( 1514707220 == next );
        }
        if( (1514707220 <= t) && (t < 1514707230) ) {
            CU_ASSERT_STRING_EQUAL(macs, "11:22:33:44:55:aa");
            CU_ASSERT( 1514707230 == next );
        }
        if( (1514707230 <= t) && (t < 1514707501) ) {
            CU_ASSERT_STRING_EQUAL(macs, "22:33:44:55:66:bb 22:33:44:55:66:b0 33:44:55:66:77:cc");
            CU_ASSERT( 1514707501 == next );
        }
        if( 1514707501 <= t ) {
            CU_ASSERT_STRING_EQUAL(macs, "11:22:33:44:55:aa 44:55:66:77:88:dd 44:55:66:77:88:d9");
            // Next Sunday at 12:00:10 AM.
            CU_ASSERT( 1515312010 == next );
        }

        if( NULL != macs ) {
            aker_free( macs );
            macs = NULL;
        }

    }
}

void decode_test()
{
    schedule_t *t;
    int ret = decode_schedule(decode_length, decode_buffer, &t);
    CU_ASSERT(0 == ret);
    print_schedule(t);
    test_schedule(t);
    destroy_schedule(t);

    ret = decode_schedule(decode_length2, decode_buffer2, &t);
    CU_ASSERT(0 == ret);
    print_schedule(t);
    test_schedule_2(t);
    destroy_schedule(t);

    t = NULL;
    ret = decode_schedule(buffer_corrupt_length, decode_buffer_corrupted, &t);
    CU_ASSERT(0 != ret);
    ret = decode_schedule(decode_length_3, decode_buffer_3, &t);
    CU_ASSERT(0 != ret);
    ret = decode_schedule(0, decode_buffer, &t);
    CU_ASSERT(0 != ret);
    ret = decode_schedule(unknown_bin_len, unknown_bin, &t);
    CU_ASSERT(0 == ret);
    destroy_schedule(t);
    t = NULL;
    ret = decode_schedule(tz2_bin_len, tz2_bin, &t);
    CU_ASSERT(0 == ret);
    destroy_schedule(t);
    ret = decode_schedule(tz1_bin_len, tz1_bin, &t);
    CU_ASSERT(0 == ret);
    destroy_schedule(t);

    t = NULL;
    ret = decode_schedule(test1_bin_len, test1_bin, &t);
    CU_ASSERT(0 != ret);
    CU_ASSERT(NULL == t);

    ret = decode_schedule(test2_bin_len, test2_bin, &t);
    CU_ASSERT(0 != ret);
    CU_ASSERT(NULL == t);

    ret = decode_schedule(test3_bin_len, test3_bin, &t);
    CU_ASSERT(0 != ret);
    CU_ASSERT(NULL == t);

    ret = decode_schedule(test4_bin_len, test4_bin, &t);
    CU_ASSERT(0 != ret);
    CU_ASSERT(NULL == t);
}

void decode_test_with_extras()
{
    schedule_t *t;
    int ret;

    t = NULL;
    ret = decode_schedule(decode_length_4, (uint8_t*)decode_buffer_4, &t);
    CU_ASSERT(0 == ret);
    CU_ASSERT(NULL != t);
    destroy_schedule(t);
}

void decode_test_with_duplicate_time()
{
    schedule_t *t;
    int ret;

    t = NULL;
    ret = decode_schedule(decode_length_infinite_loop, (uint8_t*)decode_infinite_loop, &t);
    CU_ASSERT(0 != ret);
    CU_ASSERT(NULL == t);
}

void decode_test_with_empty_list()
{
    schedule_t *t;
    int ret;

    t = NULL;
    ret = decode_schedule(scheduler_data4_bin_len, (uint8_t*)scheduler_data4_bin, &t);
    CU_ASSERT(0 == ret);
    CU_ASSERT_FATAL(NULL != t);
    destroy_schedule(t);
}

void add_suites( CU_pSuite *suite )
{
    printf("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Decode Test", decode_test);
    CU_add_test( *suite, "Decode Test With Extra Item", decode_test_with_extras);
    CU_add_test( *suite, "Decode Test With Duplicate Time", decode_test_with_duplicate_time);
    CU_add_test( *suite, "Decode Test With Unknown Failure", decode_test_with_empty_list);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main( void )
{
    unsigned rv = 1;
    CU_pSuite suite = NULL;

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

int32_t get_max_mac_limit(void)
{
    return 2048;
}

void set_gmtoff(long int timezoneoff)
{
    (void)timezoneoff;
    return;
}
