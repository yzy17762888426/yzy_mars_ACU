#include "display_show.h"
#include "display_comm.h"
#include "main.h"
#include "flash.h"
#include "flex.h"
#include "stdio.h"
#include "string.h"

/*------------------------------------------------------------------------------
 * Nextion 串口屏发送宏
 *  参数为字符串字面量,通过 C 字符串拼接生成完整指令
 *  每条指令后加 HAL_Delay(10) 保证屏幕处理时间
 *----------------------------------------------------------------------------*/
#define PIC_OFF        8
#define PIC_ON         9
#define NEX_END        "\xff\xff\xff"

#define NEX_VAL(name, v)       do { printf(name ".val=%d" NEX_END, (int)(v));          HAL_Delay(10); } while (0)
#define NEX_TXT(name, t)       do { printf(name ".txt=\"%s\"" NEX_END, (t));           HAL_Delay(10); } while (0)
#define NEX_TXT_INT(name, v)   do { printf(name ".txt=\"%d\"" NEX_END, (int)(v));      HAL_Delay(10); } while (0)
#define NEX_PIC(name, p)       do { printf(name ".pic=%d" NEX_END, (int)(p));          HAL_Delay(10); } while (0)
#define NEX_VIS(obj, on)       do { printf("vis " obj ",%d" NEX_END, (on) ? 1 : 0);   HAL_Delay(10); } while (0)
#define NEX_PAGE(name, ms)     do { printf("page " name NEX_END); HAL_Delay(ms); } while (0)

/*------------------------------------------------------------------------------
 * 模块变量
 *----------------------------------------------------------------------------*/
DataPacket_Struct Show_DataPacketType;
static uint8_t Mode = NORMAL_MODE;

/*------------------------------------------------------------------------------
 * Flash / 初始化
 *----------------------------------------------------------------------------*/
static void apply_adj_defaults(DataPacket_Struct *p)
{
    if (p->MAF_ADJ == 0xFFFF) p->MAF_ADJ = 10000;
    if (p->ETH_ADJ == 0xFFFF) p->ETH_ADJ = 10000;
    if (p->AFR_ADJ == 0xFFFF) p->AFR_ADJ = 10000;
    if (p->DAC1_ADJ == 0xFFFF) p->DAC1_ADJ = 10000;
    if (p->DAC2_ADJ == 0xFFFF) p->DAC2_ADJ = 10000;
}

void Flash_Init(void)
{
    Flash_ReadSetting((uint16_t *)&Show_DataPacketType, sizeof(Show_DataPacketType));
    apply_adj_defaults(&Show_DataPacketType);
    memcpy(&DataPacket_Type, &Show_DataPacketType, sizeof(DataPacket_Struct));
}

/*------------------------------------------------------------------------------
 * 主页
 *----------------------------------------------------------------------------*/
void Display_StartPage(void)
{
    NEX_PAGE("page0", 50);

    // 起跳 / 满量
    NEX_TXT_INT("startValueShow",  Show_DataPacketType.START1);
    NEX_TXT_INT("fullValueShow",   Show_DataPacketType.FULL1);
    NEX_TXT_INT("startValueSh2",   Show_DataPacketType.START2);
    NEX_TXT_INT("fullValueShow2",  Show_DataPacketType.FULL2);

    // 喷淋时间
    NEX_TXT_INT("sprayOp",   Show_DataPacketType.RAIN_ONTIME);
    NEX_TXT_INT("sprayIdle", Show_DataPacketType.RAIN_OFFTIME);

    // 状态图标
    NEX_PIC("Pump1Stat", Show_DataPacketType.PUMP1_EN  ? PIC_ON : PIC_OFF);
    NEX_PIC("Pump2Stat", Show_DataPacketType.PUMP2_EN  ? PIC_ON : PIC_OFF);
    NEX_PIC("RainStat",  Show_DataPacketType.SPRAYMAIN ? PIC_ON : PIC_OFF);
    NEX_PIC("ExStat",    Show_DataPacketType.EX_VAL    ? PIC_ON : PIC_OFF);

    // 排气模式 / 单位
    NEX_TXT("valueStaus", Show_DataPacketType.EX_AUTO ? "AT" : "MT");
    NEX_TXT("unit",       Show_DataPacketType.Hz_Mv   ? "mV" : "Hz");

    // 泵占空比(0~100%)
    NEX_VAL("pump1", (TIM2->CCR3 + TIM2->CCR4) / 19);
    NEX_VAL("pump2", (TIM2->CCR1 + TIM2->CCR2) / 19);

    // 测试图标
    NEX_VIS("testIco", GetMode() == TEST_MODE);

    if(GetMode() == TEST_MODE)
        printf("mafValueShow.txt=\"%d\"\xff\xff\xff", GetTestData());

    // 告警图标初始隐藏
    NEX_VIS("FluidIco", 0);
    NEX_VIS("hiIco",    0);
    NEX_VIS("lowIco",   0);
}

