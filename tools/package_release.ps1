param(
    [switch]$SkipVerification
)

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

function Copy-RequiredFile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Source,
        [Parameter(Mandatory = $true)]
        [string]$Destination
    )

    if (-not (Test-Path -LiteralPath $Source -PathType Leaf)) {
        throw "Required file is missing: $Source"
    }
    $destinationDir = Split-Path -Parent $Destination
    if (-not (Test-Path -LiteralPath $destinationDir)) {
        New-Item -ItemType Directory -Path $destinationDir | Out-Null
    }
    Copy-Item -LiteralPath $Source -Destination $Destination -Force
}

function Copy-DirectoryFiles {
    param(
        [Parameter(Mandatory = $true)]
        [string]$SourceDir,
        [Parameter(Mandatory = $true)]
        [string]$DestinationDir,
        [string[]]$Include = @("*")
    )

    foreach ($name in $Include) {
        Copy-RequiredFile (Join-Path $SourceDir $name) (Join-Path $DestinationDir $name)
    }
}

Push-Location "$PSScriptRoot\.."
try {
    $repoRoot = (Get-Location).Path
    $distRoot = Join-Path $repoRoot "dist"
    $packageRoot = Join-Path $distRoot "msp_env_delivery"

    $resolvedRepo = [System.IO.Path]::GetFullPath($repoRoot)
    $resolvedPackage = [System.IO.Path]::GetFullPath($packageRoot)
    if (-not $resolvedPackage.StartsWith($resolvedRepo, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to write a package outside the repository: $resolvedPackage"
    }

    if (-not $SkipVerification) {
        Invoke-Checked { powershell -ExecutionPolicy Bypass -File tools\run_verification.ps1 }
        Invoke-Checked { powershell -ExecutionPolicy Bypass -File tools\verify_android_apk.ps1 }
    } else {
        Invoke-Checked { & "D:\ccs\ccs\utils\bin\gmake.exe" -C Debug all }
        Invoke-Checked { powershell -ExecutionPolicy Bypass -File tools\build_android_apk.ps1 }
        Invoke-Checked { python tools\generate_diagrams.py }
        Invoke-Checked { python tools\generate_course_report.py }
        Invoke-Checked { python tools\generate_effect_report.py }
    }

    if (Test-Path -LiteralPath $packageRoot) {
        Remove-Item -LiteralPath $packageRoot -Recurse -Force
    }
    New-Item -ItemType Directory -Path $packageRoot | Out-Null

    Copy-RequiredFile "Debug\mspbluetooth.out" (Join-Path $packageRoot "firmware\mspbluetooth.out")
    Copy-RequiredFile "android\app\build\outputs\apk\debug\app-debug.apk" (Join-Path $packageRoot "android\msp430-env-debug.apk")
    Copy-RequiredFile "README.md" (Join-Path $packageRoot "README.md")
    Copy-RequiredFile "main.c" (Join-Path $packageRoot "source\main.c")
    Copy-RequiredFile "Debug\makefile" (Join-Path $packageRoot "source\Debug\makefile")
    Copy-RequiredFile "lnk_msp430f5529.cmd" (Join-Path $packageRoot "source\lnk_msp430f5529.cmd")

    Copy-DirectoryFiles "application" (Join-Path $packageRoot "source\application") @("env_monitor.c", "env_monitor.h")
    Copy-DirectoryFiles "drivers" (Join-Path $packageRoot "source\drivers") @(
        "alarm_buzzer.c", "alarm_buzzer.h",
        "bluetooth_uart.c", "bluetooth_uart.h",
        "board.c", "board.h",
        "board_pins.h",
        "encoder.c", "encoder.h",
        "env_sensors.c", "env_sensors.h",
        "flash_log.c", "flash_log.h",
        "oled_ssd1306.c", "oled_ssd1306.h",
        "platform_config.h"
    )

    foreach ($doc in @(
        "acceptance_matrix.md",
        "bluetooth_protocol_report.md",
        "board_scenario_report.md",
        "course_report_draft.md",
        "effect_acceptance_report.md",
        "encoder_threshold_report.md",
        "flash_log_report.md",
        "hardware_block_diagram.svg",
        "multipoint_temperature_system_design.md",
        "multipoint_temperature_test_plan.csv",
        "readability_report.md",
        "software_flowchart.svg",
        "test_record.csv"
    )) {
        Copy-RequiredFile (Join-Path "docs" $doc) (Join-Path $packageRoot ("docs\" + $doc))
    }

    foreach ($tool in @(
        "run_verification.ps1",
        "verify_android_apk.ps1",
        "build_android_apk.ps1",
        "firmware_static_check.py",
        "readability_audit.py",
        "bluetooth_protocol_sim.py",
        "board_scenario_sim.py",
        "flash_log_sim.py",
        "encoder_threshold_sim.py",
        "generate_diagrams.py",
        "generate_course_report.py",
        "generate_effect_report.py"
    )) {
        Copy-RequiredFile (Join-Path "tools" $tool) (Join-Path $packageRoot ("tools\" + $tool))
    }

    Copy-RequiredFile "android\README.md" (Join-Path $packageRoot "android\README.md")
    Copy-RequiredFile "android\app\src\main\AndroidManifest.xml" (Join-Path $packageRoot "android\AndroidManifest.xml")
    Copy-RequiredFile "android\app\src\main\java\com\dkjsiogu\mspbluetooth\MainActivity.java" (Join-Path $packageRoot "android\MainActivity.java")

    $manifestPath = Join-Path $packageRoot "MANIFEST.txt"
    $commitId = (git rev-parse --short HEAD).Trim()
    $packageTime = Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"
    @(
        "MSP430F5529 environment monitor delivery package",
        "commit=$commitId",
        "generated=$packageTime",
        "",
        "firmware\mspbluetooth.out",
        "android\msp430-env-debug.apk",
        "source\main.c",
        "source\application\env_monitor.c",
        "source\drivers\env_sensors.c",
        "source\drivers\flash_log.c",
        "docs\course_report_draft.md",
        "docs\effect_acceptance_report.md",
        "docs\hardware_block_diagram.svg",
        "docs\software_flowchart.svg",
        "docs\test_record.csv",
        "",
        "Note: software build, APK build, and host-side simulations are verified.",
        "Real flashing, sensor calibration, HC-05 pairing, OLED display, buzzer output, and Flash write behavior still require board testing."
    ) | Set-Content -LiteralPath $manifestPath -Encoding UTF8

    $hashPath = Join-Path $packageRoot "SHA256SUMS.txt"
    Get-ChildItem -LiteralPath $packageRoot -Recurse -File |
        Where-Object { $_.FullName -ne $hashPath } |
        Sort-Object FullName |
        ForEach-Object {
            $relative = $_.FullName.Substring($packageRoot.Length + 1)
            $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant()
            "$hash  $relative"
        } | Set-Content -LiteralPath $hashPath -Encoding ASCII

    Write-Host "Delivery package created:"
    Write-Host $packageRoot
} finally {
    Pop-Location
}
