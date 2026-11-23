#pragma once
#include <Arduino.h>
#include "LeanAngleMeter.h"
#include "UiModern.h"
#include "AlongBanner.h"

extern bool dragging;

// =========================================================================
// 极简扁平高对比 HUD 仪表配色
// =========================================================================
#define HUD_COLOR_BG       0x0000  // 纯黑背景
#define HUD_COLOR_CYAN     0x07FF  // 亮青核心高亮色
#define HUD_COLOR_CYAN_DIM 0x1A0F  // 暗青色未激活扇区
#define HUD_COLOR_AMBER    0xFD20  // 赛道预警琥珀橙 (35°~50°)
#define HUD_COLOR_RED      0xF800  // 极限危险红 (>50°)
#define HUD_COLOR_G_POLY1  0x3944  // 会话最大 G 包络填充 (红褐)
#define HUD_COLOR_G_LINE1  0xD2C7  // 会话最大 G 轮廓线 (赤陶红)
#define HUD_COLOR_G_POLY2  0x31E5  // 近期动态 G 包络填充 (暖琥珀)
#define HUD_COLOR_G_LINE2  0xCCA9  // 近期动态 G 轮廓线 (金琥珀)
#define HUD_COLOR_BLUE_G   0x03DF  // G 值球纯蓝色（用于半透明渲染）
#define HUD_COLOR_GREEN    0x0760  // 荧光绿
#define HUD_COLOR_TEXT_DIM 0x9CD3  // 清晰银灰标签

// 静态平滑变量
static float dispG = 0.0f;

// 辅助：半透明纯色实心圆（带边缘 1px 抗锯齿，无渐变、无外发光）
static void drawTranslucentDisc(float cx, float cy, float r, uint16_t col, uint8_t alpha) {
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) return;
  int x0 = max(0, (int)floorf(cx - r - 1.0f));
  int x1 = min(LCD_WIDTH - 1, (int)ceilf(cx + r + 1.0f));
  int y0 = max(0, (int)floorf(cy - r - 1.0f));
  int y1 = min(LCD_HEIGHT - 1, (int)ceilf(cy + r + 1.0f));
  for (int y = y0; y <= y1; y++) {
    float dy = (float)y - cy;
    for (int x = x0; x <= x1; x++) {
      float dx = (float)x - cx;
      float d = sqrtf(dx * dx + dy * dy);
      if (d <= r + 0.5f) {
        float cov = (d <= r - 0.5f) ? 1.0f : (r + 0.5f - d);
        uint8_t a = (uint8_t)(alpha * cov);
        uint16_t &px = fb[y * LCD_WIDTH + x];
        px = blend565(px, col, a);
      }
    }
  }
}

// 辅助：右对齐抗锯齿字符串（严格不超出 rightX 边界）
static void drawAaStringRight(float rightX, float cy, const char *str, float scale, float dotR, float spacing, uint16_t color) {
  (void)dotR;
  (void)spacing;
  if (!str || !str[0]) return;
  const SmoothFont &f = smoothPickFont(scale * 7.2f);
  smoothDrawText(f, str, (int)lroundf(rightX), (int)lroundf(cy), color, 2, 1, true);
}

// 辅助：沿圆心角度 deg（0°=12点，负=左倾，正=右倾）绘制带厚度的径向标记线/指针
static void drawLeanRadialMarker(float cx, float cy, float deg, float rStart, float rEnd, float halfWidth, uint16_t col) {
  float rad = (270.0f + deg) * (float)M_PI / 180.0f;
  float ux = cosf(rad), uy = sinf(rad);
  float nx = -uy * halfWidth, ny = ux * halfWidth;
  int x0 = (int)lroundf(cx + rStart * ux - nx), y0 = (int)lroundf(cy + rStart * uy - ny);
  int x1 = (int)lroundf(cx + rStart * ux + nx), y1 = (int)lroundf(cy + rStart * uy + ny);
  int x2 = (int)lroundf(cx + rEnd   * ux + nx), y2 = (int)lroundf(cy + rEnd   * uy + ny);
  int x3 = (int)lroundf(cx + rEnd   * ux - nx), y3 = (int)lroundf(cy + rEnd   * uy - ny);
  gfx->fillTriangle(x0, y0, x1, y1, x2, y2, col);
  gfx->fillTriangle(x0, y0, x2, y2, x3, y3, col);
}

