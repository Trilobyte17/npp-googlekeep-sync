# Google Keep Bridge

A Python-to-C++ bridge for the Notepad++ Google Keep Sync plugin. This module replaces the previous OAuth-based authentication with a simpler App Password approach using the `gkeepapi` library.

## Overview

The bridge consists of:
1. **`keep_bridge.py`** - Python script using `gkeepapi` to communicate with Google Keep
2. **`PythonBridge.h/cpp`** - C++ implementation that manages the Python subprocess
3. No more OAuth complexity - uses Google App Passwords instead

## Setup

### Prerequisites

1. **Install Python 3.7+**
2. **Install gkeepapi**:
   ```bash
   pip install gkeepapi
   ```

3. **Generate Google App Password**:
   - Go to https://myaccount.google.com/apppasswords
   - Sign in with your Google account
   - Select "Other" and name it "Notepad++ Keep Sync"
   - Copy the 16-character password

### Configuration

Authentication credentials are stored in:
- Windows: `%APPDATA%/Notepad++/plugins/config/keep_bridge/`
- Linux: `~/.config/keep_bridge/`

## Architecture

```
+----------------+      JSON/stdio       +------------------+      HTTPS       +---------------+
| Notepad++      |  <=================>  | keep_bridge.py   |  <============>  | Google Keep   |
| Plugin (C++)   |    (stdin/stdout)     | (Python/gkeepapi)|                  |   API         |
+----------------+                       +------------------+                  +---------------+
       |                                          |
       |           PythonBridge.cpp               |
       +------------------------------------------+
```

## Python Bridge API

The Python script accepts JSON commands via stdin and outputs JSON responses:

### Commands

#### Login
```json
{"command": "login", "params": {"email": "user@gmail.com", "app_password": "abcd efgh ijkl mnop"}}
```
**Response:**
```json
{"success": true, "message": "Login successful", "email": "user@gmail.com"}
```

#### Sync
```json
{"command": "sync"}
```
**Response:**
```json
{"success": true, "message": "Sync completed"}
```

#### List Notes
```json
{"command": "list", "params": {"all": false, "limit": 100, "query": "shopping"}}
```
**Response:**
```json
{
  "success": true,
  "count": 2,
  "notes": [
    {"id": "123", "title": "Shopping List", "text": "Milk, Eggs...", "pinned": true, ...},
    {"id": "456", "title": "Gift Ideas", "text": "Book for mom...", "pinned": false, ...}
  ]
}
```

#### Get Note
```json
{"command": "get", "params": {"id": "123"}}
```
**Response:**
```json
{
  "success": true,
  "note": {
    "id": "123",
    "title": "Shopping List",
    "text": "Milk, Eggs, Bread",
    "pinned": true,
    "archived": false,
    "color": "RED",
    "labels": ["Shopping"],
    "timestamps": {...}
  }
}
```

#### Delete Note
```json
{"command": "delete", "params": {"id": "123", "permanent": false}}
```
**Response:**
```json
{"success": true, "message": "Note archived", "id": "123"}
```

#### Status
```json
{"command": "status"}
```
**Response:**
```json
{
  "success": true,
  "authenticated": true,
  "email": "user@gmail.com",
  "has_sync_state": true,
  "config_dir": "C:/Users/.../keep_bridge"
}
```

## C++ Integration

### Usage Example

```cpp
#include "PythonBridge.h"

// Initialize
PythonBridge bridge;
if (!bridge.Initialize(L"python", L"C:/plugins/keep_bridge.py")) {
    // Handle error
}

// Login
auto result = bridge.Login("user@gmail.com", "abcd efgh ijkl mnop");
if (!result.success) {
    MessageBoxA(NULL, result.error_message.c_str(), "Login Failed", MB_OK);
}

// Sync
bridge.Sync();

// List notes
auto listResult = bridge.ListNotes(false, 50);
if (listResult.success) {
    auto notes = bridge.ParseNoteList(listResult.raw_json);
    for (const auto& note : notes) {
        printf("%s: %s\n", note.title.c_str(), note.text.substr(0, 50).c_str());
    }
}

// Cleanup
bridge.Shutdown();
```

### Dependencies

Add to your `CMakeLists.txt`:

```cmake
# JSON parsing
find_package(jsoncpp REQUIRED)
target_link_libraries(your_plugin PRIVATE jsoncpp_lib)
```

Or include `jsoncpp` as a submodule/vendor dependency.

## Building

### Windows

```bash
# Build the plugin normally
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### Installing the Python Component

Copy `keep_bridge.py` to your plugin directory alongside the DLL.

## Troubleshooting

### "gkeepapi not installed"
Run: `pip install gkeepapi`

### "Login failed"
- Verify you generated an App Password (not your regular password)
- Check 2FA is enabled on your Google account (required for App Passwords)

### "Sync failed"
- Ensure network connectivity
- Try re-authenticating with `Logout()` then `Login()` again

### Process Communication Timeout
- Check that Python is in your PATH
- Verify `keep_bridge.py` exists at the specified path

## Security Notes

1. **App Passwords** are safer than storing your main password
2. Credentials are Base64 encoded (not encrypted) - typical for local app storage
3. App Passwords can be revoked from Google Account settings
4. Never commit credentials to version control

## Migration from OAuth

To migrate from OAuth-based authentication:

1. Remove OAuth token files
2. Generate App Password from Google Account
3. Call `Login()` with email + new App Password
4. Old OAuth tokens will be ignored

## License

Same as main project (MIT)
