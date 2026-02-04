# Google Keep Sync Plugin - Test Script
# Run this after installation to verify functionality

param(
    [string]$NppPath = "$env:LOCALAPPDATA\Programs\Notepad++",
    [string]$TestFile = "$env:TEMP\npp_keep_test.txt"
)

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "Google Keep Sync Plugin - Test Script" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""

# Test 1: Check Notepad++ installation
Write-Host "Test 1: Checking Notepad++ installation..." -ForegroundColor Yellow
if (!(Test-Path "$NppPath\notepad++.exe")) {
    Write-Host "  FAIL: Notepad++ not found at $NppPath" -ForegroundColor Red
    exit 1
} else {
    Write-Host "  PASS: Notepad++ found" -ForegroundColor Green
    
    # Check architecture
    $arch = (Get-Item "$NppPath\notepad++.exe").VersionInfo
    Write-Host "  Version: $($arch.FileVersion)" -ForegroundColor Gray
}

# Test 2: Check plugin installation
Write-Host "`nTest 2: Checking plugin installation..." -ForegroundColor Yellow
$pluginPath = "$NppPath\plugins\GoogleKeepSync\GoogleKeepSync.dll"
if (!(Test-Path $pluginPath)) {
    Write-Host "  FAIL: Plugin DLL not found at`n  $pluginPath" -ForegroundColor Red
    exit 1
} else {
    Write-Host "  PASS: Plugin found" -ForegroundColor Green
    
    # Check DLL architecture
    try {
        $dllInfo = [System.Reflection.Assembly]::LoadFrom($pluginPath).GetName()
        Write-Host "  Architecture: $($dllInfo.ProcessorArchitecture)" -ForegroundColor Gray
    } catch {
        Write-Host "  Note: Unable to determine architecture (may be native DLL)" -ForegroundColor Gray
    }
}

# Test 3: Check configuration
Write-Host "`nTest 3: Checking configuration..." -ForegroundColor Yellow
$configPath = "$env:APPDATA\Notepad++\plugins\config\GoogleKeepSync.ini"
if (Test-Path $configPath) {
    Write-Host "  PASS: Config file exists" -ForegroundColor Green
    
    $config = Get-Content $configPath -Raw
    if ($config -match "ClientId=YOUR_CLIENT_ID_HERE") {
        Write-Host "  WARNING: OAuth credentials not configured" -ForegroundColor Yellow
        Write-Host "  Run: Plugins -> Google Keep Sync -> Configure..."
    } elseif ($config -match "ClientId=(.+)`) {
        Write-Host "  PASS: OAuth Client ID configured" -ForegroundColor Green
    }
} else {
    Write-Host "  WARNING: No config file found (will be created on first run)" -ForegroundColor Yellow
}

# Test 4: Create test file
Write-Host "`nTest 4: Creating test file..." -ForegroundColor Yellow
$testContent = @"
Google Keep Sync Test
=====================

This is a test file for the Notepad++ Google Keep Sync plugin.
Created: $(Get-Date)
Computer: $env:COMPUTERNAME
User: $env:USERNAME

If this note appears in Google Keep, the plugin is working correctly!
"@

Set-Content -Path $TestFile -Value $testContent -Encoding UTF8
Write-Host "  PASS: Test file created at $TestFile" -ForegroundColor Green

# Test 5: Simulate sync
Write-Host "`nTest 5: Manual sync test..." -ForegroundColor Yellow
Write-Host "  To test the plugin, please:" -ForegroundColor Gray
Write-Host "  1. Open Notepad++"
Write-Host "  2. Open the test file: $TestFile"
Write-Host "  3. Go to: Plugins -> Google Keep Sync -> Sync Now"
Write-Host "  4. Check Google Keep for a new note with title 'npp_keep_test'"

# Test 6: Network connectivity
Write-Host "`nTest 6: Checking network connectivity..." -ForegroundColor Yellow
$keepUrl = "https://keep.googleapis.com/v1/notes"
try {
    $response = Invoke-WebRequest -Uri $keepUrl -Method HEAD -TimeoutSec 5 -UseBasicParsing -ErrorAction Stop
    Write-Host "  PASS: Google Keep API is reachable" -ForegroundColor Green
} catch {
    Write-Host "  WARNING: Could not reach Google Keep API" -ForegroundColor Yellow
    Write-Host "  Error: $($_.Exception.Message)" -ForegroundColor Gray
}

# Summary
Write-Host "`n=========================================" -ForegroundColor Cyan
Write-Host "Test Summary:" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "Plugin Path: $pluginPath"
Write-Host "Config Path: $configPath"
Write-Host "Test File:   $TestFile"
Write-Host ""
Write-Host "Next Steps:" -ForegroundColor Cyan
Write-Host "1. Configure OAuth credentials if not done"
Write-Host "2. Open the test file in Notepad++"
Write-Host "3. Use Plugins -> Google Keep Sync -> Sync Now"
Write-Host "4. Verify note appears in keep.google.com"
Write-Host ""
Write-Host "For issues, check:" -ForegroundColor Gray
Write-Host "- %TEMP%\NppGoogleKeepSync.log (if debug logging enabled)"
Write-Host "- Notepad++ plugin documentation"
Write-Host "- Google Cloud Console for API status"
