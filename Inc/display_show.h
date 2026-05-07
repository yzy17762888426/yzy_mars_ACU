#ifndef __DISPLAY_SHOW_H
#define __DISPLAY_SHOW_H

#include "stm32f1xx_hal.h"

// 运行模式
#define NORMAL_MODE     0       // 正常运行
#define TEST_MODE       1       // 测试模式(模拟 MAF 输入)
#define FACTORY_MODE    2       // 出厂调校

// 初始化(从 Flash 读取已存设置)
void Flash_Init(void);

// 页面刷新
void Display_StartPage(void);         // 主页
void Display_SettingPage(void);       // 设置页
void Display_FactoryPage(void);       // 出厂页
void Display_BackgroundSetting(void); // 后台参数刷新(主循环调用)
void Display_Warning(void);           // 告警图标分时显示(主循环调用)

// 设置管理
void     Refresh_Setting(void);
uint8_t  GetMode(void);
void     SetMode(uint8_t mode);
uint16_t GetTestData(void);

#endif
