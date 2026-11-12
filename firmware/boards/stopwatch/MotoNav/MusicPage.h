#pragma once
#include <Arduino.h>

extern uint16_t *coverBuf;
extern bool coverValid;
extern uint32_t musicDuration;
extern uint32_t musicPos;
extern uint32_t musicPosAt;
extern bool musicLiked;
extern uint8_t musicRepeatMode;
extern int8_t musicVolume;
extern bool volSliding;
extern uint32_t volActiveUntil;
extern int coverSlideDx;

static inline void formatMusicTime(uint32_t sec, char *buf, size_t len) {
  uint32_t m = sec / 60;
  uint32_t s = sec % 60;
  snprintf(buf, len, "%02lu:%02lu", (unsigned long)m, (unsigned long)s);
}

// 绘制暗色半透明氛围背景：将 180x180 专辑封面拉伸至 466x466 全屏并暗化
static void drawCoverBackgroundDimmed() {
  if (!coverValid || coverBuf == nullptr) {
    gfx->fillScreen(C_BLACK);
    return;
  }
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) {
    gfx->fillScreen(C_BLACK);
    return;
  }
  const int W = LCD_WIDTH;  // 466
  const int H = LCD_HEIGHT; // 466
  for (int y = 0; y < H; y++) {
    int srcY = (y * 180) / H;
    const uint16_t *srcRow = &coverBuf[srcY * 180];
    uint16_t *dstRow = &fb[y * W];
    for (int x = 0; x < W; x++) {
      int srcX = (x * 180) / W;
      uint16_t c = srcRow[srcX];
      // 快速暗化到约 28% 亮度 (保留色相，与纯黑半透明混叠)
      uint32_t rb = (c & 0xF81F) * 70;
      uint32_t g  = (c & 0x07E0) * 70;
      dstRow[x] = (uint16_t)(((rb >> 8) & 0xF81F) | ((g >> 8) & 0x07E0));
    }
  }
}

// 25x20 精美大气心形点阵掩码 (与 50px 底座黄金比例协调，饱满醒目)
static const uint32_t heartFilledMask[20] = {
  0b0000111100000000011110000, // row  0
  0b0011111111000001111111100, // row  1
  0b0111111111100011111111110, // row  2
  0b0111111111110111111111110, // row  3
  0b1111111111111111111111111, // row  4
  0b1111111111111111111111111, // row  5
  0b1111111111111111111111111, // row  6
  0b0111111111111111111111110, // row  7
  0b0011111111111111111111100, // row  8
  0b0001111111111111111111000, // row  9
  0b0000111111111111111110000, // row 10
  0b0000011111111111111100000, // row 11
  0b0000001111111111111000000, // row 12
  0b0000000111111111110000000, // row 13
  0b0000000011111111100000000, // row 14
  0b0000000001111111000000000, // row 15
  0b0000000000111110000000000, // row 16
  0b0000000000011100000000000, // row 17
  0b0000000000001000000000000, // row 18
  0b0000000000001000000000000  // row 19
};

static const uint32_t heartOutlineMask[20] = {
  0b0000111100000000011110000, // row  0
  0b0011000011000001100001100, // row  1
  0b0100000000100010000000010, // row  2
  0b0100000000010100000000010, // row  3
  0b1000000000001000000000001, // row  4
  0b1000000000000000000000001, // row  5
  0b1000000000000000000000001, // row  6
  0b0100000000000000000000010, // row  7
  0b0010000000000000000000100, // row  8
  0b0001000000000000000001000, // row  9
  0b0000100000000000000010000, // row 10
  0b0000010000000000000100000, // row 11
  0b0000001000000000001000000, // row 12
  0b0000000100000000010000000, // row 13
  0b0000000010000000100000000, // row 14
  0b0000000001000001000000000, // row 15
  0b0000000000100010000000000, // row 16
  0b0000000000010100000000000, // row 17
  0b0000000000001000000000000, // row 18
  0b0000000000001000000000000  // row 19
};

