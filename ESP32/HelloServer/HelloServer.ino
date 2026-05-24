/*
 * Y&M ACU - ESP32-C6 WiFi 控制面板
 * 解析 STM32 USART3 广播的 Nextion 指令，手机网页实时显示
 *
 * 串口: 如果 Arduino IDE 开启了 "USB CDC On Boot" → 用 Serial0
 *        如果没开启 → 改为 Serial
 */
#include <WiFi.h>
#include <NetworkClient.h>
#include <WebServer.h>

#define STM32_SER  Serial0   // ← 编译不过就改为 Serial

const char* AP_SSID = "ACU Center";
const char* AP_PASS = "acu666666";
WebServer server(80);

/* ========================================================================
 * 全局数据 — 从 STM32 Nextion 输出中解析
 * ======================================================================== */
// --- 主页 ---
static int16_t g_maf = 0;                // mafValueShow.txt
static char    g_unit[8]   = "Hz";       // unit.txt
static int16_t g_p1_duty   = 0;          // pump1.val
static int16_t g_p2_duty   = 0;          // pump2.val
static int8_t  g_p1_en     = 0;          // Pump1Stat.pic  9=on
static int8_t  g_p2_en     = 0;          // Pump2Stat.pic
static int8_t  g_ex_valve  = 0;          // ExStat.pic
static int8_t  g_spray     = 0;          // RainStat.pic
static int8_t  g_spray_en  = 0;          // rainSwith.aph → 1=enabled, 0=disabled
static char    g_ex_mode[4]= "AT";       // valueStaus.txt
static int8_t  g_ex_auto   = 1;          // EX_AUTO: 1=AT, 0=MT
static int16_t g_p1_start  = 0;          // startValueShow.txt
static int16_t g_p1_full   = 0;          // fullValueShow.txt
static int16_t g_p2_start  = 0;          // startValueSh2.txt
static int16_t g_p2_full   = 0;          // fullValueShow2.txt
static int16_t g_sp_on     = 0;          // sprayOp.txt
static int16_t g_sp_off    = 0;          // sprayIdle.txt
static int16_t g_ethanol   = 0;          // eValueShow.txt  (0.1% 单位, 0~1000)
static int16_t g_temp_x10  = 0;          // tValueShow.txt  (0.1° 单位, 可负)
static int8_t  g_flex_ok   = 1;          // 1=传感器在, 0=未插入
static int8_t  g_warn_fluid = 0;         // FluidIco vis
static int8_t  g_warn_hi    = 0;         // hiIco vis (电瓶过压)
static int8_t  g_warn_low   = 0;         // lowIco vis (电瓶欠压)
static int8_t  g_test_mode  = 0;         // testIco vis

// --- 设置页 ---
static int16_t g_s_p1en    = 0;          // pumpEnable1.val
static int16_t g_s_sta1    = 0;          // STA1.val
static int16_t g_s_full1   = 0;          // FULL1.val
static int16_t g_s_p2en    = 0;          // pumpEnable2.val
static int16_t g_s_sta2    = 0;          // STA2.val
static int16_t g_s_full2   = 0;          // FULL2.val
static int16_t g_s_spm     = 0;          // spraymain.val
static int16_t g_s_spon    = 0;          // rainOnTime.val
static int16_t g_s_spoff   = 0;          // rainOffTime.val
static int16_t g_s_exauto  = 0;          // setExAuto.val
static int16_t g_s_exset   = 0;          // EX_SET.val
static int16_t g_s_exdelay = 0;          // exDelay.val
static int16_t g_s_duty    = 0;          // pumpStdDuty.val
static int16_t g_s_mtype   = 0;          // mafTypeSelect.val
static int16_t g_s_lsen    = 0;          // lightSen.val
static int16_t g_s_bright  = 0;          // bright.val
static int16_t g_s_flex0   = 0;          // flex0.val
static int16_t g_s_flex100 = 0;          // flex100.val
static int16_t g_s_fluid   = 0;          // Fluidmain.val
static int16_t g_s_exrev   = 0;          // Ex_Rev.val
static int16_t g_s_tempu   = 0;          // Temp_Select.val
static int16_t g_s_testv   = 0;          // mafValueInput.val
static int16_t g_s_testen  = 0;          // testCmd.val

// --- 页面同步 (从 STM32 page 命令解析) ---
static int8_t  g_page     = 0;          // 0=main 1=setting

/* ---- 数据记录 (环形缓冲区, 固定 char 数组避免堆碎片) ---- */
#define LOG_ENTRY_SIZE 32
#define MAX_RECORDS 6000
char   logData[MAX_RECORDS][LOG_ENTRY_SIZE];
int    recordHead = 0;   // 下一条写入位置
int    recordCount = 0;  // 已存储条数 (最大 MAX_RECORDS)
bool   isLogging   = false;
unsigned long startTime = 0;

/* ========================================================================
 * 串口解析 — Nextion 协议 (\xff\xff\xff 结尾)
 * ======================================================================== */
static String cmdBuf;
static int    ffCount = 0;

