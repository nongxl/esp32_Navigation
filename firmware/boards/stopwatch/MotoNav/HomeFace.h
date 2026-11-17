#pragma once
// ===========================================================================
// 主页表盘字库与渲染（StopWatch 待机主页）
//
// 目标：还原参考图 f047366e...jpg 的极简暗色圆形表盘 ——
//   · 纯黑圆底 + 中性灰圆环
//   · 严格上下对称：大号时间块居中于圆心，顶行/底行分列上下分隔线两侧
//   · 圆润几何粗体字：数字用"笔画化"矢量（水平/垂直胶囊 + 斜切四边形 +
//     圆角矩形环），字母用 5x7 点阵行/列合并成实心连续笔画
//   · 按墨迹实际宽度排版，字距清晰不粘连
//
// 本文件是 tools/home_preview.py 预览器的固件对应实现，几何参数 1:1 对齐。
// ===========================================================================
#include <Arduino.h>
#include <math.h>

// ---------------------------------------------------------------------------
// 颜色（RGB565，与预览器一致）
// ---------------------------------------------------------------------------
#define HOME_BG    0x0000   // 纯黑背景
#define HOME_RING  0x5AEB   // 外环 ~95 灰
#define HOME_LINE  0x528A   // 分隔线 ~82 灰
#define HOME_TXT   0xAD55   // 时间 ~172 亮灰
#define HOME_SUB   0x9492   // 顶行/底行 ~146 中灰

// ---------------------------------------------------------------------------
// 笔画定义
//   kind: 'H' 水平笔画（y 为中心行，t 厚度单位=行）
//         'V' 垂直笔画（x 为中心列，t 厚度单位=列）
//         'D' 斜笔画（斜切四边形，t 厚度单位=行）
//         'B' 方块（矩形）
//         'R' 圆角矩形环（tx 厚度单位=列，ty 厚度单位=行）
// 网格坐标：x 0..5 列，y 0..7 行
// ---------------------------------------------------------------------------
struct HomeStroke {
  char kind;
  float x0, y0, x1, y1;
  float t;   // 厚度（H/D=行，V=列）
  float t2;  // 环的纵向厚度（行）
  float rad; // 环的圆角半径（列，0=自动）
};

#define HS(n) (n)

