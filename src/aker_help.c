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
    /* it is your problem if you called with null for name ;-) */
    char *command = strrchr(name, '/');
    if (NULL == command) {
        command = name;
    } else {
        command++; //  = &command[1]; skip '/'
    }

    if (NULL == opt) {
        print_general_help(command);
    } else {
        printf("%s %s To get help on specific topic use --h=<topic> or --help=<topic>\n", command, opt);
    }
}
                
void print_general_help(char *command)
{
    debug_info("Usage:%s %s %s %s %s %s %s %s\n", command,
            "-p <parodus_url>", "-c <client_url>", "-w <firewall_cmd>",
            "-d <data_file>", "-f <md5_sig_file>", "[-m <maximum_allowed_macs>]",
            "[-h }, [--h=[<topic>]]");
}