static int16_t* findValTarget(const String &name) {
  if (name == "pump1")         return &g_p1_duty;
  if (name == "pump2")         return &g_p2_duty;
  if (name == "STA1")          return &g_s_sta1;
  if (name == "FULL1")         return &g_s_full1;
  if (name == "pumpEnable1")   return &g_s_p1en;
  if (name == "STA2")          return &g_s_sta2;
  if (name == "FULL2")         return &g_s_full2;
  if (name == "pumpEnable2")   return &g_s_p2en;
  if (name == "spraymain")     return &g_s_spm;
  if (name == "SPON")          return &g_s_spon;
  if (name == "SPIDLE")        return &g_s_spoff;
  if (name == "setExAuto")     return &g_s_exauto;
  if (name == "EX_SET")        return &g_s_exset;
  if (name == "EX_DEALY")      return &g_s_exdelay;
  if (name == "PumpStDuty")    return &g_s_duty;
  if (name == "mafTypeSelect") return &g_s_mtype;
  if (name == "LightSen")      return &g_s_lsen;
  if (name == "Bright")        return &g_s_bright;
  if (name == "flex0")         return &g_s_flex0;
  if (name == "flex100")       return &g_s_flex100;
  if (name == "Fluidmain")     return &g_s_fluid;
  if (name == "Ex_Rev")        return &g_s_exrev;
  if (name == "Temp_Select")   return &g_s_tempu;
  if (name == "mafValueInput") return &g_s_testv;
  if (name == "testCmd")       { return (int16_t*)&g_s_testen; } // same size
  return nullptr;
}

static void parseCmd(const String &cmd) {
  // 页面切换同步
  if (cmd.startsWith("page ")) {
    String pg = cmd.substring(5);
    if (pg == "page0")      g_page = 0;
    else if (pg == "page1") g_page = 1;
    return;
  }

  if (cmd.startsWith("vis ")) {
    int comma = cmd.indexOf(',');
    if (comma > 4) {
      String obj = cmd.substring(4, comma);
      int on = cmd.substring(comma + 1).toInt();
      if      (obj == "FluidIco") g_warn_fluid = on;
      else if (obj == "hiIco")    g_warn_hi    = on;
      else if (obj == "lowIco")   g_warn_low   = on;
      else if (obj == "testIco")  g_test_mode  = on;
    }
    return;
  }

  int dot = cmd.indexOf('.');
  if (dot < 0) return;
  String name = cmd.substring(0, dot);
  String prop = cmd.substring(dot + 1);

  if (prop.startsWith("txt=")) {
    int q1 = prop.indexOf('"'), q2 = prop.lastIndexOf('"');
    if (q1 < 0 || q2 <= q1) return;
    String v = prop.substring(q1 + 1, q2);
    if      (name == "mafValueShow")   g_maf = v.toInt();
    else if (name == "valueStaus")     { strncpy(g_ex_mode, v.c_str(), 3); g_ex_mode[3] = 0; g_ex_auto = (g_ex_mode[0] == 'A'); }
    else if (name == "unit")           { strncpy(g_unit, v.c_str(), 7); g_unit[7] = 0; }
    else if (name == "startValueShow") g_p1_start = v.toInt();
    else if (name == "fullValueShow")  g_p1_full  = v.toInt();
    else if (name == "startValueSh2")  g_p2_start = v.toInt();
    else if (name == "fullValueShow2") g_p2_full  = v.toInt();
    else if (name == "sprayOp")        g_sp_on    = v.toInt();
    else if (name == "sprayIdle")      g_sp_off   = v.toInt();
    else if (name == "eValueShow")  { if (v == "-") { g_ethanol = 0; g_flex_ok = 0; } else { g_ethanol = (int16_t)(v.toFloat() * 10); g_flex_ok = 1; } }
    else if (name == "tValueShow")  { if (v == "-") g_temp_x10 = -32768; else if (g_flex_ok) g_temp_x10 = (int16_t)(v.toFloat() * 10); }
  }
  else if (prop.startsWith("val=")) {
    int16_t *t = findValTarget(name);
    if (t) *t = (int16_t)prop.substring(4).toInt();
  }
  else if (prop.startsWith("pic=")) {
    int v = prop.substring(4).toInt();
    if      (name == "ExStat")    g_ex_valve = (v == 9);
    else if (name == "Pump1Stat") g_p1_en    = (v == 9);
    else if (name == "Pump2Stat") g_p2_en    = (v == 9);
    else if (name == "RainStat")  g_spray    = (v == 9);
    else if (name == "ATMT")    { g_ex_auto = (v == 17); strncpy(g_ex_mode, g_ex_auto ? "AT" : "MT", 3); g_ex_mode[3] = 0; }
  }
  else if (prop.startsWith("aph=")) {
    int v = prop.substring(4).toInt();
    if (name == "rainSwith")  g_spray_en = (v >= 127) ? 1 : 0;
  }
}

static void updateSerial() {
  while (STM32_SER.available()) {
    uint8_t b = STM32_SER.read();
    if (b == 0xFF) {
      if (++ffCount >= 3) {
        cmdBuf.trim();
        if (cmdBuf.length() > 0) parseCmd(cmdBuf);
        cmdBuf.clear();
        ffCount = 0;
      }
    } else {
      if (ffCount > 0) {
        for (int i = 0; i < ffCount; i++) cmdBuf += (char)0xFF;
        ffCount = 0;
      }
      cmdBuf += (char)b;
    }
  }
}

