/*---------------------------------------------------------------------------------------------------
 *  Copyright (c) 2026 Tomasz Jastrzębski. All rights reserved.
 *-------------------------------------------------------------------------------------------------*/

#include "ads1x2s14.h"

#include "dwt_timer.h"

// https://www.ti.com/lit/ds/symlink/ads112s14.pdf
// max CLK frequency is 16.67 MHz (5.6 Timing Requirements)

static HAL_StatusTypeDef _readRegister(spi_channel_dev_ctx* dev_ctx, uint8_t regAddress, uint8_t* data);
static HAL_StatusTypeDef _writeRegister(spi_channel_dev_ctx* dev_ctx, uint8_t regAddress, uint8_t data);
static void _configureSpi(spi_channel_dev_ctx* dev_ctx);

static void _configureSpi(spi_channel_dev_ctx* dev_ctx) {
	dev_ctx->channel->Instance->CR1 &= ~SPI_CR1_SPE;                                       // disable SPI
	dev_ctx->channel->Instance->CFG2 |= SPI_CFG2_CPHA;                                     // data on 2nd clk edge
	dev_ctx->channel->Instance->CFG2 &= ~SPI_CFG2_CPOL;                                    // clk polarity low
	dev_ctx->channel->Instance->CFG2 &= ~SPI_CFG2_LSBFRST;                                 // disable LSB first
	dev_ctx->channel->Instance->CFG2 &= ~(SPI_CFG2_SP_0 | SPI_CFG2_SP_1 | SPI_CFG2_SP_2);  // reset SPI_CFG2_SP bits - Motorola mode
	dev_ctx->channel->Instance->CR1 |= SPI_CR1_SPE;                                        // enable SPI
}

static HAL_StatusTypeDef _readRegister(spi_channel_dev_ctx* dev_ctx, uint8_t regAddress, uint8_t* data) {
	// 7.5.4 Device Commands
	// 7.5.4.2 Read Register Command
	HAL_StatusTypeDef status;
	uint16_t rxData = 0;
	uint16_t txData = 0;

	_configureSpi(dev_ctx);

	// first write 16 zeros to clear buffer
	HAL_GPIO_WritePin(dev_ctx->cs_port, dev_ctx->cs_pin, GPIO_PIN_RESET);
	dwt_delay(1);
	status = HAL_SPI_Transmit(dev_ctx->channel, (uint8_t*)&txData, 2, 1000);
	dwt_delay(1);
	HAL_GPIO_WritePin(dev_ctx->cs_port, dev_ctx->cs_pin, GPIO_PIN_SET);
	if (status != HAL_OK) return status;
	txData = (regAddress & 0xf);
	txData |= 0x40;
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

static HAL_StatusTypeDef _writeRegister(spi_channel_dev_ctx* dev_ctx, uint8_t regAddress, uint8_t data) {
	// 7.5.4 Device Commands
	// 7.5.4.3 Write Register Command
	HAL_StatusTypeDef status;
	uint16_t txData = 0;

	_configureSpi(dev_ctx);

	// first write 16 zeros to clear buffer
	HAL_GPIO_WritePin(dev_ctx->cs_port, dev_ctx->cs_pin, GPIO_PIN_RESET);
	dwt_delay(1);
	status = HAL_SPI_Transmit(dev_ctx->channel, (uint8_t*)&txData, 2, 1000);
	dwt_delay(1);
	HAL_GPIO_WritePin(dev_ctx->cs_port, dev_ctx->cs_pin, GPIO_PIN_SET);
	if (status != HAL_OK) return status;
	txData = (regAddress & 0xf);
	txData |= 0x80;
	txData |= (data << 8);
	HAL_GPIO_WritePin(dev_ctx->cs_port, dev_ctx->cs_pin, GPIO_PIN_RESET);
	dwt_delay(1);
	status = HAL_SPI_Transmit(dev_ctx->channel, (uint8_t*)&txData, 2, 1000);
	dwt_delay(1);
	HAL_GPIO_WritePin(dev_ctx->cs_port, dev_ctx->cs_pin, GPIO_PIN_SET);
	return status;
}

bool ads1x2s14_init(spi_channel_dev_ctx* dev_ctx) {
	HAL_StatusTypeDef status;
	uint8_t data = 0;

	// 8.1 DEVICE_ID Register (Address = 00h)
	// read device ID
	status = _readRegister(dev_ctx, 0x0, &data);
	if (status != HAL_OK) return false;
	data &= 0x0f;

	if (data == 0b1010) {
		// 16 bit device
	} else if (data == 0b1011) {
		// 24 bit device
	} else {
		return false;
	}

	// 8.5 CONVERSION_CTRL Register (Address = 04h) [Reset = 00h]
	// reset device
	status = _writeRegister(dev_ctx, 0x4, 0b010110 << 2);
	if (status != HAL_OK) return false;

	// 5.6 Timing Requirements
	dwt_delay(500);  // wait 500 us

	// 8.2 REVISION_ID Register (Address = 01h) [Reset = XXh]
	status = _readRegister(dev_ctx, 0x1, &data);
	if (status != HAL_OK) return false;

	// 8.3 STATUS_MSB Register (Address = 02h) [Reset = 3Eh]
	status = _readRegister(dev_ctx, 0x2, &data);
	if (status != HAL_OK) return false;
	if (data != 0x3e) return false;

	// 8.4 STATUS_LSB Register (Address = 03h) [Reset = F0h]
	status = _readRegister(dev_ctx, 0x3, &data);
	if (status != HAL_OK) return false;
	if (data != 0xf0) return false;

	return true;
}

bool ads1x2s14_gpio_config(spi_channel_dev_ctx* dev_ctx, uint8_t gpio_nbr, ads1x2s14_gpio gpio_config) {
	// 8.12 GPIO_CFG Register (Address = 0Bh) [Reset = 00h]
	assert_param(gpio_nbr < 4);
	HAL_StatusTypeDef status;
	uint8_t data = 0;

	status = _readRegister(dev_ctx, 0x0b, &data);
	if (status != HAL_OK) return false;
	uint8_t channelBits = 0x03 << (gpio_nbr * 2);
	data &= ~channelBits;
	data |= (gpio_config << (gpio_nbr * 2));
	status = _writeRegister(dev_ctx, 0x0b, data);
	if (status != HAL_OK) return false;

	return true;
}

bool ads1x2s14_gpio_setState(spi_channel_dev_ctx* dev_ctx, uint8_t gpio_nbr, bool output_state) {
	// 8.13 GPIO_DATA_OUTPUT Register (Address = 0Ch) [Reset = 00h]
	assert_param(gpio_nbr < 4);
	HAL_StatusTypeDef status;
	uint8_t data = 0;

	status = _readRegister(dev_ctx, 0x0c, &data);
	if (status != HAL_OK) return false;
	uint8_t channelBit = 0x01 << gpio_nbr;
	data &= ~channelBit;
	if (output_state == true) data |= channelBit;
	if (gpio_nbr == 2) {
		data &= ~(0x01 << 6);  // reset bit 6
	} else if (gpio_nbr == 3) {
		data &= ~(0x01 << 7);  // reset bit 7
	}
	status = _writeRegister(dev_ctx, 0x0c, data);
	if (status != HAL_OK) return false;

	return true;
}
