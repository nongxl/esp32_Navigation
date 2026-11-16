#pragma once
#include <Arduino.h>

// ============================================================
// 半圆时速表视图（参考用户设计图）
//   270° 马蹄形刻度盘 0~160 km/h，已行驶区间红色高亮
//   中央大号时速 + km/h，顶部绿色 GPS 点
//   底部白色圆角框：加粗矢量箭头 + 仿电子管（七段数码管）距离
//   纯线条 + 内置字体绘制，无位图，开销极低
// ============================================================

// ---------- 白底黑箭头的抗锯齿光栅化 ----------
// 复用固件已有的矢量箭头造型（drawTurn / drawUturn：圆角转弯、锥形箭头），
// 只把“记录-合成”环节换成本地版本：逐像素取所有图形的覆盖率并集（max），
// 黑->白混色。这样线段接缝不会被二次混合吃掉，边缘平滑且不会断点。
#define SPO_BUF 52

static inline float spoClamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }

static float spoPrimCov(const Prim &p, float px, float py) {
  if (p.type == 0) {                                  // 线段：起点平头，终点圆头
    float dx = p.bx - p.ax, dy = p.by - p.ay;
    float len2 = dx * dx + dy * dy;
    float qx = px - p.ax, qy = py - p.ay;
    if (len2 < 1e-4f) return spoClamp01(p.r - sqrtf(qx * qx + qy * qy) + 0.5f);
    float h = (qx * dx + qy * dy) / len2;
    if (h < 0) {
      float len = sqrtf(len2);
      float perp = fabsf(qx * dy - qy * dx) / len;
      return spoClamp01(p.r - perp + 0.5f) * spoClamp01(0.5f + h * len);
    }
    if (h > 1) h = 1;
    float ex = qx - dx * h, ey = qy - dy * h;
    return spoClamp01(p.r - sqrtf(ex * ex + ey * ey) + 0.5f);
  }
  if (p.type == 1) {                                  // 三角形
    float vx[3] = {p.ax, p.bx, p.cx}, vy[3] = {p.ay, p.by, p.cy};
    float area = (vx[1] - vx[0]) * (vy[2] - vy[0]) - (vy[1] - vy[0]) * (vx[2] - vx[0]);
    float sgn = area > 0 ? 1.0f : -1.0f;
    float m = 1e9f;
    for (int i = 0; i < 3; i++) {
      int j = (i + 1) % 3;
      float ex = vx[j] - vx[i], ey = vy[j] - vy[i];
      float len = sqrtf(ex * ex + ey * ey);
      if (len < 1e-4f) return 0;
      float d = sgn * ((px - vx[i]) * ey - (py - vy[i]) * ex) / len;
      if (-d < m) m = -d;
    }
    return spoClamp01(m + 0.5f);
  }
  float qx = px - p.ax, qy = py - p.ay;                // 圆
  return spoClamp01(p.r - sqrtf(qx * qx + qy * qy) + 0.5f);
}

static uint16_t spoArBuf[SPO_BUF * SPO_BUF];
static int spoArW = 0, spoArH = 0, spoArX = 0, spoArY = 0;
static uint32_t spoArKeyA = 0xFFFFFFFFu, spoArKeyB = 0xFFFFFFFFu;
static bool spoArOk = false;

static uint32_t spoStrHash(const String &s, uint32_t h) {
  for (size_t i = 0; i < s.length(); i++) { h ^= (uint8_t)s[i]; h *= 16777619u; }
  return h;
}

