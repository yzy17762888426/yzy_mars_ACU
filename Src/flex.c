#include "flex.h"
#include "main.h"
#include "display_comm.h"
#include <stdio.h>

#define NEX_END  "\xff\xff\xff"

/*------------------------------------------------------------------------------
 * PA1 Flex Fuel Sensor — TIM5_CH2 输入捕获
 *  PSC=64-1, 64MHz/64=1MHz → 1 tick = 1μs
 *  双边沿捕获: 上升沿测周期, 下降沿测高脉宽
 *----------------------------------------------------------------------------*/
TIM_HandleTypeDef htim5;

static volatile uint32_t ic_period_us  = 0;   // 周期 μs
static volatile uint32_t ic_high_us    = 0;   // 高脉宽 μs
static volatile uint32_t ic_last_rise  = 0;   // 上次上升沿 CCR
static volatile uint8_t  ic_state      = 0;   // 0=等上升沿, 1=等下降沿
static volatile uint32_t ic_tick       = 0;   // 上次捕获时刻

static uint8_t  pa1_duty        = 0;
static uint16_t pa1_freq        = 0;
static uint16_t pa1_ethanol     = 0;
static int32_t  pa1_temp_x10    = 0;

void MX_TIM5_Init(void)
{
    __HAL_RCC_TIM5_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    htim5.Instance = TIM5;
    htim5.Init.Prescaler = 64 - 1;
    htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim5.Init.Period = 0xFFFF;
    htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&htim5);

    TIM_IC_InitTypeDef sConfigIC = {0};
    sConfigIC.ICPolarity = TIM_ICPOLARITY_RISING;
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
    sConfigIC.ICFilter = 0x05;
    HAL_TIM_IC_ConfigChannel(&htim5, &sConfigIC, TIM_CHANNEL_2);

    HAL_TIM_IC_Start_IT(&htim5, TIM_CHANNEL_2);

    HAL_NVIC_SetPriority(TIM5_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(TIM5_IRQn);
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM5)
        return;

    uint32_t val = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
    ic_tick = HAL_GetTick();

    if (ic_state == 0)
    {
        // 上升沿: 计算周期
        if (ic_last_rise != 0)
            ic_period_us = (val >= ic_last_rise) ? (val - ic_last_rise)
                                                  : (0x10000UL - ic_last_rise + val);
        ic_last_rise = val;
        ic_state = 1;

        // 切到下降沿捕获
        TIM5->CCER |= TIM_CCER_CC2P;
    }
    else
    {
        // 下降沿: 计算高脉宽
        ic_high_us = (val >= ic_last_rise) ? (val - ic_last_rise)
                                            : (0x10000UL - ic_last_rise + val);
        ic_state = 0;

        // 切回上升沿捕获
        TIM5->CCER &= ~TIM_CCER_CC2P;
    }
}

#define FLEX_SAMPLE_COUNT  10

static uint32_t  flex_tick       = 0;
static uint8_t   flex_write_idx  = 0;    // 写入位置 (0~9)
static uint8_t   flex_valid_cnt  = 0;    // 有效采样计数 (达到10后满窗)
static uint16_t  flex_freq_buf[FLEX_SAMPLE_COUNT] = {0};
static int32_t   flex_temp_buf[FLEX_SAMPLE_COUNT] = {0};

void Flex_Sensor_Process(void)
{
    uint32_t now = HAL_GetTick();
    if (now - flex_tick < 1000)
        return;
    flex_tick = now;

    uint32_t period = ic_period_us;
    uint32_t high   = ic_high_us;

    // 100ms 无信号 → 视为无传感器, 清空缓冲区
    if (period == 0 || (now - ic_tick) > 100)
    {
        pa1_freq = 0;
        pa1_duty = 0;
        pa1_ethanol = 0;
        flex_valid_cnt = 0;
        memset(flex_freq_buf, 0, sizeof(flex_freq_buf));
        memset(flex_temp_buf, 0, sizeof(flex_temp_buf));
        return;
    }

    // 当前频率和低脉宽 (频率 x10，保留小数一位)
    uint16_t freq = (uint16_t)((1000000UL * 10) / period);
    int32_t  temp_raw = (415 * ic_high_us) - 812500;  // 41.5*low(ms) - 81.25 (0.1°C)

    // 存入环形缓冲区
    flex_freq_buf[flex_write_idx] = freq;
    flex_temp_buf[flex_write_idx] = temp_raw / 1000;
    if (++flex_write_idx >= FLEX_SAMPLE_COUNT)
        flex_write_idx = 0;
    if (flex_valid_cnt < FLEX_SAMPLE_COUNT)
        flex_valid_cnt++;

    // 满窗后才计算
    if (flex_valid_cnt < FLEX_SAMPLE_COUNT)
        return;

    uint32_t freq_sum = 0;
    int32_t  temp_sum = 0;
    for (uint8_t i = 0; i < FLEX_SAMPLE_COUNT; i++)
    {
        freq_sum += flex_freq_buf[i];
        temp_sum += flex_temp_buf[i];
    }
    uint16_t avg_freq = (uint16_t)(freq_sum / FLEX_SAMPLE_COUNT);
    int32_t  avg_temp = (int32_t)(temp_sum / FLEX_SAMPLE_COUNT);

    pa1_freq = avg_freq / 10;  // 频率显示整数部分
    pa1_duty = (uint8_t)(ic_high_us * 100 / period);

    // 乙醇含量 (补偿+0.2%, avg_freq x10)
    uint16_t raw_eth;
    if (avg_freq <= 500)       raw_eth = 0;
    else if (avg_freq >= 1500) raw_eth = 1000 + 2;
    else                       raw_eth = (avg_freq - 500) + 2;

    // 直接输出
    pa1_ethanol = raw_eth;
    pa1_temp_x10 = avg_temp;
}

