#pragma once
#include <Arduino.h>
#include <time.h>
#include "UiModern.h"
#include "HomeFace.h"
#include "DestHistory.h"
#include "EnavLogo.h"

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

// 说明：旧的 drawTextUTF8Scaled（44x24 离屏缓冲 + setFramebuffer）已删除。
// 它按画布宽度 466 计算行跨度写入仅 44 宽的小缓冲，造成大规模越界写，
// 是切换视图时黑屏死机的元凶。新主页为纯数字/英文，不再需要中文放大。

// 全局变量定义
DestItem destItems[MAX_DEST_ITEMS];
int destItemCount = 0;
bool destSheetOpen = false;
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

// 绘制地点与导航历史毛玻璃抽屉浮层
static void drawDestinationsSheet() {
  const int sheetW = 380;
  const int sheetH = 340;
  const int sheetX = CX - sheetW / 2; // 43
  const int sheetY = CX - sheetH / 2; // 63

  // 1. 半透明毛玻璃暗黑圆角底座 (92% 纯黑磨砂)
  drawTranslucentRoundRect(sheetX, sheetY, sheetW, sheetH, 22, 0x0000, 235);
  gfx->drawRoundRect(sheetX, sheetY, sheetW, sheetH, 22, 0x39E7);

  // 2. 顶部标题栏
  drawRockstarStar(sheetX + 26, sheetY + 28, 9.0f, 4.0f, 0xFDE0, 0xFDE0);
  if (isChineseLang) {
    drawTextUTF8(sheetX + 46, sheetY + 34, "常用收藏", C_WHITE);
  } else {
    drawAaString(sheetX + 105, sheetY + 28, "FAVORITES", 1.8f, 1.0f, 12.0f, C_WHITE, true);
  }

  // 右上角关闭按钮 (✕)
  drawTranslucentCircle(sheetX + sheetW - 28, sheetY + 28, 14, 0x2124, 160);
  gfx->drawCircle(sheetX + sheetW - 28, sheetY + 28, 14, 0x4A69);
  gfx->drawLine(sheetX + sheetW - 32, sheetY + 24, sheetX + sheetW - 24, sheetY + 32, C_WHITE);
  gfx->drawLine(sheetX + sheetW - 24, sheetY + 24, sheetX + sheetW - 32, sheetY + 32, C_WHITE);

  // 分隔线
  gfx->drawLine(sheetX + 18, sheetY + 52, sheetX + sheetW - 18, sheetY + 52, 0x2945);

  // 3. 地点卡片列表 (显示常用地点与历史记录，最多 4 项)
  destInitDefaults();
  int showCount = min(destItemCount, 4);
  const int itemW = sheetW - 32; // 348
  const int itemH = 56;
  const int itemX = sheetX + 16;
  int startY = sheetY + 64;

  for (int i = 0; i < showCount; i++) {
    int curY = startY + i * (itemH + 10);
    // 卡片背景
    drawTranslucentRoundRect(itemX, curY, itemW, itemH, 12, 0x10A2, 160);
    gfx->drawRoundRect(itemX, curY, itemW, itemH, 12, 0x31A6);

    // 左侧图标：收藏显示金色五角星，历史显示青色定位针
    if (destItems[i].type == 0) {
      drawRockstarStar(itemX + 24, curY + itemH / 2, 8.0f, 3.5f, 0xFDE0, 0xFDE0);
    } else {
      aaDisc(itemX + 24, curY + itemH / 2 - 3, 5.0f, C_CYAN);
      gfx->fillTriangle(itemX + 20, curY + itemH / 2 - 2,
                        itemX + 28, curY + itemH / 2 - 2,
                        itemX + 24, curY + itemH / 2 + 7, C_CYAN);
      aaDisc(itemX + 24, curY + itemH / 2 - 3, 2.0f, C_BLACK);
    }

    // 地点名称与地址 (支持中文与中英混排)
    const char *name = destItems[i].name[0] ? destItems[i].name : (isChineseLang ? "未知地点" : "Unknown Location");
    const char *addr = destItems[i].addr[0] ? destItems[i].addr : (isChineseLang ? "轻触立即规划" : "Tap to route");
    drawTextUTF8(itemX + 54, curY + 25, name, C_WHITE);
    drawTextUTF8(itemX + 54, curY + 45, addr, 0x7BEF);

    // 右侧导航箭头 (>)
    drawAaString(itemX + itemW - 18, curY + itemH / 2, ">", 1.8f, 1.0f, 10.0f, 0x4A69, true);
  }

  // 底部提示
  if (isChineseLang) {
    drawTextUTF8Center(CX, sheetY + sheetH - 12, "已同步手机常用收藏", 0x7BEF);
  } else {
    drawAaString(CX, sheetY + sheetH - 18, "SYNCED FAVORITE PLACES", 1.2f, 0.75f, 8.5f, 0x4A69, true);
  }
}