// 数字与符号
static const HomeStroke HOME_NUM[11][7] = {
  // '0' 闭合圆角矩形环
  {{'R',0,0, 5,7, 1.15f, 0.85f, 0}, {0}},
  // '1' 竖笔 + 左上斜切旗角
  {{'V',2.45f,0.15f, 2.45f,6.90f, 1.10f, 0, 0}, {'D',0.15f,1.70f, 2.45f,0.15f, 0.95f, 0, 0}},
  // '2'
  {{'H',0.65f,0.60f, 4.35f,0.60f, 0.90f, 0, 0}, {'D',4.35f,1.05f, 3.45f,2.95f, 0.90f, 0, 0},
   {'H',1.60f,3.10f, 3.60f,3.10f, 0.85f, 0, 0}, {'D',1.75f,3.15f, 0.75f,6.10f, 0.90f, 0, 0},
   {'H',0.65f,6.40f, 4.45f,6.40f, 0.90f, 0, 0}},
  // '3'
  {{'H',0.70f,0.60f, 4.30f,0.60f, 0.90f, 0, 0}, {'D',4.30f,1.05f, 3.40f,2.95f, 0.90f, 0, 0},
   {'H',2.00f,3.10f, 3.60f,3.10f, 0.85f, 0, 0}, {'D',3.50f,3.15f, 4.35f,6.05f, 0.90f, 0, 0},
   {'H',0.70f,6.40f, 4.30f,6.40f, 0.90f, 0, 0}},
  // '4'
  {{'D',3.35f,0.05f, 0.75f,4.55f, 0.95f, 0, 0}, {'H',0.75f,4.70f, 4.50f,4.70f, 0.90f, 0, 0},
   {'V',3.35f,0.05f, 3.35f,6.90f, 1.05f, 0, 0}},
  // '5'
  {{'H',0.80f,0.60f, 4.30f,0.60f, 0.90f, 0, 0}, {'V',0.80f,0.60f, 0.80f,3.15f, 1.05f, 0, 0},
   {'H',0.80f,3.25f, 3.70f,3.25f, 0.85f, 0, 0}, {'D',3.70f,3.30f, 4.35f,6.05f, 0.90f, 0, 0},
   {'H',0.80f,6.40f, 4.30f,6.40f, 0.90f, 0, 0}},
  // '6'
  {{'D',0.85f,2.20f, 2.50f,0.55f, 0.90f, 0, 0}, {'H',2.50f,0.55f, 4.30f,0.55f, 0.90f, 0, 0},
   {'V',0.85f,2.20f, 0.85f,6.40f, 1.05f, 0, 0}, {'H',0.85f,3.40f, 3.60f,3.40f, 0.85f, 0, 0},
   {'D',3.60f,3.45f, 4.35f,6.05f, 0.90f, 0, 0}, {'H',0.85f,6.40f, 4.30f,6.40f, 0.90f, 0, 0}},
  // '7'
  {{'H',0.45f,0.60f, 4.60f,0.60f, 0.90f, 0, 0}, {'D',4.05f,0.95f, 1.85f,6.90f, 0.95f, 0, 0}},
  // '8' 上下两个圆角矩形环
  {{'R',0,0, 5,3.50f, 1.15f, 0.80f, 0}, {'R',0,3.50f, 5,7, 1.15f, 0.80f, 0}},
  // '9'
  {{'R',0,0, 5,3.35f, 1.15f, 0.80f, 0}, {'V',4.25f,0.05f, 4.25f,6.15f, 1.05f, 0, 0}},
  // ':' 两点
  {{'B',0,1.90f, 1.30f,2.85f, 0, 0, 0}, {'B',0,4.15f, 1.30f,5.10f, 0, 0, 0}},
};
// 数字笔画条目数
static const uint8_t HOME_NUM_N[11] = {1, 2, 5, 5, 3, 5, 6, 2, 2, 2, 2};
// 数字/符号索引表：'0'..'9', ':' , '%' , '-'
static const char HOME_NUM_KEYS[13] = {'0','1','2','3','4','5','6','7','8','9',':','%','-'};

// '%' 单独定义（几何：左上块 + 斜杠 + 右下块）
static const HomeStroke HOME_PCT[3] = {
  {'B',0,0, 1.50f,2.20f, 0, 0, 0},
  {'B',3.50f,4.80f, 5.00f,7.00f, 0, 0, 0},
  {'D',0.30f,7.00f, 4.70f,0.00f, 0.55f, 0, 0},
};

// '-' 单独定义（单条水平笔画）
static const HomeStroke HOME_MINUS[1] = {
  {'H',0.60f,3.60f, 4.40f,3.60f, 0.80f, 0, 0},
};

// 取数字/符号的笔画表与条目数（未找到返回 nullptr）
static inline const HomeStroke* homeNumStrokes(char c, uint8_t &n) {
  if (c == '%') { n = 3; return HOME_PCT; }
  if (c == '-') { n = 1; return HOME_MINUS; }
  for (int i = 0; i < 11; i++) {
    if (HOME_NUM_KEYS[i] == c) { n = HOME_NUM_N[i]; return HOME_NUM[i]; }
  }
  n = 0; return nullptr;
}

