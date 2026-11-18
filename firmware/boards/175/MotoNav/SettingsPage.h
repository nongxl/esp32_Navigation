#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "UiModern.h"
#include "LeanAngleMeter.h"

// 电量与手机状态全局引用
extern int batPct;
extern bool batChg;
extern int8_t phoneBatPct;
extern bool   phoneBatChg;
extern int8_t phoneSig1;
extern int8_t phoneSig2;
extern volatile bool bleConnected;
extern volatile bool bleAuthed;

// 绘制圆屏立式电池图标 (正常电池样式，内嵌电量数字与充电闪电)
static void drawVerticalWatchBattery(int cx, int cy, int pct, bool chg) {
  if (pct < 0) return;
  const int BW = 30, BH = 44, capW = 10, capH = 3;
  int x0 = cx - BW / 2;
  int y0 = cy - BH / 2 + 2;

  const uint16_t C_VICE_PARK_GREEN = 0x760D;
  const uint16_t C_VICE_WATER_AQUA = 0x4E3B;
  uint16_t c = chg ? C_VICE_WATER_AQUA : pct > 50 ? C_VICE_PARK_GREEN : pct >= 20 ? 0xFD80 : 0xF9C6;
  if (!chg && pct < 10 && (millis() / 600) % 2) c = 0x4208;

  // 正极凸起 (顶部中央)
  gfx->fillRoundRect(cx - capW / 2, y0 - capH, capW, capH + 1, 2, c);

  // 电池外框与纯黑底
  gfx->drawRoundRect(x0, y0, BW, BH, 4, c);
  gfx->drawRoundRect(x0 + 1, y0 + 1, BW - 2, BH - 2, 3, c);
  gfx->fillRect(x0 + 2, y0 + 2, BW - 4, BH - 4, C_BLACK);

  // 垂直由下往上填充电量柱
  int innerW = BW - 4;
  int innerH = BH - 4;
  int fh = innerH * constrain(pct, 0, 100) / 100;
  if (fh > 0) {
    gfx->fillRect(x0 + 2, y0 + 2 + innerH - fh, innerW, fh, c);
  }

  // 充电闪电或内嵌居中电量数字
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", constrain(pct, 0, 100));
  if (chg) {
    int boltX = cx, boltY = y0 + 10;
    gfx->fillTriangle(boltX + 1, boltY - 4, boltX - 3, boltY, boltX, boltY, C_WHITE);
    gfx->fillTriangle(boltX - 1, boltY + 4, boltX + 3, boltY, boltX, boltY, C_WHITE);
    drawAaString((float)cx, (float)(y0 + 24), buf, 1.35f, 0.8f, 8.5f, C_WHITE, true);
  } else {
    drawAaString((float)cx, (float)(cy + 2), buf, 1.45f, 0.85f, 9.5f, C_WHITE, true);
  }

  // 下方微标标签
  drawAaString((float)cx, (float)(y0 + BH + 9), "SCREEN", 1.15f, 0.7f, 7.5f, 0x7BEF, true);
}

