#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "spi_drv.h"

#define M95P32_PAGESIZE 512U
#define M95P32_PAGECOUNT 8192U
#define M95P32_SECTORCOUNT 1024U
#define M95P32_BLOCKCOUNT 64U

HAL_StatusTypeDef m95p32_Write(spi_channel_dev_ctx* spi, uint8_t* pData, uint32_t TarAddr, uint32_t Size);
HAL_StatusTypeDef m95p32_Program(spi_channel_dev_ctx* spi, uint8_t* pData, uint32_t TarAddr, uint32_t Size);
HAL_StatusTypeDef m95p32_Read(spi_channel_dev_ctx* spi, uint8_t* pData, uint32_t TarAddr, uint32_t Size);
HAL_StatusTypeDef m95p32_WriteVolatileRegister(spi_channel_dev_ctx* spi, uint8_t regVal);
HAL_StatusTypeDef m95p32_ReadVolatileRegister(spi_channel_dev_ctx* spi, uint8_t* pData);
HAL_StatusTypeDef m95p32_ReadJEDEC(spi_channel_dev_ctx* spi, uint8_t* pData, uint32_t byteCount);

HAL_StatusTypeDef m95p32_Page_Erase(spi_channel_dev_ctx* spi, uint32_t address);
HAL_StatusTypeDef m95p32_Sector_Erase(spi_channel_dev_ctx* spi, uint32_t address);
HAL_StatusTypeDef m95p32_Block_Erase(spi_channel_dev_ctx* spi, uint32_t address);
HAL_StatusTypeDef m95p32_Chip_Erase(spi_channel_dev_ctx* spi);

HAL_StatusTypeDef m95p32_Read_ID(spi_channel_dev_ctx* spi, uint8_t* pData, uint32_t address, uint32_t byteCount);
HAL_StatusTypeDef m95p32_Write_ID(spi_channel_dev_ctx* spi, uint8_t* pData, uint32_t address, uint32_t byteCount);

#ifdef __cplusplus
}
#endif