/* ========================================================================
 * 命令下发 — Nextion 二进制协议
 *   帧格式: AA AA [CMD_L CMD_H] [PAYLOAD...] [CRC_L CRC_H] 55 55
 * ======================================================================== */
static uint16_t crc16(const uint8_t *d, uint16_t len) {
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < len; i++) {
    crc ^= d[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 1) { crc >>= 1; crc ^= 0xA001; }
      else crc >>= 1;
    }
  }
  return crc;
}

static void sendFrame(uint16_t cmd, const uint8_t *payload, uint8_t plen) {
  uint8_t buf[64];
  buf[0] = (cmd >> 8) & 0xFF;   // CMD 高字节在前 (大端，与 STM32 一致)
  buf[1] = cmd & 0xFF;
  if (payload && plen > 0) memcpy(buf + 2, payload, plen);
  uint16_t crc = crc16(buf, 2 + plen);
  STM32_SER.write(0xAA); STM32_SER.write(0xAA);
  STM32_SER.write(buf, 2 + plen);
  STM32_SER.write(crc & 0xFF); STM32_SER.write((crc >> 8) & 0xFF);
  STM32_SER.write(0x55); STM32_SER.write(0x55);
}

static void sendFrameCmdOnly(uint16_t cmd) { sendFrame(cmd, nullptr, 0); }

static void sendSaveCmd() {
  uint8_t p[44]; int o = 0;
  #define P16(v) { p[o++]=(v)&0xFF; p[o++]=(v)>>8; }
  P16(g_s_p1en);  P16(g_s_sta1);   P16(g_s_full1);
  P16(g_s_p2en);  P16(g_s_sta2);   P16(g_s_full2);
  P16(g_s_spm);   P16(g_s_spon);   P16(g_s_spoff);
  P16(g_s_exauto);P16(g_s_exset);  P16(g_s_exdelay);
  P16(g_s_duty);  P16(g_s_mtype);  P16(g_s_lsen);  P16(g_s_bright);
  P16(g_s_flex0); P16(g_s_flex100);P16(g_s_fluid);
  P16(g_s_exrev); P16(g_s_tempu);
  #undef P16
  sendFrame(0xF10A, p, o);
}

/* ========================================================================
 * HTML 页面
 * ======================================================================== */
