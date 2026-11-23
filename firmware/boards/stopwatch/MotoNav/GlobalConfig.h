#pragma once
#include <Arduino.h>
#include "AppleMediaClient.h"
#include "HidMediaKeys.h"
#include "MapThemes.h"
int themeIdx = 1;                                  // 默认罪恶都市
bool nightOn = false;                              // 夜间配色
bool curveOn = false;                              // 弯道急缓上色
int trailMode = 1;                                 // 走过的路线
bool isChineseLang = true;                            // 中英文界面自适应 (默认中文，与手机 App 联动)

// ===== 阶段一特性：实时海拔高度、累计爬升与路段坡度仪表 =====
int altitudeM = 0;
int slopePct = 0;
int climbAccumM = 0;
bool altValid = false;

// ===== 阶段一特性：一键沿途顺路搜 (加油站 / 维修点 / 服务区) =====
struct AlongPoiInfo {
  bool active = false;
  int status = 0;       // 1 = 找到, 0 = 未找到
  char name[64] = {0};  // 加油站名称
  int detourM = 0;      // 偏离米数
  int distM = 0;        // 沿途米数
  unsigned long expireAt = 0;
};
AlongPoiInfo alongPoi;
bool alongSearching = false;
bool alongSearchRequested = false;
unsigned long alongSearchUntil = 0;

// ===== 阶段一特性：沿途极端天气智能预警 =====
#define MAX_WEATHER_POINTS 8
struct RouteWeatherPoint {
  int32_t meterOnRoute = 0; // 路线桩号（米）
  uint8_t type = 0;         // 1=雨, 2=雷电, 3=大雾, 4=降雪, 5=大风
  bool valid = false;
};
RouteWeatherPoint routeWeatherPts[MAX_WEATHER_POINTS];
int routeWeatherCount = 0;

// ===== 颜色（RGB565）=====
#define C_BLACK  0x0000
#define C_WHITE  0xFFFF
#define C_GREEN  0x07E0
#define C_GREY   0x7BEF
#define C_CYAN   0x07FF
#define C_RED    0xF800
#define C_DIM    0x6B4D   // #6B6B6B 预告、非推荐车道用的灰色
#define C_TRACK  0x2945   // #2A2A2A 进度环底色
#define C_YELLOW 0xF623   // #F5C518
#define C_ORANGE 0xFBC3   // #FF7A1A
#define C_SEP    0x31A6   // #333333 车道分隔线
#define C_SIGNRED 0xE104  // #E02020 限速牌红圈

// ===== 屏幕几何：466x466，圆心 (233, 233) =====
#define CX 233
#define TRIP_RING_R 229  // 最外环：全程总进度环（外缘紧贴 233 屏幕边缘，内缘 225）
#define TURN_RING_R 219  // 内侧环：路口倒计时环（外缘紧贴 225，与总进度环无缝贴合）
#define RING_R TURN_RING_R

// ===== 布局（y 坐标）=====
#define SPEED_Y      50    // 车速数字顶部
#define ICON_CY      188   // 没有车道时，图标中心（v1.8.2 起只作初始位置，最终按可用区域自动摆放）
#define ICON_CY_LANE 172   // 有车道时，图标中心
#define ICON_TOP     120   // v1.8.2 图标可用区域上沿（车速 km/h 下方）
#define ICON_MAX_W   250   // 图标最大宽度
#define LANES_Y      238   // 车道行顶部
#define DIST_Y       270   // 没有车道时，距离数字顶部（7 号字，高 56）
#define DIST_Y_LANE  278   // 有车道时，距离数字顶部（6 号字，高 48）
#define ROAD_Y       334   // 没有车道时，路名图片顶部
#define ROAD_Y_LANE  334   // 有车道时，路名图片顶部
#define BOTTOM_Y     372   // 底部信息图片顶部（常驻显示）
#define TEXT_R       200   // 文字必须在这个半径以内（圆环内沿约 215）

// ===== 蓝牙：手机端必须用同样的名字和 UUID =====
#define DEVICE_NAME  "esp32Nav"
String bleName = DEVICE_NAME;        // v7.0 实际蓝牙名 "esp32Nav-XXXX"（XXXX = 蓝牙地址最后两个字节），开机时算好
#define SERVICE_UUID "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define RX_UUID      "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define TX_UUID      "6e400003-b5a3-f393-e0a9-e50e24dcca9e"   // v2.0 圆屏 → 手机通知
BLECharacteristic *txChar = nullptr;

