#ifndef __TEST_SCHEDULER_H__
#define __TEST_SCHEDULER_H__


#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>

time_t convert_unix_time_to_weekly(time_t unixtime);
time_t get_unix_time(void);


#ifdef __cplusplus
}
#endif
#endif
