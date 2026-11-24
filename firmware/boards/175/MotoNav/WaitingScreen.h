#pragma once
#include <Arduino.h>
#include <time.h>
#include "UiModern.h"
#include "DestHistory.h"

// 绘制中文/多语言与 ASCII 混排文本 (基于 u8g2 unifont 汉字字模库)
static inline int getUTF8Width(const char *s) {
  if (!s) return 0;
  int w = 0;
  while (*s) {
    if ((uint8_t)*s < 0x80) {
      w += 8;
      s++;
    } else if ((uint8_t)*s < 0xE0) {
      w += 16;
      s += 2;
    } else if ((uint8_t)*s < 0xF0) {
      w += 16;
      s += 3;
    } else {
      w += 16;
      s += 4;
    }
  }
  return w;
}

static inline void drawTextUTF8(int x, int y, const char *str, uint16_t color) {
  if (!str || !str[0]) return;
  #if defined(U8G2_FONT_SUPPORT)
  gfx->setFont(u8g2_font_unifont_h_chinese4);
  gfx->setUTF8Print(true);
  gfx->setTextSize(1);
  gfx->setTextColor(color);
  gfx->setCursor(x, y);
  gfx->print(str);
  gfx->setFont((const GFXfont*)nullptr);
  #else
  drawAaString((float)x, (float)y, str, 1.6f, 0.9f, 11.0f, color, false);
  #endif
}

static inline void drawTextUTF8Center(int cx, int y, const char *str, uint16_t color) {
  int w = getUTF8Width(str);
  drawTextUTF8(cx - w / 2, y, str, color);
}

static inline void drawTextUTF8Bold(int x, int y, const char *str, uint16_t color) {
  drawTextUTF8(x, y, str, color);
  drawTextUTF8(x + 1, y, str, color);
}

static inline void drawTextUTF8BoldCenter(int cx, int y, const char *str, uint16_t color) {
  int w = getUTF8Width(str);
  drawTextUTF8Bold(cx - w / 2, y, str, color);
}

