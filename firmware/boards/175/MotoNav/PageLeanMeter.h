#pragma once
#include <Arduino.h>
#include "LeanAngleMeter.h"
#include "UiModern.h"

extern bool dragging;

// 车规级哑光质感配色（低眩光、沉稳自然、拒绝刺眼荧光色）
#define LEAN_COLOR_SAFE   0x34EA  // 哑光薄荷青绿 (优雅清爽、低眩光)
#define LEAN_COLOR_WARN   0xCB42  // 落日琥珀暖橙 (沉稳醒目、不刺眼)
#define LEAN_COLOR_ALERT  0xB9C4  // 珊瑚哑光醇红 (清晰预警、柔和深邃)
#define LEAN_RING_05G     0x2104  // 0.5G 极暗灰白标尺
#define LEAN_RING_10G     0x3186  // 1.0G 哑光银灰标尺
#define LEAN_RING_15G     0x4A69  // 1.5G 外层冷银边界
#define LEAN_CROSSHAIR    0x2945  // 十字准星低对比暗灰

void drawLeanMeter() {
  onWaitScreen = false;
  frameIsMap = false;
  gfx->fillScreen(C_BLACK);

  // 1. 顶部状态栏与实时海拔坡度胶囊 (阶段一特性：实时海拔高度与路段坡度)
  aaDisc(CX - 124, 30, 4.0f, bleConnected ? LEAN_COLOR_SAFE : 0x632C);

  const int ALT_W = 196, ALT_H = 28, ALT_Y = 16;
  const int ALT_X = CX - ALT_W / 2;
  drawTranslucentRoundRect(ALT_X, ALT_Y, ALT_W, ALT_H, 14, 0x0000, 185);
  gfx->drawRoundRect(ALT_X, ALT_Y, ALT_W, ALT_H, 14, 0x31A6);

  char altBuf[16];
  if (altValid) snprintf(altBuf, sizeof(altBuf), "%dm", altitudeM);
  else snprintf(altBuf, sizeof(altBuf), "--m");

  char slpBuf[16];
  if (altValid) {
    if (slopePct > 0) snprintf(slpBuf, sizeof(slpBuf), "+%d%%", slopePct);
    else snprintf(slpBuf, sizeof(slpBuf), "%d%%", slopePct);
  } else snprintf(slpBuf, sizeof(slpBuf), "--%%");

  // 左半边：海拔 ALT
  drawAaString(ALT_X + 28, ALT_Y + 14, "ALT", 1.25f, 0.80f, 9.0f, 0x8410, true);
  drawAaString(ALT_X + 66, ALT_Y + 14, altBuf, 1.7f, 1.05f, 12.0f, C_WHITE, true);

  // 中间精致微分割线
  gfx->drawFastVLine(ALT_X + 98, ALT_Y + 6, ALT_H - 12, 0x31A6);

  // 右半边：路段坡度 SLOPE
  drawAaString(ALT_X + 130, ALT_Y + 14, "SLOPE", 1.25f, 0.80f, 9.0f, 0x8410, true);
  uint16_t slpColor = (altValid && abs(slopePct) >= 5) ? LEAN_COLOR_WARN : C_WHITE;
  drawAaString(ALT_X + 170, ALT_Y + 14, slpBuf, 1.7f, 1.05f, 12.0f, slpColor, true);

  const int CY = CX; // 466x466 圆屏中心 (233, 233)
  const float CX_DIAL = (float)CX;
  const float CY_DIAL = (float)CY;

  // ==========================================
  // 2. 外层环形：倾角扇形与加粗加大刻度盘 (R: 154 ~ 206)
  // ==========================================
  const float R_OUT = 206.0f;
  const float R_IN  = 154.0f;

  float curLean = LeanAngleMeter::leanAngle;
  float absCurLean = fabsf(curLean);

  // 扇形实心色彩填充：纯粹由当前刻度决定变色 (≤35° 薄荷青绿, 35°~50° 琥珀暖橙, >50° 珊瑚醇红)
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

    // 扇面当前倾角边缘指针 (加粗纯白线，清晰醒目)
    float curRad = (270.0f + curLean) * (float)M_PI / 180.0f;
    float cCos = cosf(curRad), cSin = sinf(curRad);
    int px1 = (int)lroundf(CX_DIAL + (R_IN - 5) * cCos), py1 = (int)lroundf(CY_DIAL + (R_IN - 5) * cSin);
    int px2 = (int)lroundf(CX_DIAL + (R_OUT + 5) * cCos), py2 = (int)lroundf(CY_DIAL + (R_OUT + 5) * cSin);
    gfx->drawLine(px1, py1, px2, py2, C_WHITE);
    gfx->drawLine(px1 + 1, py1, px2 + 1, py2, C_WHITE);
    gfx->drawLine(px1 - 1, py1, px2 - 1, py2, C_WHITE);
  }

  // 覆盖放射状刻度盘 (-60° 到 +60°)：仪表刻度统一使用纯净高雅的白色与冷银灰，拒绝荧光色！
  for (int a = -60; a <= 60; a += 2) {
    float rad = (270.0f + a) * (float)M_PI / 180.0f;
    float cosA = cosf(rad), sinA = sinf(rad);
    int absA = abs(a);

    float len = (absA % 30 == 0) ? 28.0f : ((absA % 10 == 0) ? 18.0f : ((absA % 5 == 0) ? 10.0f : 0.0f));
    if (len == 0.0f) continue;

    uint16_t c;
    if (absA == 0) {
      c = C_WHITE; // 0° 正中主刻度
    } else if (absA % 30 == 0) {
      c = C_WHITE; // 30° / 60° 主刻度纯白
    } else if (absA % 10 == 0) {
      c = 0xCE79;  // 10° / 20° / 40° / 50° 冷银白
    } else {
      c = 0x52AA;  // 5° 细分刻度暗冷银
    }

    int x1 = (int)lroundf(CX_DIAL + (R_OUT - len) * cosA);
    int y1 = (int)lroundf(CY_DIAL + (R_OUT - len) * sinA);
    int x2 = (int)lroundf(CX_DIAL + R_OUT * cosA);
    int y2 = (int)lroundf(CY_DIAL + R_OUT * sinA);
    gfx->drawLine(x1, y1, x2, y2, c);

    if (absA % 30 == 0) {
      gfx->drawLine(x1 + 1, y1, x2 + 1, y2, c);
      gfx->drawLine(x1 - 1, y1, x2 - 1, y2, c);
    } else if (absA % 10 == 0) {
      gfx->drawLine(x1 + 1, y1, x2 + 1, y2, c);
    }
  }

  // 0° 正上方中线强化 (3像素加粗纯白)
  gfx->drawLine((int)CX_DIAL - 1, (int)(CY_DIAL - R_OUT), (int)CX_DIAL - 1, (int)(CY_DIAL - R_OUT + 30), C_WHITE);
  gfx->drawLine((int)CX_DIAL,     (int)(CY_DIAL - R_OUT), (int)CX_DIAL,     (int)(CY_DIAL - R_OUT + 30), C_WHITE);
  gfx->drawLine((int)CX_DIAL + 1, (int)(CY_DIAL - R_OUT), (int)CX_DIAL + 1, (int)(CY_DIAL - R_OUT + 30), C_WHITE);

  // 刻度值数字：字号全面增大，统一纯白与冷白，骑行一目了然
  drawAaString((float)CX_DIAL, (float)(CY_DIAL - R_OUT - 14), "0", 2.2f, 1.25f, 12.0f, C_WHITE, true);

  float rad30 = 300.0f * (float)M_PI / 180.0f;
  float radM30 = 240.0f * (float)M_PI / 180.0f;
  drawAaString(CX_DIAL + (R_OUT + 18) * cosf(rad30), CY_DIAL + (R_OUT + 18) * sinf(rad30), "30", 2.0f, 1.15f, 11.5f, C_WHITE, true);
  drawAaString(CX_DIAL + (R_OUT + 18) * cosf(radM30), CY_DIAL + (R_OUT + 18) * sinf(radM30), "-30", 2.0f, 1.15f, 11.5f, C_WHITE, true);

  float rad60 = 330.0f * (float)M_PI / 180.0f;
  float radM60 = 210.0f * (float)M_PI / 180.0f;
  drawAaString(CX_DIAL + (R_OUT + 18) * cosf(rad60), CY_DIAL + (R_OUT + 18) * sinf(rad60), "60", 2.0f, 1.15f, 11.5f, 0xCE79, true);
  drawAaString(CX_DIAL + (R_OUT + 18) * cosf(radM60), CY_DIAL + (R_OUT + 18) * sinf(radM60), "-60", 2.0f, 1.15f, 11.5f, 0xCE79, true);

  // ==========================================
  // 3. 实时倾角大号数字：移至 G 值表上方黄金空白区 (Y = 104)
  // ==========================================
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

  // 正上方微注与超大号实时角度数字（视线最自然的倾角集中区）
  drawAaString(CX, 82, "LEAN", 1.15f, 0.70f, 8.5f, 0x7BEF, true);
  drawAaString(CX, 104, leanStr, 3.2f, 1.75f, 20.0f, leanColor, true);

  // ==========================================
  // 4. G 值表圆圈与多层同心标尺 (0.5G / 1.0G / 1.5G)
  // ==========================================
  const int GR = 104;
  const float MAX_G = 1.5f;

  gfx->drawCircle(CX, CY, 35, LEAN_RING_05G);
  gfx->drawCircle(CX, CY, 70, LEAN_RING_10G);
  gfx->drawCircle(CX, CY, GR, LEAN_RING_15G);

  // 十字准星 (中心留空)
  gfx->drawLine(CX - GR, CY, CX - 8, CY, LEAN_CROSSHAIR);
  gfx->drawLine(CX + 8, CY, CX + GR, CY, LEAN_CROSSHAIR);
  gfx->drawLine(CX, CY - GR, CX, CY - 8, LEAN_CROSSHAIR);
  gfx->drawLine(CX, CY + 8, CX, CY + GR, LEAN_CROSSHAIR);

  // 十字端点平滑抗锯齿微注
  drawAaString(CX, CY - GR + 13, "^ ACC", 1.2f, 0.75f, 8.5f, 0x7BEF, true);
  drawAaString(CX, CY + GR - 13, "v BRK", 1.2f, 0.75f, 8.5f, 0x7BEF, true);

  // 动态大光圈小球
  const float MOVE_R = (float)(GR - 13);
  float normLong = LeanAngleMeter::gLongitudinal / MAX_G;
  float normLat  = LeanAngleMeter::gLateral / MAX_G;
  float dx = normLat * MOVE_R;
  float dy = -normLong * MOVE_R;

  float dist = sqrtf(dx * dx + dy * dy);
  if (dist > MOVE_R) {
    dx = (dx / dist) * MOVE_R;
    dy = (dy / dist) * MOVE_R;
  }

  int bx = (int)lroundf(CX_DIAL + dx);
  int by = (int)lroundf(CY_DIAL + dy);

  // 柔和微光小球 (中心高亮，外围哑光青绿圆弧)
  aaDisc((float)bx, (float)by, 8.5f, 0x1A29);
  gfx->drawCircle(bx, by, 8, LEAN_COLOR_SAFE);
  aaDisc((float)bx, (float)by, 3.5f, C_WHITE);

  // 平滑滤波准备实时综合 G 值数值（移至底部卡片显示，圆盘内不再有重叠文字遮挡）
  static float dispG = 0.0f;
  float targetG = LeanAngleMeter::gTotal;
  if (fabsf(targetG - dispG) > 0.015f) {
    dispG += 0.10f * (targetG - dispG);
  }
  if (dispG < 0.02f) dispG = 0.0f;

  char gNumBuf[16];
  snprintf(gNumBuf, sizeof(gNumBuf), "%.2f G", dispG);

  // ==========================================
  // 5. 屏幕两侧半透明毛玻璃胶囊：MAX L 与 MAX R (大号字版)
  // ==========================================
  const int SIDE_W = 96;
  const int SIDE_H = 48;
  const int SIDE_OFFSET = 168;
  const int SIDE_L_X = CX - SIDE_OFFSET - SIDE_W / 2;
  const int SIDE_R_X = CX + SIDE_OFFSET - SIDE_W / 2;
  const int SIDE_Y   = CY - SIDE_H / 2;

  // 左侧胶囊: MAX L
  drawTranslucentRoundRect(SIDE_L_X, SIDE_Y, SIDE_W, SIDE_H, 16, 0x0000, 185);
  gfx->drawRoundRect(SIDE_L_X, SIDE_Y, SIDE_W, SIDE_H, 16, 0x31A6);

  char maxLStr[16];
  snprintf(maxLStr, sizeof(maxLStr), "-%d*", (int)lroundf(LeanAngleMeter::maxLeftLean));
  drawAaString(SIDE_L_X + SIDE_W / 2, SIDE_Y + 12, "MAX L", 1.45f, 0.90f, 10.0f, 0x8410, true);
  drawAaString(SIDE_L_X + SIDE_W / 2, SIDE_Y + 32, maxLStr, 2.4f, 1.40f, 15.0f, C_WHITE, true);

  // 右侧胶囊: MAX R
  drawTranslucentRoundRect(SIDE_R_X, SIDE_Y, SIDE_W, SIDE_H, 16, 0x0000, 185);
  gfx->drawRoundRect(SIDE_R_X, SIDE_Y, SIDE_W, SIDE_H, 16, 0x31A6);

  char maxRStr[16];
  snprintf(maxRStr, sizeof(maxRStr), "+%d*", (int)lroundf(LeanAngleMeter::maxRightLean));
  drawAaString(SIDE_R_X + SIDE_W / 2, SIDE_Y + 12, "MAX R", 1.45f, 0.90f, 10.0f, 0x8410, true);
  drawAaString(SIDE_R_X + SIDE_W / 2, SIDE_Y + 32, maxRStr, 2.4f, 1.40f, 15.0f, C_WHITE, true);

  // ==========================================
  // 6. 底部第 1 行: MAX G 极值胶囊卡片 (Y = 338, 大号字)
  // ==========================================
  const int CAP1_Y = 338;
  const int CAP1_W = 200;
  const int CAP1_H = 34;
  const int CAP1_X = CX - CAP1_W / 2;

  bool justCalib = millis() < LeanAngleMeter::calibSuccessUntil;
  if (justCalib) {
    drawTranslucentRoundRect(CAP1_X, CAP1_Y, CAP1_W, CAP1_H, 17, LEAN_COLOR_SAFE, 220);
    gfx->drawRoundRect(CAP1_X, CAP1_Y, CAP1_W, CAP1_H, 17, LEAN_COLOR_SAFE);
    drawAaString(CX, CAP1_Y + 17, "CALIBRATED OK", 1.8f, 1.05f, 12.0f, C_BLACK, true);
  } else {
    drawTranslucentRoundRect(CAP1_X, CAP1_Y, CAP1_W, CAP1_H, 17, 0x0000, 185);
    gfx->drawRoundRect(CAP1_X, CAP1_Y, CAP1_W, CAP1_H, 17, 0x31A6);

    char maxGBuf[16];
    snprintf(maxGBuf, sizeof(maxGBuf), "%.2f G", LeanAngleMeter::maxG);
    drawAaString(CX - 46, CAP1_Y + 17, "MAX G", 1.5f, 0.95f, 10.5f, 0x8410, true);
    drawAaString(CX + 38, CAP1_Y + 17, maxGBuf, 2.2f, 1.25f, 14.0f, C_WHITE, true);
  }

  // ==========================================
  // 7. 底部第 2 行: 实时综合 G 值大号数字胶囊卡片 (Y = 378，由倾角腾出)
  // ==========================================
  const int CAP2_Y = 378;
  const int CAP2_W = 200;
  const int CAP2_H = 36;
  const int CAP2_X = CX - CAP2_W / 2;

  drawTranslucentRoundRect(CAP2_X, CAP2_Y, CAP2_W, CAP2_H, 18, 0x0000, 185);
  gfx->drawRoundRect(CAP2_X, CAP2_Y, CAP2_W, CAP2_H, 18, 0x31A6);

  drawAaString(CX - 46, CAP2_Y + 18, "G-FORCE", 1.45f, 0.90f, 10.0f, 0x8410, true);
  drawAaString(CX + 38, CAP2_Y + 18, gNumBuf, 2.4f, 1.35f, 15.0f, C_WHITE, true);

  // 零位校准提示 (底部平滑微字)
  drawAaString(CX, 422, "HOLD TO CALIBRATE", 1.3f, 0.80f, 9.0f, 0x52AA, true);

  // 8. 底部页面指示点
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

  // 双击屏幕重置极值统计
  if (isDouble) {
    CompanionMode::startVibration(40, 20);
    LeanAngleMeter::resetStats();
    leanDirty = true;
  }
}
