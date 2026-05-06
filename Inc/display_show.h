#ifndef __DISPLAY_SHOW_H
#define __DISPLAY_SHOW_H

#include "stm32f1xx_hal.h"

#define NORMAL_MODE 0
#define TEST_MODE   1

void Display_StartPage(void);
void Refresh_Setting(void);
void Flash_Init(void);
void TestTask(void);


uint8_t GetMode(void);
void SetMode(uint8_t mode);
uint16_t GetTestData(void);

#endif
