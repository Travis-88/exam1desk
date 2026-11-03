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

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim3;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM3_Init(void);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM3_Init();

  /* USER CODE BEGIN 2 */

  // 啟動 TIM3 Channel 3 的 PWM 輸出 (對應 PC8 球速指示燈)
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);

  // 變數初始化宣告
  uint8_t direction_state = 0; // 0: 方向1 (PA11), 1: 方向2 (PB12), 2: 方向3 (PB11)
  uint8_t speed_level = 1;     // 1: 慢速, 2: 中速, 3: 快速

  // 初始化狀態：預設點亮方向1
  HAL_GPIO_WritePin(GPIOA, blue1_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOB, blue2_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, blue3_Pin, GPIO_PIN_RESET);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    /* 1. 偵測是否有球 (PC9 / ball_Pin)
     * 按下 (GPIO_PIN_RESET) = 沒有球 -> 紅燈亮 (PA12 / red_Pin)、綠燈滅 (PC6 / green2_Pin)
     * 放開 (GPIO_PIN_SET)   = 有球   -> 紅燈滅 (PA12 / red_Pin)、綠燈亮 (PC6 / green2_Pin)
     */
    if (HAL_GPIO_ReadPin(ball_GPIO_Port, ball_Pin) == GPIO_PIN_RESET) {
        HAL_GPIO_WritePin(GPIOA, red_Pin, GPIO_PIN_SET);                  // 紅燈亮 (無球)
        HAL_GPIO_WritePin(green2_GPIO_Port, green2_Pin, GPIO_PIN_RESET);  // 綠燈滅
    } else {
        HAL_GPIO_WritePin(GPIOA, red_Pin, GPIO_PIN_RESET);                // 紅燈滅
        HAL_GPIO_WritePin(green2_GPIO_Port, green2_Pin, GPIO_PIN_SET);    // 綠燈亮 (有球)
    }

    /* 2. 發球控制 (改為 PB13 發球鍵, PB15 發球燈 / green1_Pin)
     * 必須在「有球」的前提下 (即 PC9 放開 / ball_Pin 為 SET)，按下 PB13 發球燈才會亮 0.5 秒
     */
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13) == GPIO_PIN_RESET) { // 按下 PB13 發球鍵
        if (HAL_GPIO_ReadPin(ball_GPIO_Port, ball_Pin) == GPIO_PIN_SET) { // 確保有球
            HAL_GPIO_WritePin(GPIOB, green1_Pin, GPIO_PIN_SET); // 發球燈亮
            HAL_Delay(500);
            HAL_GPIO_WritePin(GPIOB, green1_Pin, GPIO_PIN_RESET);
        }
        // 等待發球按鍵放開，避免連續觸發
        while(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13) == GPIO_PIN_RESET);
    }

    /* 3. 方向切換按鍵 (PB2 / bluecontrol_Pin)
     * 每按一次切換 3 個方向 LED (PA11 -> PB12 -> PB11 -> 循環)
     */
    if (HAL_GPIO_ReadPin(GPIOB, bluecontrol_Pin) == GPIO_PIN_RESET) {
        HAL_Delay(30); // 簡易去彈跳
        if (HAL_GPIO_ReadPin(GPIOB, bluecontrol_Pin) == GPIO_PIN_RESET) {
            direction_state = (direction_state + 1) % 3;

            HAL_GPIO_WritePin(GPIOA, blue1_Pin, (direction_state == 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOB, blue2_Pin, (direction_state == 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOB, blue3_Pin, (direction_state == 2) ? GPIO_PIN_SET : GPIO_PIN_RESET);

            while(HAL_GPIO_ReadPin(GPIOB, bluecontrol_Pin) == GPIO_PIN_RESET);
        }
    }

    /* 4. 球速切換按鍵 (PB1 / speedcontrol_Pin)
     * 每按一次切換一種球速 (改變 PC8 的 PWM 佔空比)
     */
    if (HAL_GPIO_ReadPin(GPIOB, speedcontrol_Pin) == GPIO_PIN_RESET) {
        HAL_Delay(30);
        if (HAL_GPIO_ReadPin(GPIOB, speedcontrol_Pin) == GPIO_PIN_RESET) {
            speed_level = (speed_level % 3) + 1;

            if (speed_level == 1)      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 5000);  // 速度 1
            else if (speed_level == 2) __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 12000); // 速度 2
            else if (speed_level == 3) __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 19000); // 速度 3

            while(HAL_GPIO_ReadPin(GPIOB, speedcontrol_Pin) == GPIO_PIN_RESET);
        }
    }

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

  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 1699;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 19999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 16000;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim3);
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOB, blue3_Pin|blue2_Pin|green1_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(green2_GPIO_Port, green2_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, blue1_Pin|red_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PB13 (發球按鍵) */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP; // PB13 建議開啟上拉，按鍵接 GND
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : speedcontrol_Pin (PB1) bluecontrol_Pin (PB2) */
  GPIO_InitStruct.Pin = speedcontrol_Pin|bluecontrol_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : blue3_Pin blue2_Pin green1_Pin */
  GPIO_InitStruct.Pin = blue3_Pin|blue2_Pin|green1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : green2_Pin */
  GPIO_InitStruct.Pin = green2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(green2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ball_Pin (PC9) */
  GPIO_InitStruct.Pin = ball_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(ball_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : blue1_Pin red_Pin */
  GPIO_InitStruct.Pin = blue1_Pin|red_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
