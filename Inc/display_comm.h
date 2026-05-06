#ifndef __DISPLAY_COMM_H
#define __DISPLAY_COMM_H

#include "stm32f1xx_hal.h"
#include "stdbool.h"

#define SAVE_CMD 				0xF10A
#define BACK_CMD 	  		0xF20B
#define TEST_CMD 				0xF30C
#define PAGESETTING_CMD 0xF40D
#define RAINSET_CMD 	  0xF50E


#define HZ_MODE 	  		0x00
#define MV_MODE 	  		0x01

// 定义需要解析的变量结构
typedef struct {
    uint16_t START1;    // STA1的值
    uint16_t START2;    // STA2的值
    uint16_t FULL1;   // FULL1的值
    uint16_t FULL2;   // FULL2的值
    uint16_t EXSET;   // EXSET的值
	  uint16_t TEST_SET;   // TEST_SET的值
	  uint8_t  EX_MODE;
		uint8_t  OUT_VALUE;
	  uint16_t RAIN_ONTIME;  
	  uint16_t RAIN_OFFTIME;
		uint8_t  PUMP_ENABLE;
    uint8_t  updated; // 标记是否有数据更新
		uint16_t Hz_Mv;
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
