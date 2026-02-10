// Notepad++ Plugin Interface for Google Keep Sync Plugin
// ARM64 Windows Compatible
// Based on NppEventExec architecture patterns

#pragma once

#include <windows.h>
#include <string>
#include <vector>

// Plugin constants
#define PLUGIN_NAME           L"GoogleKeepSync"
#define PLUGIN_VERSION        L"2.0.0"
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

// Notification codes (from Notepad++ SDK)
#define NPPN_FILEBEFORESAVE 2001
#define NPPN_BUFFERSAVED    2002
#define NPPN_FILEDELETED    2003
#define NPPN_FILECLOSED     2004
#define NPPN_FILEOPENED     2005
#define NPPN_BUFFERACTIVATED 2008

// Notepad++ data structures (minimal definitions for standalone build)
struct NppData {
    HWND _nppHandle;
    HWND _scintillaMainHandle;
    HWND _scintillaSecondHandle;
};

// NMHDR structure (used by SCNotification)
struct NMHDR {
    HWND hwndFrom;
    UINT idFrom;
    UINT code;
};

// SCNotification structure
struct SCNotification {
    NMHDR nmhdr;
    int _nmmsg;
    uintptr_t _npParam;
    const char* _pszText;
    int _matchBrackets;
    int _matchBracketsBeginEnd;
    int _nrChars;
    BOOL _isModified;
    int _line;
    int _firstVisibleLine;
    int _linesOnScreen;
    int _curLine;
    int _curCol;
    BOOL _LCIsVisible;
    long long _modificationTypeSerialNumber;
    int _userActivity;
    int _isPeekState;
    void* _data;
};

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
    std::wstring clientId;      // Email address
    std::wstring clientSecret;  // App Password
    std::wstring defaultNoteTitle;
    BOOL syncFileMetadata;
    BOOL createLabels;
    std::vector<std::wstring> excludedExtensions;
};

// Helper function for JSON extraction
std::string extractJsonValue(const std::string& json, const std::string& key);
