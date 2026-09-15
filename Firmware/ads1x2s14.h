/*---------------------------------------------------------------------------------------------------
 *  Copyright (c) 2026 Tomasz Jastrzębski. All rights reserved.
 *-------------------------------------------------------------------------------------------------*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "spi_drv.h"

enum ads1x2s14_gpio : uint8_t {
	ads1x2s14_gpio_disabled = 0,
	ads1x2s14_gpio_input = 1,
	ads1x2s14_gpio_pushPull = 2,
	ads1x2s14_gpio_openDrain = 3,
};

bool ads1x2s14_init(spi_channel_dev_ctx* dev_ctx);
bool ads1x2s14_gpio_config(spi_channel_dev_ctx* dev_ctx, uint8_t gpio_nbr, ads1x2s14_gpio gpio_config);
bool ads1x2s14_gpio_setState(spi_channel_dev_ctx* dev_ctx, uint8_t gpio_nbr, bool output_state);

#ifdef __cplusplus
}
#endif