# Google Keep Sync Plugin for Notepad++ (ARM64)

A Notepad++ plugin for Windows ARM64 (Prism Lake/QuarkBridge processors) that automatically syncs file contents to Google Keep notes using your personal Google account.

## Features

- **Auto-Sync on Save**: Automatically syncs file contents to Google Keep when you save a file in Notepad++
- **Create/Update Notes**: Creates new Google Keep notes or updates existing ones
- **Configurable Sync Behavior**: Toggle auto-sync, exclude specific file extensions, customize note titles
- **App Password Authentication**: Simple email + app password (no OAuth complexity)
- **ARM64 Optimized**: Built specifically for ARM64 Windows with Prism/QuarkBridge processors

## Architecture

- **C++ Plugin**: Hooks into Notepad++ notification events (FILEBEFORESAVE, BUFFERSAVED, etc.)
- **Python Bridge**: Uses `gkeepapi` library for Google Keep API communication
- **App Password Auth**: Simple login with Google App Password (no OAuth tokens to manage)

```
+----------------+      JSON/stdio       +------------------+      HTTPS       +---------------+
| Notepad++      |  <=================>  | keep_bridge.py   |  <============>  | Google Keep   |
| Plugin (C++)   |    (stdin/stdout)     | (Python/gkeepapi)|                  |   API         |
+----------------+                       +------------------+                  +---------------+
```

## Prerequisites

1. **Notepad++ v8.0+** (64-bit ARM64 build)
2. **Windows 11 ARM64** or Windows 10 ARM64
3. **Python 3.7+** (for the gkeepapi bridge)
4. **Google App Password** (see Setup section)

## Installation

### Method 1: Pre-built Binary

1. Download the ARM64 release: `GoogleKeepSync-arm64.dll`
2. Copy `GoogleKeepSync-arm64.dll` to:
   ```
   %LOCALAPPDATA%\Programs\Notepad++\plugins\GoogleKeepSync\
   ```
3. Copy `keep_bridge.py` to the same directory
4. Install Python dependencies:
   ```bash
   pip install gkeepapi
   ```
5. Restart Notepad++

### Method 2: Build from Source

#### Requirements
- Visual Studio 2022 with ARM64 components
- CMake 3.20+
- Windows SDK 10.0.22621.0+
- Python 3.7+ (for gkeepapi)

#### Build Steps

```batch
# Clone and configure
git clone https://github.com/Trilobyte17/npp-googlekeep-sync.git
cd NppGoogleKeepSync
mkdir build && cd build

# Configure for ARM64 Windows
cmake -A ARM64 .. -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_SYSTEM_PROCESSOR=ARM64

# Build
cmake --build . --config Release

# Install to Notepad++
# The post-build step copies the DLL to dist/plugins/GoogleKeepSync/
```

## Setup: Generate App Password

### 1. Enable 2-Factor Authentication

Before you can generate an App Password, you must have 2FA enabled on your Google account:

1. Go to https://myaccount.google.com/security
2. Ensure "2-Step Verification" is enabled

### 2. Generate App Password

1. Go to https://myaccount.google.com/apppasswords
2. Sign in with your Google account
3. Under "Select app", choose "Other" and enter "Notepad++ Keep Sync"
4. Click "Generate"
5. **Copy the 16-character password** (format: `abcd efgh ijkl mnop`)
6. Use this password in the plugin configuration

**Important**: You'll need this password once to log in. The plugin stores it securely.

## Configuration

### Plugin Configuration

1. Open Notepad++
2. Go to **Plugins** → **Google Keep Sync** → **Configure...**
3. Enter:
   - **Email**: Your Google account email (e.g., `user@gmail.com`)
   - **App Password**: The 16-character password from step 6 above
4. Click **Login**
5. Status should show "Login successful"

### Manual Configuration File

You can also edit the config file directly:

```ini
; %APPDATA%\Notepad++\plugins\config\GoogleKeepSync.ini
[Settings]
AutoSync=1

[Credentials]
Email=your.email@gmail.com
AppPassword=abcd efgh ijkl mnop
```

## Usage

### Menu Options

Access via **Plugins** → **Google Keep Sync**:

- **Sync Now** - Manually sync current file to Google Keep
- **Toggle Auto-Sync** - Enable/disable automatic syncing on save
- **Configure...** - Open configuration dialog for login/settings
- **About** - Plugin information

### How It Works

1. When you save a file in Notepad++, the plugin receives a notification
2. If auto-sync is enabled, it checks if the file needs syncing
3. Creates/updates a Google Keep note with:
   - **Title**: `Notepad++ Sync: filename`
   - **Content**: Full file contents
   - **Labels**: `Notepad++`, `Auto-Sync` (configurable)
4. First save creates a new note; subsequent saves update the same note

### Excluding File Types

Edit the configuration to exclude specific extensions:

```ini
[Settings]
ExcludedExtensions=exe,dll,png,jpg,pdf
```

## Troubleshooting

### Plugin not loading
- Ensure you have the ARM64 version of Notepad++
- Check that the DLL is in `%LOCALAPPDATA%\Programs\Notepad++\plugins\GoogleKeepSync\`
- Verify `keep_bridge.py` is in the same directory

### "gkeepapi not installed"
Run: `pip install gkeepapi`

### Login fails
- Verify you generated an App Password (not your regular password)
- Ensure 2FA is enabled on your Google account
- Check that you copied the password correctly (includes spaces)

### Sync fails
- Ensure network connectivity
- Try logging out and logging in again

### Process Communication Timeout
- Check that Python is in your system PATH
- Verify `keep_bridge.py` exists in the plugin directory

## Security Notes

1. **App Passwords** are safer than storing your main password
2. Credentials are stored in `%APPDATA%\Notepad++\plugins\config\`
3. App Passwords can be revoked from https://myaccount.google.com/apppasswords
4. Never commit credentials to version control

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

- **gkeepapi**: Python library for Google Keep API
- Notepad++ plugin development guide
- Google Keep API documentation

## Support

For issues, please check:
1. This repository's Issues page
2. gkeepapi documentation
3. Notepad++ plugin development documentation

---

**Version**: 2.0.0  
**Platform**: Windows ARM64  
**Notepad++**: v8.0+  
**Auth**: Google App Password  
**Python**: gkeepapi
