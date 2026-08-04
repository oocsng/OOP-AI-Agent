# OOP AI Agents 2026 - One-Click PowerShell Build & Test Script

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " 🛠️  OOP AI AGENTS 2026 - BUILD & TEST PIPELINE" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

# Step 0: Ensure we are in project root
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
if ($ScriptDir) {
    Set-Location $ScriptDir
}
Write-Host "[1/4] Current Directory: $(Get-Location)" -ForegroundColor Yellow

# Locate standalone Kitware / System CMake BEFORE altering PATH
$SystemCmake = (Get-Command cmake -ErrorAction SilentlyContinue).Source
if ($SystemCmake -and ($SystemCmake -notlike "*msys64*")) {
    $CmakeCmd = $SystemCmake
} elseif (Test-Path "C:\Users\segou\AppData\Local\Programs\Python\Python314\Scripts\cmake.exe") {
    $CmakeCmd = "C:\Users\segou\AppData\Local\Programs\Python\Python314\Scripts\cmake.exe"
} else {
    $CmakeCmd = "cmake"
}

# Step 1: Set PATH for MSYS2 UCRT64 GCC
Write-Host "[2/4] Configuring MSYS2 UCRT64 Compiler Environment PATH..." -ForegroundColor Yellow
$env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH
Write-Host "      GCC Compiler: $(Get-Command g++ | Select-Object -ExpandProperty Source)" -ForegroundColor Green
Write-Host "      CMake Tool:   $CmakeCmd" -ForegroundColor Green

# Step 2: Configure CMake
Write-Host "[3/4] Configuring CMake MinGW Build System..." -ForegroundColor Yellow
& $CmakeCmd -B build -S . -G "MinGW Makefiles"
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ CMake configuration failed!" -ForegroundColor Red
    exit 1
}

# Step 3: Build targets in parallel
Write-Host "[4/4] Compiling all 12 C++ Targets (Parallel 8 Threads)..." -ForegroundColor Yellow
& $CmakeCmd --build build --parallel 8
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Compilation failed!" -ForegroundColor Red
    exit 1
}

Write-Host "`n==========================================================" -ForegroundColor Cyan
Write-Host " 🧪 RUNNING CTEST UNIT TESTING SUITE" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

$CtestCmd = $CmakeCmd -replace "cmake\.exe$", "ctest.exe"
if (-not (Test-Path $CtestCmd)) { $CtestCmd = "ctest" }

& $CtestCmd --test-dir build --output-on-failure
if ($LASTEXITCODE -eq 0) {
    Write-Host "`n✔ ALL BUILDS & UNIT TESTS PASSED SUCCESSFULLY 100%!" -ForegroundColor Green
} else {
    Write-Host "`n❌ Unit tests encountered failures!" -ForegroundColor Red
}
