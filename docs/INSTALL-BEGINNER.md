# esp32_Navigation 中文新手安装指引

本教程提供硬件固件烧录、手机 App 配套安装与 Key 配置全流程说明。

---

## 快速通道：安卓端（Android）小白用户 2 秒免编译打包（推荐 🌟）

如果你使用 Android 手机，**完全不需要安装任何编译环境（无需下载 JDK、Android Studio 或配置 SDK）**：

1. 前往本仓库的 **[Releases 页面](../../releases)** 下载 `esp32_Navigation-QuickPack-Windows.zip` 并解压；
2. 双击运行解压目录下的 **`一键生成专属安装包.bat`**；
3. 工具将在 **2 秒钟** 内自动生成：
   - `esp32_Navigation-personal.apk`（你的专属个人手机安装包）
   - `高德Key申请信息.txt`（内含包名与你的专属 SHA1 安全码）
4. 按照文本里的步骤，打开[高德开放平台](https://console.amap.com/)粘贴包名与 SHA1，免费领取专属 Key；
5. 将 APK 发送到手机安装，在 App 的设置中填入 Key 即可开箱使用！后续升级版本底包直接复用，**无需重新申请 Key**。

---

## 硬件与固件准备

- iPhone 和一台受支持的 Waveshare ESP32-S3-Touch-AMOLED-1.75 圆屏。其他板型不能照此教程刷写。
- 一台可运行 Sideloadly 的 Mac 或 Windows 电脑、可传数据的 USB 线，以及自己的 Apple 账号。Windows 所需 Apple 驱动按 Sideloadly 官方说明安装。
- 若要启用地图和天气，准备自己的高德 **iOS SDK Key** 与和风天气 API Host、API Key；服务权限及额度以服务商规则为准。
- **WorkBuddy 是可选工具，不是 MotoNav 的必需组件。**你也可以自行下载仓库源码并按下面的构建说明操作。不要把 Apple 账号密码或未遮蔽的服务密钥发给他人、写进公开仓库。

![MotoNav 安装教程封面示意图](media/install-guide/01.png)

## 2. 获取项目并确认板型

从 [MotoNav 官方仓库](https://github.com/zyn950929/MotoNav)获取源码。图片里的 WorkBuddy 和电脑目录只是示意；你不需要使用相同工具、目录或把 GitHub 账号交给任何助手。

确认圆屏背面/商品型号为 **Waveshare ESP32-S3-Touch-AMOLED-1.75**。仓库正式配置为 466×466 的 1.75 板型；图片里写的“1.75C”不是本文支持型号。不要把 1.75 固件刷到 1.85B、M5Stack StopWatch 或其他板子上。

![设备和准备事项示意图](media/install-guide/02.png)

## 3. 构建并烧录圆屏固件（可选）

如果要使用 7.12.3 固件，先准备 Arduino CLI、Espressif ESP32 Core 3.3.10 和仓库内的依赖库，再在仓库根目录运行对应构建命令：

```powershell
.\firmware\build.ps1 -Board 175
```

macOS 或 Linux 使用：

```sh
python3 firmware/build.py
```

构建产物和校验清单位于 `firmware/dist/175/panel0/`。**构建命令只编译、打包，不会烧录到圆屏。**请核对板型、固件版本、镜像偏移和 SHA-256 后，再按仓库的 [Mac 烧录说明](../README.md#mac-双击烧录)执行烧录。该烧录入口只支持本仓库的 1.75 板型；首次完整刷写可能影响原程序和设置，操作前先备份。

当前仓库提供的 Mac 烧录入口不能据此推断 Windows 有适用于 7.12.3 的一键烧录包；不要使用标为 7.12.1 的旧 Windows 包刷写 7.12.3。若不熟悉固件刷写，请先停在构建步骤，确认镜像与设备型号再继续。

![从仓库获取源码的流程示意图](media/install-guide/03.png)

![构建与烧录流程示意图](media/install-guide/04.png)

固件版本、构建参数和验证边界见 [7.12.3 更新说明](RELEASE-FIRMWARE-7.12.3.md) 与 [本次验证记录](VERIFY-2026-10-21.md)。

## 4. 安装 iPhone App

1. 从 [MotoNav GitHub Releases](https://github.com/zyn950929/MotoNav/releases) 下载 `MotoNav-0.13.8-unsigned.ipa`。
2. 从 [Sideloadly 官网](https://sideloadly.io/)安装适用于电脑的版本，用 USB 连接并解锁 iPhone，按手机提示信任电脑。
3. 将 IPA 拖入 Sideloadly，选择自己的 iPhone，按提示使用自己的 Apple 账号完成签名和安装。账号验证只在 Sideloadly/Apple 流程中进行，不要交给项目作者或 WorkBuddy。
4. iOS 16 及以上如提示，前往“设置 → 隐私与安全性 → 开发者模式”开启并按提示重启；如提示开发者不受信任，前往“设置 → 通用 → VPN 与设备管理”信任自己的开发者账号。

免费 Apple 账号的签名通常有效 7 天，到期后需重新签名；同时存在安装数量等限制。具体以 [Sideloadly 官方 FAQ](https://sideloadly.io/faq.html) 和 Apple 当前规则为准。更多续签、更新与故障处理见 [iPhone 安装与续签说明](INSTALL-IOS.md)。

![Sideloadly 安装示意图](media/install-guide/05.png)

## 5. 配置地图与天气

打开 MotoNav，按需授权定位、蓝牙及后台定位。进入 App 的 Key 设置页，查看安装后显示的**实际 Bundle ID**，为该 ID 申请并填写自己的高德 iOS SDK Key；不要直接照抄图片里的示例值。按需填写和风天气 API Host 与 API Key。

签名工具可能调整 App 的 Bundle ID，因此高德 Key 必须绑定 App 实际显示的 ID；重签或更换包名后，原 Key 可能无法使用。不要将真实密钥截图、提交到 GitHub 或发给他人。

![地图和天气密钥配置示意图](media/install-guide/06.png)

## 6. 配对圆屏并检查导航

先在 iPhone 蓝牙设置中配对圆屏，再在 MotoNav 中连接导航设备并按 App 提示开始导航。圆屏与手机的媒体控制配对方式和设置见 [BLE 配对说明](BLE-HID.md)。如果搜不到设备，先确认圆屏已启动、蓝牙已开启，并在手机系统蓝牙设置中完成配对。

首次配置、权限检查和路线预览请在安全停车时完成。行驶中不要操作手机或圆屏；导航信息可能不准确，始终以实际道路、交通信号和现场情况为准。

![蓝牙配对与导航示意图](media/install-guide/07.png)

![安装完成效果示意图](media/install-guide/08.png)

## 相关说明

- [iPhone 安装与续签说明](INSTALL-IOS.md)
- [固件 7.12.3 更新与构建说明](RELEASE-FIRMWARE-7.12.3.md)
- [固件验证结果与未覆盖范围](VERIFY-2026-10-21.md)
- [交通安全与项目许可](../README.md#交通安全与使用风险)
