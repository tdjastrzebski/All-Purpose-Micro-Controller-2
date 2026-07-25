#include "Sub_RTC.h"

#include <stdio.h>

#include <ctime>

#include "main.h"

extern RTC_HandleTypeDef hrtc;

void rtc_SetDate(uint8_t year, uint8_t month, uint8_t day, uint8_t weekDay) {
	RTC_DateTypeDef sDate = {0};

	sDate.Year = year;
	sDate.Month = month;
	sDate.Date = day;
	sDate.WeekDay = weekDay;

	if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
		Error_Handler();
	}
}

void rtc_SetTime(uint8_t hours, uint8_t minutes, uint8_t seconds) {
	RTC_TimeTypeDef sTime = {0};

	sTime.Hours = hours;
	sTime.Minutes = minutes;
	sTime.Seconds = seconds;
	sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	sTime.StoreOperation = RTC_STOREOPERATION_RESET;

	if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
		Error_Handler();
	}
}

time_t rtc_GetDateTime() {
	RTC_DateTypeDef rtc_date;
	RTC_TimeTypeDef rtc_time;

	HAL_RTC_GetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN);  // note: HAL_RTC_GetDate() must be called after HAL_RTC_GetTime()

	tm t;
	t.tm_hour = rtc_time.Hours;
	t.tm_min = rtc_time.Minutes;
	t.tm_sec = rtc_time.Seconds;
	t.tm_year = rtc_date.Year + 2000 - 1900;
	t.tm_mon = rtc_date.Month - 1;
	t.tm_mday = rtc_date.Date;
	t.tm_isdst = 0;

	// convert back and forth to get wday & yday set
	time_t time = mktime(&t);
	tm* gt = gmtime(&time);
	time = mktime(gt);

	return time;
}