#pragma once
#include <Arduino.h>

// v2.2 每 2 秒统计一次帧率，发给手机显示
void updateStats() {
  unsigned long now = millis();
  if (now - statAt < 2000) return;
  float secs = (now - statAt) / 1000.0f;
  if (statAt != 0 && mapFrames > 0) {
    uint32_t fc = flushCnt, fm = flushMsAcc;
    pendingStat = "S," + String(mapFrames / secs, 1) + "," + String(mapDrawMsAcc / mapFrames) + "," +
                  String(fc ? fm / fc : 0) + "," + String(qspiMHz) + "," +
                  String(mapMsAcc / mapFrames) + "," + String(ovlMsAcc / mapFrames) + "," + String(fastDev ? 1 : 0) + "," +
                  String(rasMsAcc / mapFrames) + "," + String(helperTask ? 2 : 1);   // v5.7 底图用时、几个核心在画
    String q = "Q";                                    // v5.8 分步用时（0.1 毫秒/帧）
    for (int i = 0; i < PROF_N; i++) q += "," + String(profAcc[i] / 100 / mapFrames);
    pendingProf = q;
#if PERF_LOG
    if (Serial) {
      const uint32_t n = mapFrames, pc = perf.pCnt, rc = perf.rasCalls;
      Serial.printf("[PERF] fps=%.1f dt(avg/max)=%u/%ums >50=%u >100=%u | draw=%ums map=%ums ras=%ums ovl=%ums | "
                    "core(main/wait/help)=%u/%u/%ums | flush=%ums fwait=%ums qspi=%u fast=%d\n",
                    n / secs, perf.dtSumMs / (n > 1 ? n - 1 : 1), perf.dtMaxMs, perf.over50, perf.over100,
                    mapDrawMsAcc / n, mapMsAcc / n, rasMsAcc / n, ovlMsAcc / n,
                    rc ? perf.rasMainUs / rc / 1000 : 0, rc ? perf.rasWaitUs / rc / 1000 : 0, rc ? perf.rasHelpUs / rc / 1000 : 0,
                    fc ? fm / fc : 0, (uint32_t)(flushWaitUsAcc / n / 1000), (unsigned)qspiMHz, fastDev ? 1 : 0);
      Serial.printf("[PERF] pose #p=%u gap(avg/max)=%u/%ums spd=%.0fkm/h | hdg err.max=%.1f vel.max=%.0f step.max=%.1f clamp12=%u rot.frames=%u rate.max=%.1f | "
                    "scale=%.2f raster=%d vice=%d back=%u style=%d\n",
                    pc, pc ? perf.pGapSumMs / pc : 0, perf.pGapMaxMs, posKmh,
                    perf.hdgErrMax, perf.hdgVelMax, perf.hdgStepMax, perf.stepClamp, perf.rotFrames, perf.hdgRateMax,
                    viewScale, rasterValid ? 1 : 0, rasterViceValid ? 1 : 0, perf.backFrames, styleIdx());
    }
#endif
  }
  for (int i = 0; i < PROF_N; i++) profAcc[i] = 0;
  perf = PerfStats(); flushWaitUsAcc = 0;
  statAt = now; mapFrames = 0; mapDrawMsAcc = 0; mapMsAcc = 0; ovlMsAcc = 0; flushCnt = 0; flushMsAcc = 0; rasMsAcc = 0;
}

void updateRingTarget() {
  bool hasRing = (navDir != "S") && navDist <= STAGE_FAR_M;
  float target = hasRing ? (float)(STAGE_FAR_M - navDist) / STAGE_FAR_M : 0;
  const uint32_t now = millis();
  turnRingMotion.update(ringShown, target, navDir != ringDir || !ringActive || !hasRing, now);
  ringShown = turnRingMotion.value(now);
  ringTarget = target;
  ringActive = hasRing;
  ringDir = navDir;
}

void animateRing() {
  if (!inNav || !ringActive || staleShown || rerouteAt) return;
  const uint32_t now = millis();
  if (now - ringAnimationDrawAt < 33) return;
  ringAnimationDrawAt = now;
  const float next = turnRingMotion.value(now);
  if (next == ringShown) return;
  ringShown = next;
  navDirty = true;
}

// 右侧竖排路口红绿灯绘制（适配 466x466 AMOLED 圆屏边缘）
void drawTrafficLightLab(uint32_t now) {
  const bool preview = trafficLightPreviewUntil && (int32_t)(trafficLightPreviewUntil - now) > 0;
  const bool pending = !preview && trafficLightFrame.valid(now) && !trafficLightLab.valid(now);
  if (!preview && !pending && !trafficLightLab.valid(now)) return;
  const bool other = !preview && trafficLightLab.phases[3].valid(now);
  const int rows = other ? 4 : 3, x = CX + 88, y = 140, w = 92, h = 38, gap = 3;
  static int previousRows = 0;
  if (previousRows > rows) navDirty = true;
  previousRows = rows;
  gfx->fillRoundRect(x - 2, y - 13, w + 4, rows * (h + gap) + 11, 8, 0x1082);
  printCentered(preview ? String("PREVIEW") : String("LIGHTS"), x + w / 2, y - 10, 1, C_WHITE);
  uint32_t signature = preview ? 0xFFFF : 0;
  for (int i = 0; i < rows; ++i) {
    const TrafficLightPhase &p = trafficLightLab.phases[i];
    const bool present = !preview && p.valid(now);
    const bool known = present && (p.status == 1 || p.status == 4 || p.status == -1);
    const bool rightMissing = !preview && !pending && i == 2 && !known;
    const uint16_t color = !known ? C_GREY : p.status == 1 ? C_RED : p.status == 4 ? C_GREEN : 0xFFE0;
    const int ry = y + i * (h + gap);
    gfx->fillRoundRect(x, ry, w, h, 6, 0x1082);
    gfx->drawRoundRect(x, ry, w, h, 6, color);
    if (i == 1) {
      gfx->fillRect(x + 12, ry + 15, 4, 12, color);
      gfx->fillTriangle(x + 14, ry + 6, x + 6, ry + 18, x + 22, ry + 18, color);
    } else if (i == 0) {
      gfx->fillRect(x + 18, ry + 14, 4, 13, color); gfx->fillRect(x + 9, ry + 14, 13, 4, color);
      gfx->fillTriangle(x + 3, ry + 16, x + 12, ry + 8, x + 12, ry + 24, color);
    } else if (i == 2) {
      gfx->fillRect(x + 8, ry + 14, 4, 13, color); gfx->fillRect(x + 8, ry + 14, 13, 4, color);
      gfx->fillTriangle(x + 26, ry + 16, x + 17, ry + 8, x + 17, ry + 24, color);
    } else gfx->fillCircle(x + 14, ry + 16, 5, color);
    unsigned seconds = present ? p.seconds(now) : 0;
    if (rightMissing) gfx->fillCircle(x + 60, ry + 14, 9, C_GREY);
    else printCentered(known && seconds ? String(seconds) : String("--"), x + 60, ry + 3, 3, color);
    const char *label = i == 0 ? "LEFT" : i == 1 ? "STRAIGHT" : i == 2 ? "RIGHT" : "OTHER";
    printCentered(String(label), x + 60, ry + 27, 1, C_WHITE);
    if (present && p.wait > 0) printCentered(String("W") + p.wait, x + 14, ry + 28, 1, C_GREY);
    signature = signature * 31 + (present ? ((p.status + 2) * 1000 + seconds) : 0);
  }
  static uint32_t previous = 0;
  if (signature != previous) {
    Serial.printf("TL JUNCTION DRAW layout=right-stack preview=%d count=%u L=%u S=%u R=%u inNav=%d map=%d\n",
      preview, trafficLightLab.count(now),
      trafficLightLab.phases[0].seconds(now), trafficLightLab.phases[1].seconds(now), trafficLightLab.phases[2].seconds(now),
      inNav, mapModeActive());
    previous = signature;
  }
  needFlush = true;
}

// 按当前全部状态画一整帧导航画面
void drawNavFrame() {
  onWaitScreen = false;                                 // v3.6 离开首页，猫猫不再动
  if (mapModeActive()) { drawMapFrame(); return; }      // v2.0 小地图
  viewInit = false;
  if (cruiseOn) {                                       // v4.8 巡航但还没有地图：只显示车速和测速
    gfx->fillScreen(C_BLACK);
    bool over = hasCam && camLimit > 0 && speedKmh > camLimit;
    uint32_t t = millis();
    if (over) drawEdgeAlertGlow(C_RED, (uint8_t)(85 + 95 * (0.5f + 0.5f * sinf(t * 0.012f))));
    else if (hasCam && camDist > 0 && camDist <= 300) drawEdgeAlertGlow(C_ORANGE, (uint8_t)(65 + 75 * (0.5f + 0.5f * sinf(t * 0.007f))));
    drawSpeedBlock(over);
    printCentered("CRUISE", CX, 300, 4, C_CYAN);
    drawAlongPoiBanner(t);
    drawNavStarButton(t);
    if (destSheetOpen) drawDestinationsSheet();
    if (routePickerOpen) drawRoutePickerSheet();
    needFlush = true;
    return;
  }
  gfx->fillScreen(C_BLACK);
  bool isS = (navDir == "S");
  bool isFar = !isS && navDist > STAGE_FAR_M;
  bool lanes = laneN > 0 && ringActive;                 // 跟随转弯倒计时开始显示
  bool over = hasCam && camLimit > 0 && speedKmh > camLimit;
  float laneK = lanes ? 0.85f : 1.0f;
  float iconCy = lanes ? ICON_CY_LANE : ICON_CY;

  // 边缘渐变闪烁（超速/违章拍照）+ 最外环绿色总行程进度环 + 内侧亮黄色(C_YELLOW)路口倒计时环（同时显示、不替换）
  uint32_t t = millis();
  if (over) drawEdgeAlertGlow(C_RED, (uint8_t)(85 + 95 * (0.5f + 0.5f * sinf(t * 0.012f))));
  else if (hasCam && camDist > 0 && camDist <= 300) drawEdgeAlertGlow(C_ORANGE, (uint8_t)(65 + 75 * (0.5f + 0.5f * sinf(t * 0.007f))));

  float tp = tripFrac();
  if (tp >= 0) drawTripRing(tp);
  if (ringActive) {
    drawRingTrack();
    ringShown = turnRingMotion.value(t);
    drawRingProgress(ringShown, 12, C_YELLOW);
  }

  drawSpeedBlock(over);

  // 中间图标
  // v1.8.2 图标可用区域：车速下方 ~ 车道（或距离数字）上方，各留 8 像素
  float iconTop = ICON_TOP, iconBot = (lanes ? LANES_Y : DIST_Y) - 8;
  if (isFar)     drawPreview(navDir, iconCy, 0.66f * laneK, iconTop, iconBot);
  else if (isS)  drawBigIcon("S", C_WHITE, iconCy, laneK, iconTop, iconBot);
  else           drawBigIcon(navDir, navDist <= STAGE_NEAR_M ? C_ORANGE : C_WHITE, iconCy, laneK, iconTop, iconBot);

  // 车道、距离、路名、底部信息
  if (lanes) drawLanes(LANES_Y);
  printCentered(distText(navDist), CX, lanes ? DIST_Y_LANE : DIST_Y, lanes ? 6 : 7, C_WHITE);
  drawTextImg(0, lanes ? ROAD_Y_LANE : ROAD_Y, 0xDD);
  drawTextImg(1, BOTTOM_Y, 0x99);                      // v1.7.3：常驻显示
  drawNavHints(128);                                    // v5.0
  drawTrafficLightLab(millis());                        // 红绿灯竖排卡片
  drawAlongPoiBanner(t);
  drawNavStarButton(t);
  if (destSheetOpen) drawDestinationsSheet();
  if (routePickerOpen) drawRoutePickerSheet();
  needFlush = true;
}

void resetNavState() {
  if (idlePowerOffNavigationActive(true, inNav, cruiseOn, navDir == "A"))
    idlePowerOffReset(&idlePowerOffTimer, millis());
  inNav = false;
  alongPoi.active = false;
  alongSearching = false;
  alongSearchRequested = false;
  tripOn = false; lightDist = -1;                       // v3.1
  mapCamN = 0;                                          // v3.5
  ghostN = 0;                                           // v4.4
  cruiseOn = false;                                     // v4.8
  trafN = 0; rerouteAt = 0; fasterAt = 0; gpsWeak = false;   // v5.0
  speedKmh = -1;
  hasCam = false;
  laneN = 0;
  ringActive = false; ringDir = "";
  trafficLightLab.clear(); trafficLightFrame.clear();
  for (int i = 0; i < 3; i++) { imgs[i].showW = 0; imgs[i].showH = 0; imgs[i].recvId = -1; }   // v5.3 天气栏 3、4 留着
}

int hexVal(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return 0xF;
}

