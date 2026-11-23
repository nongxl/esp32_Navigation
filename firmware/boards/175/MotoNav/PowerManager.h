#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "IdlePowerOff.h"

// 顶部小圆点：绿 = 蓝牙已连接，灰 = 未连接
// ================= v6.8 电池（AXP2101 电源芯片，和触摸屏共用 I2C，地址 0x34）=================
#define PMU_ADDR 0x34
bool pmuOK = false;
int batPct = -1;                       // -1 = 没接电池
bool batChg = false, usbIn = false;
int batMv = 0;
unsigned long batAt = 0, batSentAt = 0;
String pendingBat = "";

int pmuRead(uint8_t reg) {
  Wire.beginTransmission(PMU_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return -1;
  if (Wire.requestFrom((uint8_t)PMU_ADDR, (uint8_t)1) != 1) return -1;
  return Wire.read();
}
void pmuSetBit(uint8_t reg, uint8_t bit) {
  int v = pmuRead(reg);
  if (v < 0 || (v & (1 << bit))) return;
  Wire.beginTransmission(PMU_ADDR);
  Wire.write(reg);
  Wire.write((uint8_t)(v | (1 << bit)));
  Wire.endTransmission();
}

// 开机时调一次：只打开电量计、电池检测、电池电压测量，不动任何供电设置
void batInit() {
  // StopWatch：电源管理是 M5PM1（0x6E），已在 StopWatchBoard::init() 里完成初始化
  //（含关看门狗、使能充电、CHG_PROG/CHG_STAT 脚配置）。这里做一次首读确认电池在位。
  pmuOK = StopWatchBoard::pm1OK;
  if (!pmuOK) return;
  uint16_t mv = 0;
  if (StopWatchBoard::pm1.readVbat(&mv) != M5PM1_OK || mv < 2800) pmuOK = false;
}

// StopWatch: M5PM1 Vin > 4000mV indicates USB external power.
// Unknown/stale power readings keep the device awake, rather than guessing battery.
void idlePowerPollSupply(bool force = false) {
  const uint32_t now = millis();
  if (!force && idlePowerSupply.sampled &&
      (uint32_t)(now - idlePowerSupply.checkedAt) < 500) return;
  const bool wasBattery = idlePowerSupplyAllowsOff(&idlePowerSupply, now);
  uint16_t vin = 0;
  // If Vin read fails or pmu is not OK, status is -1 (unknown) so we stay awake safely.
  int status = -1;
  if (pmuOK && StopWatchBoard::pm1.readVin(&vin) == M5PM1_OK) {
    status = (vin > 4000) ? 0x20 : 0x00; // Bit 5 represents VBUS/external power
  }
  idlePowerSupplyUpdate(&idlePowerSupply, now, status);
  // Unplugging / recovering a valid battery-only reading starts a full interval.
  if (!wasBattery && idlePowerSupplyAllowsOff(&idlePowerSupply, now))
    idlePowerOffReset(&idlePowerOffTimer, now);
}

// 电池图标与内嵌电量：主体 52x24，右侧凸头 4x10，电量数字 setTextSize(2) 居中嵌在电池内部（更清晰饱满）
void drawBattery(int x0, int y0, bool backing = false) {
  if (batPct < 0) return;                                  // 没接电池不画
  const int BW = 52, BH = 24, capW = 4, capH = 10;
  const int totalW = BW + capW;

  if (backing) {                                           // 小地图上垫一块黑底，贴合电池主体与凸头，右侧不超限
    gfx->fillRoundRect(x0 - 2, y0 - 2, BW + 4, BH + 4, 5, C_BLACK);
    gfx->fillRoundRect(x0 + BW, y0 + (BH - capH) / 2 - 1, capW + 1, capH + 2, 2, C_BLACK);
  }

  // 罪恶都市柔和绿色系：正常电量使用罪恶都市公园绿 0x760D，充电使用青绿水面色 0x4E3B，低电量柔和暖橙 0xFD80，告别刺眼强光
  const uint16_t C_VICE_PARK_GREEN = 0x760D;
  const uint16_t C_VICE_WATER_AQUA = 0x4E3B;
  uint16_t c = batChg ? C_VICE_WATER_AQUA : batPct > 50 ? C_VICE_PARK_GREEN : batPct >= 20 ? 0xFD80 : 0xF9C6;
  if (!batChg && batPct < 10 && (millis() / 600) % 2) c = 0x4208;   // 快没电：闪烁

  // 1. 电池主体外框与正极凸头
  gfx->drawRoundRect(x0, y0, BW, BH, 4, c);
  gfx->drawRoundRect(x0 + 1, y0 + 1, BW - 2, BH - 2, 3, c);
  gfx->fillRoundRect(x0 + BW, y0 + (BH - capH) / 2, capW + 1, capH, 2, c);

  // 2. 内部底色（纯黑）
  gfx->fillRect(x0 + 2, y0 + 2, BW - 4, BH - 4, C_BLACK);

  // 3. 电量条填充
  int innerW = BW - 4; // 48
  int innerH = BH - 4; // 20
  int fw = innerW * constrain(batPct, 0, 100) / 100;
  if (fw > 0) {
    gfx->fillRect(x0 + 2, y0 + 2, fw, innerH, c);
  }

  // 4. 内嵌电量文字及充电标识（极简扁平抗锯齿字体）
  String pctStr = String(constrain(batPct, 0, 100));
  int fillEdge = x0 + 2 + fw;
  const SmoothFont &bf = SF16;
  int textW = smoothMeasureWidth(bf, pctStr.c_str(), false);
  int midY = y0 + BH / 2;

  auto drawBatDigits = [&](int startX) {
    int penX = startX;
    for (int i = 0; i < (int)pctStr.length(); i++) {
      char chStr[2] = {pctStr[i], '\0'};
      int cw = smoothMeasureWidth(bf, chStr, false);
      bool inFill = (penX + cw / 2 <= fillEdge);
      if (!inFill && penX < fillEdge + 6) {
        smoothDrawTextOutlined(bf, chStr, penX, midY, C_WHITE, C_BLACK, 0, 1, 1);
      } else {
        smoothDrawText(bf, chStr, penX, midY, inFill ? C_BLACK : C_WHITE, 0, 1, false);
      }
      penX += cw;
    }
  };

  if (batChg) {
    // 充电中：小闪电图标 + 数字
    int totalContentW = 9 + textW;
    int startX = x0 + 2 + (innerW - totalContentW) / 2;
    int boltX = startX + 3;
    int boltY = y0 + BH / 2;

    gfx->fillTriangle(boltX + 1, boltY - 6, boltX - 4, boltY, boltX, boltY, C_BLACK);
    gfx->fillTriangle(boltX - 1, boltY + 6, boltX + 4, boltY, boltX, boltY, C_BLACK);
    gfx->drawLine(boltX - 1, boltY - 3, boltX - 2, boltY + 3, C_WHITE);

    drawBatDigits(startX + 10);
  } else {
    // 未充电：电量数字完全居中
    int tx = x0 + 2 + (innerW - textW) / 2;
    drawBatDigits(tx);
  }
}

void drawStatus() {
  // 无地图导航界面已移除顶部电量图标，留出空间放大显示时速
}

// ===== 待机自适应动态调频（DFS 节能控制）=====
extern bool callOn;
extern bool pairOn;
extern volatile bool pairReq;
extern bool fxOn;
extern bool mapZoomActive;
extern unsigned long lastRxMs;
static uint32_t lastHighPerfRequiredAt = 0;

void updateCpuPerformance(bool inNavState, bool cruiseState, bool touchActive, bool slidingActive, int curPage) {
  const uint32_t now = millis();
  
  // 必须保持 240MHz 全性能的场景：
  // 1. 正在导航或巡航中 (inNav || cruiseOn)
  // 2. 正在小地图缩放操作 (mapZoomActive)
  // 3. 用户正在交互（触控中、滑动切页中、拖拽中）
  // 4. 来电通知、配对画面、到达特效进行中
  // 5. 处于倾角仪界面 (page == 3，需要高频陀螺仪与物理计算)
  // 6. 处于伴侣表情界面 (page == 4，需要 30FPS 动画)
  // 7. 最近 2.5 秒内收到过新的 BLE 数据包
  bool needHighPerf = inNavState || cruiseState || touchActive || slidingActive ||
                      callOn || pairOn || pairReq || fxOn || (curPage == 3) || (curPage == 4) ||
                      mapZoomActive;

  if (!needHighPerf && (uint32_t)(now - lastRxMs) < 2500) {
    needHighPerf = true;
  }

  if (needHighPerf) {
    lastHighPerfRequiredAt = now;
    if (getCpuFrequencyMhz() < 240) {
      setCpuFrequencyMhz(240);
    }
  } else {
    // 待机状态：无操作超过 2 秒，降频至 80MHz 节能运行（CPU 功耗直降 65%）
    if ((uint32_t)(now - lastHighPerfRequiredAt) >= 2000) {
      if (getCpuFrequencyMhz() > 80) {
        setCpuFrequencyMhz(80);
      }
    }
  }
}