// ===== 极简暗色圆形时钟主页（参考 f047366e44ec7bdd54cd06d0c6c26e35.jpg）=====
// 顶部：电池图标 + 电量百分比 + 收藏五角星（星标在百分比右侧，点击区见 waitingScreenTap）
// 中部：大号 HH:MM，上下分隔线；底部：MM-DD 与英文星期
// 布局严格上下对称：大号时间块居中于圆心，顶行/底行分列上下分隔线两侧。
#define HOME_BAT_X   104
#define HOME_BAT_Y   110
#define HOME_STAR_X  335
#define HOME_STAR_Y  129

static inline void drawCyberHome(int st) {
  (void)st;
  gfx->fillScreen(HOME_BG);

  // 1. 外圈圆环（4px 粗，中性灰）
  for (int dr = 0; dr < 5; dr++) gfx->drawCircle(CX, CX, 196 + dr, HOME_RING);

  // 2. 顶部一行：电池图标 + 电量百分比 + 收藏五角星
  int pct = batPct > 0 ? constrain(batPct, 0, 100) : 0;
  gfx->drawRoundRect(HOME_BAT_X, HOME_BAT_Y, 60, 38, 8, HOME_SUB);
  gfx->fillRoundRect(HOME_BAT_X + 60, HOME_BAT_Y + 12, 8, 14, 2, HOME_SUB);
  // 充电闪电（参考图样式：居中双段折线）
  {
    int fx = HOME_BAT_X + 30, fy = HOME_STAR_Y;
    gfx->fillTriangle(fx + 5, fy - 14, fx - 7, fy + 2, fx + 1, fy + 2, HOME_SUB);
    gfx->fillTriangle(fx + 1, fy + 2, fx - 4, fy + 14, fx + 8, fy - 4, HOME_SUB);
  }
  char pctBuf[8];
  if (batPct < 0) snprintf(pctBuf, sizeof(pctBuf), "--%%");
  else snprintf(pctBuf, sizeof(pctBuf), "%d%%", pct);
  {
    const float PCW = 4.6f, PCH = 4.9f, PGAP = 5.0f, PFW = 2.0f;
    float pw = homeTextWidth(pctBuf, PCW, PCH, PGAP, PFW);
    homeDrawText(256.0f - pw / 2.0f, HOME_STAR_Y - PCH * 7.0f / 2.0f, pctBuf,
                 PCW, PCH, PGAP, PFW, HOME_SUB, HOME_BG);
  }
  drawRockstarStar(HOME_STAR_X, HOME_STAR_Y, 18.0f, 7.5f, HOME_SUB, HOME_SUB);

  // 3. 上分隔线
  gfx->fillRect(83, 160, 301, 4, HOME_LINE);

  // 4. 大号 HH:MM（严格居中于圆心，参考图 y 183 起）
  time_t nowSec = time(NULL);
  struct tm *tmv = localtime(&nowSec);
  int hour = tmv ? tmv->tm_hour : 20;
  int min  = tmv ? tmv->tm_min  : 8;
  int mon  = tmv ? (tmv->tm_mon + 1) : 11;
  int mday = tmv ? tmv->tm_mday : 18;
  int wday = tmv ? tmv->tm_wday : 5;
  char clockBuf[12];
  snprintf(clockBuf, sizeof(clockBuf), "%02d:%02d", hour, min);
  {
    const float CW = 11.0f, CH = 14.4f, GAP = 12.0f, FW = 5.0f;
    float tw = homeTextWidth(clockBuf, CW, CH, GAP, FW);
    homeDrawText(CX - tw / 2.0f, 183.0f, clockBuf, CW, CH, GAP, FW, HOME_TXT, HOME_BG);
  }

  // 5. 下分隔线
  gfx->fillRect(83, 303, 301, 4, HOME_LINE);

  // 6. 日期 + 星期（贴近下分隔线，日期与星期间距收小）
  char dateBuf[8];
  snprintf(dateBuf, sizeof(dateBuf), "%02d-%02d", mon, mday);
  {
    const float DW = 4.4f, DH = 5.4f, DGAP = 8.0f, DFW = 2.0f;
    const float WW = 4.0f, WH = 5.0f, WGAP = 7.0f, WFW = 2.0f;
    const float GX = 32.0f;
    float dw = homeTextWidth(dateBuf, DW, DH, DGAP, DFW);
    float ww = homeTextWidth(getWdayStr(wday), WW, WH, WGAP, WFW);
    float x0 = CX - (dw + GX + ww) / 2.0f;
    homeDrawText(x0, 318.0f, dateBuf, DW, DH, DGAP, DFW, HOME_SUB, HOME_BG);
    homeDrawText(x0 + dw + GX, 318.0f, getWdayStr(wday), WW, WH, WGAP, WFW, HOME_SUB, HOME_BG);
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

  // 1. 赛博朋克时钟主页（时间 / 日期 / 电量 / BPM 装饰）
  drawCyberHome(st);

  // 2. 如果用户点击打开了目的地抽屉浮层，则覆盖绘制
  if (destSheetOpen) {
    drawDestinationsSheet();
  }

  // 3. 底部页面指示点
  drawPageDots();
  needFlush = true;
}

// 待机主页面点击处理 (在 TouchInput 中路由)
inline bool waitingScreenTap(int x, int y) {
  if (page != 0 || inNav) return false;

  // 如果目的地抽屉浮层已展开
  if (destSheetOpen) {
    const int sheetW = 380;
    const int sheetH = 340;
    const int sheetX = CX - sheetW / 2;
    const int sheetY = CX - sheetH / 2;

    // 点击右上角关闭按钮 (✕) 或卡片外部空白区域：关闭浮层
    int closeX = sheetX + sheetW - 28;
    int closeY = sheetY + 28;
    if ((x - closeX) * (x - closeX) + (y - closeY) * (y - closeY) <= 24 * 24 ||
        x < sheetX || x > sheetX + sheetW || y < sheetY || y > sheetY + sheetH) {
      destSheetOpen = false;
      drawWaiting(waitSt);
      return true;
    }

    // 检查是否点击了某个地点卡片项
    int showCount = min(destItemCount, 4);
    const int itemW = sheetW - 32;
    const int itemH = 56;
    const int itemX = sheetX + 16;
    int startY = sheetY + 64;

    for (int i = 0; i < showCount; i++) {
      int curY = startY + i * (itemH + 10);
      if (x >= itemX && x <= itemX + itemW && y >= curY && y <= curY + itemH) {
        // 触发微震反馈 (遵循暂不实现从圆屏规划导航功能)
        CompanionMode::startVibration(60, 25);
        drawWaiting(waitSt);
        return true;
      }
    }
    return true;
  }

  // 未展开浮层时：检查右上角的收藏五角星按钮 (HOME_STAR_X/Y, 半径 40)
  int dx = x - HOME_STAR_X;
  int dy = y - HOME_STAR_Y;
  if (dx * dx + dy * dy <= 40 * 40) {
    CompanionMode::startVibration(50, 20);
    destSheetOpen = true;
    // 向手机请求最新的常用地点和历史
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
