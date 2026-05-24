#ifndef __SPRAY_H
#define __SPRAY_H

#include "stm32f1xx_hal.h"

void Spray_Init(void);
void Spray_Task(void);
uint8_t Spray_GetState(void);

#endif
