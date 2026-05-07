#include "display_comm.h"
#include "string.h"
#include "stdio.h"
#include "get_maf.h"

// 传输层协议标志
#define FRAMECTRL 0xA5
#define FRAMEHEAD 0xAA
#define FRAMETAIL 0x55

TransportFrame_Struct TransportFrame_Type;
DataPacket_Struct DataPacket_Type;

static uint16_t display_cmd = 0;
uint8_t comm_buffer[50]; // 接收缓冲区

extern DMA_HandleTypeDef hdma_uart1;
extern uint8_t Rxbuffer[30];

typedef struct
{
    int *buffer;
    int head;
    int tail;
    int size;
    int offset;
} CircularBuffer;
CircularBuffer CommRxBuffer;

void initBuffer(CircularBuffer *cb, int *addr, int size)
{
    cb->buffer = addr;
    cb->size = size;
    cb->head = 0;
    cb->tail = 0;
}

void InitCommBuffer(void)
{
    initBuffer(&CommRxBuffer, (int *)&comm_buffer, sizeof(comm_buffer));
}

uint16_t GetDisplay_Cmd(void)
{
    return display_cmd;
}

// Modbus CRC16计算函数
uint16_t Modbus_crc16(uint8_t *data, uint16_t length)
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
 * @brief  传输层解包函数
 * @param  pframe : 数据帧对象
 * @param  buffer : 解包结果存储缓冲区
 * @param  size   : 解包缓冲区尺寸
 * @param  data   : 接收的数据，单 byte
 * @retval 是否成功解包
 */
bool TransportUnpacking(TransportFrame_Struct *pframe, uint8_t *buffer, uint16_t size, uint8_t data)
{
    if (data == FRAMEHEAD && pframe->lastByte == FRAMEHEAD)
    {
        pframe->offset = 0;
        pframe->checkSum = 0;
        pframe->recvFlag = true;
        return false;
    }

    if (pframe->recvFlag)
    {
        // 收到结束符
        if (data == FRAMETAIL && pframe->lastByte == FRAMETAIL)
        {
            pframe->recvFlag = false;

            if (pframe->offset < 3)
            {
                pframe->errorCount++;
                return false;
            }

            pframe->checkSum = Modbus_crc16(buffer, pframe->offset - 3);

            if (pframe->checkSum == ((buffer[pframe->offset - 2] << 8) | (buffer[pframe->offset - 3])))
            {
                return true;
            }
            else
            {
                pframe->errorCount++;
                return false;
            }
        }

        buffer[pframe->offset++] = data;

        // 数据长度超过 SIZE
        if (pframe->offset >= size)
        {
            // 复位
            pframe->recvFlag = false;
            pframe->errorCount++;
        }
    }

    pframe->lastByte = data;

    return false;
}

bool CommUsart_RecvData(uint8_t *pbuf, uint8_t *plen)
{

    uint8_t data_cnt = 50 - __HAL_DMA_GET_COUNTER(&hdma_uart1);
    *pbuf = Rxbuffer[CommRxBuffer.offset];
    if (data_cnt < CommRxBuffer.offset)
    {
        *plen = 50 - CommRxBuffer.offset;
        CommRxBuffer.offset = 0;
    }
    else
    {
        *plen = data_cnt - CommRxBuffer.offset;
        CommRxBuffer.offset = data_cnt;
    }

    if (*plen > 0)
        return true;

    return false;
}

void Comm_unpack(void)
{
    static uint8_t temp[50];
    static uint8_t lens = 0;
    if (CommUsart_RecvData(temp, &lens))
    {
        for (uint8_t i = 0; i < lens; i++)
        {
            if (TransportUnpacking(&TransportFrame_Type, comm_buffer, 50, temp[i]))
            {
                display_cmd = comm_buffer[0] << 8 | comm_buffer[1];
                DataPacket_Type.updated = 1;
                if (display_cmd == SAVE_CMD)
                {
                    DataPacket_Type.PUMP1_EN = comm_buffer[3] << 8 | comm_buffer[2];
                    DataPacket_Type.START1 = comm_buffer[5] << 8 | comm_buffer[4];
                    DataPacket_Type.FULL1 = comm_buffer[7] << 8 | comm_buffer[6];
                    DataPacket_Type.START2 = comm_buffer[9] << 8 | comm_buffer[8];
                    DataPacket_Type.FULL2 = comm_buffer[11] << 8 | comm_buffer[10];
                    DataPacket_Type.SPRAYMAIN = comm_buffer[13] << 8 | comm_buffer[12];
                    DataPacket_Type.RAIN_ONTIME = comm_buffer[15] << 8 | comm_buffer[14];
                    DataPacket_Type.RAIN_OFFTIME = comm_buffer[17] << 8 | comm_buffer[16];
                    DataPacket_Type.EX_AUTO = comm_buffer[19] << 8 | comm_buffer[18];
                    DataPacket_Type.EX_SET = comm_buffer[21] << 8 | comm_buffer[20];
                    DataPacket_Type.EX_DELAY = comm_buffer[23] << 8 | comm_buffer[22];
                    DataPacket_Type.PUMP_STDUTY = comm_buffer[25] << 8 | comm_buffer[24];
                    DataPacket_Type.Hz_Mv = comm_buffer[27] << 8 | comm_buffer[26];
                    DataPacket_Type.LightSen = comm_buffer[29] << 8 | comm_buffer[28];
                    DataPacket_Type.Bright = comm_buffer[31] << 8 | comm_buffer[30];
                    DataPacket_Type.FLEX0 = comm_buffer[33] << 8 | comm_buffer[32];
                    DataPacket_Type.FLEX100 = comm_buffer[35] << 8 | comm_buffer[34];
                    DataPacket_Type.FLUID_MAIN = comm_buffer[37] << 8 | comm_buffer[36];
                    DataPacket_Type.EX_REV = comm_buffer[39] << 8 | comm_buffer[38];
                    DataPacket_Type.TEMP_UINT = comm_buffer[41] << 1 | comm_buffer[40];
                }
                else if (display_cmd == BACK_CMD)
                {
                }
                else if (display_cmd == TEST_CMD)
                {
                    DataPacket_Type.TEST_SET = comm_buffer[3] << 8 | comm_buffer[2];
                }
                else if (display_cmd == EX_CMD)
                {
                    DataPacket_Type.EX_VAL = comm_buffer[3] << 8 | comm_buffer[2];
                }
                else if (display_cmd == PAGESETTING_CMD)
                {
                    
                }
                else if (display_cmd == FACTORY_MODE_CMD)
                {
                    
                }
                else if (display_cmd == FACTORY_SAVE_CMD)
                {
                    DataPacket_Type.MAF_ADJ = comm_buffer[3] << 8 | comm_buffer[2];
                    DataPacket_Type.ETH_ADJ = comm_buffer[5] << 8 | comm_buffer[4];
                    DataPacket_Type.AFR_ADJ = comm_buffer[7] << 8 | comm_buffer[6];
                    DataPacket_Type.LEVEL1_VAL = comm_buffer[9] << 8 | comm_buffer[8];
                    DataPacket_Type.LEVEL2_VAL = comm_buffer[11] << 8 | comm_buffer[10];
                    DataPacket_Type.LEVEL3_VAL = comm_buffer[13] << 8 | comm_buffer[12];
                }
            }
        }
    }
}
