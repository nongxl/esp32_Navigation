package com.yuminaga.motonav

import android.app.Activity
import android.app.Application
import android.content.Context
import android.os.Bundle
import android.os.Handler
import android.os.Looper

/**
 * 后台休眠省电管理器：
 * 1. 当 App 处于后台（无前台操作）且未连接圆屏蓝牙时，开始 3 分钟倒计时；
 * 2. 满 3 分钟后，若依然在后台且未连接，进入深度休眠省电（停止后台常驻服务与持续蓝牙扫描）；
 * 3. 一旦与圆屏建立蓝牙连接，即使没有开始导航，绝不休眠（以保持音乐播放器与待机控制功能）；
 * 4. 用户再次点开 App 回到前台时，自动唤醒并恢复后台服务与蓝牙扫描。
 */
object PowerSaver : Application.ActivityLifecycleCallbacks {
    private const val SLEEP_DELAY_MS = 3 * 60 * 1000L // 3 分钟
    private val handler = Handler(Looper.getMainLooper())
    private var sleepRunnable: Runnable? = null
    private var startedCount = 0
    private var appContext: Context? = null

    @Volatile var isForeground = false
        private set
    @Volatile var isSleeping = false
        private set

    fun init(app: Application) {
        if (appContext != null) return
        appContext = app
        app.registerActivityLifecycleCallbacks(this)
        BleLink.onConnectionChanged = { connected ->
            checkPowerState("BLE connection changed: connected=$connected")
        }
    }

    override fun onActivityStarted(activity: Activity) {
        startedCount++
        if (startedCount == 1) {
            isForeground = true
            checkPowerState("App entered foreground")
        }
    }

    override fun onActivityStopped(activity: Activity) {
        startedCount--
        if (startedCount <= 0) {
            startedCount = 0
            isForeground = false
            checkPowerState("App entered background")
        }
    }

    override fun onActivityCreated(activity: Activity, savedInstanceState: Bundle?) {}
    override fun onActivityResumed(activity: Activity) {}
    override fun onActivityPaused(activity: Activity) {}
    override fun onActivitySaveInstanceState(activity: Activity, outState: Bundle) {}
    override fun onActivityDestroyed(activity: Activity) {}

    fun checkPowerState(reason: String) {
        val ctx = appContext ?: return
        val connected = BleLink.isConnected

        android.util.Log.d("MotoNavPower", "checkPowerState: fg=$isForeground, connected=$connected, sleeping=$isSleeping, reason=$reason")

        if (connected || isForeground) {
            // 只要蓝牙连接着（保持音乐播放器），或处于前台，绝不休眠！
            cancelSleep()
            if (isSleeping && isForeground) {
                // 用户打开 App 回到前台，立即自动唤醒！
                wakeUp(ctx)
            }
        } else {
            // 在后台且未连接蓝牙：开始 3 分钟倒计时
            scheduleSleep(ctx)
        }
    }

    private fun scheduleSleep(ctx: Context) {
        if (sleepRunnable != null || isSleeping) return
        android.util.Log.i("MotoNavPower", "App backgrounded without BLE: scheduled to sleep in 3 minutes")
        val r = Runnable {
            if (!isForeground && !BleLink.isConnected) {
                enterSleep(ctx)
            }
            sleepRunnable = null
        }
        sleepRunnable = r
        handler.postDelayed(r, SLEEP_DELAY_MS)
    }

    private fun cancelSleep() {
        sleepRunnable?.let {
            handler.removeCallbacks(it)
            sleepRunnable = null
            android.util.Log.i("MotoNavPower", "Sleep timer cancelled")
        }
    }

    private fun enterSleep(ctx: Context) {
        isSleeping = true
        android.util.Log.i("MotoNavPower", "Entering sleep mode: stopping background service & BLE auto-scan")
        NavService.stop(ctx)
    }

    private fun wakeUp(ctx: Context) {
        isSleeping = false
        android.util.Log.i("MotoNavPower", "Waking up from sleep mode: restarting NavService & BLE auto-scan")
        NavService.start(ctx)
    }
}

