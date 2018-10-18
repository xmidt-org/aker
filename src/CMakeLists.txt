#   Copyright 2016 Comcast Cable Communications Management, LLC
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

set(PROJ_AKER aker)
set(SOURCES wrp_interface.c decode.c time.c schedule.c
            process_data.c scheduler.c schedule_print.c
            aker_md5.c md5.c aker_mem.c aker_help.c aker_msgpack.c)

if (NOT BUILD_YOCTO)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W -g -fprofile-arcs -ftest-coverage -O0")
endif ()

add_executable(aker main.c ${SOURCES})

if (NOT BUILD_YOCTO)
	add_executable(aker-cli cli.c ${SOURCES})

	target_link_libraries (aker-cli
		${CMAKE_THREAD_LIBS_INIT}
		-lwrp-c
		-lmsgpackc
		-ltrower-base64
		-lm
		-lcimplog
		-lpthread
		)
	if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	target_link_libraries (aker-cli -lrt)
	endif()
endif ()

target_link_libraries (aker
    ${CMAKE_THREAD_LIBS_INIT}
    -llibparodus
    -lnng
    -lwrp-c
    -lmsgpackc
    -ltrower-base64
    -lm
    -lcimplog
    -lpthread
    )
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (aker rt)
endif()

install (TARGETS aker DESTINATION bin)
