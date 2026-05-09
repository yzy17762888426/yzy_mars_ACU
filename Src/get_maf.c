#include "get_maf.h"
#include "main.h"
#include "display_comm.h"
#include "display_show.h"
#include "stm32f1xx_it.h"
#include "stdio.h"

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;

static uint16_t freq_show = 0;
extern DataPacket_Struct Show_DataPacketType;

// MAF 毫伏输入 → 显示值(uint64 防溢出)
static uint16_t Get_Mv_Maf(void)
{
    return (uint16_t)(((uint64_t)Show_DataPacketType.MAF_ADJ * 3300U * ADvalue[CH_MAF_MV] * 156) / (40950000U * 100));
}

void GetFreqHz_Task(void)
{
    if (GetMode() == NORMAL_MODE)
        freq_show = (Show_DataPacketType.Hz_Mv == HZ_MODE) ? Get_FreqHz() : Get_Mv_Maf();
    else if (GetMode() == TEST_MODE)
        freq_show = GetTestData();

    printf("mafValueShow.txt=\"%d\"\xff\xff\xff", freq_show);
    HAL_Delay(10);
}

uint16_t GetFreqShow(void)
{
    return freq_show;
}

/*------------------------------------------------------------------------------
 * ADC1 — 10 通道扫描 + 连续转换 + DMA 循环
 *----------------------------------------------------------------------------*/
static const struct { uint32_t channel; uint8_t rank; } adc_map[] = {
    {ADC_CHANNEL_10, CH_LIGHT_SENS + 1},  // PC0 光敏电阻
    {ADC_CHANNEL_11, CH_PUMP1_CUR  + 1},  // PC1 PUMP1 电流
    {ADC_CHANNEL_12, CH_BAT_VOLT   + 1},  // PC2 电瓶电压
    {ADC_CHANNEL_13, CH_PUMP2_CUR  + 1},  // PC3 PUMP2 电流
    {ADC_CHANNEL_14, CH_RESERVE_C4 + 1},  // PC4 预留
    {ADC_CHANNEL_6,  CH_LIQUID_LVL + 1},  // PA6 液位
    {ADC_CHANNEL_7,  CH_RESERVE_A7 + 1},  // PA7 预留
    {ADC_CHANNEL_8,  CH_RESERVE_B0 + 1},  // PB0 预留
    {ADC_CHANNEL_9,  CH_MAF_MV     + 1},  // PB1 MAF 毫伏
    {ADC_CHANNEL_0,  CH_RESERVE_A0 + 1},  // PA0 预留
};

void MX_ADC1_Init(void)
{
    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
    hadc1.Init.ContinuousConvMode = ENABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = ADC_CH_COUNT;
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
        Error_Handler();

    HAL_ADCEx_Calibration_Start(&hadc1);

    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;

    for (int i = 0; i < ADC_CH_COUNT; i++)
    {
        sConfig.Channel = adc_map[i].channel;
        sConfig.Rank    = adc_map[i].rank;
        HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    }

    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)ADvalue, ADC_CH_COUNT);
}