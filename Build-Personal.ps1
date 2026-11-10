param([string]$JavaPath='', [string]$SdkPath='', [string]$GradlePath='', [string]$Language='', [switch]$Offline)
$ErrorActionPreference = 'Stop'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
$root = $PSScriptRoot
$project = Join-Path $root 'android/standard'
$tools = Join-Path $root '.tools'

# 国际化语言适配 (Auto detect or accept parameter)
if (-not $Language) {
    $sysLang = [System.Globalization.CultureInfo]::CurrentUICulture.TwoLetterISOLanguageName
    $Language = if ($sysLang -eq 'zh') { 'zh' } else { 'en' }
}
function T($zh, $en) { if ($Language -eq 'zh') { $zh } else { $en } }

function Log-Step($step, $text) {
    Write-Host "[$step] $text" -ForegroundColor Cyan
}

function Download-FileWithProgress($url, $destPath, $desc) {
    if (Test-Path -LiteralPath $destPath) { return }
    Write-Host (T "正在下载 $desc..." "Downloading $desc...") -ForegroundColor Yellow
    Write-Host "  URL: $url" -ForegroundColor Gray
    $partPath = "$destPath.part"
    if (Test-Path -LiteralPath $partPath) { Remove-Item -LiteralPath $partPath -Force -ErrorAction SilentlyContinue }
    
    $webClient = New-Object System.Net.WebClient
    $webClient.Headers.Add("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64)")
    try {
        $webClient.DownloadFile($url, $partPath)
        Move-Item -LiteralPath $partPath -Destination $destPath -Force
        Write-Host (T "  $desc 下载完成！" "  $desc downloaded successfully!") -ForegroundColor Green
    } catch {
        if (Test-Path -LiteralPath $partPath) { Remove-Item -LiteralPath $partPath -Force -ErrorAction SilentlyContinue }
        throw (T "下载 $desc 失败：$($_.Exception.Message)" "Failed to download $($desc): $($_.Exception.Message)")
    } finally {
        $webClient.Dispose()
    }
}

Write-Host "======================================================================" -ForegroundColor Magenta
Write-Host ("  " + (T "esp32_Navigation 安卓普通版 (Standard Edition) 自动化个人专属打包" "esp32_Navigation Android Standard Edition — Auto Personalized Builder")) -ForegroundColor Magenta
Write-Host "======================================================================" -ForegroundColor Magenta
Write-Host ""

# --------------------------------------------------------------------
# 步骤 1: 检测或安装 Java 21 环境
# --------------------------------------------------------------------
Log-Step "1/5" (T "检测 Java 运行与编译环境..." "Detecting Java runtime and compiler environment...")
if (-not $JavaPath) {
    # 优先检测环境变量 JAVA_HOME
    if ($env:JAVA_HOME -and (Test-Path (Join-Path $env:JAVA_HOME 'bin/java.exe'))) {
        $JavaPath = $env:JAVA_HOME
    } else {
        # 检测系统 PATH 中的 java
        $cmdJava = Get-Command java -ErrorAction SilentlyContinue
        if ($cmdJava) {
            $binDir = Split-Path $cmdJava.Source
            if (Test-Path (Join-Path $binDir 'keytool.exe')) {
                $JavaPath = Split-Path $binDir
            }
        }
    }
    # 检测常见安装路径 (Android Studio jbr 或 Program Files)
    if (-not $JavaPath) {
        $commonPaths = @(
            "$env:ProgramFiles\Android\Android Studio\jbr",
            "$env:LOCALAPPDATA\Programs\Android Studio\jbr",
            "$env:ProgramFiles\Java\jdk-21",
            "$env:ProgramFiles\Eclipse Adoptium\jdk-21*"
        )
        foreach ($cp in $commonPaths) {
            $matched = Get-ChildItem -Path $cp -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($matched -and (Test-Path (Join-Path $matched.FullName 'bin/keytool.exe'))) {
                $JavaPath = $matched.FullName
                break
            }
        }
    }
}

