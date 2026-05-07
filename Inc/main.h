/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

#define SW_VERSION        66
#define UART_RX_BUF_SIZE  50
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN Private defines */
#define ADC_CH_COUNT 10

// ADC通道索引
#define CH_LIGHT_SENS 0  // PC0 IN10  光敏电阻
#define CH_PUMP1_CUR  1  // PC1 IN11  PUMP1电机电流
#define CH_BAT_VOLT   2  // PC2 IN12  电瓶电压
#define CH_PUMP2_CUR  3  // PC3 IN13  PUMP2电机电流
#define CH_RESERVE_C4 4  // PC4 IN14  预留
#define CH_LIQUID_LVL 5  // PA6 IN6   液位检测
#define CH_RESERVE_A7 6  // PA7 IN7   预留
#define CH_RESERVE_B0 7  // PB0 IN8   预留
#define CH_MAF_MV     8  // PB1 IN9   MAF毫伏输入
#define CH_RESERVE_A0 9  // PA0 IN0   预留

extern uint16_t ADvalue[ADC_CH_COUNT];

#define AD_LIGHT_SENS ADvalue[CH_LIGHT_SENS]
#define AD_PUMP1_CUR  ADvalue[CH_PUMP1_CUR]
#define AD_BAT_VOLT   ADvalue[CH_BAT_VOLT]
#define AD_PUMP2_CUR  ADvalue[CH_PUMP2_CUR]
#define AD_RESERVE_C4 ADvalue[CH_RESERVE_C4]
#define AD_LIQUID_LVL ADvalue[CH_LIQUID_LVL]
#define AD_RESERVE_A7 ADvalue[CH_RESERVE_A7]
#define AD_RESERVE_B0 ADvalue[CH_RESERVE_B0]
#define AD_MAF_MV     ADvalue[CH_MAF_MV]
#define AD_RESERVE_A0 ADvalue[CH_RESERVE_A0]
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
