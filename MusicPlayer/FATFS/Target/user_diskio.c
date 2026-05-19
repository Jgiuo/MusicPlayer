/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    user_diskio.c
 * @brief   FATFS to W25Q64 SPI Flash Driver Interface
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "ff_gen_drv.h"
#include "../../Hardware/bsp_spi_flash.h" // 引入你的 W25Q64 驱动
#include "FreeRTOS.h"      // 用于动态内存分配，防止爆栈

/* Private variables ---------------------------------------------------------*/
static volatile DSTATUS Stat = STA_NOINIT;

/* Private function prototypes -----------------------------------------------*/
DSTATUS USER_initialize (BYTE pdrv);
DSTATUS USER_status (BYTE pdrv);
DRESULT USER_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
#if _USE_WRITE == 1
  DRESULT USER_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
#endif 
#if _USE_IOCTL == 1
  DRESULT USER_ioctl (BYTE pdrv, BYTE cmd, void *buff);
#endif 

Diskio_drvTypeDef  USER_Driver =
{
  USER_initialize,
  USER_status,
  USER_read,
#if  _USE_WRITE
  USER_write,
#endif  
#if  _USE_IOCTL == 1
  USER_ioctl,
#endif 
};

/* Private functions ---------------------------------------------------------*/

// 1. 初始化 Flash
DSTATUS USER_initialize (BYTE pdrv)
{
    uint32_t id = SPI_FLASH_ReadID();
    // W25Q64 的 ID 通常是 0xEF4017，读取成功代表硬件通信正常
    if(id == 0xEF4017 || id != 0xFFFFFF) 
    {
        Stat &= ~STA_NOINIT; // 清除未初始化标志
        return RES_OK;
    }
    return RES_NOTRDY;
}

// 2. 获取状态
DSTATUS USER_status (BYTE pdrv)
{
    return Stat;
}

// 3. 读取扇区 (FATFS 扇区为 512 字节)
DRESULT USER_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
    if (Stat & STA_NOINIT) return RES_NOTRDY;

    // 物理地址 = FATFS 虚拟扇区号 * 512
    SPI_FLASH_BufferRead(buff, sector * 512, count * 512);
    
    return RES_OK;
}

// 4. 写入扇区 (核心：4KB 读-改-写架构)
#if _USE_WRITE == 1
DRESULT USER_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
    if (Stat & STA_NOINIT) return RES_NOTRDY;

    uint32_t WriteAddr = sector * 512;    // 目标起始物理地址
    uint32_t NumByteToWrite = count * 512; // 要写入的总字节数
    
    // 动态申请 4KB 缓存，防止在 RTOS 任务栈中定义导致栈溢出
    uint8_t *Sector_Buf = (uint8_t*)pvPortMalloc(W25Q64_Sector_Size); 
    if(Sector_Buf == NULL) return RES_ERROR; // 内存不足

    uint32_t secpos;      // 物理扇区号 (4KB)
    uint16_t secoff;      // 物理扇区内的偏移 (0~4095)
    uint16_t secremain;   // 物理扇区内的剩余空间
    uint32_t offdata = 0; // buff 数据的偏移

    secpos = WriteAddr / W25Q64_Sector_Size;
    secoff = WriteAddr % W25Q64_Sector_Size;
    secremain = W25Q64_Sector_Size - secoff;

    if (NumByteToWrite <= secremain) secremain = NumByteToWrite;

    while (1) 
    {
        // 步骤 A：把整个 4KB 物理扇区的数据先读到缓存里保护起来
        SPI_FLASH_BufferRead(Sector_Buf, secpos * W25Q64_Sector_Size, W25Q64_Sector_Size);
        
        // 步骤 B：擦除这 4KB 物理空间
        SPI_FLASH_SectorErase(secpos * W25Q64_Sector_Size);
        
        // 步骤 C：在内存中，把 FATFS 想改写的那 512 字节（或更多）覆盖进去
        for (uint16_t i = 0; i < secremain; i++) {
            Sector_Buf[secoff + i] = buff[offdata + i];
        }
        
        // 步骤 D：把拼装好的全新 4KB 数据写回 Flash
        SPI_FLASH_BufferWrite(Sector_Buf, secpos * W25Q64_Sector_Size, W25Q64_Sector_Size);

        if (NumByteToWrite == secremain) break; // 写完了，跳出循环
        else 
        {
            // 还没写完，说明跨越了 4KB 物理扇区边界，继续处理下一个物理扇区
            secpos++;
            secoff = 0;
            offdata += secremain;
            NumByteToWrite -= secremain;
            if (NumByteToWrite > W25Q64_Sector_Size) secremain = W25Q64_Sector_Size;
            else secremain = NumByteToWrite;
        }
    }
    
    vPortFree(Sector_Buf); // 释放内存，好借好还
    return RES_OK;
}
#endif /* _USE_WRITE == 1 */

// 5. IO 控制 (告诉 FATFS 这个芯片的规模)
#if _USE_IOCTL == 1
DRESULT USER_ioctl (BYTE pdrv, BYTE cmd, void *buff)
{
    if (Stat & STA_NOINIT) return RES_NOTRDY;

    DRESULT res = RES_ERROR;

    switch (cmd) 
    {
        case GET_SECTOR_COUNT: // 获取总的 512B 虚拟扇区数
            // W25Q64 = 8MB = 8388608 Bytes. 8388608 / 512 = 16384
            *(DWORD*)buff = 16384;
            res = RES_OK;
            break;
            
        case GET_SECTOR_SIZE:  // 虚拟扇区固定为 512 字节
            *(WORD*)buff = 512;
            res = RES_OK;
            break;
            
        case GET_BLOCK_SIZE:   // 擦除块包含的虚拟扇区数 (4KB / 512B = 8)
            *(DWORD*)buff = 8;
            res = RES_OK;
            break;
            
        case CTRL_SYNC:        // 同步命令 (等待 Flash 空闲)
            // W25Q64 的驱动已经自带了判忙逻辑，直接返回 OK
            res = RES_OK;
            break;
    }

    return res;
}
#endif /* _USE_IOCTL == 1 */