// ---------------------------------------------------------------------------
// 字母 5x7 点阵（行合并 + 列合并 → 实心连续笔画）
// ---------------------------------------------------------------------------
static const uint8_t HOME_LET[26][5] = {
  {0x7C,0x12,0x11,0x12,0x7C}, {0x7F,0x49,0x49,0x49,0x36}, {0x3E,0x41,0x41,0x41,0x22},
  {0x7F,0x41,0x41,0x22,0x1C}, {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01},
  {0x3E,0x41,0x49,0x49,0x7A}, {0x7F,0x08,0x08,0x08,0x7F}, {0x00,0x41,0x7F,0x41,0x00},
  {0x20,0x40,0x41,0x3F,0x01}, {0x7F,0x08,0x14,0x22,0x41}, {0x7F,0x40,0x40,0x40,0x40},
  {0x7F,0x02,0x0C,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F}, {0x3E,0x41,0x41,0x41,0x3E},
  {0x7F,0x09,0x09,0x09,0x06}, {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46},
  {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7F,0x01,0x01}, {0x3F,0x40,0x40,0x40,0x3F},
  {0x1F,0x20,0x40,0x20,0x1F}, {0x7F,0x20,0x18,0x20,0x7F}, {0x63,0x14,0x08,0x14,0x63},
  {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43},
};

// ---------------------------------------------------------------------------
// 基础绘图原语
// ---------------------------------------------------------------------------

// 小圆角矩形段（半径上限 3px，保证相邻段拼接后连续不粘连）
static inline void homeCap(int x0, int y0, int x1, int y1, uint16_t col) {
  int w = x1 - x0, h = y1 - y0;
  if (w <= 0 || h <= 0) return;
  int r = min(min(w, h) / 4, 3);
  if (r <= 0) { gfx->fillRect(x0, y0, w, h, col); return; }
  gfx->fillRoundRect(x0, y0, w, h, r, col);
}

// 斜切四边形（平头，贴近参考图的斜切端头）
static inline void homeQuad(float ax, float ay, float bx, float by,
                            float hw, float hh, uint16_t col) {
  float dx = bx - ax, dy = by - ay;
  float L = sqrtf(dx * dx + dy * dy);
  if (L < 0.001f) return;
  float nx = -dy / L, ny = dx / L;
  float px0 = ax + nx * hw, py0 = ay + ny * hh;
  float px1 = bx + nx * hw, py1 = by + ny * hh;
  float px2 = bx - nx * hw, py2 = by - ny * hh;
  float px3 = ax - nx * hw, py3 = ay - ny * hh;
  // 拆成两个三角形填充
  gfx->fillTriangle((int)lroundf(px0), (int)lroundf(py0),
                    (int)lroundf(px1), (int)lroundf(py1),
                    (int)lroundf(px2), (int)lroundf(py2), col);
  gfx->fillTriangle((int)lroundf(px0), (int)lroundf(py0),
                    (int)lroundf(px2), (int)lroundf(py2),
                    (int)lroundf(px3), (int)lroundf(py3), col);
}

// ---------------------------------------------------------------------------
// 数字/符号绘制
// ---------------------------------------------------------------------------
static inline void homeDrawNumStroke(const HomeStroke &s, float x, float y,
                                      float cw, float chh, uint16_t col, uint16_t bg) {
  switch (s.kind) {
    case 'H': {
      float yy = y + s.y0 * chh;
      homeCap((int)lroundf(x + s.x0 * cw), (int)lroundf(yy - s.t * chh / 2),
              (int)lroundf(x + s.x1 * cw), (int)lroundf(yy + s.t * chh / 2), col);
      break;
    }
    case 'V': {
      float xx = x + s.x0 * cw;
      homeCap((int)lroundf(xx - s.t * cw / 2), (int)lroundf(y + s.y0 * chh),
              (int)lroundf(xx + s.t * cw / 2), (int)lroundf(y + s.y1 * chh), col);
      break;
    }
    case 'D': {
      homeQuad(x + s.x0 * cw, y + s.y0 * chh, x + s.x1 * cw, y + s.y1 * chh,
               s.t * cw / 2, s.t * chh / 2, col);
      break;
    }
    case 'B': {
      homeCap((int)lroundf(x + s.x0 * cw), (int)lroundf(y + s.y0 * chh),
              (int)lroundf(x + s.x1 * cw), (int)lroundf(y + s.y1 * chh), col);
      break;
    }
    case 'R': {
      // 圆角矩形环：外框实心 + 内孔挖背景
      float w = (s.x1 - s.x0) * cw, h = (s.y1 - s.y0) * chh;
      int ox = (int)lroundf(x + s.x0 * cw), oy = (int)lroundf(y + s.y0 * chh);
      int ow = (int)lroundf(w), oh = (int)lroundf(h);
      int r = (s.rad > 0) ? (int)lroundf(s.rad * cw) : (int)(min(ow, oh) * 0.22f);
      gfx->fillRoundRect(ox, oy, ow, oh, r, col);
      int tx = (int)lroundf(s.t * cw), ty = (int)lroundf(s.t2 * chh);
      int ir = r - tx; if (ir < 1) ir = 1;
      gfx->fillRoundRect(ox + tx, oy + ty, ow - 2 * tx, oh - 2 * ty, ir, bg);
      break;
    }
  }
}

