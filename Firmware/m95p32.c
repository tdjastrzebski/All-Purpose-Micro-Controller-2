#include "m95p32.h"

#include <assert.h>

#include "main.h"
#include "spi_drv.h"

#define CMD_ERASE_SECTOR 0x20
#define CMD_ERASE_BLOCK 0xD8
#define CMD_ERASE_CHIP 0XC7
#define CMD_ERASE_PAGE 0xDB
#define CMD_PROG_PAGE 0x0A
#define CMD_WRITE_PAGE 0x02
#define CMD_WREN 0x06
#define CMD_WRDI 0x04
#define CMD_READ_STATUS_REG 0x05
#define CMD_READ_CONF_SAFETY_REG 0x15
#define CMD_CLEAR_SAFETY_REG 0x50
#define CMD_READ_VOLATILE_REG 0x85
#define CMD_WRITE_VOLATILE_REG 0x81
#define CMD_WRITE_STATUS_CONF_REG 0x01
#define CMD_READ_DATA 0x03
#define CMD_FAST_READ_SINGLE 0x0B
#define CMD_FAST_READ_DUAL 0x3B
#define CMD_FAST_READ_QUAD 0x6B
#define CMD_READ_ID_PAGE 0x83
#define CMD_FAST_READ_ID_PAGE 0x8B
#define CMD_WRITE_ID_PAGE 0x82
#define CMD_DEEP_POWER_DOWN 0xB9
#define CMD_DEEP_POWER_DOWN_RELEASE 0xAB
#define CMD_READ_JEDEC 0x9F
#define CMD_READ_SFDP 0x5A
#define CMD_ENABLE_RESET 0x66
#define CMD_SOFT_RESET 0x99

#define DUMMY_DATA 0xFFU

/*
The memory array configuration is organized as:
• 4 194 304 bytes (32 Mbits)
• 64 blocks of 64 Kbytes, or 1024 sectors of 4 Kbytes, or 8192 pages of 512 bytes, hence:
– each block contains 16 sectors
– each sector contains 8 pages
– each page contains 512 bytes
source: https://www.st.com/resource/en/datasheet/m95p32-i.pdf
*/

static uint8_t _cmdBuff[5];

static void _chipSelect(spi_channel_dev_ctx* spi) {
	HAL_GPIO_WritePin(spi->cs_port, spi->cs_pin, GPIO_PIN_RESET);
}

static void _chipDeSelect(spi_channel_dev_ctx* spi) {
	HAL_GPIO_WritePin(spi->cs_port, spi->cs_pin, GPIO_PIN_SET);
}

static HAL_StatusTypeDef _writeRead(spi_channel_dev_ctx* spi, uint8_t* writeData, uint16_t writeLength, uint8_t* readData, uint16_t readLength) {
	HAL_StatusTypeDef status;

	_chipSelect(spi);

	if (writeLength > 0) {
		status = HAL_SPI_Transmit(spi->channel, writeData, writeLength, 1000);

		if (status != HAL_OK) {
			_chipDeSelect(spi);
			return status;
		}
	}

	if (readLength > 0) {
		status = HAL_SPI_Receive(spi->channel, readData, readLength, 1000);
	}

	_chipDeSelect(spi);

	return status;
}

static HAL_StatusTypeDef _writeWithPooling(spi_channel_dev_ctx* spi, uint8_t* data1, uint16_t length1, uint8_t* data2, uint16_t length2, uint8_t delay) {
	HAL_StatusTypeDef status;

	_chipSelect(spi);

	if (length1 > 0) {
		status = HAL_SPI_Transmit(spi->channel, data1, length1, 1000);

		if (status != HAL_OK) {
			_chipDeSelect(spi);
			return status;
		}
	}

	if (length2 > 0) {
		status = HAL_SPI_Transmit(spi->channel, data2, length2, 1000);

		if (status != HAL_OK) {
			_chipDeSelect(spi);
			return status;
		}
	}

	uint8_t read_cmd = CMD_READ_STATUS_REG;
	uint8_t reg_val = 1;

	_chipDeSelect(spi);

	HAL_Delay(delay);

	_chipSelect(spi);
	status = HAL_SPI_Transmit(spi->channel, &read_cmd, 1, 1000);

	if (status != HAL_OK) {
		_chipDeSelect(spi);
		return status;
	}

	uint32_t tickstart = HAL_GetTick();

	while ((reg_val & 0x01U) != 0U) {
		status = HAL_SPI_Receive(spi->channel, &reg_val, 1, 1000);
		if (status != HAL_OK) break;
		if (HAL_GetTick() - tickstart > 1000) {
			status = HAL_TIMEOUT;
			break;
		}
	}

	_chipDeSelect(spi);

	return status;
}

