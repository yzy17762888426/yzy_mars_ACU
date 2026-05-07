#ifndef __GET_MAF_H
#define __GET_MAF_H

#include "stm32f1xx_hal.h"

void MX_ADC1_Init(void);            // ADC1 初始化

// MAF 信号采集任务(主循环 100ms 调用一次)
void GetFreqHz_Task(void);

// 泵进度查询(0~100)
uint8_t Get_A_PumpProgress(void);
uint8_t Get_B_PumpProgress(void);

#endif
