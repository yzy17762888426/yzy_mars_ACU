#include "display_show.h"
#include "display_comm.h"
#include "main.h"
#include "stdio.h"
#include "string.h"

/*------------------------------------------------------------------------------
 * Nextion 串口屏发送宏
 *  所有宏在发送后自动加 HAL_Delay(10),保证屏幕有处理时间
 *  NEX_PAGE 需要更长延迟,由调用方指定
 *----------------------------------------------------------------------------*/


#define PIC_OFF        8
#define PIC_ON         9


#define NEX_VAL(name, v)       do { printf("name.val=%d\xff\xff\xff", (int)(v));          HAL_Delay(10); } while (0)
#define NEX_TXT(name, t)       do { printf("name.txt=\"%s\"\xff\xff\xff", (t));           HAL_Delay(10); } while (0)
#define NEX_TXT_INT(name, v)   do { printf("name.txt=\"%d\"\xff\xff\xff", (int)(v));      HAL_Delay(10); } while (0)
#define NEX_PIC(name, p)       do { printf("name.pic=%d\xff\xff\xff", (int)(p));          HAL_Delay(10); } while (0)
#define NEX_VIS(obj, on)       do { printf("vis obj,%d\xff\xff\xff", (on) ? 1 : 0);   HAL_Delay(10); } while (0)
#define NEX_PAGE(name, ms)     do { printf("page name\xff\xff\xff"); HAL_Delay(ms); } while (0)

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
    //NEX_PAGE(page0, 500);
		printf("page page0\xff\xff\xff");

    // 起跳 / 满量
		printf("startValueShow.txt=\"%d\"\xff\xff\xff",Show_DataPacketType.START1);
		printf("fullValueShow.txt=\"%d\"\xff\xff\xff",Show_DataPacketType.FULL1);
		printf("startValueSh2.txt=\"%d\"\xff\xff\xff",Show_DataPacketType.START2);
		printf("fullValueShow2.txt=\"%d\"\xff\xff\xff",Show_DataPacketType.FULL2);

    // 喷淋时间
		printf("sprayOp.txt=\"%d\"\xff\xff\xff",Show_DataPacketType.RAIN_ONTIME);
		printf("sprayIdle.txt=\"%d\"\xff\xff\xff",Show_DataPacketType.RAIN_OFFTIME);

    // 状态图标
		printf("Pump1Stat.pic=%d\xff\xff\xff",Show_DataPacketType.PUMP1_EN? PIC_ON : PIC_OFF);
		printf("Pump2Stat.pic=%d\xff\xff\xff",Show_DataPacketType.PUMP2_EN? PIC_ON : PIC_OFF);
		printf("RainStat.pic=%d\xff\xff\xff",Show_DataPacketType.SPRAYMAIN? PIC_ON : PIC_OFF);
		printf("ExStat.pic=%d\xff\xff\xff",Show_DataPacketType.EX_VAL? PIC_ON : PIC_OFF);

    // 排气模式 / 单位
		printf("valueStaus.txt=\"%s\"\xff\xff\xff",Show_DataPacketType.EX_AUTO ? "AT" : "MT");
		printf("unit.txt=\"%s\"\xff\xff\xff",Show_DataPacketType.Hz_Mv   ? "MV" : "Hz");


    // 测试图标可见性
    if (GetMode() == NORMAL_MODE)
			printf("vis testIco,0\xff\xff\xff");
    else if (GetMode() == TEST_MODE)
      printf("vis testIco,1\xff\xff\xff");

    // 告警图标初始隐藏
		printf("vis FluidIco,0\xff\xff\xff");
		printf("vis hiIco,0\xff\xff\xff");
		printf("vis lowIco,0\xff\xff\xff");
}

/*------------------------------------------------------------------------------
 * 设置页
 *----------------------------------------------------------------------------*/