void _setCmdAndAddress(uint8_t* buffer, uint8_t cmd, uint32_t address) {
	buffer[0] = cmd;
	// convert 24-bit address to big-endian order
	buffer[1] = (address >> 16) & 0xff;
	buffer[2] = (address >> 8) & 0xff;
	buffer[3] = address & 0xff;
}

/**
 * @brief  Write enable sets the write enable latch (WEL) bit in the Status register to a 1
 */
static HAL_StatusTypeDef _writeEnable(spi_channel_dev_ctx* spi) {
	_cmdBuff[0] = CMD_WREN;
	HAL_StatusTypeDef ret = _writeRead(spi, _cmdBuff, 1, nullptr, 0);
	return ret;
}

/**
 * @brief  Write disable resets the write enable latch (WEL) bit in the Status register to a 0
 */
static HAL_StatusTypeDef _writeDisable(spi_channel_dev_ctx* spi) {
	_cmdBuff[0] = CMD_WRDI;
	HAL_StatusTypeDef ret = _writeRead(spi, _cmdBuff, 1, nullptr, 0);
	return ret;
}

/**
 * @brief  The read data single output (READ) instruction allows one or more data bytes to be sequentially read from the memory
 * @note   no byteCount length limit
 */
HAL_StatusTypeDef m95p32_Read(spi_channel_dev_ctx* spi, uint8_t* pData, uint32_t address, uint32_t byteCount) {
	_setCmdAndAddress(_cmdBuff, CMD_READ_DATA, address);
	HAL_StatusTypeDef ret = _writeRead(spi, _cmdBuff, 4, pData, byteCount);
	return ret;
}

/**
 * @brief  Fast_Read allows one or more data bytes to be sequentially read from
 *         the memory by addition of eight dummy clocks after the 24-bit
 *         address it can operate at the highest possible frequency
 * @note   no byteCount length limit
 */
HAL_StatusTypeDef m95p32_FAST_Read(spi_channel_dev_ctx* spi, uint8_t* pData, uint32_t address, uint32_t byteCount) {
	_setCmdAndAddress(_cmdBuff, CMD_FAST_READ_SINGLE, address);
	_cmdBuff[4] = DUMMY_DATA;

	HAL_StatusTypeDef ret = _writeRead(spi, _cmdBuff, 5, pData, byteCount);
	return ret;
}

/**
 * @brief  Page write allows data to be written in a single instruction (auto erase + program)
 *         leaving the other bytes of the page unchanged
 * @note   byteCount <= 512
 */
HAL_StatusTypeDef m95p32_Page_Write(spi_channel_dev_ctx* spi, uint8_t* pData, uint32_t address, uint32_t byteCount) {
	assert(byteCount <= 512);
	_setCmdAndAddress(_cmdBuff, CMD_WRITE_PAGE, address);
	HAL_StatusTypeDef status = _writeWithPooling(spi, _cmdBuff, 4, pData, byteCount, 2);
	return status;
}

/**
 * @brief  The page program (PGPR) instruction allows from one to 512 bytes of data, initially in the erased state (FFh), to be programmed to 0 or any value.
 * @note   byteCount <= 512, page has to be in erased state (all bytes FFh).
 */