/*------------------------------------------------------------------------------
 * 设置页
 *----------------------------------------------------------------------------*/
void Display_SettingPage(void)
{
    NEX_PAGE("page1", 50);

    // 泵 A
    NEX_VAL("STA1",        Show_DataPacketType.START1);
    NEX_VAL("FULL1",       Show_DataPacketType.FULL1);
    NEX_VAL("pumpEnable1", Show_DataPacketType.PUMP1_EN ? 1 : 0);

    // 泵 B
    NEX_VAL("STA2",        Show_DataPacketType.START2);
    NEX_VAL("FULL2",       Show_DataPacketType.FULL2);
    NEX_VAL("pumpEnable2", Show_DataPacketType.PUMP2_EN ? 1 : 0);

    // 喷淋
    NEX_VAL("spraymain",   Show_DataPacketType.SPRAYMAIN ? 1 : 0);
    NEX_VAL("rainOnTime",  Show_DataPacketType.RAIN_ONTIME);
    NEX_VAL("rainOffTime", Show_DataPacketType.RAIN_OFFTIME);

    // 排气
    NEX_VAL("setExAuto",   Show_DataPacketType.EX_AUTO ? 1 : 0);
    NEX_VAL("EX_SET",      Show_DataPacketType.EX_SET);
    NEX_VAL("exDelay",     Show_DataPacketType.EX_DELAY);

    // 泵占空比
    NEX_VAL("pumpStdDuty", Show_DataPacketType.PUMP_STDUTY);

    // 显示
    NEX_VAL("mafTypeSelect", Show_DataPacketType.Hz_Mv == MV_MODE ? 1 : 0);
    NEX_VAL("lightSen",      Show_DataPacketType.LightSen);
    NEX_VAL("bright",        Show_DataPacketType.Bright);

    // MAF 标定
    NEX_VAL("flex0",   Show_DataPacketType.FLEX0);
    NEX_VAL("flex100", Show_DataPacketType.FLEX100);

    // 流量
    NEX_VAL("Fluidmain", Show_DataPacketType.FLUID_MAIN);

    // 排气反向
    NEX_VAL("Ex_Rev", Show_DataPacketType.EX_REV);

    // 温度单位
    NEX_VAL("Temp_Select", Show_DataPacketType.TEMP_UINT);

    // 测试值
    NEX_VAL("mafValueInput", Show_DataPacketType.TEST_SET);

    // 按钮复位
    NEX_VAL("setSave", 0);
    if(test_en)
        printf("testCmd.val=1\xff\xff\xff");
    else
        printf("testCmd.val=0\xff\xff\xff");
    HAL_Delay(10);
    NEX_VAL("outSave", 0);
}

/*------------------------------------------------------------------------------
 * 模式 / 数据访问
 *----------------------------------------------------------------------------*/
void SetMode(uint8_t mode)
{
    Mode = mode;
}

uint8_t GetMode(void)
{
    return Mode;
}

uint16_t GetTestData(void)
{
    return Show_DataPacketType.TEST_SET;
}

