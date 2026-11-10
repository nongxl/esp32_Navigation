package com.example.motonav

import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import org.json.JSONArray
import org.json.JSONObject
import java.io.BufferedReader
import java.io.InputStreamReader
import java.net.HttpURLConnection
import java.net.URL
import java.net.URLEncoder
import java.util.concurrent.Executors

/**
 * 实时歌词同步桥接模块：
 * 1. 优先从网易云音乐公开接口秒级检索同步 LRC 歌词时间轴 (华语流行歌曲覆盖率极高、国内直连极速响应)；
 * 2. 备用全球开源 LRCLIB 数据库；
 * 3. 兜底兼容传统车载蓝牙歌词模式 (播放器开启"车载蓝牙歌词"时直接接管实时单句)；
 * 4. 本地高精度时钟插值跟踪播放进度，毫秒级定位当前唱句、前一句与后一句；
 * 5. 动态渲染为 4-bit 灰度位图，分发至圆屏槽位 12 (当前高亮聚焦行)、13 (上一句)、14 (下一句)。
 */
object LyricsBridge {
    data class LyricCue(val timeMs: Long, val text: String)

    private val handler = Handler(Looper.getMainLooper())
    private val executor = Executors.newSingleThreadExecutor()

    private val lyricsCache = HashMap<String, List<LyricCue>>()

    private var currentTrackKey = ""
    private var currentRawTitle = ""
    private var activeCues: List<LyricCue> = emptyList()
    private var isPlaying = false
    private var basePosMs = 0L
    private var baseTimeMs = 0L
    private var lastCueIdx = -2
    private var isFetching = false

    private val ticker = object : Runnable {
        override fun run() {
            try {
                tick()
                if (isPlaying && activeCues.isNotEmpty()) {
                    handler.postDelayed(this, 250)
                }
            } catch (_: Throwable) {}
        }
    }

    fun update(title: String, artist: String, durationSec: Int, posMs: Long, playing: Boolean) {
        if (Looper.myLooper() != Looper.getMainLooper()) {
            handler.post { update(title, artist, durationSec, posMs, playing) }
            return
        }

        try {
            val cleanTitle = cleanTrackName(title)
            val cleanArtist = cleanArtistName(artist)
            val trackKey = "$cleanTitle - $cleanArtist"
            val trackChanged = (trackKey != currentTrackKey)

            if (trackChanged) {
                currentTrackKey = trackKey
                currentRawTitle = title
                lastCueIdx = -2
                activeCues = emptyList()
                clearScreenLyrics()

                val cached = lyricsCache[trackKey]
                if (cached != null) {
                    activeCues = cached
                } else if (cleanTitle.isNotBlank() && cleanTitle != "未知歌曲" && cleanTitle != "暂无音乐播放") {
                    fetchLyrics(cleanTitle, cleanArtist, durationSec, trackKey)
                }
            } else if (title != currentRawTitle) {
                // 车载蓝牙歌词模式 (车机蓝牙原理)：
                // 当用户在 QQ音乐 / 网易云 / 酷狗中开启【车载蓝牙歌词】时，
                // 播放器会把当前在唱的单句实时塞进 Title 字段！
                currentRawTitle = title
                if (activeCues.isEmpty() && isPlaying && title.isNotBlank()) {
                    // 若未命中完整 LRC 时间轴，直接将播放器吐出的这句歌词推送到圆屏当前行槽位 12
                    dispatchSingleLyric(title)
                }
            }

            isPlaying = playing
            basePosMs = if (posMs >= 0) posMs else 0L
            baseTimeMs = SystemClock.elapsedRealtime()

            handler.removeCallbacks(ticker)
            tick()
            if (isPlaying && activeCues.isNotEmpty()) {
                handler.postDelayed(ticker, 250)
            }
        } catch (_: Throwable) {}
    }

    private fun tick() {
        if (activeCues.isEmpty()) return

        val curPos = if (isPlaying) {
            basePosMs + (SystemClock.elapsedRealtime() - baseTimeMs)
        } else {
            basePosMs
        }

        var curIdx = -1
        for (i in activeCues.indices) {
            if (curPos >= activeCues[i].timeMs) {
                curIdx = i
            } else {
                break
            }
        }

        if (curIdx != lastCueIdx) {
            lastCueIdx = curIdx
            dispatchLyrics(curIdx)
        }
    }

