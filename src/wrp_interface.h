#ifndef _WRP_INTERFACE_H_
#define _WRP_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif
    
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <wrp-c/wrp-c.h>
    
/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/**
 *  Process an incoming message.
 *
 *  @note If object is not NULL, it needs to be free()-ed by the caller.
 *
 *  @param msg      [in]  incoming WRP data.
 *  @param response [out] response message.
 *
 *  @return size of valid message, < 0 otherwise.
 */
ssize_t wrp_to_object(wrp_msg_t *msg, void **response);

#ifdef __cplusplus
}
#endif

#endif