volatile bool bleConnected = false;
String pendingEvt = "";

// ===== 接收队列：蓝牙线程收到的每个数据包原样放进队列，主循环再取出处理 =====
#define RXQ_SLOTS 256  // 扩大接收队列至 256 槽（PSRAM 分配），彻底杜绝高速收图丢包重传
#define RXQ_MAX   512
struct RxMsg { uint16_t len; uint8_t data[RXQ_MAX]; };
RxMsg *rxq = nullptr;
volatile int rxHead = 0, rxTail = 0;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// ===== 导航与行车状态 =====
bool shownConn = false;       // 屏幕上反映的连接状态
bool inNav = false;           // 是否处于导航中
String navDir = "";           // 当前方向
int navDist = 0;              // 当前距离（米）
float navAngle = NAN;         // v1.8 路口实际转弯角度；NAN = 没有，用标准角度
unsigned long lastRxMs = 0;   // 最近一次收到任何消息的时间
bool staleShown = false;      // 是否正在显示“数据中断”提示
bool navDirty = false;        // 导航画面需要重画

int speedKmh = -1;            // 车速，-1 表示还没有
int camLimit = 0, camDist = 0;
bool hasCam = false;          // 前方是否有测速
int laneN = 0;                // 车道数，0 表示不显示
// v3.1 小地图底部的行程信息（"#e,剩余米,到达时间HHMM,红绿灯数"）和前方红绿灯（"#g,距离"）
bool tripOn = false;
bool cruiseOn = false;        // v4.8 巡航模式（没有目的地，只显示地图、车速、电子眼、安全提示）
bool arrivedBannerOn = false;      // 到达目的地顶部卡片提示模式
unsigned long arrivedBannerAt = 0; // 到达卡片弹出时间戳
IdlePowerOffTimer idlePowerOffTimer = {0, false};
IdlePowerSupply idlePowerSupply = {0, false, false};
volatile bool powerOffPending = false;
bool cleanOn = false;         // v4.9 清爽模式：地图照常，去掉红绿灯、电子眼、安全提示、测速牌和行程信息（"#w,1/0"）
int tripRemainM = 0, tripEta = 0, tripLights = 0;
// v7.3 路程进度
#include "TripProgress.h"
TripProgress tripProgress;
bool progOn = true;
long progDone = 0;                   // 这趟已经走掉的里程（米）
int progLastRm = -1;                 // 上一次收到的剩余里程
unsigned long progLastAt = 0;
#define progReset() do { progDone = 0; progLastRm = -1; progLastAt = 0; tripProgress.reset(); } while (0)   // 宏：放这么靠前写成函数会打乱 Arduino 自动生成的声明
int lightDist = -1;           // 前方红绿灯距离（米），-1 = 不显示
// v3.5 小地图上的电子眼（手机发 "#c,坐标系,x,y,限速,类型,..."，最多 6 个）
#define MCAM_MAX 10           // v4.8 6 → 10（电子眼 + 道路安全提示）
int mapCamN = 0, mapCamFrame = -1;
int32_t mapCamX[MCAM_MAX], mapCamY[MCAM_MAX];
int mapCamSpd[MCAM_MAX];
int mapCamTyp[MCAM_MAX];      // v3.8 图标编号（手机换算好的，0~15）
int8_t mapCamPref[MCAM_MAX];  // v3.8 图标放好了没有（-1 = 还没放过）
float mapCamPrefA[MCAM_MAX];  // v4.1 类型图标在小摄像头的哪个方向（屏幕角度）
int8_t mapCamPrefK[MCAM_MAX]; // v4.3 放图标时这一处有几个类型图标（个数变了就重新放）
float mapCamLockH[MCAM_MAX];  // v4.0 放图标时的地图朝向：之后固定不动，转弯后（朝向变了且已经转稳）才重新放
uint8_t laneBg[8], laneFr[8]; // 每条车道的背景方向码、推荐方向码（0xF = 不推荐）

