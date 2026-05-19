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
  /* USER CODE BEGIN StartDefaultTask */
  // 1. 系统启动后，先初始化屏幕并清�?
	FATFS fs;
	FRESULT res;
	
  osDelay(100);
  OLED_Init();
	
	

	
  // 2. 打印�?机欢迎界�?
  OLED_ShowString(0, 0, "Music Player", 16);
  OLED_ShowString(0, 2, "Storage: W24Q64", 16);
  //OLED_ShowString(0, 4, "RTOS: Running", 16);
	
	
	OLED_ShowString(0, 4, "Mounting...", 16);
    OLED_Refresh();
	
	res = f_mount(&fs, "0:/", 1);
	// 2. 如果检测到没有文件系统，立刻执行格式化
  if (res == FR_NO_FILESYSTEM) 
  {
      OLED_ShowString(0, 4, "Formatting...  ", 16);
      OLED_Refresh();
      
      // 强制格式化 W25Q64 为 FAT 格式
      res = f_mkfs("0:/",1,0); 
      
      if(res == FR_OK) {
          // 格式化成功后重新挂载
          res = f_mount(&fs, "0:/", 1);
      }
  }
  
  // 3. 最终显示状态
  if(res == FR_OK) {
      OLED_ShowString(0, 6, "Flash Mount: OK", 16);
      OLED_Refresh();
      osDelay(1000); // 停顿一秒让你看清楚挂载成功
      
      // ------------------------------------------------
      // 🚀 开始 VFS 读写能力测试
      // ------------------------------------------------
      OLED_Clear(); 
      OLED_ShowString(0, 0, "- File Test -", 16);
      OLED_Refresh();
      
      FIL file;         // 文件对象句柄
      UINT bw, br;      // 记录实际写入和读取的字节数
      char write_buf[] = "Hello, Music!"; // 要写入的测试内容
      char read_buf[20] = {0};            // 用来存放读出来的内容
      
      // 1. 创建并打开文件 (写模式，如果存在则覆盖)
      res = f_open(&file, "0:/test.txt", FA_CREATE_ALWAYS | FA_WRITE);
      if(res == FR_OK) {
          OLED_ShowString(0, 2, "1. Open OK", 16);
          // 写入数据
          f_write(&file, write_buf, strlen(write_buf), &bw);
          // 必须关闭文件，数据才会真正刷新到 Flash 物理层
          f_close(&file);
          OLED_ShowString(0, 4, "2. Write OK", 16);
      } else {
          OLED_ShowString(0, 2, "1. Open Fail", 16);
      }
      OLED_Refresh();
      osDelay(500);

      // 2. 重新打开刚才的文件 (只读模式)
      res = f_open(&file, "0:/test.txt", FA_READ);
      if(res == FR_OK) {
          // 读取内容到 read_buf
          f_read(&file, read_buf, sizeof(read_buf)-1, &br);
          f_close(&file);
          
          // 3. 把读出来的内容展示到屏幕上！
          OLED_ShowString(0, 6, read_buf, 16); // 见证奇迹的时刻
      } else {
          OLED_ShowString(0, 6, "Read Fail!", 16);
      }
      OLED_Refresh();
  } 
  //
  
  //else {
      // 挂载失败的处理保留...

	/*
	if(res == FR_OK)
	{
		OLED_ShowString(0, 6, "SD Mount Ok", 16);
	}
	else
	{
		char err_str[20];
		sprintf(err_str, "SD Error: %d ", res);
		OLED_ShowString(0, 6, err_str, 16);
	}
	OLED_Refresh();
	*/
	
  // 🚨 加上这句，把排版好的内容�?口气推�?�到屏幕�?
  //OLED_Refresh();

  // 无限循环：UI 心跳与刷�?
  for(;;)
  {
    // 让板子上�? LED 闪烁，代表系统活�?
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    /*
    // 我们可以在屏幕右下角做一个动态心跳标�? (例如交替显示 * �? 空格)
    static uint8_t heartbeat = 0;
    if(heartbeat) {
        OLED_ShowString(110, 6, "*", 12);
    } else {
        OLED_ShowString(110, 6, " ", 12);
    }
    heartbeat = !heartbeat;  
    */
    // 阻塞延时 500ms，让�? CPU 给其他任�?
    osDelay(500);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

