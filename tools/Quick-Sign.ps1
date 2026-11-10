# ====================================================================
# esp32_Navigation - 极速专属安装包生成器 (免编译、秒级签名、专属包名)
# Quick Personal Signer for esp32_Navigation (No build tools required)
# ====================================================================

param(
    [string]$Language = "zh"
)

$ErrorActionPreference = 'Continue'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$rootDir = (Split-Path -Parent $scriptDir)

# 确保如果在根目录下运行也能正确识别
if (Test-Path (Join-Path $scriptDir "apksigner.jar")) {
    $toolsDir = $scriptDir
    $workDir = (Split-Path -Parent $scriptDir)
} else {
    $toolsDir = Join-Path $rootDir "tools"
    $workDir = $rootDir
}

function T($zh, $en) {
    if ($Language -eq "zh") { return $zh } else { return $en }
}

Write-Host "======================================================================" -ForegroundColor Cyan
Write-Host ("  " + (T "esp32_Navigation 极速免编译专属签名工具 (1秒出包)" "esp32_Navigation Quick Personal Signer (Instant Packaging)")) -ForegroundColor Cyan
Write-Host "======================================================================" -ForegroundColor Cyan
Write-Host ""

# 1. 查找底包模板 template.apk
$templateCandidates = @(
    (Join-Path $workDir "esp32_Navigation-template.apk"),
    (Join-Path $workDir "output\esp32_Navigation-personal.apk"),
    (Join-Path $workDir "android\standard\app\build\outputs\apk\release\app-release-unsigned.apk"),
    (Join-Path $workDir "esp32_Navigation.apk")
)

$templateApk = $null
foreach ($cand in $templateCandidates) {
    if (Test-Path $cand) {
        $templateApk = $cand
        break
    }
}

if (-not $templateApk) {
    Write-Host (T "【错误】未找到基础底包 esp32_Navigation-template.apk！" "[Error] esp32_Navigation-template.apk not found!") -ForegroundColor Red
    Write-Host (T "请将 GitHub Release 中下载的 esp32_Navigation-template.apk 放在当前目录下。" "Please place esp32_Navigation-template.apk from GitHub Release in current directory.") -ForegroundColor Yellow
    Write-Host ""
    Read-Host (T "按回车键退出..." "Press Enter to exit...")
    exit 1
}

Write-Host (T "-> 找到基础底包: " "-> Found template APK: ") -NoNewline
Write-Host (Split-Path -Leaf $templateApk) -ForegroundColor Green

# 2. 检测 Java 运行环境
Write-Host (T "-> 检测 Java 环境..." "-> Detecting Java environment...") -ForegroundColor Yellow
$javaExe = $null
$keytoolExe = $null

# 优先探测系统 PATH
$cmdJava = Get-Command java.exe -ErrorAction SilentlyContinue
if ($cmdJava) {
    $bin = Split-Path $cmdJava.Source
    if (Test-Path (Join-Path $bin "keytool.exe")) {
        $javaExe = $cmdJava.Source
        $keytoolExe = Join-Path $bin "keytool.exe"
    }
}

# 探测 JAVA_HOME
if (-not $javaExe -and $env:JAVA_HOME) {
    $bin = Join-Path $env:JAVA_HOME "bin"
    if (Test-Path (Join-Path $bin "java.exe")) {
        $javaExe = Join-Path $bin "java.exe"
        $keytoolExe = Join-Path $bin "keytool.exe"
    }
}

# 探测常见安装路径 (Android Studio JBR, Eclipse Adoptium, Oracle JDK)
if (-not $javaExe) {
    $probePaths = @(
        "$env:ProgramFiles\Android\Android Studio\jbr\bin",
        "$env:LOCALAPPDATA\Programs\Android Studio\jbr\bin",
        "$env:ProgramFiles\Java\jdk*\bin",
        "$env:ProgramFiles\Eclipse Adoptium\jdk*\bin",
        "$workDir\tools\jdk\*\bin"
    )
    foreach ($p in $probePaths) {
        $matched = Get-ChildItem -Path $p -ErrorAction SilentlyContinue | Where-Object { Test-Path (Join-Path $_.FullName "keytool.exe") } | Select-Object -First 1
        if ($matched) {
            $javaExe = Join-Path $matched.FullName "java.exe"
            $keytoolExe = Join-Path $matched.FullName "keytool.exe"
            break
        }
    }
}

# 如果没有检测到 Java，从微软官方 Azure CDN 极速下载微型免安装 OpenJDK JRE
if (-not $javaExe -or -not $keytoolExe) {
    Write-Host (T "未检测到本地 Java，正在从微软官方 CDN 下载超轻量免配置 Java 运行库 (仅需一次)..." "Java not found. Downloading lightweight portable Java from Microsoft CDN (one-time only)...") -ForegroundColor Yellow
    $jreZip = Join-Path $toolsDir "jre-portable.zip"
    $jreDest = Join-Path $toolsDir "jre"
    New-Item -ItemType Directory -Force $toolsDir | Out-Null
    
    $downloadUrl = "https://aka.ms/download-jdk/microsoft-jdk-21-windows-x64.zip"
    $webClient = New-Object System.Net.WebClient
    $webClient.Headers.Add("User-Agent", "Mozilla/5.0")
    try {
        $webClient.DownloadFile($downloadUrl, $jreZip)
        Write-Host (T "下载完成，正在解压..." "Downloaded. Extracting...") -ForegroundColor Yellow
        Expand-Archive -LiteralPath $jreZip -DestinationPath $jreDest -Force
        Remove-Item -LiteralPath $jreZip -Force -ErrorAction SilentlyContinue
    } catch {
        Write-Host (T "下载 Java 失败，请确保网络正常或手动安装 Java 17/21。" "Failed to download Java. Please install Java 17/21 manually.") -ForegroundColor Red
        exit 1
    } finally {
        $webClient.Dispose()
    }
    
    $foundJava = Get-ChildItem -Path "$jreDest\*\bin\java.exe" | Select-Object -First 1
    if ($foundJava) {
        $javaExe = $foundJava.FullName
        $keytoolExe = Join-Path (Split-Path $javaExe) "keytool.exe"
    }
}

Write-Host "-> Java: $javaExe" -ForegroundColor DarkGray

# 3. 管理用户个人专属独立包名 (Package Name)
$keystoreDir = Join-Path $workDir "keystore"
New-Item -ItemType Directory -Force $keystoreDir | Out-Null
$identityFile = Join-Path $keystoreDir "personal.properties"

$appId = $null
if (Test-Path $identityFile) {
    Get-Content $identityFile | ForEach-Object {
        $line = $_.Trim()
        if ($line -match '^personalApplicationId=(.+)$') { $appId = $matches[1] }
    }
}

if (-not $appId -or $appId -notmatch '^org\.esp32nav\.u[a-f0-9]{32}$') {
    # 全新专属包名：完全脱离 motonav 品牌叙事，生成唯一 32 位 GUID
    $appId = "org.esp32nav.u" + [Guid]::NewGuid().ToString('N')
    [System.IO.File]::WriteAllText($identityFile, "personalApplicationId=$appId`n", [System.Text.Encoding]::UTF8)
    Write-Host (T "-> 正在为你生成专属独立包名: " "-> Generated personal unique package name: ") -NoNewline
    Write-Host $appId -ForegroundColor Yellow
} else {
    Write-Host (T "-> 复用已有专属包名: " "-> Reusing personal package name: ") -NoNewline
    Write-Host $appId -ForegroundColor Green
}

# 4. 管理用户个人专属数字证书 (Keystore)
$keyfile = Join-Path $keystoreDir "personal.jks"
$keyProps = Join-Path $keystoreDir "keystore.properties"

$alias = "esp32nav"
$password = $null

if (Test-Path $keyProps) {
    Get-Content $keyProps | ForEach-Object {
        $line = $_.Trim()
        if ($line -match '^storePassword=(.+)$') { $password = $matches[1] }
        if ($line -match '^keyAlias=(.+)$') { $alias = $matches[1] }
    }
}

if (-not $password -or -not (Test-Path $keyfile)) {
    Write-Host (T "-> 正在为你生成唯一的专属数字签名证书..." "-> Generating your unique personal digital certificate...") -ForegroundColor Yellow
    $password = [Guid]::NewGuid().ToString('N')
    if (Test-Path $keyfile) { Remove-Item -LiteralPath $keyfile -Force -ErrorAction SilentlyContinue }
    
    & $keytoolExe -genkeypair -storetype JKS -keystore $keyfile `
        -storepass $password -keypass $password -alias $alias `
        -keyalg RSA -keysize 2048 -validity 10000 `
        -dname "CN=esp32_Navigation Personal User,O=esp32_Navigation,C=CN" 2>$null
        
    if (-not (Test-Path $keyfile)) {
        Write-Host (T "【错误】数字证书生成失败！" "[Error] Failed to generate digital certificate!") -ForegroundColor Red
        exit 1
    }
    
    [System.IO.File]::WriteAllText($keyProps, "storeFile=personal.jks`nstorePassword=$password`nkeyAlias=$alias`nkeyPassword=$password`n", [System.Text.Encoding]::UTF8)
} else {
    Write-Host (T "-> 复用已有个人专属证书 (升级版本无需重新申请高德Key)" "-> Reusing existing personal certificate (no need to re-register Map key)") -ForegroundColor Green
}

# 5. 计算并提取证书 SHA1 指纹
$certDer = Join-Path $keystoreDir "certificate.der"
if (Test-Path $certDer) { Remove-Item -LiteralPath $certDer -Force -ErrorAction SilentlyContinue }
& $keytoolExe -exportcert -keystore $keyfile -storepass $password -alias $alias -file $certDer 2>$null
if (-not (Test-Path $certDer)) {
    Write-Host (T "【错误】导出证书公钥失败！" "[Error] Failed to export certificate!") -ForegroundColor Red
    exit 1
}
$sha1 = (Get-FileHash -LiteralPath $certDer -Algorithm SHA1).Hash -replace '(..)(?!$)', '$1:'
Remove-Item -LiteralPath $certDer -Force -ErrorAction SilentlyContinue

# 6. 动态就地修改 APK 内部包名为专属包名 (0.05秒完成，无需编译)
Write-Host (T "-> 正在注入个人专属包名到安装包..." "-> Injecting personal package name into APK...") -ForegroundColor Yellow

Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$outputApk = Join-Path $workDir "esp32_Navigation-personal.apk"
if (Test-Path $outputApk) { Remove-Item -LiteralPath $outputApk -Force -ErrorAction SilentlyContinue }
Copy-Item -LiteralPath $templateApk -Destination $outputApk -Force

# 打开 APK 修改 AndroidManifest.xml
$zip = [System.IO.Compression.ZipFile]::Open($outputApk, [System.IO.Compression.ZipArchiveMode]::Update)
$manifestEntry = $zip.GetEntry("AndroidManifest.xml")

if ($manifestEntry) {
    $ms = New-Object System.IO.MemoryStream
    $s = $manifestEntry.Open()
    $s.CopyTo($ms)
    $s.Dispose()
    $bytes = $ms.ToArray()
    
    # 查找 UTF-16LE 编码的包名模式 (org.esp32nav.u... 或 com.motonav.u...)
    $latin1 = [System.Text.Encoding]::GetEncoding("iso-8859-1")
    $pattern = [System.Text.RegularExpressions.Regex]"o\x00r\x00g\x00\.\x00e\x00s\x00p\x003\x002\x00n\x00a\x00v\x00\.\x00u\x00[a-f0-9\x00]{64}|c\x00o\x00m\x00\.\x00m\x00o\x00t\x00o\x00n\x00a\x00v\x00\.\x00u\x00[a-f0-9\x00]{64}"
    $match = $pattern.Match($latin1.GetString($bytes))
    
    if ($match.Success) {
        $idx = $match.Index
        $newBytes = [System.Text.Encoding]::Unicode.GetBytes($appId)
        [System.Buffer]::BlockCopy($newBytes, 0, $bytes, $idx, $newBytes.Length)
        $manifestEntry.Delete()
        $newEntry = $zip.CreateEntry("AndroidManifest.xml", [System.IO.Compression.CompressionLevel]::Optimal)
        $ws = $newEntry.Open()
        $ws.Write($bytes, 0, $bytes.Length)
        $ws.Dispose()
    }
}
$zip.Dispose()

# 7. 调用 apksigner 极速重签名 (仅需 1~2 秒)
Write-Host (T "-> 正在使用专属证书签名 APK (1~2秒完成)..." "-> Signing APK with your unique certificate...") -ForegroundColor Yellow

$apksignerJar = Join-Path $toolsDir "apksigner.jar"
if (-not (Test-Path $apksignerJar)) {
    Write-Host (T "【错误】缺少 tools/apksigner.jar 签名工具！" "[Error] tools/apksigner.jar missing!") -ForegroundColor Red
    exit 1
}

$tempSigned = Join-Path $workDir "esp32_Navigation-personal-signed.apk"
if (Test-Path $tempSigned) { Remove-Item -LiteralPath $tempSigned -Force -ErrorAction SilentlyContinue }

& $javaExe -jar $apksignerJar sign `
    --ks $keyfile `
    --ks-pass "pass:$password" `
    --key-pass "pass:$password" `
    --ks-key-alias $alias `
    --out $tempSigned `
    $outputApk 2>$null

if (Test-Path $tempSigned) {
    Move-Item -LiteralPath $tempSigned -Destination $outputApk -Force
} else {
    Write-Host (T "【错误】APK 签名失败！" "[Error] APK signing failed!") -ForegroundColor Red
    exit 1
}

# 8. 生成高德 Key 申请指引文件
$infoPath = Join-Path $workDir "高德Key申请信息.txt"
$infoContent = @"
======================================================================
  esp32_Navigation 高德开放平台 API Key 申请专属信息
======================================================================

【应用包名】:
$appId

【专属 SHA1 安全码】:
$sha1

----------------------------------------------------------------------
申请 3 步指南 (完全免费，仅需 1 分钟):
1. 浏览器打开高德开放平台并登录: https://console.amap.com/dev/key/app
2. 点击右上角【创建新应用】(名称任意填写，如：随行导航)
3. 在新创建的应用下点击【添加 Key】:
   - Key 名称: 任意填写 (如: 手机端)
   - 服务平台: 务必勾选【Android 平台】
   - 发布版安全码 SHA1: 粘贴上方专属 SHA1
   - PackageName: 粘贴上方应用包名 ($appId)
4. 点击提交后，复制生成的【Key】(一串英文字母和数字)。
5. 打开手机上的 esp32_Navigation App，进入设置页将 Key 粘贴保存即可！
======================================================================
"@
$utf8BOM = New-Object System.Text.UTF8Encoding($true)
[System.IO.File]::WriteAllText($infoPath, $infoContent, $utf8BOM)

# 9. 打印结果
Write-Host ""
Write-Host "======================================================================" -ForegroundColor Green
Write-Host (T "  恭喜！专属安装包生成成功！(耗时仅需数秒，已就绪)" "  Congratulations! Personalized APK is ready!") -ForegroundColor Green
Write-Host "======================================================================" -ForegroundColor Green
Write-Host ""
Write-Host (T "【生成安装包】: " "[Output APK  ]: ") -NoNewline
Write-Host $outputApk -ForegroundColor Cyan
Write-Host (T "【专属包名  】: " "[PackageName ]: ") -NoNewline
Write-Host $appId -ForegroundColor Yellow
Write-Host (T "【证书 SHA1 】: " "[Cert SHA1   ]: ") -NoNewline
Write-Host $sha1 -ForegroundColor Yellow
Write-Host ""
Write-Host (T "说明信息已自动保存至: 高德Key申请信息.txt" "Registration details saved to: 高德Key申请信息.txt") -ForegroundColor Gray
Write-Host "----------------------------------------------------------------------" -ForegroundColor DarkGray
Write-Host (T "下一步：" "Next steps:") -ForegroundColor White
Write-Host (T "1. 将 esp32_Navigation-personal.apk 发送到手机安装。" "1. Send esp32_Navigation-personal.apk to your phone and install.") -ForegroundColor White
Write-Host (T "2. 复制上方包名与 SHA1，去高德开放平台申请 Key 并填入 App 即可！" "2. Copy PackageName & SHA1 to AMap console to get Key.") -ForegroundColor White
Write-Host "======================================================================" -ForegroundColor Green
Write-Host ""
