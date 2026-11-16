#pragma once
#include <Arduino.h>
#include <time.h>
#include "UiModern.h"
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

// ===== 赛博朋克风格时钟主页（参考 IMG_20261113_151246.jpg）=====
static inline void drawHazardBand(int y, int h) {
  gfx->fillRect(36, y, 394, h, C_BLACK);
  for (int x = 28; x < 438; x += 22) {
    gfx->fillTriangle(x, y + h, x + 11, y + h, x + 22, y, C_ORANGE);
    gfx->fillTriangle(x, y + h, x + 22, y, x + 11, y, C_ORANGE);
  }
}

static inline void drawCyberHome(int st) {
  extern void drawBattery(int x0, int y0, bool backing = false);
  gfx->fillScreen(C_BLACK);

  const bool linked = (st == WAIT_READY) || (bleConnected && bleAuthed);

  // 顶部标题：時間（水平居中，避免圆形边缘裁切） | KACL 里程装饰 | 连接状态点
  drawTextUTF8Center(CX, 96, "時間", C_WHITE);
  gfx->fillRect(CX - 44, 64, 6, 30, C_RED);
  drawAaString(122, 76, "KACL", 1.5f, 1.0f, 11.0f, C_WHITE, false);
  drawAaString(122, 102, "00900", 1.7f, 1.1f, 12.0f, C_WHITE, false);
  aaDisc(404.0f, 82.0f, 5.0f, linked ? 0x760D : C_GREY);

  // 双警示条纹带
  drawHazardBand(116, 20);
  drawAaString(CX, 163, "STANDARD TIME", 1.5f, 1.0f, 10.0f, C_WHITE, true);

  // 时间：时 / 分 / 秒
  time_t nowSec = time(NULL);
  struct tm *t = localtime(&nowSec);
  const int hour = t ? t->tm_hour : 20;
  const int min  = t ? t->tm_min  : 24;
  const int sec  = t ? t->tm_sec  : 0;
  const int mon  = t ? (t->tm_mon + 1) : 11;
  const int mday = t ? t->tm_mday : 18;
  char buf[8];
  snprintf(buf, sizeof(buf), "%02d", hour);
  drawAaString(96, 222, buf, 4.6f, 2.7f, 28.0f, C_WHITE, true);
  snprintf(buf, sizeof(buf), "%02d", min);
  drawAaString(196, 222, buf, 4.6f, 2.7f, 28.0f, C_WHITE, true);
  snprintf(buf, sizeof(buf), "%02d", sec);
  drawAaString(296, 222, buf, 4.6f, 2.7f, 28.0f, C_GREY, true);
  drawAaString(140, 198, "H", 1.5f, 1.0f, 0.0f, C_WHITE, true);
  drawAaString(240, 198, "M", 1.5f, 1.0f, 0.0f, C_WHITE, true);
  drawTextUTF8(86, 252, "時", C_WHITE);
  drawTextUTF8(186, 252, "分", C_WHITE);
  drawTextUTF8(286, 252, "秒", C_GREY);

  // 日期框（参考图 12/18 样式）
  gfx->fillRoundRect(350, 190, 78, 34, 6, C_WHITE);
  char dateBuf[12];
  snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d", mon, mday);
  gfx->setTextSize(2);
  gfx->setTextColor(C_BLACK);
  int tw = (int)strlen(dateBuf) * 12;
  gfx->setCursor(350 + (78 - tw) / 2, 199);
  gfx->print(dateBuf);

  drawHazardBand(272, 20);

  // 下半区左右对半：左半电源/电量（仅主页显示），右半 ENAV logo
  int pct = batPct > 0 ? constrain(batPct, 0, 100) : 0;
  int lit = batPct < 0 ? 3 : (pct * 5 + 50) / 100;

  // 闪电符号（左上，放大）
  gfx->fillTriangle(66, 296, 40, 336, 56, 336, C_RED);
  gfx->fillTriangle(60, 380, 76, 340, 60, 340, C_RED);
  gfx->drawLine(62, 332, 54, 348, C_ORANGE);
  gfx->drawLine(54, 348, 64, 348, C_ORANGE);
  gfx->drawLine(64, 348, 56, 366, C_ORANGE);

  // 电源红底标签（放大）
  gfx->fillRect(84, 296, 88, 38, C_RED);
  drawTextUTF8(92, 318, "電源", C_WHITE);
  drawAaString(96, 348, "BATTERY", 1.5f, 1.0f, 10.0f, C_WHITE, false);

  // 电量百分比数字：紧贴 "BATTERY" 标签右侧，同一行
  char pctBuf[8];
  if (batPct < 0) snprintf(pctBuf, sizeof(pctBuf), "--%%");
  else snprintf(pctBuf, sizeof(pctBuf), "%d%%", pct);
  drawAaString(176, 348, pctBuf, 1.9f, 1.2f, 13.0f, C_ORANGE, false);

  // 5 格电量条（放大，占满左半宽度）
  for (int i = 0; i < 5; i++) {
    int bx = 84 + i * 26;
    gfx->drawRect(bx, 368, 20, 16, C_WHITE);
    if (i < lit) gfx->fillRect(bx + 3, 371, 14, 10, batChg ? C_CYAN : C_ORANGE);
  }

  // 右半：用户设计的 ENAV logo（红黑撕裂风格位图，已裁掉上下延伸图案只保字体）
  EnavLogo::draw(242, 300);
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
  screenSaverActive = false;

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

  // 未展开浮层时：检查是否点击了 R星五角星按钮 (CX=233, btnCY=322, 半径 38)
  int dx = x - CX;
  int dy = y - 322;
  if (dx * dx + dy * dy <= 38 * 38) {
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
