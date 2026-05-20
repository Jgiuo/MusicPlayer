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
/* Definitions for AppControlTask */
osThreadId_t AppControlTaskHandle;
const osThreadAttr_t AppControlTask_attributes = {
  .name = "AppControlTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for AudioGateTask */
osThreadId_t AudioGateTaskHandle;
const osThreadAttr_t AudioGateTask_attributes = {
  .name = "AudioGateTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for FatfsLyricTask */
osThreadId_t FatfsLyricTaskHandle;
const osThreadAttr_t FatfsLyricTask_attributes = {
  .name = "FatfsLyricTask",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for BluetoothTask */
osThreadId_t BluetoothTaskHandle;
const osThreadAttr_t BluetoothTask_attributes = {
  .name = "BluetoothTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for AudioCmdQueue */
osMessageQueueId_t AudioCmdQueueHandle;
const osMessageQueueAttr_t AudioCmdQueue_attributes = {
  .name = "AudioCmdQueue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void AppControlTaskFun(void *argument);
void AudioGateTaskFun(void *argument);
void FatfsLyricTaskFun(void *argument);
void BluetoothTaskFun(void *argument);

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

  /* Create the queue(s) */
  /* creation of AudioCmdQueue */
  AudioCmdQueueHandle = osMessageQueueNew (10, sizeof(uint32_t), &AudioCmdQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of AppControlTask */
  AppControlTaskHandle = osThreadNew(AppControlTaskFun, NULL, &AppControlTask_attributes);

  /* creation of AudioGateTask */
  AudioGateTaskHandle = osThreadNew(AudioGateTaskFun, NULL, &AudioGateTask_attributes);

  /* creation of FatfsLyricTask */
  //FatfsLyricTaskHandle = osThreadNew(FatfsLyricTaskFun, NULL, &FatfsLyricTask_attributes);

  /* creation of BluetoothTask */
  //BluetoothTaskHandle = osThreadNew(BluetoothTaskFun, NULL, &BluetoothTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_AppControlTaskFun */
/**
  * @brief  Function implementing the AppControlTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_AppControlTaskFun */
void AppControlTaskFun(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN AppControlTaskFun */
  /* =======================================================
 * 任务 1：UI 与 按键任务 (Normal 优先级)
 * ======================================================= */
  AudioCmd_t msg;
  uint8_t current_track = 1;
  uint8_t is_playing = 1;
  
  OLED_Clear();
  OLED_ShowString(0, 0, "--- MP3 ---", 16);
  OLED_Refresh();
  /* Infinite loop */
  for(;;)
  {
     // 扫描 PA0 按键 (假设是暂停/播放) key1
      if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) 
      {
          osDelay(20);
          if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET)
          {
              msg.cmd_type = 2; 
              osMessageQueuePut(AudioCmdQueueHandle, &msg, 0, 0); // 塞入队列
              while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) osDelay(10);
          }
      }
      // 扫描 PC13 按键 (假设是下一曲)  key2
      if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET) 
      {
          osDelay(20);
          if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET)
          {
              msg.cmd_type = 3; 
              osMessageQueuePut(AudioCmdQueueHandle, &msg, 0, 0); // 塞入队列
              current_track++;
              while(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET) osDelay(10);
          }
      }
      
      // 更新屏幕...
      osDelay(50); // 保持 UI 流畅
  }
  /* USER CODE END AppControlTaskFun */
}

/* USER CODE BEGIN Header_AudioGateTaskFun */
/**
* @brief Function implementing the AudioGateTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_AudioGateTaskFun */
void AudioGateTaskFun(void *argument)
{
  /* USER CODE BEGIN AudioGateTaskFun */
  /* =======================================================
 * 任务 2：音频守门员任务 (Above Normal 优先级)
 * 核心：平时死等，只有收到消息才发串口，绝对解耦！
 * ======================================================= */
  AudioCmd_t r_msg;
  osDelay(3000); // 开机等 BY8301 准备好
  
  // 首次开机播第 1 首
  BY8301_PlayIndex(1);
  /* Infinite loop */
  for(;;)
  {
    // osWaitForever：如果队列空着，这个任务就永远休眠，不吃 CPU
      if(osMessageQueueGet(AudioCmdQueueHandle, &r_msg, NULL, osWaitForever) == osOK)
      {
          if(r_msg.cmd_type == 1)      BY8301_PlayIndex(r_msg.value);
          else if(r_msg.cmd_type == 2) BY8301_PausePlay();
          else if(r_msg.cmd_type == 3) BY8301_Next();
          else if(r_msg.cmd_type == 4) BY8301_SetVolume(r_msg.value);
      }
  }
  /* USER CODE END AudioGateTaskFun */
}

/* USER CODE BEGIN Header_FatfsLyricTaskFun */
/**
* @brief Function implementing the FatfsLyricTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_FatfsLyricTaskFun */
void FatfsLyricTaskFun(void *argument)
{
  /* USER CODE BEGIN FatfsLyricTaskFun */
  /* =======================================================
 * 任务 3：后台歌词解析任务 (Below Normal 优先级)
 * ======================================================= */
	
  /* Infinite loop */
  for(;;)
  {
    // 暂时空置，下一节课我们在这里手撕 FATFS 歌词滚动
      osDelay(500);
  }
  /* USER CODE END FatfsLyricTaskFun */
}

/* USER CODE BEGIN Header_BluetoothTaskFun */
/**
* @brief Function implementing the BluetoothTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_BluetoothTaskFun */
void BluetoothTaskFun(void *argument)
{
  /* USER CODE BEGIN BluetoothTaskFun */
  /* =======================================================
 * 任务 4：蓝牙维护任务 (High 优先级)
 * ======================================================= */
	
  /* Infinite loop */
  for(;;)
  {
    // 中断已经把事情做完了，这里可以放一些蓝牙心跳检测、AT指令配置等慢逻辑
      osDelay(1000);
  }
  /* USER CODE END BluetoothTaskFun */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