static inline void drawHeartIcon(int cx, int cy, uint16_t color, bool filled) {
  const uint32_t *mask = filled ? heartFilledMask : heartOutlineMask;
  int startX = cx - 12;
  int startY = cy - 10;
  for (int r = 0; r < 20; r++) {
    uint32_t rowBits = mask[r];
    for (int c = 0; c < 25; c++) {
      if ((rowBits >> (24 - c)) & 1) {
        gfx->drawPixel(startX + c, startY + r, color);
      }
    }
  }
}

// 将 180x180 的 coverBuf 高速等比放大绘制至 dstSize x dstSize (例如 206x206)
static void drawScaledCover(int dstX, int dstY, int dstSize) {
  if (!coverBuf) return;
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) return;
  const int W = LCD_WIDTH;
  const int H = LCD_HEIGHT;
  const int srcSize = 180;
  for (int y = 0; y < dstSize; y++) {
    int py = dstY + y;
    if (py < 0 || py >= H) continue;
    int srcY = (y * srcSize) / dstSize;
    const uint16_t *srcRow = &coverBuf[srcY * srcSize];
    uint16_t *dstRow = &fb[py * W];
    for (int x = 0; x < dstSize; x++) {
      int px = dstX + x;
      if (px < 0 || px >= W) continue;
      int srcX = (x * srcSize) / dstSize;
      dstRow[px] = srcRow[srcX];
    }
  }
}

// 绘制与 VOLUME 字母风格完全一致的音量百分比字符串 (水平居中)
static void drawVolumeDigits(int cx, int cy, int vol, uint16_t color) {
  char str[10];
  snprintf(str, sizeof(str), "%d%%", vol);
  drawAaString((float)cx, (float)cy, str, 2.1f, 1.15f, 14.0f, color, true);
}

// 绘制 iPhone 风格小巧中央音量弹窗 HUD (显示在屏幕正正中间，统一抗锯齿无衬线字体)
static void drawCenterVolumeHUD(int vol) {
  // 小巧精致的 116x116 卡片，居中位于屏幕正正中心 (CX=233, CY=233)
  const int cardW = 116;
  const int cardH = 116;
  const int cardX = CX - cardW / 2; // 175
  const int cardY = CX - cardH / 2; // 175

  // 1. 半透明毛玻璃圆角底座 (75% 纯黑磨砂混合)
  drawTranslucentRoundRect(cardX, cardY, cardW, cardH, 18, 0x0000, 192);

  // 2. 矢量喇叭图标 (水平居中于 CX，Y = cardY + 28 = 203)
  const int spkX = CX - 6;
  const int spkY = cardY + 28;

  // 喇叭后腔矩形与扩散口
  gfx->fillRect(spkX - 10, spkY - 5, 6, 10, C_WHITE);
  gfx->fillTriangle(spkX - 4, spkY - 5, spkX - 4, spkY + 5, spkX + 4, spkY + 10, C_WHITE);
  gfx->fillTriangle(spkX - 4, spkY - 5, spkX + 4, spkY + 10, spkX + 4, spkY - 10, C_WHITE);

  // 喇叭声波 (音量为 0 静音时右侧只显示单条利落斜杠)
  if (vol == 0) {
    gfx->drawLine(spkX + 8, spkY - 6, spkX + 16, spkY + 6, C_WHITE);
  } else {
    const int arcRadii[3] = {8, 13, 18};
    int numArcs = (vol > 70) ? 3 : (vol > 30 ? 2 : 1);
    for (int a = 0; a < numArcs; a++) {
      int r = arcRadii[a];
      for (int deg = -40; deg <= 40; deg += 4) {
        float rad = deg * (float)M_PI / 180.0f;
        int px = (int)lroundf(spkX + 4 + r * cosf(rad));
        int py = (int)lroundf(spkY + r * sinf(rad));
        gfx->drawPixel(px, py, C_WHITE);
      }
    }
  }

  // 3. iOS 风格水平胶囊音量条 (居中位于屏幕中心线 Y = 233)
  const int barW = 86;
  const int barH = 8;
  const int barX = CX - barW / 2; // 190
  const int barY = cardY + 58;

  // 底槽
  gfx->fillRoundRect(barX, barY - 4, barW, barH, 4, 0x2124);
  // 填充白色高光亮条
  int fillW = constrain((barW * vol) / 100, (vol > 0 ? 6 : 0), barW);
  if (fillW > 0) {
    gfx->fillRoundRect(barX, barY - 4, fillW, barH, 4, C_WHITE);
  }
  gfx->drawRoundRect(barX, barY - 4, barW, barH, 4, 0x4A69);

  // 4. 百分比数字：使用与 VOLUME 字母一模一样的矢量抗锯齿平滑字体 (无锯齿点阵)
  drawVolumeDigits(CX, cardY + 88, vol, C_WHITE);
}

