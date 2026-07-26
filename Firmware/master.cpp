#include "master.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "Sub_RTC.h"
#include "Timer.h"
#include "dwt_timer.h"
#include "lvgl_sub.h"
#include "m95p32.h"
#include "my_printf.h"
#include "spi_drv.h"
#include "st7789.h"
#include "stm32u5xx_ll_utils.h"
#include "terminal_colors.h"

extern TIM_HandleTypeDef EncoderTimer;
extern TIM_HandleTypeDef LcdPwmTimer;
extern SPI_HandleTypeDef LcdSpi;
extern SPI_HandleTypeDef BoardSpi;
extern RTC_HandleTypeDef RtcTimer;
extern TIM_HandleTypeDef htim1;

static timer_ctx _lcdPwmTimer = {.tim = &LcdPwmTimer, .channel = timer_ch1};
static timer_ctx _ledPwmTimer = {.tim = &htim1, .channel = timer_ch1};
static spi_channel_dev_ctx _lcd_spi = {.channel = &LcdSpi, .cs_port = LCD_CS_GPIO_Port, .cs_pin = LCD_CS_Pin};
static spi_channel_dev_ctx _board_spi = {.channel = &BoardSpi, .cs_port = SPI_CS1_GPIO_Port, .cs_pin = SPI_CS1_Pin};

static void _encoderTimerCaptureCallback(TIM_HandleTypeDef* htim);
static void _rtcAlarmAEventCallback(RTC_HandleTypeDef* hrtc);

void PreInit(void) {}

void SysInit(void) {}

void Init(void) {
	HAL_PWREx_EnableBatteryCharging(PWR_BATTERY_CHARGING_RESISTOR_1_5);
	HAL_PWREx_DisableUCPDDeadBattery();
}

void PostInit(void) {
	// rtc_SetDate(26, 07, 26, 6);
	// rtc_SetTime(14, 06, 30);

	HAL_Delay(100);  // wait a bit more
	time_t dateTime = rtc_GetDateTime();
	tm* dt = gmtime(&dateTime);
	my_printf("\n" YELLOW("* restarted %.2i:%.2i:%.2i *") "\n", dt->tm_hour, dt->tm_min, dt->tm_sec);

	dwt_init();  // always init

	HAL_StatusTypeDef status;

	if (HAL_TIM_Encoder_Start_IT(&EncoderTimer, TIM_CHANNEL_1 | TIM_CHANNEL_2) != HAL_OK) {
		Error_Handler();
	}

	timer_init(16000000.0);
	timer_start_pwm(&_lcdPwmTimer, 100.0, 0.5);
	timer_start_pwm(&_ledPwmTimer, 100.0, 0.1);

	st7789_Init(&_lcd_spi);

	st7789_FillScreen(&_lcd_spi, st7789_color_black);

	uint8_t data[3]{0};
	bool isOk = true;
	status = m95p32_ReadJEDEC(&_board_spi, data, 3);
	if (status != HAL_OK) isOk = false;
	if (data[0] != 0x20 || data[1] != 0 || data[2] != 0x16) isOk = false;
	if (isOk) {
		my_printf(GREEN("ext eeprom memory test passed") "\n");
	} else {
		my_printf(RED("ext eeprom memory test failed") "\n");
	}

	// Draw a single green pixel at (120, 140)
	st7789_DrawPixel(&_lcd_spi, 120, 140, st7789_color_blue);

	for (uint16_t i = 0; i < 240; i++) {
		st7789_DrawPixel(&_lcd_spi, i, 140, st7789_color_blue);
		st7789_DrawPixel(&_lcd_spi, i, 10, st7789_color_blue);
		st7789_DrawPixel(&_lcd_spi, i, 269, st7789_color_blue);
	}
	for (uint16_t i = 0; i < 280; i++) {
		st7789_DrawPixel(&_lcd_spi, 10, i, st7789_color_green);
		st7789_DrawPixel(&_lcd_spi, 120, i, st7789_color_green);
		st7789_DrawPixel(&_lcd_spi, 230, i, st7789_color_green);
	}

	EncoderTimer.IC_CaptureCallback = _encoderTimerCaptureCallback;
	RtcTimer.AlarmAEventCallback = _rtcAlarmAEventCallback;

	lvgl_init();
}

void MainLoop(void) {
	static uint32_t pattern = 0xF0F0CCC0;
	static uint32_t shift = 0;
	HAL_GPIO_WritePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin, (pattern >> shift) & 0x1 ? GPIO_PIN_SET : GPIO_PIN_RESET);
	shift++;
	shift %= 32;
	HAL_Delay(200);
}

static void _rtcAlarmAEventCallback(RTC_HandleTypeDef* hrtc) {
	time_t dateTime = rtc_GetDateTime();
	tm* dt = gmtime(&dateTime);
	lvgl_showTime(dt);
}

static void _encoderTimerCaptureCallback(TIM_HandleTypeDef* htim) {
	int16_t count = (int16_t)__HAL_TIM_GET_COUNTER(&EncoderTimer);
	if (__HAL_TIM_IS_TIM_COUNTING_DOWN(htim)) {
		my_printf(" down c %i\n", count);
	} else {
		my_printf(" up c %i\n", count);
	}
	lvgl_showNumber(count);
}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin) {
	time_t dateTime = rtc_GetDateTime();
	tm* dt = gmtime(&dateTime);
	my_printf("\n" YELLOW("* clicked at %.2i:%.2i:%.2i *") "\n", dt->tm_hour, dt->tm_min, dt->tm_sec);

	switch (GPIO_Pin) {
	case BTN1_Pin: {
		my_printf(" BTN1 INT\n");
		break;
	}
	case BTN2_Pin: {
		my_printf(" BTN2 INT\n");
		break;
	}
	}
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin) {
	switch (GPIO_Pin) {
	case EF1_Pin: {
		my_printf(" EF1 INT\n");
		break;
	}
	case EF2_Pin: {
		my_printf(" EF2 INT\n");
		break;
	}
	}
}