// 只有方向/角度/尺寸变化时才重新光栅化，其余帧直接复用缓存
static void spoBuildArrow(int cx, int cy, int half, const String &dir) {
  uint32_t k2 = (uint32_t)(half * 131) ^ (uint32_t)(isnan(navAngle) ? 0 : ((int)navAngle * 7919));
  uint32_t k1 = spoStrHash(dir, 2166136261u);
  if (spoArOk && k1 == spoArKeyA && k2 == spoArKeyB) return;
  spoArKeyA = k1; spoArKeyB = k2;

  // 到达：实心圆环（不走矢量路径，避免内部挖空需要“减”运算）
  if (dir == "A") {
    float r = (float)(half - 2);
    gfx->fillCircle(cx, cy, r, C_BLACK);
    gfx->fillCircle(cx, cy, r * 5.0f / 11.0f, C_WHITE);
    spoArOk = false;
    return;
  }

  int top = cy - half, bot = cy + half;
  iconBegin();
  float deg;
  if (dir == "U")                          drawUturn(0xFFFF, (float)cy, 0.50f);
  else if (dirAngle(dir, deg))             drawTurn(deg, 0xFFFF, (float)cy, 0.52f);
  else                                     drawTurn(0, 0xFFFF, (float)cy, 0.40f);   // 直行
  iconFitAt((float)cx, (float)top, (float)bot, (float)(half * 2));
  recIcon = false;                          // 停止记录，接下来自己逐像素合成
  if (primN == 0) { spoArOk = false; return; }

  // 外框（含 2px 抗锯齿余量），裁到屏幕与缓存尺寸内
  int bx0 = cx - half - 2, bx1 = cx + half + 2;
  int by0 = top - 2, by1 = bot + 2;
  if (bx0 < 0) bx0 = 0;
  if (by0 < 0) by0 = 0;
  if (bx1 > LCD_WIDTH - 1) bx1 = LCD_WIDTH - 1;
  if (by1 > LCD_HEIGHT - 1) by1 = LCD_HEIGHT - 1;
  int w = bx1 - bx0 + 1, h = by1 - by0 + 1;
  if (w <= 0 || h <= 0) { spoArOk = false; return; }
  if (w > SPO_BUF) w = SPO_BUF;
  if (h > SPO_BUF) h = SPO_BUF;

  for (int y = 0; y < h; y++) {
    float py = by0 + y + 0.5f;
    for (int x = 0; x < w; x++) {
      float px = bx0 + x + 0.5f;
      float a = 0;
      for (int i = 0; i < primN; i++) {
        const Prim &p = prims[i];
        if (px < p.x0 || px > p.x1 || py < p.y0 || py > p.y1) continue;
        float v = spoPrimCov(p, px, py);
        if (v > a) a = v;
      }
      int lv = (int)lroundf(255.0f * (1.0f - a));     // 白底黑字：完全覆盖→纯黑
      spoArBuf[y * SPO_BUF + x] = (uint16_t)(((lv >> 3) << 11) | ((lv >> 2) << 5) | (lv >> 3));
    }
  }
  spoArX = bx0; spoArY = by0; spoArW = w; spoArH = h;
  spoArOk = true;
}

static void spoDrawArrow(int cx, int cy, int half, const String &dir) {
  spoBuildArrow(cx, cy, half, dir);
  if (!spoArOk) return;
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) return;
  for (int y = 0; y < spoArH; y++) {
    int py = spoArY + y;
    if (py < 0 || py >= LCD_HEIGHT) continue;
    memcpy(fb + (size_t)py * LCD_WIDTH + spoArX, spoArBuf + y * SPO_BUF, spoArW * 2);
  }
}

