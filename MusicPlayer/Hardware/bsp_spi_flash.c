#include "bsp_spi_flash.h"
#include <stdio.h> // 用于 printf 报错

#include "main.h"

// ================= 底层基础函数 =================

// 收发一个字节 (SPI 的核心就是交换，发一个字节必定同时收一个字节)
static uint8_t SPI_FLASH_SendByte(uint8_t byte) {
    uint8_t rx_data = 0;
    HAL_SPI_TransmitReceive(&hspi1, &byte, &rx_data, 1, 1000);
    return rx_data;
}

// 开启写使能 (擦除和写入之前，必须先喊这一句暗号)
static void SPI_FLASH_WriteEnable(void) {
    FLASH_CS_ENABLE();
    SPI_FLASH_SendByte(W25X_WriteEnable);
    FLASH_CS_DISABLE();
}

// 极其关键的“判忙”函数 (老板在烧锅炉，必须等他烧完才能干下一步)
static void SPI_FLASH_WaitForWriteEnd(void) {
    uint8_t status = 0;
    FLASH_CS_ENABLE();
    SPI_FLASH_SendByte(W25X_ReadStatusReg);
    do {
        status = SPI_FLASH_SendByte(0xFF); // 发送空字节换取状态
    } while ((status & 0x01) == 1); // 如果最低位是 1，说明还在忙！死等！
    FLASH_CS_DISABLE();
}

// ================= 给用户调用的高阶函数 =================

// 1. 读取芯片 ID (用于验明正身)
uint32_t SPI_FLASH_ReadID(void) {
    uint32_t Temp = 0;
    FLASH_CS_ENABLE();
    SPI_FLASH_SendByte(W25X_JedecDeviceID);
    Temp = (Temp | SPI_FLASH_SendByte(0xFF)) << 8; // 产商 ID
    Temp = (Temp | SPI_FLASH_SendByte(0xFF)) << 8; // 内存类型
    Temp = (Temp | SPI_FLASH_SendByte(0xFF));      // 容量 ID
    FLASH_CS_DISABLE();
    return Temp;
}

// 2. 擦除扇区 (4096 字节为最小擦除单位)
void SPI_FLASH_SectorErase(uint32_t SectorAddr) {
    SPI_FLASH_WriteEnable();
    SPI_FLASH_WaitForWriteEnd(); // 先确认没在忙
    
    FLASH_CS_ENABLE();
    SPI_FLASH_SendByte(W25X_SectorErase);
    SPI_FLASH_SendByte((SectorAddr & 0xFF0000) >> 16);
    SPI_FLASH_SendByte((SectorAddr & 0xFF00) >> 8);
    SPI_FLASH_SendByte(SectorAddr & 0xFF);
    FLASH_CS_DISABLE();
    
    SPI_FLASH_WaitForWriteEnd(); // 擦除需要几十毫秒，必须等！
}

// 3. 读取数据 (多长都可以)
void SPI_FLASH_BufferRead(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead) {
    FLASH_CS_ENABLE();
    SPI_FLASH_SendByte(W25X_ReadData);
    SPI_FLASH_SendByte((ReadAddr & 0xFF0000) >> 16);
    SPI_FLASH_SendByte((ReadAddr & 0xFF00) >> 8);
    SPI_FLASH_SendByte(ReadAddr & 0xFF);
    while (NumByteToRead--) {
        *pBuffer = SPI_FLASH_SendByte(0xFF);
        pBuffer++;
    }
    FLASH_CS_DISABLE();
}

// 4. 页内写入内部调用 (一次最多 256 字节，绝不能跨页)
static void SPI_FLASH_PageWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite) {
    SPI_FLASH_WriteEnable();
    FLASH_CS_ENABLE();
    SPI_FLASH_SendByte(W25X_PageProgram);
    SPI_FLASH_SendByte((WriteAddr & 0xFF0000) >> 16);
    SPI_FLASH_SendByte((WriteAddr & 0xFF00) >> 8);
    SPI_FLASH_SendByte(WriteAddr & 0xFF);

    while (NumByteToWrite--) {
        SPI_FLASH_SendByte(*pBuffer);
        pBuffer++;
    }
    FLASH_CS_DISABLE();
    SPI_FLASH_WaitForWriteEnd(); // 写入需要时间，等！
}

// 5. 终极自适应无脑写入函数 (自动处理跨页计算，你随便传数据就行)
void SPI_FLASH_BufferWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite) {
    uint8_t NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;
    Addr = WriteAddr % W25Q64_Page_Size;
    count = W25Q64_Page_Size - Addr;
    NumOfPage =  NumByteToWrite / W25Q64_Page_Size;
    NumOfSingle = NumByteToWrite % W25Q64_Page_Size;

    if (Addr == 0) { // 地址刚好对齐一页
        if (NumOfPage == 0) {
            SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumByteToWrite);
        } else {
            while (NumOfPage--) {
                SPI_FLASH_PageWrite(pBuffer, WriteAddr, W25Q64_Page_Size);
                WriteAddr +=  W25Q64_Page_Size;
                pBuffer += W25Q64_Page_Size;
            }
            SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumOfSingle);
        }
    } else { // 地址没有对齐
        if (NumOfPage == 0) {
            if (NumOfSingle > count) { // 跨越了下一页
                temp = NumOfSingle - count;
                SPI_FLASH_PageWrite(pBuffer, WriteAddr, count);
                WriteAddr +=  count;
                pBuffer += count;
                SPI_FLASH_PageWrite(pBuffer, WriteAddr, temp);
            } else { // 没跨页
                SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumByteToWrite);
            }
        } else { // 跨越了多页
            NumByteToWrite -= count;
            NumOfPage =  NumByteToWrite / W25Q64_Page_Size;
            NumOfSingle = NumByteToWrite % W25Q64_Page_Size;

            SPI_FLASH_PageWrite(pBuffer, WriteAddr, count);
            WriteAddr +=  count;
            pBuffer += count;

            while (NumOfPage--) {
                SPI_FLASH_PageWrite(pBuffer, WriteAddr, W25Q64_Page_Size);
                WriteAddr +=  W25Q64_Page_Size;
                pBuffer += W25Q64_Page_Size;
            }
            if (NumOfSingle != 0) {
                SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumOfSingle);
            }
        }
    }
}
