#include "Timer.h"

#include <assert.h>

#include "main.h"

static uint32_t getHwdChannel(timer_channel channel);
static uint32_t getHwdLpChannel(timer_channel channel);
static uint32_t nearest_ceil_power_of_two(uint32_t value);
static int32_t power_of_two_index(uint32_t value);
static float _timers_clock_frequency;
static float _lptimers_clock_frequency;

void timer_init(float timers_clock_frequency) {
	_timers_clock_frequency = timers_clock_frequency;
}

#if defined(HAL_LPTIM_MODULE_ENABLED)
void lptimer_init(float lptimers_clock_frequency) {
	_lptimers_clock_frequency = lptimers_clock_frequency;
}

void lptimer_start_pwm(lptimer_ctx* ctx, float frequency, float dutyCycle) {
	// see RM0456, section 58 Low-power timer (LPTIM)
	// 58.4.19 PWM mode
	assert(dutyCycle <= 1.0f);
	uint32_t periodCycleCount = _lptimers_clock_frequency / frequency;  // counter period in timer clock cycles
	uint32_t prescalerDivRatio = 0;                                     // by default prescaler divider set to 1

	if (periodCycleCount > 65535) {
		uint32_t prescalerDivRatio = (periodCycleCount + 65536 - 1) / 65536;  // ceiling division
		// unlike in case of regular timer, lp timer prescaler value is power of two, 128 max
		prescalerDivRatio = nearest_ceil_power_of_two(prescalerDivRatio);
		if (prescalerDivRatio > 128) Error_Handler();  // max lp timer prescaler div ratio is 128
		float timerClockFrequency = _lptimers_clock_frequency / prescalerDivRatio;
		periodCycleCount = timerClockFrequency / frequency;
		// See: 58.7.10 LPTIM configuration register (LPTIM_CFGR)
		prescalerDivRatio = power_of_two_index(prescalerDivRatio);  // calculate log2
	}

	// "The LPTIM_CFGR register must only be modified when the LPTIM is disabled (ENABLE bit reset to 0)."
	if (LL_LPTIM_GetPrescaler(ctx->tim->Instance) != prescalerDivRatio) {
		LL_LPTIM_Disable(ctx->tim->Instance);
		LL_LPTIM_SetPrescaler(ctx->tim->Instance, prescalerDivRatio);
	}

	if (!LL_LPTIM_IsEnabled(ctx->tim->Instance)) {
		LL_LPTIM_Enable(ctx->tim->Instance);
		while (!LL_LPTIM_IsEnabled(ctx->tim->Instance));
	}

	// make sure new ARR value takes effect at the end of the current PWM cycle to prevent glitches
	// 58.7.13 LPTIM autoreload register (LPTIM_ARR)
	// "The LPTIM_ARR register must only be modified when the LPTIM is enabled (ENABLE bit set to 1)."
	LL_LPTIM_SetUpdateMode(ctx->tim->Instance, LL_LPTIM_UPDATE_MODE_ENDOFPERIOD);
	LL_LPTIM_SetAutoReload(ctx->tim->Instance, periodCycleCount);  // sets ARR register
	uint32_t pulseCycleCount = periodCycleCount * dutyCycle;       // pulse length

	// set CCRx register
	// "The LPTIM_CCR1 register must only be modified when the LPTIM is enabled (ENABLE bit set to 1)."
	if (ctx->channel == timer_ch1) {
		// 58.7.12 LPTIM compare register 1 (LPTIM_CCR1)
		LL_LPTIM_SetCompareCH1(ctx->tim->Instance, periodCycleCount - pulseCycleCount);
	} else if (ctx->channel == timer_ch2) {
		// 58.7.18 LPTIM compare register 2 (LPTIM_CCR2)
		LL_LPTIM_SetCompareCH2(ctx->tim->Instance, periodCycleCount - pulseCycleCount);
	} else {
		Error_Handler();
	}

	LL_LPTIM_StartCounter(ctx->tim->Instance, LL_LPTIM_OPERATING_MODE_CONTINUOUS);
}

void lptimer_stop(lptimer_ctx* ctx) {
	LL_LPTIM_Disable(ctx->tim->Instance);
}
#endif

