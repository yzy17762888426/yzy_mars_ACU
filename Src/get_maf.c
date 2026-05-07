#include "get_maf.h"
#include "stdio.h"
#include "stm32f1xx_it.h"
#include "display_comm.h"
#include "display_show.h"
#include "main.h"

static uint16_t freq_show = 0;

static uint8_t a_progress = 0;

static uint8_t b_progress = 0;

extern DataPacket_Struct Show_DataPacketType;

uint16_t Get_Mv_Maf(void)
{
	return Show_DataPacketType.MAF_ADJ * 3300 * ADvalue[CH_MAF_MV] / 40950000;
}

void Cal_A_PumpProgress(void)
{
	if (Show_DataPacketType.PUMP1_EN)
	{
		if (freq_show > Show_DataPacketType.START1)
		{
			if (freq_show < Show_DataPacketType.FULL1)
			{
				a_progress = (freq_show - Show_DataPacketType.START1) * 100 / (Show_DataPacketType.FULL1 - Show_DataPacketType.START1);
			}
			else
			{
				a_progress = 100;
			}
		}
		else
		{
			a_progress = 0;
		}
	}
	else
		a_progress = 0;
}

void Cal_B_PumpProgress(void)
{
	if (Show_DataPacketType.PUMP2_EN)
	{
		if (freq_show > Show_DataPacketType.START2)
		{
			if (freq_show < Show_DataPacketType.FULL2)
			{
				b_progress = (freq_show - Show_DataPacketType.START2) * 100 / (Show_DataPacketType.FULL2 - Show_DataPacketType.START2);
			}
			else
			{
				b_progress = 100;
			}
		}
		else
		{
			b_progress = 0;
		}
	}
	else
		b_progress = 0;
}

void GetFreqHz_Task(void)
{
	static uint32_t starttime = 0;
	if (GetMode() == NORMAL_MODE)
	{
		if(Show_DataPacketType.Hz_Mv == HZ_MODE)
			freq_show = Get_FreqHz(); //
		else
			freq_show = Get_Mv_Maf(); //
	}
	else if (GetMode() == TEST_MODE)
	{
		freq_show = GetTestData(); //
	}
	Cal_A_PumpProgress();
	Cal_B_PumpProgress();
	printf("mafValueShow.txt=\"%d\"\xff\xff\xff", freq_show);
}

uint8_t Get_A_PumpProgress(void)
{
	return a_progress;
}
uint8_t Get_B_PumpProgress(void)
{
	return b_progress;
}
