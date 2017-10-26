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
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>


#include "schedule.h"
#include "time.h"
#include "process_data.h"
#include "aker_log.h"
#include "scheduler.h"
#include "decode.h"


static void sig_handler(int sig);
void process_schedule_data(size_t len, uint8_t *data);
static schedule_t *current_schedule = NULL;

void *scheduler_thread(void *args)
{
    int32_t file_version = get_schedule_file_version();
 
    (void ) args;
    (void ) file_version;
    
    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);
    signal(SIGSEGV, sig_handler);
    signal(SIGBUS, sig_handler);
    signal(SIGKILL, sig_handler);
    signal(SIGFPE, sig_handler);
    signal(SIGILL, sig_handler);
    signal(SIGQUIT, sig_handler);
    signal(SIGHUP, sig_handler);
    signal(SIGALRM, sig_handler);    
  
    while (1) {
        int32_t new_file_version = get_schedule_file_version();
        uint8_t *data;
        if (new_file_version >= 0 && (new_file_version != file_version)) {
            size_t data_size;
            file_version = new_file_version;
            data_size = read_file_from_disk(&data);
            if (data) {
                process_schedule_data(data_size, data);
                free(data);
            }
        }
 /*
  TODO: See if any blocking needs to be done or changed ...
  */
        
        sleep(5);
    }
    
    
    
    return NULL;    
}


/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
void process_schedule_data(size_t len, uint8_t *data) 
{
    if (NULL != current_schedule) {
        destroy_schedule(current_schedule);
    }
    
    if (0 != decode_schedule(len, data, &current_schedule)) {
         destroy_schedule(current_schedule);
         current_schedule =NULL;
         debug_error("process_schedule_data() Failed to decode\n");
         return;
    }
    /*
     TODO: Now what? 
     */
}

static void sig_handler(int sig)
{
    if( sig == SIGINT ) {
        signal(SIGINT, sig_handler); /* reset it to this function */
        debug_info("SIGINT received!\n");
        exit(0);
    } else if( sig == SIGUSR1 ) {
        signal(SIGUSR1, sig_handler); /* reset it to this function */
        debug_info("SIGUSR1 received!\n");
    } else if( sig == SIGUSR2 ) {
        signal(SIGUSR2, sig_handler);
        debug_info("SIGUSR2 received!\n");
    } else if( sig == SIGCHLD ) {
        signal(SIGCHLD, sig_handler); /* reset it to this function */
        debug_info("SIGHLD received!\n");
    } else if( sig == SIGPIPE ) {
        signal(SIGPIPE, sig_handler); /* reset it to this function */
        debug_info("SIGPIPE received!\n");
    } else if( sig == SIGALRM ) {
        signal(SIGALRM, sig_handler); /* reset it to this function */
        debug_info("SIGALRM received!\n");
    } else {
        debug_info("Signal %d received!\n", sig);
        exit(0);
    }
}

