#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef struct {
	SPI_HandleTypeDef* channel;
	GPIO_TypeDef* cs_port;
    uint16_t cs_pin;
} spi_channel_dev_ctx;

#ifdef __cplusplus
}
#endif