    private fun dispatchLyrics(curIdx: Int) {
        try {
            val prevText = if (curIdx > 0 && curIdx - 1 < activeCues.size) activeCues[curIdx - 1].text else ""
            val curText = if (curIdx >= 0 && curIdx < activeCues.size) activeCues[curIdx].text else ""
            val nextText = if (curIdx + 1 < activeCues.size) activeCues[curIdx + 1].text else ""

            // 槽位 12：当前在唱高亮放大聚焦行 (字号 34f，加粗突出)
            val curImg = if (curText.isNotBlank()) TextImage.render(curText.take(80), 400, 34f, 22f) else null
            BleLink.sendImage(12, curImg?.w ?: 0, curImg?.h ?: 0, curImg?.data ?: byteArrayOf())

            // 槽位 13：上一句歌词 (字号 24f，暗淡纵深)
            val prevImg = if (prevText.isNotBlank()) TextImage.render(prevText.take(80), 340, 24f, 16f) else null
            BleLink.sendImage(13, prevImg?.w ?: 0, prevImg?.h ?: 0, prevImg?.data ?: byteArrayOf())

            // 槽位 14：下一句歌词 (字号 24f，暗淡纵深)
            val nextImg = if (nextText.isNotBlank()) TextImage.render(nextText.take(80), 340, 24f, 16f) else null
            BleLink.sendImage(14, nextImg?.w ?: 0, nextImg?.h ?: 0, nextImg?.data ?: byteArrayOf())
        } catch (_: Throwable) {}
    }

    private fun dispatchSingleLyric(text: String) {
        try {
            val curImg = TextImage.render(text.take(80), 400, 34f, 22f)
            BleLink.sendImage(12, curImg?.w ?: 0, curImg?.h ?: 0, curImg?.data ?: byteArrayOf())
        } catch (_: Throwable) {}
    }

    private fun clearScreenLyrics() {
        try {
            BleLink.sendImage(12, 0, 0, byteArrayOf())
            BleLink.sendImage(13, 0, 0, byteArrayOf())
            BleLink.sendImage(14, 0, 0, byteArrayOf())
        } catch (_: Throwable) {}
    }

    private fun fetchLyrics(title: String, artist: String, durationSec: Int, trackKey: String) {
        if (isFetching) return
        isFetching = true

        executor.execute {
            try {
                // 1. 优先从国内网易云音乐接口秒级获取 LRC (覆盖率最高、速度最快)
                var lrcText = queryNeteaseLyrics(title, artist)

                // 2. 备用从 LRCLIB 获取
                if (lrcText.isNullOrBlank()) {
                    lrcText = queryLrcLibGet(title, artist, durationSec)
                }
                if (lrcText.isNullOrBlank()) {
                    lrcText = queryLrcLibSearch(title, artist)
                }

                if (!lrcText.isNullOrBlank()) {
                    val parsed = parseLrc(lrcText)
                    if (parsed.isNotEmpty()) {
                        handler.post {
                            try {
                                if (lyricsCache.size > 50) {
                                    val firstKey = lyricsCache.keys.firstOrNull()
                                    if (firstKey != null) lyricsCache.remove(firstKey)
                                }
                                lyricsCache[trackKey] = parsed
                                if (currentTrackKey == trackKey) {
                                    activeCues = parsed
                                    lastCueIdx = -2
                                    tick()
                                    if (isPlaying) {
                                        handler.removeCallbacks(ticker)
                                        handler.postDelayed(ticker, 250)
                                    }
                                }
                            } catch (_: Throwable) {}
                        }
                    }
                }
            } catch (_: Throwable) {
            } finally {
                isFetching = false
            }
        }
    }