// UTF-8 安全自动回行绘制（最多 2 行，绝不截断多字节汉字）
static inline void drawTextUTF8Wrap2(int x, int y, int maxW, int lineH, const char *str, uint16_t color) {
  if (!str || !str[0]) return;
  char line1[96], line2[96];
  int i = 0, w1 = 0, l1 = 0;
  while (str[i]) {
    uint8_t c = (uint8_t)str[i];
    int clen = (c < 0x80) ? 1 : (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
    int cw = (c < 0x80) ? 8 : 16;
    if (w1 + cw > maxW && l1 > 0) break;
    if (l1 + clen >= (int)sizeof(line1) - 1) break;
    for (int k = 0; k < clen && str[i]; k++) line1[l1++] = str[i++];
    w1 += cw;
  }
  line1[l1] = '\0';
  drawTextUTF8(x, y, line1, color);

  if (!str[i]) return;
  int w2 = 0, l2 = 0;
  while (str[i]) {
    uint8_t c = (uint8_t)str[i];
    int clen = (c < 0x80) ? 1 : (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
    int cw = (c < 0x80) ? 8 : 16;
    if (w2 + cw > maxW - 12 && l2 > 0) {
      if (l2 + 3 < (int)sizeof(line2) - 1) { line2[l2++] = '.'; line2[l2++] = '.'; line2[l2++] = '.'; }
      break;
    }
    if (l2 + clen >= (int)sizeof(line2) - 1) break;
    for (int k = 0; k < clen && str[i]; k++) line2[l2++] = str[i++];
    w2 += cw;
  }
  line2[l2] = '\0';
  if (l2 > 0) drawTextUTF8(x, y + lineH, line2, color);
}

// 全局变量定义
DestItem destItems[MAX_DEST_ITEMS];
int destItemCount = 0;
bool destSheetOpen = false;
bool destBgSynced = false;
int destScrollOffset = 0;

bool onWaitScreen = false;
bool sliding = false;
int waitSt = 0;

// 页面分配：
// 0 = 导航 / 待机主页
// 1 = 圆屏设置 (SettingsPage)
// 2 = 音乐播放 (MusicPage)
// 3 = 倾角与 G 值仪表 (PageLeanMeter)
// 4 = 伴侣模式 (CompanionMode)
int page = 0;
unsigned long pageAt = 0;
bool leanDirty = false;
bool settingsDirty = false;

void drawLeanMeter();
void leanMeterTap(int x, int y);
void drawSettingsPage();
void settingsPageTap(int x, int y);
void drawMusic();
void musicTap(int x, int y);

// 绘制页面位置指示点 (圆屏底部/顶部现代微光圆点)
void drawPageDots() {
  if (page == 0 && inNav) return;
  // 横向三页：音乐在左(2)，主页居中(0)，设置在右(1)
  const int pageOrder[] = {2, 0, 1};
  for (int i = 0; i < 3; i++) {
    aaDisc(CX + (i - 1) * 18, 436, 3.5f, page == pageOrder[i] ? C_WHITE : 0x31A6);
  }
  // 底部第4点：倾角与G值仪表在下方(3)
  aaDisc(CX, 451, 3.5f, page == 3 ? C_WHITE : 0x31A6);
  // 顶部第5点：伴侣模式在上方(4)
  aaDisc(CX, 10, 3.5f, page == 4 ? C_WHITE : 0x31A6);
}

// 格式化月份缩写
static inline const char* getMonthStr(int m) {
  static const char* const months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
  if (m >= 1 && m <= 12) return months[m - 1];
  return "NOV";
}

// 格式化星期缩写
static inline const char* getWdayStr(int w) {
  static const char* const wdays[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  if (w >= 0 && w <= 6) return wdays[w];
  return "TUE";
}

// 绘制快捷沿途/周边服务矢量图标（放大约 1.35x，清晰易辨）
static void drawQuickPoiIcon(int cx, int cy, int kind, uint16_t col) {
  if (kind == 0) {
    // 加油站：放大油机机身 + 屏幕 + 油管
    gfx->fillRoundRect(cx - 10, cy - 11, 15, 21, 3, col);
    gfx->fillRect(cx - 7, cy - 8, 9, 7, C_BLACK);
    gfx->drawLine(cx + 5, cy - 7, cx + 11, cy - 2, col);
    gfx->drawLine(cx + 6, cy - 7, cx + 12, cy - 2, col);
    gfx->drawLine(cx + 11, cy - 2, cx + 11, cy + 7, col);
    gfx->drawLine(cx + 12, cy - 2, cx + 12, cy + 7, col);
    gfx->drawLine(cx + 8, cy + 7, cx + 11, cy + 7, col);
  } else if (kind == 1) {
    // 服务区：放大屋顶 + P 徽标
    for (int d = 0; d <= 1; d++) {
      gfx->drawLine(cx - 13, cy - 1 + d, cx, cy - 12 + d, col);
      gfx->drawLine(cx, cy - 12 + d, cx + 13, cy - 1 + d, col);
      gfx->drawLine(cx - 9 + d, cy - 1, cx - 9 + d, cy + 10, col);
      gfx->drawLine(cx + 9 - d, cy - 1, cx + 9 - d, cy + 10, col);
    }
    smoothDrawText(SF16, "P", cx, cy + 4, C_WHITE, 1, 1, false);
  } else if (kind == 2) {
    // 维修点：放大交叉扳手
    for (int d = -1; d <= 1; d++) {
      gfx->drawLine(cx - 9 + d, cy - 9, cx + 9 + d, cy + 9, col);
      gfx->drawLine(cx + 9 + d, cy - 9, cx - 9 + d, cy + 9, col);
    }
    aaDisc(cx - 8, cy - 8, 4.0f, col);
    aaDisc(cx + 8, cy + 8, 4.0f, col);
  } else {
    // 洗手间：放大 WC 徽标
    smoothDrawText(SF22, "WC", cx, cy, col, 1, 1, false);
  }
}

// 导航/巡航态轻触屏幕空白处浮现的 9 点方向主题收藏按钮
void drawNavStarButton(unsigned long now) {
  if (!inNav || destSheetOpen || routePickerOpen || callOn || pairOn || arrivedBannerOn) return;
  if (now >= navStarUntil) return;
  const int btnCX = 52, btnCY = CX;
  drawThemedFavIcon(btnCX, btnCY, 30, false);
}

static inline int destMaxScrollPx() {
  const int itemH = 76, itemGap = 8, viewH = 246;
  int totalH = (destItemCount > 0) ? (destItemCount * (itemH + itemGap) - itemGap) : 0;
  return max(0, totalH - viewH);
}

// 绘制地点收藏与沿途快捷服务抽屉浮层（实心高速渲染，外侧顶部居中关闭按钮，一页显示 3 条收藏）
void drawDestinationsSheet() {
  const int sheetW = 388;
  const int sheetH = 374;
  const int sheetX = CX - sheetW / 2; // 39
  const int sheetY = 60;              // 顶部留出 60px 给外侧关闭按钮
  const int listTop = sheetY + 92;    // 152
  const int listBot = sheetY + sheetH - 36; // 398
  const int listViewH = listBot - listTop;  // 246（正好容纳 3 条 76px + 2×8px 间距 = 244px）

  destInitDefaults();
  int maxScr = destMaxScrollPx();
  destScrollOffset = constrain(destScrollOffset, 0, maxScr);

  // 0. 卡片外侧正上方居中关闭按钮（半径 20，独立浮于卡片上方，放大点击热区）
  const int closeX = CX;
  const int closeY = 30;
  gfx->fillCircle(closeX, closeY, 20, 0x2124);
  gfx->drawCircle(closeX, closeY, 20, 0x8410);
  for (int d = -1; d <= 1; d++) {
    gfx->drawLine(closeX - 7 + d, closeY - 7, closeX + 7 + d, closeY + 7, C_WHITE);
    gfx->drawLine(closeX + 7 + d, closeY - 7, closeX - 7 + d, closeY + 7, C_WHITE);
  }

  // 1. 实心深色圆角底座（消除 14 万次 PSRAM 半透明读改写）
  gfx->fillRoundRect(sheetX, sheetY, sheetW, sheetH, 22, 0x0841);

  // 2. 可滚动常用收藏地点列表（先画列表，再由顶部标题栏与底部提示栏自然裁切遮罩，零溢出）
  const int itemW = sheetW - 32; // 356 (右侧留出滚动条空间)
  const int itemH = 76;
  const int itemGap = 8;
  const int itemX = sheetX + 14;

  for (int i = 0; i < destItemCount; i++) {
    int curY = listTop + i * (itemH + itemGap) - destScrollOffset;
    if (curY + itemH <= listTop || curY >= listBot) continue;

    // 预混合深灰卡片底色（零 PSRAM 读取混合，提速 10x）
    gfx->fillRoundRect(itemX, curY, itemW, itemH, 14, 0x1082);
    gfx->drawRoundRect(itemX, curY, itemW, itemH, 14, 0x31A6);

    // 左侧图标
    if (destItems[i].type == 0) {
      drawRockstarStar(itemX + 26, curY + itemH / 2, 11.5f, 4.8f, 0xFDE0, 0xFDE0, 0.4f);
    } else {
      aaDisc(itemX + 26, curY + itemH / 2 - 4, 7.5f, C_CYAN);
      gfx->fillTriangle(itemX + 19, curY + itemH / 2 - 2,
                        itemX + 33, curY + itemH / 2 - 2,
                        itemX + 26, curY + itemH / 2 + 11, C_CYAN);
      aaDisc(itemX + 26, curY + itemH / 2 - 4, 3.0f, C_BLACK);
    }

    // 右侧路况耗时徽章或箭头
    bool hasCommute = (destItems[i].commuteMin > 0);
    int textMaxW = hasCommute ? (itemW - 56 - 76) : (itemW - 56 - 28);

    const char *name = destItems[i].name[0] ? destItems[i].name : (isChineseLang ? "未知地点" : "Unknown Location");
    const char *addr = destItems[i].addr[0] ? destItems[i].addr : (isChineseLang ? "轻触规划路线" : "Tap to route");
    drawTextUTF8Bold(itemX + 52, curY + 23, name, C_WHITE);
    drawTextUTF8Wrap2(itemX + 52, curY + 43, textMaxW, 18, addr, 0x9CF3);

    if (hasCommute) {
      char tcBuf[16];
      snprintf(tcBuf, sizeof(tcBuf), "%dm", destItems[i].commuteMin);
      uint16_t tcCol = (destItems[i].trafficStatus >= 2) ? 0xF9A4 : (destItems[i].trafficStatus == 1) ? 0xFDE0 : 0x05E0;
      uint16_t tcBg  = blend565(0x1082, tcCol, 150);
      gfx->fillRoundRect(itemX + itemW - 70, curY + (itemH - 32) / 2, 60, 32, 10, tcBg);
      gfx->drawRoundRect(itemX + itemW - 70, curY + (itemH - 32) / 2, 60, 32, 10, tcCol);
      smoothDrawText(SF22, tcBuf, itemX + itemW - 40, curY + itemH / 2, C_WHITE, 1, 1, false);
    } else {
      smoothDrawText(SF22, ">", itemX + itemW - 18, curY + itemH / 2, 0x6B4D, 1, 1, false);
    }
  }

  // 右侧纤细滚动条指示器（超出可视高度时显示）
  if (maxScr > 0) {
    int trackX = sheetX + sheetW - 10;
    int trackY = listTop + 4;
    int trackH = listViewH - 8;
    gfx->fillRoundRect(trackX, trackY, 4, trackH, 2, 0x2104);
    int totalH = destItemCount * (itemH + itemGap) - itemGap;
    int thumbH = max(24, trackH * listViewH / max(1, totalH));
    int thumbY = trackY + (trackH - thumbH) * destScrollOffset / maxScr;
    gfx->fillRoundRect(trackX, thumbY, 4, thumbH, 2, 0x9CF3);
  }

  // 3. 顶部不透明遮罩 + 紧凑单行标题栏 + 4 个快捷服务按钮（盖住滚动越界部分）
  gfx->fillRoundRect(sheetX, sheetY, sheetW, listTop - sheetY, 22, 0x0841);
  gfx->fillRect(sheetX, sheetY + 22, sheetW, listTop - sheetY - 22, 0x0841);

  // 紧凑单行标题（仅占 30px 高度，节省列表空间）
  if (isChineseLang) {
    drawTextUTF8BoldCenter(CX, sheetY + 23, "收藏与沿途查找", C_WHITE);
  } else {
    smoothDrawText(SF22, "FAVORITES & POI", CX, sheetY + 16, C_WHITE, 1, 1, false);
  }

  // 顶部 4 个沿途/周边快捷服务图标栏
  static const char *const qLabelsZh[4] = {"加油站", "服务区", "维修点", "洗手间"};
  static const char *const qLabelsEn[4] = {"GAS", "SVC", "REP", "WC"};
  static const uint16_t qCols[4] = {0xFA60, 0x06BF, 0xFDE0, 0x3E7F};
  const int qY = sheetY + 32;
  const int qW = 84;
  const int qH = 54;
  const int qGap = 8;
  const int qStartX = sheetX + 14;

  for (int k = 0; k < 4; k++) {
    int bx = qStartX + k * (qW + qGap);
    gfx->fillRoundRect(bx, qY, qW, qH, 12, 0x10A2);
    gfx->drawRoundRect(bx, qY, qW, qH, 12, qCols[k]);
    drawQuickPoiIcon(bx + qW / 2, qY + 18, k, qCols[k]);
    if (isChineseLang) {
      drawTextUTF8BoldCenter(bx + qW / 2, qY + 46, qLabelsZh[k], C_WHITE);
    } else {
      smoothDrawText(SF16, qLabelsEn[k], bx + qW / 2, qY + 42, C_WHITE, 1, 1, false);
    }
  }

  // 4. 底部不透明遮罩 + 下移后的精简提示（为 3 条收藏留足空间）
  gfx->fillRect(sheetX, listBot, sheetW, sheetY + sheetH - listBot - 22, 0x0841);
  gfx->fillRoundRect(sheetX, listBot, sheetW, sheetY + sheetH - listBot, 22, 0x0841);
  gfx->drawFastHLine(sheetX + 16, listBot, sheetW - 32, 0x2945);
  if (isChineseLang) {
    if (inNav) {
      drawTextUTF8Center(CX, listBot + 15, "快捷图标顺路加途经点", 0x9CF3);
      drawTextUTF8Center(CX, listBot + 31, "收藏地点一键换目的地", 0x7BEF);
    } else {
      drawTextUTF8Center(CX, listBot + 15, "轻触地点圆屏直接选路", 0x9CF3);
      drawTextUTF8Center(CX, listBot + 31, "上下滑动查看更多收藏", 0x7BEF);
    }
  } else {
    smoothDrawText(SF12, "TAP PLACE TO PLAN ROUTE", CX, listBot + 11, 0x9CF3, 1, 1, false);
    smoothDrawText(SF12, "SWIPE UP/DOWN TO SCROLL", CX, listBot + 26, 0x7BEF, 1, 1, false);
  }

  // 外框收边
  gfx->drawRoundRect(sheetX, sheetY, sheetW, sheetH, 22, 0x39E7);
}

// 绘制圆屏端路线选择浮层（选择候选路线并一键开始导航）
void drawRoutePickerSheet() {
  if (!routePickerOpen) return;
  unsigned long now = millis();
  if (routePickerErrUntil > 0 && now >= routePickerErrUntil) {
    routePickerOpen = false;
    routePickerErrUntil = 0;
    rpErrMsg[0] = 0;
    return;
  }

  const int sheetW = 384;
  const int sheetH = 356;
  const int sheetX = CX - sheetW / 2;
  const int sheetY = CX - sheetH / 2;

  drawTranslucentRoundRect(sheetX, sheetY, sheetW, sheetH, 22, 0x0000, 242);
  gfx->drawRoundRect(sheetX, sheetY, sheetW, sheetH, 22, C_CYAN);

  // 顶部交通工具胶囊 + 目的地名称
  const char *modeStr = (rpMode == 1) ? (isChineseLang ? "骑行" : "BIKE")
                      : (rpMode == 2) ? (isChineseLang ? "步行" : "WALK")
                                      : (isChineseLang ? "驾车" : "DRIVE");
  uint16_t modeCol = (rpMode == 1) ? 0x06BF : (rpMode == 2) ? 0xFDE0 : 0x3E7F;
  drawTranslucentRoundRect(sheetX + 16, sheetY + 14, 52, 24, 8, modeCol, 150);
  gfx->drawRoundRect(sheetX + 16, sheetY + 14, 52, 24, 8, modeCol);
  if (isChineseLang) {
    drawTextUTF8Center(sheetX + 42, sheetY + 31, modeStr, C_WHITE);
  } else {
    drawAaString(sheetX + 42, sheetY + 26, modeStr, 1.2f, 0.75f, 8.5f, C_WHITE, true);
  }

  drawTextUTF8(sheetX + 76, sheetY + 32, rpDestName[0] ? rpDestName : "目的地", C_WHITE);

  // 关闭按钮
  drawTranslucentCircle(sheetX + sheetW - 26, sheetY + 26, 14, 0x2124, 160);
  gfx->drawCircle(sheetX + sheetW - 26, sheetY + 26, 14, 0x4A69);
  gfx->drawLine(sheetX + sheetW - 30, sheetY + 22, sheetX + sheetW - 22, sheetY + 30, C_WHITE);
  gfx->drawLine(sheetX + sheetW - 22, sheetY + 22, sheetX + sheetW - 30, sheetY + 30, C_WHITE);

  gfx->drawLine(sheetX + 16, sheetY + 46, sheetX + sheetW - 16, sheetY + 46, 0x2945);

  if (routePickerLoading) {
    float pulse = 0.5f + 0.5f * sinf(now * 0.008f);
    aaDisc(CX, sheetY + 150, 8.0f + 4.0f * pulse, C_CYAN);
    char lbuf[64];
    snprintf(lbuf, sizeof(lbuf), "正在按%s模式规划路线...", modeStr);
    drawTextUTF8Center(CX, sheetY + 195, lbuf, C_WHITE);
    return;
  }

  if (rpErrMsg[0]) {
    drawTextUTF8Center(CX, sheetY + 170, rpErrMsg, 0xF9A4);
    return;
  }

  // 渲染 1~3 条候选路线卡片
  const int optW = sheetW - 28;
  const int optH = 62;
  const int optX = sheetX + 14;
  int optStartY = sheetY + 58;
  int cnt = (rpOptCount < MAX_ROUTE_PLAN_OPTS) ? rpOptCount : MAX_ROUTE_PLAN_OPTS;

  for (int i = 0; i < cnt; i++) {
    int cy = optStartY + i * (optH + 8);
    bool sel = (i == rpSelected);
    uint16_t bgCol = sel ? 0x09CE : 0x10A2;
    uint16_t bdCol = sel ? C_CYAN : 0x31A6;
    drawTranslucentRoundRect(optX, cy, optW, optH, 12, bgCol, sel ? 210 : 160);
    gfx->drawRoundRect(optX, cy, optW, optH, 12, bdCol);
    if (sel) gfx->drawRoundRect(optX + 1, cy + 1, optW - 2, optH - 2, 11, bdCol);

    // 左侧路况竖条 (绿/黄/红)
    uint16_t tCol = (rpOpts[i].trafficStatus >= 2) ? 0xF9A4 : (rpOpts[i].trafficStatus == 1) ? 0xFDE0 : 0x05E0;
    gfx->fillRoundRect(optX + 8, cy + 10, 5, optH - 20, 2, tCol);

    // 耗时 + 标签
    char minBuf[24];
    snprintf(minBuf, sizeof(minBuf), "%d min", rpOpts[i].mins);
    drawAaString(optX + 24, cy + 23, minBuf, 2.0f, 1.15f, 13.5f, C_WHITE, false);
    if (rpOpts[i].tag[0]) {
      drawTextUTF8(optX + 128, cy + 27, rpOpts[i].tag, sel ? C_CYAN : 0xAFBD);
    }

    // 里程 + 红绿灯
    char subBuf[48];
    if (rpOpts[i].lights > 0) {
      snprintf(subBuf, sizeof(subBuf), "%s · %d个红绿灯", rpOpts[i].dist, rpOpts[i].lights);
    } else {
      snprintf(subBuf, sizeof(subBuf), "%s", rpOpts[i].dist);
    }
    drawTextUTF8(optX + 24, cy + 50, subBuf, 0x9CF3);

    // 右侧选中标识圆点
    if (sel) {
      aaDisc(optX + optW - 22, cy + optH / 2, 7.0f, C_CYAN);
      aaDisc(optX + optW - 22, cy + optH / 2, 3.0f, C_BLACK);
    } else {
      gfx->drawCircle(optX + optW - 22, cy + optH / 2, 7, 0x4A69);
    }
  }

  // 底部“开始导航”按钮
  const int goW = 220, goH = 44;
  const int goX = CX - goW / 2;
  const int goY = sheetY + sheetH - 58;
  drawTranslucentRoundRect(goX, goY, goW, goH, 22, 0x05E0, 225);
  gfx->drawRoundRect(goX, goY, goW, goH, 22, 0x7FE0);
  if (isChineseLang) {
    drawTextUTF8Center(CX, goY + 29, "开始导航", C_BLACK);
  } else {
    drawAaString(CX, goY + 22, "START NAV", 1.9f, 1.1f, 12.5f, C_BLACK, true);
  }
}

// 待机主页面完整重绘
void drawWaiting(int st) {
  waitSt = st;
  if (sliding) return;

  if (page == 2) { onWaitScreen = false; musicDirty = true; return; }
  if (page == 1) { onWaitScreen = false; settingsDirty = true; return; }
  if (page == 3) { onWaitScreen = false; leanDirty = true; return; }
  if (page == 4) { onWaitScreen = false; return; }

  onWaitScreen = true;

  // 1. 绘制正宗罪恶都市风格地图背景 (暖沙色街区、青绿水面、公园绿地、粉紫路线与车标，半透明暗化)
  drawThemedMapBackground();

  // 2. 居中大时钟与纯数字日期毛玻璃半透明卡片（超大扁平时间 SF76 + 数字日期）
  const int clockW = 296, clockH = 136;
  const int clockX = CX - clockW / 2;
  const int clockY = 56;
  drawTranslucentRoundRect(clockX, clockY, clockW, clockH, 24, 0x0000, 200);
  gfx->drawRoundRect(clockX, clockY, clockW, clockH, 24, 0x39E7);

  // 获取实时系统时间
  time_t nowSec = time(NULL);
  struct tm *t = localtime(&nowSec);
  int year = t ? (t->tm_year + 1900) : 2026;
  int hour = t ? t->tm_hour : 12;
  int min  = t ? t->tm_min  : 0;
  int mon  = t ? (t->tm_mon + 1) : 11;
  int mday = t ? t->tm_mday : 23;

  char clockBuf[12];
  snprintf(clockBuf, sizeof(clockBuf), "%02d:%02d", hour, min);
  smoothDrawText(SF76, clockBuf, CX, clockY + 48, C_WHITE, 1, 1, false);

  char dateBuf[24];
  snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d", year, mon, mday);
  smoothDrawText(SF22, dateBuf, CX, clockY + 108, 0xAFBD, 1, 1, false);

  // 3. READY / NO LINK 连接状态提示 (移动到时间卡片正下方 Y=222)
  const int readyY = clockY + clockH + 30; // 222
  if (st == WAIT_READY || (bleConnected && bleAuthed)) {
    aaDisc((float)(CX - 38), (float)readyY, 4.5f, 0x760D);
    drawAaString(CX + 6, readyY, "READY", 1.8f, 1.05f, 11.5f, 0x760D, true);
  } else if (st == WAIT_NOLINK || !bleConnected) {
    aaDisc((float)(CX - 42), (float)readyY, 4.5f, C_GREY);
    drawAaString(CX + 6, readyY, "NO LINK", 1.8f, 1.05f, 11.5f, C_GREY, true);
  } else {
    aaDisc((float)(CX - 48), (float)readyY, 4.5f, C_RED);
    drawAaString(CX + 6, readyY, "LINK LOST", 1.8f, 1.05f, 11.5f, C_RED, true);
  }

  // 收藏点路况仅在用户打开收藏列表抽屉 (drawDestinationsSheet) 时显示，首页保持极简清爽

  // 4. 随主题自适应的专属收藏按钮 (CX=233, Y=322)
  const int btnCY = 322;
  drawThemedFavIcon(CX, btnCY, 34, false);

  // 按钮下方文字标签
  drawAaString(CX, btnCY + 46, "FAVORITES", 1.4f, 0.85f, 9.5f, 0x7BEF, true);

  // 5. 如果用户点击打开了目的地抽屉或路线选择浮层，则覆盖绘制
  if (destSheetOpen) {
    drawDestinationsSheet();
  }
  if (routePickerOpen) {
    drawRoutePickerSheet();
  }

  // 6. 底部页面指示点
  drawPageDots();
  destBgSynced = false;
  needFlush = true;
}

// 刷新当前页面辅助函数（兼容待机与导航/巡航态）
static inline void refreshOverlayPage() {
  if (inNav) navDirty = true;
  else if (page == 0) drawWaiting(waitSt);
}

// 收藏抽屉滚动极速重绘：待机态免重画背景/大时钟 + 异步推屏，消除滑动卡顿
static inline void refreshDestSheetScroll() {
  if (inNav) {
    navDirty = true;
    return;
  }
  if (page != 0 || !destSheetOpen) return;
  if (!destBgSynced && fbBuf[0] && fbBuf[1]) {
    // 等待上一帧推屏完成后立即归还 flushDone 信号量，严防死锁
    if (flushDone && xSemaphoreTake(flushDone, pdMS_TO_TICKS(100)) == pdTRUE) {
      xSemaphoreGive(flushDone);
    }
    memcpy(fbBuf[fbCur], fbBuf[fbCur ^ 1], (size_t)LCD_WIDTH * LCD_HEIGHT * 2);
    destBgSynced = true;
  }
  drawDestinationsSheet();
  drawPageDots();
  frameIsMap = true; // 走 flushAsync() 异步推屏，不阻塞触摸采样
  needFlush = true;
}

// 圆屏选路浮层点击处理（待机/巡航/导航通用）
inline bool routePickerTap(int x, int y) {
  if (!routePickerOpen) return false;
  const int sheetW = 384;
  const int sheetH = 356;
  const int sheetX = CX - sheetW / 2;
  const int sheetY = CX - sheetH / 2;

  // 点击右上角关闭按钮或外部空白区域：取消选路
  int closeX = sheetX + sheetW - 26;
  int closeY = sheetY + 26;
  if ((x - closeX) * (x - closeX) + (y - closeY) * (y - closeY) <= 26 * 26 ||
      x < sheetX || x > sheetX + sheetW || y < sheetY || y > sheetY + sheetH) {
    routePickerOpen = false;
    routePickerLoading = false;
    routePickerErrUntil = 0;
    rpErrMsg[0] = 0;
    if (bleConnected) pendingEvt = "T,RP_CANCEL";
    CompanionMode::startVibration(45, 18);
    refreshOverlayPage();
    return true;
  }

  if (routePickerLoading || rpErrMsg[0]) return true;

  // 检查点击候选路线项
  const int optW = sheetW - 28;
  const int optH = 62;
  const int optX = sheetX + 14;
  int optStartY = sheetY + 58;
  int cnt = (rpOptCount < MAX_ROUTE_PLAN_OPTS) ? rpOptCount : MAX_ROUTE_PLAN_OPTS;
  for (int i = 0; i < cnt; i++) {
    int cy = optStartY + i * (optH + 8);
    if (x >= optX && x <= optX + optW && y >= cy && y <= cy + optH) {
      rpSelected = i;
      CompanionMode::startVibration(50, 20);
      refreshOverlayPage();
      return true;
    }
  }

  // 检查点击底部“开始导航”按钮
  const int goW = 220, goH = 44;
  const int goX = CX - goW / 2;
  const int goY = sheetY + sheetH - 58;
  if (x >= goX - 10 && x <= goX + goW + 10 && y >= goY - 8 && y <= goY + goH + 8 && cnt > 0) {
    CompanionMode::startVibration(80, 35);
    routePickerOpen = false;
    routePickerLoading = false;
    if (bleConnected) {
      pendingEvt = String("T,RP_GO,") + rpSelected;
    }
    refreshOverlayPage();
    return true;
  }

  return true;
}

// 收藏与沿途快捷服务抽屉点击处理（待机/巡航/导航通用）
inline bool destSheetTap(int x, int y) {
  if (!destSheetOpen) return false;
  const int sheetW = 388;
  const int sheetH = 374;
  const int sheetX = CX - sheetW / 2;
  const int sheetY = 60;
  const int listTop = sheetY + 92;
  const int listBot = sheetY + sheetH - 36;

  // 点击外侧顶部居中关闭按钮（放大热区半径 44）或卡片外部任意区域：关闭浮层
  int closeX = CX;
  int closeY = 30;
  if ((x - closeX) * (x - closeX) + (y - closeY) * (y - closeY) <= 44 * 44 ||
      x < sheetX || x > sheetX + sheetW || y < sheetY || y > sheetY + sheetH) {
    destSheetOpen = false;
    CompanionMode::startVibration(45, 18);
    refreshOverlayPage();
    return true;
  }

  // 1. 检查是否点击顶部 4 个沿途/周边快捷服务图标
  static const char *const qCmds[4] = {"T,Q,GAS", "T,Q,SVC", "T,Q,REP", "T,Q,WC"};
  static const char *const qTipZh[4] = {"正在查找顺路加油站...", "正在查找顺路服务区...", "正在查找附近维修点...", "正在查找附近洗手间..."};
  const int qY = sheetY + 32;
  const int qW = 84;
  const int qH = 54;
  const int qGap = 8;
  const int qStartX = sheetX + 14;
  for (int k = 0; k < 4; k++) {
    int bx = qStartX + k * (qW + qGap);
    if (x >= bx && x <= bx + qW && y >= qY && y <= qY + qH) {
      CompanionMode::startVibration(65, 25);
      destSheetOpen = false;
      navStarUntil = 0;
      alongPoi.active = false;
      alongPoi.status = 0;
      alongPoi.detourM = 0;
      alongPoi.distM = 0;
      strncpy(alongPoi.name, qTipZh[k], sizeof(alongPoi.name) - 1);
      alongPoi.name[sizeof(alongPoi.name) - 1] = 0;
      alongPoi.expireAt = millis() + 6500;
      alongSearchRequested = true;
      alongSearching = true;
      alongSearchUntil = millis() + 6500;
      if (bleConnected) pendingEvt = qCmds[k];
      refreshOverlayPage();
      return true;
    }
  }

  // 2. 检查是否点击可滚动收藏地点项（仅在可视列表区域内生效）
  if (y >= listTop && y <= listBot) {
    const int itemW = sheetW - 32;
    const int itemH = 76;
    const int itemGap = 8;
    const int itemX = sheetX + 14;
    for (int i = 0; i < destItemCount; i++) {
      int curY = listTop + i * (itemH + itemGap) - destScrollOffset;
      if (x >= itemX && x <= itemX + itemW && y >= curY && y <= curY + itemH) {
        CompanionMode::startVibration(65, 28);
        destSheetOpen = false;
        navStarUntil = 0;
        if (bleConnected) {
          strncpy(rpDestName, destItems[i].name[0] ? destItems[i].name : "目的地", sizeof(rpDestName) - 1);
          rpDestName[sizeof(rpDestName) - 1] = 0;
          rpOptCount = 0;
          rpSelected = 0;
          rpErrMsg[0] = 0;
          routePickerErrUntil = 0;
          routePickerOpen = true;
          routePickerLoading = true;
          pendingEvt = String("T,DEST_GO,") + i;
        }
        refreshOverlayPage();
        return true;
      }
    }
  }
  return true;
}

// 待机主页面点击处理 (在 TouchInput 中路由)
inline bool waitingScreenTap(int x, int y) {
  if (page != 0 || inNav) return false;
  if (routePickerOpen) return routePickerTap(x, y);
  if (destSheetOpen) return destSheetTap(x, y);

  // 未展开浮层时：检查是否点击了收藏按钮 (CX=233, btnCY=322, 半径 40)
  int dx = x - CX;
  int dy = y - 322;
  if (dx * dx + dy * dy <= 40 * 40) {
    CompanionMode::startVibration(50, 20);
    destScrollOffset = 0;
    destSheetOpen = true;
    if (bleConnected) {
      pendingEvt = "T,DEST_Q";
    }
    drawWaiting(waitSt);
    return true;
  }

  // 需求 2：圆屏已连接 app 的情况下，在圆屏首页点击空白区域可进入巡航模式
  if (bleConnected && bleAuthed) {
    int distSq = (x - CX) * (x - CX) + (y - CX) * (y - CX);
    if (distSq <= 210 * 210) {
      CompanionMode::startVibration(70, 30);
      pendingEvt = "T,CRUISE";
      return true;
    }
  }

  return false;
}
