/*---------------------------------------------------------------------------------------------------
 *  Copyright (c) 2026 Tomasz Jastrzębski. All rights reserved.
 *-------------------------------------------------------------------------------------------------*/

#include "ads1x2s14_drv.h"

#include "dwt_timer.h"
#include "main.h"

#define DEVICE_ID 0x00
#define REVISION_ID 0x01
#define STATUS_MSB 0x02
#define STATUS_LSB 0x03
#define CONVERSION_CTRL 0x04
#define DEVICE_CFG 0x05
#define DATA_RATE_CFG 0x06
#define MUX_CFG 0x07
#define GAIN_CFG 0x08
#define REFERENCE_CFG 0x09
#define DIGITAL_CFG 0x0a
#define GPIO_CFG 0x0b
#define GPIO_DATA_OUTPUT 0x0c
#define IDAC_MAG_CFG 0x0d
#define IDAC_MUX_CFG 0x0e
#define REG_MAP_CRC 0x0f

#define SET_BIT_STATE(value, bit, state) ((value) = ((value) & ~(1u << (bit))) | ((!!(state)) << (bit)))

static ads1x2s14_resolution _resolution = ads1x2s14_resolution_unknown;  // TODO: keep resolution in user device state/info

// https://www.ti.com/lit/ds/symlink/ads112s14.pdf
// max CLK frequency is 16.67 MHz (see: 5.6 Timing Requirements)

static void _chipSelect(spi_channel_dev_ctx* spi) {
	HAL_GPIO_WritePin(spi->cs_port, spi->cs_pin, GPIO_PIN_RESET);
	dwt_delay(1);
}

static void _chipDeSelect(spi_channel_dev_ctx* spi) {
	HAL_GPIO_WritePin(spi->cs_port, spi->cs_pin, GPIO_PIN_SET);
}

static HAL_StatusTypeDef _readRegister(spi_channel_dev_ctx* dev_ctx, uint8_t regAddress, uint8_t* data) {
	// 7.5.4 Device Commands
	// 7.5.4.2 Read Register Command
	HAL_StatusTypeDef result;
	uint16_t rxData = 0;
	uint16_t txData = 0;

	spi_drv_configureSpi(dev_ctx, spi_drv_direction_fullDuplex, spi_drv_mode_1);

	txData = (regAddress & 0xf);
	txData |= 0x40;
	_chipSelect(dev_ctx);
	result = HAL_SPI_TransmitReceive(dev_ctx->channel, (uint8_t*)&txData, (uint8_t*)&txData, 2, 1000);
	_chipDeSelect(dev_ctx);
	if (result != HAL_OK) return result;
	txData = 0;
	_chipSelect(dev_ctx);
	result = HAL_SPI_TransmitReceive(dev_ctx->channel, (uint8_t*)&txData, (uint8_t*)&rxData, 2, 1000);
	if (result != HAL_OK) return result;
	*data = rxData;  // registry value is in the first received byte (Figure 7-23)
	return result;
}

static HAL_StatusTypeDef _writeRegister(spi_channel_dev_ctx* dev_ctx, uint8_t regAddress, uint8_t data) {
	// 7.5.4 Device Commands
	// 7.5.4.3 Write Register Command
	HAL_StatusTypeDef result;
	uint16_t txData = 0;
	uint16_t rxData = 0;

	spi_drv_configureSpi(dev_ctx, spi_drv_direction_fullDuplex, spi_drv_mode_1);

	_chipSelect(dev_ctx);
	result = HAL_SPI_TransmitReceive(dev_ctx->channel, (uint8_t*)&txData, (uint8_t*)&rxData, 2, 1000);
	_chipDeSelect(dev_ctx);
	if (result != HAL_OK) return result;
	txData = (regAddress & 0xf);
	txData |= 0x80;
	txData |= (data << 8);
	_chipSelect(dev_ctx);
	result = HAL_SPI_TransmitReceive(dev_ctx->channel, (uint8_t*)&txData, (uint8_t*)&rxData, 2, 1000);
	_chipDeSelect(dev_ctx);
	return result;
}

