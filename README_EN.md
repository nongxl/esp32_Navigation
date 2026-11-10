# esp32_Navigation — All-Scenario ESP32 Smart Companion Screen & Navigation System

<p align="center">
  <b><a href="README.md">简体中文</a></b> | <b><a href="README_EN.md">English</a></b>
</p>

[![Platform](https://img.shields.io/badge/Platform-ESP32--S3-orange.svg)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/)
[![License](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](LICENSE.md)
[![Screen](https://img.shields.io/badge/Display-466%C3%97466%20AMOLED%20Round-purple.svg)](https://m5stack.com/)

**esp32_Navigation** is an open-source, all-scenario portable smart navigation and wireless companion screen system tailored for **pedestrians (walking), cyclists, motorcyclists, and automobile drivers**.

Moving away from the legacy single-purpose motorcycle narrative and outdated 8-bit pixel art of former MotoNav, this project introduces a **modern, minimalist, sleek and functional design language** (aligned with our modern music player UI, iOS-style glassmorphism HUDs, and subpixel anti-aliased vector rendering), delivering an elegant, versatile companion display for everyday transit and desktop life.

> **Independent Maintenance**: Because the upstream project ceased updates, this repository has branched into a completely independent project named **`esp32_Navigation`** on GitHub for continuous evolution.  
> **Hardware Support**: Primarily optimized for the **M5Stack Stopwatch** (1.75-inch 466×466 round AMOLED display), while maintaining code-level logic alignment for **Waveshare 1.75** round displays and the **iOS Companion App**.  
> ⚠️ **Community Note**: As the maintainer does not have active iOS physical debugging hardware or Waveshare test units, the iOS app and Waveshare firmware rely on open-source community contributions and tests. Pull Requests are warmly welcome!

---

## Key Highlights & Innovations

### 1. Multi-Modal Transit & Dual Map Engines
- **Dual Map Engine Abstraction Layer**:
  - **AutoNavi / AMap**: Recommended for mainland China, offering rich lane-level guidance, speed-camera detection, and real-time traffic;
  - **OpenStreetMap (OSM) / Mapbox**: Native WGS-84 coordinate engine for global and international travelers, eliminating commercial key restrictions and coordinate drift;
- **All-Scenario Transit Modes**: Seamlessly switch between **Car Driving** (traffic congestion avoidance, tolls/traffic-light estimation), **Bicycle/Cycling**, **Walking/Hiking**, and **Motorcycle-specific routes**;
- **Cruise & Road Sign Mode**: Real-time rotating map and safety alerts without setting a destination;
- **Automated GPX Trip Logger**: Automatically logs background GPS tracks locally, ready to export as standard `.gpx` files.

### 2. Modern Minimalist UI (Say Goodbye to Pixelated Aesthetics)
- **Subpixel Anti-Aliased Vector Rendering**: Clean, modern typography and smooth vector clock/gauges replace blocky pixel fonts;
- **Obsidian Dark & Glassmorphism**: Deep space obsidian black (`#111216`) paired with modern translucent cards (`#1B1D24`), rounded capsules (14~18dp), and frosted glass textures;
- **Unified Companion App Design**: Both Android and iOS apps adopt matching dark-card layouts, high-tech deep blue highlights, and ruby-red heart like indicators.

### 3. Round Screen Interactive Experience
- **Standby Dashboard**:
  - Procedural micro-glow vector map background with an elegant, bold digital clock;
  - Quick-Access Favorites Drawer (★): Instantly view saved places and history synced from your phone for one-tap navigation;
- **Modern Music Player**:
  - Swipe album cover to fly out next/previous song with vinyl record base animation;
  - Tap cover center to play/pause (with 140ms anti-jitter filter);
  - Double-tap or tap the bottom base to toggle the ruby-heart Like favorite (bidirectional phone sync);
  - iPhone-style frosted glass volume HUD floating in the center with vector speaker icons and smooth capsule sliders;
- **Attitude & G-Force Meter**:
  - Powered by the onboard BMI270 6-axis IMU, displaying real-time lean angles and 2D G-force vector trajectory balls;
  - Automotive-grade palette: soft mint green (safe), sunset gold (warning), and coral red (limit);
- **On-Screen Quick Settings**:
  - Swipe left on the watchface to adjust brightness (25%/50%/75%/100%), map styles (VICE/DARK/CLEAN/RADAR), screen rotation (0°/90°/180°/270°), and sleep timeouts;
- **Companion Mode & Hardware Shutdown**:
  - Features the KK procedural 60fps dynamic character avatar engine;
  - Hardware-level auto shutdown via the M5PM1 power management IC after 2 minutes of disconnection to prevent standby battery drain.

---

## Quick Start & Firmware Upload

### 1. Flash Firmware (Recommended via M5Burner)
- **Easiest Method**: Search for `esp32_Navigation` or `Stopwatch` in the official **[M5Burner](https://burner.m5stack.com)** app to download and flash with one click.
- **Compile from Source**: Built with PlatformIO for ESP32-S3:
  ```bash
  pio run -e stopwatch -t upload
  ```

### 2. Mobile Companion Apps

The round screen connects to your smartphone over Bluetooth Low Energy (BLE). The phone computes routing and pushes compressed map tiles, turn-by-turn guidance, and media metadata.

#### Android Companion App

##### 🌟 Method 1: Instant Zero-Build Signing (Recommended for Beginners — 2 Seconds, No SDK)
No need to download hundreds of megabytes of Android SDK or compiler tools:
1. **Download QuickPack**: Go to [Releases](../../releases) and download `esp32_Navigation-QuickPack-Windows.zip`;
2. **Instant Packaging**: Extract the archive and double-click `一键生成专属安装包.bat`;
3. **Instant Output**: In less than **2 seconds**, your personal `esp32_Navigation-personal.apk` and `高德Key申请信息.txt` are ready;
4. **Free Key Setup**: Register your free key on AutoNavi (AMap) console following the generated SHA-1, install the APK, and paste your key in App settings!
5. **Seamless Updates**: When updating in the future, simply extract the new base template and run; your signature is preserved without re-registering.

##### 🛠️ Method 2: Full Source Code Local Build (For Developers)
1. **One-Click Local Build**: Run `一键打包-普通版.bat` or execute in PowerShell:
   ```powershell
   .\Build-Personal.ps1 -Language en
   ```
2. **Automatic Build**: The script automatically fetches dependencies, configures your private keystore, and compiles with Gradle. Output is in `output/`.

#### iOS (SwiftUI)
- Built with modern SwiftUI, featuring multi-modal routing, glassmorphism card UI, and BLE communication;
- Open to community developers to build, test, and contribute via Xcode.

---

## Physical Hardware Button Guide (M5Stopwatch)

| Button Action | Standby Watchface Behavior | Navigation / Sub-page Behavior |
| :--- | :--- | :--- |
| **Short Press Left (KEY A)** | Swipe left into Music Player | Return to Home |
| **Short Press Right (KEY B)** | Swipe right into Round Settings | Return to Home |
| **Long Press Left (KEY A, ≥500ms)** | Global Previous Track (Vibration feedback) | Global Previous Track |
| **Long Press Right (KEY B, ≥500ms)** | Global Next Track (Vibration feedback) | Global Next Track |
| **Dual Long Press (A + B, ≥800ms)** | Toggle between Navigation & G-Meter | Exit Companion Mode |

---

## Safety & Disclaimer

1. **Safety First**: Never operate the device or stare at the screen continuously while moving. Always pull over in a safe area before planning or inspecting routes;
2. **Ground Truth**: Information displayed on screen does not replace real-world road conditions, signs, or traffic officers. Always abide by local traffic laws;
3. **Open-Source Non-Commercial**: This software is provided under open-source terms without warranty. Users assume full responsibility for road and operational safety.

---

## Acknowledgments & Licenses

### Upstream & Dependencies
- **[zyn950929/MotoNav](https://github.com/zyn950929/MotoNav)**: Foundation of original navigation workflows;
- **[Trentct/m5stack-stopwatch-avatar](https://github.com/Trentct/m5stack-stopwatch-avatar)**: KK dynamic 60fps avatar engine (AGPL-3.0);
- **[M5Stack](https://m5stack.com/)**: Hardware and driver ecosystem;
- **[vanddccd](https://github.com/vanddccd)**: Hardware-level power management and true shutdown optimization;
- **Qingye**: Extensive road-testing feedback and interaction design suggestions.

### Licensing
- Original code and documentation in this repository are licensed under [LICENSE.md](LICENSE.md);
- The avatar engine components (`AvatarEngine.h`, `AvatarEngine.cpp`) are licensed under **[GNU Affero General Public License v3.0 (AGPL-3.0-or-later)](licenses/LICENSE-AGPL-3.0.txt)**.

