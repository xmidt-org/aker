#   Copyright 2017 Comcast Cable Communications Management, LLCD
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

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W -g -fprofile-arcs -ftest-coverage -O0")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W  -g -fprofile-arcs -ftest-coverage -O0")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -O0")
set (AKER_COMMON_LIBS -lcunit -lcimplog -lwrp-c -lpthread -lmsgpackc
     -lnanomsg -ltrower-base64)
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
set (AKER_COMMON_LIBS "${AKER_COMMON_LIBS} gcov -Wl,--no-as-needed")
endif()

if(NOT DISABLE_VALGRIND)
set (MEMORY_CHECK valgrind --leak-check=full --show-reachable=yes -v)
endif ()

link_directories ( ${LIBRARY_DIR} )

#-------------------------------------------------------------------------------
#   test_schedule
#-------------------------------------------------------------------------------
add_test(NAME test_schedule COMMAND ${MEMORY_CHECK} ./test_schedule)
add_executable(test_schedule test_schedule.c ../src/schedule_print.c 
               ../src/schedule.c ../src/decode.c ../src/process_data.c
               ../src/aker_md5.c ../src/md5.c ../src/aker_msgpack.c
               ../src/scheduler.c mem_wrapper.c common_test_stubs.c)
target_link_libraries (test_schedule ${AKER_COMMON_LIBS})

#-------------------------------------------------------------------------------
#   test_time_changes
#-------------------------------------------------------------------------------
add_test(NAME test_time_changes COMMAND ${MEMORY_CHECK} ./test_time_changes)
add_executable(test_time_changes test_time_changes.c ../src/schedule_print.c 
               ../src/schedule.c ../src/time.c mem_wrapper.c)
target_link_libraries (test_time_changes ${AKER_COMMON_LIBS})

#-------------------------------------------------------------------------------
#   test_wrp_interface
#-------------------------------------------------------------------------------
add_test(NAME test_wrp_interface COMMAND ${MEMORY_CHECK} ./test_wrp_interface)
add_executable(test_wrp_interface test_wrp_interface.c ../src/wrp_interface.c
               mem_wrapper.c )
target_link_libraries (test_wrp_interface ${AKER_COMMON_LIBS})

#-------------------------------------------------------------------------------
#   test_aker_msgpack
#-------------------------------------------------------------------------------
add_test(NAME test_aker_msgpack COMMAND ${MEMORY_CHECK} ./test_aker_msgpack)
add_executable(test_aker_msgpack test_aker_msgpack.c ../src/aker_msgpack.c
               mem_wrapper.c )
target_link_libraries (test_aker_msgpack ${AKER_COMMON_LIBS})


#-------------------------------------------------------------------------------
#   test_process_data
#-------------------------------------------------------------------------------
add_test(NAME test_process_data COMMAND ${MEMORY_CHECK} ./test_process_data)
add_executable(test_process_data test_process_data.c ../src/process_data.c
               ../src/aker_md5.c ../src/md5.c ../src/schedule_print.c
               ../src/schedule.c ../src/decode.c ../src/time.c 
               ../src/scheduler.c ../src/aker_msgpack.c mem_wrapper.c )

target_link_libraries (test_process_data ${AKER_COMMON_LIBS})

#-------------------------------------------------------------------------------
#   test_process_ret_now
#-------------------------------------------------------------------------------
add_test(NAME test_process_ret_now COMMAND ${MEMORY_CHECK} ./test_process_ret_now)
add_executable(test_process_ret_now test_process_ret_now.c ../src/process_data.c
               ../src/aker_msgpack.c mem_wrapper.c )

target_link_libraries (test_process_ret_now ${AKER_COMMON_LIBS})

#-------------------------------------------------------------------------------
#   test_process_is_create_ok
#-------------------------------------------------------------------------------
add_test(NAME test_process_is_create_ok COMMAND ${MEMORY_CHECK} ./test_process_is_create_ok)
add_executable(test_process_is_create_ok test_process_is_create_ok.c ../src/process_data.c
               ../src/aker_md5.c ../src/md5.c ../src/schedule_print.c
               ../src/schedule.c ../src/decode.c ../src/time.c 
               ../src/scheduler.c ../src/aker_msgpack.c mem_wrapper.c )

target_link_libraries (test_process_is_create_ok ${AKER_COMMON_LIBS})

#-------------------------------------------------------------------------------
#   test_decode
#-------------------------------------------------------------------------------
add_test(NAME test_decode COMMAND ${MEMORY_CHECK} ./test_decode)
add_executable(test_decode test_decode.c ../src/decode.c ../src/schedule.c
               ../src/time.c mem_wrapper.c ../src/schedule_print.c)
target_link_libraries (test_decode ${AKER_COMMON_LIBS})

#-------------------------------------------------------------------------------
#   test_md5
#-------------------------------------------------------------------------------
add_test(NAME test_md5 COMMAND ${MEMORY_CHECK} ./test_md5)
add_executable(test_md5 test_md5.c ../src/process_data.c ../src/aker_md5.c 
               ../src/md5.c ../src/scheduler.c ../src/time.c ../src/schedule.c
               ../src/decode.c ../src/schedule_print.c ../src/aker_msgpack.c 
               mem_wrapper.c )
target_link_libraries (test_md5 ${AKER_COMMON_LIBS})

#-------------------------------------------------------------------------------
#   test_time
#-------------------------------------------------------------------------------
add_test(NAME test_time COMMAND ${MEMORY_CHECK} ./test_time)
add_executable(test_time test_time.c ../src/time.c mem_wrapper.c )
target_link_libraries (test_time ${AKER_COMMON_LIBS})


#-------------------------------------------------------------------------------
#   test_scheduleR
#-------------------------------------------------------------------------------
add_test(NAME test_scheduler COMMAND ${MEMORY_CHECK} ./test_scheduler)
add_executable(test_scheduler test_scheduler.c ../src/schedule_print.c
               ../src/schedule.c ../src/decode.c ../src/process_data.c
               ../src/aker_md5.c ../src/md5.c ../src/aker_msgpack.c
               ../src/scheduler.c mem_wrapper.c common_test_stubs.c)
target_link_libraries (test_scheduler ${AKER_COMMON_LIBS})


add_custom_target(coverage
COMMAND lcov -q --capture --directory 
${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/test_time.dir/__/src --output-file time.info
COMMAND lcov -q --capture --directory
${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/test_md5.dir/__/src --output-file md5.info
COMMAND lcov -q --capture --directory 
${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/test_decode.dir/__/src --output-file decode.info
COMMAND lcov -q --capture --directory 
${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/test_schedule.dir/__/src --output-file schedule.info
COMMAND lcov -q --capture --directory 
${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/test_process_data.dir/__/src --output-file process.info
COMMAND lcov -q --capture --directory 
${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/test_process_ret_now.dir/__/src --output-file process_now.info
COMMAND lcov -q --capture --directory 
${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/test_process_is_create_ok.dir/__/src --output-file process_is_create_ok.info
COMMAND lcov -q --capture --directory 
${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/test_wrp_interface.dir/__/src --output-file wrp.info
COMMAND lcov -q --capture --directory
${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/test_scheduler.dir/__/src --output-file scheduler.info
COMMAND lcov -q --capture --directory
${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/test_aker_msgpack.dir/__/src --output-file aker_msgpack.info

COMMAND lcov -a md5.info -a decode.info -a process_now.info -a process_is_create_ok.info
-a schedule.info -a process.info -a time.info -a scheduler.info
-a wrp.info -a aker_msgpack.info --output-file coverage.info

COMMAND genhtml coverage.info
WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