ads1x2s14_resolution ads1x2s14_init(spi_channel_dev_ctx* dev_ctx) {
	HAL_StatusTypeDef result;
	uint8_t data = 0;

	_resolution = ads1x2s14_resolution_unknown;

	// 8.5 CONVERSION_CTRL Register (Address = 04h) [Reset = 00h]
	// reset device
	result = _writeRegister(dev_ctx, CONVERSION_CTRL, 0b010110 << 2);
	if (result != HAL_OK) return ads1x2s14_resolution_unknown;

	// 5.6 Timing Requirements
	dwt_delay(500);  // wait 500 us

	// 8.1 DEVICE_ID Register (Address = 00h)
	// read device ID
	result = _readRegister(dev_ctx, DEVICE_ID, &data);
	if (result != HAL_OK) return ads1x2s14_resolution_unknown;
	data &= 0x0f;

	if (data == 0b1010) {
		// 16 bit device
		_resolution = ads1x2s14_resolution_16b;
	} else if (data == 0b1011) {
		// 24 bit device
		_resolution = ads1x2s14_resolution_24b;
	} else {
		return ads1x2s14_resolution_unknown;
	}

	// 8.2 REVISION_ID Register (Address = 01h) [Reset = XXh]
	result = _readRegister(dev_ctx, REVISION_ID, &data);
	if (result != HAL_OK) return ads1x2s14_resolution_unknown;

	// 8.3 STATUS_MSB Register (Address = 02h) [Reset = 3Eh]
	result = _readRegister(dev_ctx, STATUS_MSB, &data);
	if (result != HAL_OK) return ads1x2s14_resolution_unknown;
	if (data != 0x3e) return ads1x2s14_resolution_unknown;

	// 8.4 STATUS_LSB Register (Address = 03h) [Reset = F0h]
	result = _readRegister(dev_ctx, STATUS_LSB, &data);
	if (result != HAL_OK) return ads1x2s14_resolution_unknown;
	if (data != 0xf0) return ads1x2s14_resolution_unknown;

	// reset RESETn and all fault flags
	result = _writeRegister(dev_ctx, STATUS_MSB, 0b11111110);
	if (result != HAL_OK) return ads1x2s14_resolution_unknown;

	return _resolution;
}

// 8 Registers

bool ads1x2s14_gpio_config(spi_channel_dev_ctx* dev_ctx, uint8_t gpio_nbr, ads1x2s14_gpio gpio_config) {
	// 8.12 GPIO_CFG Register (Address = 0Bh) [Reset = 00h]
	assert_param(gpio_nbr < 4);
	HAL_StatusTypeDef result;
	uint8_t data = 0;

	result = _readRegister(dev_ctx, GPIO_CFG, &data);
	if (result != HAL_OK) return false;

	uint8_t channelBits = 0x03 << (gpio_nbr * 2);
	data &= ~channelBits;
	data |= (gpio_config << (gpio_nbr * 2));

	result = _writeRegister(dev_ctx, GPIO_CFG, data);
	if (result != HAL_OK) return false;

	return true;
}

bool ads1x2s14_gpio_setState(spi_channel_dev_ctx* dev_ctx, uint8_t gpio_nbr, bool output_state) {
	// 8.13 GPIO_DATA_OUTPUT Register (Address = 0Ch) [Reset = 00h]
	assert_param(gpio_nbr < 4);
	HAL_StatusTypeDef result;
	uint8_t data = 0;

	result = _readRegister(dev_ctx, GPIO_DATA_OUTPUT, &data);
	if (result != HAL_OK) return false;

	uint8_t channelBit = 0x01 << gpio_nbr;
	data &= ~channelBit;
	if (output_state == true) data |= channelBit;
	if (gpio_nbr == 2) {
		data &= ~(0x01 << 6);  // reset bit 6
	} else if (gpio_nbr == 3) {
		data &= ~(0x01 << 7);  // reset bit 7
	}

	result = _writeRegister(dev_ctx, GPIO_DATA_OUTPUT, data);
	if (result != HAL_OK) return false;

	return true;
}

