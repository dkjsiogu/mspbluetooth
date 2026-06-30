param(
    [string]$SerialLog,
    [string]$I2sCsv,
    [string]$OutputDir = "dist\real_board_acceptance",
    [switch]$UseGeneratedSamples
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

function Resolve-RequiredFile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [Parameter(Mandatory = $true)]
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        throw "$Label is required unless -UseGeneratedSamples is set"
    }

    $resolved = Resolve-Path -LiteralPath $Path -ErrorAction Stop
    if (-not (Test-Path -LiteralPath $resolved -PathType Leaf)) {
        throw "$Label is not a file: $Path"
    }
    return $resolved.Path
}

Push-Location "$PSScriptRoot\.."
try {
    $repoRoot = (Get-Location).Path
    $resolvedOutput = [System.IO.Path]::GetFullPath((Join-Path $repoRoot $OutputDir))
    $resolvedRepo = [System.IO.Path]::GetFullPath($repoRoot)
    if (-not $resolvedOutput.StartsWith($resolvedRepo, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to write outside the repository: $resolvedOutput"
    }

    if (Test-Path -LiteralPath $resolvedOutput) {
        Remove-Item -LiteralPath $resolvedOutput -Recurse -Force
    }
    New-Item -ItemType Directory -Path $resolvedOutput | Out-Null

    $reportPath = Join-Path $resolvedOutput "hardware_evidence_report.md"
    $summaryPath = Join-Path $resolvedOutput "REAL_BOARD_SUMMARY.txt"
    $commitId = (git rev-parse --short HEAD).Trim()
    $mode = "real"

    if ($UseGeneratedSamples) {
        $mode = "sample"
        $sampleSerial = Join-Path $resolvedOutput "sample_serial_log.txt"
        $sampleI2s = Join-Path $resolvedOutput "sample_i2s_capture.csv"
        Invoke-Checked {
            python tools\hardware_evidence_check.py `
                --serial-sample-out $sampleSerial `
                --i2s-sample-out $sampleI2s `
                --report $reportPath
        }
    } else {
        $resolvedSerial = Resolve-RequiredFile -Path $SerialLog -Label "SerialLog"
        $resolvedI2s = Resolve-RequiredFile -Path $I2sCsv -Label "I2sCsv"
        Copy-Item -LiteralPath $resolvedSerial -Destination (Join-Path $resolvedOutput "real_serial_log.txt") -Force
        Copy-Item -LiteralPath $resolvedI2s -Destination (Join-Path $resolvedOutput "real_i2s_capture.csv") -Force
        Invoke-Checked {
            python tools\hardware_evidence_check.py `
                --serial-log $resolvedSerial `
                --i2s-csv $resolvedI2s `
                --report $reportPath
        }
    }

    @(
        "MSP430F5529 Bluetooth WAV player board acceptance",
        "commit=$commitId",
        "mode=$mode",
        "generated=$(Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz")",
        "",
        "report=hardware_evidence_report.md",
        "firmware=Debug\mspbluetooth.out",
        "apk=android\app\build\outputs\apk\debug\app-debug.apk",
        "",
        "Interpretation:",
        "- mode=real means SerialLog and I2sCsv were supplied by a physical board test.",
        "- mode=sample proves this acceptance workflow only; it is not physical completion evidence.",
        "- Speaker/headphone listening, EC11 rotation, local buttons, and wiring photos still belong in docs\test_record.csv or the course report."
    ) | Set-Content -LiteralPath $summaryPath -Encoding UTF8

    $hashPath = Join-Path $resolvedOutput "SHA256SUMS.txt"
    Get-ChildItem -LiteralPath $resolvedOutput -Recurse -File |
        Where-Object { $_.FullName -ne $hashPath } |
        Sort-Object FullName |
        ForEach-Object {
            $relative = $_.FullName.Substring($resolvedOutput.Length + 1)
            $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant()
            "$hash  $relative"
        } | Set-Content -LiteralPath $hashPath -Encoding ASCII

    Write-Host "Board acceptance evidence package created:"
    Write-Host $resolvedOutput
} finally {
    Pop-Location
}
