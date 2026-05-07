#ifndef __PUMP_H
#define __PUMP_H

#include "stm32f1xx_hal.h"

void MX_TIM2_Init(void);            // TIM2 PWM 初始化

// 泵 PWM 任务(主循环 100ms 调用)
void A_MotorPump_Task(void);
void B_MotorPump_Task(void);

#endif
