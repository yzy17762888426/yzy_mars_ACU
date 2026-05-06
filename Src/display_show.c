#include "display_show.h"
#include "display_comm.h"
#include "stdio.h"
#include "config.h"
#include "flash.h"
#include "string.h"
#include "get_maf.h"


DataPacket_Struct Show_DataPacketType;
extern DataPacket_Struct DataPacket_Type;

static uint8_t Mode = NORMAL_MODE;




void Flash_Init(void)
{
	Flash_ReadSetting((uint16_t*)&Show_DataPacketType,sizeof(Show_DataPacketType));
	memcpy(&DataPacket_Type,&Show_DataPacketType,sizeof(DataPacket_Struct));
}

void Display_StartPage(void)
{
	printf("page page0\xff\xff\xff");
	HAL_Delay(500);
	printf("startValueShow.txt=\"%d\"\xff\xff\xff",Show_DataPacketType.START1);
	HAL_Delay(10);
	printf("fullValueShow.txt=\"%d\"\xff\xff\xff",Show_DataPacketType.FULL1);
	HAL_Delay(10);
	if(Show_DataPacketType.EX_MODE)
		printf("valueStaus.txt=\"%s\"\xff\xff\xff","AT");
	else
		printf("valueStaus.txt=\"%s\"\xff\xff\xff","MT");
	
	if(GetMode() == NORMAL_MODE)    printf("vis testIco,0\xff\xff\xff"); 
	if(GetMode() == TEST_MODE)      printf("vis testIco,1\xff\xff\xff"); 
	
	
}

void Display_SettingPage(void)
{
	printf("page page1\xff\xff\xff");
	HAL_Delay(100);
	printf("STA1.val=%d\xff\xff\xff",Show_DataPacketType.START1);
	HAL_Delay(10);
	printf("STA2.val=%d\xff\xff\xff",Show_DataPacketType.START2);
	HAL_Delay(10);
	printf("FULL1.val=%d\xff\xff\xff",Show_DataPacketType.FULL1);
	HAL_Delay(10);
	printf("FULL2.val=%d\xff\xff\xff",Show_DataPacketType.FULL2);
	HAL_Delay(10);
	printf("EX_SET.val=%d\xff\xff\xff",Show_DataPacketType.EXSET);
	HAL_Delay(10);
	printf("setSave.val=0\xff\xff\xff");
	HAL_Delay(10);
	printf("testCmd.val=0\xff\xff\xff");
	HAL_Delay(10);
	printf("outSave.val=0\xff\xff\xff");	
	HAL_Delay(10);
	printf("mafValueInput.val=%d\xff\xff\xff",Show_DataPacketType.TEST_SET);
	HAL_Delay(10);
	
	if(Show_DataPacketType.EX_MODE)
		printf("setExAuto.val=1\xff\xff\xff");
	else
		printf("setExAuto.val=0\xff\xff\xff");	
	HAL_Delay(10);
	if(Show_DataPacketType.OUT_VALUE & 0x01)
		printf("out1.val=1\xff\xff\xff");
	else
		printf("out1.val=0\xff\xff\xff");
	HAL_Delay(10);
	if(Show_DataPacketType.OUT_VALUE & 0x02)
		printf("out2.val=1\xff\xff\xff");
	else
		printf("out2.val=0\xff\xff\xff");
	HAL_Delay(10);
	if(Show_DataPacketType.OUT_VALUE & 0x04)
		printf("out3.val=1\xff\xff\xff");
	else
		printf("out3.val=0\xff\xff\xff");
	HAL_Delay(10);
	if(Show_DataPacketType.OUT_VALUE & 0x08)
		printf("out4.val=1\xff\xff\xff");
	else
		printf("out4.val=0\xff\xff\xff");
	HAL_Delay(10);
	if(Show_DataPacketType.OUT_VALUE & 0x10)
		printf("out5.val=1\xff\xff\xff");
	else
		printf("out5.val=0\xff\xff\xff");
	
	if(Show_DataPacketType.PUMP_ENABLE & 0x01)
		printf("pumpEnable1.val=1\xff\xff\xff");
	else
		printf("pumpEnable1.val=0\xff\xff\xff");
	
	if(Show_DataPacketType.PUMP_ENABLE & 0x02)
		printf("pumpEnable2.val=1\xff\xff\xff");
	else
		printf("pumpEnable2.val=0\xff\xff\xff");
	
}

void SetMode(uint8_t mode)
{
	Mode = mode;
}
uint8_t GetMode(void)
{
	return Mode;
}

uint16_t GetTestData(void)
{
	return Show_DataPacketType.TEST_SET;
}


void Display_Test(void)
{
  SetMode(TEST_MODE);
//	Display_StartPage();
   
}

void Display_OutSave(void)
{
//	HAL_Delay(500);
	printf("page page0\xff\xff\xff");
}

void Display_RainsSave(void)
{
//	HAL_Delay(500);
	printf("page page0\xff\xff\xff");	
}


void Refresh_Setting(void)
{
	if(DataPacket_Type.updated)
	{
		DataPacket_Type.updated = 0;
		if(memcmp(&DataPacket_Type,&Show_DataPacketType,sizeof(DataPacket_Struct)))
		{
				Flash_WriteSetting((uint16_t*)&DataPacket_Type,sizeof(DataPacket_Type));
				if(DataPacket_Type.Hz_Mv != Show_DataPacketType.Hz_Mv)
				{
					HAL_Delay(500);
					__set_FAULTMASK(1); // ??????
					NVIC_SystemReset(); // ??????
				}
				Flash_ReadSetting((uint16_t*)&Show_DataPacketType,sizeof(DataPacket_Type));
		}
		
		if(GetDisplay_Cmd() == BACK_CMD)
		{
			Display_StartPage(); //save finsh go to start page
		}
		else if(GetDisplay_Cmd() == TEST_CMD)
		{
			Display_Test();
		}
		else if(GetDisplay_Cmd() == PAGESETTING_CMD)
		{
			Display_SettingPage();//go to setting page refresh setting value
		}

		else if(GetDisplay_Cmd() == RAINSET_CMD)
		{
			Display_RainsSave();
		}
	}
}
