<#
.SYNOPSIS
    AbyssCrawler Win64 패키징 스크립트.

.DESCRIPTION
    에디터의 Platforms -> Windows -> Package Project 메뉴 대신 이걸 쓴다.
    에디터 안에서 패키징하면 에디터가 잡고 있는 5GB+ 때문에 쿡이 메모리 부족에 빠진다.

    2026-08-09 실패 이력 참고:
      - 셰이더 컴파일이 IncrediBuild(라이선스 없음)로 넘어가 잡당 평균 323초, 워커가
        0xC0000005로 죽으며 Error_UnknownCookFailure. -> Config/DefaultEngine.ini의
        r.XGEController.Enabled=0 으로 해결. 이 스크립트도 UBT/쿠커에 한 번 더 못을 박는다.
      - build/cook 은 성공했는데 stage 산출물이 없었음. 로그가 덮여서 원인 불명이었으므로
        이 스크립트는 전체 출력을 Saved/PackageLogs 아래에 타임스탬프로 남긴다.

.PARAMETER Config
    Shipping (기본) | Development | Test

.PARAMETER ArchiveDir
    최종 산출물 위치. 기본 D:\AbyssCrawlerBuild (C:는 여유가 적음).

.PARAMETER SkipCook
    직전 쿡 결과를 재사용하고 stage/pak/archive 만 다시 돈다. 콘텐츠를 건드리지 않았을 때만.

.PARAMETER Clean
    반복 쿡을 무시하고 전체 쿡을 강제한다.

.EXAMPLE
    .\Tools\Package-Win64.ps1
    .\Tools\Package-Win64.ps1 -Config Development -ArchiveDir D:\Test
    .\Tools\Package-Win64.ps1 -SkipCook
#>
[CmdletBinding()]
param(
    [ValidateSet('Shipping', 'Development', 'Test')]
    [string]$Config = 'Shipping',

    [string]$ArchiveDir = 'D:\AbyssCrawlerBuild',

    [switch]$SkipCook,

    [switch]$Clean
)

$ErrorActionPreference = 'Stop'

$RepoRoot   = Split-Path -Parent $PSScriptRoot
$UProject   = Join-Path $RepoRoot 'AbyssCrawler\AbyssCrawler.uproject'
$EngineRoot = 'C:\UE_5.7'
$RunUAT     = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$LogDir     = Join-Path $RepoRoot 'AbyssCrawler\Saved\PackageLogs'

# ---------------------------------------------------------------- 사전 점검 --

if (-not (Test-Path $UProject)) { throw "uproject를 찾을 수 없음: $UProject" }
if (-not (Test-Path $RunUAT))   { throw "RunUAT.bat을 찾을 수 없음: $RunUAT (엔진 경로 확인)" }

# 에디터가 떠 있으면 쿡이 메모리 부족으로 스래싱한다. 8/9 실패의 원인 중 하나.
$editor = Get-Process UnrealEditor, UnrealEditor-Win64-DebugGame -ErrorAction SilentlyContinue
if ($editor) {
    throw "언리얼 에디터가 실행 중입니다 (PID: $($editor.Id -join ', ')). 완전히 종료한 뒤 다시 실행하세요."
}

# 산출물은 쿡 2GB + 스테이지 사본까지 넉넉히 잡는다.
$archiveDrive = (Split-Path -Qualifier $ArchiveDir).TrimEnd(':')
$freeGB = [math]::Round((Get-PSDrive $archiveDrive).Free / 1GB, 1)
if ($freeGB -lt 20) {
    throw "$archiveDrive 드라이브 여유 공간이 ${freeGB}GB 뿐입니다. 최소 20GB 확보하거나 -ArchiveDir 를 바꾸세요."
}

New-Item -ItemType Directory -Force -Path $LogDir, $ArchiveDir | Out-Null
$stamp   = Get-Date -Format 'yyyy.MM.dd-HH.mm.ss'
$logFile = Join-Path $LogDir "Package-$Config-$stamp.log"

# ------------------------------------------------------------------- 실행 --

$uatArgs = @(
    'BuildCookRun'
    "-project=`"$UProject`""
    '-noP4'
    '-utf8output'
    '-nocompileeditor'
    '-nodebuginfo'                       # 230MB짜리 pdb를 스테이지에서 제외
    '-platform=Win64'
    "-clientconfig=$Config"
    '-build'
    '-stage'
    '-pak'
    '-iostore'
    '-compressed'
    '-prereqs'
    '-archive'
    "-archivedirectory=`"$ArchiveDir`""
    '-ubtargs="-NoXGE"'                  # C++ 빌드도 IncrediBuild로 넘기지 않는다
)

if ($SkipCook) {
    $uatArgs += '-skipcook'
} else {
    $uatArgs += '-cook'
    # ini의 r.XGEController.Enabled=0 에 더해 쿠커 프로세스에도 직접 지정
    $uatArgs += '-additionalcookeroptions="-noxgecontroller"'
    if ($Clean) { $uatArgs += '-clean' }
}

Write-Host ""
Write-Host "  프로젝트 : $UProject"
Write-Host "  구성     : $Config"
Write-Host "  산출물   : $ArchiveDir  (여유 ${freeGB}GB)"
Write-Host "  로그     : $logFile"
Write-Host ""
Write-Host "  첫 패키징(전체 쿡)은 30분~1시간 걸립니다. 이후 반복 쿡은 훨씬 빠릅니다."
Write-Host ""

$sw = [System.Diagnostics.Stopwatch]::StartNew()
& cmd.exe /c "`"$RunUAT`" $($uatArgs -join ' ') 2>&1" | Tee-Object -FilePath $logFile
$exit = $LASTEXITCODE
$sw.Stop()

# ------------------------------------------------------------------- 결과 --

$elapsed = '{0:hh\:mm\:ss}' -f $sw.Elapsed
Write-Host ""

if ($exit -ne 0) {
    Write-Host "패키징 실패 (ExitCode=$exit, 소요 $elapsed)" -ForegroundColor Red
    Write-Host "로그: $logFile"
    Write-Host ""
    Write-Host "--- 로그에서 찾은 에러 ---"
    Select-String -Path $logFile -Pattern 'Error:|ERROR:|error code|BUILD FAILED|AutomationException' |
        Select-Object -Last 25 |
        ForEach-Object { $_.Line }
    exit $exit
}

$exe = Join-Path $ArchiveDir 'Windows\AbyssCrawler.exe'
Write-Host "패키징 성공 (소요 $elapsed)" -ForegroundColor Green

if (Test-Path $exe) {
    $sizeGB = [math]::Round((Get-ChildItem (Split-Path $exe) -Recurse -File | Measure-Object Length -Sum).Sum / 1GB, 2)
    Write-Host "  실행 파일 : $exe"
    Write-Host "  전체 크기 : ${sizeGB}GB"
} else {
    # 8/9처럼 build/cook만 되고 stage 산출물이 없는 상태를 조용히 넘기지 않는다.
    Write-Host "경고: UAT는 성공했는데 $exe 가 없습니다. stage 단계를 로그에서 확인하세요." -ForegroundColor Yellow
    Get-ChildItem $ArchiveDir -Recurse -Depth 2 -ErrorAction SilentlyContinue |
        Select-Object -First 20 FullName
    exit 1
}

Write-Host "  로그      : $logFile"
