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

Write-Output "Android APK verification passed"
Write-Output $apk