uint8_t  Get_PA1_Duty(void)        { return pa1_duty; }
uint16_t Get_PA1_Freq(void)        { return pa1_freq; }
uint16_t Get_Flex_Ethanol(void)    { return pa1_ethanol; }
int32_t  Get_Flex_Temp_x10(void)   { return pa1_temp_x10; }

/*------------------------------------------------------------------------------
 * DAC — PA4(CH1) / PA5(CH2), 12 位右对齐, 0~4095 → 0~3.3V
 *----------------------------------------------------------------------------*/
DAC_HandleTypeDef hdac;

void MX_DAC_Init(void)
{
    DAC_ChannelConfTypeDef sConfig = {0};

    __HAL_RCC_DAC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    hdac.Instance = DAC;
    if (HAL_DAC_Init(&hdac) != HAL_OK)
        Error_Handler();

    sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;

    if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1) != HAL_OK)
        Error_Handler();

    if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_2) != HAL_OK)
        Error_Handler();

    HAL_DAC_Start(&hdac, DAC_CHANNEL_1);
    HAL_DAC_Start(&hdac, DAC_CHANNEL_2);
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0);
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, 0);
}

void Set_DAC1(uint16_t value)
{
    if (value > 4095) value = 4095;
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, value);
}

void Set_DAC2(uint16_t value)
{
    if (value > 4095) value = 4095;
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, value);
}

/*------------------------------------------------------------------------------
 * 乙醇含量 → DAC 输出(正常模式下主循环调用)
 *  FLEX0 / FLEX100 = 线性插值端点
 *  DAC1_ADJ / DAC2_ADJ = 校准系数(× / 10000)
 *----------------------------------------------------------------------------*/
extern DataPacket_Struct Show_DataPacketType;

void Flex_DAC_Out(void)
{
    uint16_t eth = Get_Flex_Ethanol();  // 0~1000 (0.1% 单位)

    // 乙醇插值 + DAC校准，先乘加后统一除，四舍五入
    // dac1 = (FLEX0*1000 + eth*(FLEX100-FLEX0)) * ETH_ADJ / 10000000
    int16_t dac1 = (Show_DataPacketType.FLEX0 + eth * (Show_DataPacketType.FLEX100 - Show_DataPacketType.FLEX0) /1000) * Show_DataPacketType.ETH_ADJ / 10000;
    if (dac1 < 0)     dac1 = 0;
    if (dac1 > 5000)  dac1 = 5000;

    // int32_t dac2 = (int32_t)((int64_t)base * Show_DataPacketType.DAC2_ADJ / 10000);
    // if (dac2 < 0)     dac2 = 0;
    // if (dac2 > 4095)  dac2 = 4095;

    Set_DAC1(dac1 * 4095 * 66 / 330000);
//    Set_DAC2((uint16_t)dac2);

    // 乙醇含量显示 (0.1% → X.X%)
    uint16_t eth_show = Get_Flex_Ethanol();
    // 频率不在 50~150 Hz 范围 → 传感器未插入
    if (pa1_freq < 50 || pa1_freq > 150)
        printf("eValueShow.txt=\"-\"" NEX_END);
    else
        printf("eValueShow.txt=\"%d.%d\"" NEX_END, eth_show / 10, eth_show % 10);

    // 乙醇温度显示 (0.1°C 单位, TEMP_UINT=0→℃ 1→℉)
    if (pa1_freq < 50 || pa1_freq > 150)
    {
        printf("tValueShow.txt=\"-\"" NEX_END);
    }
    else
    {
        int32_t temp = Get_Flex_Temp_x10();
        if (Show_DataPacketType.TEMP_UINT)
            temp = (int32_t)(temp * 18 / 10 + 320);
        if (temp >= 0)
            printf("tValueShow.txt=\"%d.%d\"" NEX_END, temp / 10, temp % 10);
        else
            printf("tValueShow.txt=\"-%d.%d\"" NEX_END, (-temp) / 10, (-temp) % 10);
    }
}
