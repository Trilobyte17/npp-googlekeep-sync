# Google Keep Sync Plugin for Notepad++ (ARM64)

A Notepad++ plugin for Windows ARM64 that syncs file contents to Google Keep notes.

## ⚠️ Authentication Notice

**Google has deprecated app password authentication for Keep API** (since January 2025). The login functionality may not work for personal Gmail accounts.

**Current Status:**
- `gpsoauth` library may fail with "BadAuthentication" errors
- This affects all third-party Keep API clients
- Google OAuth requires Workspace/Enterprise accounts

## Workaround: Use a Cached Master Token

If you can obtain a valid master token from another device or at a different time, you can use it:

1. Get a master token using Docker (requires different network/location):
   ```bash
   docker run --rm -it --entrypoint /bin/sh python:3 -c '
   pip install gpsoauth
   python3 -c "
   from gpsoauth import exchange_token, perform_master_login
   email = input(\"Email: \")
   app_password = input(\"App Password: \")
   android_id = \"ae7d752d1764a7b6\"
   master = exchange_token(email, app_password, android_id)
   print(\"Master Token:\", master.get(\"Token\", master.get(\"Error\")))
   "
   '
   ```

2. Use the token manually:
   ```bash
   python -c "
   import json
   import sys
   sys.path.insert(0, 'C:/Program Files/Notepad++/plugins/GoogleKeepSync')
   from keep_bridge import KeepBridge
   bridge = KeepBridge()
   result = bridge.handle_set_token({
       'email': 'your.email@gmail.com',
       'master_token': 'your-master-token-here',
       'device_id': 'ae7d752d1764a7b6'
   })
   print(json.dumps(result, indent=2))
   "
   ```

3. Once token is saved, restart Notepad++ - the plugin will use the cached token

## Prerequisites

1. **Notepad++ v8.0+** (ARM64 build)
2. **Windows 11/10 ARM64**
3. **Python 3.7+** with dependencies:
   ```
   pip install gkeepapi gpsoauth
   ```

## Installation

1. Copy `GoogleKeepSync.dll` and `keep_bridge.py` to:
   ```
   %LOCALAPPDATA%\Programs\Notepad++\plugins\GoogleKeepSync\
   ```
2. Install Python dependencies
3. Restart Notepad++

## Configuration

**Plugins** → **Google Keep Sync** → **Configure...**

## Security Notes

⚠️ Credentials are stored in:
```
%APPDATA%\Notepad++\plugins\config\GoogleKeepSync.ini
```

## License

MIT License
