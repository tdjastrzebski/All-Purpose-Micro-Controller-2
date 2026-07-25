// Source: https://community.st.com/s/question/0D50X0000BGkxmCSQR/stm32l462-delay-in-a-microsecondus
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32u5xx_hal.h"

bool dwt_init(void);
uint32_t dwt_get_us(void);
void dwt_reset(void);
void dwt_delay(uint32_t microseconds);

#ifdef __cplusplus
}
#endif