# Clang-Tidy Build Script for Resonance Audio
# Run with: .\run-clang-tidy.ps1

param(
    [string[]]$Directories = @(
        "resonance_audio",
        "platforms/vst"
    ),
    [string]$BuildDir = ".build/clang-tidy",
    [string[]]$BuildArgs = @(
        "-DBUILD_RESONANCE_AUDIO_TESTS=ON", 
        "-DBUILD_SHARED_LIBS=OFF", 
        "-DBUILD_VST_MONITOR_PLUGIN=ON"
    ),
    [switch]$Fix,
    [switch]$RecreateBuild,
    [int]$ThrottleTasks = 8
)

$ProgressPreference = 'SilentlyContinue'
$ErrorActionPreference = "Stop"

Write-Host "=================================" -ForegroundColor Cyan
Write-Host "Resonance Audio Clang-Tidy Runner" -ForegroundColor Cyan
Write-Host "=================================" -ForegroundColor Cyan

# Step 1: Prepare build folder
Write-Host ""
Write-Host "[1/3] Preparing build folder..." -ForegroundColor Yellow

if ((Test-Path $BuildDir) -and $RecreateBuild) {
    $null = Remove-Item $BuildDir -Recurse -Force
    Write-Host ""
}
if (-not (Test-Path $BuildDir)) {
    Write-Host "  Running CMake with Ninja..."
    Enter-DevShell | Out-Null

    $BuildArgs += @("-DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
    cmake -B $BuildDir -S . $BuildArgs -G Ninja

    if (-not (Test-Path "$BuildDir/compile_commands.json")) {
        Write-Host "ERROR: compile_commands.json not found!" -ForegroundColor Red
        exit 1
    }
}

Write-Host ""
Write-Host "Done!" -ForegroundColor Green

# Step 2: Generate Ninja clang-tidy build
Write-Host ""
Write-Host "[2/3] Generating clang-tidy ninja build..." -ForegroundColor Yellow

$compileDB = Join-Path $BuildDir "compile_commands.json"

if (-not (Test-Path $compileDB)) {
    Write-Host "ERROR: compile_commands.json not found!" -ForegroundColor Red
    exit 1
}

$db = Get-Content $compileDB | ConvertFrom-Json

$projectRoot = (Get-Location).Path

$entries = $db | ForEach-Object {
    $abs = [System.IO.Path]::GetFullPath($_.file)
    $rel = [System.IO.Path]::GetRelativePath($projectRoot, $abs)
    $rel.Replace("\", "/")
}
$entries = $entries | Where-Object {
    $file = $_
    $Directories | Where-Object { $file -like "$_/*" }
}

Write-Host "  Found $($entries.Count) compile units"

$ninjaFile = Join-Path $BuildDir "clang-tidy.ninja"

$ninja = @()
if ($Fix) {
    $ninja += "rule clang_tidy"
    $ninja += "  command = clang-tidy `$in -p $BuildDir --fix"
    $ninja += "  description = Tidy `$in"
}
else {
    $ninja += "rule clang_tidy"
    $ninja += "  command = clang-tidy `$in -p $BuildDir"
    $ninja += "  description = Tidy Check `$in"
}
$ninja += ""

foreach ($src in $entries) {
    $src = [System.IO.Path]::GetRelativePath($BuildDir, $src)
    $obj = $src + ".tidy"
    $ninja += "build $($obj): clang_tidy $src"
}

Set-Content $ninjaFile $ninja

Write-Host "  Ninja file written: $ninjaFile"

# Step 3: Ninja ninjutso
Write-Host ""
Write-Host "[3/3] Running ninja..." -ForegroundColor Yellow

Push-Location $BuildDir
try {
    $ninjaFile = [System.IO.Path]::GetRelativePath($BuildDir, $ninjaFile)
    ninja -f $ninjaFile -j $ThrottleTasks
}
finally {
    Pop-Location
}

Write-Host ""
Write-Host "Done!" -ForegroundColor Green
