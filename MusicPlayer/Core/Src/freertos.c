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
#include <stdlib.h>
#include "ff.h"
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
 

extern uint8_t current_track ;     // 当前曲目
extern uint8_t is_playing;        // 1: 正在播放, 0: 已暂停
extern uint8_t ui_update_flag ;    // 1: 需要刷新屏幕, 0: 不需要 (防止屏幕狂刷闪烁)


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
  FatfsLyricTaskHandle = osThreadNew(FatfsLyricTaskFun, NULL, &FatfsLyricTask_attributes);

  /* creation of BluetoothTask */
  BluetoothTaskHandle = osThreadNew(BluetoothTaskFun, NULL, &BluetoothTask_attributes);

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
 * 任务 1：UI 与 按键任务 (只管交互，不管发串口)
 * ======================================================= */
    AudioCmd_t msg;
	static uint8_t led_tick = 0;
    char text_buf[20];
	//记录按键的上一次状态，用于边缘检测
	uint8_t key1_prev = GPIO_PIN_RESET;
	uint8_t key2_prev = GPIO_PIN_SET;
	
	
    OLED_Clear();
	OLED_ShowString(0, 0, "-- MP3 Player --",16);
	OLED_ShowString(0, 2, "System Booting..",16);
    OLED_Refresh();
    
    for(;;)
    {
        //  按键 1: 播放 / 暂停 (PA0)
		GPIO_PinState key1_curr = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
        if(key1_curr == GPIO_PIN_SET && key1_prev == GPIO_PIN_RESET)			
        {
           
                msg.cmd_type = 1; // 1代表Toggle(播放/暂停)
                osMessageQueuePut(AudioCmdQueueHandle, &msg, 0, 0);
                
        }
		key1_prev = key1_curr;
        
        // 🔍按键 2: 下一曲 (PC13)
		GPIO_PinState key2_curr = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);
        if(key2_curr == GPIO_PIN_RESET && key2_prev == GPIO_PIN_SET) 
        {
           
                msg.cmd_type = 2; // 2代表下一曲
                osMessageQueuePut(AudioCmdQueueHandle, &msg, 0, 0);
           
        }
		key2_prev = key2_curr;
        
        //  OLED 智能刷新机制 (只有状态改变了，才去刷屏幕，杜绝闪烁卡死)
        if (ui_update_flag == 1) 
        {
			 OLED_Clear();
            OLED_ShowString(0, 0, "-- MP3 Player --", 16);
            
            sprintf(text_buf, "Track: %02d   ", current_track);
            OLED_ShowString(0, 2, text_buf, 16);
            
            if (is_playing) OLED_ShowString(0, 4, "Status: Playing", 16);
            else            OLED_ShowString(0, 4, "Status: Paused ", 16);
            
            OLED_Refresh();
            ui_update_flag = 0; // 刷新完毕，放下旗帜
        }
			led_tick++;
		if(led_tick >= 10) //10 * 50ms
		{
			HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin); // 心跳灯
            led_tick = 0;
		}
		osDelay(50); // 每 50ms 跑一圈 
        
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
 * 任务 2：后台音频守门员任务 (只管发串口，负责同步状态)
 * ======================================================= */

    AudioCmd_t rx_msg;
    
    osDelay(2500); // 必须给 BY8301 留足开机读卡的时间
    
    BY8301_PlayIndex(current_track); // 开机播第一首
    is_playing = 1;
    ui_update_flag = 1; // 通知屏幕该显示了

    for(;;)
    {
        // 等待队列消息，如果没有操作，这个任务就不占用 CPU
        if(osMessageQueueGet(AudioCmdQueueHandle, &rx_msg, NULL, osWaitForever) == osOK)
        {
            if(rx_msg.cmd_type == 1) // 收到播放/暂停切换请求
            {
                if (is_playing) {
                    BY8301_Stop();
                    is_playing = 0;
                } else {
                    BY8301_Play();
                    is_playing = 1;
                }
                ui_update_flag = 1; // 状态改变，叫醒 UI 刷新
            }
            else if(rx_msg.cmd_type == 2) // 收到下一曲请求
            {
                BY8301_Next();
                current_track++;
                if (current_track > 99) current_track = 1; // 防止曲目溢出
                is_playing = 1;     // BY8301切歌后会自动播放
                ui_update_flag = 1; // 状态改变，叫醒 UI 刷新

                load_lyric_flag = 1;
            }
            
            osDelay(50); // 给串口和模块一点点消化时间，防止疯狂连按死锁
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

 /* =======================================================
 * 任务 3：后台歌词解析任务 (慢任务，专职读取 W25Q64)
 * ======================================================= */
	
  FIL file;
    char path[20];
    char line_buf[64];

    for(;;)
    {
        // 如果收到指令，说明切歌了，需要重新加载歌词
        if (load_lyric_flag == 1) 
        {
            load_lyric_flag = 0; // 清除标志位
            total_lrc_lines = 0; // 重置歌词计数
            
            // 组装文件路径，比如 "0:/0001.txt"
            sprintf(path, "0:/%04d.txt", current_track); 
            
            // 尝试打开文件 (FA_READ 读模式)
            if (f_open(&file, path, FA_READ) == FR_OK) 
            {
                // 一行一行地把文本读进 line_buf 里
                while (f_gets(line_buf, sizeof(line_buf), &file) != NULL) 
                {
                    if (total_lrc_lines >= 15) break; // 数组满了就别读了
                    
                    // LRC 格式: [00:03.00] xxxxxx
                    // 检查是不是标准的标签
                    if (line_buf[0] == '[' && line_buf[3] == ':') 
                    {
                        // 1. 算时间：提取分钟和秒
                        uint8_t min = (line_buf[1] - '0') * 10 + (line_buf[2] - '0');
                        uint8_t sec = (line_buf[4] - '0') * 10 + (line_buf[5] - '0');
                        lrc_array[total_lrc_lines].time_sec = min * 60 + sec;
                        
                        // 2. 抠歌词：找到 ']' 后面的内容
                        char *text_start = strchr(line_buf, ']');
                        if (text_start != NULL) {
                            // 复制到结构体里 (跳过 ']')
                            strncpy(lrc_array[total_lrc_lines].text, text_start + 1, 31);
                            total_lrc_lines++;
                        }
                    }
                }
                f_close(&file); // 读完务必关掉文件，释放内存
            }
            
            // 为了验证是否读取成功，我们借用一下 UI 刷新的权利
            // 临时在屏幕上打印第一句歌词！
            if (total_lrc_lines > 0) {
                OLED_ShowString(0, 6, lrc_array[0].text, 16); 
                OLED_Refresh();
            } else {
                OLED_ShowString(0, 6, "No Lyric Found  ", 16);
                OLED_Refresh();
            }
        }
        
        // 慢任务，每 200ms 去看一眼有没有切歌就行，绝不抢前台 CPU
        osDelay(200); 
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

