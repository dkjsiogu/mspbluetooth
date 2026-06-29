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
    Invoke-Checked { python tools\board_scenario_sim.py }
    Invoke-Checked { python tools\local_button_sim.py }
    Invoke-Checked { python tools\epaper_preview_sim.py }
    Invoke-Checked { python tools\generate_diagrams.py }
    Invoke-Checked { python tools\generate_course_report.py }
    Invoke-Checked { python tools\prepare_sdcard_assets.py --seconds 0.25 }
    Invoke-Checked { python tools\wav_asset_check.py --report dist\verification\wav_asset_report.md }
} finally {
    Pop-Location
}
