#include "pump.h"
#include "get_maf.h"
#include "display_comm.h"
#include "main.h"
#include "stdio.h"

extern TIM_HandleTypeDef htim2;
extern DataPacket_Struct Show_DataPacketType;

// PWM 参数 (TIM2 ARR=1000, 双通道并联)
#define PUMP_DUTY_KICK     950   // 软启动冲击占空比
#define PUMP_DUTY_MAX      950   // 满量占空比上限
#define PUMP_KICK_MS       100   // 软启动持续时间

// 泵上下文:双通道 CCR + 软启动状态
typedef struct {
    __IO uint32_t *ccr_a;
    __IO uint32_t *ccr_b;
    uint8_t       kicking;   // 正在软启动
    uint32_t      kick_tick; // 软启动起始时刻
} PumpCtx;

static PumpCtx pump_a = { &TIM2->CCR3, &TIM2->CCR4, 0, 0 };
static PumpCtx pump_b = { &TIM2->CCR1, &TIM2->CCR2, 0, 0 };

static void MotorPump_Update(PumpCtx *ctx, uint8_t enable, uint8_t progress, const char *name)
{
    if (!enable)
    {
        ctx->kicking   = 0;
        *ctx->ccr_a    = 0;
        *ctx->ccr_b    = 0;
        printf("%s.val=0\xff\xff\xff", name);
        return;
    }

    if (progress > 0)
    {
        if (ctx->kicking && (HAL_GetTick() - ctx->kick_tick) < PUMP_KICK_MS)
        {
            *ctx->ccr_a = PUMP_DUTY_KICK;
            *ctx->ccr_b = PUMP_DUTY_KICK;
        }
        else
        {
            ctx->kicking = 0;
            // base: PUMP_STDUTY(0~100%) → CCR(0~1000)
            uint32_t base = (uint32_t)Show_DataPacketType.PUMP_STDUTY * 10;
            uint32_t duty = base + (uint32_t)progress * (PUMP_DUTY_MAX - base) / 100;
            *ctx->ccr_a = duty;
            *ctx->ccr_b = duty;
        }
    }
    else
    {
        ctx->kicking = 1;
        ctx->kick_tick = HAL_GetTick();
        *ctx->ccr_a = 0;
        *ctx->ccr_b = 0;
    }

    printf("%s.val=%d\xff\xff\xff", name, (*ctx->ccr_a + *ctx->ccr_b) / (PUMP_DUTY_KICK * 2));
}

void A_MotorPump_Task(void)
{
    MotorPump_Update(&pump_a, Show_DataPacketType.PUMP1_EN, Get_A_PumpProgress(), "pump1");
}

void B_MotorPump_Task(void)
{
    MotorPump_Update(&pump_b, Show_DataPacketType.PUMP2_EN, Get_B_PumpProgress(), "pump2");
}

/*------------------------------------------------------------------------------
 * TIM2 — PWM 4 通道, PB8~PB11
 *  PSC=1, ARR=1000 → 64MHz / 2 / 1000 = 32 kHz
 *----------------------------------------------------------------------------*/
static const uint32_t tim2_channels[] = {
    TIM_CHANNEL_1, TIM_CHANNEL_2, TIM_CHANNEL_3, TIM_CHANNEL_4
};

void MX_TIM2_Init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 2 - 1;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 1000;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
        Error_Handler();

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
        Error_Handler();

    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
        Error_Handler();

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
        Error_Handler();

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    for (int i = 0; i < 4; i++)
    {
        if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, tim2_channels[i]) != HAL_OK)
            Error_Handler();
    }

    HAL_TIM_MspPostInit(&htim2);

    for (int i = 0; i < 4; i++)
        HAL_TIM_PWM_Start(&htim2, tim2_channels[i]);
}
