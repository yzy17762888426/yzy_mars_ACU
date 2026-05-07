#ifndef __DISPLAY_COMM_H
#define __DISPLAY_COMM_H

#include "stm32f1xx_hal.h"
#include "stdbool.h"

#define SAVE_CMD 								0xF10A
#define BACK_CMD 	  		    		0xF20B
#define TEST_CMD 								0xF30C
#define PAGESETTING_CMD         0xF40D
#define EX_CMD                  0xF60F
#define FACTORY_MODE_CMD        0xF912
#define FACTORY_SAVE_CMD        0xF811


#define HZ_MODE 	  		0x00
#define MV_MODE 	  		0x01

// 定义需要解析的变量结构
typedef struct {
    uint16_t PUMP1_EN;
    uint16_t START1;    // STA1的值
    uint16_t FULL1;   // FULL1的值

    uint16_t PUMP2_EN;
    uint16_t START2;    // STA2的值
    uint16_t FULL2;   // FULL2的值

    uint16_t SPRAYMAIN; 
    uint16_t RAIN_ONTIME;  
		uint16_t RAIN_OFFTIME;

    uint16_t EX_AUTO;   // EXSET的值
    uint16_t EX_SET;   // EXSET的值
    uint16_t EX_DELAY;   // EXSET的值

    uint16_t PUMP_STDUTY;   // EXSET的值
    uint16_t Hz_Mv;
    uint16_t LightSen;
    uint16_t Bright;

    uint16_t FLEX0;
    uint16_t FLEX100;

    uint16_t FLUID_MAIN;

    uint16_t EX_REV;
    uint16_t TEMP_UINT;

		uint16_t TEST_SET;   // TEST_SET的值
    uint16_t EX_VAL;
    uint16_t updated; // 标记是否有数据更新

    uint16_t MAF_ADJ;   // TEST_SET的值
    uint16_t ETH_ADJ;
    uint16_t AFR_ADJ; // 标记是否有数据更新
    uint16_t LEVEL1_VAL;   // TEST_SET的值
    uint16_t LEVEL2_VAL;
    uint16_t LEVEL3_VAL; // 标记是否有数据更新
} DataPacket_Struct;


// 数据包结构定义
typedef struct {
    uint8_t offset;               
    uint16_t checkSum;        
    uint16_t recvFlag;     
    uint16_t errorCount;
		uint8_t lastByte; 
} TransportFrame_Struct;


void Comm_unpack(void);
uint16_t GetDisplay_Cmd(void);
void InitCommBuffer(void);


bool enqueue(uint8_t value);
bool dequeue(uint8_t *value);

extern DataPacket_Struct DataPacket_Type;
#endif
