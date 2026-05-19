#ifndef __BSP_SPI_FLASH_H
#define __BSP_SPI_FLASH_H

#include "main.h"

// 声明外部的 SPI 句柄 (CubeMX 生成在 spi.c 中)
extern SPI_HandleTypeDef hspi1;

// W25Q64 容量参数 (8MB)
#define W25Q64_Flash_Size   (8 * 1024 * 1024)
#define W25Q64_Sector_Size  4096
#define W25Q64_Page_Size    256

// W25Q 核心指令表
#define W25X_WriteEnable        0x06 
#define W25X_ReadStatusReg      0x05 
#define W25X_ReadData           0x03 
#define W25X_PageProgram        0x02 
#define W25X_SectorErase        0x20 
#define W25X_ChipErase          0xC7 
#define W25X_JedecDeviceID      0x9F 

// 片选操作宏 (利用 CubeMX 生成的标签)
#define FLASH_CS_ENABLE()   HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET)
#define FLASH_CS_DISABLE()  HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET)

// 外部可调用的神兵利器
uint32_t SPI_FLASH_ReadID(void);
void SPI_FLASH_SectorErase(uint32_t SectorAddr);
void SPI_FLASH_BufferRead(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);
void SPI_FLASH_BufferWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);

#endif
