#include "flex.h"
#include "main.h"
#include "display_comm.h"
#include <stdio.h>

#define NEX_END  "\xff\xff\xff"

/*------------------------------------------------------------------------------
 * PA1 Flex Fuel Sensor 信号采样 — SysTick 1ms 调用,10s 窗口
 *  频率:    50~150 Hz  → 乙醇含量 = freq - 50  (0~100 %)
 *  低脉宽:  ~1~5 ms    → 燃油温度 = 41.5 * Tlow_ms - 81.25  (°C)
 *----------------------------------------------------------------------------*/
#define PA1_WINDOW_MS  10000U

static uint16_t pa1_high_cnt    = 0;
static uint16_t pa1_rising_cnt  = 0;
static uint16_t pa1_window_cnt  = 0;
static uint8_t  pa1_last_state  = 0;
static uint8_t  pa1_duty        = 0;   // 0~100 %
static uint16_t pa1_freq        = 0;   // Hz
static uint16_t pa1_ethanol     = 0;   // 0.1 % 单位, 0~1000
static int16_t  pa1_temp_x10    = 0;   // 0.1 °C 单位
static uint32_t pa1_ethanol_sum = 0;   // EMA 滤波累加 (×4)
static int32_t  pa1_temp_sum    = 0;   // EMA 滤波累加 (×4)
static uint16_t raw_eth;
static int16_t  raw_temp;
static uint32_t low_cnt;

void PA1_Sample(void)
{
    uint8_t s = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET) ? 1 : 0;

    if (s) pa1_high_cnt++;
    if (s && !pa1_last_state) pa1_rising_cnt++;

    pa1_last_state = s;
    pa1_window_cnt++;

    if (pa1_window_cnt >= PA1_WINDOW_MS)
    {
        pa1_duty = (uint8_t)((uint32_t)pa1_high_cnt * 100U / PA1_WINDOW_MS);
        pa1_freq = pa1_rising_cnt / (PA1_WINDOW_MS / 1000U);

        // 乙醇含量原始值
        if (pa1_rising_cnt <= 500)       raw_eth = 0;
        else if (pa1_rising_cnt >= 1500) raw_eth = 1000;
        else                             raw_eth = pa1_rising_cnt - 500;

        // 温度原始值
        if (pa1_rising_cnt > 0)
        {
            low_cnt = PA1_WINDOW_MS - pa1_high_cnt;
            raw_temp = (int16_t)(415U * low_cnt / pa1_rising_cnt) - 813;
        }
        else
        {
            raw_temp = 0;
        }

        // EMA 滤波: new = (old * 3 + raw) / 4
        pa1_ethanol_sum = pa1_ethanol_sum * 3 + (uint32_t)raw_eth * 4;
        pa1_ethanol_sum /= 4;
        pa1_ethanol = (uint16_t)(pa1_ethanol_sum / 4);

        pa1_temp_sum = pa1_temp_sum * 3 + (int32_t)raw_temp * 4;
        pa1_temp_sum /= 4;
        pa1_temp_x10 = (int16_t)(pa1_temp_sum / 4);

        pa1_high_cnt   = 0;
        pa1_rising_cnt = 0;
        pa1_window_cnt = 0;
    }
}

uint8_t  Get_PA1_Duty(void)        { return pa1_duty; }
uint16_t Get_PA1_Freq(void)        { return pa1_freq; }
uint16_t Get_Flex_Ethanol(void)    { return pa1_ethanol; }
int16_t  Get_Flex_Temp_x10(void)   { return pa1_temp_x10; }

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

    int32_t base = (int32_t)Show_DataPacketType.FLEX0 +
                   (int32_t)eth * ((int32_t)Show_DataPacketType.FLEX100 -
                                   (int32_t)Show_DataPacketType.FLEX0) / 1000;

    int32_t dac1 = (int32_t)((int64_t)base * Show_DataPacketType.ETH_ADJ / 10000);
    if (dac1 < 0)     dac1 = 0;
    if (dac1 > 4095)  dac1 = 4095;

    // int32_t dac2 = (int32_t)((int64_t)base * Show_DataPacketType.DAC2_ADJ / 10000);
    // if (dac2 < 0)     dac2 = 0;
    // if (dac2 > 4095)  dac2 = 4095;

//    Set_DAC1((uint16_t)dac1);
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
        int16_t temp = Get_Flex_Temp_x10();
        if (!Show_DataPacketType.TEMP_UINT)
            temp = (int16_t)((int32_t)temp * 18 / 10 + 320);
        if (temp >= 0)
            printf("tValueShow.txt=\"%d.%d\"" NEX_END, temp / 10, temp % 10);
        else
            printf("tValueShow.txt=\"-%d.%d\"" NEX_END, (-temp) / 10, (-temp) % 10);
    }
}
