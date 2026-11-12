#pragma once
#include <Arduino.h>

void resetNavState();

#ifndef ST_VICE
#define ST_VICE 1
#endif
#ifndef ST_NEON
#define ST_NEON 3
#endif
#ifndef ST_DRAGON
#define ST_DRAGON 11
#endif

#define FX_DEST_MS 300000UL    // 导航结束结算界面：停留显示（5分钟防跑电安全保护超时）
#define FX_WP_MS   3000        // 途经点短提示：3秒自动退回
#define CCY 233                // 屏幕中心 y（圆屏 466×466）

bool fxOn = false, fxWp = false;
unsigned long fxAt = 0, fxDur = 0;
bool touchEat = false;         // 点掉特效的那一下：不当成滑动换页
long tripM = -1; int tripMin = -1, tripMaxKmh = -1; unsigned long tripAt = 0;   // "#O" 行程统计

static inline void drawFxRing(int cx, int cy, int r, int w, uint16_t c) {
  for (int k = 0; k < w; k++) gfx->drawCircle(cx, cy, r + k, c);
}

void startArriveFx(bool wp) {
  fxOn = true; fxWp = wp;
  fxAt = millis();
  fxDur = wp ? FX_WP_MS : FX_DEST_MS;
}

void stopArriveFx() {
  if (fxOn) {
    bool wasDest = !fxWp;
    fxOn = false;
    if (wasDest) {
      resetNavState();
      progReset();
    }
  }
}

// 格式化行程统计数据 (如 "12.8 km", "28 min", "72 km/h")
static inline void getTripStrings(char *distBuf, size_t dSz, char *timeBuf, size_t tSz, char *topBuf, size_t mSz) {
  if (tripM >= 1000) {
    snprintf(distBuf, dSz, "%.1f km", tripM / 1000.0f);
  } else if (tripM >= 0) {
    snprintf(distBuf, dSz, "%ld m", tripM);
  } else {
    snprintf(distBuf, dSz, "--");
  }

  if (tripMin > 0) {
    snprintf(timeBuf, tSz, "%d min", tripMin);
  } else if (tripMin == 0) {
    snprintf(timeBuf, tSz, "<1 min");
  } else {
    snprintf(timeBuf, tSz, "--");
  }

  if (tripMaxKmh > 0) {
    snprintf(topBuf, mSz, "%d km/h", tripMaxKmh);
  } else {
    snprintf(topBuf, mSz, "--");
  }
}

// 绘制通用现代行程结算数据卡片 (3 个半透明胶囊，分别放置 里程、用时、极速)
static void drawTripStatCards(int startY, uint16_t cardBg, uint16_t borderCol, uint16_t labelCol, uint16_t valCol) {
  char dStr[16], tStr[16], sStr[16];
  getTripStrings(dStr, sizeof(dStr), tStr, sizeof(tStr), sStr, sizeof(sStr));

  const int cW = 116;
  const int cH = 66;
  const int gap = 12;
  const int totalW = 3 * cW + 2 * gap; // 372
  const int startX = CX - totalW / 2;  // 47

  struct StatCard {
    const char *lblZh;
    const char *lblEn;
    const char *val;
  } cards[3] = {
    {"骑行里程", "DISTANCE", dStr},
    {"骑行用时", "DURATION", tStr},
    {"最高车速", "TOP SPEED", sStr}
  };

  for (int i = 0; i < 3; i++) {
    int cx = startX + i * (cW + gap);
    drawTranslucentRoundRect(cx, startY, cW, cH, 14, cardBg, 220);
    gfx->drawRoundRect(cx, startY, cW, cH, 14, borderCol);

    // 卡片标签
    if (isChineseLang) {
      drawTextUTF8Center(cx + cW / 2, startY + 22, cards[i].lblZh, labelCol);
    } else {
      drawAaString((float)(cx + cW / 2), (float)(startY + 18), cards[i].lblEn, 1.2f, 0.75f, 8.5f, labelCol, true);
    }

    // 卡片数值
    drawAaString((float)(cx + cW / 2), (float)(startY + 48), cards[i].val, 1.7f, 1.0f, 11.5f, valCol, true);
  }
}