bool ads1x2s14_pga_setGain(spi_channel_dev_ctx* dev_ctx, ads1x2s14_gain gain) {
	// 8.9 GAIN_CFG Register (Address = 08h) [Reset = 01h]
	HAL_StatusTypeDef result;
	uint8_t data = gain;
	// TODO: allow for SYS_MON setup
	result = _writeRegister(dev_ctx, GAIN_CFG, data);
	if (result != HAL_OK) return false;

	return true;
}

bool ads1x2s14_mux_select(spi_channel_dev_ctx* dev_ctx, ads1x2s14_mux ainp, ads1x2s14_mux ainn) {
	// 8.8 MUX_CFG Register (Address = 07h) [Reset = 01h]
	HAL_StatusTypeDef result;
	uint8_t data = ((ainp << 4) | ainn);

	result = _writeRegister(dev_ctx, MUX_CFG, data);
	if (result != HAL_OK) return false;

	return true;
}

bool ads1x2s14_dev_config(spi_channel_dev_ctx* dev_ctx, bool pwdDown, bool stdby, ads1x2s14_cnvMode cnvMode, ads1x2s14_speed speed) {
	// 8.6 DEVICE_CFG Register (Address = 05h) [Reset = 00h]
	HAL_StatusTypeDef result;
	uint8_t data = 0;

	result = _readRegister(dev_ctx, DEVICE_CFG, &data);
	if (result != HAL_OK) return false;

	SET_BIT_STATE(data, 7, pwdDown);
	SET_BIT_STATE(data, 6, stdby);
	SET_BIT_STATE(data, 2, cnvMode);
	data &= ~0x03;
	data |= speed;

	result = _writeRegister(dev_ctx, DEVICE_CFG, data);
	if (result != HAL_OK) return false;

	return true;
}

bool ads1x2s14_cnv_start(spi_channel_dev_ctx* dev_ctx) {
	// 8.5 CONVERSION_CTRL Register (Address = 04h) [Reset = 00h]
	uint8_t data = 2;

	HAL_StatusTypeDef result = _writeRegister(dev_ctx, CONVERSION_CTRL, data);
	if (result != HAL_OK) return false;

	return true;
}

bool ads1x2s14_cnv_stop(spi_channel_dev_ctx* dev_ctx) {
	// 8.5 CONVERSION_CTRL Register (Address = 04h) [Reset = 00h]
	uint8_t data = 1;

	HAL_StatusTypeDef result = _writeRegister(dev_ctx, CONVERSION_CTRL, data);
	if (result != HAL_OK) return false;

	return true;
}

bool ads1x2s14_reset(spi_channel_dev_ctx* dev_ctx) {
	// 8.5 CONVERSION_CTRL Register (Address = 04h) [Reset = 00h]
	uint8_t data = 0b010110 << 2;

	HAL_StatusTypeDef result = _writeRegister(dev_ctx, CONVERSION_CTRL, data);
	if (result != HAL_OK) return false;

	return true;
}

bool ads1x2s14_ref_set(spi_channel_dev_ctx* dev_ctx, ads1x2s14_ref ref) {
	// 8.10 REFERENCE_CFG Register (Address = 09h) [Reset = 00h]
	assert_param(ref < 2);  // ext ref currently not supported
	HAL_StatusTypeDef result;
	uint8_t data = 0;

	data &= ~0x03;  // reset bits 1:0 for internal reference
	SET_BIT_STATE(data, 2, ref);

	result = _writeRegister(dev_ctx, REFERENCE_CFG, data);
	if (result != HAL_OK) return false;

	return true;
}

