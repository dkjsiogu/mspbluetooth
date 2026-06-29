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

Push-Location "$PSScriptRoot\.."
try {
    Invoke-Checked { & "D:\ccs\ccs\utils\bin\gmake.exe" -C Debug clean }
    Invoke-Checked { & "D:\ccs\ccs\utils\bin\gmake.exe" -C Debug all }
    Invoke-Checked { python tools\firmware_static_check.py }
    Invoke-Checked { python tools\bluetooth_protocol_sim.py }
    Invoke-Checked { python tools\bluetooth_diagnostic_sim.py }
    Invoke-Checked { python tools\serial_acceptance_check.py }
    Invoke-Checked { python tools\android_ui_parser_sim.py }
    Invoke-Checked { python tools\android_command_coverage.py }
    Invoke-Checked { python tools\end_to_end_demo_sim.py }
    Invoke-Checked { python tools\board_scenario_sim.py }
    Invoke-Checked { python tools\encoder_quadrature_sim.py }
    Invoke-Checked { python tools\local_button_sim.py }
    Invoke-Checked { python tools\epaper_preview_sim.py }
    Invoke-Checked { python tools\epaper_driver_trace_sim.py }
    Invoke-Checked { python tools\generate_diagrams.py }
    Invoke-Checked { python tools\generate_course_report.py }
    Invoke-Checked { python tools\prepare_sdcard_assets.py --seconds 0.25 }
    Invoke-Checked { python tools\wav_asset_check.py --report dist\verification\wav_asset_report.md }
    Invoke-Checked { python tools\audio_stream_sim.py }
    Invoke-Checked { python tools\i2s_frame_sim.py }
    Invoke-Checked { python tools\generate_effect_report.py }
} finally {
    Pop-Location
}
