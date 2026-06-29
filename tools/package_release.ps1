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
        Invoke-Checked { python tools\generate_diagrams.py }
        Invoke-Checked { python tools\generate_course_report.py }
        Invoke-Checked { python tools\prepare_sdcard_assets.py --seconds 0.25 }
        Invoke-Checked { python tools\wav_asset_check.py --report dist\verification\wav_asset_report.md }
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
    Copy-RequiredFile "docs\report_outline.md" (Join-Path $packageRoot "docs\report_outline.md")
    Copy-RequiredFile "docs\course_report_draft.md" (Join-Path $packageRoot "docs\course_report_draft.md")
    Copy-RequiredFile "docs\test_record.csv" (Join-Path $packageRoot "docs\test_record.csv")
    Copy-RequiredFile "docs\hardware_block_diagram.svg" (Join-Path $packageRoot "docs\hardware_block_diagram.svg")
    Copy-RequiredFile "docs\software_flowchart.svg" (Join-Path $packageRoot "docs\software_flowchart.svg")
    Copy-RequiredFile "sdcard\README.md" (Join-Path $packageRoot "sdcard\README.md")
    Invoke-Checked { python tools\epaper_preview_sim.py --output (Join-Path $packageRoot "docs\epaper_preview.pgm") }

    Get-ChildItem -LiteralPath "sdcard" -Filter "TRACK*.WAV" | ForEach-Object {
        Copy-RequiredFile $_.FullName (Join-Path $packageRoot ("sdcard\" + $_.Name))
    }
    Invoke-Checked { python tools\wav_asset_check.py --input (Join-Path $packageRoot "sdcard") --report (Join-Path $packageRoot "docs\wav_asset_report.md") }

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
        "docs\report_outline.md",
        "docs\course_report_draft.md",
        "docs\test_record.csv",
        "docs\hardware_block_diagram.svg",
        "docs\software_flowchart.svg",
        "docs\epaper_preview.pgm",
        "docs\wav_asset_report.md",
        "",
        "Note: software build and simulations are verified by this package script.",
        "Physical flashing and board-level audio tests still need the real MSP430F5529 hardware."
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