// VOLUME 字符抗锯齿切线旋转排布 (5x7 矢量点阵)
static const uint8_t volumeCharBitmaps[6][5] = {
  {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
  {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
  {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
  {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
  {0x7F, 0x02, 0x1C, 0x02, 0x7F}, // M
  {0x7F, 0x49, 0x49, 0x49, 0x41}  // E
};

// VOLUME 字母放到圆屏 9 点钟位置，紧贴屏幕边缘
static void drawVolumeTextRotated(uint16_t color) {
  const float R_text = 211.0f; // 紧贴音量轨内侧，极大贴近圆屏外边缘
  // 9 点钟方向为 p = 0.75，字母依序在 9 点钟上下沿切线展开
  const float p_start = 0.785f; // V 在上方 (约 9 点 25 分)
  const float p_end   = 0.715f; // E 在下方 (约 8 点 35 分)
  const float scale = 1.6f;

  for (int i = 0; i < 6; i++) {
    float p = p_start - i * ((p_start - p_end) / 5.0f);
    float a = p * 2.0f * (float)M_PI;
    float cx = CX + R_text * sinf(a);
    float cy = CX - R_text * cosf(a);
    const uint8_t *bmp = volumeCharBitmaps[i];

    float cosA = cosf(a), sinA = sinf(a);
    for (int col = 0; col < 5; col++) {
      uint8_t b = bmp[col];
      for (int row = 0; row < 7; row++) {
        if ((b >> row) & 1) {
          float lx = (col - 2.0f) * scale;
          float ly = (row - 3.0f) * scale;
          float px = cx - lx * cosA + ly * sinA;
          float py = cy - lx * sinA - ly * cosA;
          aaDisc(px, py, 1.15f, color);
        }
      }
    }
  }
}

// 绘制 9 点钟贴边 VOLUME 字符 (作为滑动触控位置提示，进度与数字在中央 HUD 呈现)
static void drawVolumeArc(int vol, bool showActive) {
  drawVolumeTextRotated(showActive ? 0xFFFF : 0x8410);
}

// 绘制全屏沉浸式多行歌词界面 (背景保持专辑暗化模糊，高亮放大当前在唱行，平滑翻滚动效)
static void drawLyricsMode() {
  drawCoverBackgroundDimmed();
  onWaitScreen = false;
  frameIsMap = false;

  bool linked = bleConnected && bleAuthed;

  // 1. 顶部小标题 (精致呈现当前歌名)
  if (linked || (musicHave && imgs[5].showW > 0)) {
    drawTextImg(5, 42, 0x88);
  }

  // 2. 歌词五行立体排布 (去掉进度条，文字放大两行，当前行支持横向平滑滚动)
  int yOffset = (int)lroundf(lyricScrollOffset);
  bool hasLyrics = (imgs[12].show && imgs[12].showW > 0);

  if (hasLyrics) {
    // 上上句歌词 (槽位 15)：字号微小，0x40 暗调
    if (imgs[15].show && imgs[15].showW > 0) {
      int prev2Y = CX - 100 - imgs[15].showH / 2 + yOffset;
      drawTextImg(15, prev2Y, 0x40);
    }
    // 上一句歌词 (槽位 13)：字号中等，0x78 柔灰，形成纵深聚焦对比
    if (imgs[13].show && imgs[13].showW > 0) {
      int prevY = CX - 52 - imgs[13].showH / 2 + yOffset;
      drawTextImg(13, prevY, 0x78);
    }
    // 当前在唱聚焦行 (槽位 12)：字号最大，0xFF 全白高亮，居中吸睛；超出屏幕长度时自动跑马灯横向滚动
    int curY = CX - imgs[12].showH / 2 + yOffset;
    drawTextImgLyricScroll(12, curY, 0xFF, 420);

    // 下一句歌词 (槽位 14)：字号中等，0x78 柔灰
    if (imgs[14].show && imgs[14].showW > 0) {
      int nextY = CX + 52 - imgs[14].showH / 2 + yOffset;
      drawTextImg(14, nextY, 0x78);
    }
    // 下下句歌词 (槽位 16)：字号微小，0x40 暗调
    if (imgs[16].show && imgs[16].showW > 0) {
      int next2Y = CX + 100 - imgs[16].showH / 2 + yOffset;
      drawTextImg(16, next2Y, 0x40);
    }
  } else {
    // 暂无歌词或未加载完成时的优雅微光居中状态
    if (linked && musicHave) {
      printCentered("LYRICS", CX, CX - 36, 2, 0x6B6D);
      printCentered("NO SYNCED LYRICS", CX, CX, 2, C_WHITE);
      if (imgs[6].show && imgs[6].showW > 0) {
        drawTextImg(6, CX + 36, 0x88);
      }
    } else {
      printCentered("NO MUSIC PLAYING", CX, CX, 2, C_GREY);
    }
  }

  // 3. 底部页面指示点
  drawPageDots();

  needFlush = true;
}

// 音乐页：沉浸式半透明背景、大专辑封面、内嵌播放键、封面右下角Like、弧形贴边音量、滑动切歌
void drawMusic() {
  if (musicLyricsView) {
    drawLyricsMode();
    return;
  }
  drawCoverBackgroundDimmed();
  onWaitScreen = false;
  frameIsMap = false;

  bool linked = bleConnected && bleAuthed;

  // 1. 贴合圆弧的音量条 (默认仅显示VOLUME，按住触发抗锯齿圆弧显示)
  bool showVol = volSliding || (millis() < volActiveUntil);
  drawVolumeArc(musicVolume, showVol);

  // 2. 专辑封面区 (尺寸放大至 206x206，位置下移至 Y: 48 ~ 254，离线保留显示)
  const int covSize = 206;
  const int covBaseX = CX - covSize / 2; // 130
  const int covBaseY = 48;

  // 原位置常驻半透明底图矩形边框与唱盘托架（切歌划开封面时自然露出底座）
  gfx->fillRoundRect(covBaseX - 3, covBaseY - 3, covSize + 6, covSize + 6, 16, 0x18C3);
  gfx->drawRoundRect(covBaseX - 3, covBaseY - 3, covSize + 6, covSize + 6, 16, 0x4A69);
  gfx->drawRoundRect(covBaseX - 1, covBaseY - 1, covSize + 2, covSize + 2, 14, 0x2945);
  // 底座内部同心凹槽（留声机托盘）
  gfx->drawCircle(CX, covBaseY + covSize / 2, 60, 0x2945);
  gfx->drawCircle(CX, covBaseY + covSize / 2, 26, 0x2945);

  // 封面实际绘制坐标（随滑动/离开动画水平偏移）
  int drawCovX = covBaseX + coverSlideDx;

  if (coverValid && coverBuf != nullptr) {
    gfx->fillRoundRect(drawCovX - 2, covBaseY - 2, covSize + 4, covSize + 4, 14, 0x2124);
    drawScaledCover(drawCovX, covBaseY, covSize);
    gfx->drawRoundRect(drawCovX - 2, covBaseY - 2, covSize + 4, covSize + 4, 14, 0x5AEB);
  } else {
    // 拟物黑胶唱片
    const int discR = 98;
    const int discX = drawCovX + covSize / 2;
    const int discY = covBaseY + covSize / 2;
    aaDisc(discX, discY, discR, 0x10A2);
    gfx->drawCircle(discX, discY, 78, 0x2124);
    gfx->drawCircle(discX, discY, 56, 0x2124);
    aaDisc(discX, discY, 36, 0x249F);
    aaDisc(discX, discY, 10, C_BLACK);
    gfx->drawCircle(discX, discY, 10, 0xDEFB);
  }

  // 3. 嵌入专辑封面的播放/暂停按钮 (暂停时才显示大播放三角，半透明毛玻璃底座)
  if (!musicPlaying && abs(coverSlideDx) < 60) {
    int pcx = drawCovX + covSize / 2, pcy = covBaseY + covSize / 2;
    drawTranslucentCircle(pcx, pcy, 32, 0x0000, 140);
    gfx->fillTriangle(pcx - 9, pcy - 15, pcx - 9, pcy + 15, pcx + 15, pcy, C_WHITE);
  }

  // 4. 嵌入专辑封面右下角的 Like 按钮 (加大至 50px 纯黑半透明底座，无论点亮与否背景绝不变红)
  const int likeX = drawCovX + covSize - 30;
  const int likeY = covBaseY + covSize - 30;
  drawTranslucentCircle(likeX, likeY, 25, 0x0000, 150);
  gfx->drawCircle(likeX, likeY, 25, 0x39E7); // 精致微光半透明边框
  drawHeartIcon(likeX, likeY, musicLiked ? 0xF800 : 0xCE79, musicLiked);

  // 6. 歌曲进度条与大号时间数字 (下移至 Y: 270 ~ 302，宽度自适应放大)
  if (musicHave && musicDuration > 0) {
    uint32_t curSec = musicPos;
    if (musicPlaying && linked) {
      uint32_t elapsed = (millis() - musicPosAt) / 1000;
      curSec = min(musicDuration, musicPos + elapsed);
    }
    const int barW = 260;
    const int barH = 8;
    const int barX = CX - barW / 2; // 103
    const int barY = 270;

    // 进度条背景
    gfx->fillRoundRect(barX, barY, barW, barH, 4, 0x2124);

    // 进度指示
    float progress = constrain((float)curSec / (float)musicDuration, 0.0f, 1.0f);
    int fillW = max(6, (int)(barW * progress));
    gfx->fillRoundRect(barX, barY, fillW, barH, 4, 0x07FF); // C_CYAN
    gfx->fillCircle(barX + fillW, barY + barH / 2, 6, C_WHITE);

    // 大号时间文字 (当前时间 / 总时长)
    char timeCur[16], timeTotal[16];
    formatMusicTime(curSec, timeCur, sizeof(timeCur));
    formatMusicTime(musicDuration, timeTotal, sizeof(timeTotal));
    gfx->setTextColor(0xCE79);
    gfx->setTextSize(2);
    gfx->setCursor(barX, barY + barH + 6);
    gfx->print(timeCur);
    gfx->setCursor(barX + barW - 60, barY + barH + 6);
    gfx->print(timeTotal);
  }

  // 7. 歌名与歌手 (下移至 Y: 322, 356)
  if (hidMusicMode && linked && !appleMedia.available) {
    printCentered("PHONE MEDIA KEYS", CX, 324, 2, C_WHITE);
    printCentered("OPEN MUSIC APP ON PHONE", CX, 356, 2, C_GREY);
  } else if (linked || (musicHave && imgs[5].showW > 0)) {
    drawTextImg(5, 322, 0xFF);
    drawTextImg(6, 356, 0xBB);
  } else {
    printCentered("PHONE OFFLINE", CX, 336, 2, C_GREY);
  }

  // 8. 底部页面指示点
  drawPageDots();

  // 9. iPhone 风格半透明中央音量弹窗 HUD (调节音量时浮现于屏幕正中央)
  if (showVol) {
    drawCenterVolumeHUD(musicVolume);
  }

  needFlush = true;
}