// ===== 文字图片（手机渲染好的 4 位灰度图）=====
#define IMG_SLOTS 18           // 0 = 路名，1 = 底部信息，2 = 来电人名字，3/4 = 天气地名和风湿度，5/6 = 音乐歌名和歌手，12 = 当前聚焦歌词，13/15 = 上句，14/16 = 下句
#define IMG_MAX   8192        // 每张图最多字节数
struct TextImg {
  uint8_t *show;              // 正在显示的图
  uint16_t showW; uint8_t showH;
  uint8_t *recv;              // 正在接收的图
  int recvId; uint16_t recvW; uint8_t recvH; uint8_t recvParts; uint32_t recvMask;
};
TextImg imgs[IMG_SLOTS];
AppleMediaClient appleMedia;
volatile uint32_t mediaConnectionEpoch = 0;
uint32_t lyricImageGeneration = 0; uint16_t lyricImageTitleRevision = 0, lyricImageArtistRevision = 0;
bool musicLyrics = false;                  // #ML,0/1; never persist an enabled default
bool musicLyricsView = false;              // 音乐页是否进入全屏歌词模式
float lyricScrollOffset = 0.0f;           // 切歌词时的平滑翻滚动效垂直位移 (像素)
uint32_t lyricScrollStartMs = 0;          // 翻滚缓动启动时间戳
bool musicDirty = true;
bool musicHave = false, musicPlaying = false;
int musicActions = 0;
int musicPressed = -1, musicAck = 0;
unsigned long musicAckUntil = 0;
uint32_t musicDuration = 0;
uint32_t musicPos = 0;
uint32_t musicPosAt = 0;
bool musicLiked = false;
uint8_t musicRepeatMode = 0;                  // 0: 列表循环, 1: 单曲循环, 2: 随机播放
int8_t musicVolume = 50;                      // 音量百分比 0~100
bool volSliding = false;                      // 九点钟贴边滑动调音量状态
uint32_t volActiveUntil = 0;                  // 音量条激活显示截止时间
bool coverTouchActive = false;                // 是否在专辑封面上操作
uint32_t lastCoverTapAt = 0;                  // 上次封面点击时间 (双击判定)
bool coverPendingSingleTap = false;           // 单击等待延迟判定
int coverSlideDx = 0;                         // 封面水平位移 (拖动与离开动画)
int coverFlySpeed = 0;                        // 封面飞出动画速度 (+向右, -向左, 0无动画)
// HID adds a second GATT service to the existing authenticated navigation connection.
bool hidMusicMode = false;                  // persisted by #HM,0/1; Android explicitly selects 0
BLEHIDDevice *mediaHid = nullptr;
BLECharacteristic *mediaInput = nullptr;
volatile bool hidSubscribed = false, hidSuspended = false, hidReportMode = true;
volatile uint16_t hidSubscriber = 0xFFFF;
volatile bool hidNotifyAccepted = false;
volatile uint32_t hidSessionEpoch = 0;
uint32_t hidSeenEpoch = 0;
bool hidReadyShown = false, hidReleaseOnReady = true;
String pendingHidStatus, pendingLyricsStatus, pendingMediaStatus;
unsigned long hidInitialReleaseAt = 0;
HidMediaKeys::Pulses hidPulses;
unsigned long hidTapAt = 0;
void hidMediaTick();
bool hidMediaReady();
bool wxDirty = false;                 // v5.3 天气页要重画

// ===== 地图右侧边缘滑动缩放控制 =====
float userMapScale = 1.0f;           // 缩放比例 (默认 1.0x，范围 0.2x ~ 1.0x)
bool mapZoomActive = false;          // 是否正在右侧边缘滑动缩放地图
uint32_t mapZoomHintUntil = 0;       // 缩放提示卡片显示截止时间
uint32_t mapZoomBtnUntil = 0;        // 缩放加减按钮显示截止时间（操作后或刚进入导航时显示10s）
int16_t mapZoomStartX = 0;           // 滑动初始 X
int16_t mapZoomStartY = 0;           // 滑动初始 Y
float mapZoomStartScale = 1.0f;      // 滑动初始基础比例
uint32_t lastMapTapAt = 0;           // 保留兼容
int16_t lastMapTapX = 0, lastMapTapY = 0;
unsigned long mapZoomSaveAt = 0;     // 缩放保存到 NVS 的延迟时间

