package com.example.motonav

/**
 * esp32_Navigation 全场景地图服务适配层统一契约
 * 
 * 抽象统一各家地图底图、地点搜索、路线规划与导航指引，
 * 彻底解耦高德、OpenStreetMap / Mapbox 以及未来其他扩展。
 */

enum class MapProviderType(val id: Int, val displayName: String, val isGlobal: Boolean) {
    AMAP(0, "高德地图 (AMap · 国内推荐)", false),
    MAPBOX_OSM(1, "OpenStreetMap / Mapbox (国际/全球开源)", true);

    companion object {
        fun fromId(id: Int): MapProviderType = entries.find { it.id == id } ?: AMAP
    }
}

/** 统一通用地理坐标点（经纬度均以标准 WGS-84 存储，国内 SDK 内部加偏） */
data class GeoPoint(
    val lat: Double,
    val lng: Double,
    val name: String = "",
    val address: String = ""
)

/** 统一出行方式 */
enum class AppTravelMode(val raw: Int, val label: String) {
    DRIVE(0, "驾车"),
    BICYCLE(1, "骑行"),
    WALK(2, "步行"),
    MOTORCYCLE(3, "摩托");

    companion object {
        fun fromLocalMode(m: Int): AppTravelMode = when (m) {
            1 -> BICYCLE
            2 -> WALK
            3 -> MOTORCYCLE
            else -> DRIVE
        }
    }
}

/** 统一导航单步动作（Turn-by-Turn） */
data class GuidanceStep(
    val instruction: String,
    val distanceM: Int,
    val turnSymbol: String, // "S", "L", "R", "SL", "SR", "U", "A"
    val turnAngle: Int,     // 0~360
    val roadName: String,
    val endPoint: GeoPoint
)

/** 统一路径规划结果 */
data class PlannedRoute(
    val id: Int,
    val provider: MapProviderType,
    val mode: AppTravelMode,
    val totalDistanceM: Int,
    val totalDurationS: Int,
    val trafficLights: Int = 0,
    val tolls: Int = 0,
    val polyline: List<GeoPoint>,
    val steps: List<GuidanceStep>,
    val label: String = "推荐路线"
)

/** 通用地图服务提供商接口 */
interface IMapProvider {
    val type: MapProviderType
    val name: String

    /** 是否已完成必要配置（如 Key 或网络环境） */
    fun isReady(context: android.content.Context): Boolean

    /** 发起路线规划 */
    fun planRoute(
        start: GeoPoint?,
        end: GeoPoint,
        mode: AppTravelMode,
        avoidHighway: Boolean,
        callback: (Boolean, List<PlannedRoute>, String?) -> Unit
    )

    /** 停止导航 */
    fun stopGuidance()
}

