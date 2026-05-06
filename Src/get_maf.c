#include "get_maf.h"
#include "stdio.h"
#include "stm32f1xx_it.h"
#include "display_comm.h"
#include "display_show.h"

static uint16_t freq_show = 0;


static uint8_t a_progress = 0;



static uint8_t b_progress = 0;


extern DataPacket_Struct Show_DataPacketType;

void Cal_A_PumpProgress(void)
{
	  if(Show_DataPacketType.PUMP_ENABLE & 0x01)
		{
				if(freq_show > Show_DataPacketType.START1)
				{
					if(freq_show < Show_DataPacketType.FULL1)
					{
						a_progress = (freq_show -Show_DataPacketType.START1) * 100 / (Show_DataPacketType.FULL1 - Show_DataPacketType.START1);
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
		if(Show_DataPacketType.PUMP_ENABLE & 0x02)
		{
				if(freq_show > Show_DataPacketType.START2)
				{
						b_progress = 100;
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
	  if(GetMode() == NORMAL_MODE)
		{
			freq_show = HAL_GetTick() % 10000;//Get_FreqHz();//
			starttime = HAL_GetTick();
		}
		else
		{
			if(HAL_GetTick() - starttime < 5000)
			{
				freq_show = GetTestData();
			}
			else
			{
				SetMode(NORMAL_MODE);
				printf("vis testIco,0\xff\xff\xff"); 
			}
		}
		Cal_A_PumpProgress();
		Cal_B_PumpProgress();
		printf("mafValueShow.txt=\"%d\"\xff\xff\xff",freq_show);
}




uint8_t Get_A_PumpProgress(void)
{
	  return a_progress;
}
uint8_t Get_B_PumpProgress(void)
{
	  return b_progress ;
}

