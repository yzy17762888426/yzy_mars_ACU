#include "exhaust.h"
#include "display_comm.h"
#include "get_maf.h"
#include <stdio.h>

#define NEX_END         "\xff\xff\xff"
#define NEX_PIC(name,p) do { printf(name ".pic=%d" NEX_END, (int)(p)); HAL_Delay(10); } while(0)
#define PIC_ON          9
#define PIC_OFF         8

extern DataPacket_Struct Show_DataPacketType;

/* 排气阀 IO */
#define EX_PORT         GPIOB
#define EX_PIN          GPIO_PIN_14

/* 阀门状态 */
static uint8_t  valve_open = 0;
static uint8_t  close_delay_active = 0;
static uint32_t close_delay_start  = 0;

static void Ex_WritePin(uint8_t open)
{
    // EX_REV=0: 高电平触发(开=HIGH)  EX_REV=1: 低电平触发(开=LOW)
    GPIO_PinState state = open ?
        (Show_DataPacketType.EX_REV ? GPIO_PIN_RESET : GPIO_PIN_SET) :
        (Show_DataPacketType.EX_REV ? GPIO_PIN_SET   : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(EX_PORT, EX_PIN, state);
}

void Exhaust_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = EX_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(EX_PORT, &GPIO_InitStruct);

    // 上电默认关闭
    valve_open = 0;
    close_delay_active = 0;
    Ex_WritePin(0);
}

void ExhaustValve_Task(void)
{
    uint8_t should_open = 0;

    if (Show_DataPacketType.EX_AUTO)
    {
        // AT 模式: EX_VAL 作为使能开关, freq_show > EX_SET 时开启
        if (Show_DataPacketType.EX_VAL)
        {
            if (GetFreqShow() > Show_DataPacketType.EX_SET)
                should_open = 1;
        }
    }
    else
    {
        // MT 模式: EX_VAL 直接控制开关
        if (Show_DataPacketType.EX_VAL)
            should_open = 1;
    }

    if (should_open)
    {
        valve_open = 1;
        close_delay_active = 0;
    }
    else if (valve_open)
    {
        // 关闭延迟 EX_DELAY (ms)
        if (!close_delay_active)
        {
            close_delay_active = 1;
            close_delay_start  = HAL_GetTick();
        }
        else if ((HAL_GetTick() - close_delay_start) >= Show_DataPacketType.EX_DELAY)
        {
            valve_open = 0;
            close_delay_active = 0;
        }
    }

    Ex_WritePin(valve_open);

    // 状态灯: EX_VAL=1 → pic=9, 否则 pic=8
    NEX_PIC("ExStat", Show_DataPacketType.EX_VAL ? PIC_ON : PIC_OFF);
}