void drawLeanMeter() {
  onWaitScreen = false;
  frameIsMap = false;
  gfx->fillScreen(HUD_COLOR_BG);

  const float K = (float)LCD_WIDTH / 466.0f;
  const float CX_CENTER = (float)CX;
  const float CY_CENTER = (float)(LCD_HEIGHT / 2);

  float curLean = LeanAngleMeter::leanAngle;
  if (curLean < -89.0f) curLean = -89.0f;
  if (curLean >  89.0f) curLean =  89.0f;
  float absCurLean = fabsf(curLean);

  // =========================================================================
  // 1. 倾角仪：加宽的无极平滑上半圆 180° 刻度带 (9点 ~ 3点) + 指针 + 历史峰值线
  // =========================================================================
  const float R_OUT = CX_CENTER - 4.0f * K;
  const float R_IN  = R_OUT - 26.0f * K;
  {
    uint16_t *fb = gfx->getFramebuffer();
    if (fb) {
      const float rInPad2 = (R_IN - 1.0f) * (R_IN - 1.0f);
      const float rOutPad2 = (R_OUT + 1.0f) * (R_OUT + 1.0f);
      const int yMin = max(0, (int)floorf(CY_CENTER - R_OUT - 1.0f));
      const int yMax = min(LCD_HEIGHT - 1, (int)ceilf(CY_CENTER));
      for (int y = yMin; y <= yMax; y++) {
        float dy = (float)y - CY_CENTER;
        float dy2 = dy * dy;
        if (dy2 > rOutPad2) continue;
        float ho = sqrtf(rOutPad2 - dy2);
        int xa = max(0, (int)floorf(CX_CENTER - ho));
        int xb = min(LCD_WIDTH - 1, (int)ceilf(CX_CENTER + ho));
        uint16_t *row = fb + y * LCD_WIDTH;
        for (int x = xa; x <= xb; x++) {
          float dx = (float)x - CX_CENTER;
          float d2 = dx * dx + dy2;
          if (d2 < rInPad2 || d2 > rOutPad2) continue;
          float r = sqrtf(d2);
          float cov = fminf(r - R_IN + 0.5f, R_OUT - r + 0.5f);
          if (cov <= 0.0f) continue;
          if (cov > 1.0f) cov = 1.0f;

          float angDeg = atan2f(dx, -dy) * 57.2957795f; // 12点为 0°, -90° ~ +90°
          bool isActive = (curLean >= 0.0f) ? (angDeg >= 0.0f && angDeg <= curLean)
                                            : (angDeg <= 0.0f && angDeg >= curLean);
          uint16_t col = HUD_COLOR_CYAN_DIM;
          if (isActive) {
            float absAng = fabsf(angDeg);
            col = (absAng > 50.0f) ? HUD_COLOR_RED : ((absAng > 35.0f) ? HUD_COLOR_AMBER : HUD_COLOR_CYAN);
          }
          uint8_t a = (uint8_t)(cov * 255.0f);
          row[x] = (a >= 255) ? col : blend565(row[x], col, a);
        }
      }
    }
  }

  // 12 点钟零度基准短标线
  drawLeanRadialMarker(CX_CENTER, CY_CENTER, 0.0f, R_IN - 2.0f * K, R_OUT + 2.0f * K, 1.0f * K, 0x7BEF);

  // 历史最大左/右倾角保持标记线
  if (LeanAngleMeter::maxLeftLean >= 1.5f) {
    float maxL = LeanAngleMeter::maxLeftLean;
    if (maxL > 88.0f) maxL = 88.0f;
    uint16_t lCol = (maxL > 50.0f) ? HUD_COLOR_RED : HUD_COLOR_AMBER;
    drawLeanRadialMarker(CX_CENTER, CY_CENTER, -maxL, R_IN - 4.0f * K, R_OUT + 3.0f * K, 1.4f * K, lCol);
  }
  if (LeanAngleMeter::maxRightLean >= 1.5f) {
    float maxR = LeanAngleMeter::maxRightLean;
    if (maxR > 88.0f) maxR = 88.0f;
    uint16_t rCol = (maxR > 50.0f) ? HUD_COLOR_RED : HUD_COLOR_AMBER;
    drawLeanRadialMarker(CX_CENTER, CY_CENTER, maxR, R_IN - 4.0f * K, R_OUT + 3.0f * K, 1.4f * K, rCol);
  }

  // 当前倾斜角度指针（高亮白色尖端指针）
  {
    float pDeg = curLean;
    if (pDeg < -88.0f) pDeg = -88.0f;
    if (pDeg >  88.0f) pDeg =  88.0f;
    drawLeanRadialMarker(CX_CENTER, CY_CENTER, pDeg, R_IN - 7.0f * K, R_OUT + 3.0f * K, 2.2f * K, C_WHITE);
  }

  // 1.2 超大驾驶级倾角读数 (上半圆穹顶正中，已去掉 STABLE/LEAN L/LEAN R 文字)
  char leanNumStr[12];
  snprintf(leanNumStr, sizeof(leanNumStr), "%d*", (int)lroundf(absCurLean));
  uint16_t numColor = (absCurLean > 50.0f) ? HUD_COLOR_RED :
                      ((absCurLean > 35.0f) ? HUD_COLOR_AMBER : C_WHITE);
  drawAaString(CX_CENTER, CY_CENTER - 98.0f * K, leanNumStr, 8.5f * K, 4.6f * K, 50.0f * K, numColor, true);

  // =========================================================================
  // 2. 进一步放大的 G 值表 (无框、无蓝底、无左上角圆点、半透明纯蓝色圆圈指示)
  // =========================================================================
  const int GX = (int)lroundf(CX_CENTER - 96.0f * K);
  const int GY = (int)lroundf(CY_CENTER + 20.0f * K);
  const float G_SCALE_R = 78.0f * K;

  gfx->drawCircle(GX, GY, (int)G_SCALE_R,          0x294A); // 1.5G
  gfx->drawCircle(GX, GY, (int)(G_SCALE_R * 0.67f), 0x4208); // 1.0G
  gfx->drawCircle(GX, GY, (int)(G_SCALE_R * 0.33f), 0x294A); // 0.5G
  gfx->drawLine(GX - (int)(12 * K), GY, GX + (int)(12 * K), GY, 0x4208);
  gfx->drawLine(GX, GY - (int)(12 * K), GX, GY + (int)(12 * K), 0x4208);

  const float PI2 = 6.2831853f;
  int pxMax[16], pyMax[16];
  int pxRec[16], pyRec[16];

  for (int i = 0; i < 16; i++) {
    float secAng = (float)i * (PI2 / 16.0f);
    float cosS = cosf(secAng), sinS = sinf(secAng);

    float rMax = (LeanAngleMeter::maxEnvelope[i] / 1.5f) * G_SCALE_R;
    if (rMax > G_SCALE_R) rMax = G_SCALE_R;
    if (rMax < 9.0f * K) rMax = 9.0f * K;
    pxMax[i] = (int)lroundf((float)GX + rMax * cosS);
    pyMax[i] = (int)lroundf((float)GY + rMax * sinS);

    float rRec = (LeanAngleMeter::recentEnvelope[i] / 1.5f) * G_SCALE_R;
    if (rRec > G_SCALE_R) rRec = G_SCALE_R;
    if (rRec < 9.0f * K) rRec = 9.0f * K;
    pxRec[i] = (int)lroundf((float)GX + rRec * cosS);
    pyRec[i] = (int)lroundf((float)GY + rRec * sinS);
  }

  for (int i = 0; i < 16; i++) {
    int nextI = (i + 1) % 16;
    gfx->fillTriangle(GX, GY, pxMax[i], pyMax[i], pxMax[nextI], pyMax[nextI], HUD_COLOR_G_POLY1);
  }
  for (int i = 0; i < 16; i++) {
    int nextI = (i + 1) % 16;
    gfx->drawLine(pxMax[i], pyMax[i], pxMax[nextI], pyMax[nextI], HUD_COLOR_G_LINE1);
  }

  for (int i = 0; i < 16; i++) {
    int nextI = (i + 1) % 16;
    gfx->fillTriangle(GX, GY, pxRec[i], pyRec[i], pxRec[nextI], pyRec[nextI], HUD_COLOR_G_POLY2);
  }
  for (int i = 0; i < 16; i++) {
    int nextI = (i + 1) % 16;
    gfx->drawLine(pxRec[i], pyRec[i], pxRec[nextI], pyRec[nextI], HUD_COLOR_G_LINE2);
  }

  // 半透明纯蓝色实心圆圈指示实时 G 值向量（无橙色光晕、无渐变）
  float normLong = LeanAngleMeter::gLongitudinal / 1.5f;
  float normLat  = LeanAngleMeter::gLateral / 1.5f;
  float gdx = normLat * G_SCALE_R;
  float gdy = -normLong * G_SCALE_R;
  float gDist = sqrtf(gdx * gdx + gdy * gdy);
  if (gDist > G_SCALE_R) {
    gdx = (gdx / gDist) * G_SCALE_R;
    gdy = (gdy / gDist) * G_SCALE_R;
  }
  drawTranslucentDisc((float)GX + gdx, (float)GY + gdy, 9.5f * K, HUD_COLOR_BLUE_G, 180);

  // 放大版底部实时 G 值读数
  float targetG = LeanAngleMeter::gTotal;
  if (fabsf(targetG - dispG) > 0.015f) dispG += 0.12f * (targetG - dispG);
  if (dispG < 0.02f) dispG = 0.0f;
  char gNumBuf[16];
  snprintf(gNumBuf, sizeof(gNumBuf), "%.2fG", dispG);
  drawAaString((float)GX, (float)GY + G_SCALE_R + 22.0f * K, gNumBuf, 2.80f * K, 1.55f * K, 17.5f * K, C_WHITE, true);

  // =========================================================================
  // 3. 右侧数据列表 (向右移动以缩小标签与值间距，同时避让放大版 G 值表)
  // =========================================================================
  const float LIST_LEFT  = CX_CENTER + 24.0f * K;
  const float LIST_RIGHT = CX_CENTER + 178.0f * K;
  const float LIST_Y0    = CY_CENTER - 14.0f * K;
  const float ROW_GAP    = 44.0f * K;

  // 第 1 行：ALT
  char altValBuf[16];
  if (altValid) snprintf(altValBuf, sizeof(altValBuf), "%dm", altitudeM);
  else snprintf(altValBuf, sizeof(altValBuf), "--m");
  drawAaString(LIST_LEFT, LIST_Y0, "ALT", 2.00f * K, 1.15f * K, 12.5f * K, HUD_COLOR_TEXT_DIM, false);
  drawAaStringRight(LIST_RIGHT, LIST_Y0, altValBuf, 3.00f * K, 1.65f * K, 18.0f * K, C_WHITE);

  gfx->drawLine((int)LIST_LEFT, (int)(LIST_Y0 + 22.0f * K), (int)LIST_RIGHT, (int)(LIST_Y0 + 22.0f * K), 0x2104);

  // 第 2 行：UP
  char climbBuf[16];
  if (altValid) snprintf(climbBuf, sizeof(climbBuf), "+%dm", climbAccumM);
  else snprintf(climbBuf, sizeof(climbBuf), "+0m");
  float yRow2 = LIST_Y0 + ROW_GAP;
  drawAaString(LIST_LEFT, yRow2, "UP", 2.00f * K, 1.15f * K, 12.5f * K, HUD_COLOR_TEXT_DIM, false);
  drawAaStringRight(LIST_RIGHT, yRow2, climbBuf, 3.00f * K, 1.65f * K, 18.0f * K, 0x34EA);

  gfx->drawLine((int)LIST_LEFT, (int)(yRow2 + 22.0f * K), (int)LIST_RIGHT, (int)(yRow2 + 22.0f * K), 0x2104);

  // 第 3 行：SLP
  char slpValBuf[16];
  if (altValid) {
    if (slopePct > 0) snprintf(slpValBuf, sizeof(slpValBuf), "+%d%%", slopePct);
    else snprintf(slpValBuf, sizeof(slpValBuf), "%d%%", slopePct);
  } else snprintf(slpValBuf, sizeof(slpValBuf), "--%%");
  float yRow3 = LIST_Y0 + ROW_GAP * 2.0f;
  uint16_t slpColor = (altValid && abs(slopePct) >= 5) ? HUD_COLOR_AMBER : HUD_COLOR_CYAN;
  drawAaString(LIST_LEFT, yRow3, "SLP", 2.00f * K, 1.15f * K, 12.5f * K, HUD_COLOR_TEXT_DIM, false);
  drawAaStringRight(LIST_RIGHT, yRow3, slpValBuf, 3.00f * K, 1.65f * K, 18.0f * K, slpColor);

  // =========================================================================
  // 4. 底部统计胶囊与操作提示（宽度足够，文本绝不越界）
  // =========================================================================
  const int BOT_Y = (int)lroundf(LCD_HEIGHT - 88.0f * K);
  const int BOT_W = (int)lroundf(268.0f * K);
  const int BOT_H = (int)lroundf(32.0f * K);
  const int BOT_X = (int)CX_CENTER - BOT_W / 2;

  bool justCalib = millis() < LeanAngleMeter::calibSuccessUntil;
  if (justCalib) {
    drawTranslucentRoundRect(BOT_X, BOT_Y, BOT_W, BOT_H, (int)(16 * K), HUD_COLOR_GREEN, 230);
    drawAaString(CX_CENTER, BOT_Y + BOT_H * 0.5f, "CALIBRATED OK", 2.00f * K, 1.15f * K, 12.5f * K, C_BLACK, true);
  } else {
    drawTranslucentRoundRect(BOT_X, BOT_Y, BOT_W, BOT_H, (int)(16 * K), 0x0821, 180);
    gfx->drawRoundRect(BOT_X, BOT_Y, BOT_W, BOT_H, (int)(16 * K), 0x296B);

    char statBuf[32];
    snprintf(statBuf, sizeof(statBuf), "L%d* R%d* %.2fG",
             (int)lroundf(LeanAngleMeter::maxLeftLean),
             (int)lroundf(LeanAngleMeter::maxRightLean),
             LeanAngleMeter::maxG);
    drawAaString(CX_CENTER, BOT_Y + BOT_H * 0.5f, statBuf, 1.90f * K, 1.10f * K, 12.0f * K, C_WHITE, true);
  }

  drawAaString(CX_CENTER, LCD_HEIGHT - 42.0f * K, "HOLD TO CALIB & RESET", 1.45f * K, 0.85f * K, 9.0f * K, 0x528A, true);

  drawAlongPoiBanner();
  drawPageDots();
  needFlush = true;
}

void leanMeterTap(int x, int y) {
  (void)x;
  (void)y;
}