bool ads1x2s14_digital_config(spi_channel_dev_ctx* dev_ctx, ads1x2s14_coding coding, bool enableStatusHdr, bool enableSpiCrc, bool enableRegCrc, bool cntRead, bool sdoDualMode) {
	// 8.10 8.11 DIGITAL_CFG Register (Address = 0Ah) [Reset = 00h]
	HAL_StatusTypeDef result;
	uint8_t data = 0;

	result = _readRegister(dev_ctx, DIGITAL_CFG, &data);
	if (result != HAL_OK) return false;

	SET_BIT_STATE(data, 6, enableRegCrc);
	SET_BIT_STATE(data, 5, enableSpiCrc);
	SET_BIT_STATE(data, 4, enableStatusHdr);
	SET_BIT_STATE(data, 2, cntRead);
	SET_BIT_STATE(data, 1, coding);
	SET_BIT_STATE(data, 0, sdoDualMode);

	result = _writeRegister(dev_ctx, DIGITAL_CFG, data);
	if (result != HAL_OK) return false;

	return true;
}

bool ads1x2s14_dataRate_config(spi_channel_dev_ctx* dev_ctx, bool globalChop, ads1x2s14_filter filter, ads1x2s14_delay delay) {
	// 8.7 DATA_RATE_CFG Register (Address = 06h) [Reset = 00h]
	uint8_t data = 0;

	SET_BIT_STATE(data, 3, globalChop);
	data |= (delay << 4);
	data |= filter;

	HAL_StatusTypeDef result = _writeRegister(dev_ctx, DATA_RATE_CFG, data);
	if (result != HAL_OK) return false;

	return true;
}

bool ads1x2s14_data_read(spi_channel_dev_ctx* dev_ctx, uint32_t* rxData) {
	// 7.5.4.1 No Operation (Read Conversion Data)
	HAL_StatusTypeDef result;
	const uint32_t txData = 0;

	spi_drv_configureSpi(dev_ctx, spi_drv_direction_fullDuplex, spi_drv_mode_1);
	_chipSelect(dev_ctx);
	result = HAL_SPI_TransmitReceive(dev_ctx->channel, (uint8_t*)&txData, (uint8_t*)rxData, _resolution, 1000);
	_chipDeSelect(dev_ctx);
	if (result != HAL_OK) return false;

	*rxData = __REV(*rxData);  // reverse byte order back to little-endian

	if (_resolution == ads1x2s14_resolution_16b) {
		*rxData >>= 16;
	} else if (_resolution == ads1x2s14_resolution_24b) {
		*rxData >>= 8;
	} else {
		*rxData = 0;
	}

	return true;
}

void ads1x2s14_status_decode(uint16_t data, ads1x2s14_status* status) {
	uint8_t lsb = data;
	uint8_t msb = data >> 8;
	status->reset_occured = !(msb & (0b1 << 7));
	status->avdd_uv = !(msb & (0b1 << 6));
	status->ref_uv = !(msb & (0b1 << 5));
	status->spi_crc_fault = !(msb & (0b1 << 4));
	status->reg_crc_fault = !(msb & (0b1 << 3));
	status->mem_fault = !(msb & (0b1 << 2));
	status->reg_write_fault = !(msb & (0b1 << 1));
	status->data_ready = (msb & (0b1 << 0));
	status->conv_count = (lsb >> 4) & 0x0f;
	status->gpio3_dat_in = (lsb & (0b1 << 3));
	status->gpio2_dat_in = (lsb & (0b1 << 2));
	status->gpio1_dat_in = (lsb & (0b1 << 1));
	status->gpio0_dat_in = (lsb & (0b1 << 0));
}

bool ads1x2s14_status_read(spi_channel_dev_ctx* dev_ctx, ads1x2s14_status* status) {
	// 8.3 STATUS_MSB Register (Address = 02h) [Reset = 3Eh]
	// 8.4 STATUS_LSB Register (Address = 03h) [Reset = F0h]
	uint8_t lsb;
	uint8_t msb;
	HAL_StatusTypeDef result;

	result = _readRegister(dev_ctx, STATUS_LSB, &lsb);
	if (result != HAL_OK) return false;

	result = _readRegister(dev_ctx, STATUS_MSB, &msb);
	if (result != HAL_OK) return false;

	ads1x2s14_status_decode((msb << 8) | lsb, status);

	return true;
}