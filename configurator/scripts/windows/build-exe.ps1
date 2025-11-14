# Build script for RC Gamepad Configurator (Windows)
# PowerShell script to build Windows executable with PyInstaller

# Stop on errors
$ErrorActionPreference = "Stop"

Write-Host "=== RC Gamepad Dongle Windows Build Script ===" -ForegroundColor Cyan

# Navigate to configurator directory if not already there
if (-not (Test-Path "src\rc-gamepad-dongle.py")) {
    if (Test-Path "configurator\src\rc-gamepad-dongle.py") {
        Set-Location configurator
    } else {
        Write-Host "Error: Must be run from project root or configurator directory" -ForegroundColor Red
        exit 1
    }
}

# Check for Python
try {
    $pythonVersion = & python --version 2>&1
    Write-Host "Python found: $pythonVersion" -ForegroundColor Green
} catch {
    Write-Host "Python not found. Please install Python 3.8 or higher." -ForegroundColor Red
    exit 1
}

# Check/Create virtual environment
if (-not (Test-Path ".venv")) {
    Write-Host "Creating virtual environment..." -ForegroundColor Yellow
    python -m venv .venv
    Write-Host "Virtual environment created" -ForegroundColor Green
}

# Activate virtual environment
Write-Host "Activating virtual environment..." -ForegroundColor Yellow
& ".\.venv\Scripts\Activate.ps1"

# Install/Update requirements
Write-Host "Installing/Updating dependencies..." -ForegroundColor Yellow
& .\.venv\Scripts\python.exe -m pip install --upgrade pip
& .\.venv\Scripts\pip.exe install -r requirements.txt
Write-Host "Dependencies installed" -ForegroundColor Green

# Convert logo to ICO format if it doesn't exist
if (-not (Test-Path "assets\logo.ico")) {
    Write-Host "Converting logo to ICO format..." -ForegroundColor Yellow
    $pythonCode = "from PIL import Image; img = Image.open('assets/logo_512.png'); img.save('assets/logo.ico', format='ICO', sizes=[(256,256), (128,128), (64,64), (48,48), (32,32), (16,16)])"
    & .\.venv\Scripts\python.exe -c $pythonCode
    Write-Host "Logo converted to ICO" -ForegroundColor Green
} else {
    Write-Host "Logo ICO file exists" -ForegroundColor Green
}

# Clean previous builds
Write-Host "Cleaning previous builds..." -ForegroundColor Yellow
if (Test-Path "build") {
    Remove-Item -Recurse -Force "build"
}
if (Test-Path "dist") {
    Remove-Item -Recurse -Force "dist"
}
if (Test-Path "src\build") {
    Remove-Item -Recurse -Force "src\build"
}
if (Test-Path "src\dist") {
    Remove-Item -Recurse -Force "src\dist"
}
# Clean PyInstaller cache
if (Test-Path "$env:LOCALAPPDATA\pyinstaller") {
    Remove-Item -Recurse -Force "$env:LOCALAPPDATA\pyinstaller"
}
Write-Host "Cleanup complete" -ForegroundColor Green

# Check for spec file
$specFile = "src\rc-gamepad-dongle-windows.spec"
if (-not (Test-Path $specFile)) {
    Write-Host "Windows spec file not found: $specFile" -ForegroundColor Red
    Write-Host "Creating default spec file..." -ForegroundColor Yellow
    
    # Create the spec file
    $specContent = @'
# -*- mode: python ; coding: utf-8 -*-
# Windows-specific PyInstaller spec file for RC Gamepad Dongle

block_cipher = None

a = Analysis(
    ['rc-gamepad-dongle.py'],
    pathex=[],
    binaries=[],
    datas=[],
    hiddenimports=['PySide6.QtSerialPort'],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    cipher=block_cipher,
    noarchive=False,
)

pyz = PYZ(a.pure, a.zipped_data, cipher=block_cipher)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.zipfiles,
    a.datas,
    [],
    name='rc-gamepad-dongle',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=False,
    disable_windowed_traceback=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
    icon='../assets/logo.ico',
)
'@
    Set-Content -Path $specFile -Value $specContent
    Write-Host "Spec file created" -ForegroundColor Green
} else {
    Write-Host "Using existing spec file: $specFile" -ForegroundColor Green
}

# Build with PyInstaller
Write-Host "Building with PyInstaller..." -ForegroundColor Yellow
Push-Location src
try {
    & ..\.venv\Scripts\pyinstaller.exe --clean rc-gamepad-dongle-windows.spec
    Write-Host "PyInstaller build complete" -ForegroundColor Green
} catch {
    Write-Host "Build failed: $_" -ForegroundColor Red
    Pop-Location
    exit 1
}
Pop-Location

# Check if executable was created
if (-not (Test-Path "src\dist\rc-gamepad-dongle.exe")) {
    Write-Host "Build failed: Executable not found" -ForegroundColor Red
    exit 1
}

# Get executable info
$exeInfo = Get-Item "src\dist\rc-gamepad-dongle.exe"
$exeSizeMB = [math]::Round($exeInfo.Length / 1MB, 2)

Write-Host ""
Write-Host "Build successful!" -ForegroundColor Green
Write-Host "Executable: src\dist\rc-gamepad-dongle.exe" -ForegroundColor Cyan
Write-Host "Size: $exeSizeMB MB" -ForegroundColor Cyan
Write-Host "Built: $($exeInfo.LastWriteTime)" -ForegroundColor Cyan
Write-Host ""
Write-Host "To test: .\src\dist\rc-gamepad-dongle.exe" -ForegroundColor Yellow
Write-Host "To distribute: Copy the .exe file to your desired location" -ForegroundColor Yellow
