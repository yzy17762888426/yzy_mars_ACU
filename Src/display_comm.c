#include "display_comm.h"
#include "main.h"
#include <string.h>

extern UART_HandleTypeDef huart2;

// 传输层协议标志
#define FRAMEHEAD       0xAA
#define FRAMETAIL       0x55

#define COMM_BUF_SIZE   100
#define DMA_RX_SIZE     100  // 必须与 main.h 里 UART_RX_BUF_SIZE 一致

DataPacket_Struct DataPacket_Type;
uint8_t test_en = 0;     // TEST_CMD payload[4]: 1=进入 test, 0=退出 test
uint8_t log_en  = 0;     // LOG_START_CMD=1, LOG_STOP_CMD=0

static TransportFrame_Struct TransportFrame_Type;
static uint16_t display_cmd = 0;
static uint8_t  comm_buffer[COMM_BUF_SIZE];
static uint8_t  recv_offset = 0;

static TransportFrame_Struct EspFrame_Type;
static uint16_t esp_cmd = 0;
static uint8_t  esp_buffer[COMM_BUF_SIZE];
static uint8_t  esp_recv_offset = 0;

extern DMA_HandleTypeDef hdma_uart1;
extern DMA_HandleTypeDef hdma_usart3_rx;
extern uint8_t Rxbuffer[];
extern uint8_t Esp_Rxbuffer[];

static inline uint16_t buf_u16_le(const uint8_t *buf, uint8_t idx)
{
    return ((uint16_t)buf[idx + 1] << 8) | buf[idx];
}

void InitCommBuffer(void)
{
    recv_offset = 0;
    HAL_Delay(4000);
}

uint16_t GetDisplay_Cmd(void)
{
    uint16_t cmd = display_cmd;
    if (cmd == 0)
        cmd = esp_cmd;
    display_cmd = 0;
    esp_cmd = 0;
    return cmd;
}

// Modbus CRC16
static uint16_t Modbus_crc16(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/**
 * @brief 传输层逐字节解包
 * @retval true=解出一帧合法数据,false=未完成或错误
 */
static bool TransportUnpacking(TransportFrame_Struct *pframe, uint8_t *buffer, uint16_t size, uint8_t data)
{
    // 帧头 0xAA 0xAA
    if (data == FRAMEHEAD && pframe->lastByte == FRAMEHEAD)
    {
        pframe->offset = 0;
        pframe->checkSum = 0;
        pframe->recvFlag = true;
        return false;
    }

    if (pframe->recvFlag)
    {
        // 帧尾 0x55 0x55
        if (data == FRAMETAIL && pframe->lastByte == FRAMETAIL)
        {
            pframe->recvFlag = false;

            if (pframe->offset < 3)
            {
                pframe->errorCount++;
                return false;
            }

            pframe->checkSum = Modbus_crc16(buffer, pframe->offset - 3);
            uint16_t recvCrc = ((uint16_t)buffer[pframe->offset - 2] << 8) | buffer[pframe->offset - 3];

            if (pframe->checkSum == recvCrc)
                return true;

            pframe->errorCount++;
            return false;
        }

        buffer[pframe->offset++] = data;

        if (pframe->offset >= size)
        {
            pframe->recvFlag = false;
            pframe->errorCount++;
        }
    }

    pframe->lastByte = data;
    return false;
}

// 从 DMA 环形缓冲复制所有新到的字节到 pbuf,返回长度
static bool CommUsart_RecvData(uint8_t *pbuf, uint8_t *plen)
{
    uint8_t data_cnt = DMA_RX_SIZE - __HAL_DMA_GET_COUNTER(&hdma_uart1);

    if (data_cnt == recv_offset)
    {
        *plen = 0;
        return false;
    }

    if (data_cnt > recv_offset)
    {
        *plen = data_cnt - recv_offset;
        memcpy(pbuf, &Rxbuffer[recv_offset], *plen);
    }
    else
    {
        // DMA 环形回绕
        uint8_t len1 = DMA_RX_SIZE - recv_offset;
        uint8_t len2 = data_cnt;
        memcpy(pbuf, &Rxbuffer[recv_offset], len1);
        memcpy(pbuf + len1, Rxbuffer, len2);
        *plen = len1 + len2;
    }

    recv_offset = data_cnt;
    return true;
}

static void Parse_Save(const uint8_t *buf)
{
    DataPacket_Type.PUMP1_EN     = buf_u16_le(buf, 2);
    DataPacket_Type.START1       = buf_u16_le(buf, 4);
    DataPacket_Type.FULL1        = buf_u16_le(buf, 6);
    DataPacket_Type.PUMP2_EN     = buf_u16_le(buf, 8);
    DataPacket_Type.START2       = buf_u16_le(buf, 10);
    DataPacket_Type.FULL2        = buf_u16_le(buf, 12);
    DataPacket_Type.SPRAYMAIN    = buf_u16_le(buf, 14);
    DataPacket_Type.RAIN_ONTIME  = buf_u16_le(buf, 16);
    DataPacket_Type.RAIN_OFFTIME = buf_u16_le(buf, 18);
    DataPacket_Type.EX_AUTO      = buf_u16_le(buf, 20);
    DataPacket_Type.EX_SET       = buf_u16_le(buf, 22);
    DataPacket_Type.EX_DELAY     = buf_u16_le(buf, 24);
    DataPacket_Type.PUMP_STDUTY  = buf_u16_le(buf, 26);
    DataPacket_Type.Hz_Mv        = buf_u16_le(buf, 28);
    DataPacket_Type.LightSen     = buf_u16_le(buf, 30);
    DataPacket_Type.Bright       = buf_u16_le(buf, 32);
    DataPacket_Type.FLEX0        = buf_u16_le(buf, 34);
    DataPacket_Type.FLEX100      = buf_u16_le(buf, 36);
    DataPacket_Type.FLUID_MAIN   = buf_u16_le(buf, 38);
    DataPacket_Type.EX_REV       = buf_u16_le(buf, 40);
    DataPacket_Type.TEMP_UINT    = buf_u16_le(buf, 42);
}

static void Parse_FactorySave(const uint8_t *buf)
{
    DataPacket_Type.MAF_ADJ    = buf_u16_le(buf, 2);
    DataPacket_Type.ETH_ADJ    = buf_u16_le(buf, 4);
    DataPacket_Type.AFR_ADJ    = buf_u16_le(buf, 6);
    DataPacket_Type.LEVEL1_VAL = buf_u16_le(buf, 8);
    DataPacket_Type.LEVEL2_VAL = buf_u16_le(buf, 10);
    DataPacket_Type.LEVEL3_VAL = buf_u16_le(buf, 12);
    DataPacket_Type.DAC1_ADJ   = buf_u16_le(buf, 14);
    DataPacket_Type.DAC2_ADJ   = buf_u16_le(buf, 16);
}

void Comm_unpack(void)
{
    static uint8_t temp[100];
    static uint8_t lens = 0;

    if (!CommUsart_RecvData(temp, &lens))
        return;

    for (uint8_t i = 0; i < lens; i++)
    {
        if (!TransportUnpacking(&TransportFrame_Type, comm_buffer, COMM_BUF_SIZE, temp[i]))
            continue;

        display_cmd = ((uint16_t)comm_buffer[0] << 8) | comm_buffer[1];
        DataPacket_Type.updated = 1;

        switch (display_cmd)
        {
        case SAVE_CMD:
            Parse_Save(comm_buffer);
            break;
        case TEST_CMD:
            DataPacket_Type.TEST_SET = (comm_buffer[3] << 8) | comm_buffer[2];
            test_en = comm_buffer[4];
            break;
        case EX_CMD:
            DataPacket_Type.EX_VAL = comm_buffer[2] ? 1 : 0;
            break;
        case SPRAY_CMD:
            DataPacket_Type.SPRAYMAIN = comm_buffer[2] ? 1 : 0;
            break;
        case FACTORY_SAVE_CMD:
            Parse_FactorySave(comm_buffer);
            break;
        case LOG_START_CMD:
            log_en = 1;
            break;
        case LOG_STOP_CMD:
            log_en = 0;
            break;
        case BACK_CMD:
        case PAGESETTING_CMD:
        case FACTORY_MODE_CMD:
        default:
            break;
        }
    }
}

// 从 ESP32 USART3 DMA 环形缓冲复制所有新到的字节到 pbuf,返回长度
static bool EspUsart_RecvData(uint8_t *pbuf, uint8_t *plen)
{
    uint8_t data_cnt = DMA_RX_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart3_rx);

    if (data_cnt == esp_recv_offset)
    {
        *plen = 0;
        return false;
    }

    if (data_cnt > esp_recv_offset)
    {
        *plen = data_cnt - esp_recv_offset;
        memcpy(pbuf, &Esp_Rxbuffer[esp_recv_offset], *plen);
    }
    else
    {
        // DMA 环形回绕
        uint8_t len1 = DMA_RX_SIZE - esp_recv_offset;
        uint8_t len2 = data_cnt;
        memcpy(pbuf, &Esp_Rxbuffer[esp_recv_offset], len1);
        memcpy(pbuf + len1, Esp_Rxbuffer, len2);
        *plen = len1 + len2;
    }

    esp_recv_offset = data_cnt;
    return true;
}