// 1. 龙珠雷达专属结算画面 (Dragon Radar Settlement)
static void drawDragonRadarSettlement(unsigned long t) {
  // 暗绿军工雷达格栅底色
  gfx->fillScreen(0x0101);

  // 绘制同心雷达环与十字刻度线
  drawFxRing(CX, CCY, 80, 1, 0x12A2);
  drawFxRing(CX, CCY, 150, 1, 0x1B03);
  drawFxRing(CX, CCY, 215, 2, 0x2BC5);

  gfx->drawFastHLine(CX - 210, CCY, 420, 0x12A2);
  gfx->drawFastVLine(CX, CCY - 210, 420, 0x12A2);

  // 中心龙珠位置 (CX, 142)
  const int ballX = CX;
  const int ballY = 142;

  // 龙珠外圈柔和光波呼吸扩散
  float pulse = 0.5f + 0.5f * sinf(t * 0.005f);
  int waveR = (int)(48.0f + 14.0f * pulse);
  drawFxRing(ballX, ballY, waveR, 2, blend565(0xFDC0, 0x0101, (int)(pulse * 180)));

  // 四星龙珠晶体球体 (亮澄澄金黄琥珀色)
  aaDisc((float)ballX, (float)ballY, 40.0f, 0xF940); // 暖红橙外边缘
  aaDisc((float)ballX, (float)ballY, 37.0f, 0xFD60); // 晶莹橙黄
  aaDisc((float)ballX, (float)ballY, 32.0f, 0xFE40); // 龙珠金黄内芯

  // 左上方高光晶莹反光点
  aaDisc((float)(ballX - 14), (float)(ballY - 14), 7.0f, C_WHITE);
  aaDisc((float)(ballX - 8),  (float)(ballY - 20), 3.5f, C_WHITE);

  // 龙珠中心四颗红星 (经典 4-Star Dragon Ball)
  const float sRadOut = 4.2f;
  const float sRadIn  = 1.9f;
  drawRockstarStar((float)ballX,        (float)(ballY - 12), sRadOut, sRadIn, 0xD800, 0xD800);
  drawRockstarStar((float)ballX,        (float)(ballY + 12), sRadOut, sRadIn, 0xD800, 0xD800);
  drawRockstarStar((float)(ballX - 12), (float)ballY,        sRadOut, sRadIn, 0xD800, 0xD800);
  drawRockstarStar((float)(ballX + 12), (float)ballY,        sRadOut, sRadIn, 0xD800, 0xD800);

  // 龙珠雷达专属标题
  if (isChineseLang) {
    drawTextUTF8Center(CX, 218, "搜寻完成 · 龙珠获取", 0xFFE0);
    drawTextUTF8Center(CX, 244, "已到达目的地坐标", 0x56E9);
  } else {
    drawAaString((float)CX, 212.0f, "TARGET ACQUIRED", 2.0f, 1.15f, 13.5f, 0xFFE0, true);
    drawAaString((float)CX, 238.0f, "DRAGON RADAR SYSTEM", 1.4f, 0.85f, 9.5f, 0x56E9, true);
  }

  // 结算卡片 (深绿底、翠绿边、暖黄数)
  drawTripStatCards(272, 0x0942, 0x2BC5, 0x7E31, 0xFFE0);

  // 底部返回提示
  float exitBlink = 0.5f + 0.5f * sinf(t * 0.006f);
  uint16_t tipCol = blend565(0x2385, 0x8633, (int)(exitBlink * 220));
  if (isChineseLang) {
    drawTextUTF8Center(CX, 388, "轻触屏幕完成搜寻", tipCol);
  } else {
    drawAaString((float)CX, 382.0f, "TAP SCREEN TO RETURN", 1.3f, 0.8f, 9.0f, tipCol, true);
  }
}

// 2. 罪恶都市专属结算画面 (Vice City Settlement)
static void drawViceCitySettlement(unsigned long t) {
  // 深紫黑夜空底色
  gfx->fillScreen(0x0802);
  drawThemedMapBackground();

  // 中心终点圆环 (CX, 142)
  const int badgeX = CX;
  const int badgeY = 142;

  // 迈阿密霓虹外环脉冲
  float pulse = 0.5f + 0.5f * sinf(t * 0.005f);
  int waveR = (int)(44.0f + 6.0f * pulse);
  drawFxRing(badgeX, badgeY, waveR, 2, blend565(0xF973, 0x0802, (int)(pulse * 170)));

  // 黑白格赛车终点徽章底盘
  aaDisc((float)badgeX, (float)badgeY, 38.0f, 0x10A2);
  drawFxRing(badgeX, badgeY, 38, 2, C_CYAN);

  // 内部精致方格旗纹样 (6x6 黑白交错格)
  const int bSize = 6;
  const int gridLeft = badgeX - 18;
  const int gridTop  = badgeY - 18;
  for (int r = 0; r < 6; r++) {
    for (int c = 0; c < 6; c++) {
      uint16_t col = ((r + c) & 1) ? 0x18C3 : C_WHITE;
      gfx->fillRect(gridLeft + c * bSize, gridTop + r * bSize, bSize, bSize, col);
    }
  }

  // 罪恶都市粉色/青色标题
  if (isChineseLang) {
    drawTextUTF8Center(CX, 218, "顺利到达目的地", C_WHITE);
    drawTextUTF8Center(CX, 244, "骑行结束 · 行程结算", 0xF973);
  } else {
    drawAaString((float)CX, 212.0f, "MISSION PASSED", 2.2f, 1.25f, 15.0f, 0xF973, true);
    drawAaString((float)CX, 238.0f, "DESTINATION REACHED", 1.4f, 0.85f, 9.5f, C_CYAN, true);
  }

  // 结算卡片 (暗紫底、霓虹青/紫边、荧光白数)
  drawTripStatCards(272, 0x18C3, 0x39E9, 0x9BD7, C_WHITE);

  // 底部返回提示
  float exitBlink = 0.5f + 0.5f * sinf(t * 0.006f);
  uint16_t tipCol = blend565(0x4A49, C_WHITE, (int)(exitBlink * 180));
  if (isChineseLang) {
    drawTextUTF8Center(CX, 388, "轻触屏幕返回待机主页", tipCol);
  } else {
    drawAaString((float)CX, 382.0f, "TAP SCREEN TO RETURN", 1.3f, 0.8f, 9.0f, tipCol, true);
  }
}