// ===== 车速分段自适应缩放 =====
// <40km/h: 1.0x | 40-60km/h: 0.8x | 60-80km/h: 0.6x | 80-100km/h: 0.4x | >=100km/h: 0.2x
int currentSpeedZone = -1;           // 当前速度区间 (-1: 未初始化, 0: <40, 1: 40-60, 2: 60-80, 3: 80-100, 4: >=100)
bool userZoomOverridden = false;     // 用户是否在当前速度区间临时手动调整了缩放（跨区间时清除重置）

static inline int calculateSpeedZone(float kmh, int curZone) {
  if (kmh < 0.0f) kmh = 0.0f;
  const float HYST = 1.5f; // 1.5 km/h 迟滞防抖门限
  if (curZone < 0) {
    if (kmh < 40.0f) return 0;
    if (kmh < 60.0f) return 1;
    if (kmh < 80.0f) return 2;
    if (kmh < 100.0f) return 3;
    return 4;
  }
  switch (curZone) {
    case 0:
      if (kmh >= 40.0f + HYST) return (kmh >= 60.0f) ? calculateSpeedZone(kmh, -1) : 1;
      return 0;
    case 1:
      if (kmh < 40.0f - HYST) return 0;
      if (kmh >= 60.0f + HYST) return (kmh >= 80.0f) ? calculateSpeedZone(kmh, -1) : 2;
      return 1;
    case 2:
      if (kmh < 60.0f - HYST) return (kmh < 40.0f) ? calculateSpeedZone(kmh, -1) : 1;
      if (kmh >= 80.0f + HYST) return (kmh >= 100.0f) ? calculateSpeedZone(kmh, -1) : 3;
      return 2;
    case 3:
      if (kmh < 80.0f - HYST) return (kmh < 60.0f) ? calculateSpeedZone(kmh, -1) : 2;
      if (kmh >= 100.0f + HYST) return 4;
      return 3;
    case 4:
    default:
      if (kmh < 100.0f - HYST) return (kmh < 80.0f) ? calculateSpeedZone(kmh, -1) : 3;
      return 4;
  }
}

static inline float getSpeedZoneScale(int zone) {
  switch (zone) {
    case 0: return 1.0f;
    case 1: return 0.8f;
    case 2: return 0.6f;
    case 3: return 0.4f;
    case 4: default: return 0.2f;
  }
}

// 手机状态（蓝牙连接后由 App 同步，#BAT,<pct>,<charging>,<sig1>,<sig2>）
int8_t phoneBatPct = -1;             // 手机电量 0~100，-1 表示未连接/未知
bool   phoneBatChg = false;          // 手机是否在充电
int8_t phoneSig1 = -1;               // 卡1信号 0~4 格，-1 表示无卡/未知
int8_t phoneSig2 = -1;               // 卡2信号 0~4 格，-1 表示未插双卡/无卡

void saveMapZoomIfDue() {
  if (!mapZoomSaveAt || (long)(millis() - mapZoomSaveAt) < 0) return;
  mapZoomSaveAt = 0;
  Preferences pr;
  pr.begin("motonav", false);
  pr.putFloat("mapzoom", userMapScale);
  pr.end();
}

// 导航/巡航态轻触空白处唤起的底部五角星按钮显示截止时间
unsigned long navStarUntil = 0;

// 圆屏端独立选路数据结构（支持多条候选路线在圆屏直接选择并启动导航）
struct RoutePlanOption {
  char tag[24];
  int mins;
  char dist[16];
  int lights;
  int trafficStatus; // 0:畅通(绿), 1:缓行(黄), 2:拥堵(红)
};
#define MAX_ROUTE_PLAN_OPTS 3
RoutePlanOption rpOpts[MAX_ROUTE_PLAN_OPTS];
int rpOptCount = 0;
int rpSelected = 0;
int rpMode = 0; // 0:驾车 1:骑行 2:步行
char rpDestName[48] = {0};
char rpErrMsg[48] = {0};
bool routePickerOpen = false;
bool routePickerLoading = false;
unsigned long routePickerErrUntil = 0;

// 跨模块前置声明
void drawLanesAt(float centerX, float y0, float maxW, float maxH);
void touchVectorToCanvas(float &dx, float &dy);
bool mapModeActive();