// 数字/符号的墨迹左缘（相对网格原点）与宽度（像素）
static inline float homeNumLeft(char c, float cw, float chh) {
  float mn = 1e9f;
  uint8_t n = 0;
  const HomeStroke *st = homeNumStrokes(c, n);
  if (!st) return 0;
  for (uint8_t i = 0; i < n; i++) {
    const HomeStroke &s = st[i];
    if (s.kind == 'H' || s.kind == 'B') mn = min(mn, s.x0 * cw);
    else if (s.kind == 'V') mn = min(mn, s.x0 * cw - s.t * cw / 2);
    else if (s.kind == 'D') {
      float ax = s.x0 * cw, ay = s.y0 * chh, bx = s.x1 * cw, by = s.y1 * chh;
      float dx = bx - ax, dy = by - ay;
      float L = sqrtf(dx * dx + dy * dy); if (L < 0.001f) continue;
      float nx = -dy / L, hx = s.t * cw / 2;
      mn = min(mn, min(min(ax + nx * hx, ax - nx * hx), min(bx + nx * hx, bx - nx * hx)));
    }
    else if (s.kind == 'R') mn = min(mn, s.x0 * cw);
  }
  return (mn > 1e8f) ? 0 : mn;
}

static inline float homeNumWidth(char c, float cw, float chh) {
  float mn = 1e9f, mx = -1e9f;
  uint8_t n = 0;
  const HomeStroke *st = homeNumStrokes(c, n);
  if (!st) return 0;
  for (uint8_t i = 0; i < n; i++) {
    const HomeStroke &s = st[i];
    float a = 0, b = 0;
    if (s.kind == 'H' || s.kind == 'B') { a = s.x0 * cw; b = s.x1 * cw; }
    else if (s.kind == 'V') { a = s.x0 * cw - s.t * cw / 2; b = s.x0 * cw + s.t * cw / 2; }
    else if (s.kind == 'D') {
      float ax = s.x0 * cw, ay = s.y0 * chh, bx = s.x1 * cw, by = s.y1 * chh;
      float dx = bx - ax, dy = by - ay;
      float L = sqrtf(dx * dx + dy * dy); if (L < 0.001f) continue;
      float nx = -dy / L, hx = s.t * cw / 2;
      float q[4] = {ax + nx * hx, ax - nx * hx, bx + nx * hx, bx - nx * hx};
      a = q[0]; b = q[0];
      for (int k = 1; k < 4; k++) { a = min(a, q[k]); b = max(b, q[k]); }
    }
    else if (s.kind == 'R') { a = s.x0 * cw; b = s.x1 * cw; }
    mn = min(mn, a); mx = max(mx, b);
  }
  return (mx < -1e8f) ? 0 : (mx - mn);
}

