# Google Keep Sync Plugin for Notepad++ (ARM64)

A Notepad++ plugin for Windows ARM64 that syncs file contents to Google Keep notes using your Google account.

## Features

- **Auto-Sync on Save**: Automatically syncs file contents to Google Keep when you save
- **App Password Authentication**: Simple email + 16-char app password (no OAuth complexity)
- **ARM64 Optimized**: Built specifically for ARM64 Windows

## Prerequisites

1. **Notepad++ v8.0+** (ARM64 build)
2. **Windows 11/10 ARM64**
3. **Python 3.7+** with `gkeepapi` installed
4. **Google App Password** (see Setup section)

## Installation

1. Copy `GoogleKeepSync.dll` and `keep_bridge.py` to:
   ```
   %LOCALAPPDATA%\Programs\Notepad++\plugins\GoogleKeepSync\
   ```
2. Install Python dependency: `pip install gkeepapi`
3. Restart Notepad++

## Setup: Generate App Password

1. Enable 2FA: https://myaccount.google.com/security
2. Generate password: https://myaccount.google.com/apppasswords
3. Copy the 16-character password

## Configuration

1. **Plugins** → **Google Keep Sync** → **Configure...**
2. Enter your Gmail email and App Password
3. Click **Login**

## Security Notes

⚠️ **Important**: The app password is stored in plain text in:
```
%APPDATA%\Notepad++\plugins\config\GoogleKeepSync.ini
```

**Recommendations:**
1. Use a dedicated Google account for this plugin
2. Revoke the app password when not using the plugin
3. Consider the INI file permissions on shared systems
4. The app password can be revoked anytime at: https://myaccount.google.com/apppasswords

## Usage

- **Sync Now**: Manually sync current file
- **Toggle Auto-Sync**: Enable/disable auto-sync on save
- **Configure...**: Login and settings
- **About**: Plugin info

## Troubleshooting

**Plugin not loading:**
- Verify ARM64 Notepad++ version
- Check DLL is in correct plugins folder

**Login fails:**
- Ensure 2FA is enabled
- Copy password correctly (includes spaces)

## License

MIT License