// 绘制非智能机时代有天线的经典复古手机外形电池图标 (突出天线、微型听筒、液晶屏幕电量、实体按键区)
static void drawVerticalPhoneBattery(int cx, int cy, int pct, bool chg) {
  if (pct < 0) return;
  const int PW = 30, PH = 44;
  int x0 = cx - PW / 2;
  int y0 = cy - PH / 2 + 2; // y0 = cy - 20, 底部在 y0 + PH = cy + 24，与圆屏电池底部平齐

  const uint16_t C_VICE_PARK_GREEN = 0x760D;
  const uint16_t C_VICE_WATER_AQUA = 0x4E3B;
  uint16_t c = chg ? C_VICE_WATER_AQUA : pct > 50 ? C_VICE_PARK_GREEN : pct >= 20 ? 0xFD80 : 0xF9C6;
  if (!chg && pct < 10 && (millis() / 600) % 2) c = 0x4208;

  // 1. 经典复古手机突出式外部天线 (位于机身左上方)
  // 天线柱 (宽 3，高 9，从 y0 - 8 到 y0 + 1)
  gfx->fillRoundRect(x0 + 4, y0 - 8, 3, 9, 1, 0x64BD);
  // 天线顶端圆角护帽 (宽 5，高 3，极具老天线特质)
  gfx->fillRoundRect(x0 + 3, y0 - 9, 5, 3, 1, 0x9CD3);

  // 2. 老手机机身外壳轮廓 (圆角机身，经典硬朗科技感)
  gfx->drawRoundRect(x0, y0, PW, PH, 4, 0x5AEB);
  gfx->drawRoundRect(x0 + 1, y0 + 1, PW - 2, PH - 2, 3, 0x2124);
  gfx->fillRect(x0 + 2, y0 + 2, PW - 4, PH - 4, C_BLACK);

  // 3. 顶部微型听筒槽
  gfx->fillRoundRect(cx - 3, y0 + 3, 6, 2, 1, 0x5AEB);

  // 4. 中部方形液晶屏幕显示区 (内嵌电量柱和百分比)
  int sw = PW - 6; // 24
  int sh = 23;     // 高度 23
  int sx = cx - sw / 2;
  int sy = y0 + 7;
  gfx->drawRect(sx - 1, sy - 1, sw + 2, sh + 2, 0x31A6);

  int fh = sh * constrain(pct, 0, 100) / 100;
  if (fh > 0) {
    gfx->fillRect(sx, sy + sh - fh, sw, fh, c);
  }

  // 充电闪电或内嵌居中电量数字
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", constrain(pct, 0, 100));
  if (chg) {
    int boltX = cx, boltY = sy + 6;
    gfx->fillTriangle(boltX + 1, boltY - 4, boltX - 3, boltY, boltX, boltY, C_WHITE);
    gfx->fillTriangle(boltX - 1, boltY + 4, boltX + 3, boltY, boltX, boltY, C_WHITE);
    drawAaString((float)cx, (float)(sy + 17), buf, 1.30f, 0.75f, 8.0f, C_WHITE, true);
  } else {
    drawAaString((float)cx, (float)(sy + sh / 2 + 1), buf, 1.40f, 0.85f, 9.0f, C_WHITE, true);
  }

  // 5. 机身下方经典实体按键区 (功能键与数字键盘微孔横向点阵)
  int ky = sy + sh + 3; // y0 + 33
  // 经典导航中键与左右功能键
  gfx->drawRoundRect(cx - 3, ky, 6, 3, 1, 0x5AEB);
  gfx->drawLine(cx - 9, ky + 1, cx - 6, ky + 1, 0x4A69);
  gfx->drawLine(cx + 6, ky + 1, cx + 9, ky + 1, 0x4A69);
  // 下方键盘微孔点
  gfx->fillRect(cx - 8, ky + 5, 2, 2, 0x39E7);
  gfx->fillRect(cx - 1, ky + 5, 2, 2, 0x39E7);
  gfx->fillRect(cx + 6, ky + 5, 2, 2, 0x39E7);

  // 6. 下方微标标签 (与圆屏电池 SCREEN 对齐在 y0 + PH + 9 = cy + 33)
  drawAaString((float)cx, (float)(y0 + PH + 9), "PHONE", 1.15f, 0.7f, 7.5f, 0x7BEF, true);
}

