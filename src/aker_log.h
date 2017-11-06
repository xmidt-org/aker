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
/**
 * @file aker_log.h
 *
 * @description This header defines log levels
 *
 */
 
#ifndef __AKER_LOG_H__
#define __AKER_LOG_H__

#include <stdarg.h>
#include <cimplog/cimplog.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define debug_error(...)      cimplog_error("Aker", __VA_ARGS__)
#define debug_info(...)       cimplog_info("Aker", __VA_ARGS__)
#define debug_print(...)      cimplog_debug("Aker", __VA_ARGS__)

#endif
