#include "display_show.h"
#include "display_comm.h"
#include "stdio.h"
#include "config.h"
#include "flash.h"
#include "string.h"
#include "get_maf.h"
#include "main.h"

DataPacket_Struct Show_DataPacketType;
extern DataPacket_Struct DataPacket_Type;

static uint8_t Mode = NORMAL_MODE;

void Flash_Init(void)
{
	Flash_ReadSetting((uint16_t *)&Show_DataPacketType, sizeof(Show_DataPacketType));
	memcpy(&DataPacket_Type, &Show_DataPacketType, sizeof(DataPacket_Struct));
}

void Display_StartPage(void)
{
	printf("page page0\xff\xff\xff");
	HAL_Delay(500);
	printf("startValueShow.txt=\"%d\"\xff\xff\xff", Show_DataPacketType.START1);
	HAL_Delay(10);
	printf("fullValueShow.txt=\"%d\"\xff\xff\xff", Show_DataPacketType.FULL1);
	HAL_Delay(10);
	printf("startValueSh2.txt=\"%d\"\xff\xff\xff", Show_DataPacketType.START2);
	HAL_Delay(10);
	printf("fullValueShow2.txt=\"%d\"\xff\xff\xff", Show_DataPacketType.FULL2);
	HAL_Delay(10);
	printf("sprayOp.txt=\"%d\"\xff\xff\xff", Show_DataPacketType.RAIN_ONTIME);
	HAL_Delay(10);
	printf("sprayIdle.txt=\"%d\"\xff\xff\xff", Show_DataPacketType.RAIN_OFFTIME);
	HAL_Delay(10);

	if (Show_DataPacketType.PUMP1_EN)
		printf("Pump1Stat.pic=9\xff\xff\xff");
	else
		printf("Pump1Stat.pic=8\xff\xff\xff");

	if (Show_DataPacketType.PUMP2_EN)
		printf("Pump2Stat.pic=9\xff\xff\xff");
	else
		printf("Pump2Stat.pic=8\xff\xff\xff");

	if (Show_DataPacketType.SPRAYMAIN)
		printf("RainStat.pic=9\xff\xff\xff");
	else
		printf("RainStat.pic=8\xff\xff\xff");

	if (Show_DataPacketType.EX_VAL)
		printf("ExStat.pic=9\xff\xff\xff");
	else
		printf("ExStat.pic=8\xff\xff\xff");

	if (Show_DataPacketType.EX_AUTO)
		printf("valueStaus.txt=\"%s\"\xff\xff\xff", "AT");
	else
		printf("valueStaus.txt=\"%s\"\xff\xff\xff", "MT");

	if (Show_DataPacketType.Hz_Mv)
		printf("unit.txt=\"%s\"\xff\xff\xff", "mV");
	else
		printf("unit.txt=\"%s\"\xff\xff\xff", "HZ");

	if (GetMode() == NORMAL_MODE)
		printf("vis testIco,0\xff\xff\xff");
	if (GetMode() == TEST_MODE)
		printf("vis testIco,1\xff\xff\xff");
}

