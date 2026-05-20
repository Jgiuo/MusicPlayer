/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "../../Hardware/oled.h"
#include "fatfs.h"
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
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN StartDefaultTask */
  FATFS fs;
  FRESULT res;
  FIL file;
  UINT bw, br;
  char write_buf[] = "Hello, Music!";
  char read_buf[20] = {0};
  char log_str[30]; // 用于存放打印日志的缓冲区
  
  osDelay(100); 
  OLED_Init();
  
  // 🔍 诊断 1：先看单片机能不能认�? W25Q64 的真�?
  uint32_t flash_id = SPI_FLASH_ReadID();
  sprintf(log_str, "ID: 0x%06X", flash_id);
  OLED_ShowString(0, 0, log_str, 16); // 正常应该显示 0xEF4017
  OLED_Refresh();
  osDelay(1000); // 停顿�?秒让你看�? ID

  // �?始挂�?
  res = f_mount(&fs, "0:/", 1); 
  
  if (res == FR_NO_FILESYSTEM) 
  {
      OLED_ShowString(0, 2, "Formatting...  ", 16);
      OLED_Refresh();
      res = f_mkfs("0:/", 1, 0); 
      if(res == FR_OK) res = f_mount(&fs, "0:/", 1);
  }
  
  if(res == FR_OK) 
  {
      OLED_ShowString(0, 2, "1. Mount: OK   ", 16);
      OLED_Refresh();
      
      // 🔍 诊断 2：尝试打�?文件，并抓取具体的错误码
      res = f_open(&file, "0:/test.txt", FA_CREATE_ALWAYS | FA_WRITE);
      if(res == FR_OK) {
          OLED_ShowString(0, 4, "2. Open: OK   ", 16);
          f_write(&file, write_buf, strlen(write_buf), &bw);
          f_close(&file);
      } else {
          // 如果失败，打印出具体的错误数�? (比如 1 代表 DISK_ERR, 3 代表 NOT_READY)
          sprintf(log_str, "2. Open Err: %d", res);
          OLED_ShowString(0, 4, log_str, 16);
      }
      OLED_Refresh();
      osDelay(1000);

      // 尝试读取
      res = f_open(&file, "0:/test.txt", FA_READ);
      if(res == FR_OK) {
          f_read(&file, read_buf, sizeof(read_buf)-1, &br);
          f_close(&file);
          OLED_ShowString(0, 6, read_buf, 16); 
      } else {
          sprintf(log_str, "3. Read Err: %d", res);
          OLED_ShowString(0, 6, log_str, 16);
      }
      OLED_Refresh();
  } 
  else 
  {
      sprintf(log_str, "Mount Err: %d", res);
      OLED_ShowString(0, 2, log_str, 16);
      OLED_Refresh();
  }

  // 放在 Flash Mount: OK 或你的文件系统读写测试之后
  osDelay(500); // 稍微等模块开机初始化一下
  
  OLED_ShowString(0, 6, "Playing Music...", 16);
  OLED_Refresh();
  
  BY8301_SetVolume(25); // 设置较大音量 (0~30)
  osDelay(20);          // 两条指令间稍微留点喘息时间
  BY8301_PlayIndex(1);  // 播放第一首歌！

  for(;;) { 
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    osDelay(500);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