if (-not $JavaPath) {
    $jdk = Get-ChildItem -Path "$tools/jdk/*/bin/java.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $jdk) {
        New-Item -ItemType Directory -Force $tools | Out-Null
        # 优先使用微软国内 Azure CDN 高速分发的 OpenJDK 21（国内连接极速且稳定）
        $msJdkUrl = 'https://aka.ms/download-jdk/microsoft-jdk-21-windows-x64.zip'
        $adoptiumUrl = 'https://api.adoptium.net/v3/binary/latest/21/ga/windows/x64/jdk/hotspot/normal/eclipse'
        try {
            Download-FileWithProgress $msJdkUrl "$tools/jdk.zip" "OpenJDK 21 运行环境 (微软官方高速 CDN)"
        } catch {
            Write-Host "微软 CDN 下载异常，尝试使用备选 Adoptium 源..." -ForegroundColor Yellow
            Download-FileWithProgress $adoptiumUrl "$tools/jdk.zip" "OpenJDK 21 运行环境 (Adoptium 官方源)"
        }
        Write-Host "正在解压 Java 环境..." -ForegroundColor Yellow
        Expand-Archive -LiteralPath "$tools/jdk.zip" -DestinationPath "$tools/jdk" -Force
        $jdk = Get-ChildItem -Path "$tools/jdk/*/bin/java.exe" | Select-Object -First 1
    }
    $JavaPath = Split-Path (Split-Path $jdk.FullName)
}

$env:JAVA_HOME = $JavaPath
$env:PATH = "$JavaPath\bin;" + $env:PATH
Write-Host "  Java 路径: $JavaPath" -ForegroundColor DarkGray