HAL_StatusTypeDef m95p32_Page_Prog(spi_channel_dev_ctx* spi, uint8_t* pData, uint32_t address, uint32_t byteCount) {
	assert(byteCount <= 512);
	_setCmdAndAddress(_cmdBuff, CMD_PROG_PAGE, address);
	HAL_StatusTypeDef status = _writeWithPooling(spi, _cmdBuff, 4, pData, byteCount, 1);
	return status;
}

/**
 * @brief  Page erase sets a page of 512 bytes within the device to the erased state of all 1s (FFh)
 */
HAL_StatusTypeDef m95p32_Page_Erase(spi_channel_dev_ctx* spi, uint32_t address) {
	HAL_StatusTypeDef ret = _writeEnable(spi);
	if (ret != HAL_OK) return ret;
	_setCmdAndAddress(_cmdBuff, CMD_ERASE_PAGE, address);
	ret = _writeWithPooling(spi, _cmdBuff, 4, nullptr, 0, 1);
	return ret;
}

/**
 * @brief  Sector erase sets all memory bits within a specified sector (4 Kbytes) to the erased state of all 1s(FFh)
 */
HAL_StatusTypeDef m95p32_Sector_Erase(spi_channel_dev_ctx* spi, uint32_t address) {
	HAL_StatusTypeDef ret = _writeEnable(spi);
	if (ret != HAL_OK) return ret;
	_setCmdAndAddress(_cmdBuff, CMD_ERASE_SECTOR, address);
	ret = _writeWithPooling(spi, _cmdBuff, 4, nullptr, 0, 1);
	return ret;
}

/**
 * @brief  Block erase sets all memory bits within a specified block (64 Kbytes) to the erased state of all 1s(FFh)
 */
HAL_StatusTypeDef m95p32_Block_Erase(spi_channel_dev_ctx* spi, uint32_t address) {
	HAL_StatusTypeDef ret = _writeEnable(spi);
	if (ret != HAL_OK) return ret;
	_setCmdAndAddress(_cmdBuff, CMD_ERASE_BLOCK, address);
	ret = _writeWithPooling(spi, _cmdBuff, 4, nullptr, 0, 4);
	return ret;
}

/**
 * @brief  Chip erase sets all memory bits within the device to the erased state of all 1s(FFh)
 */
HAL_StatusTypeDef m95p32_Chip_Erase(spi_channel_dev_ctx* spi) {
	HAL_StatusTypeDef ret = _writeEnable(spi);
	if (ret != HAL_OK) return ret;
	_cmdBuff[0] = CMD_ERASE_CHIP;
	ret = _writeWithPooling(spi, _cmdBuff, 1, nullptr, 0, 15);
	return ret;
}

/**
 * @brief  Read identification allows one or more data bytes in the two identification pages (512 bytes each) to be sequentially read
 * @note   byteCount <= 512, the first three bytes (address 00h, 01h, and 02h) of the identification page can also be read
 * with the read JEDEC identification (JDID) instruction
 */
HAL_StatusTypeDef m95p32_Read_ID(spi_channel_dev_ctx* spi, uint8_t* pData, uint32_t address, uint32_t byteCount) {
	assert(byteCount <= 512);
	_setCmdAndAddress(_cmdBuff, CMD_READ_ID_PAGE, address);
	HAL_StatusTypeDef ret = _writeRead(spi, _cmdBuff, 4, pData, byteCount);
	return ret;
}

/**
 * @brief  The Read Volatile register allow the 8-bit Volatile register to be read
 */
HAL_StatusTypeDef m95p32_ReadVolatileRegister(spi_channel_dev_ctx* spi, uint8_t* pData) {
	_cmdBuff[0] = CMD_READ_VOLATILE_REG;
	HAL_StatusTypeDef ret = _writeRead(spi, _cmdBuff, 1, pData, 1);
	return ret;
}

/**
 * @brief  The Write volatile register allows the volatile register to be written.
 *         The writable volatile register bits include BUFEN and BUFLD
 */