// 绘制手机信号图标 (支持单卡 4 阶信号柱，或双卡双信号柱并排)
static void drawPhoneSignalIcon(int cx, int cy, int sig1, int sig2) {
  if (sig1 < 0 && sig2 < 0) return;

  const int baseY = cy + 24; // 与电池底部 (cy + 24) 严密对齐

  if (sig2 < 0) {
    // --- 单卡模式 (4 级经典递增梯级蜂窝柱) ---
    const int barW = 3, gap = 2;
    const int heights[4] = {6, 12, 18, 24};
    int totalW = 4 * barW + 3 * gap; // 18px
    int startX = cx - totalW / 2;

    for (int i = 0; i < 4; i++) {
      int h = heights[i];
      int bx = startX + i * (barW + gap);
      int by = baseY - h;
      uint16_t col = (sig1 > i) ? C_WHITE : 0x2124;
      gfx->fillRoundRect(bx, by, barW, h, 1, col);
    }
    drawAaString((float)cx, (float)(cy + 33), "CELL", 1.15f, 0.7f, 7.5f, 0x7BEF, true);
  } else {
    // --- 双卡模式 (两组紧凑信号柱，分别标 1 和 2) ---
    const int barW = 2, gap = 1;
    const int heights[4] = {5, 9, 14, 19};
    int simW = 4 * barW + 3 * gap; // 11px

    // 卡 1 (左侧)
    int startX1 = cx - simW - 3;
    for (int i = 0; i < 4; i++) {
      int h = heights[i];
      int bx = startX1 + i * (barW + gap);
      uint16_t col = (sig1 > i) ? C_WHITE : 0x2124;
      gfx->fillRoundRect(bx, baseY - h, barW, h, 1, col);
    }
    drawAaString((float)(startX1 + simW / 2), (float)(baseY - 22), "1", 0.95f, 0.6f, 6.5f, 0x760D, true);

    // 卡 2 (右侧)
    int startX2 = cx + 3;
    for (int i = 0; i < 4; i++) {
      int h = heights[i];
      int bx = startX2 + i * (barW + gap);
      uint16_t col = (sig2 > i) ? C_WHITE : 0x2124;
      gfx->fillRoundRect(bx, baseY - h, barW, h, 1, col);
    }
    drawAaString((float)(startX2 + simW / 2), (float)(baseY - 22), "2", 0.95f, 0.6f, 6.5f, 0x760D, true);

    drawAaString((float)cx, (float)(cy + 33), "DUAL", 1.15f, 0.7f, 7.5f, 0x7BEF, true);
  }
}

// 设置项全局状态
extern int qspiMHz;
extern int screenRot;
extern void setScreenDeg(int deg, bool save);
extern void setScreenDegFloat(float deg, bool save);

// 1. 亮度设置 (25%, 50%, 75%, 100%)
static const uint8_t brightnessLevels[] = {64, 128, 192, 255};
static const char* const brightnessLabels[] = {"25%", "50%", "75%", "100%"};
static int brightnessIdx = 2; // 默认 75% (192)

// 2. 地图主题样式（精简为与 App 严格一致的 4 款核心主题）
// 1: 罪恶都市(默认), 0: 原版, 5: 极简黑白, 11: 龙珠雷达
static const int supportedThemes[4] = {1, 0, 5, 11};
static const char* const supportedThemeLabelsEn[4] = {"VICE CITY", "CLASSIC", "MINIMAL", "DRAGON"};
static const char* const supportedThemeLabelsZh[4] = {"罪恶都市", "经典原版", "极简黑白", "龙珠雷达"};
extern int themeIdx;

// 3. 屏幕重力旋转随动状态
bool settingsAutoRot = false;
static float autoRotAngleSmoothed = 0.0f;
static unsigned long lastAutoRotCheck = 0;

// 4. 待机休眠时间 (3分钟, 5分钟, 10分钟, 常亮)
static const uint32_t sleepTimeouts[] = {180, 300, 600, 0};
static const char* const sleepLabelsEn[] = {"3 MIN", "5 MIN", "10 MIN", "NEVER"};
static const char* const sleepLabelsZh[] = {"3分钟", "5分钟", "10分钟", "常亮"};
static int sleepIdx = 1; // 默认 5 分钟

inline void syncBrightnessFromPhone(int b) {
  b = constrain(b, 5, 255);
  for (int i = 0; i < 4; i++) {
    if (abs((int)brightnessLevels[i] - b) < 32) { brightnessIdx = i; break; }
  }
  Preferences pr;
  if (pr.begin("motonav", false)) {
    pr.putInt("brightness", b);
    pr.end();
  }
  if (page == 1) settingsDirty = true;
}