const char* HTML_MAIN = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="utf-8">
<title>Y&amp;M ACU</title>
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<style>
*{margin:0;padding:0;box-sizing:border-box}
html,body{width:100%;min-height:100%;font-family:'Segoe UI',Roboto,sans-serif;background:#121212;color:#eee}
.app{max-width:480px;margin:0 auto;background:#1a1a1a;min-height:100vh;display:flex;flex-direction:column}
/* tabs */
.tabs{display:flex;background:#111;border-bottom:2px solid #333}
.tab{flex:1;padding:12px 0;text-align:center;font-size:14px;font-weight:700;color:#888;cursor:pointer;border-bottom:3px solid transparent}
.tab.on{color:#ff9800;border-bottom-color:#ff9800}
/* page */
.pg{display:none;flex:1;padding:16px;overflow-y:auto}
.pg.on{display:block}
/* card */
.cd{background:#0d0d0d;border-radius:16px;padding:16px;margin-bottom:14px;border:1px solid #222}
.sec{color:#ff9800;font-size:13px;font-weight:700;letter-spacing:1px;margin-bottom:10px;padding-bottom:6px;border-bottom:1px solid #222}
/* MAF */
.maf{font-size:clamp(30px,9vw,48px);font-weight:600;font-family:Consolas,monospace;text-align:center;letter-spacing:2px;margin-bottom:16px;text-shadow:2px 0 rgba(255,0,0,.3),-2px 0 rgba(0,255,255,.3)}
.bw{width:100%;height:36px;background:#333;border-radius:10px;overflow:hidden;margin-bottom:8px}
.bar{height:100%;width:0;background:linear-gradient(90deg,#4caf50,#ff9800,#f44336);transition:width .15s}
.sc{display:flex;justify-content:space-between;font-size:12px;color:#666;font-family:Consolas,monospace;padding:0 4px}
/* rows */
.row{display:flex;justify-content:space-between;align-items:center;padding:10px 0;border-bottom:1px solid #1a1a1a}
.row:last-child{border-bottom:none}
.lbl{color:#bbb;font-size:14px}
.val{color:#fff;font-weight:700;font-family:Consolas,monospace;font-size:15px}
/* dot */
.dt{display:inline-block;width:10px;height:10px;border-radius:50%;margin-right:6px;vertical-align:middle}
.dt.on{background:#4caf50}.dt.off{background:#555}
/* input */
.inp{width:70px;background:#222;border:1px solid #444;border-radius:8px;color:#fff;font-family:Consolas,monospace;font-size:13px;padding:5px 6px;text-align:center}
.inp:focus{border-color:#ff9800;outline:none}
/* select */
.sel{width:70px;background:#222;border:1px solid #444;border-radius:8px;color:#fff;font-family:Consolas,monospace;font-size:13px;padding:5px 6px;text-align:center;text-align-last:center;appearance:none;-webkit-appearance:none}
.sel:focus{border-color:#ff9800;outline:none}
/* toggle */
.sw{position:relative;width:44px;height:24px;display:inline-block}
.sw input{opacity:0;width:0;height:0}
.sl{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background:#444;border-radius:24px;transition:.2s}
.sl:before{content:'';position:absolute;width:18px;height:18px;left:3px;bottom:3px;background:#fff;border-radius:50%;transition:.2s}
.sw input:checked+.sl{background:#4caf50}
.sw input:checked+.sl:before{transform:translateX(20px)}
/* buttons */
.btn{padding:12px;border:none;border-radius:12px;font-size:15px;font-weight:700;color:#fff;cursor:pointer;width:100%;margin-top:6px}
.btn:active{transform:scale(.97)}
.bg{background:#0a9d58}.br{background:#991a2c}.bo{background:#ff9800}.bb{background:#333}
.sm{padding:8px 16px;border-radius:10px;font-size:13px;font-weight:700;color:#fff;border:none;cursor:pointer}
.sm:active{transform:scale(.96)}
.smg{background:#0a9d58}.smr{background:#991a2c}.smo{background:#ff9800}.sm.disabled{opacity:.35;pointer-events:none}
.brow{display:flex;gap:8px;margin-top:8px}
.foot{text-align:center;font-size:11px;color:#444;padding:12px}
</style>
</head><body>
<div class="app">

<!-- ===== TABS ===== -->
<div class="tabs">
  <div class="tab on" onclick="swPage(0)">Display</div>
  <div class="tab" onclick="swPage(1)">Setting</div>
</div>

<!-- ===== PAGE 0: 显示 ===== -->
<div id="p0" class="pg on">
  <div id="warnBar" style="display:none;background:#3a1515;border:1px solid #f44336;border-radius:10px;padding:8px 14px;margin-bottom:10px;text-align:center;font-weight:700;font-size:14px;color:#f44336;overflow:hidden;white-space:nowrap"></div>
  <div id="testBanner" style="display:none;background:#ff9800;color:#000;text-align:center;padding:6px;font-weight:700;border-radius:10px;margin-bottom:10px">TEST MODE</div>
  <div class="cd">
    <div class="sec">MAF</div>
    <div id="nMaf" class="maf">0 Hz</div>
    <div class="bw"><div id="bMaf" class="bar"></div></div>
    <div class="sc"><span>0%</span><span>50%</span><span>100%</span></div>
  </div>
  <div class="cd">
    <div class="sec">PUMP OUTPUT</div>
    <div class="row"><span class="lbl"><span id="dP1" class="dt off"></span>Pump A</span><span id="vP1" class="val">0%</span></div>
    <div class="bw"><div id="bP1" class="bar" style="background:#4caf50"></div></div>
    <div class="row" style="margin-top:10px"><span class="lbl"><span id="dP2" class="dt off"></span>Pump B</span><span id="vP2" class="val">0%</span></div>
    <div class="bw"><div id="bP2" class="bar" style="background:#2196f3"></div></div>
  </div>
  <div class="cd">
    <div class="sec">ETHANOL &amp; TEMP</div>
    <div class="row"><span class="lbl">Ethanol</span><span id="vEth" class="val">0.0%</span></div>
    <div class="row"><span class="lbl">Fuel Temp</span><span id="vTmp" class="val">0.0</span></div>
  </div>
  <div class="cd">
    <div class="sec">STATUS</div>
    <div class="row"><span class="lbl">Exhaust Mode</span><span id="vExM" class="val">AT</span></div>
    <div class="row"><span class="lbl"><span id="dEx" class="dt off"></span>Status</span><span id="vExS" class="val">CLOSED</span></div>
    <div class="row"><span class="lbl">Spray Enable</span><span id="vSpE" class="val">OFF</span></div>
    <div class="row"><span class="lbl"><span id="dSp" class="dt off"></span>Status</span><span id="vSpS" class="val">OFF</span></div>
  </div>
  <div class="cd">
    <div class="sec">THRESHOLD</div>
    <div class="row"><span class="lbl">Pump A Start</span><span id="vP1S" class="val">0</span></div>
    <div class="row"><span class="lbl">Pump A Full</span><span id="vP1F" class="val">0</span></div>
    <div class="row"><span class="lbl">Pump B Start</span><span id="vP2S" class="val">0</span></div>
    <div class="row"><span class="lbl">Pump B Full</span><span id="vP2F" class="val">0</span></div>
    <div class="row"><span class="lbl">Spray On/Off</span><span id="vSpT" class="val">0/0 s</span></div>
    <div class="row"><span class="lbl">Ex Threshold</span><span id="vExT" class="val">0</span></div>
    <div class="row"><span class="lbl">Ex Delay</span><span id="vExD" class="val">0 ms</span></div>
    <div class="row"><span class="lbl">Pump Duty</span><span id="vPD" class="val">0%</span></div>
  </div>
  <div class="cd">
    <div class="sec">EXHAUST</div>
    <div class="brow">
      <button class="sm smo" onclick="cmd('exmode')">AT / MT</button>
      <button class="sm smo" id="btnEx" onclick="cmdEx()">Ex Valve</button>
    </div>
  </div>
  <div class="cd">
    <div class="sec">SPRAY</div>
    <div class="brow">
      <button class="sm smo" onclick="cmd('spray')">Spray</button>
    </div>
  </div>
  <div class="cd">
    <div class="sec">LOG</div>
    <div class="brow">
      <button class="sm smg" id="btnLogStart" onclick="startRec()">Start Record</button>
      <button class="sm smr disabled" id="btnLogStop" onclick="stopRec()">Stop Record</button>
    </div>
  </div>
  <div class="foot">LIVE UPDATE 200ms</div>
</div>

<!-- ===== PAGE 1: 设置 ===== -->
<div id="p1" class="pg">
  <div class="cd">
    <div class="sec">PUMP A</div>
    <div class="row"><span class="lbl">Enable</span><label class="sw"><input type="checkbox" id="iPE1"><span class="sl"></span></label></div>
    <div class="row"><span class="lbl">Start</span><input type="number" id="iS1" class="inp"></div>
    <div class="row"><span class="lbl">Full</span><input type="number" id="iF1" class="inp"></div>
  </div>
  <div class="cd">
    <div class="sec">PUMP B</div>
    <div class="row"><span class="lbl">Enable</span><label class="sw"><input type="checkbox" id="iPE2"><span class="sl"></span></label></div>
    <div class="row"><span class="lbl">Start</span><input type="number" id="iS2" class="inp"></div>
    <div class="row"><span class="lbl">Full</span><input type="number" id="iF2" class="inp"></div>
  </div>
  <div class="cd">
    <div class="sec">SPRAY</div>
    <div class="row"><span class="lbl">Main</span><label class="sw"><input type="checkbox" id="iSPM"><span class="sl"></span></label></div>
    <div class="row"><span class="lbl">On Time (s)</span><input type="number" id="iSPO" class="inp"></div>
    <div class="row"><span class="lbl">Off Time (s)</span><input type="number" id="iSPF" class="inp"></div>
  </div>
  <div class="cd">
    <div class="sec">EXHAUST</div>
    <div class="row"><span class="lbl">Auto Mode</span><label class="sw"><input type="checkbox" id="iEXA"><span class="sl"></span></label></div>
    <div class="row"><span class="lbl">Threshold</span><input type="number" id="iEXS" class="inp"></div>
    <div class="row"><span class="lbl">Delay (s)</span><input type="number" id="iEXD" class="inp"></div>
  </div>
  <div class="cd">
    <div class="sec">DISPLAY</div>
    <div class="row"><span class="lbl">Airflow Unit</span><select id="iMT" class="sel"><option value="0">Hz</option><option value="1">mV</option></select></div>
    <div class="row"><span class="lbl">Pump Duty %</span><input type="number" id="iPD" class="inp"></div>
  </div>
  <div class="cd">
    <div class="sec">CALIBRATION</div>
    <div class="row"><span class="lbl">Flex 0%</span><input type="number" id="iFX0" class="inp"></div>
    <div class="row"><span class="lbl">Flex 100%</span><input type="number" id="iFX100" class="inp"></div>
    <div class="row"><span class="lbl">Fluid Enable</span><label class="sw"><input type="checkbox" id="iFM"><span class="sl"></span></label></div>
    <div class="row"><span class="lbl">Ex Reverse</span><label class="sw"><input type="checkbox" id="iER"><span class="sl"></span></label></div>
    <div class="row"><span class="lbl">Temp Unit</span><select id="iTU" class="sel"><option value="0">&#176;C</option><option value="1">&#176;F</option></select></div>
  </div>
  <div class="cd">
    <div class="sec">TEST MODE</div>
    <div class="row"><span class="lbl">Test Value</span><input type="number" id="iTV" class="inp"></div>
    <div class="row"><span class="lbl">Test Enable</span><label class="sw"><input type="checkbox" id="iTE"><span class="sl"></span></label></div>
  </div>
  <button class="btn bg" onclick="saveSetting()">SAVE SETTING</button>
  <button class="btn bb" onclick="swPage(0)" style="margin-top:8px">BACK</button>
  <div style="height:20px"></div>
</div>

</div><!-- app -->
<script>
var p1s=0,p1f=0,curPage=0;
// 用户已经手动改过的字段 — polling 不再覆盖这些,直到 SAVE/BACK
var userTouched={};
function clearTouched(){userTouched={};}
// 手动切页后短时间内忽略 polling 的 g_page 同步,防止旧响应把页面拉回去
var ignorePgSyncUntil=0;
// 排气当前模式: true=AT, false=MT
var exAuto=true;
function swPage(n){
  curPage=n;
  // 进入/离开设置页都清掉,SAVE 也会清,等同于"重新打开"
  clearTouched();
  // 3 秒内忽略服务器页面同步,避免并行的旧 /getdata 响应把页面拉回原状态
  ignorePgSyncUntil=Date.now()+3000;
  document.querySelectorAll('.tab').forEach(function(t,i){t.className=i===n?'tab on':'tab'});
  document.querySelectorAll('.pg').forEach(function(p,i){p.className=i===n?'pg on':'pg'});
  // 网页切页 → 通知 ESP32 → 转发命令给 STM32
  var x=new XMLHttpRequest();
  if(n===0) x.open('GET','/cmd?type=back',true);
  else if(n===1) x.open('GET','/cmd?type=settings',true);
  x.send();
}
function $(id){return document.getElementById(id)}
function pi(v){return parseInt(v)||0}

// fetch data
setInterval(function(){
  var x=new XMLHttpRequest();
  x.onload=function(){
    try{var d=JSON.parse(this.responseText)}catch(e){return}
    // TFT 页面切换同步
    if(d.pg!==undefined&&d.pg!==curPage&&Date.now()>ignorePgSyncUntil){
      // 屏幕端切了页 → 清掉本地 touched 标志,重新跟随
      clearTouched();
      curPage=d.pg;
      document.querySelectorAll('.tab').forEach(function(t,i){t.className=i===d.pg?'tab on':'tab'});
      document.querySelectorAll('.pg').forEach(function(p,i){p.className=i===d.pg?'pg on':'pg'});
    }
    // page0
    $('nMaf').innerText=d.maf+' '+d.unit;
    var pct=0;
    if(p1f>p1s){if(d.maf>=p1f)pct=100;else if(d.maf>p1s)pct=Math.round((d.maf-p1s)*100/(p1f-p1s))}
    $('bMaf').style.width=pct+'%';
    // Pump bars
    $('dP1').className='dt '+(d.p1e?'on':'off');
    $('vP1').innerText=d.p1d+'%';
    $('bP1').style.width=d.p1d+'%';
    $('dP2').className='dt '+(d.p2e?'on':'off');
    $('vP2').innerText=d.p2d+'%';
    $('bP2').style.width=d.p2d+'%';
    // Ethanol
    var ethVal=d.eth||0;
    if(d.fok) $('vEth').innerText=(ethVal/10).toFixed(1)+'%';
    else $('vEth').innerText='-';
    // Temperature
    var t=d.tmp||0;
    if(d.fok) $('vTmp').innerText=(t/10).toFixed(1)+'°'+(d.stu?'F':'C');
    else $('vTmp').innerText='-';
    // Status
    exAuto=(d.exm==='AT');
    $('vExM').innerText=exAuto?'AT':'MT';
    $('vExM').style.color='#ff9800';
    $('dEx').className='dt '+(d.exv?'on':'off');
    $('vExS').innerText=d.exv?'OPEN':'CLOSED';
    $('vExS').style.color=d.exv?'#4caf50':'#f44336';
    if(exAuto) $('btnEx').classList.add('disabled');
    else $('btnEx').classList.remove('disabled');
    $('vSpE').innerText=d.spe?'ON':'OFF';
    $('vSpE').style.color=d.spe?'#4caf50':'#666';
    $('dSp').className='dt '+(d.sp?'on':'off');
    $('vSpS').innerText=d.sp?'ON':'OFF';
    $('vSpS').style.color=d.sp?'#4caf50':'#666';
    $('vP1S').innerText=d.p1s+' '+d.unit;
    $('vP1F').innerText=d.p1f+' '+d.unit;
    $('vP2S').innerText=d.p2s+' '+d.unit;
    $('vP2F').innerText=d.p2f+' '+d.unit;
    p1s=d.p1s;p1f=d.p1f;
    $('vSpT').innerText=d.spon+'/'+d.spoff+' s';
    $('vExT').innerText=d.sexs+' '+d.unit;
    $('vExD').innerText=d.sexd+' ms';
    $('vPD').innerText=d.spd+'%';
    // warning — 单横条轮切显示
    var warns=[];
    if(d.wf)warns.push({t:'Low Fluid Level',c:'#ff9800'});
    if(d.wh)warns.push({t:'Battery Over Voltage',c:'#f44336'});
    if(d.wl)warns.push({t:'Battery Under Voltage',c:'#ffeb3b'});
    var eWB=$('warnBar');
    if(eWB){
      if(warns.length>0){
        eWB.style.display='block';
        if(warns.length===1||eWB.dataset.wl!=String(warns.length)){
          eWB.dataset.wl=warns.length;eWB.dataset.wi='0';
        }
        var ci=parseInt(eWB.dataset.wi)||0;
        eWB.innerText=warns[ci].t;eWB.style.color=warns[ci].c;eWB.style.borderColor=warns[ci].c;
        var ni=(ci+1)%warns.length;eWB.dataset.wi=String(ni);
      }else{eWB.style.display='none';}
    }
    // test mode
    var eT=$('testBanner');if(eT)eT.style.display=d.tm?'block':'none';
    // setting/factory 字段:用户没动过的持续从 STM32 同步,动过的冻结直到 SAVE/BACK
    function sv(id,v){
      if(userTouched[id]) return;
      var e=$(id); if(!e) return;
      if(e.type==='checkbox') e.checked=!!v;
      else if(e.tagName==='SELECT') e.value=''+v;
      else if(document.activeElement!==e) e.value=v;
    }
    sv('iPE1',d.sp1e?1:0);sv('iS1',d.ssta1);sv('iF1',d.sful1);
    sv('iPE2',d.sp2e?1:0);sv('iS2',d.ssta2);sv('iF2',d.sful2);
    sv('iSPM',d.sspm?1:0);sv('iSPO',d.sspon);sv('iSPF',d.sspof);
    sv('iEXA',d.sexa?1:0);sv('iEXS',d.sexs);sv('iEXD',d.sexd);
    sv('iMT',d.smt);sv('iPD',d.spd);
    sv('iFX0',d.sf0);sv('iFX100',d.sf100);sv('iFM',d.sfm?1:0);
    sv('iER',d.srev?1:0);sv('iTU',d.stu);sv('iTV',d.stv);sv('iTE',d.ste?1:0);
  };
  x.open('GET','/getdata',true);x.send();
},200);

function gv(id){var e=$(id);if(e.type==='checkbox')return e.checked?1:0;return parseInt(e.value)||0}
function cmd(t){var x=new XMLHttpRequest();x.open('GET','/cmd?type='+t,true);x.send()}
function cmdEx(){
  if(!exAuto) cmd('ex');   // 只有 MT 模式下才能控制阀门, AT 模式下点击无反应
}
function saveSetting(){
  var p='type=save';
  p+='&p1e='+gv('iPE1')+'&s1='+gv('iS1')+'&f1='+gv('iF1');
  p+='&p2e='+gv('iPE2')+'&s2='+gv('iS2')+'&f2='+gv('iF2');
  p+='&spm='+gv('iSPM')+'&spon='+gv('iSPO')+'&spof='+gv('iSPF');
  p+='&exa='+gv('iEXA')+'&exs='+gv('iEXS')+'&exd='+gv('iEXD');
  p+='&spd='+gv('iPD')+'&mt='+gv('iMT');
  p+='&f0='+gv('iFX0')+'&f100='+gv('iFX100')+'&fm='+gv('iFM');
  p+='&rev='+gv('iER')+'&tu='+gv('iTU')+'&tv='+gv('iTV')+'&te='+gv('iTE');
  var x=new XMLHttpRequest();
  x.open('POST','/cmd',true);
  x.setRequestHeader('Content-Type','application/x-www-form-urlencoded');
  x.send(p);
  clearTouched();
  alert('Setting saved!');
}
function stopRec(){
  var x=new XMLHttpRequest();
  x.onload=function(){
    var b=new Blob([this.responseText],{type:'text/csv'}),u=URL.createObjectURL(b),a=document.createElement('a');
    a.href=u;a.download='acu_log.csv';document.body.appendChild(a);a.click();document.body.removeChild(a);
    $('btnLogStart').classList.remove('disabled');
    $('btnLogStop').classList.add('disabled');
  };
  x.open('GET','/stopRecord',true);x.send();
}
function startRec(){
  cmd('logstart');
  $('btnLogStart').classList.add('disabled');
  $('btnLogStop').classList.remove('disabled');
}
// 用户改过的字段做标记,polling 就不会覆盖 — 等同于"编辑结构体"独立于 show 结构体
['iPE1','iS1','iF1','iPE2','iS2','iF2','iSPM','iSPO','iSPF',
 'iEXA','iEXS','iEXD','iMT','iPD','iFX0','iFX100','iFM',
 'iER','iTU','iTV','iTE'].forEach(function(id){
  var e=document.getElementById(id);
  if(!e) return;
  e.addEventListener('input',function(){userTouched[id]=true;});
  e.addEventListener('change',function(){userTouched[id]=true;});
});
</script>
</body></html>
)rawliteral";

/* ========================================================================
 * HTTP 处理
 * ======================================================================== */
void handleRoot() { server.send(200, "text/html", HTML_MAIN); }

void handleGetData() {
  String j = "{";
  j += "\"maf\":" + String(g_maf) + ",\"unit\":\"" + g_unit + "\",";
  j += "\"p1d\":" + String(g_p1_duty) + ",\"p2d\":" + String(g_p2_duty) + ",";
  j += "\"p1e\":" + String(g_p1_en) + ",\"p2e\":" + String(g_p2_en) + ",";
  j += "\"exm\":\"" + String(g_ex_mode) + "\",\"exv\":" + String(g_ex_valve) + ",";
  j += "\"sp\":" + String(g_spray) + ",\"spe\":" + String(g_spray_en) + ",";
  j += "\"p1s\":" + String(g_p1_start) + ",\"p1f\":" + String(g_p1_full) + ",";
  j += "\"p2s\":" + String(g_p2_start) + ",\"p2f\":" + String(g_p2_full) + ",";
  j += "\"spon\":" + String(g_sp_on) + ",\"spoff\":" + String(g_sp_off) + ",";
  j += "\"eth\":" + String(g_ethanol) + ",\"tmp\":" + String(g_temp_x10) + ",\"fok\":" + String(g_flex_ok) + ",";
  // settings
  j += "\"sp1e\":" + String(g_s_p1en) + ",\"ssta1\":" + String(g_s_sta1) + ",\"sful1\":" + String(g_s_full1) + ",";
  j += "\"sp2e\":" + String(g_s_p2en) + ",\"ssta2\":" + String(g_s_sta2) + ",\"sful2\":" + String(g_s_full2) + ",";
  j += "\"sspm\":" + String(g_s_spm) + ",\"sspon\":" + String(g_s_spon) + ",\"sspof\":" + String(g_s_spoff) + ",";
  j += "\"sexa\":" + String(g_s_exauto) + ",\"sexs\":" + String(g_s_exset) + ",\"sexd\":" + String(g_s_exdelay) + ",";
  j += "\"spd\":" + String(g_s_duty) + ",\"smt\":" + String(g_s_mtype) + ",";
  j += "\"sls\":" + String(g_s_lsen) + ",\"sbr\":" + String(g_s_bright) + ",";
  j += "\"sf0\":" + String(g_s_flex0) + ",\"sf100\":" + String(g_s_flex100) + ",";
  j += "\"sfm\":" + String(g_s_fluid) + ",\"srev\":" + String(g_s_exrev) + ",";
  j += "\"stu\":" + String(g_s_tempu) + ",\"stv\":" + String(g_s_testv) + ",\"ste\":" + String(g_s_testen);
  j += ",\"pg\":" + String(g_page);
  j += ",\"wf\":" + String(g_warn_fluid) + ",\"wh\":" + String(g_warn_hi) + ",\"wl\":" + String(g_warn_low);
  j += ",\"tm\":" + String(g_test_mode);
  j += "}";
  server.send(200, "application/json", j);
}

void handleCmd() {
  String type = server.arg("type");

  if (type == "ex") {
    uint8_t v = g_ex_valve ? 0 : 1;
    sendFrame(0xF60F, &v, 1);
  }
  else if (type == "exmode") {
    uint8_t v = g_ex_auto ? 0 : 1;   // AT->MT, MT->AT
    sendFrame(0xFC15, &v, 1);
  }
  else if (type == "spray") {
    uint8_t v = g_spray ? 0 : 1;
    sendFrame(0xF710, &v, 1);
  }
  else if (type == "save") {
    // update globals from query params
    g_s_p1en   = server.arg("p1e").toInt();
    g_s_sta1   = server.arg("s1").toInt();
    g_s_full1  = server.arg("f1").toInt();
    g_s_p2en   = server.arg("p2e").toInt();
    g_s_sta2   = server.arg("s2").toInt();
    g_s_full2  = server.arg("f2").toInt();
    g_s_spm    = server.arg("spm").toInt();
    g_s_spon   = server.arg("spon").toInt();
    g_s_spoff  = server.arg("spof").toInt();
    g_s_exauto = server.arg("exa").toInt();
    g_s_exset  = server.arg("exs").toInt();
    g_s_exdelay= server.arg("exd").toInt();
    g_s_duty   = server.arg("spd").toInt();
    g_s_mtype  = server.arg("mt").toInt();
    g_s_flex0  = server.arg("f0").toInt();
    g_s_flex100= server.arg("f100").toInt();
    g_s_fluid  = server.arg("fm").toInt();
    g_s_exrev  = server.arg("rev").toInt();
    g_s_tempu  = server.arg("tu").toInt();
    g_s_testv  = server.arg("tv").toInt();
    g_s_testen = server.arg("te").toInt();
    // Send TEST_CMD first so STM32 sets test_en before SAVE_CMD reads it
    uint8_t tp[3] = {(uint8_t)(g_s_testv & 0xFF), (uint8_t)(g_s_testv >> 8), (uint8_t)g_s_testen};
    sendFrame(0xF30C, tp, 3);
    delay(50);
    sendSaveCmd();
  }
  else if (type == "logstart") {
    recordHead = 0;
    recordCount = 0;
    isLogging = true;
    startTime = millis();
  }
  else if (type == "logstop")  { isLogging = false; }
  else if (type == "back")     { sendFrameCmdOnly(0xF20B); g_page = 0; }
  else if (type == "settings") { sendFrameCmdOnly(0xF40D); g_page = 1; }

  server.send(200, "text/plain", "OK");
}

void handleStartRecord() {
  recordHead = 0;
  recordCount = 0;
  isLogging = true;
  startTime = millis();
  server.send(200, "text/plain", "OK");
}

void handleStopRecord() {
  isLogging = false;
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/csv", "Time(ms),MAF,PumpA(%),PumpB(%),ExValve\n");
  int start = (recordHead - recordCount + MAX_RECORDS) % MAX_RECORDS;
  for (int i = 0; i < recordCount; i++)
    server.sendContent(logData[(start + i) % MAX_RECORDS]);
  server.sendContent("");  // 结束 chunked 响应
}

/* ========================================================================
 * Setup & Loop
 * ======================================================================== */
void setup() {
  STM32_SER.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  server.on("/",             handleRoot);
  server.on("/getdata",      handleGetData);
  server.on("/cmd",          handleCmd);
  server.on("/startRecord",  handleStartRecord);
  server.on("/stopRecord",   handleStopRecord);
  server.begin();
}

void loop() {
  updateSerial();
  server.handleClient();

  static unsigned long lastRec = 0;
  if (isLogging && millis() - lastRec >= 100) {
    lastRec = millis();
    snprintf(logData[recordHead], LOG_ENTRY_SIZE, "%lu,%d,%d,%d,%d\n",
      millis(), g_maf, g_p1_duty, g_p2_duty, g_ex_valve);
    recordHead = (recordHead + 1) % MAX_RECORDS;
    if (recordCount < MAX_RECORDS) recordCount++;
  }
}
