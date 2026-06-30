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
    Invoke-Checked { python tools\readability_audit.py }
    Invoke-Checked { python tools\bluetooth_protocol_sim.py }
    Invoke-Checked { python tools\board_scenario_sim.py }
    Invoke-Checked { python tools\flash_log_sim.py }
    Invoke-Checked { python tools\encoder_threshold_sim.py }
    Invoke-Checked { python tools\generate_diagrams.py }
    Invoke-Checked { python tools\generate_course_report.py }
    Invoke-Checked { python tools\generate_effect_report.py }
} finally {
    Pop-Location
}
