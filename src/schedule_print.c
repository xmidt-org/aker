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
#include <stdio.h>
#include <string.h>

#include "schedule.h"
#include "aker_log.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See schedule.h for details. */
void print_schedule( schedule_t *s )
{
    size_t i;
    schedule_event_t *p;

    if( NULL == s ) {
        debug_info( "schedule {}\n" );
        return;
    }

    debug_info( "schedule {\n" );

    debug_info( "   s->time_zone: %s\n", ((NULL == s->time_zone) ? "NULL" : s->time_zone));

    debug_info( "   s->mac_count: %zd\n", s->mac_count );
    for( i = 0; i < s->mac_count; i++ ) {
        debug_info( "       [%zd]: '%s'\n", i, (char*) &s->macs[i].mac[0] );
    }

    p = s->absolute;
    debug_info( "   s->absolute:\n" );
    if( NULL == p ) {
        debug_info( "       NULL\n" );
    }
    while( NULL != p ) {
        char block_list[128] = {0}, *comma = "", *t;
        for( i = 0, t = block_list; i < p->block_count; i++ ) {
            sprintf( t, "%s%3d", comma, p->block[i] );
            t += (3 + strlen(comma));
            comma = ", ";
        }
        debug_info( "       time: %ld, block_count: %zd [ %s ]\n", p->time, p->block_count, block_list );
        p = p->next;
    }

    p = s->weekly;
    debug_info( "   s->weekly:\n" );
    if( NULL == p ) {
        debug_info( "       NULL\n" );
    }
    while( NULL != p ) {
        char block_list[128] = {0}, *comma = "", *t;
        for( i = 0, t = block_list; i < p->block_count; i++ ) {
            sprintf( t, "%s%3d", comma, p->block[i] );
            t += (3 + strlen(comma));
            comma = ", ";
        }
        debug_info( "       time: %ld, block_count: %zd [ %s ]\n", p->time, p->block_count, block_list );
        p = p->next;
    }
    debug_info( "}\n" );
}


/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
