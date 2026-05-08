/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stdio.h"
#include "display_show.h"
#include "pump.h"
#include "get_maf.h"
#include "display_comm.h"
#include "flex.h"
#include "exhaust.h"

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
DMA_HandleTypeDef hdma_uart1;
TIM_HandleTypeDef htim2;
UART_HandleTypeDef huart2;

uint16_t ADvalue[ADC_CH_COUNT] = {0};
uint8_t  Rxbuffer[UART_RX_BUF_SIZE];

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);

/*------------------------------------------------------------------------------
 * printf 重定向到 USART2
 *----------------------------------------------------------------------------*/
int fputc(int ch, FILE *f)
{
    uint8_t temp[1] = {ch};
    HAL_UART_Transmit(&huart2, temp, 1, HAL_MAX_DELAY);
    return ch;
}

/*------------------------------------------------------------------------------
 * main
 *----------------------------------------------------------------------------*/
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_TIM2_Init();
    MX_USART2_UART_Init();
    MX_DAC_Init();

    Flash_Init();

    Exhaust_Init();

    InitCommBuffer();
    Display_StartPage();

    while (1)
    {
        uint8_t  mode = GetMode();
        uint32_t tick = HAL_GetTick();

        if (mode == NORMAL_MODE)
        {
            if (tick % 100 == 0)
            {
                GetFreqHz_Task();
      //          Flex_DAC_Out();
            }
            else if (tick % 100 == 20)
            {
                A_MotorPump_Task();
                B_MotorPump_Task();
                ExhaustValve_Task();
                Display_Warning();
            }
        }
        else if (mode == TEST_MODE)
        {
            if (tick % 100 == 0)
            {
                GetFreqHz_Task();
            }
            else if (tick % 100 == 20)
            {
                A_MotorPump_Task();
                B_MotorPump_Task();
                ExhaustValve_Task();
                Display_Warning();
            }
        }
        else if (mode == FACTORY_MODE)
        {
            Display_FactoryPage();
        }

        Display_BackgroundSetting();
        Refresh_Setting();
        Comm_unpack();
    }
}

/*------------------------------------------------------------------------------
 * System Clock — HSI 8MHz × PLL ×16 = 64 MHz
 *  APB1 = 32 MHz (÷2), APB2 = 64 MHz (÷1), ADC = PCLK2 ÷6
 *----------------------------------------------------------------------------*/
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSEState = RCC_HSE_OFF;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        Error_Handler();

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
        Error_Handler();

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
        Error_Handler();
}

/*------------------------------------------------------------------------------
 * GPIO
 *  PC5: EXTI 双边沿(频率计数)
 *  PA1: 普通输入(脉冲占空比采样)
 *----------------------------------------------------------------------------*/
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

void Error_Handler(void)
{
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif