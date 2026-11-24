extern int coverSlideDx;
extern int coverFlySpeed;
#pragma once
#include <Arduino.h>
#include "TouchDrvCSTXXX.hpp"
#include "BoardStopWatchInit.h"
#include "AlongBanner.h"

// ================= v5.1 来电显示 =================
// 手机来电时整屏显示：上面 “INCOMING CALL”，中间绿色电话图标带往外扩散的波纹，下面是名字（手机渲染好的
// 文字图片，图片栏 2）和号码；接听或挂断后回到原来的画面（地图 / 大箭头 / 首页）
bool callOn = false;
String callNum = "";
unsigned long callAt = 0, callSeenAt = 0;

// v5.2 触屏接电话：按住任意位置 HOLD_MS 毫秒 → 发 "T,A" 给手机（v6.9 800 → 200）
#define HOLD_MS 200
TouchDrvCST816 touch;   // StopWatch：CST820B（chipId 0xB7）属于 CST816 族；微雪 1.75 的 CST9217 才用 CST92xx
bool touchOK = false;
unsigned long touchHoldAt = 0, touchLastSeen = 0;
float touchHold = 0;                 // 0~1 按住的进度
bool callAnswerSent = false;
unsigned long callAnswerAt = 0;
String pendingProf = "";             // v5.8 分步用时
// pendingEvt moved to GlobalConfig.h

// v5.3 触摸芯片一有动静就拉低中断脚；平时只看这个标记，不用一直读 I2C
volatile bool touchIrq = false;
void IRAM_ATTR onTouchIrq() { touchIrq = true; }

void touchInit() {
  StopWatchBoard::tpReset();      // StopWatch：触摸复位在 M5IOE1 上，按官方时序手动来一次
  touch.setPins(TP_RESET, TP_INT);
  touchOK = touch.begin(Wire, 0x15, IIC_SDA, IIC_SCL);   // StopWatch CST820B 地址 0x15（微雪 CST9217 是 0x5A）
  if (!touchOK) {
    // M5IOE1 初始化会把总线提到 400kHz，CST820 保守按 100kHz 跑——降速重试一次（换料批次时留个后手）
    Wire.setClock(100000);
    delay(50);
    StopWatchBoard::tpReset();
    touchOK = touch.begin(Wire, 0x15, IIC_SDA, IIC_SCL);
  }
  Serial.println(touchOK ? "touch ok" : "touch not found");
  // 采用纯定时轮询 CST820，完全不依赖外部 GPIO 硬件中断
  // 彻底避免断开电脑独立供电时 TP_INT 引脚悬空/杂波引发中断雪崩或 FreeRTOS ipc1 栈溢出崩溃
}

// 执行上一曲 (在任意界面通用)
static void triggerKeyPrevTrack() {
  if (hidMusicMode) {
    if (!hidMediaReady() && !appleMedia.controlReady(0)) {
      musicAck = 7; musicAckUntil = millis() + 2500;
    } else {
      appleMedia.control(0);
    }
  } else {
    pendingEvt = "T,M,P,0,0,0";
    musicAck = 1; musicAckUntil = millis() + 2500;
  }
  if (page == 2) {
    coverFlySpeed = +32;
  }
  CompanionMode::startVibration(120, 45);
  musicDirty = true;
}

// 执行下一曲 (在任意界面通用)
static void triggerKeyNextTrack() {
  if (hidMusicMode) {
    if (!hidMediaReady() && !appleMedia.controlReady(2)) {
      musicAck = 7; musicAckUntil = millis() + 2500;
    } else {
      appleMedia.control(2);
    }
  } else {
    pendingEvt = "T,M,N,0,0,2";
    musicAck = 1; musicAckUntil = millis() + 2500;
  }
  if (page == 2) {
    coverFlySpeed = -32;
  }
  CompanionMode::startVibration(120, 45);
  musicDirty = true;
}

// StopWatch 实体按键交互：
//   面对屏幕看：左侧 KEY A (GPIO 2 / G2)，右侧 KEY B (GPIO 1 / G1)
//   - 长按 KEY A (>= 500ms)：任意界面上一首 (带震动反馈)
//   - 长按 KEY B (>= 500ms)：任意界面下一首 (带震动反馈)
//   - 同时按住 AB 键：切换倾角/G值表 <-> 主界面 (伴侣模式下长按800ms退回主页)
//   - 单按 KEY A / B (松手确认，防长按冲突)：
//       - 伴侣模式：浏览上一个 / 下一个表情
//       - 主页：A 切音乐页(左滑入)，B 切天气页(右滑入)
//       - 其它页：退回主页
void buttonTick() {
  static bool lastA = true, lastB = true;
  static unsigned long aDownAt = 0, bDownAt = 0;
  static bool aHoldFired = false, bHoldFired = false;
  static bool comboActive = false;
  static unsigned long lastActionAt = 0;
  static unsigned long abBothDownAt = 0;
  static bool abHoldTriggered = false;

  bool a = digitalRead(BTN_A);
  bool b = digitalRead(BTN_B);
  unsigned long now = millis();

  // 1. 到达目的地结算界面：按任意实体键立即退出结算返回主页
  if (fxOn) {
    if ((!a && lastA) || (!b && lastB)) {
      // 目的地结算前 600ms 防按键残留动作误触
      if (!fxWp && (now - fxAt < 600)) {
        // 忽略
      } else {
        stopArriveFx();
        CompanionMode::startVibration(140, 50);
        lastA = a; lastB = b;
        return;
      }
    }
    lastA = a; lastB = b;
    return;
  }

  // 2. 双键同时按下检测（优先级最高）
  if (!a && !b) {
    // 抑制单键短按与单键长按切歌
    aHoldFired = true;
    bHoldFired = true;
    aDownAt = 0;
    bDownAt = 0;

    // 伴侣模式专用：双键长按 800ms 返回主页
    if (page == 4) {
      if (abBothDownAt == 0) abBothDownAt = now;
      if (!abHoldTriggered && now - abBothDownAt >= 800) {
        abHoldTriggered = true;
        CompanionMode::startVibration(180, 70);
        showPageAnimated(0, 0, 1);
      }
      lastA = a; lastB = b;
      return;
    }

    // 非伴侣模式：双键切换倾角/G值表 <-> 主页
    if (!comboActive && (now - lastActionAt > 350)) {
      comboActive = true;
      lastActionAt = now;
      if (!sliding && !callOn && !pairOn && !fxOn) {
        if (page != 3) {
          showPageAnimated(3, 0, -1);
        } else {
          showPageAnimated(0, 0, 1);
        }
      }
    }
    lastA = a; lastB = b;
    return;
  } else {
    abBothDownAt = 0;
    if (a && b) abHoldTriggered = false;
  }

  // 双键均释放时解除双键锁
  if (a && b) {
    comboActive = false;
  }

  // 若双键尚未完全释放，避免残留单键动作
  if (comboActive) {
    lastA = a; lastB = b;
    return;
  }

  // 3. KEY A 状态机 (长按 500ms 上一首，短按松开换页)
  if (!a) {
    if (lastA) {
      // 刚按下
      aDownAt = now;
      aHoldFired = false;
    } else if (aDownAt > 0 && !aHoldFired && (now - aDownAt >= 500)) {
      // 持续按住满 500ms：触发上一首！
      aHoldFired = true;
      triggerKeyPrevTrack();
    }
  } else if (!lastA) {
    // 刚松开
    if (!aHoldFired && aDownAt > 0 && (now - aDownAt < 500) && (now - lastActionAt > 200)) {
      // 短按松手确认：执行原本单按 A 动作
      lastActionAt = now;
      if (page == 4) {
        CompanionMode::noteInteraction();
        CompanionMode::avatar.previous(now);
        CompanionMode::startVibration(125, 35);
      } else if (!sliding && !callOn && !pairOn && !fxOn) {
        if (page == 0) {
          showPageAnimated(2, 1, 0); // 主页 -> 音乐页 (从左滑入)
        } else if (page == 3) {
          showPageAnimated(0, 0, 1); // 倾角页 -> 主页
        } else {
          showPageAnimated(0, page == 2 ? -1 : 1, 0); // 音乐/天气页 -> 主页
        }
      }
    }
    aDownAt = 0;
    aHoldFired = false;
  }

  // 4. KEY B 状态机 (长按 500ms 下一首，短按松开换页)
  if (!b) {
    if (lastB) {
      // 刚按下
      bDownAt = now;
      bHoldFired = false;
    } else if (bDownAt > 0 && !bHoldFired && (now - bDownAt >= 500)) {
      // 持续按住满 500ms：触发下一首！
      bHoldFired = true;
      triggerKeyNextTrack();
    }
  } else if (!lastB) {
    // 刚松开
    if (!bHoldFired && bDownAt > 0 && (now - bDownAt < 500) && (now - lastActionAt > 200)) {
      // 短按松手确认：执行原本单按 B 动作
      lastActionAt = now;
      if (page == 4) {
        CompanionMode::noteInteraction();
        CompanionMode::avatar.next(now);
        CompanionMode::startVibration(125, 35);
      } else if (!sliding && !callOn && !pairOn && !fxOn) {
        if (page == 0) {
          showPageAnimated(1, -1, 0); // 主页 -> 天气页 (从右滑入)
        } else if (page == 3) {
          showPageAnimated(0, 0, 1);  // 倾角页 -> 主页
        } else {
          showPageAnimated(0, page == 2 ? -1 : 1, 0);  // 音乐/天气页 -> 主页
        }
      }
    }
    bDownAt = 0;
    bHoldFired = false;
  }

  lastA = a;
  lastB = b;
}

bool touchDown = false;
int16_t touchX0 = 0, touchY0 = 0, touchX1 = 0, touchY1 = 0;
void dragMove(int dx, int dy, unsigned long now);   // v5.5 跟手换页
int musicButtonAt(int x, int y);
void musicTap(int x, int y);
void dragEnd();
void dragAbort();

// 读触摸：按下 → 报坐标；来电时按住够久 → 接听；拖动 → 跟手换页（v5.5）
// 手指离开超过 0.15 秒才算松开（防止读数偶尔丢一下）
void touchPoll() {
  static unsigned long lastPoll = 0;
  static bool calibTriggered = false;
  static bool destDragging = false;
  static int destDragStartY = 0;
  static int destDragStartOff = 0;
  static bool cruiseExitTriggered = false;
  unsigned long now = millis();
  if (!touchOK) return;
  // 纯 12ms 定时轮询：无论是否连接电脑、无论中断脚电平如何，均稳定高效响应
  if (!touchDown && !callOn && (now - lastPoll < 12)) return;
  lastPoll = now;
  if (callAnswerSent && now - callAnswerAt > 4000) callAnswerSent = false;   // 4 秒还在响：允许再按一次
  int16_t x[5] = {0}, y[5] = {0};
  uint8_t n = touch.getPoint(x, y, 1);
  if (page == 4) {
    if (n > 0) {
      touchDown = true;
      touchLastSeen = now;
      float tdx = x[0] - CX, tdy = y[0] - CX;
      touchVectorToCanvas(tdx, tdy);
      CompanionMode::handleTouch(true, (int16_t)lroundf(CX + tdx), (int16_t)lroundf(CX + tdy));
    } else {
      touchDown = false;
      CompanionMode::handleTouch(false, 0, 0);
    }
    return;
  }
  if (n > 0) {
    if (!touchDown) {
      touchDown = true;
      touchHoldAt = now;
      touchX0 = touchX1 = x[0]; touchY0 = touchY1 = y[0];
      pendingEvt = "T,P," + String(x[0]) + "," + String(y[0]);
      float tdx0 = x[0] - CX, tdy0 = y[0] - CX;
      touchVectorToCanvas(tdx0, tdy0);
      int cx0 = (int)lroundf(CX + tdx0);
      int cy0 = (int)lroundf(CX + tdy0);
      // 导航地图页面：右侧边缘滑动缩放检测 (对标音乐页面的左侧音量轨操作)
      if (page == 0 && (inNav || cruiseOn || mapModeActive()) && !callOn && !pairOn && !fxOn) {
        float distSq = (float)(cx0 - CX) * (cx0 - CX) + (float)(cy0 - CX) * (cy0 - CX);
        // 右侧边缘扇区：X 贴近右侧 (cx0 >= CX + 85)，Y 在上下 120 像素以内，半径在弧边带 (140~245)
        if (cx0 >= (CX + 85) && abs(cy0 - CX) <= 120 && distSq >= 140.0f * 140.0f && distSq <= 245.0f * 245.0f) {
          mapZoomActive = true;
          touchEat = true;
          mapZoomStartX = cx0;
          mapZoomStartY = cy0;
          mapZoomStartScale = userMapScale;
          mapZoomHintUntil = now + 2500;
          mapZoomBtnUntil = now + 10000;
          dragAbort(); // 拦截页面拖拽换页
          CompanionMode::startVibration(50, 20);
        }
      }
      if (page == 2 && !callOn && !pairOn) {
        // 1. 正左方 9 点钟贴边 VOLUME 与音量轨区域 (仅在控制面板模式下有效)
        float distSq = (float)(cx0 - CX) * (cx0 - CX) + (float)(cy0 - CX) * (cy0 - CX);
        if (!musicLyricsView && cx0 <= 120 && abs(cy0 - CX) <= 90 && distSq >= 150.0f * 150.0f) {
          volSliding = true;
          volActiveUntil = now + 2000;
          touchEat = true;
          float a = atan2f((float)(cx0 - CX), (float)(CX - cy0));
          if (a < 0) a += 2.0f * (float)M_PI;
          float p = a / (2.0f * (float)M_PI);
          int newVol = constrain(lroundf((p - 0.68f) / (0.82f - 0.68f) * 100.0f), 0, 100);
          if (newVol != musicVolume) {
            musicVolume = newVol;
            musicDirty = true;
          }
        }
        // 2. 专辑封面区域 (仅在控制面板模式下有效，放大至 206x206, X: 120~346, Y: 42~260)
        else if (!musicLyricsView && cx0 >= 120 && cx0 <= 346 && cy0 >= 42 && cy0 <= 260) {
          coverTouchActive = true;
          touchEat = true; // 拦截整页切换滑动
        }
        // 4. 歌词模式下或封面之外其它区域：不设 touchEat，抬手轻触触发 musicTap 切换模式，滑动允许 dragMove 拖拽换页！
      }
      if (arrivedBannerOn && !callOn && !pkCountdownActive) {
        int touchLimitY = LCD_HEIGHT - ((LCD_WIDTH > 300) ? 168 : 105);
        if (LCD_WIDTH > 300 && cx0 >= CX - 118 && cx0 <= CX + 118 && cy0 >= 372 && cy0 <= 430) {
          // 阶段二：点击 ARRIVED 横幅内的「记住车位 · 寻车信标」按钮 -> 通知手机记车位并进入 5 秒息屏倒计时
          arrivedBannerOn = false;
          resetNavState();
          progReset();
          pkBeaconTriggerFromArrived();
          touchEat = true;
        } else if (cy0 >= touchLimitY) {
          arrivedBannerOn = false;
          resetNavState();
          progReset();
          CompanionMode::startVibration(140, 50);
          if (txChar) {
            txChar->setValue((uint8_t *)"T,EXIT_NAV", 10);
            txChar->notify();
          }
          touchEat = true;
        }
      }
      if (pkCountdownActive) {
        // 5 秒信标倒计时界面：点击底部「取消」按钮区域中止倒计时
        if (cx0 >= CX - 95 && cx0 <= CX + 95 && cy0 >= 350 && cy0 <= 425) {
          pkBeaconCancelCountdown();
        }
        touchEat = true;
      } else if (fxOn && !callOn) {
        // 目的地结算前 600ms 防到达瞬间触屏误触
        if (!fxWp && (now - fxAt < 600)) {
          touchEat = true;
        } else if (!fxWp && cx0 >= CX - 122 && cx0 <= CX + 122 && cy0 >= 344 && cy0 <= 406) {
          // 阶段二：点击 Arrived 底部「记住车位 · 寻车信标」按钮 -> 通知手机记车位并启动 5s 息屏倒计时
          pkBeaconTriggerFromArrived();
          touchEat = true;
        } else {
          stopArriveFx();
          CompanionMode::startVibration(140, 50); // 触屏轻触退出也给予震动反馈
          touchEat = true;
        }
      }          // v7.1 点一下结束到达特效
    }
    touchX1 = x[0]; touchY1 = y[0];
    touchLastSeen = now;

    if (page == 2 && volSliding) {
      float tdx = touchX1 - CX, tdy = touchY1 - CX;
      touchVectorToCanvas(tdx, tdy);
      int cx1 = (int)lroundf(CX + tdx);
      int cy1 = (int)lroundf(CX + tdy);
      float a = atan2f((float)(cx1 - CX), (float)(CX - cy1));
      if (a < 0) a += 2.0f * (float)M_PI;
      float p = a / (2.0f * (float)M_PI);
      int newVol = constrain(lroundf((p - 0.68f) / (0.82f - 0.68f) * 100.0f), 0, 100);
      if (abs(newVol - musicVolume) >= 2) {
        musicVolume = newVol;
        volActiveUntil = now + 2000;
        musicDirty = true;
      }
    }

    if (page == 2 && coverTouchActive && !volSliding) {
      float vx = touchX1 - touchX0, vy = touchY1 - touchY0;
      touchVectorToCanvas(vx, vy);
      int curDx = (int)lroundf(vx);
      int curDy = (int)lroundf(vy);
      if (abs(curDx) > 10 && abs(curDx) > abs(curDy)) {
        coverSlideDx = constrain(curDx, -180, 180);
        musicDirty = true;
      }
    }

    // 倾角仪表页面：长按屏幕任意位置 1.2 秒同时实现 G 值表与倾角仪复位及水平校准
    if (page == 3 && touchDown && !callOn && !pairOn) {
      if (abs(touchX1 - touchX0) < 35 && abs(touchY1 - touchY0) < 35) {
        if (now - touchHoldAt >= 1200 && !calibTriggered) {
          calibTriggered = true;
          LeanAngleMeter::resetStats();
          if (!LeanAngleMeter::calibrateZero()) {
            LeanAngleMeter::leanAngle = 0.0f;
            LeanAngleMeter::calibSuccessUntil = millis() + 2000;
          }
          CompanionMode::startVibration(90, 40);
          #ifdef M5PM1_ADDR
          M5PM1.setVibration(120);
          #endif
          touchEat = true;
          dragAbort();
          leanDirty = true;
        }
      }
    }

    // 收藏列表抽屉：上下拖动平滑滚动（仅在可滚动时接管拖动 + 16ms 节流 + 极速局部刷新）
    if (destSheetOpen && touchDown && !callOn && !pairOn) {
      int maxScr = destMaxScrollPx();
      if (maxScr > 0) {
        float vx = touchX1 - touchX0, vy = touchY1 - touchY0;
        touchVectorToCanvas(vx, vy);
        if (!destDragging && fabsf(vy) >= 8.0f) {
          destDragging = true;
          destDragStartY = (int)lroundf(vy);
          destDragStartOff = destScrollOffset;
        }
        if (destDragging) {
          static unsigned long lastDestDrawAt = 0;
          int dy = (int)lroundf(vy) - destDragStartY;
          int nextOff = constrain(destDragStartOff - dy, 0, maxScr);
          if (nextOff != destScrollOffset) {
            destScrollOffset = nextOff;
            if (now - lastDestDrawAt >= 16) {
              lastDestDrawAt = now;
              refreshDestSheetScroll();
            }
          }
          touchEat = true;
        }
      }
    }

    // 巡航模式：长按地图空白处 1.0 秒退出巡航模式（仅巡航态生效，避免与导航缩放冲突）
    if (page == 0 && inNav && cruiseOn && touchDown && !callOn && !pairOn && !fxOn &&
        !mapZoomActive && !destSheetOpen && !routePickerOpen && !arrivedBannerOn) {
      if (abs(touchX1 - touchX0) < 35 && abs(touchY1 - touchY0) < 35) {
        if (now - touchHoldAt >= 1000 && !cruiseExitTriggered) {
          cruiseExitTriggered = true;
          touchEat = true;
          dragAbort();
          CompanionMode::startVibration(140, 50);
          if (txChar && bleConnected) {
            txChar->setValue((uint8_t *)"T,EXIT_NAV", 10);
            txChar->notify();
          } else if (bleConnected) {
            pendingEvt = "T,EXIT_NAV";
          }
          progReset();
          resetNavState();
          drawWaiting(bleConnected ? WAIT_READY : WAIT_NOLINK);
        }
      }
    }

    if (page == 0 && mapZoomActive) {
      touchEat = true;
      float tdx1 = touchX1 - CX, tdy1 = touchY1 - CX;
      touchVectorToCanvas(tdx1, tdy1);
      int cy1 = (int)lroundf(CX + tdy1);
      float dy = (float)(cy1 - mapZoomStartY); // 画布方向：dy < 0 向上滑放大，dy > 0 向下滑缩小
      
      // 线性平滑手感：上下滑动行程对应缩放 0.20x ~ 1.00x
      float targetScale = constrain(mapZoomStartScale - dy * 0.0065f, 0.20f, 1.00f);
      if (fabsf(targetScale - userMapScale) >= 0.008f) {
        userMapScale = constrain(userMapScale + (targetScale - userMapScale) * 0.55f, 0.20f, 1.00f);
        userZoomOverridden = true; // 用户手动设置，当前速度区间临时生效
        mapZoomHintUntil = now + 2000;
        mapZoomBtnUntil = now + 10000;
        navDirty = true;
      }
    }

    if (callOn || pairOn || touchEat || fxOn || mapZoomActive || destSheetOpen || routePickerOpen) dragAbort();
    else {
      dragMove(touchX1 - touchX0, touchY1 - touchY0, now);
    }
  } else if (touchDown && now - touchLastSeen > 70) {
    touchDown = false;
    unsigned long pressDuration = now - touchHoldAt;
    touchHoldAt = 0;
    calibTriggered = false;
    if (destDragging) {
      destDragging = false;
      refreshDestSheetScroll();
    }
    cruiseExitTriggered = false;
    if (page == 0 && mapZoomActive) {
      mapZoomActive = false;
      touchEat = true;
      mapZoomSaveAt = now + 1500;
      mapZoomHintUntil = now + 2000;
      mapZoomBtnUntil = now + 10000;

      float tdx0 = touchX0 - CX, tdy0 = touchY0 - CX;
      touchVectorToCanvas(tdx0, tdy0);
      int cy0 = (int)lroundf(CX + tdy0);

      float tdx1 = touchX1 - CX, tdy1 = touchY1 - CX;
      touchVectorToCanvas(tdx1, tdy1);
      int cy1 = (int)lroundf(CX + tdy1);

      float dragDist = hypotf((float)(touchX1 - touchX0), (float)(touchY1 - touchY0));

      // 若只是轻触一下右侧区域（按压时间短且位移小：< 450ms, dist < 30px），支持轻点 + / - 单步步进调节！
      if (pressDuration < 450 && dragDist < 30.0f) {
        if (cy0 < CX) {
          // 点在加号附近 (上半区)：放大 0.1x
          userMapScale = min(1.00f, userMapScale + 0.10f);
        } else {
          // 点在减号附近 (下半区)：缩小 0.1x
          userMapScale = max(0.20f, userMapScale - 0.10f);
        }
        userZoomOverridden = true; // 用户手动设置，当前速度区间临时生效
      }
      CompanionMode::startVibration(70, 25);
      if (bleConnected) pendingEvt = "T,ZOOM," + String((int)lroundf(userMapScale * 100.0f));
      navDirty = true;
    }
    if (page == 2 && volSliding) {
      volSliding = false;
      volActiveUntil = now + 1500;
      touchEat = true;
      pendingEvt = "T,M,V," + String(musicVolume);
      musicDirty = true;
    }
    if (page == 2 && coverTouchActive) {
      coverTouchActive = false;
      touchEat = true;
      float vx = touchX1 - touchX0, vy = touchY1 - touchY0;
      touchVectorToCanvas(vx, vy);
      int dx = (int)lroundf(vx);
      int dy = (int)lroundf(vy);
      // 1. 明确的左右横向手势切歌 (位移 >= 45 像素，且水平明显大于垂直)
      if (abs(dx) >= 45 && abs(dx) >= (int)(abs(dy) * 1.2f)) {
        coverPendingSingleTap = false;
        lastCoverTapAt = 0;
        if (dx < 0) {
          triggerKeyNextTrack(); // 向左滑 -> 下一曲 (发指令+启动向左飞出动画)
        } else {
          triggerKeyPrevTrack(); // 向右滑 -> 上一曲 (发指令+启动向右飞出动画)
        }
      }
      // 2. 原地轻触判定 (短按点击或位移极小，绝对优先作为点击播放/暂停，绝不误切歌)
      else if ((pressDuration >= 20 && pressDuration < 450) || (abs(dx) <= 35 && abs(dy) <= 35)) {
        coverSlideDx = 0;
        coverFlySpeed = 0;
        float tdx = touchX0 - CX, tdy = touchY0 - CX;
        touchVectorToCanvas(tdx, tdy);
        int cx0 = (int)lroundf(CX + tdx);
        int cy0 = (int)lroundf(CX + tdy);

        int ldx = cx0 - 306, ldy = cy0 - 224;

        if (ldx * ldx + ldy * ldy <= 44 * 44) {
          // 点击封面右下角：切换 Like / Dislike (增大热区半径至 44px，触控灵敏随手可点)
          coverPendingSingleTap = false;
          lastCoverTapAt = 0;
          musicLiked = !musicLiked;
          pendingEvt = String("T,M,L,") + (musicLiked ? "1" : "0");
          musicAck = 1; musicAckUntil = now + 2500;
          CompanionMode::startVibration(120, 40);
          musicDirty = true;
        } else {
          // 封面中心主体：双击 vs 单击判定播放/暂停
          unsigned long dt = (lastCoverTapAt > 0) ? (now - lastCoverTapAt) : 99999UL;
          if (dt < 140) {
            // 物理抬手抖动或电容屏脱离瞬态毛刺 (< 140ms)：直接过滤丢弃！维持原单击定时器正常判定
          } else if (dt <= 360) {
            // 真正的人类双击 (两次敲击间隔在 140ms ~ 360ms 之间)：触发 Like / Dislike
            coverPendingSingleTap = false;
            lastCoverTapAt = 0;
            musicLiked = !musicLiked;
            pendingEvt = String("T,M,L,") + (musicLiked ? "1" : "0");
            musicAck = 1; musicAckUntil = now + 2500;
            CompanionMode::startVibration(150, 45);
            musicDirty = true;
          } else {
            // 第一次轻触：开启延迟确认，等待是否会有第二次敲击
            coverPendingSingleTap = true;
            lastCoverTapAt = now;
          }
        }
      }
      // 3. 拖拽了但不够切歌阈值：弹性回弹原位
      else {
        if (coverSlideDx != 0) {
          coverFlySpeed = (coverSlideDx > 0) ? -20 : +20;
          musicDirty = true;
        }
      }
    }
    if (callOn || pairOn || touchEat || fxOn) dragAbort();
    else {
      // 统一将硬件物理触摸点按当前屏幕旋转映射至画布坐标系，彻底杜绝旋转后的触控错位！
      float tdx = touchX0 - CX, tdy = touchY0 - CX;
      touchVectorToCanvas(tdx, tdy);
      int canvasX = (int)lroundf(CX + tdx);
      int canvasY = (int)lroundf(CX + tdy);

      // 1. 最高优先级：圆屏独立选路浮层 / 收藏与沿途搜抽屉 / 沿途搜确认卡片
      if (routePickerOpen && routePickerTap(canvasX, canvasY)) { dragAbort(); touchEat = false; return; }
      if (destSheetOpen && destSheetTap(canvasX, canvasY)) { dragAbort(); touchEat = false; return; }
      if (alongPoiTap(canvasX, canvasY)) { dragAbort(); touchEat = false; return; }

      // 2. 导航中或巡航模式轻触空白处：9点方向浮现主题收藏按钮；点击展开收藏与沿途查找抽屉
      if (page == 0 && inNav && abs(touchX1 - touchX0) < 36 && abs(touchY1 - touchY0) < 36) {
        if (now < navStarUntil) {
          int sdx = canvasX - 52;
          int sdy = canvasY - CX;
          if (sdx * sdx + sdy * sdy <= 44 * 44) {
            CompanionMode::startVibration(55, 22);
            destScrollOffset = 0;
            destSheetOpen = true;
            navStarUntil = 0;
            if (bleConnected) pendingEvt = "T,DEST_Q";
            navDirty = true;
            dragAbort();
            touchEat = false;
            return;
          }
        }
        // 轻触空白处：唤起/延长底部五角星按钮显示 5.5 秒
        navStarUntil = now + 5500;
        navDirty = true;
        CompanionMode::startVibration(35, 14);
        dragAbort();
        touchEat = false;
        return;
      }

      if (page == 2 && abs(touchX1 - touchX0) < 36 && abs(touchY1 - touchY0) < 36) { musicTap(canvasX, canvasY); CompanionMode::startVibration(65, 22); }
      if (page == 0 && !inNav && abs(touchX1 - touchX0) < 36 && abs(touchY1 - touchY0) < 36) { waitingScreenTap(canvasX, canvasY); CompanionMode::startVibration(65, 22); }
      if (page == 1 && abs(touchX1 - touchX0) < 36 && abs(touchY1 - touchY0) < 36) { settingsPageTap(canvasX, canvasY); CompanionMode::startVibration(65, 22); }
      dragEnd();
    }
    touchEat = false;
    if (musicPressed >= 0) { musicPressed = -1; musicDirty = true; }
  }
  // 来电：按住不动才算（滑动不算）
  bool still = abs(touchX1 - touchX0) < 60 && abs(touchY1 - touchY0) < 60;
  touchHold = (callOn && touchDown && still) ? min(1.0f, (now - touchHoldAt) / (float)HOLD_MS) : 0;
  if (touchHold >= 1 && !callAnswerSent) {
    callAnswerSent = true;
    callAnswerAt = now;
    touchHold = 0;
    pendingEvt = "T,A";
  }
}

// 电话听筒：一段拱形的圆点串当手柄，两头各一个大点当听筒和话筒，整体斜过来（常见的电话图标）
