#pragma once
#include <stdint.h>

// 圆环快色表：把逐像素的 bandCov64+blendF 预计算成按半径索引的颜色表。
// 借鉴 Koo0527/MotoNav_sdgoods185 的同名优化，适配本板三层圆环结构。
// 只在 alertAlpha==0（无超速/无测速预警，绝大多数帧）时启用；
// 有预警光晕时调用方回退到原来的慢路径 ringPixelSlow。
namespace RingFastColors {
// 覆盖半径 210~236（黑边与圆环所在范围），单位 = 半径×64。
// 210 以下区域不含任何环带，交由慢路径保留底图原色，避免误涂黑。
static constexpr int R_LO = 210 * 64;
static constexpr int R_HI = 236 * 64 + 63;
static constexpr int Count = R_HI - R_LO + 1;

// 相位位：bit0 = 内侧路口环(prog) 走过的灰 vs 亮色；bit1 = 外侧行程环(trip) 走过 vs 未走
static constexpr int PHASES = 4;

struct Key {
  bool track;          // 是否画内侧路口环
  bool trip;           // 是否画外侧行程环 (trip>=0)
  bool prog;           // 是否有进度 (prog>0.002)
  int halfWidth;       // 内侧环半宽 (ph)
  uint16_t pcol;       // 内侧环亮色
};

static inline bool same(const Key& a, const Key& b) {
  return a.track == b.track && a.trip == b.trip && a.prog == b.prog &&
         a.halfWidth == b.halfWidth && a.pcol == b.pcol;
}

// 覆盖率 0~64：r64 = 半径×64；带宽 [a,b]（单位 64 分之一像素）
static inline int coverage(int r64, int a, int b) {
  int x = r64 - (a * 64 - 32), y = (b * 64 + 32) - r64;
  int c = x < y ? x : y;
  return c <= 0 ? 0 : (c >= 64 ? 64 : c);
}

static inline uint16_t blend(uint16_t a, uint16_t b, unsigned t) {
  unsigned s = 256 - t;
  return (uint16_t)(((((a >> 11) * s + (b >> 11) * t) >> 8) << 11) |
                    (((((a >> 5) & 63) * s + ((b >> 5) & 63) * t) >> 8) << 5) |
                    (((a & 31) * s + (b & 31) * t) >> 8));
}

// 不含 alert 的圆环基础色。phase: bit0=prog done, bit1=trip done
static inline uint16_t color(int r64, int phase, const Key& key,
                             int turnR, int tripR, int tripH,
                             uint16_t cTrack, uint16_t tripDone, uint16_t tripLeft,
                             uint16_t cBlack) {
  uint16_t c = cBlack;  // 环带内底色（黑）；bandCov(211,240) 黑边由调用方背景保证
  int a;
  // 内侧路口环
  if (key.track && (a = coverage(r64, turnR - key.halfWidth, turnR + key.halfWidth))) {
    uint16_t rc = (phase & 1) ? cTrack : key.pcol;
    c = a >= 64 ? rc : blend(c, rc, a * 4);
  }
  // 外侧行程环
  if (key.trip && (a = coverage(r64, tripR - tripH, tripR + tripH))) {
    uint16_t tc = (phase & 2) ? tripDone : tripLeft;
    c = a >= 64 ? tc : blend(c, tc, a * 4);
  }
  return c;
}
}  // namespace RingFastColors
