#ifndef __GET_MAF_H
#define __GET_MAF_H

#include "stm32f1xx_hal.h"

void MX_ADC1_Init(void);            // ADC1 初始化

// MAF 信号采集任务(主循环 100ms 调用一次)
void GetFreqHz_Task(void);

// 当前 MAF 显示值
uint16_t GetFreqShow(void);

// MAF ADC → 校准后毫伏值
uint16_t Get_Mv_Maf_FromAdc(uint16_t adc);

// 当前 MAF 通道的校准后毫伏值
uint16_t Get_Mv_Maf(void);

#endif
