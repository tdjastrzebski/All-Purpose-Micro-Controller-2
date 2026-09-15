#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "spi_drv.h"

bool ads1x2s14_init(spi_channel_dev_ctx* dev_ctx);

#ifdef __cplusplus
}
#endif