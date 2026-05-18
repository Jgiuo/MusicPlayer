#include "oled.h"
#include "i2c.h"     // 恢复使用硬件 I2C
#include "oledfont.h" 

// 1KB 显存
uint8_t OLED_GRAM[8][128]; 

// 发送单条命令 (底层封装)
void OLED_WR_Cmd(uint8_t cmd) {
    HAL_I2C_Mem_Write(&hi2c1, 0x78, 0x00, I2C_MEMADD_SIZE_8BIT, &cmd, 1, 100);
}

// 🎯【终极杀手锏】：一波流刷新，中途绝无 STOP 信号！
void OLED_Refresh(void)
{
    for(uint8_t i = 0; i < 8; i++)
    {
        OLED_WR_Cmd(0xb0 + i); // 设置页
        OLED_WR_Cmd(0x00);     // 设置列低位 (如果是SH1106可以改成0x02)
        OLED_WR_Cmd(0x10);     // 设置列高位
        
        // 🔥 一次性把 128 个字节砸给屏幕，完美避开 STOP 信号的清零 Bug！
        HAL_I2C_Mem_Write(&hi2c1, 0x78, 0x40, I2C_MEMADD_SIZE_8BIT, OLED_GRAM[i], 128, HAL_MAX_DELAY);
    }
}

void OLED_Clear(void)
{
    for(uint8_t i = 0; i < 8; i++)
        for(uint8_t n = 0; n < 128; n++)
            OLED_GRAM[i][n] = 0; 
    OLED_Refresh();              
}

void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t Char_Size)
{
    unsigned char c = chr - ' ';
    if(x > 128 - 8) { x = 0; y = y + 2; }
    if(Char_Size == 16)
    {
        for(uint8_t i = 0; i < 8; i++) {
            OLED_GRAM[y][x+i] = asc2_1608[c][i];       
            OLED_GRAM[y+1][x+i] = asc2_1608[c][i+8];   
        }
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, char *chr, uint8_t Char_Size)
{
    unsigned char j = 0;
    while (chr[j] != '\0')
    {
        OLED_ShowChar(x, y, chr[j], Char_Size);
        x += 8;
        if (x > 120) { x = 0; y += 2; }
        j++;
    }
}

void OLED_Init(void)
{
    OLED_WR_Cmd(0xAE); 
    OLED_WR_Cmd(0x20); 
    OLED_WR_Cmd(0x02); // 强制页寻址
    OLED_WR_Cmd(0xb0); 
    OLED_WR_Cmd(0xc8); 
    OLED_WR_Cmd(0x00); 
    OLED_WR_Cmd(0x10); 
    OLED_WR_Cmd(0x40); 
    OLED_WR_Cmd(0x81); 
    OLED_WR_Cmd(0xff); 
    OLED_WR_Cmd(0xa1); 
    OLED_WR_Cmd(0xa6); 
    OLED_WR_Cmd(0xa8); 
    OLED_WR_Cmd(0x3F); 
    OLED_WR_Cmd(0xa4); 
    OLED_WR_Cmd(0xd3); 
    OLED_WR_Cmd(0x00); 
    OLED_WR_Cmd(0xd5); 
    OLED_WR_Cmd(0x80); 
    OLED_WR_Cmd(0xd9); 
    OLED_WR_Cmd(0x22); 
    OLED_WR_Cmd(0xda); 
    OLED_WR_Cmd(0x12);
    OLED_WR_Cmd(0xdb); 
    OLED_WR_Cmd(0x20); 
    OLED_WR_Cmd(0x8D); 
    OLED_WR_Cmd(0x14); 
    OLED_WR_Cmd(0xAF); 
    OLED_Clear();
}

