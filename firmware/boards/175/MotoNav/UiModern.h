#pragma once
#include <Arduino.h>

// =========================================================================
// MotoNav Modern UI Framework (Glassmorphism & Anti-Aliased Vector Primitives)
// 为 M5Stopwatch 打造的现代毛玻璃与抗锯齿矢量 UI 渲染引擎
// =========================================================================

// 1. 半透明毛玻璃圆角矩形 (带有磨砂玻璃质感与边缘微光)
static inline void drawTranslucentRoundRect(int rx, int ry, int rw, int rh, int radius, uint16_t tintColor = 0x0000, uint8_t alpha = 185) {
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) return;
  const int W = LCD_WIDTH;
  const int H = LCD_HEIGHT;
  int invA = 256 - alpha;
  uint32_t tintRB = (tintColor & 0xF81F) * alpha;
  uint32_t tintG  = (tintColor & 0x07E0) * alpha;

  int y0 = max(0, ry);
  int y1 = min(H - 1, ry + rh - 1);
  int r2 = radius * radius;

  for (int y = y0; y <= y1; y++) {
    int dy = 0;
    if (y < ry + radius) dy = (ry + radius) - y;
    else if (y > ry + rh - 1 - radius) dy = y - (ry + rh - 1 - radius);

    int cornerInset = 0;
    if (dy > 0 && radius > 0) {
      int span = (int)sqrtf((float)max(0, r2 - dy * dy));
      cornerInset = radius - span;
    }

    int x0 = max(0, rx + cornerInset);
    int x1 = min(W - 1, rx + rw - 1 - cornerInset);
    if (x0 > x1) continue;

    uint16_t *row = &fb[y * W];
    for (int x = x0; x <= x1; x++) {
      uint16_t c = row[x];
      uint32_t rb = (c & 0xF81F) * invA + tintRB;
      uint32_t g  = (c & 0x07E0) * invA + tintG;
      row[x] = (uint16_t)(((rb >> 8) & 0xF81F) | ((g >> 8) & 0x07E0));
    }
  }
}

// 2. 半透明毛玻璃圆形底座
static inline void drawTranslucentCircle(int cx, int cy, int r, uint16_t tintColor = 0x0000, uint8_t alpha = 140) {
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) return;
  const int W = LCD_WIDTH;
  const int H = LCD_HEIGHT;
  int invA = 256 - alpha;
  uint32_t tintRB = (tintColor & 0xF81F) * alpha;
  uint32_t tintG  = (tintColor & 0x07E0) * alpha;

  int y0 = max(0, cy - r);
  int y1 = min(H - 1, cy + r);
  for (int y = y0; y <= y1; y++) {
    int dy = y - cy;
    int maxDx = (int)sqrtf((float)(r * r - dy * dy));
    int x0 = max(0, cx - maxDx);
    int x1 = min(W - 1, cx + maxDx);
    uint16_t *row = &fb[y * W];
    for (int x = x0; x <= x1; x++) {
      uint16_t c = row[x];
      uint32_t rb = (c & 0xF81F) * invA + tintRB;
      uint32_t g  = (c & 0x07E0) * invA + tintG;
      row[x] = (uint16_t)(((rb >> 8) & 0xF81F) | ((g >> 8) & 0x07E0));
    }
  }
}

// =========================================================================
// 现代 5x7 矢量抗锯齿字模库 (完全告别锯齿方块点阵)
// 支持 0-9, A-Z, :, %, -, +, ., 空格
// =========================================================================