HAL_StatusTypeDef m95p32_WriteVolatileRegister(spi_channel_dev_ctx* spi, uint8_t regVal) {
	_cmdBuff[0] = CMD_WRITE_VOLATILE_REG;
	_cmdBuff[1] = regVal;
	HAL_StatusTypeDef ret = _writeWithPooling(spi, _cmdBuff, 2, nullptr, 0, 4);
	return ret;
}

/**
 * @brief  The RDCR reads the two bytes of Configuration and Safety registers (one for each register)
 */
HAL_StatusTypeDef m95p32_ReadConfigReg(spi_channel_dev_ctx* spi, uint8_t* pData, uint32_t byteCount) {
	assert(byteCount <= 2);
	_cmdBuff[0] = CMD_READ_CONF_SAFETY_REG;
	HAL_StatusTypeDef ret = _writeRead(spi, _cmdBuff, 1, pData, byteCount);
	return ret;
}

/**
 * @brief  Write status register allows the status and configuration register to be written
 */
HAL_StatusTypeDef m95p32_Write_StatusConfigReg(spi_channel_dev_ctx* spi, uint8_t* pData, uint32_t byteCount) {
	assert(byteCount <= 2);
	_cmdBuff[0] = CMD_WRITE_STATUS_CONF_REG;
	HAL_StatusTypeDef ret = _writeWithPooling(spi, _cmdBuff, 1, pData, byteCount, 4);
	return ret;
}

/**
 * @brief  The Clear Safety register resets all the bits of the Safety register
 */
HAL_StatusTypeDef ClearSafetyFlag(spi_channel_dev_ctx* spi) {
	_cmdBuff[0] = CMD_CLEAR_SAFETY_REG;
	HAL_StatusTypeDef ret = _writeRead(spi, _cmdBuff, 1, nullptr, 0);
	return ret;
}

/**
 * @brief  The Read SFDP allows the SFDP register format to be read
 */
HAL_StatusTypeDef Read_SFDP(spi_channel_dev_ctx* spi, uint8_t* pData, uint32_t address, uint32_t byteCount) {
	assert(byteCount <= 512);
	_setCmdAndAddress(_cmdBuff, CMD_READ_SFDP, address);
	_cmdBuff[4] = DUMMY_DATA;
	HAL_StatusTypeDef ret = _writeRead(spi, _cmdBuff, 5, pData, byteCount);
	return ret;
}

/**
 * @brief  Write identification page instruction (WRID) allows the identification page to be written
 */
HAL_StatusTypeDef m95p32_Write_ID(spi_channel_dev_ctx* spi, uint8_t* pData, uint32_t address, uint32_t byteCount) {
	assert(byteCount <= 512);
	// a write enable (06h) instruction must have been executed first
	_setCmdAndAddress(_cmdBuff, CMD_WRITE_ID_PAGE, address);
	HAL_StatusTypeDef ret = _writeWithPooling(spi, _cmdBuff, 4, pData, byteCount, 4);
	return ret;
}

/**
 * @brief  The deep power-down enter allows to put the device in a very low
 *         consumption state in which a limited number of commands are available
 */
HAL_StatusTypeDef Deep_Power_Down(spi_channel_dev_ctx* spi) {
	_cmdBuff[0] = CMD_DEEP_POWER_DOWN;
	HAL_StatusTypeDef ret = _writeRead(spi, _cmdBuff, 1, nullptr, 0);
	return ret;
}

/**
 * @brief  The deep power-down release allows to release the device from the
 *         deep power-down state to a standby-mode state
 */
HAL_StatusTypeDef Deep_Power_Down_Release(spi_channel_dev_ctx* spi) {
	_cmdBuff[0] = CMD_DEEP_POWER_DOWN_RELEASE;
	HAL_StatusTypeDef ret = _writeRead(spi, _cmdBuff, 1, nullptr, 0);
	return ret;
}

/**
 * @brief  The JEDEC identification allows to read in loop mode the three
 *         device identification bytes in loop mode
 */
