#pragma once
#include <Arduino.h>
#include "UiModern.h"
#include "WaitingScreen.h"

// 阶段一特性：一键沿途顺路加油站提示与一键加途经点改线卡片
inline void drawAlongPoiBanner(unsigned long t = millis()) {
  // 1. 如果正在搜索中：显示微型加载提示
  if (alongSearching) {
    if (t > alongSearchUntil) {
      alongSearching = false;
      alongSearchRequested = false;
      return;
    }
    const int W = 220, H = 32, Y = 68;
    const int X = CX - W / 2;
    drawTranslucentRoundRect(X, Y, W, H, 16, 0x0000, 195);
    gfx->drawRoundRect(X, Y, W, H, 16, 0xFDE0);
    aaDisc(X + 20, Y + H / 2, 4.0f, 0xFDE0);
    if (isChineseLang) {
      drawTextUTF8(X + 34, Y + 22, "正在寻找顺路加油站...", C_WHITE);
    } else {
      drawAaString(X + 110, Y + H / 2, "SEARCHING GAS...", 1.3f, 0.8f, 9.0f, C_WHITE, true);
    }
    return;
  }

  // 2. 如果收到搜索结果且尚未过期：渲染顺路加油站卡片
  if (alongPoi.active) {
    if (t > alongPoi.expireAt) {
      alongPoi.active = false;
      return;
    }

    const int CARD_W = 340, CARD_H = 72, CARD_Y = 64;
    const int CARD_X = CX - CARD_W / 2;
    drawTranslucentRoundRect(CARD_X, CARD_Y, CARD_W, CARD_H, 18, 0x0000, 225);
    uint16_t borderCol = (alongPoi.status == 1) ? 0xFDE0 : 0x528A;
    gfx->drawRoundRect(CARD_X, CARD_Y, CARD_W, CARD_H, 18, borderCol);

    if (alongPoi.status == 1) {
      // 搜到加油站：左侧加油机符号
      aaDisc(CARD_X + 28, CARD_Y + 28, 12.0f, 0xFDE0);
      gfx->drawFastVLine(CARD_X + 28, CARD_Y + 22, 12, C_BLACK);
      gfx->drawFastHLine(CARD_X + 24, CARD_Y + 22, 8, C_BLACK);
      gfx->drawFastHLine(CARD_X + 24, CARD_Y + 34, 8, C_BLACK);

      // 名称 (限制宽度避免越界)
      char nameBuf[32];
      strncpy(nameBuf, alongPoi.name[0] ? alongPoi.name : "顺路加油站", sizeof(nameBuf) - 1);
      nameBuf[sizeof(nameBuf) - 1] = '\0';
      drawTextUTF8(CARD_X + 48, CARD_Y + 28, nameBuf, C_WHITE);

      // 顺路信息：偏离米数与沿途剩余距离
      char subBuf[48];
      if (alongPoi.distM >= 1000) {
        snprintf(subBuf, sizeof(subBuf), "偏离+%dm  距%d.%dkm", alongPoi.detourM, alongPoi.distM / 1000, (alongPoi.distM % 1000) / 100);
      } else {
        snprintf(subBuf, sizeof(subBuf), "偏离+%dm  距%dm", alongPoi.detourM, alongPoi.distM);
      }
      drawTextUTF8(CARD_X + 48, CARD_Y + 54, subBuf, 0x34EA);

      // 右侧操作提示胶囊：[加途经点]
      const int BTN_W = 76, BTN_H = 34, BTN_X = CARD_X + CARD_W - BTN_W - 12, BTN_Y = CARD_Y + (CARD_H - BTN_H) / 2;
      drawTranslucentRoundRect(BTN_X, BTN_Y, BTN_W, BTN_H, 10, 0x07E0, 180);
      gfx->drawRoundRect(BTN_X, BTN_Y, BTN_W, BTN_H, 10, C_WHITE);
      if (isChineseLang) {
        drawTextUTF8(BTN_X + 8, BTN_Y + 23, "加途径点", C_WHITE);
      } else {
        drawAaString(BTN_X + BTN_W / 2, BTN_Y + BTN_H / 2, "+VIA", 1.4f, 0.9f, 9.0f, C_WHITE, true);
      }

      // 剩余倒计时进度环
      float remainRatio = (float)(alongPoi.expireAt - t) / 7500.0f;
      remainRatio = constrain(remainRatio, 0.0f, 1.0f);
      drawTranslucentCircle(CARD_X + CARD_W - 14, CARD_Y + 14, 5, 0x0000, 160);
      aaDisc(CARD_X + CARD_W - 14, CARD_Y + 14, 3.5f * remainRatio, 0xFDE0);
    } else {
      // 未找到顺路加油站提示
      if (isChineseLang) {
        drawTextUTF8Center(CX, CARD_Y + 32, "沿途500m内暂无加油站", C_WHITE);
        drawTextUTF8Center(CX, CARD_Y + 54, "建议保持当前路线行驶", 0x8410);
      } else {
        drawAaString(CX, CARD_Y + CARD_H / 2, "NO GAS ON ROUTE", 1.5f, 0.9f, 10.0f, C_WHITE, true);
      }
    }
  }
}

// 触摸点击沿途卡片判定
inline bool alongPoiTap(int x, int y) {
  if (!alongPoi.active || millis() > alongPoi.expireAt) return false;
  const int CARD_W = 340, CARD_H = 72, CARD_Y = 64;
  const int CARD_X = CX - CARD_W / 2;
  if (x >= CARD_X && x <= CARD_X + CARD_W && y >= CARD_Y && y <= CARD_Y + CARD_H) {
    if (alongPoi.status == 1) {
      if (txChar) {
        txChar->setValue((uint8_t *)"T,QC,1", 6);
        txChar->notify();
      }
      CompanionMode::startVibration(140, 50);
    }
    alongPoi.active = false;
    navDirty = true;
    leanDirty = true;
    return true;
  }
  return false;
}

// 主动发起沿途加油站搜索
inline void triggerAlongGasSearch() {
  alongSearchRequested = true;
  alongSearching = true;
  alongSearchUntil = millis() + 6500;
  alongPoi.active = false;
  if (txChar) {
    txChar->setValue((uint8_t *)"T,Q,GAS", 7);
    txChar->notify();
  }
  CompanionMode::startVibration(120, 45);
  navDirty = true;
  leanDirty = true;
}
