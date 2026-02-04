# Google Keep Sync Plugin for Notepad++ (ARM64)

A Notepad++ plugin for Windows ARM64 (Prism Lake/QuarkBridge processors) that automatically syncs file contents to Google Keep notes.

## Features

- **Auto-Sync on Save**: Automatically syncs file contents to Google Keep when you save a file in Notepad++
- **Create/Update Notes**: Creates new Google Keep notes or updates existing ones
- **Configurable Sync Behavior**: Toggle auto-sync, exclude specific file extensions, customize note titles
- **OAuth 2.0 Authentication**: Secure PKCE-based authentication flow
- **ARM64 Optimized**: Built specifically for ARM64 Windows with Prism/QuarkBridge processors

## Architecture

This plugin follows the NppEventExec plugin pattern:
- Hooks into Notepad++ notification events (FILEBEFORESAVE, BUFFERSAVED, etc.)
- Uses C++ native Windows APIs (WinHTTP, Windows Cryptography)
- Implements OAuth 2.0 with PKCE for secure credential handling
- Uses Google Keep REST API v1

## Prerequisites

1. **Notepad++ v8.0+** (64-bit ARM64 build)
2. **Windows 11 ARM64** or Windows 10 ARM64
3. **Google Cloud OAuth Credentials** (see Setup section)

## Installation

### Method 1: Pre-built Binary

1. Download the ARM64 release: `GoogleKeepSync-arm64.dll`
2. Copy to Notepad++ plugins directory:
   ```
   %LOCALAPPDATA%\Programs\Notepad++\plugins\GoogleKeepSync\
   ```
3. Restart Notepad++

### Method 2: Build from Source

#### Requirements
- Visual Studio 2022 with ARM64 components
- CMake 3.20+
- Windows SDK 10.0.22621.0+

#### Build Steps

```batch
# Clone and configure
git clone <repository>
cd NppGoogleKeepSync
mkdir build && cd build

# Configure for ARM64 Windows
cmake -A ARM64 .. -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_SYSTEM_PROCESSOR=ARM64

# Build
cmake --build . --config Release

# Install to Notepad++
# The post-build step copies the DLL to dist/plugins/GoogleKeepSync/
```

## Google Cloud Setup

### 1. Create OAuth 2.0 Credentials

1. Go to [Google Cloud Console](https://console.cloud.google.com/)
2. Create a new project or select existing
3. Enable the Google Keep API:
   - Navigate to **APIs & Services** → **Library**
   - Search for "Google Keep API"
   - Click **Enable**

4. Create OAuth 2.0 credentials:
   - Go to **APIs & Services** → **Credentials**
   - Click **Create Credentials** → **OAuth client ID**
   - Application type: **Desktop app**
   - Name: "Notepad++ Google Keep Sync"
   - Click **Create**

5. **Important**: Download the client credentials JSON file

### 2. Configure Plugin

1. Open Notepad++
2. Go to **Plugins** → **Google Keep Sync** → **Configure...**
3. Enter your OAuth credentials:
   - Client ID (from Google Cloud Console)
   - Client Secret (from Google Cloud Console)
4. Click **Authenticate**
5. A browser window will open for Google authorization
6. Grant permission when prompted
7. The plugin will automatically capture the authorization code via localhost callback

### OAuth Configuration File

Alternatively, create the config file manually:

```ini
; %APPDATA%\Notepad++\plugins\config\GoogleKeepSync.ini
[Settings]
AutoSync=1

[OAuth]
ClientId=YOUR_CLIENT_ID_HERE
ClientSecret=YOUR_CLIENT_SECRET_HERE
RefreshToken=(populated after first auth)
```

## Usage

### Menu Options

Access via **Plugins** → **Google Keep Sync**:

- **Sync Now** - Manually sync current file to Google Keep
- **Toggle Auto-Sync** - Enable/disable automatic syncing on save
- **Configure...** - Open configuration dialog for OAuth settings
- **About** - Plugin information

### How It Works

1. When you save a file in Notepad++, the plugin receives a notification
2. If auto-sync is enabled, it checks if the file needs syncing
3. Creates a Google Keep note with:
   - **Title**: Filename (without extension)
   - **Content**: Full file contents
   - **Labels**: "Notepad++", "Auto-Sync" (configurable)
4. First save creates a new note; subsequent saves update the same note

### Excluding File Types

Edit the configuration to exclude specific extensions:

```ini
[Settings]
ExcludedExtensions=exe,dll,png,jpg,pdf
```

## API Implementation Details

### Google Keep REST API

The plugin uses these Keep API endpoints:

```
POST https://keep.googleapis.com/v1/notes
GET  https://keep.googleapis.com/v1/notes/{noteId}
PATCH https://keep.googleapis.com/v1/notes/{noteId}
```

### OAuth 2.0 Flow (PKCE)

1. Plugin generates code verifier and challenge
2. Opens browser with authorization URL including PKCE parameters
3. User authenticates with Google
4. Google redirects to `http://localhost:8899/callback`
5. Plugin's embedded HTTP server captures the authorization code
6. Code is exchanged for access token and refresh token
7. Refresh token is stored securely for future API calls

## Troubleshooting

### Plugin not loading
- Ensure you have the ARM64 version of Notepad++
- Check that the DLL is in `%LOCALAPPDATA%\Programs\Notepad++\plugins\GoogleKeepSync\`

### Authentication fails
- Verify OAuth credentials are correct in Google Cloud Console
- Ensure Google Keep API is enabled in your project
- Check firewall/antivirus isn't blocking localhost:8899

### Sync fails
- Check OAuth tokens haven't expired
- Verify file size is under Google Keep limits (content length)
- Ensure network connectivity to keep.googleapis.com

### ARM64 specific issues
- Check Visual C++ Redistributables are installed (ARM64 version)
- Ensure Windows is on ARM64 build (run `systeminfo` to verify)

## Building for Other Architectures

While this plugin is optimized for ARM64, it can also be built for x64:

```batch
cmake -A x64 ..
cmake --build . --config Release
```

The code is portable and will work on x64 Windows as well.

## License

MIT License - See LICENSE file

## Acknowledgments

- Based on the NppEventExec and PythonScript plugin architectures
- Google Keep API documentation
- Notepad++ plugin development guide

## Support

For issues, please check:
1. Notepad++ plugin development documentation
2. Google Keep API reference
3. Windows ARM64 development guides

---

**Version**: 1.0.0  
**Platform**: Windows ARM64  
**Notepad++**: v8.0+  
**API**: Google Keep REST API v1