inline void settingsInit() {
  Preferences pr;
  if (pr.begin("motonav", true)) {
    isChineseLang = pr.getBool("chinese", true);
    int b = pr.getInt("brightness", 192);
    for (int i = 0; i < 4; i++) {
      if (abs((int)brightnessLevels[i] - b) < 32) { brightnessIdx = i; break; }
    }
    int s = pr.getInt("autosleep", 300);
    for (int i = 0; i < 4; i++) {
      if (sleepTimeouts[i] == (uint32_t)s) { sleepIdx = i; break; }
    }
    pr.end();
  }
}

// 统一极简冷银白矢量图标 (简约高雅风格)
static void drawSettingsIcon(int id, int cx, int cy, uint16_t color = 0xDEFB) {
  switch (id) {
    case 0: // 太阳 (亮度)
      aaDisc(cx, cy, 4.5f, color);
      for (int a = 0; a < 360; a += 45) {
        float rad = a * (float)M_PI / 180.0f;
        gfx->drawLine((int)lroundf(cx + 7 * cosf(rad)), (int)lroundf(cy + 7 * sinf(rad)),
                      (int)lroundf(cx + 11 * cosf(rad)), (int)lroundf(cy + 11 * sinf(rad)), color);
      }
      break;

    case 1: // 地图图层 (地图风格)
      gfx->drawRoundRect(cx - 10, cy - 8, 20, 16, 2, color);
      gfx->drawLine(cx - 10, cy - 1, cx + 10, cy - 1, color);
      gfx->drawLine(cx - 3, cy - 8, cx - 3, cy + 8, color);
      aaDisc(cx + 4, cy + 3, 2.0f, color);
      break;

    case 2: // 旋转箭头 (屏幕角度/重力旋转)
      gfx->drawCircle(cx, cy, 9, color);
      gfx->drawCircle(cx, cy, 10, color);
      // 右上方开口与小箭头
      gfx->fillRect(cx + 3, cy - 11, 8, 8, C_BLACK);
      gfx->fillTriangle(cx + 2, cy - 11, cx + 10, cy - 11, cx + 6, cy - 5, color);
      break;

    case 3: // 月牙休眠 (待机时间)
      aaDisc(cx, cy, 8.0f, color);
      aaDisc(cx + 4.0f, cy - 3.0f, 7.0f, C_BLACK);
      break;
  }
}

// 获取当前选择的 4 款主题名称
static inline const char* getThemeDisplayLabel() {
  for (int i = 0; i < 4; i++) {
    if (supportedThemes[i] == themeIdx) return isChineseLang ? supportedThemeLabelsZh[i] : supportedThemeLabelsEn[i];
  }
  return isChineseLang ? "罪恶都市" : "VICE CITY";
}

