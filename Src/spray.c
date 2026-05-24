#include "spray.h"
#include "display_comm.h"
#include <stdio.h>

#define NEX_END         "\xff\xff\xff"
#define NEX_PIC(name,p) do { printf(name ".pic=%d" NEX_END, (int)(p)); HAL_Delay(10); } while(0)
#define PIC_ON          9
#define PIC_OFF         8

extern DataPacket_Struct Show_DataPacketType;

/* 喷淋主输出 IO */
#define SPRAY_PORT       GPIOB
#define SPRAY_PIN        GPIO_PIN_15

static uint8_t  spray_on    = 0;
static uint32_t state_start = 0;
static uint8_t  last_main   = 0;

uint8_t Spray_GetState(void)
{
    return HAL_GPIO_ReadPin(SPRAY_PORT, SPRAY_PIN) == GPIO_PIN_SET ? 1 : 0;
}

void Spray_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = SPRAY_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SPRAY_PORT, &GPIO_InitStruct);

    spray_on  = 0;
    last_main = 0;
    HAL_GPIO_WritePin(SPRAY_PORT, SPRAY_PIN, GPIO_PIN_RESET);
}

void Spray_Task(void)
{
    // 主开关关闭: 输出低
    if (!Show_DataPacketType.SPRAYMAIN)
    {
        spray_on  = 0;
        last_main = 0;
        HAL_GPIO_WritePin(SPRAY_PORT, SPRAY_PIN, GPIO_PIN_RESET);
    }
    else
    {
        uint32_t now = HAL_GetTick();

        // 主开关刚打开: 立即进入 ON 状态
        if (!last_main)
        {
            last_main   = 1;
            spray_on    = 1;
            state_start = now;
            HAL_GPIO_WritePin(SPRAY_PORT, SPRAY_PIN, GPIO_PIN_SET);
        }
        else
        {
            uint32_t elapsed = now - state_start;

            if (spray_on)
            {
                if (elapsed >= (uint32_t)Show_DataPacketType.RAIN_ONTIME * 1000)
                {
                    spray_on    = 0;
                    state_start = now;
                    HAL_GPIO_WritePin(SPRAY_PORT, SPRAY_PIN, GPIO_PIN_RESET);
                }
            }
            else
            {
                if (elapsed >= (uint32_t)Show_DataPacketType.RAIN_OFFTIME * 1000)
                {
                    spray_on    = 1;
                    state_start = now;
                    HAL_GPIO_WritePin(SPRAY_PORT, SPRAY_PIN, GPIO_PIN_SET);
                }
            }
        }
    }

    // 状态灯: 反映实际IO状态, rainSwith透明度 (log期间不发)
    if (!log_en)
    {
        NEX_PIC("RainStat", Spray_GetState() ? PIC_ON : PIC_OFF);
        printf("rainSwith.aph=%d\xff\xff\xff", Show_DataPacketType.SPRAYMAIN ? 127 : 50);
    }
}