void Display_SettingPage(void)
{
	printf("page page1\xff\xff\xff");
	HAL_Delay(100);

	// 泵A参数
	printf("STA1.val=%d\xff\xff\xff", Show_DataPacketType.START1);
	HAL_Delay(10);
	printf("FULL1.val=%d\xff\xff\xff", Show_DataPacketType.FULL1);
	HAL_Delay(10);
	if (Show_DataPacketType.PUMP1_EN)
		printf("pumpEnable1.val=1\xff\xff\xff");
	else
		printf("pumpEnable1.val=0\xff\xff\xff");
	HAL_Delay(10);

	// 泵B参数
	printf("STA2.val=%d\xff\xff\xff", Show_DataPacketType.START2);
	HAL_Delay(10);
	printf("FULL2.val=%d\xff\xff\xff", Show_DataPacketType.FULL2);
	HAL_Delay(10);
	if (Show_DataPacketType.PUMP2_EN)
		printf("pumpEnable2.val=1\xff\xff\xff");
	else
		printf("pumpEnable2.val=0\xff\xff\xff");
	HAL_Delay(10);

	// 喷淋参数
	if (Show_DataPacketType.SPRAYMAIN)
		printf("sprayMain.val=1\xff\xff\xff");
	else
		printf("sprayMain.val=0\xff\xff\xff");
	HAL_Delay(10);
	printf("rainOnTime.val=%d\xff\xff\xff", Show_DataPacketType.RAIN_ONTIME);
	HAL_Delay(10);
	printf("rainOffTime.val=%d\xff\xff\xff", Show_DataPacketType.RAIN_OFFTIME);
	HAL_Delay(10);

	// 排气参数
	if (Show_DataPacketType.EX_AUTO)
		printf("setExAuto.val=1\xff\xff\xff");
	else
		printf("setExAuto.val=0\xff\xff\xff");
	HAL_Delay(10);
	printf("EX_SET.val=%d\xff\xff\xff", Show_DataPacketType.EX_SET);
	HAL_Delay(10);
	printf("exDelay.val=%d\xff\xff\xff", Show_DataPacketType.EX_DELAY);
	HAL_Delay(10);

	// 泵占空比
	printf("pumpStdDuty.val=%d\xff\xff\xff", Show_DataPacketType.PUMP_STDUTY);
	HAL_Delay(10);

	// 显示参数
	if (Show_DataPacketType.Hz_Mv == MV_MODE)
		printf("mafTypeSelect.val=1\xff\xff\xff");
	else
		printf("mafTypeSelect.val=0\xff\xff\xff");

	HAL_Delay(10);
	printf("lightSen.val=%d\xff\xff\xff", Show_DataPacketType.LightSen);
	HAL_Delay(10);
	printf("bright.val=%d\xff\xff\xff", Show_DataPacketType.Bright);
	HAL_Delay(10);

	// Flex参数
	printf("flex0.val=%d\xff\xff\xff", Show_DataPacketType.FLEX0);
	HAL_Delay(10);
	printf("flex100.val=%d\xff\xff\xff", Show_DataPacketType.FLEX100);
	HAL_Delay(10);

	// 流量主值
	printf("Fluidmain.val=%d\xff\xff\xff", Show_DataPacketType.FLUID_MAIN);
	HAL_Delay(10);

	// 排气反向
	printf("Ex_Rev.val=%d\xff\xff\xff", Show_DataPacketType.EX_REV);
	HAL_Delay(10);

	// 温度
	printf("Temp_Select.val=%d\xff\xff\xff", Show_DataPacketType.TEMP_UINT);
	HAL_Delay(10);

	// 测试值
	printf("mafValueInput.val=%d\xff\xff\xff", Show_DataPacketType.TEST_SET);
	HAL_Delay(10);

	// 按钮复位
	printf("setSave.val=0\xff\xff\xff");
	HAL_Delay(10);
	printf("testCmd.val=0\xff\xff\xff");
	HAL_Delay(10);
	printf("outSave.val=0\xff\xff\xff");
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

void Display_FactoryPage(void)
{
	printf("mafvadj.val=%d\xff\xff\xff", Show_DataPacketType.MAF_ADJ);
	HAL_Delay(10);
	printf("ethvadj.val=%d\xff\xff\xff", Show_DataPacketType.ETH_ADJ);
	HAL_Delay(10);
	printf("afrvadj.val=%d\xff\xff\xff", Show_DataPacketType.AFR_ADJ);
	HAL_Delay(10);

	printf("level1.val=%d\xff\xff\xff", Show_DataPacketType.LEVEL1_VAL);
	HAL_Delay(10);
	printf("level2.val=%d\xff\xff\xff", Show_DataPacketType.LEVEL2_VAL);
	HAL_Delay(10);
	printf("level3.val=%d\xff\xff\xff", Show_DataPacketType.LEVEL3_VAL);
	HAL_Delay(10);

	printf("version.val=%d\xff\xff\xff", SW_VERSION);
	HAL_Delay(10);

	printf("lightsensor.val=%d\xff\xff\xff", ADvalue[CH_LIGHT_SENS]);
	HAL_Delay(10);
}

void Display_BackgroundSetting(void)
{
	
}
void Refresh_Setting(void)
{
	if (DataPacket_Type.updated)
	{
		DataPacket_Type.updated = 0;
		if (memcmp(&DataPacket_Type, &Show_DataPacketType, sizeof(DataPacket_Struct)))
		{
			Flash_WriteSetting((uint16_t *)&DataPacket_Type, sizeof(DataPacket_Type));
			if (DataPacket_Type.Hz_Mv != Show_DataPacketType.Hz_Mv)
			{
				HAL_Delay(500);
				__set_FAULTMASK(1); // ??????
				NVIC_SystemReset(); // ??????
			}
			Flash_ReadSetting((uint16_t *)&Show_DataPacketType, sizeof(DataPacket_Type));
		}

		if (GetDisplay_Cmd() == BACK_CMD || GetDisplay_Cmd() == SAVE_CMD)
		{
			Display_StartPage(); // save finsh go to start page
		}
		else if (GetDisplay_Cmd() == TEST_CMD)
		{
		}
		else if (GetDisplay_Cmd() == PAGESETTING_CMD)
		{
			Display_SettingPage(); // go to setting page refresh setting value
		}
		else if (GetDisplay_Cmd() == FACTORY_MODE_CMD)
		{
			SetMode(FACTORY_MODE);
		}
		else if (GetDisplay_Cmd() == FACTORY_SAVE_CMD)
		{
			SetMode(NORMAL_MODE);
			Display_StartPage(); // save finsh go to start page
		}
		
	}
}
