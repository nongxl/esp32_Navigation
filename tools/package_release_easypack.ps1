# ====================================================================
# esp32_Navigation - 开发者一键导出小白极速打包工具包 (EasyPack Release)
# ====================================================================

[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$rootDir = (Split-Path -Parent $scriptDir)
$outputDir = Join-Path $rootDir "output"
New-Item -ItemType Directory -Force $outputDir | Out-Null

Write-Host "======================================================================" -ForegroundColor Magenta
Write-Host "  正在构建并打包 esp32_Navigation-QuickPack (小白极速免编译分发包)..." -ForegroundColor Magenta
Write-Host "======================================================================" -ForegroundColor Magenta

# 1. 寻找基础底包
$sourceApk = Join-Path $outputDir "esp32_Navigation-personal.apk"
if (-not (Test-Path $sourceApk)) {
    $sourceApk = Join-Path $rootDir "android\standard\app\build\outputs\apk\release\app-release-unsigned.apk"
}

if (-not (Test-Path $sourceApk)) {
    Write-Host "检测到尚未生成基础 APK，正在自动调用普通版打包脚本编译底包..." -ForegroundColor Yellow
    & (Join-Path $rootDir "Build-Personal.ps1")
    $sourceApk = Join-Path $outputDir "esp32_Navigation-personal.apk"
}

if (-not (Test-Path $sourceApk)) {
    Write-Host "【错误】无法找到或生成基础底包！" -ForegroundColor Red
    exit 1
}

# 2. 创建临时打包目录
$stageDir = Join-Path $outputDir "QuickPack_Staging"
if (Test-Path $stageDir) { Remove-Item -LiteralPath $stageDir -Recurse -Force }
New-Item -ItemType Directory -Force (Join-Path $stageDir "tools") | Out-Null

Write-Host "-> 复制基础底包为 esp32_Navigation-template.apk..." -ForegroundColor Yellow
Copy-Item -LiteralPath $sourceApk -Destination (Join-Path $stageDir "esp32_Navigation-template.apk") -Force

Write-Host "-> 复制启动脚本与签名工具..." -ForegroundColor Yellow
Copy-Item -LiteralPath (Join-Path $rootDir "一键生成专属安装包.bat") -Destination $stageDir -Force
Copy-Item -LiteralPath (Join-Path $rootDir "tools\Quick-Sign.ps1") -Destination (Join-Path $stageDir "tools\Quick-Sign.ps1") -Force
Copy-Item -LiteralPath (Join-Path $rootDir "tools\apksigner.jar") -Destination (Join-Path $stageDir "tools\apksigner.jar") -Force

# 创建小白说明文件
$lines = @(
    "======================================================================",
    "  esp32_Navigation 极速免编译专属安装包生成包 (小白专用)",
    "======================================================================",
    "",
    "【使用步骤】：",
    "1. 双击运行当前目录下的【一键生成专属安装包.bat】；",
    "2. 稍等 2~3 秒，当前目录下会自动生成：",
    "   - esp32_Navigation-personal.apk (你的专属手机安装包)",
    "   - 高德Key申请信息.txt (内含包名与你的专属 SHA1 安全码)",
    "3. 按照文本里的说明去高德开放平台免费申请 Key (1分钟搞定)；",
    "4. 将 APK 发送到手机安装，把申请到的 Key 填入 App 设置页即可！",
    "",
    "* 本工具全程本地运行，零配置环境，无需安装 Android Studio 或编译源码。",
    "* 后续软件升级新版本时，直接下载新版底包覆盖并双击运行，签名自动复用，无需重新申请 Key！",
    "======================================================================"
)
$readmeText = $lines -join "`r`n"
[System.IO.File]::WriteAllText((Join-Path $stageDir "使用说明.txt"), $readmeText, [System.Text.Encoding]::UTF8)

# 3. 压缩为 Zip
$zipPath = Join-Path $outputDir "esp32_Navigation-QuickPack-Windows.zip"
if (Test-Path $zipPath) { Remove-Item -LiteralPath $zipPath -Force -ErrorAction SilentlyContinue }

Write-Host "-> 正在打包为: $zipPath ..." -ForegroundColor Yellow
Compress-Archive -Path "$stageDir\*" -DestinationPath $zipPath -Force

# 清理临时暂存区
Remove-Item -LiteralPath $stageDir -Recurse -Force -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "======================================================================" -ForegroundColor Green
Write-Host "  打包完成！可直接用于 GitHub Release 发布资产：" -ForegroundColor Green
Write-Host "  $zipPath" -ForegroundColor Cyan
Write-Host "======================================================================" -ForegroundColor Green
