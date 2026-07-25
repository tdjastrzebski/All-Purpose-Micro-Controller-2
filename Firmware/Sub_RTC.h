#pragma once

#include <ctime>

#ifdef __cplusplus
extern "C" {
#endif

void rtc_SetDate(uint8_t year, uint8_t month, uint8_t day, uint8_t weekDay);
void rtc_SetTime(uint8_t hours, uint8_t minutes, uint8_t seconds);
time_t rtc_GetDateTime(void);

#ifdef __cplusplus
}
#endif