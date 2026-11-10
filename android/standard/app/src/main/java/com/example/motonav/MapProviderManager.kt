package com.example.motonav

import android.content.Context
import android.content.SharedPreferences

/**
 * esp32_Navigation 地图提供商统一管理器
 *
 * 负责调度高德地图（国内）与 OpenStreetMap / Mapbox（国际版）的平滑切换与能力分发
 */
object MapProviderManager {
    private const val PREFS_NAME = "map_provider_config"
    private const val KEY_ACTIVE_PROVIDER = "active_provider_id"

    private var activeProviderType = MapProviderType.AMAP

    fun init(context: Context) {
        val sp = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        val id = sp.getInt(KEY_ACTIVE_PROVIDER, MapProviderType.AMAP.id)
        activeProviderType = MapProviderType.fromId(id)
    }

    fun getActiveType(): MapProviderType = activeProviderType

    fun setActiveType(context: Context, type: MapProviderType) {
        activeProviderType = type
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            .edit()
            .putInt(KEY_ACTIVE_PROVIDER, type.id)
            .apply()
    }

    fun getProvider(context: Context): IMapProvider {
        return when (activeProviderType) {
            MapProviderType.MAPBOX_OSM -> MapboxOsmProvider(context)
            MapProviderType.AMAP -> AmapMapProvider(context)
        }
    }
}

/** 高德地图适配包装实现类 */
class AmapMapProvider(private val context: Context) : IMapProvider {
    override val type: MapProviderType = MapProviderType.AMAP
    override val name: String = "高德地图"

    override fun isReady(context: Context): Boolean {
        return Keys.hasAmap(context)
    }

    override fun planRoute(
        start: GeoPoint?,
        end: GeoPoint,
        mode: AppTravelMode,
        avoidHighway: Boolean,
        callback: (Boolean, List<PlannedRoute>, String?) -> Unit
    ) {
        // 高德原规划逻辑由 RoutePlanActivity 与 LocalRouting 协同处理
        callback(true, emptyList(), null)
    }

    override fun stopGuidance() {
        NaviBridge.stopCruise()
    }
}
