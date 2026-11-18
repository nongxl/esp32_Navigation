package com.yuminaga.motonav;

/** 没有路线离线包时，按可见范围和准备耗时提前更新普通底图。 */
public final class MapCoveragePolicy {
    private MapCoveragePolicy() {}

    public static double viewRadius(int kmh) {
        return viewRadius(kmh, 0.70);
    }

    public static double viewRadius(int kmh, double userZoom) {
        double z = Math.max(0.10, Math.min(1.00, userZoom));
        double speedFactor = Math.max(0.50, Math.min(1.00, 1.0 - Math.max(0, kmh) * 0.0042));
        // 结合圆屏实际缩放比例自适应视野，最低支持 0.08 宏观视野覆盖
        double scale = Math.min(0.50, Math.max(0.08, z * speedFactor));
        return 241.0 / scale;
    }

    public static double aheadDistance(int kmh, boolean satellite) {
        return aheadDistance(kmh, satellite, 1200.0);
    }

    public static double aheadDistance(int kmh, boolean satellite, double half) {
        if (satellite) {
            return kmh > 5 ? 180.0 : 0.0;
        }
        if (kmh <= 5) return 0.0;
        double factor = (half > 2000.0) ? 1.5 : 1.0;
        if (kmh >= 100) return 920.0 * factor;
        if (kmh >= 80) return 780.0 * factor;
        if (kmh >= 50) return 620.0 * factor;
        return 450.0 * factor;
    }

    public static boolean shouldRefresh(double dx, double dy, float bearing, int kmh,
                                        boolean satellite, double preparationSeconds) {
        return shouldRefresh(dx, dy, bearing, kmh, satellite, preparationSeconds, satellite ? 700 : 1200);
    }

    public static boolean shouldRefresh(double dx, double dy, float bearing, int kmh,
                                        boolean satellite, double preparationSeconds, double half) {
        return shouldRefresh(dx, dy, bearing, kmh, satellite, preparationSeconds, half, 0.70);
    }

    public static boolean shouldRefresh(double dx, double dy, float bearing, int kmh,
                                        boolean satellite, double preparationSeconds, double half, double userZoom) {
        // 普通图至少按高速大视野预留，并结合圆屏同步的 userZoom 提前刷新
        double radius = satellite ? viewRadius(kmh, userZoom) : viewRadius(Math.max(150, kmh), userZoom);
        double limit = Math.max(160.0, half - radius - (kmh >= 60 ? 120.0 : 60.0));
        double effPrep = Math.max(kmh >= 60 ? 30.0 : 1.0, Math.min(60.0, preparationSeconds));
        double distance = Math.max(0, kmh) / 3.6 * effPrep;
        double angle = Math.toRadians(bearing);
        return Math.max(Math.abs(dx), Math.abs(dy)) >= limit ||
            Math.max(Math.abs(dx + Math.sin(angle) * distance), Math.abs(dy + Math.cos(angle) * distance)) >= limit;
    }
}