static const uint8_t aaGlyphDigits[10][5] = {
  {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
  {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
  {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
  {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
  {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
  {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
  {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
  {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
  {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
  {0x06, 0x49, 0x49, 0x29, 0x1E}  // 9
};

static const uint8_t aaGlyphLetters[26][5] = {
  {0x7C, 0x12, 0x11, 0x12, 0x7C}, // A
  {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
  {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
  {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
  {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
  {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
  {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
  {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
  {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
  {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
  {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
  {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
  {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
  {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
  {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
  {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
  {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
  {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
  {0x46, 0x49, 0x49, 0x49, 0x31}, // S
  {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
  {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
  {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
  {0x7F, 0x20, 0x18, 0x20, 0x7F}, // W
  {0x63, 0x14, 0x08, 0x14, 0x63}, // X
  {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
  {0x61, 0x51, 0x49, 0x45, 0x43}  // Z
};

static const uint8_t aaGlyphColon[5]   = {0x00, 0x36, 0x36, 0x00, 0x00}; // :
static const uint8_t aaGlyphPercent[5] = {0x63, 0x33, 0x18, 0x0C, 0x66}; // %
static const uint8_t aaGlyphMinus[5]   = {0x08, 0x08, 0x08, 0x08, 0x08}; // -
static const uint8_t aaGlyphPlus[5]    = {0x08, 0x08, 0x3E, 0x08, 0x08}; // +
static const uint8_t aaGlyphDot[5]     = {0x00, 0x60, 0x60, 0x00, 0x00}; // .
static const uint8_t aaGlyphDegree[5]  = {0x00, 0x06, 0x09, 0x06, 0x00}; // * or °

static inline const uint8_t* getAaGlyph(char c) {
  if (c >= '0' && c <= '9') return aaGlyphDigits[c - '0'];
  if (c >= 'A' && c <= 'Z') return aaGlyphLetters[c - 'A'];
  if (c >= 'a' && c <= 'z') return aaGlyphLetters[c - 'a'];
  if (c == ':') return aaGlyphColon;
  if (c == '%') return aaGlyphPercent;
  if (c == '-') return aaGlyphMinus;
  if (c == '+') return aaGlyphPlus;
  if (c == '.') return aaGlyphDot;
  if (c == '*' || c == '^') return aaGlyphDegree;
  return nullptr;
}

// 绘制单个抗锯齿矢量字符 (以 cx, cy 为中心)
static inline void drawAaChar(float cx, float cy, const uint8_t bmp[5], float scale, float dotR, uint16_t color) {
  if (!bmp) return;
  for (int col = 0; col < 5; col++) {
    uint8_t b = bmp[col];
    for (int row = 0; row < 7; row++) {
      if ((b >> row) & 1) {
        float px = cx + (col - 2.0f) * scale;
        float py = cy + (row - 3.0f) * scale;
        aaDisc(px, py, dotR, color);
      }
    }
  }
}

// 绘制抗锯齿字符串 (极简扁平现代无衬线字体，支持居中或左对齐，* 和 ^ 自动映射为 °)
static inline void drawAaString(float cx, float cy, const char *str, float scale, float dotR, float spacing, uint16_t color, bool center = true) {
  (void)dotR;
  (void)spacing;
  if (!str || !str[0]) return;
  const SmoothFont &f = smoothPickFont(scale * 7.2f);
  int x = center ? (int)lroundf(cx) : (int)lroundf(cx - 2.0f * scale);
  int y = (int)lroundf(cy);
  smoothDrawText(f, str, x, y, color, center ? 1 : 0, 1, true);
}

// 3. 精确 SDF 亚像素抗锯齿五角星 (彻底解决锯齿、边缘模糊与填充不均，锐利均匀)
static inline float sdStar5Polygon(float px, float py, const float vx[10], const float vy[10]) {
  float minD2 = 1e9f;
  bool inside = false;
  for (int i = 0, j = 9; i < 10; j = i++) {
    float ex = vx[i] - vx[j], ey = vy[i] - vy[j];
    float wx = px - vx[j], wy = py - vy[j];
    float el2 = ex * ex + ey * ey;
    float t = (el2 > 1e-6f) ? constrain((wx * ex + wy * ey) / el2, 0.0f, 1.0f) : 0.0f;
    float bx = wx - ex * t, by = wy - ey * t;
    float d2 = bx * bx + by * by;
    if (d2 < minD2) minD2 = d2;
    if (((vy[i] > py) != (vy[j] > py)) &&
        (px < (vx[j] - vx[i]) * (py - vy[i]) / (vy[j] - vy[i] + 1e-7f) + vx[i])) {
      inside = !inside;
    }
  }
  float d = sqrtf(minD2);
  return inside ? -d : d;
}

static inline uint16_t uiBlend565(uint16_t bg, uint16_t fg, float alpha) {
  if (alpha <= 0.0f) return bg;
  if (alpha >= 1.0f) return fg;
  uint32_t t = (uint32_t)(alpha * 256.0f + 0.5f);
  uint32_t inv = 256u - t;
  uint32_t r = (((bg >> 11) & 31u) * inv + ((fg >> 11) & 31u) * t) >> 8;
  uint32_t g = (((bg >> 5)  & 63u) * inv + ((fg >> 5)  & 63u) * t) >> 8;
  uint32_t b = ((bg         & 31u) * inv + (fg         & 31u) * t) >> 8;
  return (uint16_t)((r << 11) | (g << 5) | b);
}

// 锐利抗锯齿五角星：cx, cy 为中心, outerR 外径, innerR 内径, strokeW 描边宽
static inline void drawRockstarStar(int cx, int cy, float outerR, float innerR, uint16_t fillColor, uint16_t outlineColor, float strokeW = 1.6f) {
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) return;
  float vx[10], vy[10];
  for (int i = 0; i < 10; i++) {
    float angle = (-90.0f + i * 36.0f) * (float)M_PI / 180.0f;
    float r = (i % 2 == 0) ? outerR : innerR;
    vx[i] = cx + r * cosf(angle);
    vy[i] = cy + r * sinf(angle);
  }
  int pad = (int)ceilf(outerR + strokeW + 2.0f);
  int x0 = max(0, cx - pad), x1 = min(LCD_WIDTH - 1, cx + pad);
  int y0 = max(0, cy - pad), y1 = min(LCD_HEIGHT - 1, cy + pad);
  for (int y = y0; y <= y1; y++) {
    uint16_t *row = fb + y * LCD_WIDTH;
    float py = y + 0.5f;
    for (int x = x0; x <= x1; x++) {
      float sd = sdStar5Polygon(x + 0.5f, py, vx, vy);
      if (sd > strokeW + 0.8f) continue;
      if (fillColor == outlineColor || strokeW <= 0.05f) {
        float a = constrain(0.5f - sd, 0.0f, 1.0f);
        if (a > 0.0f) row[x] = uiBlend565(row[x], fillColor, a);
      } else {
        float aOut = constrain(strokeW + 0.5f - sd, 0.0f, 1.0f);
        if (aOut > 0.0f) row[x] = uiBlend565(row[x], outlineColor, aOut);
        float aIn = constrain(0.35f - sd, 0.0f, 1.0f);
        if (aIn > 0.0f) row[x] = uiBlend565(row[x], fillColor, aIn);
      }
    }
  }
}

// 3b. 随主题自适应的专属收藏图标
static inline void drawThemedFavIcon(int cx, int cy, int btnR, bool isHeader = false) {
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) return;
  int tid = styleIdx();

  if (tid == ST_DRAGON) {
    if (!isHeader) {
      drawTranslucentCircle(cx, cy, btnR, 0x0100, 200);
      gfx->drawCircle(cx, cy, btnR, 0x56E9);
    }
    float ballR = isHeader ? (float)btnR * 0.92f : (float)btnR * 0.72f;
    int rPad = (int)ceilf(ballR + 2.0f);
    int x0 = max(0, cx - rPad), x1 = min(LCD_WIDTH - 1, cx + rPad);
    int y0 = max(0, cy - rPad), y1 = min(LCD_HEIGHT - 1, cy + rPad);
    for (int y = y0; y <= y1; y++) {
      uint16_t *row = fb + y * LCD_WIDTH;
      float dy = (y + 0.5f) - cy;
      for (int x = x0; x <= x1; x++) {
        float dx = (x + 0.5f) - cx;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist > ballR + 0.8f) continue;
        float cov = constrain(ballR + 0.5f - dist, 0.0f, 1.0f);
        float nr = dist / ballR;
        uint16_t col = uiBlend565(0xFEE5, 0xE2A0, constrain(nr * nr, 0.0f, 1.0f));
        if (dist > ballR - 1.5f) col = uiBlend565(col, 0x9180, constrain((dist - (ballR - 1.5f)) / 1.5f, 0.0f, 1.0f));
        float hx = dx + ballR * 0.34f, hy = dy + ballR * 0.36f;
        float hDist = sqrtf(hx * hx + hy * hy);
        if (hDist < ballR * 0.36f) {
          float ha = (1.0f - hDist / (ballR * 0.36f));
          col = uiBlend565(col, 0xFFFF, ha * ha * 0.78f);
        }
        row[x] = uiBlend565(row[x], col, cov);
      }
    }
    float sr = ballR * 0.44f;
    drawRockstarStar(cx, cy + 1, sr, sr * 0.42f, 0xD8A2, 0x9800, isHeader ? 0.5f : 0.8f);
    return;
  }

  if (tid == ST_MINIMAL) {
    uint16_t bgCol   = nightOn ? 0x10A3 : 0xF7DF;
    uint16_t rimCol  = nightOn ? 0x4A8C : 0xC67B;
    uint16_t bmCol   = 0x0B3F;
    if (!isHeader) {
      drawTranslucentCircle(cx, cy, btnR, bgCol, nightOn ? 225 : 235);
      gfx->drawCircle(cx, cy, btnR, rimCol);
      gfx->drawCircle(cx, cy, btnR - 1, nightOn ? 0x2127 : 0xE77E);
    }
    float bw = isHeader ? (btnR * 1.15f) : (btnR * 0.74f);
    float bh = isHeader ? (btnR * 1.40f) : (btnR * 0.92f);
    float hw = bw * 0.5f, hh = bh * 0.5f;
    float cornerR = max(2.0f, bw * 0.18f);
    int x0 = max(0, (int)floorf(cx - hw - 2)), x1 = min(LCD_WIDTH - 1, (int)ceilf(cx + hw + 2));
    int y0 = max(0, (int)floorf(cy - hh - 2)), y1 = min(LCD_HEIGHT - 1, (int)ceilf(cy + hh + 2));
    for (int y = y0; y <= y1; y++) {
      uint16_t *row = fb + y * LCD_WIDTH;
      float py = (y + 0.5f) - cy;
      for (int x = x0; x <= x1; x++) {
        float px = fabsf((x + 0.5f) - cx);
        float dBox;
        if (py < -hh + cornerR && px > hw - cornerR) {
          dBox = hypotf(px - (hw - cornerR), py - (-hh + cornerR)) - cornerR;
        } else {
          dBox = fmaxf(px - hw, fmaxf(-hh - py, py - hh));
        }
        float notchY = (hh - bh * 0.30f) + (px / hw) * (bh * 0.30f);
        float dNotch = py - notchY;
        float sd = fmaxf(dBox, dNotch);
        float a = constrain(0.5f - sd, 0.0f, 1.0f);
        if (a > 0.0f) row[x] = uiBlend565(row[x], bmCol, a);
      }
    }
    return;
  }

  if (tid == 1) {
    if (!isHeader) {
      drawTranslucentCircle(cx, cy, btnR, 0x1022, 210);
      gfx->drawCircle(cx, cy, btnR, 0xFDC0);
      gfx->drawCircle(cx, cy, btnR - 1, 0x4126);
    }
    float sr = isHeader ? (btnR * 0.85f) : (btnR * 0.58f);
    drawRockstarStar(cx, cy, sr, sr * 0.40f, C_WHITE, 0xFDC0, isHeader ? 1.1f : 1.9f);
    return;
  }

  if (!isHeader) {
    drawTranslucentCircle(cx, cy, btnR, 0x0842, 210);
    gfx->drawCircle(cx, cy, btnR, 0x249F);
    gfx->drawCircle(cx, cy, btnR - 1, 0x18E5);
  }
  float headR = isHeader ? (btnR * 0.56f) : (btnR * 0.38f);
  float headY = cy - headR * 0.32f;
  float tipY  = cy + headR * 1.48f;
  aaDisc((float)cx, headY, headR, 0x249F);
  int tx0 = (int)lroundf(cx - headR * 0.86f), tx1 = (int)lroundf(cx + headR * 0.86f);
  int ty0 = (int)lroundf(headY + headR * 0.46f);
  gfx->fillTriangle(tx0, ty0, tx1, ty0, cx, (int)lroundf(tipY), 0x249F);
  aaDisc((float)cx, tipY - 0.5f, 1.3f, 0x249F);
  aaDisc((float)cx, headY, headR * 0.46f, C_WHITE);
  aaDisc((float)cx, headY, headR * 0.20f, 0xFE80);
}

// 4. 动态主题地图背景生成 (根据用户选择的地图主题 themeIdx 和 nightOn，动态呈现对应风格的精美地图底图)
static inline void drawThemedMapBackground() {
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) return;
  const int W = LCD_WIDTH;  // 466
  const int H = LCD_HEIGHT; // 466

  int tid = constrain(themeIdx, 0, THEME_N - 1);
  const MapTheme &th = (nightOn ? THEMES_NIGHT : THEMES)[tid];

  uint16_t c_block = th.block;
  uint16_t c_road  = th.road;
  uint16_t c_park  = th.park;
  uint16_t c_water = th.water;
  uint16_t c_route = th.route;
  uint16_t c_node  = th.route2 ? th.route2 : th.route;
  uint16_t c_blip  = th.blip ? th.blip : C_WHITE;

  // 1. 基础地面底色填充
  gfx->fillScreen(c_block);

  // 特殊主题附加几何元素
  if (tid == ST_MINIMAL) {
    // 极简主题专属底图：微弱色块 + 细腻次路 + 醒目主干道 + 电光蓝导航线
    uint16_t c_major = nightOn ? C_MINIMAL_MAJOR_NIGHT : C_MINIMAL_MAJOR_DAY;
    gfx->fillRect(108, 130, 88, 72, c_park);
    gfx->fillRect(280, 210, 76, 94, c_water);
    gfx->fillRect(156, 304, 96, 62, c_park);
    for (int x = 58; x < W; x += 64) gfx->fillRect(x, 0, 2, H, c_road);
    for (int y = 64; y < H; y += 64) gfx->fillRect(0, y, W, 2, c_road);
    for (int d = -2; d <= 2; d++) {
      gfx->drawLine(24, 248 + d, 326, 310 + d, c_major);
      gfx->drawLine(196 + d, 182, 316 + d, 326, c_major);
      gfx->drawLine(294 + d, 40, 260 + d, 350, c_major);
    }
    gfx->fillRect(CX - 3, 120, 6, 165, c_route);
    gfx->fillRect(CX - 3, 117, 132, 6, c_route);
    aaDisc((float)CX, 120.0f, 5.5f, C_WHITE);
    aaDisc((float)CX, 120.0f, 3.0f, c_route);
    return;
  } else if (tid == ST_DRAGON) {
    // 龙珠雷达：绿色同心网格圆与十字基准线
    uint16_t gridCol = 0x04A0;
    gfx->drawCircle(CX, CX, 72, gridCol);
    gfx->drawCircle(CX, CX, 142, gridCol);
    gfx->drawCircle(CX, CX, 206, gridCol);
    gfx->drawFastHLine(0, CX, W, gridCol);
    gfx->drawFastVLine(CX, 0, H, gridCol);
    aaDisc((float)(CX + 62), (float)(CX - 56), 6.5f, 0xFFE0);
    aaDisc((float)(CX - 78), (float)(CX + 48), 6.5f, 0xFFE0);
  } else if (tid == ST_BLUEPRINT) {
    // 蓝图：工程图纸微细白色方格
    for (int x = 22; x < W; x += 26) gfx->drawFastVLine(x, 0, H, 0x1A94);
    for (int y = 22; y < H; y += 26) gfx->drawFastHLine(0, y, W, 0x1A94);
  } else if (tid == ST_RADAR) {
    // 雷达模式：极坐标同心刻度圆
    uint16_t rCol = 0x0B80;
    gfx->drawCircle(CX, CX, 75, rCol);
    gfx->drawCircle(CX, CX, 150, rCol);
    gfx->drawCircle(CX, CX, 210, rCol);
  }

  // 2. 左侧蜿蜒水面 / 河流 (仅在主题具有独立水体色且非零时绘制)
  if (c_water != 0 && c_water != c_block) {
    for (int y = 0; y < H; y++) {
      float ny = (float)y / 466.0f;
      int wx = (int)(78 + 32.0f * sinf(ny * 3.14159f * 1.5f));
      int x0 = max(0, wx - 22);
      int x1 = min(W - 1, wx + 22);
      for (int x = x0; x <= x1; x++) {
        fb[y * W + x] = c_water;
      }
    }
  }

  // 3. 街区公园绿地 (仅在主题具有独立绿地色且非零时绘制)
  if (c_park != 0 && c_park != c_block && c_park != c_water) {
    gfx->fillRect(140, 70, 52, 44, c_park);
    gfx->fillRect(276, 268, 64, 52, c_park);
    gfx->fillRect(160, 360, 48, 38, c_park);
  }

  // 4. 纵横深色街道网格 (线宽 4px)
  if (c_road != 0 && c_road != c_block) {
    for (int x = 42; x < W; x += 48) {
      gfx->fillRect(x - 2, 0, 4, H, c_road);
    }
    for (int y = 42; y < H; y += 48) {
      gfx->fillRect(0, y - 2, W, 4, c_road);
    }
  }

  // 5. 导航行车历史轨迹 (深灰色线 / 主题次要路线)
  uint16_t c_trail = th.done ? th.done : 0x4208;
  gfx->fillRect(CX - 1, 280, 3, 140, c_trail);

  // 6. 导航路线 (使用当前主题指定的 route 颜色与 node 颜色，线宽 6px，路口直角转弯)
  if (c_route != 0) {
    // 纵向段：从 Y=280 向上延伸至 Y=186
    gfx->fillRect(CX - 3, 186, 6, 94, c_route);
    // 横向段：从 X=233 向右延伸至 X=378
    gfx->fillRect(CX - 3, 183, 148, 6, c_route);
    // 转弯路口外圈小圆环节点
    aaDisc((float)CX, 186.0f, 6.0f, c_node);
    aaDisc((float)CX, 186.0f, 3.0f, c_route);
  }

  // 7. 自车定位航向箭头 (Blip：采用主题专属车标色与黑色外框，朝向路线前方)
  const int blipX = CX;
  const int blipY = 276;
  gfx->fillTriangle(blipX, blipY - 14, blipX - 8, blipY + 6, blipX + 8, blipY + 6, C_BLACK);
  gfx->fillTriangle(blipX, blipY - 11, blipX - 6, blipY + 4, blipX + 6, blipY + 4, c_blip);
  gfx->fillTriangle(blipX, blipY - 1, blipX - 4, blipY + 4, blipX + 4, blipY + 4, C_BLACK);

  // 8. 半透明暗化与暗角融合 (针对不同明暗底色自适应，外圈平滑暗角)
  uint32_t r_b = (c_block >> 11) & 0x1F;
  uint32_t g_b = (c_block >> 5) & 0x3F;
  uint32_t b_b = c_block & 0x1F;
  uint32_t lum = r_b * 8 + g_b * 4 + b_b * 8; // 0 ~ 500
  uint32_t baseRatio = (lum > 180) ? 118 : 205; // 亮色底暗化至 46%，深色底保留 80%

  for (int y = 0; y < H; y++) {
    int dy = y - H / 2;
    uint16_t *row = &fb[y * W];
    for (int x = 0; x < W; x++) {
      int dx = x - W / 2;
      int r2 = dx * dx + dy * dy;

      uint16_t c = row[x];
      uint32_t ratio = baseRatio; // 移除 175px 边缘暗角遮罩，使背景地图完整铺满全屏

      // 安全分离 RGB565 三通道独立缩放，彻底杜绝位移重叠导致的纯蓝色偏！
      uint32_t r = ((c >> 11) & 0x1F) * ratio >> 8;
      uint32_t g = ((c >> 5)  & 0x3F) * ratio >> 8;
      uint32_t b = (c & 0x1F)         * ratio >> 8;
      row[x] = (uint16_t)((r << 11) | (g << 5) | b);
    }
  }
}

// 兼容别名
static inline void drawViceCityBackground() {
  drawThemedMapBackground();
}

