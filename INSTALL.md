# Installation Guide - Google Keep Sync Plugin (ARM64)

## Prerequisites

- **Notepad++ v8.0 or later** (ARM64 build)
- **Windows 11 ARM64** or **Windows 10 ARM64** (build 19041+)
- **Google Cloud account** with OAuth 2.0 credentials

## Quick Install (Recommended)

### 1. Download Pre-built Binary

Download the latest ARM64 release: `GoogleKeepSync-arm64-v1.0.0.dll`

### 2. Install Plugin

```batch
REM Create plugin directory
mkdir "%LOCALAPPDATA%\Programs\Notepad++\plugins\GoogleKeepSync"

REM Copy DLL
copy GoogleKeepSync-arm64-v1.0.0.dll "%LOCALAPPDATA%\Programs\Notepad++\plugins\GoogleKeepSync\GoogleKeepSync.dll"
```

### 3. Restart Notepad++

Close and reopen Notepad++ to load the plugin.

### 4. Configure OAuth Credentials

See [OAuth Setup](#oauth-setup) section below.

## Build from Source

### Requirements

- Visual Studio 2022 with ARM64 build tools
- CMake 3.20+
- Windows SDK 10.0.22621.0 or later

### Build Steps

```batch
REM Open "Developer Command Prompt for VS 2022" with ARM64 support
REM Navigate to project directory
cd NppGoogleKeepSync

REM Run build script
build-arm64.bat

REM Or manually:
mkdir build && cd build
cmake -A ARM64 .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

The output will be in `build/bin/Release/GoogleKeepSync.dll`

## OAuth Setup

### 1. Google Cloud Configuration

1. Visit [Google Cloud Console](https://console.cloud.google.com/)
2. Create a new project
3. Enable the **Google Keep API**:
   - Go to **APIs & Services** → **Library**
   - Search "Google Keep API"
   - Click **Enable**

4. Create OAuth 2.0 credentials:
   - **APIs & Services** → **Credentials**
   - **Create Credentials** → **OAuth client ID**
   - Application type: **Desktop app**
   - Name: "Notepad++ Google Keep Sync"
   - Click **Create**

5. Download the JSON credentials file

### 2. Configure Plugin

#### Option A: Via Configuration Dialog

1. Open Notepad++
2. Go to **Plugins** → **Google Keep Sync** → **Configure...**
3. Paste Client ID and Client Secret
4. Click **Authenticate**
5. Follow browser prompts

#### Option B: Manual INI File

Create/copy the sample config file:

```batch
copy config\GoogleKeepSync.sample.ini %APPDATA%\Notepad++\plugins\config\GoogleKeepSync.ini
```

Edit with your credentials:

```ini
[OAuth]
ClientId=1234567890-abc123def456.apps.googleusercontent.com
ClientSecret=YourClientSecretHere
```

## Troubleshooting

### Plugin doesn't appear in menu

1. Verify Notepad++ is 64-bit ARM64 build
2. Check DLL is in correct directory:
   ```batch
   dir "%LOCALAPPDATA%\Programs\Notepad++\plugins\GoogleKeepSync\"
   ```
3. Check Windows Event Viewer for errors

### Authentication fails

1. Verify OAuth credentials are correct (no extra spaces)
2. Ensure Google Keep API is enabled
3. Check firewall/antivirus isn't blocking port 8899
4. Try manual INI configuration

### Sync errors

1. Verify internet connectivity to `keep.googleapis.com`
2. Check Google account has Keep access
3. Review log file: `%TEMP%\NppGoogleKeepSync.log`

### ARM64-specific issues

Verify your system is ARM64:
```batch
systeminfo | findstr /B /C:"OS Name" /C:"System Type"
```

Expected: "ARM64-based PC"

## Uninstallation

```batch
REM Remove plugin files
rmdir /s "%LOCALAPPDATA%\Programs\Notepad++\plugins\GoogleKeepSync"

REM Remove configuration (optional, keeps your data)
del "%APPDATA%\Notepad++\plugins\config\GoogleKeepSync.ini"
del "%APPDATA%\Notepad++\GoogleKeepSync.mappings"

REM Remove Google OAuth tokens (recommended)
REM Go to https://myaccount.google.com/permissions and revoke access
```

## Verification

After installation, verify:

1. Plugin appears in **Plugins** menu
2. Configuration dialog opens
3. Test sync creates note in Google Keep

## Support

- Check README.md for detailed documentation
- Review Google Keep API documentation
- Windows ARM64 development resources
