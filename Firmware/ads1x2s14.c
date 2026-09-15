#include "ads1x2s14.h"

#include "dwt_timer.h"

// https://www.ti.com/lit/ds/symlink/ads112s14.pdf
// max CLK frequency is 16.67 MHz (5.6 Timing Requirements)

static HAL_StatusTypeDef _readRegister(spi_channel_dev_ctx* dev_ctx, uint8_t regAddress, uint8_t* data) {
	// 7.5.4 Device Commands
	// 7.5.4.3 Write Register Command
	// 7.5.4.2 Read Register Command

	dev_ctx->channel->Instance->CR1 &= ~SPI_CR1_SPE;       // disable SPI
	dev_ctx->channel->Instance->CFG2 |= SPI_CFG2_CPHA;     // 2nd edge
	dev_ctx->channel->Instance->CFG2 &= ~SPI_CFG2_CPOL;    // polarity low
	dev_ctx->channel->Instance->CFG2 &= ~SPI_CFG2_LSBFRST;  // disable LSB first
	dev_ctx->channel->Instance->CFG2 &= ~(SPI_CFG2_SP_0 | SPI_CFG2_SP_1 | SPI_CFG2_SP_2); // reset SPI_CFG2_SP bits - Motorola mode
	dev_ctx->channel->Instance->CR1 |= SPI_CR1_SPE;        // enable SPI

	HAL_StatusTypeDef status;
	uint32_t rxData = 0;
	uint32_t txData = ((regAddress & 0xf) << 0);
	txData |= (0x40 << 0);
	HAL_GPIO_WritePin(dev_ctx->cs_port, dev_ctx->cs_pin, GPIO_PIN_RESET);
	dwt_delay(1);
	status = HAL_SPI_Transmit(dev_ctx->channel, (uint8_t*)&txData, 2, 1000);
	dwt_delay(1);
	HAL_GPIO_WritePin(dev_ctx->cs_port, dev_ctx->cs_pin, GPIO_PIN_SET);
	if (status != HAL_OK) return status;
	dwt_delay(1);
	HAL_GPIO_WritePin(dev_ctx->cs_port, dev_ctx->cs_pin, GPIO_PIN_RESET);
	status = HAL_SPI_Receive(dev_ctx->channel, (uint8_t*)&rxData, 2, 1000);
	HAL_GPIO_WritePin(dev_ctx->cs_port, dev_ctx->cs_pin, GPIO_PIN_SET);
	if (status != HAL_OK) return status;
	*data = rxData;
	return status;
}

bool ads1x2s14_init(spi_channel_dev_ctx* dev_ctx) {
	// 8.1 DEVICE_ID Register (Address = 00h)
	uint8_t data = 0;
	_readRegister(dev_ctx, 0x0, &data);
	uint8_t id = data & 0x0f;
	if (id != 0b1011 && id != 0b1010) {
		return false;
	}
    // 8.2 REVISION_ID Register (Address = 01h)
    _readRegister(dev_ctx, 0x1, &data);
    // 8.3 STATUS_MSB Register (Address = 02h) 
    _readRegister(dev_ctx, 0x2, &data);
    // 8.4 STATUS_LSB Register (Address = 03h)
    _readRegister(dev_ctx, 0x3, &data);
	return true;
}
