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

#include "schedule.h"

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
        printf( "schedule {}\n" );
        return;
    }

    printf( "schedule {\n" );

    printf( "   s->time_zone: %s\n", ((NULL == s->time_zone) ? "NULL" : s->time_zone));
    printf( "   s->report_rate: %u\n", s->report_rate_s);

    printf( "   s->mac_count: %zd\n", s->mac_count );
    for( i = 0; i < s->mac_count; i++ ) {
        printf( "       [%zd]: '%s'\n", i, (char*) &s->macs[i].mac[0] );
    }

    p = s->absolute;
    printf( "   s->absolute:\n" );
    if( NULL == p ) {
    printf( "       NULL\n" );
    }
    while( NULL != p ) {
        char *comma = "";
        printf( "       time: %ld, block_count: %zd [", p->time, p->block_count );
        for( i = 0; i < p->block_count; i++ ) {
            printf( "%s%d", comma, p->block[i] );
            comma = ", ";
        }
        printf( "]\n" );
        p = p->next;
    }

    p = s->weekly;
    printf( "   s->weekly:\n" );
    if( NULL == p ) {
        printf( "       NULL\n" );
    }
    while( NULL != p ) {
        char *comma = "";
        printf( "       time: %ld, block_count: %zd [", p->time, p->block_count );
        for( i = 0; i < p->block_count; i++ ) {
            printf( "%s%d", comma, p->block[i] );
            comma = ", ";
        }
        printf( "]\n" );
        p = p->next;
    }
    printf( "}\n" );
}


/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
