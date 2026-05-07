#include "flex.h"
#include "main.h"
#include "display_comm.h"

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

        // 乙醇含量 0.1% = 频率0.1Hz - 500, 限幅 0~1000
        if (pa1_rising_cnt <= 500)       pa1_ethanol = 0;
        else if (pa1_rising_cnt >= 1500) pa1_ethanol = 1000;
        else                             pa1_ethanol = pa1_rising_cnt - 500;

        // 燃油温度 = 41.5 * 低脉宽(ms) - 81.25 → T_x10 = 415 * low_cnt / rising_cnt - 813
        if (pa1_rising_cnt > 0)
        {
            uint32_t low_cnt = PA1_WINDOW_MS - pa1_high_cnt;
            pa1_temp_x10 = (int16_t)(415U * low_cnt / pa1_rising_cnt) - 813;
        }
        else
        {
            pa1_temp_x10 = 0;
        }

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

    int32_t dac1 = base * (int32_t)Show_DataPacketType.DAC1_ADJ / 10000;
    if (dac1 < 0)     dac1 = 0;
    if (dac1 > 4095)  dac1 = 4095;

    int32_t dac2 = base * (int32_t)Show_DataPacketType.DAC2_ADJ / 10000;
    if (dac2 < 0)     dac2 = 0;
    if (dac2 > 4095)  dac2 = 4095;

    Set_DAC1((uint16_t)dac1);
    Set_DAC2((uint16_t)dac2);
}
