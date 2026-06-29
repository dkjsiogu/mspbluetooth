$ErrorActionPreference = "Stop"

$root = Resolve-Path "$PSScriptRoot\.."
$sdkRoot = Join-Path $root ".tools\android-sdk"
$gradleZip = Join-Path $root ".tools\gradle-8.10.2-bin.zip"
$gradleRoot = Join-Path $root ".tools\gradle"
$gradleBat = Join-Path $gradleRoot "bin\gradle.bat"

if (!(Test-Path $gradleBat)) {
    New-Item -ItemType Directory -Force (Split-Path $gradleZip) | Out-Null
    if (!(Test-Path $gradleZip)) {
        curl.exe -L --retry 3 --retry-delay 5 -o $gradleZip https://services.gradle.org/distributions/gradle-8.10.2-bin.zip
        if ($LASTEXITCODE -ne 0) {
            throw "Gradle download failed with exit code $LASTEXITCODE"
        }
    }
    $extractDir = Join-Path $root ".tools\gradle-extract"
    Remove-Item -Recurse -Force $extractDir -ErrorAction SilentlyContinue
    Expand-Archive -Path $gradleZip -DestinationPath $extractDir -Force
    Remove-Item -Recurse -Force $gradleRoot -ErrorAction SilentlyContinue
    Move-Item -Path (Join-Path $extractDir "gradle-8.10.2") -Destination $gradleRoot
    Remove-Item -Recurse -Force $extractDir -ErrorAction SilentlyContinue
}

$env:ANDROID_HOME = $sdkRoot
$env:ANDROID_SDK_ROOT = $sdkRoot

$localProperties = Join-Path $root "android\local.properties"
"sdk.dir=$($sdkRoot.Replace('\', '\\'))" | Set-Content -Encoding ASCII $localProperties

Push-Location (Join-Path $root "android")
try {
    & $gradleBat --no-daemon assembleDebug
    if ($LASTEXITCODE -ne 0) {
        throw "Gradle build failed with exit code $LASTEXITCODE"
    }
} finally {
    Pop-Location
}