void Display_SettingPage(void)
{
    printf("page page1\xff\xff\xff");

    // 泵 A
    printf("STA1.val=%d\xff\xff\xff",Show_DataPacketType.START1);
    printf("FULL1.val=%d\xff\xff\xff", Show_DataPacketType.FULL1);
    printf("pumpEnable1.val=%d\xff\xff\xff", Show_DataPacketType.PUMP1_EN ? 1 : 0);

    // 泵 B
    printf("STA2.val=%d\xff\xff\xff",Show_DataPacketType.START2);
    printf("FULL2.val=%d\xff\xff\xff", Show_DataPacketType.FULL2);
    printf("pumpEnable2.val=%d\xff\xff\xff", Show_DataPacketType.PUMP2_EN ? 1 : 0);

    // 喷淋
    printf("sprayMain.val=%d\xff\xff\xff",Show_DataPacketType.SPRAYMAIN ? 1 : 0);
    printf("rainOnTime.val=%d\xff\xff\xff", Show_DataPacketType.RAIN_ONTIME);
    printf("rainOffTime.val=%d\xff\xff\xff", Show_DataPacketType.RAIN_OFFTIME);

    // 排气
    printf("setExAuto.val=%d\xff\xff\xff",Show_DataPacketType.EX_AUTO ? 1 : 0);
    printf("EX_SET.val=%d\xff\xff\xff", Show_DataPacketType.EX_SET);
    printf("exDelay.val=%d\xff\xff\xff", Show_DataPacketType.EX_DELAY);

    // 泵占空比
    printf("pumpStdDuty.val=%d\xff\xff\xff", Show_DataPacketType.PUMP_STDUTY);

    // 显示
    printf("mafTypeSelect.val=%d\xff\xff\xff",Show_DataPacketType.Hz_Mv == MV_MODE ? 1 : 0);
    printf("lightSen.val=%d\xff\xff\xff", Show_DataPacketType.LightSen);
    printf("bright.val=%d\xff\xff\xff", Show_DataPacketType.Bright);

    // MAF 标定
    printf("flex0.val=%d\xff\xff\xff", Show_DataPacketType.FLEX0);
    printf("flex100.val=%d\xff\xff\xff", Show_DataPacketType.FLEX100);

    // 流量
    printf("Fluidmain.val=%d\xff\xff\xff", Show_DataPacketType.FLUID_MAIN);

    // 排气反向
    printf("Ex_Rev.val=%d\xff\xff\xff", Show_DataPacketType.EX_REV);

    // 温度单位
    printf("Temp_Select.val=%d\xff\xff\xff", Show_DataPacketType.TEMP_UINT);

    // 测试值
    printf("mafValueInput.val=%d\xff\xff\xff", Show_DataPacketType.TEST_SET);

    // 按钮复位
    printf("setSave.val=0\xff\xff\xff");
    printf("testCmd.val=0\xff\xff\xff");
    printf("outSave.val=0\xff\xff\xff");
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
    NEX_VAL("mafvadj", Show_DataPacketType.MAF_ADJ);
    NEX_VAL("ethvadj", Show_DataPacketType.ETH_ADJ);
    NEX_VAL("afrvadj", Show_DataPacketType.AFR_ADJ);

    NEX_VAL("level1", Show_DataPacketType.LEVEL1_VAL);
    NEX_VAL("level2", Show_DataPacketType.LEVEL2_VAL);
    NEX_VAL("level3", Show_DataPacketType.LEVEL3_VAL);

    NEX_VAL("dac1_adj", Show_DataPacketType.DAC1_ADJ);
    NEX_VAL("dac2_adj", Show_DataPacketType.DAC2_ADJ);

    NEX_VAL("version",      SW_VERSION);
    NEX_VAL("lightsensor",  ADvalue[CH_LIGHT_SENS]);

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
    {
        dim = (uint8_t)((uint32_t)AD_LIGHT_SENS * 100U / 4095U);
    }
    else
    {
        dim = (uint8_t)Show_DataPacketType.Bright;
    }

    if (dim != last_dim)
    {
        last_dim = dim;
//        printf("dim=%d" NEX_END, dim);
    }
}

/*------------------------------------------------------------------------------
 * 告警图标分时显示(主循环调用,1s 轮切)
 *  FluidIco — 液位低(FLUID_MAIN 开启且 ADC < LEVEL1_VAL)
 *  hiIco    — 电瓶过压(ADC > BAT_ADC_HIGH)
 *  lowIco   — 电瓶欠压(ADC < BAT_ADC_LOW)
 *  三个图标同位置,多告警时轮流显示
 *----------------------------------------------------------------------------*/
#define BAT_ADC_HIGH    3500    // 电瓶过压 ADC 阈值,需根据实际分压校准
#define BAT_ADC_LOW     2400    // 电瓶欠压 ADC 阈值
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
//        printf("vis %s,0" NEX_END, warn_icons[warn_shown]);
        HAL_Delay(10);
    }

    // 显示下一个活跃告警(轮切)
    if (count > 0)
    {
        warn_slot %= count;
        warn_shown = active[warn_slot++];
//        printf("vis %s,1" NEX_END, warn_icons[warn_shown]);
        HAL_Delay(10);
    }
    else
    {
        warn_shown = 0xFF;
        warn_slot  = 0;
    }
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

        // Hz/Mv 模式切换需要重启
        if (DataPacket_Type.Hz_Mv != Show_DataPacketType.Hz_Mv)
        {
            HAL_Delay(500);
            __set_FAULTMASK(1);
            NVIC_SystemReset();
        }

        Flash_ReadSetting((uint16_t *)&Show_DataPacketType, sizeof(DataPacket_Type));
    }

    uint16_t cmd = GetDisplay_Cmd();

    switch (cmd)
    {
    case BACK_CMD:
    case SAVE_CMD:
        Display_StartPage();
        break;
    case PAGESETTING_CMD:
				SetMode(SETTING_MODE);
        Display_SettingPage();
        break;
    case FACTORY_MODE_CMD:
        SetMode(FACTORY_MODE);
        Set_DAC1(4095);   // 出厂模式: DAC 输出满量程(外部放大至 5V)
        Set_DAC2(4095);
        break;
    case FACTORY_SAVE_CMD:
        SetMode(NORMAL_MODE);
        Set_DAC1(0);
        Set_DAC2(0);
        Display_StartPage();
        break;
    default:
        break;
    }
}