# --------------------------------------------------------------------
# 步骤 2: 检测或安装 Android SDK
# --------------------------------------------------------------------
Log-Step "2/5" "检测 Android SDK 构建套件..."
if (-not $SdkPath) {
    $candidateProps = @(
        (Join-Path $project 'local.properties'),
        (Join-Path $root 'android/countdown/local.properties')
    )
    foreach ($lp in $candidateProps) {
        if (Test-Path $lp) {
            $line = Get-Content $lp | Where-Object { $_ -match '^sdk\.dir\s*=\s*(.+)$' } | Select-Object -First 1
            if ($line) {
                $val = ($line -split '=', 2)[1].Trim()
                $val = $val.Replace('\: ', ': ').Replace('\:', ':').Replace('\/', '/').Replace('\', '/')
                if (Test-Path $val) { $SdkPath = $val; break }
            }
        }
    }
    if (-not $SdkPath -and $env:ANDROID_HOME -and (Test-Path $env:ANDROID_HOME)) { $SdkPath = $env:ANDROID_HOME }
    if (-not $SdkPath -and $env:ANDROID_SDK_ROOT -and (Test-Path $env:ANDROID_SDK_ROOT)) { $SdkPath = $env:ANDROID_SDK_ROOT }
    if (-not $SdkPath) {
        $defaultSdk = Join-Path $env:USERPROFILE 'AppData/Local/Android/Sdk'
        if (Test-Path $defaultSdk) { $SdkPath = $defaultSdk }
    }
    if (-not $SdkPath) { $SdkPath = Join-Path $tools 'android-sdk' }
}

$env:ANDROID_HOME = $SdkPath
$env:ANDROID_SDK_ROOT = $SdkPath

# 自动静默注入 Android SDK 协议许可，杜绝命令行弹出复杂英文并阻断用户
function Ensure-SdkLicenses($rootSdk) {
    $licDir = Join-Path $rootSdk 'licenses'
    New-Item -ItemType Directory -Force $licDir | Out-Null
    $licFile = Join-Path $licDir 'android-sdk-license'
    $knownHashes = "24333f8a63b1d8e60fe4ac64ab9463cbd8648bfa`n8933bad161af4178b1185d1a37fbf41ea5269c55`nd56f5187479451eabf01fb78af6dfcb131a6481e`n"
    [IO.File]::WriteAllText($licFile, $knownHashes)
    $previewLic = Join-Path $licDir 'android-sdk-preview-license'
    [IO.File]::WriteAllText($previewLic, "84831b9409646a918e30573bab4c9c91346d8abd`n")
}
Ensure-SdkLicenses $SdkPath

$apksigner = Get-ChildItem -Path "$SdkPath/build-tools/*/apksigner.bat" -ErrorAction SilentlyContinue | Sort-Object FullName -Descending | Select-Object -First 1
$hasPlatform = Test-Path "$SdkPath/platforms/android-35/android.jar"

if (-not $apksigner -or -not $hasPlatform) {
    New-Item -ItemType Directory -Force $tools | Out-Null
    $manager = Join-Path $SdkPath 'cmdline-tools/latest/bin/sdkmanager.bat'
    if (-not (Test-Path $manager)) {
        Download-FileWithProgress 'https://dl.google.com/android/repository/commandlinetools-win-15859902_latest.zip' "$tools/sdk-tools.zip" "Android SDK 命令行工具套件"
        Write-Host "正在解压 Android SDK 命令行工具..." -ForegroundColor Yellow
        Expand-Archive -LiteralPath "$tools/sdk-tools.zip" -DestinationPath "$tools/sdk-unpacked" -Force
        New-Item -ItemType Directory -Force "$SdkPath/cmdline-tools/latest" | Out-Null
        Copy-Item -Path "$tools/sdk-unpacked/cmdline-tools/*" -Destination "$SdkPath/cmdline-tools/latest" -Recurse -Force
    }
    Ensure-SdkLicenses $SdkPath
    Write-Host "正在静默安装 Android SDK 核心平台包 (API 35 与 Build-Tools 35)..." -ForegroundColor Yellow
    & $manager "--sdk_root=$SdkPath" 'platforms;android-35' 'build-tools;35.0.0' 'platform-tools'
    if ($LASTEXITCODE -ne 0) { throw 'Android SDK 套件自动下载安装失败，请检查网络！' }
    $apksigner = Get-ChildItem -Path "$SdkPath/build-tools/*/apksigner.bat" | Sort-Object FullName -Descending | Select-Object -First 1
}

Write-Host "  SDK 路径:  $SdkPath" -ForegroundColor DarkGray
Write-Host "  签名工具:  $($apksigner.FullName)" -ForegroundColor DarkGray

# --------------------------------------------------------------------
# 步骤 3: 生成个人专属 ApplicationId 与独立数字证书
# --------------------------------------------------------------------
Log-Step "3/5" "配置个人专属随机包名与独立数字证书..."
$identity = Join-Path $project 'personal.properties'
$signing = Join-Path $project 'keystore/keystore.properties'
$keyfile = Join-Path $project 'keystore/personal.jks'
New-Item -ItemType Directory -Force (Split-Path -Parent $keyfile) | Out-Null

if (-not (Test-Path $identity)) {
    if (Test-Path $keyfile) { throw 'personal.properties 缺失！若曾生成过证书，请从备份中恢复，防止覆盖导致高德 Key 无法识别。' }
    $appId = 'com.motonav.u' + [Guid]::NewGuid().ToString('N')
    [IO.File]::WriteAllText($identity, "personalApplicationId=$appId`n")
}
$appId = (([IO.File]::ReadAllText($identity)).Trim() -split '=', 2)[1].Trim()
if ($appId -notmatch '^com\.motonav\.u[a-f0-9]{32}$') { throw 'personal.properties 中的包名格式不正确。' }

if (-not (Test-Path $signing)) {
    if (Test-Path $keyfile) { throw 'keystore.properties 缺失，请从备份中恢复。' }
    $password = [Guid]::NewGuid().ToString('N') + [Guid]::NewGuid().ToString('N')
    $env:MOTONAV_SIGN_PASSWORD = $password
    & "$JavaPath/bin/keytool.exe" -genkeypair -storetype JKS -keystore $keyfile -storepass:env MOTONAV_SIGN_PASSWORD -keypass:env MOTONAV_SIGN_PASSWORD -alias motonav -keyalg RSA -keysize 2048 -validity 10000 -dname 'CN=MotoNav Personal,O=MotoNav,C=CN'
    if ($LASTEXITCODE -ne 0) { throw '专属签名数字证书生成失败。' }
    [IO.File]::WriteAllText($signing, "storeFile=personal.jks`nstorePassword=$password`nkeyAlias=motonav`nkeyPassword=$password`n")
}
if (-not (Test-Path $keyfile)) { throw '未找到 personal.jks 证书文件。' }

$props = ConvertFrom-StringData ([IO.File]::ReadAllText($signing))
$env:MOTONAV_SIGN_PASSWORD = $props.storePassword
$out = Join-Path $root 'output'
New-Item -ItemType Directory -Force $out | Out-Null
& "$JavaPath/bin/keytool.exe" -exportcert -keystore $keyfile -storepass:env MOTONAV_SIGN_PASSWORD -alias motonav -file "$out/certificate.der"
if ($LASTEXITCODE -ne 0) { throw '证书公钥导出失败。' }
$fingerprint = (Get-FileHash -LiteralPath "$out/certificate.der" -Algorithm SHA1).Hash -replace '(..)(?!$)', '$1:'

# 写入便于用户复制的文本
$infoContent = "======================================================================`r`n" +
               "  高德开放平台 (console.amap.com) 申请 Key 所需信息：`r`n" +
               "======================================================================`r`n`r`n" +
               "包名 (Package Name):`r`n$appId`r`n`r`n" +
               "SHA1 安全码:`r`n$fingerprint`r`n`r`n" +
               "说明：在开放平台创建应用 -> 添加 Key -> 服务平台选 Android 平台，依次粘贴上方包名和 SHA1 即可免费获取 Key！`r`n"
[IO.File]::WriteAllText("$out/Application-info.txt", $infoContent)

# 写入 local.properties 供 Gradle 识别 SDK 路径
$escapedSdk = $SdkPath.Replace('\', '/').Replace(':', '\:')
[IO.File]::WriteAllText("$project/local.properties", "sdk.dir=$escapedSdk`n")

# --------------------------------------------------------------------
# 步骤 4: 执行 Gradle 编译
# --------------------------------------------------------------------
Log-Step "4/5" (T "正在执行 Gradle 自动化编译与打包（请稍候 1~3 分钟）..." "Executing automated Gradle build (please wait 1-3 minutes)...")
if (-not $GradlePath) { $GradlePath = Join-Path $project 'gradlew.bat' }
Push-Location -LiteralPath $project
try {
    $buildArgs = @('--no-daemon', '--max-workers=1', '-Dorg.gradle.jvmargs=-Xmx1536m -Dfile.encoding=UTF-8', '-Pkotlin.compiler.execution.strategy=in-process', "-PpersonalApplicationId=$appId", 'assembleRelease')
    if ($Offline) { $buildArgs = @('--offline') + $buildArgs }
    & $GradlePath @buildArgs
    if ($LASTEXITCODE -ne 0) { throw (T 'Gradle 编译失败，请检查上方日志输出！' 'Gradle build failed, please check the logs above!') }
} finally {
    Pop-Location
}

# --------------------------------------------------------------------
# 步骤 5: 验证签名并导出 APK
# --------------------------------------------------------------------
Log-Step "5/5" (T "校验 APK 签名并导出安装包..." "Verifying APK signature and exporting installer...")
$apk = Join-Path $project 'app/build/outputs/apk/release/app-release.apk'
& $apksigner.FullName verify --verbose --print-certs $apk
if ($LASTEXITCODE -ne 0) { throw (T 'APK 签名有效性校验失败！' 'APK signature verification failed!') }

$finalApk = Join-Path $out "esp32_Navigation-personal.apk"
Copy-Item -LiteralPath $apk -Destination $finalApk -Force
$env:MOTONAV_SIGN_PASSWORD = $null

Write-Host ""
Write-Host "======================================================================" -ForegroundColor Green
Write-Host ("                " + (T "[打包成功] 个人专属 APK 生成完毕！" "[BUILD SUCCESS] Personalized APK generated successfully!")) -ForegroundColor Green
Write-Host "======================================================================" -ForegroundColor Green
Write-Host ""
Write-Host ((T "【专属包名】: " "[Package Name]: ") + $appId) -ForegroundColor Yellow
Write-Host ((T "【证书SHA1】: " "[Cert SHA-1 ]: ") + $fingerprint) -ForegroundColor Yellow
Write-Host ""
Write-Host ("  1. " + (T "手机安装包路径: output\esp32_Navigation-personal.apk" "Mobile APK file path: output\esp32_Navigation-personal.apk")) -ForegroundColor White
Write-Host ("  2. " + (T "高德Key申请信息已保存至: output\Application-info.txt" "Map Key registration info saved to: output\Application-info.txt")) -ForegroundColor White
Write-Host ("  3. " + (T "支持国内高德地图与国际版 OpenStreetMap / Mapbox 双引擎导航！" "Supports both AMap (Domestic) and OpenStreetMap / Mapbox (Global) navigation!")) -ForegroundColor White
Write-Host ""
Write-Host (T "【备份提示】后续更新覆盖安装时，请妥善备份以下文件，以保证签名与包名不变：" "[Backup Notice] Please backup the following files to preserve your package name & key:") -ForegroundColor Gray
Write-Host "  - android/standard/personal.properties" -ForegroundColor Gray
Write-Host "  - android/standard/keystore/personal.jks" -ForegroundColor Gray
Write-Host "======================================================================" -ForegroundColor Green
