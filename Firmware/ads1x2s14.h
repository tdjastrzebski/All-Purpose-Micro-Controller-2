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

enum ads1x2s14_gain : uint8_t {
	ads1x2s14_gain_05 = 0b0000,
	ads1x2s14_gain_1 = 0b0001,
	ads1x2s14_gain_2 = 0b0010,
	ads1x2s14_gain_4 = 0b0011,
	ads1x2s14_gain_5 = 0b0100,
	ads1x2s14_gain_8 = 0b0101,
	ads1x2s14_gain_10 = 0b0110,
	ads1x2s14_gain_16 = 0b0111,
	ads1x2s14_gain_20 = 0b1000,
	ads1x2s14_gain_32 = 0b1001,
	ads1x2s14_gain_50 = 0b1010,
	ads1x2s14_gain_64 = 0b1011,
	ads1x2s14_gain_100 = 0b1100,
	ads1x2s14_gain_128 = 0b1101,
	ads1x2s14_gain_200 = 0b1110,
	ads1x2s14_gain_256 = 0b1111,
};

enum ads1x2s14_mux : uint8_t {
	ads1x2s14_mux_ain0 = 0b0000,
	ads1x2s14_mux_ain1 = 0b0001,
	ads1x2s14_mux_ain2 = 0b0010,
	ads1x2s14_mux_ain3 = 0b0011,
	ads1x2s14_mux_ain4 = 0b0100,
	ads1x2s14_mux_ain5 = 0b0101,
	ads1x2s14_mux_ain6 = 0b0110,
	ads1x2s14_mux_ain7 = 0b0111,
	ads1x2s14_mux_gnd = 0b1000,
};

enum ads1x2s14_cnvMode : bool {
	ads1x2s14_cnvMode_continuous = false,
	ads1x2s14_cnvMode_singleShot = true,
};

enum ads1x2s14_intRef : bool {
	ads1x2s14_intRef_1_25V = false,
	ads1x2s14_intRef_2_5V = true,
};

enum ads1x2s14_speed : uint8_t {
	ads1x2s14_speed_32kHz = 0b00,
	ads1x2s14_speed_256kHz = 0b01,
	ads1x2s14_speed_512kHz = 0b10,
	ads1x2s14_speed_1024kHz = 0b11,
};

enum ads1x2s14_coding : bool {
	ads1x2s14_coding_2_complement = false,
	ads1x2s14_coding_unipolar = true,
};

enum ads1x2s14_filter : uint8_t {
	ads1x2s14_filter_16 = 0b000,     // (Sinc4 OSR = 16)
	ads1x2s14_filter_32 = 0b001,     // (Sinc4 OSR = 32)
	ads1x2s14_filter_128 = 0b010,    // (Sinc4 OSR = 32, Sinc1 OSR = 4)
	ads1x2s14_filter_256 = 0b011,    // (Sinc4 OSR = 32, Sinc1 OSR = 8)
	ads1x2s14_filter_512 = 0b100,    // (Sinc4 OSR = 32, Sinc1 OSR = 16)
	ads1x2s14_filter_1024 = 0b101,   // (Sinc4 OSR = 32, Sinc1 OSR =32)
	ads1x2s14_filter_25sps = 0b110,  // 25SPS (independent of speed mode)
	ads1x2s14_filter_20sps = 0b111,  // 20SPS (independent of speed mode
};

enum ads1x2s14_delay : uint8_t {
	ads1x2s14_delay_0 = 0b0000,      // 0x tMOD
	ads1x2s14_delay_1 = 0b0001,      // 1x tMOD
	ads1x2s14_delay_2 = 0b0010,      // 2x tMOD
	ads1x2s14_delay_4 = 0b0011,      // 4x tMOD
	ads1x2s14_delay_8 = 0b0100,      // 8x tMOD
	ads1x2s14_delay_16 = 0b0101,     // 16x tMOD
	ads1x2s14_delay_32 = 0b0110,     // 32x tMOD
	ads1x2s14_delay_64 = 0b0111,     // 64x tMOD
	ads1x2s14_delay_128 = 0b1000,    // 128x tMOD
	ads1x2s14_delay_256 = 0b1001,    // 256x tMOD
	ads1x2s14_delay_512 = 0b1010,    // 512x tMOD
	ads1x2s14_delay_1024 = 0b1011,   // 1024x tMOD
	ads1x2s14_delay_2048 = 0b1100,   // 2048x tMOD
	ads1x2s14_delay_4096 = 0b1101,   // 4096x tMOD
	ads1x2s14_delay_8192 = 0b1110,   // 8192x tMOD
	ads1x2s14_delay_16384 = 0b1111,  // 16384x tMOD
};

enum ads1x2s14_resolution : bool {
	ads1x2s14_resolution_16b = false,
	ads1x2s14_resolution_24b = true,
};

bool ads1x2s14_init(spi_channel_dev_ctx* dev_ctx);
bool ads1x2s14_gpio_config(spi_channel_dev_ctx* dev_ctx, uint8_t gpio_nbr, ads1x2s14_gpio gpio_config);
bool ads1x2s14_gpio_setState(spi_channel_dev_ctx* dev_ctx, uint8_t gpio_nbr, bool output_state);
bool ads1x2s14_pga_setGain(spi_channel_dev_ctx* dev_ctx, ads1x2s14_gain gain);
bool ads1x2s14_mux_select(spi_channel_dev_ctx* dev_ctx, ads1x2s14_mux ainp, ads1x2s14_mux ainn = ads1x2s14_mux_gnd);
bool ads1x2s14_device_config(spi_channel_dev_ctx* dev_ctx, bool pwdDown, bool stdby, ads1x2s14_cnvMode cnvMode, ads1x2s14_speed speed);
bool ads1x2s14_cnv_start(spi_channel_dev_ctx* dev_ctx);
bool ads1x2s14_cnv_stop(spi_channel_dev_ctx* dev_ctx);
bool ads1x2s14_reset(spi_channel_dev_ctx* dev_ctx);
bool ads1x2s14_intRef_set(spi_channel_dev_ctx* dev_ctx, ads1x2s14_intRef intRef);
bool ads1x2s14_digital_config(spi_channel_dev_ctx* dev_ctx, ads1x2s14_coding coding, bool cntRead = false, bool enableRegCrc = false, bool enableSpiCrc = false, bool enableStatusHdr = false);
bool ads1x2s14_dataRate_config(spi_channel_dev_ctx* dev_ctx, bool globalChop, ads1x2s14_filter filter, ads1x2s14_delay delay);
// bool ads1x2s14_status_read(spi_channel_dev_ctx* dev_ctx); // TODO: read STATUS_LSB and STATUS_MSB
bool ads1x2s14_data_read(spi_channel_dev_ctx* dev_ctx, uint8_t byteCount, uint8_t* data);

#ifdef __cplusplus
}
#endif