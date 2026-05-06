#include "pump.h"
#include "get_maf.h"
#include "stdio.h"
#include "display_comm.h"

static uint8_t first_Pump = 0;
static uint32_t FP_starttime = 0;

extern	DataPacket_Struct Show_DataPacketType;

void A_MotorPump_Task(void)
{	
	 uint8_t a_progress = Get_A_PumpProgress();
	  if(a_progress > 0)
		{
			if(first_Pump && ((HAL_GetTick() - FP_starttime) < 100))
			{
				TIM2->CCR3 = 950;
				TIM2->CCR4 = 950;
			}
			else
			{
				first_Pump = 0;
				TIM2->CCR3 = 300 + a_progress * 65 /10;
				TIM2->CCR4 = 300 + a_progress * 65 /10;
			}		
			printf("Pump1Stat.pic=9\xff\xff\xff");		
		}
		else	
		{
			first_Pump = 1;
			FP_starttime = HAL_GetTick();
			TIM2->CCR3 = 0;
			TIM2->CCR4 = 0;
			printf("Pump1Stat.pic=8\xff\xff\xff");
		}
		
		if(Show_DataPacketType.EX_MODE && a_progress)  
				printf("chimneySwitch.val=1\xff\xff\xff");	
    else
		{
				if(Show_DataPacketType.EX_MODE == 0)
					printf("chimneySwitch.val=0\xff\xff\xff");
				else 
					printf("chimneySwitch.val=0\xff\xff\xff");
		}			
		
		
		printf("pump1.val=%d\xff\xff\xff",a_progress);		
}
void B_MotorPump_Task(void)
{
		uint8_t b_progress = Get_B_PumpProgress();
	
		if(b_progress)
		{
				TIM2->CCR1 = 950;
				TIM2->CCR2 = 950;
			  printf("Pump2Stat.pic=9\xff\xff\xff");
		}
		else
		{
				TIM2->CCR1 = 0;
				TIM2->CCR2 = 0;
			  printf("Pump2Stat.pic=8\xff\xff\xff");
		}
		printf("pump2.val=%d\xff\xff\xff",b_progress);			
}

