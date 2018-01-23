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
#include "aker_mem.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define COMMA ", "
#define INDEX "ddd"

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
void print_indices( schedule_event_t *e );

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
    print_indices( p );

    p = s->weekly;
    debug_info( "   s->weekly:\n" );
    if( NULL == p ) {
        debug_info( "       NULL\n" );
    }
    print_indices( p );
    debug_info( "}\n" );
}


/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/**
 *  Print to debug, indices of a given schedule event. 
 *
 *  @param e schedule event
 */
void print_indices( schedule_event_t *e )
{
    size_t i;

    while( NULL != e ) {
        size_t count = e->block_count;
        if( 0 < count ) {
            size_t buf_size = sizeof(char) * (((count - 1) * strlen(COMMA)) + (count * strlen(INDEX)) + 1);
            char *buf = (char *) aker_malloc( buf_size );
            if( NULL != buf) {
                char *t;
                memset( buf, '\0', buf_size );
                for( i = 0, t = buf; i < e->block_count; i++ ) {
                    if( 0 < i) {
                        strcat( buf, ", " );
                        t += 2;
                    }
                    sprintf( t, "%3d", e->block[i] );
                    t += 3;
                }
            }
            debug_info( "       time: %ld, block_count: %zd [ %s ]\n", e->time, e->block_count, buf );
            if( NULL != buf ) {
                aker_free( buf );
            }
        }
        e = e->next;
    }
}
