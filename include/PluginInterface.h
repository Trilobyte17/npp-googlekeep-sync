// Notepad++ Plugin Interface for Google Keep Sync Plugin
// ARM64 Windows Compatible
// Based on NppEventExec architecture patterns

#pragma once

#include <windows.h>
#include <string>
#include <vector>

// Plugin constants
#define PLUGIN_NAME           L"GoogleKeepSync"
#define PLUGIN_VERSION        L"1.0.0"
#define PLUGIN_CONFIG_FILE    L"GoogleKeepSync.ini"

// Menu command IDs
#define ID_PLUGIN_SYNC_NOW        0x01
#define ID_PLUGIN_CONFIGURE       0x02
#define ID_PLUGIN_TOGGLE_AUTOSYNC 0x03
#define ID_PLUGIN_ABOUT           0x04

// Function index for Notepad++
#define NOTEPADPLUS_USER   (WM_USER + 1000)
#define NPPM_GETCURRENTBUFFERID     (NOTEPADPLUS_USER + 4)
#define NPPM_GETFULLCURRENTPATH     (NOTEPADPLUS_USER + 5)
#define NPPM_NOTIFYBUFFERACTIVATED  (NOTEPADPLUS_USER + 21)
#define NPPM_FILEBEFORESAVE         (NOTEPADPLUS_USER + 23)
#define NPPM_FILEDDELETED           (NOTEPADPLUS_USER + 33)
#define NPPM_FILEBEFOREDELETE       (NOTEPADPLUS_USER + 32)

// Plugin message structure
struct PluginMessage {
    INT nCmdID;
    BOOL bEnabled;
    LPCWSTR pszName;
};

// Keep API Constants
#define KEEP_API_BASE_URL     L"https://keep.googleapis.com/v1/"
#define KEEP_AUTH_URL         L"https://accounts.google.com/o/oauth2/v2/auth"
#define KEEP_TOKEN_URL        L"https://oauth2.googleapis.com/token"
#define KEEP_SCOPE            L"https://www.googleapis.com/auth/keep"

// Note sync status
enum class SyncStatus {
    PENDING,
    SYNCED,
    FAILED,
    DISABLED
};

struct NoteMapping {
    std::wstring filePath;
    std::wstring keepNoteId;
    std::wstring lastSyncHash;
    SyncStatus status;
    FILETIME lastSyncTime;
};

// Plugin configuration
struct PluginConfig {
    BOOL autoSyncEnabled;
    std::wstring clientId;
    std::wstring clientSecret;
    std::wstring accessToken;
    std::wstring refreshToken;
    std::wstring defaultNoteTitle;
    BOOL syncFileMetadata;
    BOOL createLabels;
    std::vector<std::wstring> excludedExtensions;
};
