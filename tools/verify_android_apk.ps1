$ErrorActionPreference = "Stop"

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)]
        [scriptblock]$Command
    )

    & $Command
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE"
    }
}

$root = Resolve-Path "$PSScriptRoot\.."
$apk = Join-Path $root "android\app\build\outputs\apk\debug\app-debug.apk"
$aapt = Join-Path $root ".tools\android-sdk\build-tools\35.0.0\aapt.exe"
$mainActivity = Join-Path $root "android\app\src\main\java\com\dkjsiogu\mspbluetooth\MainActivity.java"

Invoke-Checked { powershell -ExecutionPolicy Bypass -File (Join-Path $root "tools\build_android_apk.ps1") }

if (!(Test-Path $apk)) {
    throw "APK was not generated: $apk"
}

$badging = & $aapt dump badging $apk
if ($LASTEXITCODE -ne 0) {
    throw "aapt dump badging failed with exit code $LASTEXITCODE"
}

$required = @(
    "package: name='com.dkjsiogu.mspbluetooth'",
    "sdkVersion:'23'",
    "targetSdkVersion:'35'",
    "uses-permission: name='android.permission.BLUETOOTH_CONNECT'",
    "application-label:'MSP430 Player'"
)

foreach ($needle in $required) {
    if (($badging -join "`n").IndexOf($needle, [StringComparison]::Ordinal) -lt 0) {
        throw "APK badging missing: $needle"
    }
}

$source = Get-Content -Raw $mainActivity
$requiredCommands = @('"p"', '"s"', '"r"', '"n"', '"b"', '"+"', '"-"', '"m"', '"o"', '"t"', '"i"', '"e"', '"l"', '"d"', '"?"')
foreach ($command in $requiredCommands) {
    if ($source.IndexOf("sendButton", [StringComparison]::Ordinal) -ge 0 -and
        $source.IndexOf($command, [StringComparison]::Ordinal) -lt 0) {
        throw "Android UI source missing command button for $command"
    }
}

$requiredUiMarkers = @(
    "dashboardView",
    "displayView",
    "parseIncomingLine",
    '"status="',
    '"progress="',
    "Progress:",
    '"display 1:"',
    '"display 2:"',
    '"display 3:"'
)
foreach ($marker in $requiredUiMarkers) {
    if ($source.IndexOf($marker, [StringComparison]::Ordinal) -lt 0) {
        throw "Android UI source missing status/display parser marker: $marker"
    }
}

Write-Output "Android APK verification passed"
Write-Output $apk
