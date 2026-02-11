# ⛔ ARCHIVED - Google Keep Sync Plugin for Notepad++

> **This project is archived.** Google has deprecated third-party authentication for Google Keep API for personal accounts (since January 2025).

## Status

- ⛔ **No longer maintained**
- ❌ **Authentication broken** - Google deprecated app password auth for Keep API
- 📭 **Archived** - No further development planned unless Google restores personal account access

## What Happened

Google has effectively shut down third-party Keep API access for personal Gmail accounts. Both the `gkeepapi` library and `gpsoauth` authentication method now fail with "BadAuthentication" errors.

This affects:
- This plugin
- All apps using `gpsoauth` for Keep API
- Most third-party Google Keep integrations

## If Google Restores Access

If Google re-enables personal account access to Keep API:

1. The authentication flow would need to be updated to work with the new requirements
2. Check the [gkeepapi issues](https://github.com/kiwiz/gkeepapi/issues) for updates
3. This repository can be re-activated if a solution is found

## Original Features (when working)

- Auto-sync files to Google Keep on save
- App Password authentication (no OAuth complexity)
- ARM64 Windows optimized

## Installation (historical)

1. Copy `GoogleKeepSync.dll` and `keep_bridge.py` to:
   ```
   %LOCALAPPDATA%\Programs\Notepad++\plugins\GoogleKeepSync\
   ```
2. Install dependencies: `pip install gkeepapi gpsoauth`
3. Restart Notepad++

## For Personal Gmail Users

Options if you need Keep + Notepad++ integration:

1. **Google Takeout** - Export Keep notes periodically
2. **Manual copy/paste** - Use Keep web interface
3. **Alternative services** - Consider Evernote, OneNote, or Notion
4. **Enterprise accounts** - Google Workspace accounts may still have Keep API access

## License

MIT License - See LICENSE file

---

**Archived:** February 2026  
**Last working commit:** N/A - Authentication was deprecated before full functionality was achieved
