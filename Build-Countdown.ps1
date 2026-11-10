param([string]$ProviderPath='',[string]$ApktoolPath='',[string]$JavaPath=$env:JAVA_HOME,
      [string]$SdkPath=$env:ANDROID_SDK_ROOT,[string]$GradlePath='',
      [string]$ZigPath='',[string]$PythonPath='python',[switch]$Offline)
$ErrorActionPreference='Stop'
$project=Join-Path $PSScriptRoot 'android/countdown'

Write-Host "======================================================================" -ForegroundColor Magenta
Write-Host "  esp32_Navigation 安卓融合读秒版 (Countdown Edition) 自动化打包" -ForegroundColor Magenta
Write-Host "======================================================================" -ForegroundColor Magenta
Write-Host ""

function Need-Path([string]$Value,[string]$Prompt,[string]$HelpInfo="") {
    if(!$Value){
        if ($HelpInfo) { Write-Host $HelpInfo -ForegroundColor Yellow }
        $Value=Read-Host $Prompt
    }
    $Value=$Value.Trim().Trim('"')
    if(!(Test-Path -LiteralPath $Value)){throw "路径不存在：$Value"}
    return [IO.Path]::GetFullPath($Value)
}

if($ProviderPath){$ProviderPath=Need-Path $ProviderPath '车机底包 APK 路径'}

# 自动检测 Apktool
if(!$ApktoolPath){
    $commonJar = "$env:USERPROFILE\scoop\apps\apktool\current\apktool.jar"
    if(Test-Path $commonJar){$ApktoolPath = $commonJar}
}
$ApktoolPath=Need-Path $ApktoolPath '请输入 apktool.jar 完整文件路径' '【提示】读秒版需反编译车机资源，请先下载 apktool.jar (例如从 https://github.com/iBotPeaches/Apktool/releases 获取)'
if(Test-Path -PathType Container $ApktoolPath){
    $subJar = Join-Path $ApktoolPath 'apktool.jar'
    if(Test-Path $subJar){$ApktoolPath = $subJar}
}

# 自动检测 Java
if(!$JavaPath){
    $cmdJava = Get-Command java -ErrorAction SilentlyContinue
    if($cmdJava){
        $binDir = Split-Path $cmdJava.Source
        if(Test-Path (Join-Path $binDir 'keytool.exe')){
            $JavaPath = Split-Path $binDir
        }
    }
}
$JavaPath=Need-Path $JavaPath '请输入 JDK 根目录 (JAVA_HOME)'

# 自动检测 Android SDK
if(!$SdkPath){
    $commonSdk = "$env:USERPROFILE\AppData\Local\Android\Sdk"
    if(Test-Path $commonSdk){$SdkPath = $commonSdk}
}
if(!$SdkPath){
    $toolsSdk = Join-Path $PSScriptRoot '.tools\android-sdk'
    if(Test-Path $toolsSdk){$SdkPath = $toolsSdk}
}
$SdkPath=Need-Path $SdkPath '请输入 Android SDK 根目录路径'

# 自动检测 Gradle
if(!$GradlePath){$GradlePath=Join-Path $project 'gradlew.bat'}
$GradlePath=Need-Path $GradlePath 'gradlew.bat 路径'

# 自动检测 Zig
if(!$ZigPath){
    $cmd=Get-Command zig -ErrorAction SilentlyContinue
    if($cmd){$ZigPath=$cmd.Source}
    else {
        $commonZig = "$env:USERPROFILE\scoop\shims\zig.exe"
        if(Test-Path $commonZig){$ZigPath = $commonZig}
    }
}
$ZigPath=Need-Path $ZigPath '请输入 Zig 0.13.0 可执行文件路径 (zig.exe)' '【提示】读秒版需 Zig 编译器编译 native 动态库 (https://ziglang.org/download/)'

$flags=@((Join-Path $project 'build-countdown.py'),'--apktool',$ApktoolPath,
    '--java-home',$JavaPath,'--sdk',$SdkPath,'--gradle',$GradlePath,'--zig',$ZigPath)
if($ProviderPath){$flags+=@('--provider',$ProviderPath)}
if($Offline){$flags+='--offline'}

Write-Host "正在启动 build-countdown 构建进程..." -ForegroundColor Cyan
Write-Host "  Apktool: $ApktoolPath" -ForegroundColor DarkGray
Write-Host "  Java:    $JavaPath" -ForegroundColor DarkGray
Write-Host "  SDK:     $SdkPath" -ForegroundColor DarkGray
Write-Host "  Zig:     $ZigPath" -ForegroundColor DarkGray

& $PythonPath @flags
if($LASTEXITCODE -ne 0){throw '构建读秒版失败，请检查上方日志输出！'}

Write-Host "======================================================================" -ForegroundColor Green
Write-Host "  [成功] 读秒版构建完成！" -ForegroundColor Green
Write-Host "======================================================================" -ForegroundColor Green
