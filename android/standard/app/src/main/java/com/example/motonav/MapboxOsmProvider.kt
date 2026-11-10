package com.example.motonav

import android.content.Context
import android.os.Handler
import android.os.Looper
import org.json.JSONObject
import java.io.BufferedReader
import java.io.InputStreamReader
import java.net.HttpURLConnection
import java.net.URL
import java.util.concurrent.Executors
import kotlin.math.*

/**
 * OpenStreetMap / Mapbox 全球路线规划与导航服务实现类
 *
 * 1. 采用全球标准的 WGS-84 原生坐标，全球可用，无商业资质或加偏限制；
 * 2. 算路引擎：默认直连全球公用 OSRM 路由服务，并兼容 Mapbox Directions 规范；
 * 3. 完美适配全场景出行：驾车 (car/driving)、骑行 (bike/cycling)、步行 (foot/walking)；
 * 4. 自动解析 step 转向角并转化为圆屏所需的抗锯齿转向指令流与距离步进。
 */
class MapboxOsmProvider(private val context: Context) : IMapProvider {
    override val type: MapProviderType = MapProviderType.MAPBOX_OSM
    override val name: String = "OpenStreetMap / Mapbox"

    private val executor = Executors.newSingleThreadExecutor()
    private val mainHandler = Handler(Looper.getMainLooper())
    private var isNavigating = false
    private var guidanceThread: Thread? = null

    override fun isReady(context: Context): Boolean {
        // OpenStreetMap / OSRM 为完全公开服务，开箱即用，无需预先填入 Key 即可完成全球算路
        return true
    }

    override fun planRoute(
        start: GeoPoint?,
        end: GeoPoint,
        mode: AppTravelMode,
        avoidHighway: Boolean,
        callback: (Boolean, List<PlannedRoute>, String?) -> Unit
    ) {
        val s = start ?: GeoPoint(39.9042, 116.4074) // 默认起点备用
        executor.execute {
            try {
                // 确定 OSRM profile
                val profile = when (mode) {
                    AppTravelMode.BICYCLE -> "bike"
                    AppTravelMode.WALK -> "foot"
                    else -> "driving"
                }

                // OSRM 请求格式：lng,lat;lng,lat
                val urlStr = "https://router.project-osrm.org/route/v1/$profile/${s.lng},${s.lat};${end.lng},${end.lat}?overview=full&geometries=geojson&steps=true"
                val url = URL(urlStr)
                val conn = url.openConnection() as HttpURLConnection
                conn.connectTimeout = 10000
                conn.readTimeout = 10000
                conn.setRequestProperty("User-Agent", "esp32_Navigation/1.0 (OpenStreetMap Provider)")

                if (conn.responseCode != 200) {
                    mainHandler.post { callback(false, emptyList(), "OSM 服务响应异常: HTTP ${conn.responseCode}") }
                    return@execute
                }

                val reader = BufferedReader(InputStreamReader(conn.inputStream))
                val resp = reader.readText()
                reader.close()

                val root = JSONObject(resp)
                val code = root.optString("code")
                if (code != "Ok") {
                    mainHandler.post { callback(false, emptyList(), "路线规划未找到可行路径: $code") }
                    return@execute
                }

                val routesJson = root.optJSONArray("routes")
                if (routesJson == null || routesJson.length() == 0) {
                    mainHandler.post { callback(false, emptyList(), "未获取到有效路线数据") }
                    return@execute
                }

                val plannedRoutes = ArrayList<PlannedRoute>()
                for (i in 0 until routesJson.length()) {
                    val r = routesJson.getJSONObject(i)
                    val distanceM = r.optDouble("distance", 0.0).toInt()
                    val durationS = r.optDouble("duration", 0.0).toInt()

                    // 解析 geometry polyline
                    val geometry = r.optJSONObject("geometry")
                    val coords = geometry?.optJSONArray("coordinates")
                    val polyline = ArrayList<GeoPoint>()
                    if (coords != null) {
                        for (ci in 0 until coords.length()) {
                            val pt = coords.getJSONArray(ci)
                            polyline.add(GeoPoint(pt.getDouble(1), pt.getDouble(0)))
                        }
                    }

                    // 解析 steps
                    val legs = r.optJSONArray("legs")
                    val stepsList = ArrayList<GuidanceStep>()
                    if (legs != null && legs.length() > 0) {
                        val steps = legs.getJSONObject(0).optJSONArray("steps")
                        if (steps != null) {
                            for (si in 0 until steps.length()) {
                                val st = steps.getJSONObject(si)
                                val stepDist = st.optDouble("distance", 0.0).toInt()
                                val roadName = st.optString("name", "未知道路").ifBlank { "前进道路" }
                                val maneuver = st.optJSONObject("maneuver")

                                val mType = maneuver?.optString("type", "turn") ?: "turn"
                                val mMod = maneuver?.optString("modifier", "") ?: ""
                                val bearBefore = maneuver?.optInt("bearing_before", 0) ?: 0
                                val bearAfter = maneuver?.optInt("bearing_after", 0) ?: 0
                                var angleDelta = (bearAfter - bearBefore) % 360
                                if (angleDelta < 0) angleDelta += 360

                                val (turnSymbol, instruction) = parseOsmManeuver(mType, mMod, roadName)
                                val loc = maneuver?.optJSONArray("location")
                                val endPt = if (loc != null) GeoPoint(loc.getDouble(1), loc.getDouble(0)) else end

                                stepsList.add(GuidanceStep(instruction, stepDist, turnSymbol, angleDelta, roadName, endPt))
                            }
                        }
                    }

                    plannedRoutes.add(
                        PlannedRoute(
                            id = i,
                            provider = MapProviderType.MAPBOX_OSM,
                            mode = mode,
                            totalDistanceM = distanceM,
                            totalDurationS = durationS,
                            trafficLights = max(1, distanceM / 1500), // 程序化估算
                            tolls = 0,
                            polyline = polyline,
                            steps = stepsList,
                            label = if (i == 0) "OSM 推荐路径" else "备选路线 $i"
                        )
                    )
                }

                mainHandler.post { callback(true, plannedRoutes, null) }
            } catch (e: Exception) {
                mainHandler.post { callback(false, emptyList(), "OSM 规划异常: ${e.message}") }
            }
        }
    }

