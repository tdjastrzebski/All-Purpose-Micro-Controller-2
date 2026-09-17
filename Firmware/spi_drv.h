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

enum spi_drv_direction : bool {
	spi_drv_direction_fullDuplex = false,  // use TrasmitReceive only
	spi_drv_direction_halfDuplex = true,   // use Trasmit or Receive only
};

enum spi_drv_mode : uint8_t {
	spi_drv_mode_0 = 0,
	spi_drv_mode_1 = 1,
	spi_drv_mode_2 = 2,
	spi_drv_mode_3 = 3,
};

void spi_drv_configureSpi(spi_channel_dev_ctx* dev_ctx, spi_drv_direction direction, spi_drv_mode mode);

#ifdef __cplusplus
}
#endif