// 字母：5x7 点阵行合并 + 列合并 → 实心连续笔画
static inline void homeDrawLetter(char c, float x, float y, float cw, float chh,
                                   float fw, uint16_t col) {
  if (c < 'A' || c > 'Z') return;
  const uint8_t *g = HOME_LET[c - 'A'];
  // 行合并
  for (int r = 0; r < 7; r++) {
    int cc = 0;
    while (cc < 5) {
      if (!((g[cc] >> r) & 1)) { cc++; continue; }
      int c0 = cc;
      while (cc < 5 && ((g[cc] >> r) & 1)) cc++;
      homeCap((int)lroundf(x + c0 * cw), (int)lroundf(y + r * chh),
              (int)lroundf(x + cc * cw) - 1, (int)lroundf(y + (r + 1) * chh) - 1, col);
    }
  }
  // 列合并
  for (int cc = 0; cc < 5; cc++) {
    int r = 0;
    while (r < 7) {
      if (!((g[cc] >> r) & 1)) { r++; continue; }
      int r0 = r;
      while (r < 7 && ((g[cc] >> r) & 1)) r++;
      homeCap((int)lroundf(x + cc * cw - fw / 2), (int)lroundf(y + r0 * chh),
              (int)lroundf(x + cc * cw + cw - 1 + fw / 2), (int)lroundf(y + r * chh) - 1, col);
    }
  }
}

static inline float homeLetLeft(char c, float cw, float fw) {
  if (c < 'A' || c > 'Z') return 0;
  const uint8_t *g = HOME_LET[c - 'A'];
  int mn = 5;
  for (int i = 0; i < 5; i++) if (g[i]) { mn = i; break; }
  return mn * cw - fw / 2;
}

static inline float homeLetWidth(char c, float cw, float fw) {
  if (c < 'A' || c > 'Z') return 0;
  const uint8_t *g = HOME_LET[c - 'A'];
  int mn = 5, mx = -1;
  for (int i = 0; i < 5; i++) if (g[i]) { if (mn > i) mn = i; if (mx < i) mx = i; }
  if (mx < 0) return 0;
  return (mx - mn) * cw + cw + fw;
}

// ---------------------------------------------------------------------------
// 排版：按墨迹实际宽度推进，字距清晰
// ---------------------------------------------------------------------------
static inline float homeTextWidth(const char *s, float cw, float chh, float gap, float fw) {
  float tot = 0; bool first = true;
  for (const char *p = s; *p; p++) {
    char c = *p;
    if (c == ' ') continue;
    float w = (c >= '0' && c <= '9') || c == ':' || c == '%' || c == '-'
                ? homeNumWidth(c, cw, chh)
                : homeLetWidth((char)((c >= 'a' && c <= 'z') ? c - 32 : c), cw, fw);
    if (w <= 0) continue;
    if (!first) tot += gap;
    tot += w; first = false;
  }
  return tot;
}

static inline void homeDrawText(float x, float y, const char *s, float cw, float chh,
                                float gap, float fw, uint16_t col, uint16_t bg) {
  float cur = x;
  for (const char *p = s; *p; p++) {
    char c = *p;
    if (c == ' ') continue;
    bool isNum = (c >= '0' && c <= '9') || c == ':' || c == '%' || c == '-';
    if (isNum) {
      float left = homeNumLeft(c, cw, chh);
      float gx = cur - left;
      uint8_t n = 0;
      const HomeStroke *st = homeNumStrokes(c, n);
      if (st) {
        for (uint8_t k = 0; k < n; k++)
          homeDrawNumStroke(st[k], gx, y, cw, chh, col, bg);
      }
      cur += homeNumWidth(c, cw, chh) + gap;
    } else {
      char uc = (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
      float left = homeLetLeft(uc, cw, fw);
      homeDrawLetter(uc, cur - left, y, cw, chh, fw, col);
      cur += homeLetWidth(uc, cw, fw) + gap;
    }
  }
}
