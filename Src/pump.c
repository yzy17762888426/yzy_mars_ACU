#include "pump.h"
#include "get_maf.h"
#include "stdio.h"
#include "display_comm.h"

static uint8_t first_pump1 = 0;
static uint8_t first_pump2 = 0;
static uint32_t FP1_starttime = 0;
static uint32_t FP2_starttime = 0;

extern	DataPacket_Struct Show_DataPacketType;

void A_MotorPump_Task(void)
{	
	 uint8_t a_progress = Get_A_PumpProgress();
	  if(a_progress > 0)
		{
			if(first_pump1 && ((HAL_GetTick() - FP1_starttime) < 100))
			{
				TIM2->CCR3 = 950;
				TIM2->CCR4 = 950;
			}
			else
			{
				first_pump1 = 0;
				TIM2->CCR3 = 300 + a_progress * 65 /10;
				TIM2->CCR4 = 300 + a_progress * 65 /10;
			}			
		}
		else	
		{
			first_pump1 = 1;
			FP1_starttime = HAL_GetTick();
			TIM2->CCR3 = 0;
			TIM2->CCR4 = 0;
		}				
		printf("pump1.val=%d\xff\xff\xff",(TIM2->CCR3 + TIM2->CCR4) / 1900);		
}
void B_MotorPump_Task(void)
{
		uint8_t b_progress = Get_B_PumpProgress();
	
	  	if(b_progress > 0)
		{
			if(first_pump2 && ((HAL_GetTick() - FP2_starttime) < 100))
			{
				TIM2->CCR1 = 950;
				TIM2->CCR2 = 950;
			}
			else
			{
				first_pump2 = 0;
				TIM2->CCR1 = 300 + b_progress * 65 /10;
				TIM2->CCR2 = 300 + b_progress * 65 /10;
			}			
		}
		else	
		{
			first_pump2 = 1;
			FP2_starttime = HAL_GetTick();
			TIM2->CCR1 = 0;
			TIM2->CCR2 = 0;
		}	
		printf("pump2.val=%d\xff\xff\xff",(TIM2->CCR1 + TIM2->CCR2) / 1900);		
}