// 3. 极简黑白 & 经典原版结算画面 (Minimalist & Classic Settlement)
static void drawMinimalSettlement(unsigned long t) {
  gfx->fillScreen(C_BLACK);

  // 中心极简终点标志 (CX, 142)
  const int badgeX = CX;
  const int badgeY = 142;

  aaDisc((float)badgeX, (float)badgeY, 36.0f, 0x18C3);
  drawFxRing(badgeX, badgeY, 36, 2, C_WHITE);

  // 内部方格旗图形
  const int bSize = 6;
  const int gridLeft = badgeX - 18;
  const int gridTop  = badgeY - 18;
  for (int r = 0; r < 6; r++) {
    for (int c = 0; c < 6; c++) {
      uint16_t col = ((r + c) & 1) ? 0x2945 : C_WHITE;
      gfx->fillRect(gridLeft + c * bSize, gridTop + r * bSize, bSize, bSize, col);
    }
  }

  // 极简冷灰与钛银文字
  if (isChineseLang) {
    drawTextUTF8Center(CX, 218, "已到达目的地", C_WHITE);
    drawTextUTF8Center(CX, 244, "骑行行程结算", 0x9CD3);
  } else {
    drawAaString((float)CX, 212.0f, "ARRIVED", 2.2f, 1.25f, 15.0f, C_WHITE, true);
    drawAaString((float)CX, 238.0f, "TRIP SUMMARY", 1.4f, 0.85f, 9.5f, 0x9CD3, true);
  }

  // 结算卡片 (黑曜石底、钛白细边、纯白数)
  drawTripStatCards(272, 0x0821, 0x31A6, 0x7BEF, C_WHITE);

  // 底部返回提示
  float exitBlink = 0.5f + 0.5f * sinf(t * 0.006f);
  uint16_t tipCol = blend565(0x31A6, 0x9CD3, (int)(exitBlink * 180));
  if (isChineseLang) {
    drawTextUTF8Center(CX, 388, "轻触屏幕返回待机主页", tipCol);
  } else {
    drawAaString((float)CX, 382.0f, "TAP SCREEN TO RETURN", 1.3f, 0.8f, 9.0f, tipCol, true);
  }
}

// 4. 途经点到达非打扰 HUD 悬浮胶囊 (Waypoint Banner)
static void drawWaypointHud(unsigned long t) {
  const int hudW = 240;
  const int hudH = 52;
  const int hudX = CX - hudW / 2;
  const int hudY = 70;

  drawTranslucentRoundRect(hudX, hudY, hudW, hudH, 26, 0x0000, 230);
  gfx->drawRoundRect(hudX, hudY, hudW, hudH, 26, C_CYAN);

  // 左侧途经点图标
  aaDisc((float)(hudX + 26), (float)(hudY + hudH / 2), 6.0f, C_CYAN);
  aaDisc((float)(hudX + 26), (float)(hudY + hudH / 2), 2.5f, C_BLACK);

  // 文字
  if (isChineseLang) {
    drawTextUTF8(hudX + 44, hudY + 32, "已途经导航点", C_WHITE);
  } else {
    drawAaString((float)(hudX + 130), (float)(hudY + hudH / 2), "WAYPOINT PASSED", 1.4f, 0.85f, 9.5f, C_WHITE, true);
  }
}

void drawArriveFx(unsigned long t) {
  onWaitScreen = false;
  frameIsMap = false;

  // 途经点提示：非打扰短消息
  if (fxWp) {
    drawWaypointHud(t);
    needFlush = true;
    return;
  }

  // 目的地到达结算：根据当前激活的主题风格选择专属结算画面
  int st = styleIdx();
  if (st == ST_DRAGON) {
    drawDragonRadarSettlement(t);
  } else if (st == ST_VICE || st == ST_NEON) {
    drawViceCitySettlement(t);
  } else {
    drawMinimalSettlement(t);
  }

  needFlush = true;
}