// ---------- 仿电子管（七段数码管）数字 ----------
static void spoSegChar(int x, int y, int w, int h, int t, char c, uint16_t col) {
  bool A = false, B = false, C = false, D = false, E = false, F = false, G = false;
  switch (c) {
    case '0': A = B = C = D = E = F = true; break;
    case '1': B = C = true; break;
    case '2': A = B = G = E = D = true; break;
    case '3': A = B = G = C = D = true; break;
    case '4': F = G = B = C = true; break;
    case '5': A = F = G = C = D = true; break;
    case '6': A = F = G = E = D = C = true; break;
    case '7': A = B = C = true; break;
    case '8': A = B = C = D = E = F = G = true; break;
    case '9': A = B = C = D = F = G = true; break;
    case '-': G = true; break;
    default: return;
  }
  int half = h / 2;
  int r = t / 2;                                   // 圆头段，更像电子管
  int ht = half - t / 2;                           // 竖段半高（含端点重叠，拐角不留缝）
  if (A) gfx->fillRoundRect(x + t / 2, y, w - t, t, r, col);
  if (G) gfx->fillRoundRect(x + t / 2, y + half - t / 2, w - t, t, r, col);
  if (D) gfx->fillRoundRect(x + t / 2, y + h - t, w - t, t, r, col);
  if (F) gfx->fillRoundRect(x, y + t / 2, t, ht, r, col);
  if (B) gfx->fillRoundRect(x + w - t, y + t / 2, t, ht, r, col);
  if (E) gfx->fillRoundRect(x, y + half, t, ht, r, col);
  if (C) gfx->fillRoundRect(x + w - t, y + half, t, ht, r, col);
}

// 右对齐绘制距离数字：右边界固定，单位另画在固定位置，数字位数变化时不会推动单位
static void spoDrawSegNumber(const String &num, int rightX, int cy, uint16_t col) {
  int n = num.length();
  if (n == 0) return;
  int w, h, t;
  if (n <= 1)      { w = 28; h = 38; t = 6; }
  else if (n == 2) { w = 25; h = 36; t = 6; }
  else if (n == 3) { w = 20; h = 32; t = 5; }
  else if (n == 4) { w = 17; h = 27; t = 4; }
  else             { w = 14; h = 23; t = 3; }
  int gap = t > 3 ? t - 1 : 3;

  int total = 0;                                   // 先算总宽度，才能右对齐
  for (int i = 0; i < n; i++) {
    total += (num[i] == '.') ? (t + 1) : w;
    if (i) total += gap;
  }
  int x = rightX - total;
  int y = cy - h / 2;
  for (int i = 0; i < n; i++) {
    char ch = num[i];
    if (ch == '.') {
      gfx->fillRect(x, y + h - t, t + 1, t, col);
      x += t + 1 + gap;
      continue;
    }
    spoSegChar(x, y, w, h, t, ch, col);
    x += w + gap;
  }
}

// 居中绘制大号七段数码管数字（表盘中央时速）：整体水平/垂直居中，尺寸随时速位数自适应
static void spoDrawBigSegNumber(const String &num, int cx, int cy, uint16_t col) {
  int n = num.length();
  if (n == 0) return;
  int w, h, t;
  if (n <= 1)      { w = 58; h = 92; t = 16; }
  else if (n == 2) { w = 54; h = 90; t = 15; }
  else             { w = 46; h = 84; t = 13; }   // 3 位（0~160）
  int gap = t - 1;

  int total = 0;                                 // 先算总宽度，才能整体居中
  for (int i = 0; i < n; i++) {
    total += (num[i] == '.') ? (t + 1) : w;
    if (i) total += gap;
  }
  int x = cx - total / 2;
  int y = cy - h / 2;
  for (int i = 0; i < n; i++) {
    char ch = num[i];
    if (ch == '.') {
      gfx->fillRect(x, y + h - t, t + 1, t, col);
      x += t + 1 + gap;
      continue;
    }
    spoSegChar(x, y, w, h, t, ch, col);
    x += w + gap;
  }
}