// 绘制重力感应旋转校准界面（完整保留设置页背景与卡片全貌，实时呈现随动旋转效果，纯白基准十字线呈 90° 相交）
void drawRotationCalibPage() {
  onWaitScreen = false;
  frameIsMap = false;

  // 1. 绘制暗黑主题背景底图
  gfx->fillScreen(C_BLACK);
  drawThemedMapBackground();

  // 绘制 4 张设置卡片（让用户在转动时能实时看到设置界面的真实旋转效果）
  const int cardW = 164, cardH = 108, gapX = 14, gapY = 16;
  const int leftX  = CX - cardW - gapX / 2;
  const int rightX = CX + gapX / 2;
  const int topY   = 94, botY = topY + cardH + gapY;

  // 4 个卡片底座与边框
  drawTranslucentRoundRect(leftX,  topY, cardW, cardH, 16, 0x0000, 180);
  gfx->drawRoundRect(leftX,  topY, cardW, cardH, 16, 0x2965);
  drawTranslucentRoundRect(rightX, topY, cardW, cardH, 16, 0x0000, 180);
  gfx->drawRoundRect(rightX, topY, cardW, cardH, 16, 0x2965);
  drawTranslucentRoundRect(leftX,  botY, cardW, cardH, 16, 0x0000, 180);
  gfx->drawRoundRect(leftX,  botY, cardW, cardH, 16, 0x2965);
  drawTranslucentRoundRect(rightX, botY, cardW, cardH, 16, 0x0000, 180);
  gfx->drawRoundRect(rightX, botY, cardW, cardH, 16, 0x2965);

  // 绘制各卡片图标与文字
  drawSettingsIcon(0, leftX + cardW / 2, topY + 26, 0xDEFB);
  drawAaString(leftX + cardW / 2, topY + 54, "BRIGHTNESS", 1.3f, 0.8f, 9.0f, 0x8C71, true);

  drawSettingsIcon(1, rightX + cardW / 2, topY + 26, 0xDEFB);
  drawAaString(rightX + cardW / 2, topY + 54, "MAP STYLE", 1.3f, 0.8f, 9.0f, 0x8C71, true);

  drawSettingsIcon(2, leftX + cardW / 2, botY + 26, 0xDEFB);
  drawAaString(leftX + cardW / 2, botY + 54, "ROTATION", 1.3f, 0.8f, 9.0f, 0x8C71, true);

  drawSettingsIcon(3, rightX + cardW / 2, botY + 26, 0xDEFB);
  drawAaString(rightX + cardW / 2, botY + 54, "AUTO SLEEP", 1.3f, 0.8f, 9.0f, 0x8C71, true);

  // 2. 纯白基准十字/T字线（根据当前平滑重力角度计算旋转端点并动态绘制，水平地平线 + 重力铅垂线）
  float rad = autoRotAngleSmoothed * (float)M_PI / 180.0f;
  float cosA = cosf(rad), sinA = sinf(rad);

  // 水平地平基准线：贯穿屏幕中心
  float hx0 = (float)CX - (float)CX * cosA;
  float hy0 = (float)CX - (float)CX * sinA;
  float hx1 = (float)CX + (float)CX * cosA;
  float hy1 = (float)CX + (float)CX * sinA;
  float hnx = -sinA, hny = cosA;
  gfx->drawLine((int)lroundf(hx0 - hnx), (int)lroundf(hy0 - hny), (int)lroundf(hx1 - hnx), (int)lroundf(hy1 - hny), C_WHITE);
  gfx->drawLine((int)lroundf(hx0), (int)lroundf(hy0), (int)lroundf(hx1), (int)lroundf(hy1), C_WHITE);
  gfx->drawLine((int)lroundf(hx0 + hnx), (int)lroundf(hy0 + hny), (int)lroundf(hx1 + hnx), (int)lroundf(hy1 + hny), C_WHITE);

  // 垂直地面的铅垂线：从中心垂直指向重力下方
  float vx1 = (float)CX - (float)CX * sinA;
  float vy1 = (float)CX + (float)CX * cosA;
  float vnx = cosA, vny = sinA;
  gfx->drawLine((int)lroundf((float)CX - vnx), (int)lroundf((float)CX - vny), (int)lroundf(vx1 - vnx), (int)lroundf(vy1 - vny), C_WHITE);
  gfx->drawLine(CX, CX, (int)lroundf(vx1), (int)lroundf(vy1), C_WHITE);
  gfx->drawLine((int)lroundf((float)CX + vnx), (int)lroundf((float)CX + vny), (int)lroundf(vx1 + vnx), (int)lroundf(vy1 + vny), C_WHITE);

  // 垂线末端指示小圆点
  aaDisc(vx1, vy1, 3.5f, C_WHITE);

  // 中心相交点纯白精致圆点
  aaDisc((float)CX, (float)CX, 5.0f, C_WHITE);
  aaDisc((float)CX, (float)CX, 2.0f, C_BLACK);

  // 3. 旋转角度数字显示：在水平线中间上方（纯白色大号抗锯齿平滑字体，带高对比半透明黑色底衬）
  int curAngle = (int)lroundf(autoRotAngleSmoothed);
  curAngle = ((curAngle % 360) + 360) % 360;
  char degBuf[16];
  snprintf(degBuf, sizeof(degBuf), "%d*", curAngle);
  drawTranslucentRoundRect(CX - 50, CX - 54, 100, 38, 12, 0x0000, 210);
  gfx->drawRoundRect(CX - 50, CX - 54, 100, 38, 12, 0x4A69);
  drawAaString((float)CX, (float)(CX - 35), degBuf, 3.2f, 1.8f, 22.0f, C_WHITE, true);

  // 4. 顶部与底部极简提示
  // 顶部：AUTO ROTATE
  drawTranslucentRoundRect(CX - 72, 44, 144, 28, 14, 0x0000, 200);
  gfx->drawRoundRect(CX - 72, 44, 144, 28, 14, 0x39E7);
  if (isChineseLang) {
    drawTextUTF8Center(CX, 62, "重力随动旋转", C_WHITE);
  } else {
    drawAaString((float)CX, 58.0f, "AUTO ROTATE", 1.4f, 0.85f, 9.5f, C_WHITE, true);
  }

  // 底部：TAP ANYWHERE TO LOCK (点击任意位置锁定)
  drawTranslucentRoundRect(CX - 110, 396, 220, 30, 15, 0x0000, 200);
  gfx->drawRoundRect(CX - 110, 396, 220, 30, 15, 0x760D);
  if (isChineseLang) {
    drawTextUTF8Center(CX, 416, "轻触屏幕任意位置锁定", 0x760D);
  } else {
    drawAaString((float)CX, 411.0f, "TAP ANYWHERE TO LOCK", 1.3f, 0.8f, 9.0f, 0x760D, true);
  }

  needFlush = true;
}