void timer_start_pwm(timer_ctx* ctx, float frequency, float dutyCycle) {
	if (ctx->channel == timer_ch_none) return;
	// set PWM timer period and frequency - see AN4776 5.3, 6.2
	// RM0456 56.4.12 PWM mode

	// 56.7.6 TIM15 event generation register (TIM15_EGR) - Bit 0 UG: Update generation
	uint32_t periodCycleCount = _timers_clock_frequency / frequency;  // counter period in timer clock cycles
	// check if ARR and CCR1, 16b except for timers 2-5 (32b)
	if (periodCycleCount > 65535 && !IS_TIM_32B_COUNTER_INSTANCE(ctx->tim->Instance)) {
		uint32_t prescalerDivRatio = periodCycleCount / 65536;
		float timerClockFrequency = _timers_clock_frequency / (prescalerDivRatio + 1);
		periodCycleCount = timerClockFrequency / frequency;
		__HAL_TIM_SET_PRESCALER(ctx->tim, prescalerDivRatio);
	} else {
		__HAL_TIM_SET_PRESCALER(ctx->tim, 0);
	}

	HAL_StatusTypeDef result;
	uint32_t hwdChannel = getHwdChannel(ctx->channel);
	uint32_t pulseCycleCount = periodCycleCount * dutyCycle;  // counter value at which output state changes

	if (periodCycleCount < 2 || pulseCycleCount == 0) {
		// stop the timer since it is not possible to generate pwm with less than 2 cycles in period or 0 pulse counts
		timer_stop(ctx);
		return;
	}

	// **Note** do not set CCRx to zero if "output compare preload" is set as this causes the output to remain low all the time, even after value is changes to non-zero value later.
	// When CCRx = 0, timer waits with update until Counter = -1, which never happens. Consequently, update condition never occurs.
	// See: RM0490, section 25.4.8, "PWM mode"

	__HAL_TIM_SET_AUTORELOAD(ctx->tim, periodCycleCount - 1);      // set ARR: auto-reload registry - determines frequency, subtract 1 since timer counts from zero and reloads *from the next cycle* (not immediately)
	__HAL_TIM_SET_COMPARE(ctx->tim, hwdChannel, pulseCycleCount);  // set CCRx: capture compare registry, state changes immediately once CCR = CNT, do NOT subtract 1

	if (ctx->channel < timer_ch1n && TIM_CHANNEL_STATE_GET(ctx->tim, hwdChannel) == HAL_TIM_CHANNEL_STATE_READY) {
		result = HAL_TIM_PWM_Start(ctx->tim, hwdChannel);
		if (result != HAL_OK) Error_Handler();
	} else if (ctx->channel >= timer_ch1n && TIM_CHANNEL_N_STATE_GET(ctx->tim, hwdChannel) == HAL_TIM_CHANNEL_STATE_READY) {
		// start one of the N channels
		result = HAL_TIMEx_PWMN_Start(ctx->tim, hwdChannel);
		if (result != HAL_OK) Error_Handler();
	} else {
		// timer already started (BUSY) or not configured (RESET)
	}
}

void timer_stop(timer_ctx* ctx) {
	uint32_t hwdChannel = getHwdChannel(ctx->channel);

	if (ctx->channel < timer_ch1n && TIM_CHANNEL_STATE_GET(ctx->tim, hwdChannel) == HAL_TIM_CHANNEL_STATE_BUSY) {
		if (HAL_TIM_PWM_Stop(ctx->tim, getHwdChannel(ctx->channel)) != HAL_OK) {
			Error_Handler();
		}
	} else if (ctx->channel >= timer_ch1n && TIM_CHANNEL_N_STATE_GET(ctx->tim, hwdChannel) == HAL_TIM_CHANNEL_STATE_BUSY) {
		// stop one of the N channels
		if (HAL_TIMEx_PWMN_Stop(ctx->tim, getHwdChannel(ctx->channel)) != HAL_OK) {
			Error_Handler();
		}
	} else {
		// timer already stopped (READY) or not configured (RESET)
	}
}

#if defined(HAL_LPTIM_MODULE_ENABLED)
static uint32_t getHwdLpChannel(timer_channel channel) {
	switch (channel) {
	case timer_ch1:
		return LPTIM_CHANNEL_1;
	case timer_ch2:
		return LPTIM_CHANNEL_2;
	default:
		Error_Handler();
		return 0;  // this is just to avoid the warning
	}
}
#endif

static uint32_t getHwdChannel(timer_channel channel) {
	switch (channel) {
	case timer_ch1:
	case timer_ch1n:
		return TIM_CHANNEL_1;
	case timer_ch2:
	case timer_ch2n:
		return TIM_CHANNEL_2;
	case timer_ch3:
	case timer_ch3n:
		return TIM_CHANNEL_3;
	case timer_ch4:
	case timer_ch4n:
		return TIM_CHANNEL_4;
	case timer_ch5:
	case timer_ch5n:
		return TIM_CHANNEL_5;
	case timer_ch6:
	case timer_ch6n:
		return TIM_CHANNEL_6;
	default:
		Error_Handler();
		return 0;  // this is just to avoid the warning
	}
}

static uint32_t nearest_ceil_power_of_two(uint32_t value) {
	if (value == 0) return 1;
	return 1U << (32 - __builtin_clz(value - 1));
}

static int32_t power_of_two_index(uint32_t value) {
	if (value == 0 || (value & (value - 1)) != 0) return -1;
	return 31 - __builtin_clz(value);
}