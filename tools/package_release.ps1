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

Push-Location "$PSScriptRoot\.."
try {
    $repoRoot = (Get-Location).Path
    $distRoot = Join-Path $repoRoot "dist"
    $packageRoot = Join-Path $distRoot "mspbluetooth_delivery"

    $resolvedRepo = [System.IO.Path]::GetFullPath($repoRoot)
    $resolvedDist = [System.IO.Path]::GetFullPath($distRoot)
    if (-not $resolvedDist.StartsWith($resolvedRepo, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean a path outside the repository: $resolvedDist"
    }

    if (-not $SkipVerification) {
        Invoke-Checked { powershell -ExecutionPolicy Bypass -File tools\run_verification.ps1 }
        Invoke-Checked { powershell -ExecutionPolicy Bypass -File tools\verify_android_apk.ps1 }
    } else {
        Invoke-Checked { & "D:\ccs\ccs\utils\bin\gmake.exe" -C Debug all }
        Invoke-Checked { powershell -ExecutionPolicy Bypass -File tools\build_android_apk.ps1 }
        Invoke-Checked { python tools\readability_audit.py }
        Invoke-Checked { python tools\generate_diagrams.py }
        Invoke-Checked { python tools\generate_course_report.py }
        Invoke-Checked { python tools\prepare_sdcard_assets.py --seconds 0.25 }
        Invoke-Checked { python tools\wav_asset_check.py --report dist\verification\wav_asset_report.md }
        Invoke-Checked { python tools\audio_stream_sim.py }
        Invoke-Checked { python tools\i2s_frame_sim.py }
        Invoke-Checked { python tools\i2s_capture_check.py }
        Invoke-Checked { python tools\hardware_evidence_check.py }
        Invoke-Checked { powershell -ExecutionPolicy Bypass -File tools\run_real_board_acceptance.ps1 -UseGeneratedSamples -OutputDir dist\verification\real_board_acceptance }
        Invoke-Checked { python tools\bluetooth_diagnostic_sim.py }
        Invoke-Checked { python tools\serial_acceptance_check.py }
        Invoke-Checked { python tools\epaper_driver_trace_sim.py }
        Invoke-Checked { python tools\android_command_coverage.py }
        Invoke-Checked { python tools\android_acceptance_log_sim.py }
        Invoke-Checked { python tools\android_offline_demo_sim.py }
        Invoke-Checked { python tools\android_hardware_check_sim.py }
        Invoke-Checked { python tools\end_to_end_demo_sim.py }
        Invoke-Checked { python tools\status_led_pattern_sim.py }
        Invoke-Checked { python tools\generate_effect_report.py }
    }

    if (Test-Path -LiteralPath $distRoot) {
        Remove-Item -LiteralPath $distRoot -Recurse -Force
    }

    New-Item -ItemType Directory -Path $packageRoot | Out-Null

    Copy-RequiredFile "Debug\mspbluetooth.out" (Join-Path $packageRoot "firmware\mspbluetooth.out")
    Copy-RequiredFile "android\app\build\outputs\apk\debug\app-debug.apk" (Join-Path $packageRoot "android\mspbluetooth-debug.apk")
    Copy-RequiredFile "README.md" (Join-Path $packageRoot "README.md")
    Copy-RequiredFile "docs\hardware_verification.md" (Join-Path $packageRoot "docs\hardware_verification.md")
    Copy-RequiredFile "docs\acceptance_matrix.md" (Join-Path $packageRoot "docs\acceptance_matrix.md")
    Copy-RequiredFile "docs\real_board_acceptance.md" (Join-Path $packageRoot "docs\real_board_acceptance.md")
    Copy-RequiredFile "docs\report_outline.md" (Join-Path $packageRoot "docs\report_outline.md")
    Copy-RequiredFile "docs\course_report_draft.md" (Join-Path $packageRoot "docs\course_report_draft.md")
    Copy-RequiredFile "docs\readability_report.md" (Join-Path $packageRoot "docs\readability_report.md")
    Copy-RequiredFile "docs\effect_acceptance_report.md" (Join-Path $packageRoot "docs\effect_acceptance_report.md")
    Copy-RequiredFile "docs\bluetooth_diagnostic_report.md" (Join-Path $packageRoot "docs\bluetooth_diagnostic_report.md")
    Copy-RequiredFile "docs\serial_acceptance_report.md" (Join-Path $packageRoot "docs\serial_acceptance_report.md")
    Copy-RequiredFile "docs\android_command_coverage_report.md" (Join-Path $packageRoot "docs\android_command_coverage_report.md")
    Copy-RequiredFile "docs\android_acceptance_script_report.md" (Join-Path $packageRoot "docs\android_acceptance_script_report.md")
    Copy-RequiredFile "docs\android_offline_demo_report.md" (Join-Path $packageRoot "docs\android_offline_demo_report.md")
    Copy-RequiredFile "docs\android_hardware_check_report.md" (Join-Path $packageRoot "docs\android_hardware_check_report.md")
    Copy-RequiredFile "docs\end_to_end_demo_report.md" (Join-Path $packageRoot "docs\end_to_end_demo_report.md")
    Copy-RequiredFile "docs\status_led_report.md" (Join-Path $packageRoot "docs\status_led_report.md")
    Copy-RequiredFile "docs\audio_stream_report.md" (Join-Path $packageRoot "docs\audio_stream_report.md")
    Copy-RequiredFile "docs\i2s_frame_report.md" (Join-Path $packageRoot "docs\i2s_frame_report.md")
    Copy-RequiredFile "docs\i2s_capture_report.md" (Join-Path $packageRoot "docs\i2s_capture_report.md")
    Copy-RequiredFile "docs\hardware_evidence_report.md" (Join-Path $packageRoot "docs\hardware_evidence_report.md")
    Copy-RequiredFile "docs\epaper_gallery_report.md" (Join-Path $packageRoot "docs\epaper_gallery_report.md")
    Copy-RequiredFile "docs\epaper_driver_report.md" (Join-Path $packageRoot "docs\epaper_driver_report.md")
    Copy-RequiredFile "docs\test_record.csv" (Join-Path $packageRoot "docs\test_record.csv")
    Copy-RequiredFile "docs\hardware_block_diagram.svg" (Join-Path $packageRoot "docs\hardware_block_diagram.svg")
    Copy-RequiredFile "docs\software_flowchart.svg" (Join-Path $packageRoot "docs\software_flowchart.svg")
    Copy-RequiredFile "sdcard\README.md" (Join-Path $packageRoot "sdcard\README.md")
    Copy-RequiredFile "drivers\platform_config.h" (Join-Path $packageRoot "drivers\platform_config.h")
    Copy-RequiredFile "tools\run_real_board_acceptance.ps1" (Join-Path $packageRoot "tools\run_real_board_acceptance.ps1")
    Copy-RequiredFile "tools\hardware_evidence_check.py" (Join-Path $packageRoot "tools\hardware_evidence_check.py")
    Copy-RequiredFile "tools\serial_acceptance_check.py" (Join-Path $packageRoot "tools\serial_acceptance_check.py")
    Copy-RequiredFile "tools\i2s_capture_check.py" (Join-Path $packageRoot "tools\i2s_capture_check.py")
    Copy-RequiredFile "tools\i2s_frame_sim.py" (Join-Path $packageRoot "tools\i2s_frame_sim.py")
    Copy-RequiredFile "tools\bluetooth_protocol_sim.py" (Join-Path $packageRoot "tools\bluetooth_protocol_sim.py")
    Invoke-Checked {
        python tools\epaper_preview_sim.py `
            --output (Join-Path $packageRoot "docs\epaper_preview.pgm") `
            --gallery-dir (Join-Path $packageRoot "docs\epaper_gallery") `
            --report (Join-Path $packageRoot "docs\epaper_gallery_report.md")
    }

    Get-ChildItem -LiteralPath "sdcard" -Filter "TRACK*.WAV" | ForEach-Object {
        Copy-RequiredFile $_.FullName (Join-Path $packageRoot ("sdcard\" + $_.Name))
    }
    Invoke-Checked { python tools\wav_asset_check.py --input (Join-Path $packageRoot "sdcard") --report (Join-Path $packageRoot "docs\wav_asset_report.md") }
    Invoke-Checked { python tools\audio_stream_sim.py --input (Join-Path $packageRoot "sdcard") --report (Join-Path $packageRoot "docs\audio_stream_report.md") }
    Invoke-Checked { python tools\i2s_frame_sim.py --report (Join-Path $packageRoot "docs\i2s_frame_report.md") }
    Invoke-Checked {
        python tools\i2s_capture_check.py `
            --sample-out (Join-Path $packageRoot "docs\i2s_capture_sample.csv") `
            --report (Join-Path $packageRoot "docs\i2s_capture_report.md")
    }
    Invoke-Checked { python tools\bluetooth_diagnostic_sim.py --report (Join-Path $packageRoot "docs\bluetooth_diagnostic_report.md") }
    Invoke-Checked {
        python tools\serial_acceptance_check.py `
            --report (Join-Path $packageRoot "docs\serial_acceptance_report.md") `
            --sample-out (Join-Path $packageRoot "docs\serial_acceptance_sample.txt")
    }
    Invoke-Checked { python tools\epaper_driver_trace_sim.py --report (Join-Path $packageRoot "docs\epaper_driver_report.md") }
    Invoke-Checked { python tools\android_command_coverage.py --report (Join-Path $packageRoot "docs\android_command_coverage_report.md") }
    Invoke-Checked {
        python tools\android_acceptance_log_sim.py `
            --log (Join-Path $packageRoot "docs\android_acceptance_log.txt") `
            --report (Join-Path $packageRoot "docs\android_acceptance_script_report.md")
    }
    Invoke-Checked { python tools\android_offline_demo_sim.py --report (Join-Path $packageRoot "docs\android_offline_demo_report.md") }
    Invoke-Checked { python tools\android_hardware_check_sim.py --report (Join-Path $packageRoot "docs\android_hardware_check_report.md") }
    Invoke-Checked { python tools\end_to_end_demo_sim.py --report (Join-Path $packageRoot "docs\end_to_end_demo_report.md") }
    Invoke-Checked { python tools\status_led_pattern_sim.py --report (Join-Path $packageRoot "docs\status_led_report.md") }
    Invoke-Checked { python tools\generate_effect_report.py --input (Join-Path $packageRoot "sdcard") --output (Join-Path $packageRoot "docs\effect_acceptance_report.md") }
    Invoke-Checked { python tools\readability_audit.py --report (Join-Path $packageRoot "docs\readability_report.md") }

    $manifestPath = Join-Path $packageRoot "MANIFEST.txt"
    $commitId = (git rev-parse --short HEAD).Trim()
    $packageTime = Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"
    @(
        "MSP430F5529 Bluetooth WAV player delivery package",
        "commit=$commitId",
        "generated=$packageTime",
        "",
        "firmware\mspbluetooth.out",
        "android\mspbluetooth-debug.apk",
        "sdcard\TRACK01.WAV",
        "sdcard\TRACK02.WAV",
        "sdcard\TRACK03.WAV",
        "docs\hardware_verification.md",
        "docs\acceptance_matrix.md",
        "docs\real_board_acceptance.md",
        "docs\report_outline.md",
        "docs\course_report_draft.md",
        "docs\readability_report.md",
        "docs\effect_acceptance_report.md",
        "docs\bluetooth_diagnostic_report.md",
        "docs\serial_acceptance_report.md",
        "docs\serial_acceptance_sample.txt",
        "docs\android_command_coverage_report.md",
        "docs\android_acceptance_script_report.md",
        "docs\android_acceptance_log.txt",
        "docs\android_offline_demo_report.md",
        "docs\android_hardware_check_report.md",
        "docs\end_to_end_demo_report.md",
        "docs\status_led_report.md",
        "docs\audio_stream_report.md",
        "docs\i2s_frame_report.md",
        "docs\i2s_capture_report.md",
        "docs\i2s_capture_sample.csv",
        "docs\hardware_evidence_report.md",
        "docs\hardware_evidence_serial_sample.txt",
        "docs\hardware_evidence_i2s_sample.csv",
        "docs\epaper_gallery_report.md",
        "docs\epaper_driver_report.md",
        "docs\test_record.csv",
        "docs\hardware_block_diagram.svg",
        "docs\software_flowchart.svg",
        "docs\epaper_preview.pgm",
        "docs\epaper_gallery\epaper_playing.pgm",
        "docs\epaper_gallery\epaper_paused.pgm",
        "docs\epaper_gallery\epaper_stopped.pgm",
        "docs\epaper_gallery\epaper_error.pgm",
        "docs\wav_asset_report.md",
        "drivers\platform_config.h",
        "tools\run_real_board_acceptance.ps1",
        "tools\hardware_evidence_check.py",
        "tools\serial_acceptance_check.py",
        "tools\i2s_capture_check.py",
        "tools\i2s_frame_sim.py",
        "tools\bluetooth_protocol_sim.py",
        "",
        "Note: software build and simulations are verified by this package script.",
        "Physical flashing and board-level audio tests still need the real MSP430F5529 hardware."
    ) | Set-Content -LiteralPath $manifestPath -Encoding UTF8

    Invoke-Checked {
        python tools\hardware_evidence_check.py `
            --serial-sample-out (Join-Path $packageRoot "docs\hardware_evidence_serial_sample.txt") `
            --i2s-sample-out (Join-Path $packageRoot "docs\hardware_evidence_i2s_sample.csv") `
            --report (Join-Path $packageRoot "docs\hardware_evidence_report.md")
    }

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
