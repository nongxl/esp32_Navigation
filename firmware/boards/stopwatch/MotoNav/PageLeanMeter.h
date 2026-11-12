#pragma once
#include <Arduino.h>
#include "LeanAngleMeter.h"
#include "UiModern.h"

extern bool dragging;

// 车规级机械腕表哑光质感配色 (低眩光、专业沉稳、防反光)
#define LEAN_COLOR_SAFE   0x34EA  // 哑光薄荷青绿 (优雅清爽、安全倾角)
#define LEAN_COLOR_WARN   0xCB42  // 落日琥珀暖橙 (赛道激进、中大倾角)
#define LEAN_COLOR_ALERT  0xB9C4  // 珊瑚哑光醇红 (极限倾角预警)
#define LEAN_DIAL_BG      0x0821  // 子表盘沉稳黑曜石暗底
#define LEAN_DIAL_RING    0x3186  // 子表盘金属灰标尺外圈
#define LEAN_CROSSHAIR    0x2124  // 十字准星低对比暗灰
#define LEAN_TICK_WHITE   0xFFFF  // 刻度纯白
#define LEAN_TICK_DIM     0x4208  // 细刻度冷银暗灰

// 静态平滑变量
static float dispG = 0.0f;

void drawLeanMeter() {
  onWaitScreen = false;
  frameIsMap = false;
  gfx->fillScreen(C_BLACK);

  // 1. 顶部状态栏：精致状态小圆点
  aaDisc(CX - 120, 24, 4.0f, bleConnected ? LEAN_COLOR_SAFE : 0x632C);

  const float CX_DIAL = (float)CX;
  const float CY_DIAL = (float)CX; // 466x466 圆屏中心 (233, 233)

  // ==========================================
  // 2. 主表盘：上方倾角刻度弧与扇面 (R: 168 ~ 212)
  // ==========================================
  const float R_OUT = 212.0f;
  const float R_IN  = 168.0f;

  float curLean = LeanAngleMeter::leanAngle;
  float absCurLean = fabsf(curLean);

  // 倾角扇面色彩填充 (≤35° 薄荷青绿, 35°~50° 琥珀暖橙, >50° 珊瑚醇红)
  if (absCurLean >= 1.0f) {
    float a1 = 270.0f;
    float a2 = 270.0f + curLean;
    if (a1 > a2) { float tmp = a1; a1 = a2; a2 = tmp; }

    uint16_t sectorColor;
    if (absCurLean <= 35.0f) sectorColor = LEAN_COLOR_SAFE;
    else if (absCurLean <= 50.0f) sectorColor = LEAN_COLOR_WARN;
    else sectorColor = LEAN_COLOR_ALERT;

    const float STEP = 0.5f;
    for (float a = a1; a < a2; a += STEP) {
      float nextA = a + STEP;
      if (nextA > a2) nextA = a2;

      float rad1 = a * (float)M_PI / 180.0f;
      float rad2 = nextA * (float)M_PI / 180.0f;

      float cos1 = cosf(rad1), sin1 = sinf(rad1);
      float cos2 = cosf(rad2), sin2 = sinf(rad2);

      int x_in1  = (int)lroundf(CX_DIAL + R_IN * cos1);
      int y_in1  = (int)lroundf(CY_DIAL + R_IN * sin1);
      int x_out1 = (int)lroundf(CX_DIAL + R_OUT * cos1);
      int y_out1 = (int)lroundf(CY_DIAL + R_OUT * sin1);

      int x_in2  = (int)lroundf(CX_DIAL + R_IN * cos2);
      int y_in2  = (int)lroundf(CY_DIAL + R_IN * sin2);
      int x_out2 = (int)lroundf(CX_DIAL + R_OUT * cos2);
      int y_out2 = (int)lroundf(CY_DIAL + R_OUT * sin2);

      gfx->fillTriangle(x_in1, y_in1, x_out1, y_out1, x_out2, y_out2, sectorColor);
      gfx->fillTriangle(x_in1, y_in1, x_out2, y_out2, x_in2, y_in2, sectorColor);
    }

    // 扇面当前倾角边缘指针 (加粗纯白线)
    float curRad = (270.0f + curLean) * (float)M_PI / 180.0f;
    float cCos = cosf(curRad), cSin = sinf(curRad);
    int px1 = (int)lroundf(CX_DIAL + (R_IN - 4) * cCos), py1 = (int)lroundf(CY_DIAL + (R_IN - 4) * cSin);
    int px2 = (int)lroundf(CX_DIAL + (R_OUT + 4) * cCos), py2 = (int)lroundf(CY_DIAL + (R_OUT + 4) * cSin);
    gfx->drawLine(px1, py1, px2, py2, C_WHITE);
    gfx->drawLine(px1 + 1, py1, px2 + 1, py2, C_WHITE);
  }

  // 机械刻度盘 (-60° 到 +60°)
  for (int a = -60; a <= 60; a += 2) {
    float rad = (270.0f + a) * (float)M_PI / 180.0f;
    float cosA = cosf(rad), sinA = sinf(rad);
    int absA = abs(a);

    float len = (absA % 30 == 0) ? 26.0f : ((absA % 10 == 0) ? 16.0f : ((absA % 5 == 0) ? 9.0f : 0.0f));
    if (len == 0.0f) continue;

    uint16_t c;
    if (absA == 0) c = C_WHITE;
    else if (absA % 30 == 0) c = C_WHITE;
    else if (absA % 10 == 0) c = 0xCE79;
    else c = LEAN_TICK_DIM;

    int x1 = (int)lroundf(CX_DIAL + (R_OUT - len) * cosA);
    int y1 = (int)lroundf(CY_DIAL + (R_OUT - len) * sinA);
    int x2 = (int)lroundf(CX_DIAL + R_OUT * cosA);
    int y2 = (int)lroundf(CY_DIAL + R_OUT * sinA);
    gfx->drawLine(x1, y1, x2, y2, c);
    if (absA % 30 == 0) gfx->drawLine(x1 + 1, y1, x2 + 1, y2, c);
  }

  // 0° 正中垂直线
  gfx->drawLine((int)CX_DIAL, (int)(CY_DIAL - R_OUT), (int)CX_DIAL, (int)(CY_DIAL - R_OUT + 28), C_WHITE);
  gfx->drawLine((int)CX_DIAL - 1, (int)(CY_DIAL - R_OUT), (int)CX_DIAL - 1, (int)(CY_DIAL - R_OUT + 28), C_WHITE);

  // 刻度数值标签
  drawAaString((float)CX_DIAL, (float)(CY_DIAL - R_OUT - 12), "0", 2.0f, 1.15f, 11.0f, C_WHITE, true);

  float rad30 = 300.0f * (float)M_PI / 180.0f;
  float radM30 = 240.0f * (float)M_PI / 180.0f;
  drawAaString(CX_DIAL + (R_OUT + 14) * cosf(rad30), CY_DIAL + (R_OUT + 14) * sinf(rad30), "30", 1.8f, 1.05f, 10.5f, C_WHITE, true);
  drawAaString(CX_DIAL + (R_OUT + 14) * cosf(radM30), CY_DIAL + (R_OUT + 14) * sinf(radM30), "-30", 1.8f, 1.05f, 10.5f, C_WHITE, true);

  // ==========================================
  // 3. 历史最大倾角：直接在刻度盘上以三角标出 (放弃原来的侧边方块)
  // ==========================================
  if (LeanAngleMeter::maxLeftLean >= 2.0f) {
    float mRad = (270.0f - LeanAngleMeter::maxLeftLean) * (float)M_PI / 180.0f;
    float mCos = cosf(mRad), mSin = sinf(mRad);
    // 刻度边缘指向内的小三角游标 (琥珀色醒目机械指针)
    int tipX = (int)lroundf(CX_DIAL + (R_OUT + 2) * mCos);
    int tipY = (int)lroundf(CY_DIAL + (R_OUT + 2) * mSin);
    int b1X  = (int)lroundf(CX_DIAL + (R_OUT + 13) * mCos - 6 * mSin);
    int b1Y  = (int)lroundf(CY_DIAL + (R_OUT + 13) * mSin + 6 * mCos);
    int b2X  = (int)lroundf(CX_DIAL + (R_OUT + 13) * mCos + 6 * mSin);
    int b2Y  = (int)lroundf(CY_DIAL + (R_OUT + 13) * mSin - 6 * mCos);
    gfx->fillTriangle(tipX, tipY, b1X, b1Y, b2X, b2Y, LEAN_COLOR_WARN);
  }

  if (LeanAngleMeter::maxRightLean >= 2.0f) {
    float mRad = (270.0f + LeanAngleMeter::maxRightLean) * (float)M_PI / 180.0f;
    float mCos = cosf(mRad), mSin = sinf(mRad);
    int tipX = (int)lroundf(CX_DIAL + (R_OUT + 2) * mCos);
    int tipY = (int)lroundf(CY_DIAL + (R_OUT + 2) * mSin);
    int b1X  = (int)lroundf(CX_DIAL + (R_OUT + 13) * mCos - 6 * mSin);
    int b1Y  = (int)lroundf(CY_DIAL + (R_OUT + 13) * mSin + 6 * mCos);
    int b2X  = (int)lroundf(CX_DIAL + (R_OUT + 13) * mCos + 6 * mSin);
    int b2Y  = (int)lroundf(CY_DIAL + (R_OUT + 13) * mSin - 6 * mCos);
    gfx->fillTriangle(tipX, tipY, b1X, b1Y, b2X, b2Y, LEAN_COLOR_WARN);
  }

  // 正上方倾角实时大号数字 (Y = 100)
  char leanStr[16];
  if (absCurLean < 0.5f) {
    snprintf(leanStr, sizeof(leanStr), "0*");
  } else if (curLean < 0) {
    snprintf(leanStr, sizeof(leanStr), "-%d*", (int)lroundf(absCurLean));
  } else {
    snprintf(leanStr, sizeof(leanStr), "+%d*", (int)lroundf(absCurLean));
  }

  uint16_t leanColor = (absCurLean < 0.5f) ? C_WHITE :
                       ((absCurLean <= 35.0f) ? LEAN_COLOR_SAFE :
                       ((absCurLean <= 50.0f) ? LEAN_COLOR_WARN : LEAN_COLOR_ALERT));

  drawAaString(CX, 80, "LEAN", 1.2f, 0.75f, 9.0f, 0x7BEF, true);
  drawAaString(CX, 102, leanStr, 3.2f, 1.75f, 20.0f, leanColor, true);

  // ==========================================
  // 4. 机械计时码表 9点钟子表盘：缩小版 G 值表 (CX=122, CY=233, R=52)
  // ==========================================
  const int G_SUB_X = 122;
  const int G_SUB_Y = CY_DIAL;
  const int G_SUB_R = 52;
  const float MAX_G = 1.5f;

  // 9点位子表盘外框与背景
  drawTranslucentCircle(G_SUB_X, G_SUB_Y, G_SUB_R, LEAN_DIAL_BG, 220);
  gfx->drawCircle(G_SUB_X, G_SUB_Y, G_SUB_R, LEAN_DIAL_RING);
  gfx->drawCircle(G_SUB_X, G_SUB_Y, G_SUB_R * 2 / 3, 0x2104); // 1.0G 标尺圈
  gfx->drawCircle(G_SUB_X, G_SUB_Y, G_SUB_R / 3,     0x18C3); // 0.5G 标尺圈

  // 十字准星
  gfx->drawLine(G_SUB_X - G_SUB_R + 6, G_SUB_Y, G_SUB_X + G_SUB_R - 6, G_SUB_Y, LEAN_CROSSHAIR);
  gfx->drawLine(G_SUB_X, G_SUB_Y - G_SUB_R + 6, G_SUB_X, G_SUB_Y + G_SUB_R - 6, LEAN_CROSSHAIR);

  // 子表盘标题与加速/刹车微标
  drawAaString(G_SUB_X, G_SUB_Y - G_SUB_R + 9, "G-METER", 1.15f, 0.70f, 8.5f, 0x7BEF, true);

  // 动态 G 值大圆点 (用户特别要求：圆点太小了，显著放大！)
  const float MOVE_R = (float)(G_SUB_R - 14);
  float normLong = LeanAngleMeter::gLongitudinal / MAX_G;
  float normLat  = LeanAngleMeter::gLateral / MAX_G;
  float gdx = normLat * MOVE_R;
  float gdy = -normLong * MOVE_R;
  float gDist = sqrtf(gdx * gdx + gdy * gdy);
  if (gDist > MOVE_R) {
    gdx = (gdx / gDist) * MOVE_R;
    gdy = (gdy / gDist) * MOVE_R;
  }
  int gbx = (int)lroundf((float)G_SUB_X + gdx);
  int gby = (int)lroundf((float)G_SUB_Y + gdy);

  // 绘制放大版高能 G 圆球 (半径 10.5px，双层外晕 + 中心高亮白核)
  aaDisc((float)gbx, (float)gby, 10.5f, 0x1A29);
  aaDisc((float)gbx, (float)gby, 7.5f,  LEAN_COLOR_SAFE);
  aaDisc((float)gbx, (float)gby, 3.5f,  C_WHITE);

  // 综合 G 值读数平滑滤波与显示
  float targetG = LeanAngleMeter::gTotal;
  if (fabsf(targetG - dispG) > 0.015f) dispG += 0.12f * (targetG - dispG);
  if (dispG < 0.02f) dispG = 0.0f;
  char gNumBuf[16];
  snprintf(gNumBuf, sizeof(gNumBuf), "%.2fG", dispG);
  drawAaString(G_SUB_X, G_SUB_Y + G_SUB_R - 10, gNumBuf, 1.8f, 1.05f, 12.0f, C_WHITE, true);

  // ==========================================
  // 5. 机械计时码表 3点钟子表盘：海拔、爬升与坡度视窗 (CX=344, CY=233, R=52)
  // ==========================================
  const int ALT_SUB_X = 344;
  const int ALT_SUB_Y = CY_DIAL;
  const int ALT_SUB_R = 52;

  // 3点位子表盘外框与背景
  drawTranslucentCircle(ALT_SUB_X, ALT_SUB_Y, ALT_SUB_R, LEAN_DIAL_BG, 220);
  gfx->drawCircle(ALT_SUB_X, ALT_SUB_Y, ALT_SUB_R, LEAN_DIAL_RING);

  // 标题
  drawAaString(ALT_SUB_X, ALT_SUB_Y - ALT_SUB_R + 9, "ALTIMETER", 1.15f, 0.70f, 8.5f, 0x7BEF, true);

  // 1) 实时海拔 (ALT)
  char altValBuf[16];
  if (altValid) snprintf(altValBuf, sizeof(altValBuf), "%dm", altitudeM);
  else snprintf(altValBuf, sizeof(altValBuf), "--m");
  drawAaString(ALT_SUB_X - 18, ALT_SUB_Y - 14, "ALT", 1.15f, 0.70f, 8.5f, 0x8410, true);
  drawAaString(ALT_SUB_X + 20, ALT_SUB_Y - 14, altValBuf, 1.7f, 1.05f, 12.0f, C_WHITE, true);

  // 分隔细线
  gfx->drawLine(ALT_SUB_X - 32, ALT_SUB_Y - 3, ALT_SUB_X + 32, ALT_SUB_Y - 3, 0x2124);

  // 2) 累计爬升 (CLIMB)
  char climbBuf[16];
  if (altValid) snprintf(climbBuf, sizeof(climbBuf), "+%dm", climbAccumM);
  else snprintf(climbBuf, sizeof(climbBuf), "+0m");
  drawAaString(ALT_SUB_X - 18, ALT_SUB_Y + 9, "UP", 1.15f, 0.70f, 8.5f, 0x8410, true);
  drawAaString(ALT_SUB_X + 20, ALT_SUB_Y + 9, climbBuf, 1.5f, 0.95f, 11.0f, 0xCE79, true);

  // 3) 路段坡度 (SLOPE)
  char slpValBuf[16];
  if (altValid) {
    if (slopePct > 0) snprintf(slpValBuf, sizeof(slpValBuf), "+%d%%", slopePct);
    else snprintf(slpValBuf, sizeof(slpValBuf), "%d%%", slopePct);
  } else snprintf(slpValBuf, sizeof(slpValBuf), "--%%");
  uint16_t slpColor = (altValid && abs(slopePct) >= 5) ? LEAN_COLOR_WARN : 0x7BEF;
  drawAaString(ALT_SUB_X, ALT_SUB_Y + ALT_SUB_R - 10, slpValBuf, 1.7f, 1.05f, 12.0f, slpColor, true);

  // ==========================================
  // 6. 底部 6点钟位置：极限 G 统计胶囊 (Y = 368)
  // ==========================================
  const int BOT_Y = 368;
  const int BOT_W = 168;
  const int BOT_H = 32;
  const int BOT_X = CX - BOT_W / 2;

  bool justCalib = millis() < LeanAngleMeter::calibSuccessUntil;
  if (justCalib) {
    drawTranslucentRoundRect(BOT_X, BOT_Y, BOT_W, BOT_H, 16, LEAN_COLOR_SAFE, 220);
    drawAaString(CX, BOT_Y + 16, "CALIBRATED OK", 1.8f, 1.05f, 12.0f, C_BLACK, true);
  } else {
    drawTranslucentRoundRect(BOT_X, BOT_Y, BOT_W, BOT_H, 16, 0x0000, 190);
    gfx->drawRoundRect(BOT_X, BOT_Y, BOT_W, BOT_H, 16, 0x31A6);

    char maxGBuf[16];
    snprintf(maxGBuf, sizeof(maxGBuf), "%.2f G", LeanAngleMeter::maxG);
    drawAaString(CX - 38, BOT_Y + 16, "MAX G", 1.35f, 0.85f, 9.5f, 0x8410, true);
    drawAaString(CX + 32, BOT_Y + 16, maxGBuf, 2.0f, 1.15f, 13.0f, C_WHITE, true);
  }

  // 底部极简提示
  drawAaString(CX, 418, "DOUBLE TAP RESET · HOLD CALIB", 1.15f, 0.70f, 8.5f, 0x4208, true);

  // 7. 页面指示点
  drawPageDots();
  needFlush = true;
}

void leanMeterTap(int x, int y) {
  if (dragging || page != 3 || callOn || pairOn) return;
  float dx = x - CX, dy = y - CX;
  touchVectorToCanvas(dx, dy);
  x = (int)lroundf(CX + dx); y = (int)lroundf(CX + dy);

  static unsigned long lastTapAt = 0;
  unsigned long now = millis();
  bool isDouble = (now - lastTapAt < 400);
  lastTapAt = now;

  // 双击屏幕重置极值统计 (最大倾角与最大 G 值)
  if (isDouble) {
    CompanionMode::startVibration(40, 20);
    LeanAngleMeter::resetStats();
    leanDirty = true;
  }
}
