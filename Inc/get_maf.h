#ifndef __GET_MAF_H
#define __GET_MAF_H

#include "stm32f1xx_hal.h"
	
void GetFreqHz_Task(void);
uint8_t Get_A_PumpProgress(void);
uint8_t Get_B_PumpProgress(void);
void SetAFreq(void);
void SetBFreq(void);

#endif