// 画一整帧时速表
void drawSpeedometer() {
  onWaitScreen = false;
  gfx->fillScreen(C_BLACK);

  const float CXF = (float)CX;
  const float CY = 236.0f;                               // 表盘圆心略偏上，给底部提示框留空间
  const float R_OUT = 205.0f, R_TICK_IN = 179.0f;        // 刻度外沿 / 短刻度内沿
  const float R_TICK_BIG = 164.0f;                       // 大刻度内沿（更长）
  const float R_NUM = 146.0f;                            // 数字半径
  const float A0 = 135.0f, A1 = 405.0f;                  // 起点(左下) → 终点(右下)，跨 270°
  const int MAXK = 160;

  int spd = speedKmh < 0 ? 0 : constrain(speedKmh, 0, MAXK);
  bool over = hasCam && camLimit > 0 && speedKmh > camLimit;

  // ---- 马蹄形刻度：每 5 km/h 一格，每 20 大格；已行驶(<=当前速度)红色，未到暗灰 ----
  for (int v = 0; v <= MAXK; v += 5) {
    float a = (A0 + (A1 - A0) * (float)v / MAXK) * PI / 180.0f;
    float ca = cosf(a), sa = sinf(a);
    bool major = (v % 20 == 0);
    float rin = major ? R_TICK_BIG : R_TICK_IN;
    uint16_t col = (v <= spd) ? (over ? 0xFC10 : C_RED) : 0xC618;
    if (v == 0) col = C_RED;
    float w = major ? 6.4f : 2.4f;                       // 带数字的大刻度明显加粗、加长
    thickLine(CXF + ca * rin, CY + sa * rin, CXF + ca * R_OUT, CY + sa * R_OUT, w, col);

    if (major) {                                         // 大刻度数字
      gfx->setTextSize(2);
      gfx->setTextColor(col);
      String s = String(v);
      int tw = s.length() * 6 * 2;
      gfx->setCursor((int)(CXF + ca * R_NUM) - tw / 2, (int)(CY + sa * R_NUM) - 7);
      gfx->print(s);
    }
  }

  // ---- 顶部绿色 GPS 点（带一层浅色光晕）----
  aaDisc(CXF, 150.0f, 9.0f, 0x07E0);
  aaDisc(CXF, 150.0f, 5.0f, 0xAFF6);

  // ---- 中央大号时速：白色仿电子管（七段数码管）字体，超速变红 ----
  int shown = speedKmh < 0 ? 0 : speedKmh;
  spoDrawBigSegNumber(String(shown), (int)CXF, 210, over ? C_RED : C_WHITE);
  printCentered("km/h", (int)CXF, 268, 2, over ? 0xFC10 : 0xAD75);

  // ---- 底部白色圆角框：加粗矢量箭头 + 仿电子管距离 + 固定单位 ----
  const int bw = 210, bh = 54, bx = (int)CXF - bw / 2, by = 356;
  gfx->fillRoundRect(bx, by, bw, bh, 12, C_WHITE);

  spoDrawArrow(bx + 30, by + bh / 2, 21, navDir);        // 箭头整体收在框内，含抗锯齿余量

  String dv = distText(navDist);
  String num = dv, unit = "m";
  if (navDist < 0) { num = "--"; unit = "m"; }
  else if (dv.endsWith("km")) { num = dv.substring(0, dv.length() - 2); unit = "km"; }
  else if (dv.endsWith("m"))  { num = dv.substring(0, dv.length() - 1); unit = "m"; }

  spoDrawSegNumber(num, bx + 172, by + bh / 2, C_BLACK);  // 右边界固定 → 位数变化不左右跳
  gfx->setTextSize(2);
  gfx->setTextColor(C_BLACK);
  gfx->setCursor(bx + 178, by + bh / 2 + 6);              // 单位位置固定，不随数字摇摆
  gfx->print(unit);

  // ---- 超速拍照 / 边缘告警沿用原逻辑 ----
  uint32_t t = millis();
  if (over) drawEdgeAlertGlow(C_RED, (uint8_t)(85 + 95 * (0.5f + 0.5f * sinf(t * 0.012f))));
  else if (hasCam && camDist > 0 && camDist <= 300) drawEdgeAlertGlow(C_ORANGE, (uint8_t)(65 + 75 * (0.5f + 0.5f * sinf(t * 0.007f))));

  needFlush = true;
}
