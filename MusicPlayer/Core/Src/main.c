/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "fatfs.h"
#include "i2c.h"
#include "spi.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "freertos.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */



/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

extern UART_HandleTypeDef huart3;
extern osMessageQueueId_t AudioCmdQueueHandle;




// 存放往队列里塞的消息结构体
typedef struct {
    uint8_t cmd_type; // 1: 播放/暂停切换, 2: 下一曲
} AudioCmd_t;

// 蓝牙接收缓存 (1个字节)
uint8_t ble_rx_buf[1]; 

// 全局状态变量 (让后台任务和 UI 任务能互相看到)
uint8_t current_track = 1;     // 当前曲目
uint8_t is_playing = 1;        // 1: 正在播放, 0: 已暂停
uint8_t ui_update_flag = 1;    // 1: 需要刷新屏幕, 0: 不需要 (防止屏幕狂刷闪烁)



// ==============================================================
// 🛠�? 工业级稳定版 BY8301 控制�? (拒绝花哨指针，硬核组�?)
// ==============================================================

// 1. 指定播放曲目 (对应硬编�? 7E 05 41 00 xx xx EF)
void BY8301_PlayIndex(uint16_t index) 
{
    uint8_t send_buf[7];
    uint8_t checksum;
    
    send_buf[0] = 0x7E;         // 帧头
    send_buf[1] = 0x05;         // 长度 (固定�? 5)
    send_buf[2] = 0x41;         // 命令�? (播放指定曲目)
    send_buf[3] = (uint8_t)(index >> 8); // 曲目号高�?
    send_buf[4] = (uint8_t)(index);      // 曲目号低�?
    
    // 手动计算异或校验�?: 长度 ^ 命令�? ^ 参数�? ^ 参数�?
    checksum = send_buf[1] ^ send_buf[2] ^ send_buf[3] ^ send_buf[4];
    send_buf[5] = checksum;     // 校验�?
    send_buf[6] = 0xEF;         // 帧尾
    
    HAL_UART_Transmit(&huart3, send_buf, 7, 100); 
}

// 2. 暂停 / 继续 (固定 5 字节指令)
void BY8301_PausePlay(void) 
{
    // 7E 03 02 01 EF (长度3, 命令02, 校验 03^02=01)
    uint8_t hardcode_pause[5] = {0x7E, 0x03, 0x02, 0x01, 0xEF};
    HAL_UART_Transmit(&huart3, hardcode_pause, 5, 100); 
}

// 3. 停止播放 (固定 5 字节指令)
void BY8301_Stop(void) 
{
    // 7E 03 0E 0D EF (长度3, 命令0E, 校验 03^0E=0D)
    uint8_t hardcode_stop[5] = {0x7E, 0x03, 0x0E, 0x0D, 0xEF};
    HAL_UART_Transmit(&huart3, hardcode_stop, 5, 100); 
}

// 4. 设置音量 0~30 (对应 6 字节指令)
void BY8301_SetVolume(uint8_t vol) 
{
    if(vol > 30) vol = 30; // 保护限制
    
    uint8_t send_buf[6];
    send_buf[0] = 0x7E;  
    send_buf[1] = 0x04;         // 长度固定�? 4
    send_buf[2] = 0x31;         // 命令�? (设置音量)
    send_buf[3] = vol;          // 音量参数
    send_buf[4] = 0x04 ^ 0x31 ^ vol; // 校验�?
    send_buf[5] = 0xEF;
    
    HAL_UART_Transmit(&huart3, send_buf, 6, 100); 
}

// 发�?�纯播放/恢复指令 (命令�? 0x01, 异或校验�?: 0x03 ^ 0x01 = 0x02)
void BY8301_Play(void) 
{
    uint8_t cmd[5] = {0x7E, 0x03, 0x01, 0x02, 0xEF};
    HAL_UART_Transmit(&huart3, cmd, 5, 100); 
}

// 发�?�纯暂停指令 (命令�? 0x02, 异或校验�?: 0x03 ^ 0x02 = 0x01)
void BY8301_Pause(void) 
{
    uint8_t cmd[5] = {0x7E, 0x03, 0x02, 0x01, 0xEF};
    HAL_UART_Transmit(&huart3, cmd, 5, 100); 
}

// 3. 下一曲 (固定 5 字节指令)
void BY8301_Next(void) 
{
    // 7E(头) 03(长) 03(令) 00(校验:03^03=00) EF(尾)
    uint8_t cmd_next[5] = {0x7E, 0x03, 0x03, 0x00, 0xEF};
    HAL_UART_Transmit(&huart3, cmd_next, 5, 100); 
}

// 4. 上一曲 (固定 5 字节指令)
void BY8301_Prev(void) 
{
    // 7E(头) 03(长) 04(令) 07(校验:03^04=07) EF(尾)
    uint8_t cmd_prev[5] = {0x7E, 0x03, 0x04, 0x07, 0xEF};
    HAL_UART_Transmit(&huart3, cmd_prev, 5, 100); 
}


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_USART1_UART_Init();
  MX_I2C1_Init();
  MX_FATFS_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */

 // �?�? USART1 中断接收，随时等手机发信�?
  
  HAL_UART_Receive_IT(&huart2, ble_rx_buf, 1);

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

// 串口接收完成回调函数
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    // ⚠️ 已经为你修改为 USART2 (蓝牙所在的串口)
    if(huart->Instance == USART2) 
    {
        AudioCmd_t msg;
        // 手机发 'P' 切换播放/暂停，发 'N' 切下一曲
        if(ble_rx_buf[0] == 'P') {
            msg.cmd_type = 1; 
            osMessageQueuePut(AudioCmdQueueHandle, &msg, 0, 0);
        }
        else if(ble_rx_buf[0] == 'N') {
            msg.cmd_type = 2; 
            osMessageQueuePut(AudioCmdQueueHandle, &msg, 0, 0);
        }
        
        // 重新开启 USART2 的接收中断
        HAL_UART_Receive_IT(&huart2, ble_rx_buf, 1);
    }
}

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
	
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
   while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
