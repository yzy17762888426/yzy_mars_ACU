/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stdio.h"
#include "display_show.h"
#include "pump.h"
#include "get_maf.h"
#include "display_comm.h"
#include "flex.h"
#include "exhaust.h"
#include "spray.h"
#include "log.h"
#include "flash.h"

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
DMA_HandleTypeDef hdma_uart1;
DMA_HandleTypeDef hdma_usart3_rx;
TIM_HandleTypeDef htim2;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

uint16_t ADvalue[ADC_CH_COUNT] = {0};
uint8_t  Rxbuffer[UART_RX_BUF_SIZE];
uint8_t  Esp_Rxbuffer[UART_RX_BUF_SIZE];

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
void MX_USART3_UART_Init(void);

/*------------------------------------------------------------------------------
 * printf 重定向到 USART2 + USART3 (Nextion + ESP32)
 *----------------------------------------------------------------------------*/
int fputc(int ch, FILE *f)
{
    while (!(USART2->SR & USART_SR_TXE)) {}
    USART2->DR = (uint8_t)ch;
    while (!(USART3->SR & USART_SR_TXE)) {}
    USART3->DR = (uint8_t)ch;
    return ch;
}

/*------------------------------------------------------------------------------
 * main
 *----------------------------------------------------------------------------*/
int main(void)
{
		USER_FlashReadProtection(0);
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_TIM2_Init();
    MX_TIM5_Init();
    MX_USART2_UART_Init();
    MX_USART3_UART_Init();
    MX_DAC_Init();

    Flash_Init();

    Exhaust_Init();
    Spray_Init();

    InitCommBuffer();
    Display_StartPage();

    while (1)
    {
        uint8_t  mode = GetMode();
        uint32_t tick = HAL_GetTick();

        if (mode == NORMAL_MODE || mode == TEST_MODE)
        {
            if (tick % 100 == 15)
            {
                GetFreqHz_Task();
                Log_Task();
                Flex_Sensor_Process();
                Flex_DAC_Out();
            }
            else if (tick % 100 == 40)
            {
                A_MotorPump_Task();
                B_MotorPump_Task();
                ExhaustValve_Task();
            }
            else if (tick % 100 == 75)
            {
                Spray_Task();
            }
            else if (tick % 1000 == 95)
            {
                Display_Warning(); 
            }
        }
        else if (mode == FACTORY_MODE)
        {
            if (tick % 500 == 50)
                Display_FactoryPage();
        }

        Display_BackgroundSetting();
        Refresh_Setting();
        Comm_unpack();
        Esp_unpack();
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
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PB6 / PB7: 泵运行状态指示输出 */
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_RESET);

    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

/*------------------------------------------------------------------------------
 * USART3 — ESP32 通信 (PC10 TX / PC11 RX, 115200, DMA 循环接收)
 *----------------------------------------------------------------------------*/
void MX_USART3_UART_Init(void)
{
    huart3.Instance = USART3;
    huart3.Init.BaudRate = 115200;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart3) != HAL_OK)
        Error_Handler();

    __HAL_UART_CLEAR_OREFLAG(&huart3);

    if (HAL_UART_Receive_DMA(&huart3, Esp_Rxbuffer, UART_RX_BUF_SIZE) != HAL_OK)
        Error_Handler();
}

void Error_Handler(void)
{
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif