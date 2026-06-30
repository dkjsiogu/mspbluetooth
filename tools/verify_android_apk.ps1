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

$badgingText = $badging -join "`n"
$requiredBadging = @(
    "package: name='com.dkjsiogu.mspbluetooth'",
    "sdkVersion:'23'",
    "targetSdkVersion:'35'",
    "uses-permission: name='android.permission.BLUETOOTH_CONNECT'",
    "application-label:'MSP430 Env'"
)

foreach ($needle in $requiredBadging) {
    if ($badgingText.IndexOf($needle, [StringComparison]::Ordinal) -lt 0) {
        throw "APK badging missing: $needle"
    }
}

$source = Get-Content -Raw $mainActivity
$requiredMarkers = @(
    '"MSP430 Env Monitor"',
    '"Data"',
    '"T+"',
    '"T-"',
    '"SETT="',
    '"HIST?"',
    '"HIST 1"',
    '"DUMP"',
    '"CLRLOG"',
    '"Demo RX"',
    '"Save Log"',
    '"DATA "',
    '"TH="',
    '"HIST COUNT="',
    '"REC "',
    '"pin "',
    '"trace "',
    '"info "',
    '"cmd "',
    "REQUEST_SAVE_LOG",
    "Intent.ACTION_CREATE_DOCUMENT",
    "getContentResolver().openOutputStream",
    "DEMO_RX_LINES",
    "parseIncomingLine",
    "updateDataPanel",
    "appendPanelLine"
)

foreach ($marker in $requiredMarkers) {
    if ($source.IndexOf($marker, [StringComparison]::Ordinal) -lt 0) {
        throw "Android source missing environment monitor marker: $marker"
    }
}

Write-Output "Android APK verification passed"
Write-Output $apk
