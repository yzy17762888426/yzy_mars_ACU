#ifndef __DISPLAY_COMM_H
#define __DISPLAY_COMM_H

#include "stm32f1xx_hal.h"
#include "stdbool.h"

/*------------------------------------------------------------------------------
 * 屏幕通信协议
 *  帧格式:  AA AA  [CMD_H][CMD_L]  [PAYLOAD ...]  [CRC_L][CRC_H]  55 55
 *  CRC:    Modbus CRC16,覆盖 CMD + PAYLOAD
 *  字节序: payload 内的 uint16 都是小端(低字节在前)
 *----------------------------------------------------------------------------*/

// 屏幕命令字
#define SAVE_CMD                    0xF10A  // 设置页保存
#define BACK_CMD                    0xF20B  // 返回主页
#define TEST_CMD                    0xF30C  // 测试模式
#define PAGESETTING_CMD             0xF40D  // 进入设置页
#define EX_CMD                      0xF60F  // 排气阀手动控制
#define SPRAY_CMD                   0xF710  // 喷淋手动控制
#define FACTORY_SAVE_CMD            0xF811  // 出厂模式保存
#define FACTORY_MODE_CMD            0xF912  // 进入出厂模式
#define LOG_START_CMD               0xFA13  // 启动 LOG 记录
#define LOG_STOP_CMD                0xFB14  // 停止 LOG 记录
#define AT_MT_CMD                   0xFC15  // 排气 AT/MT 模式切换: 0=MT, 1=AT
#define V_AUTOSET_CMD               0xFD16  // 排气 AT/MT 模式切换: 0=MT, 1=AT
#define OUT_AUTOSET_CMD             0xFE17  // 排气 AT/MT 模式切换: 0=MT, 1=AT

// MAF 显示模式(Hz_Mv 字段使用)
#define HZ_MODE             0x00    // 频率模式
#define MV_MODE             0x01    // 毫伏模式

/**
 * 设置数据包
 *  - 由屏幕通过 SAVE_CMD/FACTORY_SAVE_CMD 等命令下发
 *  - Refresh_Setting 检测 updated 标志后,把整个结构体写入 Flash
 *  - 顺序与 Flash 持久化布局绑定,改动会破坏已存数据
 */
typedef struct
{
    /* 泵 A */
    uint16_t PUMP1_EN;      // 泵 A 使能
    uint16_t START1;        // 泵 A 起跳阈值
    uint16_t FULL1;         // 泵 A 满量阈值

    /* 泵 B */
    uint16_t PUMP2_EN;      // 泵 B 使能
    uint16_t START2;        // 泵 B 起跳阈值
    uint16_t FULL2;         // 泵 B 满量阈值

    /* 喷淋 */
    uint16_t SPRAYMAIN;     // 喷淋主开关
    uint16_t RAIN_ONTIME;   // 喷淋开启时长(秒)
    uint16_t RAIN_OFFTIME;  // 喷淋关闭时长(秒)

    /* 排气 */
    uint16_t EX_AUTO;       // 排气模式: 1=自动 0=手动
    uint16_t EX_SET;        // 排气触发阈值
    uint16_t EX_DELAY;      // 排气延迟(秒)

    /* 杂项 */
    uint16_t PUMP_STDUTY;   // 泵待机占空比
    uint16_t Hz_Mv;         // MAF 显示模式 HZ_MODE/MV_MODE
    uint16_t LightSen;      // 光敏阈值
    uint16_t Bright;        // 屏幕亮度

    /* MAF 标定线性参数 */
    uint16_t FLEX0;         // 0% 对应输入
    uint16_t FLEX100;       // 100% 对应输入

    uint16_t FLUID_MAIN;    // 流量主值

    uint16_t EX_REV;        // 排气反向标志
    uint16_t TEMP_UINT;     // 温度单位选择(0=℃ 1=℉)

    /* 测试模式 */
    uint16_t TEST_SET;      // 测试值(模拟 MAF 输入)
    uint16_t EX_VAL;        // 排气阀手动状态
    uint16_t updated;       // 收到一帧置 1, Refresh_Setting 处理后清零

    /* 出厂调校 */
    uint16_t MAF_ADJ;       // MAF 电压校准
    uint16_t ETH_ADJ;       // 乙醇校准
    uint16_t AFR_ADJ;       // 空燃比校准
    uint16_t LEVEL1_VAL;    // 液位 1 阈值
    uint16_t LEVEL2_VAL;    // 液位 2 阈值
    uint16_t LEVEL3_VAL;    // 液位 3 阈值
    uint16_t DAC1_ADJ;      // DAC1 校准系数
    uint16_t DAC2_ADJ;      // DAC2 校准系数
} DataPacket_Struct;

/**
 * 传输层逐字节解包用的状态机上下文
 */
typedef struct
{
    uint8_t  offset;        // 当前写入位置
    uint16_t checkSum;      // 计算/比对 CRC
    uint16_t recvFlag;      // 帧头已收到,正在收 payload
    uint16_t errorCount;    // 累计错误帧数(可选诊断用)
    uint8_t  lastByte;      // 上一字节,用于检测连续 AA / 55
} TransportFrame_Struct;

/* 公共接口 */
void     MX_USART2_UART_Init(void); // USART2 初始化
void     Comm_unpack(void);         // 主循环调用,处理 DMA 缓冲新数据
void     Esp_unpack(void);          // 主循环调用,处理 ESP32 USART3 DMA 缓冲
void     InitCommBuffer(void);      // 启动前调用一次,清空接收游标
uint16_t GetDisplay_Cmd(void);      // 返回最近一次解出的命令字

extern DataPacket_Struct DataPacket_Type;
extern uint8_t test_en;     // TEST_CMD payload[4]: 1=进入 test, 0=退出 test
extern uint8_t log_en;      // 1=LOG 记录中, 0=停止

#endif