/*------------------------------------------------------------------------------
 * 出厂页 — 校准参数 + 自动校准
 *  输入 2.5V 标准电压后,自动计算校准系数并刷新到屏幕
 *  公式: coeff = 2500 × 40950000 / (3300 × ADC)
 *----------------------------------------------------------------------------*/
#define CAL_VREF_MV     2500U

static uint16_t Cal_Coeff(uint16_t adc)
{
    if (adc == 0) return 0;
    return (uint16_t)((uint64_t)CAL_VREF_MV * 40950000UL / ((uint64_t)3300UL * adc));
}

static uint32_t factory_cal_tick = 0;

void Display_FactoryPage(void)
{
    // 静态参数(每次进页发送)
    NEX_VAL("mafvadj",     Show_DataPacketType.MAF_ADJ);
    NEX_VAL("ethvadj",     Show_DataPacketType.ETH_ADJ);
    NEX_VAL("afrvadj",     Show_DataPacketType.AFR_ADJ);

    NEX_VAL("level1",      Show_DataPacketType.LEVEL1_VAL);
    NEX_VAL("level2",      Show_DataPacketType.LEVEL2_VAL);
    NEX_VAL("level3",      Show_DataPacketType.LEVEL3_VAL);

    NEX_VAL("dac1_adj",    Show_DataPacketType.DAC1_ADJ);
    NEX_VAL("dac2_adj",    Show_DataPacketType.DAC2_ADJ);

    NEX_VAL("version",     SW_VERSION);
    NEX_VAL("lightsensor", ADvalue[CH_LIGHT_SENS]);

    // 自动校准: 1s 刷新一次,读取当前 ADC 计算系数
    uint32_t now = HAL_GetTick();
    if (now - factory_cal_tick >= 1000)
    {
        factory_cal_tick = now;
        NEX_VAL("mafvadj", Cal_Coeff(AD_MAF_MV));
        NEX_VAL("ethvadj", Cal_Coeff(AD_RESERVE_B0));
        NEX_VAL("afrvadj", Cal_Coeff(AD_RESERVE_A0));
    }
}

/*------------------------------------------------------------------------------
 * 后台亮度控制(主循环调用)
 *  自动模式(LightSen≠0): ADC 光敏值线性映射 0~4095 → 0~100 %
 *  手动模式(LightSen==0): 直接使用 Bright 设定值
 *----------------------------------------------------------------------------*/
static uint8_t last_dim = 0;

void Display_BackgroundSetting(void)
{
    uint8_t dim;

    if (Show_DataPacketType.LightSen != 0)
        dim = (uint8_t)((uint32_t)AD_LIGHT_SENS * 100U / 4095U);
    else
        dim = (uint8_t)Show_DataPacketType.Bright;

		if(dim<=5) dim=5;
    if (dim != last_dim)
    {
        last_dim = dim;
        printf("dim=%d\xff\xff\xff", dim);
        HAL_Delay(10);
    }
}

/*------------------------------------------------------------------------------
 * 告警图标分时显示(主循环调用,1s 轮切)
 *  FluidIco — 液位低(FLUID_MAIN 开启且 ADC < LEVEL1_VAL)
 *  hiIco    — 电瓶过压(ADC > BAT_ADC_HIGH)
 *  lowIco   — 电瓶欠压(ADC < BAT_ADC_LOW)
 *  三个图标同位置,多告警时轮流显示
 *----------------------------------------------------------------------------*/
#define BAT_ADC_HIGH    3500
#define BAT_ADC_LOW     2400
#define LIQUID_LOW      2600

static const char * const warn_icons[] = {"FluidIco", "hiIco", "lowIco"};
static uint8_t  warn_shown = 0xFF;
static uint8_t  warn_slot  = 0;
static uint32_t warn_tick  = 0;