    override fun stopGuidance() {
        isNavigating = false
        guidanceThread?.interrupt()
        guidanceThread = null
    }

    /** 开启由该引擎驱动的导航，将实时的路口、角度与步进数据同步给 NaviBridge */
    fun startGuidance(route: PlannedRoute, emulate: Boolean) {
        stopGuidance()
        isNavigating = true

        guidanceThread = Thread {
            try {
                var currentStepIdx = 0
                val totalSteps = route.steps.size

                while (isNavigating && currentStepIdx < totalSteps) {
                    val step = route.steps[currentStepIdx]
                    val remainM = step.distanceM
                    val road = step.roadName
                    val dir = step.turnSymbol

                    // 将转向、路名与距离发送给底层 NaviBridge 推送至圆屏
                    mainHandler.post {
                        NaviBridge.manualPushNavData(
                            dir = dir,
                            dist = remainM,
                            speed = if (emulate) 38f else 0f,
                            road = road,
                            angle = step.turnAngle,
                            totalRemainM = route.totalDistanceM,
                            totalRemainS = route.totalDurationS
                        )
                    }

                    if (emulate) {
                        // 模拟步进
                        val sleepMs = max(500L, min(4000L, remainM * 40L))
                        Thread.sleep(sleepMs)
                        currentStepIdx++
                    } else {
                        // 真实导航：等待 GPS 更新推动
                        Thread.sleep(1000)
                    }
                }

                // 达到终点
                if (isNavigating) {
                    mainHandler.post {
                        NaviBridge.manualPushNavData("A", 0, 0f, "已到达目的地", 0, 0, 0)
                    }
                }
            } catch (_: InterruptedException) {}
        }.apply { start() }
    }

    private fun parseOsmManeuver(type: String, modifier: String, road: String): Pair<String, String> {
        return when (type) {
            "depart" -> "S" to "从当前位置出发，沿 $road 前进"
            "arrive" -> "A" to "已到达目的地附近"
            "roundabout" -> "SL" to "进入环岛，沿 $road 驶出"
            "uturn" -> "U" to "在前方掉头"
            else -> {
                when (modifier) {
                    "left" -> "L" to "前方左转进入 $road"
                    "sharp left" -> "L" to "前方急左转进入 $road"
                    "slight left" -> "SL" to "前方靠左前方行驶，进入 $road"
                    "right" -> "R" to "前方右转进入 $road"
                    "sharp right" -> "R" to "前方急右转进入 $road"
                    "slight right" -> "SR" to "前方靠右前方行驶，进入 $road"
                    "uturn" -> "U" to "前方掉头进入 $road"
                    else -> "S" to "沿 $road 继续直行"
                }
            }
        }
    }
}

