#ifndef __EXHAUST_H
#define __EXHAUST_H

#include "stm32f1xx_hal.h"

void Exhaust_Init(void);
void ExhaustValve_Task(void);
uint8_t ExhaustValve_GetState(void);

#endif
