// Source: https://community.st.com/s/question/0D50X0000BGkxmCSQR/stm32l462-delay-in-a-microsecondus
#include "dwt_timer.h"

static bool _isInitialized = false;
uint32_t _dwtFrqMhz; // DWT clock frequency in MHz

/**
 * @brief  Initializes DWT_Clock_Cycle_Count for DWT_Delay_us function
 */
bool dwt_init(void) {
	if (_isInitialized) return true;
	if (DWT->CTRL & DWT_CTRL_NOCYCCNT_Msk) return false;  // DWT not supported
	/* Disable TRC */
	CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;  // ~0x01000000;
	/* Enable TRC */
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  // 0x01000000;

	/* Disable clock cycle counter */
	DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;  //~0x00000001;
	/* Enable  clock cycle counter */
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;  // 0x00000001;

	/* Reset the clock cycle counter value */
	DWT->CYCCNT = 0;

	/* 3 NO OPERATION instructions */
	__ASM volatile("NOP");
	__ASM volatile("NOP");
	__ASM volatile("NOP");
	
	_dwtFrqMhz = HAL_RCC_GetHCLKFreq() / 1000000;

	/* Check if clock cycle counter has started */
	if (DWT->CYCCNT) {
		_isInitialized = false;
		return false; /*clock cycle counter started*/
	} else {
		_isInitialized = true;
		return true; /*clock cycle counter not started*/
	}
}

// get elapsed time since reset in µs
uint32_t dwt_get_us(void) {
	uint32_t us = DWT->CYCCNT / _dwtFrqMhz;
	return us;
}

// reset µs timer
void dwt_reset(void) {
	DWT->CYCCNT = 0;
}

/**
 * @brief  This function provides a delay in µs
 * @param  microseconds: delay in microseconds
 */
inline void dwt_delay(uint32_t microseconds) {
	uint32_t clk_cycle_start = DWT->CYCCNT;

	// FIXME: what if DWT->CYCCNT just rolls over?

	/* Go to number of cycles for system */
	microseconds *= _dwtFrqMhz;

	/* Delay till end */
	while ((DWT->CYCCNT - clk_cycle_start) < microseconds)
		;
}

/* Use DWT_Delay_Init (); and DWT_Delay_us (microseconds) in the main */
