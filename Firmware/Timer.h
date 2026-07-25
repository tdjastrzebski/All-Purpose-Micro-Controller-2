#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

enum timer_channel : uint8_t {
	// note: this order must be kept - n-channels defined last
	timer_ch_none = 0,
	timer_ch1 = 1,
	timer_ch2 = 2,
	timer_ch3 = 3,
	timer_ch4 = 4,
	timer_ch5 = 5,
	timer_ch6 = 6,
	timer_ch1n = 7,
	timer_ch2n = 8,
	timer_ch3n = 9,
	timer_ch4n = 10,
	timer_ch5n = 11,
	timer_ch6n = 12,
};

typedef struct {
	TIM_HandleTypeDef* tim;
	timer_channel channel;
} timer_ctx;

#if defined(HAL_LPTIM_MODULE_ENABLED)
typedef struct {
	LPTIM_HandleTypeDef* tim;
	timer_channel channel;
} lptimer_ctx;
#endif

void timer_init(float timers_clock_frequency);
void timer_start_pwm(timer_ctx* ctx, float frequency, float dutyCycle);
void timer_stop(timer_ctx* ctx);

#if defined(HAL_LPTIM_MODULE_ENABLED)
void lptimer_init(float lptimers_clock_frequency);
void lptimer_start_pwm(lptimer_ctx* ctx, float frequency, float dutyCycle);
void lptimer_stop(lptimer_ctx* ctx);
#endif

#ifdef __cplusplus
}
#endif