void Display_Warning(void)
{
    uint32_t now = HAL_GetTick();
    if (now - warn_tick < 1000)
        return;
    warn_tick = now;

    // 收集活跃告警
    uint8_t active[3];
    uint8_t count = 0;

    if (Show_DataPacketType.FLUID_MAIN != 0 &&
        AD_LIQUID_LVL < LIQUID_LOW)
        active[count++] = 0;

    if (AD_BAT_VOLT > BAT_ADC_HIGH)
        active[count++] = 1;

    if (AD_BAT_VOLT < BAT_ADC_LOW)
        active[count++] = 2;

    // 隐藏当前图标
    if (warn_shown < 3)
    {
        printf("vis %s,0\xff\xff\xff", warn_icons[warn_shown]);
        HAL_Delay(10);
    }

    // 显示下一个活跃告警(轮切)
    if (count > 0)
    {
        warn_slot %= count;
        warn_shown = active[warn_slot++];
        printf("vis %s,1\xff\xff\xff", warn_icons[warn_shown]);
        HAL_Delay(10);
    }
    else
    {
        warn_shown = 0xFF;
        warn_slot  = 0;
    }
}

/*------------------------------------------------------------------------------
 * 测试模式: 根据 TEST_SET 直接设置泵 PWM
 *  base = PUMP_STDUTY(%) × 10 → CCR 基础占空比
 *  映射: TEST_SET < START → 0, START~FULL → base~950, > FULL → 950
 *----------------------------------------------------------------------------*/
static void TestPump_SetDuty(__IO uint32_t *ccr_a, __IO uint32_t *ccr_b,
                             uint16_t enable, uint16_t start, uint16_t full)
{
    if (!enable)
    {
        *ccr_a = 0;
        *ccr_b = 0;
        return;
    }

    uint32_t val = Show_DataPacketType.TEST_SET;

    if (val > full)
    {
        *ccr_a = 950;
        *ccr_b = 950;
    }
    else if (val >= start && full > start)
    {
        uint32_t base = (uint32_t)Show_DataPacketType.PUMP_STDUTY * 950 / 100;
        uint32_t d = base + (val - start) * (950 - base) / (full - start);
        *ccr_a = d;
        *ccr_b = d;
    }
    else
    {
        *ccr_a = 0;
        *ccr_b = 0;
    }
}

static void TestPumpOutput(void)
{
    TestPump_SetDuty(&TIM2->CCR3, &TIM2->CCR4,
                     Show_DataPacketType.PUMP1_EN,
                     Show_DataPacketType.START1,
                     Show_DataPacketType.FULL1);

    TestPump_SetDuty(&TIM2->CCR1, &TIM2->CCR2,
                     Show_DataPacketType.PUMP2_EN,
                     Show_DataPacketType.START2,
                     Show_DataPacketType.FULL2);
}

/*------------------------------------------------------------------------------
 * 设置同步:屏幕下发 → Flash + 本地缓存
 *----------------------------------------------------------------------------*/
void Refresh_Setting(void)
{
    if (!DataPacket_Type.updated)
        return;

    DataPacket_Type.updated = 0;

    if (memcmp(&DataPacket_Type, &Show_DataPacketType, sizeof(DataPacket_Struct)) != 0)
    {
        Flash_WriteSetting((uint16_t *)&DataPacket_Type, sizeof(DataPacket_Type));
        Flash_ReadSetting((uint16_t *)&Show_DataPacketType, sizeof(DataPacket_Type));
    }

    uint16_t cmd = GetDisplay_Cmd();

    switch (cmd)
    {
    case BACK_CMD:
    case SAVE_CMD:
        SetMode(test_en ? TEST_MODE : NORMAL_MODE);
        Display_StartPage();
        break;
    case PAGESETTING_CMD:
        SetMode(SETTING_MODE);
        Display_SettingPage();
        break;
    case TEST_CMD:
        if (test_en)
            TestPumpOutput();
        break;
    case FACTORY_MODE_CMD:
        SetMode(FACTORY_MODE);
        Set_DAC1(4095);
        Set_DAC2(4095);
        break;
    case FACTORY_SAVE_CMD:
        test_en = 0;
        SetMode(NORMAL_MODE);
        Set_DAC1(0);
        Set_DAC2(0);
        Display_StartPage();
        break;
    default:
        break;
    }
}