// 重力感应屏幕旋转动态刷新（浮点级无阶梯顿挫，40 FPS 流体般自然旋转）
inline void settingsUpdateAutoRot() {
  if (!settingsAutoRot || page != 1 || !LeanAngleMeter::available) return;
  unsigned long now = millis();
  if (now - lastAutoRotCheck < 25) return; // 40 FPS 超高频响应
  lastAutoRotCheck = now;

  LeanAngleMeter::imu.getSensorData();
  // 硬件旋转映射：屏幕向右 = +accelY, 屏幕向上 = -accelX
  float ax = LeanAngleMeter::imu.data.accelY;
  float ay = -LeanAngleMeter::imu.data.accelX;
  float aMag = sqrtf(ax * ax + ay * ay);

  // 只要屏幕有适当倾角（非水平平放于桌面上），实时随重力转动
  if (aMag >= 0.15f) {
    float rad = atan2f(ax, ay);
    float deg = rad * (180.0f / (float)M_PI);
    if (deg < 0) deg += 360.0f;

    // 环形平滑插值滤波，消除传感器抖动
    float diff = deg - autoRotAngleSmoothed;
    while (diff > 180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;

    // 动态平滑跟手：大角度快速跟随，小角度柔和滤除抖动
    float alpha = fabsf(diff) > 25.0f ? 0.40f : 0.22f;
    autoRotAngleSmoothed += diff * alpha;
    while (autoRotAngleSmoothed >= 360.0f) autoRotAngleSmoothed -= 360.0f;
    while (autoRotAngleSmoothed < 0.0f) autoRotAngleSmoothed += 360.0f;

    drawRotationCalibPage();
  }
}

// 绘制设置页面 (466x466 AMOLED 圆屏极简现代风格)
void drawSettingsPage() {
  onWaitScreen = false;
  frameIsMap = false;

  // 若处于随动旋转模式，直接绘制流体旋转校准视图
  if (settingsAutoRot) {
    drawRotationCalibPage();
    return;
  }

  // 1. 绘制统一沉稳的暗黑磨砂背景
  gfx->fillScreen(C_BLACK);
  drawThemedMapBackground();

  // 2. 顶部极简毛玻璃药丸标题
  const int titleW = 150;
  const int titleH = 32;
  const int titleX = CX - titleW / 2;
  const int titleY = 36;
  drawTranslucentRoundRect(titleX, titleY, titleW, titleH, 16, 0x0000, 205);
  gfx->drawRoundRect(titleX, titleY, titleW, titleH, 16, 0x2965);
  if (isChineseLang) {
    drawTextUTF8Center(CX, titleY + 22, "设置", C_WHITE);
  } else {
    drawAaString(CX, titleY + 16, "SETTINGS", 1.8f, 1.0f, 12.0f, C_WHITE, true);
  }

  // 3. 4 个半透明毛玻璃设置卡片 (极简克制高级配色)
  const int cardW = 164;
  const int cardH = 108;
  const int gapX = 14;
  const int gapY = 16;
  const int leftX  = CX - cardW - gapX / 2; // 62
  const int rightX = CX + gapX / 2;          // 240
  const int topY   = 94;
  const int botY   = topY + cardH + gapY;   // 218

  // 格式化当前锁定的整数角度
  char rotBuf[16];
  snprintf(rotBuf, sizeof(rotBuf), "%d*", screenDeg);

  struct CardConfig {
    int x, y;
    int iconId;
    const char *titleZh;
    const char *titleEn;
    const char *val;
    bool valIsZh;
  } cards[4] = {
    {leftX,  topY, 0, "屏幕亮度", "BRIGHTNESS", brightnessLabels[brightnessIdx], false},
    {rightX, topY, 1, "地图风格", "MAP STYLE",  getThemeDisplayLabel(), isChineseLang},
    {leftX,  botY, 2, "屏幕旋转", "ROTATION",   rotBuf, false},
    {rightX, botY, 3, "自动休眠", "AUTO SLEEP", isChineseLang ? sleepLabelsZh[sleepIdx] : sleepLabelsEn[sleepIdx], isChineseLang}
  };

  for (int i = 0; i < 4; i++) {
    int cx = cards[i].x;
    int cy = cards[i].y;

    // 极简深黑毛玻璃底座
    drawTranslucentRoundRect(cx, cy, cardW, cardH, 16, 0x0000, 190);
    gfx->drawRoundRect(cx, cy, cardW, cardH, 16, 0x2965);

    // 图标 (居中偏上，冷银白)
    drawSettingsIcon(cards[i].iconId, cx + cardW / 2, cy + 26, 0xDEFB);

    // 标题文字
    if (isChineseLang) {
      drawTextUTF8Center(cx + cardW / 2, cy + 58, cards[i].titleZh, 0x8C71);
    } else {
      drawAaString(cx + cardW / 2, cy + 54, cards[i].titleEn, 1.3f, 0.8f, 9.0f, 0x8C71, true);
    }

    // 当前数值底座
    const int valPillW = cardW - 32;
    const int valPillH = 26;
    const int valPillX = cx + 16;
    const int valPillY = cy + 72;
    drawTranslucentRoundRect(valPillX, valPillY, valPillW, valPillH, 13, 0x0000, 160);
    gfx->drawRoundRect(valPillX, valPillY, valPillW, valPillH, 13, 0x31A6);

    if (cards[i].valIsZh) {
      drawTextUTF8Center(cx + cardW / 2, valPillY + 18, cards[i].val, C_WHITE);
    } else {
      drawAaString(cx + cardW / 2, valPillY + 13, cards[i].val, 1.5f, 0.9f, 10.0f, C_WHITE, true);
    }
  }

  // 4. 设置界面下方状态区：圆屏电池、手机形状电池、手机信号图标依次水平排列
  const int iconCY = 368;
  bool hasPhone = bleConnected && (phoneBatPct >= 0);
  bool hasSignal = (phoneSig1 >= 0 || phoneSig2 >= 0);

  if (hasPhone) {
    if (hasSignal) {
      // 顺序依次为：圆屏电池图标、手机电池图标、手机信号图标
      drawVerticalWatchBattery(CX - 84, iconCY, batPct, batChg);
      drawVerticalPhoneBattery(CX, iconCY, phoneBatPct, phoneBatChg);
      drawPhoneSignalIcon(CX + 84, iconCY, phoneSig1, phoneSig2);
    } else {
      drawVerticalWatchBattery(CX - 44, iconCY, batPct, batChg);
      drawVerticalPhoneBattery(CX + 44, iconCY, phoneBatPct, phoneBatChg);
    }
  } else {
    // 未连接蓝牙或无手机数据时：圆屏电池单独居中显示
    drawVerticalWatchBattery(CX, iconCY, batPct, batChg);
  }

  // 5. 底部页面指示点
  drawPageDots();
  needFlush = true;
}

// 设置页面点击处理 (接收已按屏幕旋转换算为画布坐标系的 x, y)
void settingsPageTap(int x, int y) {
  if (page != 1 || callOn || pairOn || sliding) return;

  // 在调整角度模式下：用户按屏幕任意位置都立即锁定角度，自然贴合到整数角度
  if (settingsAutoRot) {
    settingsAutoRot = false;
    int finalDeg = (int)lroundf(autoRotAngleSmoothed);
    finalDeg = ((finalDeg % 360) + 360) % 360;
    setScreenDeg(finalDeg, true);
    CompanionMode::startVibration(70, 30);
    drawSettingsPage();
    needFlush = true;
    return;
  }

  const int cardW = 164;
  const int cardH = 108;
  const int gapX = 14;
  const int gapY = 16;
  const int leftX  = CX - cardW - gapX / 2;
  const int rightX = CX + gapX / 2;
  const int topY   = 94;
  const int botY   = topY + cardH + gapY;

  int clickedCard = -1;
  if (x >= leftX && x <= leftX + cardW) {
    if (y >= topY && y <= topY + cardH) clickedCard = 0;       // 亮度
    else if (y >= botY && y <= botY + cardH) clickedCard = 2;  // 旋转
  } else if (x >= rightX && x <= rightX + cardW) {
    if (y >= topY && y <= topY + cardH) clickedCard = 1;       // 地图风格
    else if (y >= botY && y <= botY + cardH) clickedCard = 3;  // 休眠
  }

  if (clickedCard < 0) return;

  // 震动触感反馈
  CompanionMode::startVibration(50, 20);

  switch (clickedCard) {
    case 0: { // 亮度调节 (25% -> 50% -> 75% -> 100%)
      brightnessIdx = (brightnessIdx + 1) % 4;
      uint8_t b = brightnessLevels[brightnessIdx];
      panel->setBrightness(b);
      Preferences pr;
      if (pr.begin("motonav", false)) {
        pr.putInt("brightness", b);
        pr.end();
      }
      if (bleConnected) {
        pendingEvt = "T,SET_BRIGHT," + String(b);
      }
      break;
    }
    case 1: { // 地图风格切换（在 App 的 4 款核心主题中循环：罪恶都市 -> 原版 -> 极简黑白 -> 龙珠雷达）
      int curPos = 0;
      for (int i = 0; i < 4; i++) {
        if (supportedThemes[i] == themeIdx) { curPos = i; break; }
      }
      curPos = (curPos + 1) % 4;
      themeIdx = supportedThemes[curPos];

      Preferences pr;
      if (pr.begin("motonav", false)) {
        pr.putInt("maptheme", themeIdx);
        pr.end();
      }
      // 通知手机端同步地图风格
      if (bleConnected) {
        pendingEvt = "T,SET_THEME," + String(themeIdx);
      }
      break;
    }
    case 2: { // 点击进入重力旋转随动校准模式
      settingsAutoRot = true;
      autoRotAngleSmoothed = (float)screenDeg;
      lastAutoRotCheck = 0;
      setScreenDeg(0, false); // 校准模式下推屏保持基准正向，由 T 字参考线随动旋转指示重力
      drawRotationCalibPage();
      return;
    }
    case 3: { // 待机休眠
      sleepIdx = (sleepIdx + 1) % 4;
      uint32_t s = sleepTimeouts[sleepIdx];
      Preferences pr;
      if (pr.begin("motonav", false)) {
        pr.putInt("autosleep", s);
        pr.end();
      }
      break;
    }
  }

  drawSettingsPage();
}
