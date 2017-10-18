/**
 * @file aker_log.h
 *
 * @description This header defines log levels
 *
 * Copyright (c) 2017  Comcast
 */
 
#ifndef __AKER_LOG_H__
#define __AKER_LOG_H__

#include <stdarg.h>
#include <cimplog/cimplog.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define debug_error(...)      cimplog_error("aker", __VA_ARGS__)
#define debug_info(...)       cimplog_info("aker", __VA_ARGS__)
#define debug_print(...)      cimplog_debug("aker", __VA_ARGS__)

#endif
