#include "oled.h"
#include "i2c.h" // 确保能调用 hi2c1
#include "oledfont.h"

// 最底层的硬件 I2C 发送函数
void OLED_WR_Byte(uint8_t dat, uint8_t cmd)
{
    if(cmd) {
        HAL_I2C_Mem_Write(&hi2c1, OLED_I2C_ADDRESS, 0x40, I2C_MEMADD_SIZE_8BIT, &dat, 1, 100);
    } else {
        HAL_I2C_Mem_Write(&hi2c1, OLED_I2C_ADDRESS, 0x00, I2C_MEMADD_SIZE_8BIT, &dat, 1, 100);
    }
}

// 设置光标位置
void OLED_Set_Pos(uint8_t x, uint8_t y) 
{ 
    OLED_WR_Byte(0xb0 + y, 0); // 设置页地址（行）
    OLED_WR_Byte(((x & 0xf0) >> 4) | 0x10, 0); // 设置列高地址
    OLED_WR_Byte((x & 0x0f), 0); // 设置列低地址
} 

// 清屏函数
void OLED_Clear(void)  
{  
    uint8_t i, n;		    
    for(i = 0; i < 8; i++)  
    {  
        OLED_WR_Byte(0xb0 + i, 0);    
        OLED_WR_Byte(0x00, 0);      
        OLED_WR_Byte(0x10, 0);      
        for(n = 0; n < 128; n++) OLED_WR_Byte(0, 1); 
    } 
}

// 屏幕出厂初始化序列 (SSD1306 标准参数)
void OLED_Init(void)
{
    HAL_Delay(100); // 等待屏幕上电稳定
    OLED_WR_Byte(0xAE, 0); // 关闭显示
    OLED_WR_Byte(0x20, 0); // 设置内存寻址模式
    OLED_WR_Byte(0x10, 0); // 00,水平寻址; 01,垂直寻址; 10,页寻址 (默认)
    OLED_WR_Byte(0xb0, 0); // 设置页寻址起始页 (0~7)
    OLED_WR_Byte(0xc8, 0); // 设置 COM 输出扫描方向，正常显示
    OLED_WR_Byte(0x00, 0); // 设置列低地址
    OLED_WR_Byte(0x10, 0); // 设置列高地址
    OLED_WR_Byte(0x40, 0); // 设置起始行 (0~63)
    OLED_WR_Byte(0x81, 0); // 设置对比度控制寄存器
    OLED_WR_Byte(0xff, 0); // 对比度亮度 0x00~0xff
    OLED_WR_Byte(0xa1, 0); // 设置段重映射，正常显示
    OLED_WR_Byte(0xa6, 0); // 设置正常显示，0xa7 为反色显示
    OLED_WR_Byte(0xa8, 0); // 设置多路复用率 (1~64)
    OLED_WR_Byte(0x3F, 0); // 1/64 duty
    OLED_WR_Byte(0xa4, 0); // 输出 RAM 内容
    OLED_WR_Byte(0xd3, 0); // 设置显示偏移
    OLED_WR_Byte(0x00, 0); // 不偏移
    OLED_WR_Byte(0xd5, 0); // 设置显示时钟分频比/振荡器频率
    OLED_WR_Byte(0xf0, 0); // 设置分频比
    OLED_WR_Byte(0xd9, 0); // 设置预充电周期
    OLED_WR_Byte(0x22, 0); 
    OLED_WR_Byte(0xda, 0); // 设置 COM 引脚硬件配置
    OLED_WR_Byte(0x12, 0);
    OLED_WR_Byte(0xdb, 0); // 设置 VCOMH 取消选择级别
    OLED_WR_Byte(0x20, 0); 
    OLED_WR_Byte(0x8D, 0); // 设置电荷泵
    OLED_WR_Byte(0x14, 0); // 使能电荷泵
    OLED_WR_Byte(0xAF, 0); // 开启显示
    OLED_Clear();
}

/**
  * @brief  在指定位置显示一个字符
  * @param  x: 0~127 (列)
  * @param  y: 0~7 (页/行)
  * @param  chr: 要显示的字符
  * @param  Char_Size: 字体大小 (目前用的是16)
  */
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t Char_Size)
{
    unsigned char c = 0, i = 0;
    c = chr - ' '; // 计算字符在字库数组中的偏移量 (因为字库是从空格 ' ' 开始的)

    if(x > 128 - 8) { x = 0; y = y + 2; } // 如果这一行右边显示不下了，自动换行

    if(Char_Size == 16)
    {
        // 1. 画字符的上半部分 (占 8x8 像素)
        OLED_Set_Pos(x, y); 
        for(i = 0; i < 8; i++) 
            OLED_WR_Byte(asc2_1608[c][i], 1); 

        // 2. 画字符的下半部分 (占 8x8 像素，所以在下一页 y+1)
        OLED_Set_Pos(x, y + 1); 
        for(i = 0; i < 8; i++) 
            OLED_WR_Byte(asc2_1608[c][i + 8], 1); 
    }
}

/**
  * @brief  在指定位置显示一串字符串
  * @param  x: 0~127 (列)
  * @param  y: 0~7 (页/行)
  * @param  chr: 字符串指针 ("Music Player" 这种)
  * @param  Char_Size: 字体大小 (目前用的是16)
  */
void OLED_ShowString(uint8_t x, uint8_t y, char *chr, uint8_t Char_Size)
{
    unsigned char j = 0;
    while (chr[j] != '\0') // 只要没遇到字符串结束符就一直画
    {
        OLED_ShowChar(x, y, chr[j], Char_Size); // 画当前的单字符
        x += 8;                                 // 因为是 8x16 的字体，画完一个光标向右移动 8 个像素
        
        if (x > 120)                            // 如果碰到了屏幕最右侧边缘
        { 
            x = 0;                              // 回到最左边
            y += 2;                             // 向下换行 (因为16像素高的字符占2页，所以 +2)
        }
        j++; // 指向下一个字符
    }
}

