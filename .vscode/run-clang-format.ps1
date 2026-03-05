# Clang-Format Build Script for Resonance Audio
# Run with: .\run-clang-format.ps1

param(
    [string[]]$Directories = @(
        "resonance_audio",
        "platforms/vst"
    ),
    [string]$BuildDir = ".build/clang-format",
    [switch]$Fix,
    [int]$ThrottleTasks = 8
)

$ProgressPreference = 'SilentlyContinue'
$ErrorActionPreference = "Stop"

Write-Host "===================================" -ForegroundColor Cyan
Write-Host "Resonance Audio Clang-Format Runner" -ForegroundColor Cyan
Write-Host "===================================" -ForegroundColor Cyan

# Step 2: Generate Ninja clang-tidy build
Write-Host ""
Write-Host "[1/2] Generating clang-tidy ninja build..." -ForegroundColor Yellow

$projectRoot = (Get-Location).Path
$BuildDir = (Join-Path $projectRoot $BuildDir)

$extensions = @(
    "*.c", "*.cc", "*.cpp", "*.cxx",
    "*.h", "*.hh", "*.hpp", "*.hxx",
    "*.ipp", "*.inl"
)

$entries = foreach ($dir in $Directories) {
    if (-not (Test-Path $dir)) { continue }

    Get-ChildItem $dir -Recurse -File -Include $extensions |
    ForEach-Object {
        $rel = [System.IO.Path]::GetRelativePath($projectRoot, $_.FullName)
        $rel.Replace("\", "/")
    }
}

$entries = $entries | Sort-Object -Unique

Write-Host "  Found $($entries.Count) compile units"

$null = New-Item -Force $BuildDir -ItemType Directory -ErrorAction Ignore
$ninjaFile = Join-Path $BuildDir "clang-format.ninja"

$ninja = @()
if ($Fix) {
    $ninja += "rule clang_format"
    $ninja += "  command = clang-format --style=file -i `$in"
    $ninja += "  description = Format `$in"
}
else {
    $ninja += "rule clang_format"
    $ninja += "  command = clang-format --style=file --dry-run -Werror -i `$in"
    $ninja += "  description = Format Check `$in"
}
$ninja += ""

foreach ($src in $entries) {
    $src = [System.IO.Path]::GetRelativePath($BuildDir, $src)
    $obj = $src + ".formatted"    
    $ninja += "build $($obj): clang_format $src"
}

Set-Content $ninjaFile $ninja

Write-Host "  Ninja file written: $ninjaFile"

# Step 3: Ninja ninjutso
Write-Host ""
Write-Host "[2/2] Running ninja..." -ForegroundColor Yellow

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
