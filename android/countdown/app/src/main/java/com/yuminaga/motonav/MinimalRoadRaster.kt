package com.yuminaga.motonav

/**
 * Minimalist map raster classifier from a label-free AMap SDK image (4-class 0..3):
 * 0 = Land background
 * 1 = Minor / local roads (AMap white roads)
 * 2 = Subtle park & water land blocks (unified low-contrast slate tint)
 * 3 = Major / arterial / highway roads (AMap yellow / orange roads)
 */
object MinimalRoadRaster {
    fun isMinorRoad(c: Int): Boolean {
        val r = (c ushr 16) and 255; val g = (c ushr 8) and 255; val b = c and 255
        return r >= 254 && g >= 254 && b >= 254
    }
    fun isMajorRoad(c: Int): Boolean {
        val r = (c ushr 16) and 255; val g = (c ushr 8) and 255; val b = c and 255
        return r >= 200 && g in 125..245 && b + 25 < g && r >= g && r - b >= 45
    }
    fun isRoad(c: Int): Boolean = isMinorRoad(c) || isMajorRoad(c)

    fun classifyPixel(c: Int): Int {
        if (isMajorRoad(c)) return 3
        if (isMinorRoad(c)) return 1
        return when (MapGeo.classifyPixel(c, AmapRasterColors.ROAD_REFERENCE_LUM)) {
            MapGeo.PX_PARK, MapGeo.PX_WATER -> 2
            else -> 0
        }
    }

    fun indices(raw: IntArray, n: Int): ByteArray {
        require(raw.size == n * n)
        val rawCls = ByteArray(raw.size) { classifyPixel(raw[it]).toByte() }
        // Clean isolated SDK icons/noise while keeping minor and major roads connected.
        val baseMask = ByteArray(raw.size) { i ->
            val v = rawCls[i].toInt()
            (if (v == 3) 1 else v).toByte()
        }
        val cleaned = MapGeo.cleanRaster(baseMask, n, n)
        for (i in cleaned.indices) {
            val v = cleaned[i].toInt() and 3
            cleaned[i] = if (v == 1 && rawCls[i].toInt() == 3) 3.toByte() else v.toByte()
        }
        return cleaned
    }
}
