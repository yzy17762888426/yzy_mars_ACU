#ifndef __FLEX_H
#define __FLEX_H

#include "stm32f1xx_hal.h"

// PA1 Flex Fuel Sensor
void     PA1_Sample(void);            // SysTick 1ms 调用
uint8_t  Get_PA1_Duty(void);          // 占空比 0~100 %
uint16_t Get_PA1_Freq(void);          // 频率 Hz
uint16_t Get_Flex_Ethanol(void);      // 乙醇含量 0.1% 单位, 0~1000
int16_t  Get_Flex_Temp_x10(void);     // 燃油温度 0.1 °C 单位

// DAC 输出
void     MX_DAC_Init(void);           // PA4(CH1) / PA5(CH2)
void     Set_DAC1(uint16_t value);    // 0~4095
void     Set_DAC2(uint16_t value);    // 0~4095

// 乙醇 → DAC
void     Flex_DAC_Out(void);          // 正常模式主循环调用

#endif