    /**
     * 网易云音乐公开歌词接口 (国内毫秒级响应，华语歌词极全)
     */
    private fun queryNeteaseLyrics(title: String, artist: String): String? {
        return try {
            val query = if (artist.isNotBlank() && artist != "未知") "$title $artist" else title
            val encQ = URLEncoder.encode(query.trim(), "UTF-8")
            val searchUrl = "https://music.163.com/api/search/get/web?s=$encQ&type=1&limit=1"
            val searchJson = httpGet(searchUrl) ?: return null
            val sObj = JSONObject(searchJson)
            val songs = sObj.optJSONObject("result")?.optJSONArray("songs") ?: return null
            if (songs.length() == 0) return null
            val songId = songs.getJSONObject(0).optLong("id")
            if (songId <= 0) return null

            val lrcUrl = "https://music.163.com/api/song/lyric?os=pc&id=$songId&lv=-1&kv=-1&tv=-1"
            val lrcJson = httpGet(lrcUrl) ?: return null
            val lyric = JSONObject(lrcJson).optJSONObject("lrc")?.optString("lyric")
            if (!lyric.isNullOrBlank() && lyric != "null") lyric else null
        } catch (_: Throwable) {
            null
        }
    }

    private fun queryLrcLibGet(title: String, artist: String, durationSec: Int): String? {
        return try {
            val encTitle = URLEncoder.encode(title, "UTF-8")
            val encArtist = URLEncoder.encode(artist, "UTF-8")
            val urlStr = "https://lrclib.net/api/get?track_name=$encTitle&artist_name=$encArtist" +
                    (if (durationSec > 0) "&duration=$durationSec" else "")
            httpGetJsonField(urlStr, "syncedLyrics")
        } catch (_: Throwable) {
            null
        }
    }

    private fun queryLrcLibSearch(title: String, artist: String): String? {
        return try {
            val query = "$title $artist".trim()
            val encQ = URLEncoder.encode(query, "UTF-8")
            val urlStr = "https://lrclib.net/api/search?q=$encQ"
            val raw = httpGet(urlStr) ?: return null
            val arr = JSONArray(raw)
            for (i in 0 until arr.length()) {
                val item = arr.getJSONObject(i)
                val synced = item.optString("syncedLyrics", "")
                if (synced.isNotBlank() && synced != "null") {
                    return synced
                }
            }
            null
        } catch (_: Throwable) {
            null
        }
    }

    private fun httpGetJsonField(urlStr: String, field: String): String? {
        val raw = httpGet(urlStr) ?: return null
        return try {
            val obj = JSONObject(raw)
            val v = obj.optString(field, "")
            if (v.isNotBlank() && v != "null") v else null
        } catch (_: Throwable) {
            null
        }
    }

    private fun httpGet(urlStr: String): String? {
        var conn: HttpURLConnection? = null
        return try {
            val url = URL(urlStr)
            conn = (url.openConnection() as HttpURLConnection).apply {
                connectTimeout = 4000
                readTimeout = 4000
                requestMethod = "GET"
                setRequestProperty("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64)")
            }
            if (conn.responseCode == 200) {
                BufferedReader(InputStreamReader(conn.inputStream, Charsets.UTF_8)).use { it.readText() }
            } else {
                null
            }
        } catch (_: Throwable) {
            null
        } finally {
            try { conn?.disconnect() } catch (_: Throwable) {}
        }
    }

    private fun parseLrc(lrcContent: String): List<LyricCue> {
        val list = mutableListOf<LyricCue>()
        val regex = Regex("\\[(\\d{1,2}):(\\d{2}(?:\\.\\d+)?)\\](.*)")
        for (rawLine in lrcContent.lines()) {
            val line = rawLine.trim()
            if (line.isEmpty() || line.startsWith("[ti:") || line.startsWith("[ar:") ||
                line.startsWith("[al:") || line.startsWith("[by:") || line.startsWith("[offset:")) continue

            val match = regex.find(line) ?: continue
            val min = match.groupValues[1].toLongOrNull() ?: 0L
            val secFloat = match.groupValues[2].toDoubleOrNull() ?: 0.0
            val text = match.groupValues[3].trim()
            if (text.isEmpty()) continue

            val timeMs = (min * 60 * 1000) + (secFloat * 1000).toLong()
            list.add(LyricCue(timeMs, text))
        }
        return list.sortedBy { it.timeMs }
    }

    private fun cleanTrackName(name: String): String {
        return name.replace(Regex("\\s*[\\(\\[（【].*?[\\)\\]）】]"), "")
            .replace(Regex("(?i)\\s*-\\s*(remastered|live|instrumental|伴奏).*"), "")
            .trim()
    }

    private fun cleanArtistName(artist: String): String {
        return artist.split('/', '&', ',', ';').firstOrNull()?.trim() ?: artist.trim()
    }
}