void Esp_unpack(void)
{
    static uint8_t temp[100];
    static uint8_t lens = 0;

    if (!EspUsart_RecvData(temp, &lens))
        return;

    for (uint8_t i = 0; i < lens; i++)
    {
        if (!TransportUnpacking(&EspFrame_Type, esp_buffer, COMM_BUF_SIZE, temp[i]))
            continue;

        esp_cmd = ((uint16_t)esp_buffer[0] << 8) | esp_buffer[1];
        DataPacket_Type.updated = 1;

        switch (esp_cmd)
        {
        case SAVE_CMD:
            Parse_Save(esp_buffer);
            break;
        case TEST_CMD:
            DataPacket_Type.TEST_SET = (esp_buffer[3] << 8) | esp_buffer[2];
            test_en = esp_buffer[4];
            break;
        case EX_CMD:
            DataPacket_Type.EX_VAL = esp_buffer[2] ? 1 : 0;
            break;
        case SPRAY_CMD:
            DataPacket_Type.SPRAYMAIN = esp_buffer[2] ? 1 : 0;
            break;
        case FACTORY_SAVE_CMD:
            Parse_FactorySave(esp_buffer);
            break;
        case LOG_START_CMD:
            log_en = 1;
            break;
        case LOG_STOP_CMD:
            log_en = 0;
            break;
        case BACK_CMD:
        case PAGESETTING_CMD:
        case FACTORY_MODE_CMD:
        default:
            break;
        }
    }
}

/*------------------------------------------------------------------------------
 * USART2 — 230400 baud, 8N1, DMA 循环接收
 *----------------------------------------------------------------------------*/
void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 230400;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK)
        Error_Handler();

    // 清除 UART 错误标志(ORE/FE/NE),防止首次开机残留错误状态
    __HAL_UART_CLEAR_OREFLAG(&huart2);

    if (HAL_UART_Receive_DMA(&huart2, Rxbuffer, UART_RX_BUF_SIZE) != HAL_OK)
        Error_Handler();
}