HAL_StatusTypeDef m95p32_ReadJEDEC(spi_channel_dev_ctx* spi, uint8_t* pData, uint32_t byteCount) {
	assert(byteCount <= 3);
	_cmdBuff[0] = CMD_READ_JEDEC;
	HAL_StatusTypeDef ret = _writeRead(spi, _cmdBuff, 1, pData, byteCount);
	return ret;
}

/**
 * @brief  The enable reset initiate the reset the device
 */
HAL_StatusTypeDef Reset_Enable(spi_channel_dev_ctx* spi) {
	_cmdBuff[0] = CMD_ENABLE_RESET;
	HAL_StatusTypeDef ret = _writeRead(spi, _cmdBuff, 1, nullptr, 0);
	return ret;
}

/**
 * @brief  The Software reset initiate the reset the device
 */
HAL_StatusTypeDef Soft_Reset(spi_channel_dev_ctx* spi) {
	_cmdBuff[0] = CMD_SOFT_RESET;
	HAL_StatusTypeDef ret = _writeRead(spi, _cmdBuff, 1, nullptr, 0);
	return ret;
}

/**
 * @brief  Read status register allow the 8-bit Status registers to be read
 */
HAL_StatusTypeDef Read_StatusReg(spi_channel_dev_ctx* spi, uint8_t* pData) {
	_cmdBuff[0] = CMD_READ_STATUS_REG;
	HAL_StatusTypeDef ret = _writeRead(spi, _cmdBuff, 1, pData, 1);
	return ret;
}

/**
 * @brief  Write any number of bytes anywhere, memory does not need to be erased first.
 */
HAL_StatusTypeDef m95p32_Write(spi_channel_dev_ctx* spi, uint8_t* pData, uint32_t address, uint32_t byteCount) {
	assert(spi);
	assert(pData);

	HAL_StatusTypeDef ret = HAL_OK;
	uint32_t writeByteCount;

	/* Calculate the starting page and offset */
	uint32_t startOffset = address % M95P32_PAGESIZE;
	uint32_t offset = startOffset;

	/* Check WIP status bit*/
	_writeWithPooling(spi, nullptr, 0, nullptr, 0, 0);  // just pool without writing

	/* Iterate over the pages and write the data */
	while (byteCount > 0U) {
		writeByteCount = (byteCount < (M95P32_PAGESIZE - offset)) ? byteCount : (M95P32_PAGESIZE - offset);
		ret = _writeEnable(spi);
		if (ret != HAL_OK) break;
		ret = m95p32_Page_Write(spi, pData, address, writeByteCount);
		if (ret != HAL_OK) break;

		/* Update the pointers and sizes for the next page */
		pData += writeByteCount;
		byteCount -= writeByteCount;
		address += writeByteCount;
		offset = address % M95P32_PAGESIZE;
	}

	return ret;
}

/**
 * @brief  Program any number of bytes, anywhere, memory HAS to be erased first.
 */
HAL_StatusTypeDef m95p32_Program(spi_channel_dev_ctx* spi, uint8_t* pData, uint32_t address, uint32_t byteCount) {
	assert(spi);
	assert(pData);

	HAL_StatusTypeDef ret = HAL_OK;
	uint32_t writeByteCount;

	/* Calculate the starting page and offset */
	uint32_t startOffset = address % M95P32_PAGESIZE;
	uint32_t offset = startOffset;

	/* Check WIP status bit*/
	_writeWithPooling(spi, nullptr, 0, nullptr, 0, 0);  // just pool without writing

	/* Iterate over the pages and write the data */
	while (byteCount > 0U) {
		writeByteCount = (byteCount < (M95P32_PAGESIZE - offset)) ? byteCount : (M95P32_PAGESIZE - offset);

		ret = _writeEnable(spi);
		if (ret != HAL_OK) break;
		ret = m95p32_Page_Prog(spi, pData, address, writeByteCount);
		if (ret != HAL_OK) break;

		/* Update the pointers and sizes for the next page */
		pData += writeByteCount;
		byteCount -= writeByteCount;
		address += writeByteCount;
		offset = address % M95P32_PAGESIZE;
	}

	return ret;
}
