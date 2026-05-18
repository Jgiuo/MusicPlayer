#ifndef __OLED_H
#define __OLED_H

#include "main.h" // 引入 HAL 库

#define OLED_I2C_ADDRESS 0x78 // 默认I2C地址，如果点不亮可以改成 0x7A 试试

void OLED_WR_Byte(uint8_t dat, uint8_t cmd);
void OLED_Set_Pos(uint8_t x, uint8_t y);
void OLED_Clear(void);
void OLED_Init(void);
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t Char_Size);
void OLED_ShowString(uint8_t x, uint8_t y, char *chr, uint8_t Char_Size);

#endif
