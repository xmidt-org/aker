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
#include "aker_log.h"

static void print_general_help(char *name);

void aker_help(char *name, char *opt)
{
    if (NULL == opt) {
        print_general_help(name);
    } else {
        printf("%s %s has no help yet ;-)\n", name, opt);
    }
}
                
void print_general_help(char *name)
{
    debug_info("Usage:%s %s %s %s %s %s %s %s\n", name,
            "-p <parodus_url>", "-c <client_url>", "-w <firewall_cmd>",
            "-d <data_file>", "-f <md5_sig_file>", "[-m <maximum_allowed_macs>]",
            "[-h [<topic>]]");
}