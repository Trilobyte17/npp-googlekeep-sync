// Main Plugin Core
// ARM64 Windows Compatible

#pragma once

#include "PluginInterface.h"
#include "GoogleKeepAPI.h"
#include <thread>
#include <mutex>
#include <queue>

// File watcher for auto-sync
class FileSyncManager {
public:
    FileSyncManager();
    ~FileSyncManager();
    
    BOOL Initialize(const PluginConfig& config);
    void Shutdown();
    
    BOOL RegisterFile(const std::wstring& filePath);
    BOOL UnregisterFile(const std::wstring& filePath);
    BOOL SyncFile(const std::wstring& filePath, BOOL force = FALSE);
    
    void SetAutoSync(BOOL enabled);
    BOOL IsAutoSyncEnabled() const;
    
    NoteMapping GetMapping(const std::wstring& filePath) const;
    BOOL SetMapping(const std::wstring& filePath, const NoteMapping& mapping);
    
    void LoadMappings();
    void SaveMappings() const;
    
private:
    mutable std::mutex m_mutex;
    PluginConfig m_config;
    std::unordered_map<std::wstring, NoteMapping> m_mappings;
    std::wstring m_mappingsFile;
    BOOL m_autoSyncEnabled;
    std::unique_ptr<GoogleKeepClient> m_keepClient;
    
    std::wstring CalculateFileHash(const std::wstring& filePath);
    std::wstring ReadFileContents(const std::wstring& filePath);
    BOOL ShouldSync(const std::wstring& filePath);
};

// Main Plugin Class
class GoogleKeepSyncPlugin {
public:
    static GoogleKeepSyncPlugin& Instance();
    
    BOOL Init(HINSTANCE hInstance, HWND hwndNpp);
    void Terminate();
    
    // Notepad++ notification handlers
    void OnFileBeforeSave(const std::wstring& filePath);
    void OnFileSaved(const std::wstring& filePath);
    void OnFileClosed(const std::wstring& filePath);
    void OnBufferActivated(const std::wstring& filePath);
    
    // Menu commands
    void OnSyncNow();
    void OnConfigure();
    void OnToggleAutoSync();
    void OnAbout();
    
    // Plugin info
    const WCHAR* GetName() const { return PLUGIN_NAME; }
    const WCHAR* GetVersion() const { return PLUGIN_VERSION; }
    
    // Config accessors
    PluginConfig& GetConfig() { return m_config; }
    const PluginConfig& GetConfig() const { return m_config; }
    void SaveConfig();
    void LoadConfig();
    
    // ARM64 specific
    static BOOL IsARM64() { return TRUE; } // Compiled for ARM64
    
private:
    GoogleKeepSyncPlugin() = default;
    ~GoogleKeepSyncPlugin() = default;
    
    HINSTANCE m_hInstance;
    HWND m_hwndNpp;
    HWND m_hwndPlugin;
    
    PluginConfig m_config;
    std::unique_ptr<FileSyncManager> m_syncManager;
    std::unique_ptr<GoogleKeepClient> m_keepClient;
    
    // Menu handles
    HMENU m_hPluginMenu;
    INT m_funcCount;
    
    void CreateMenu();
    void ShowConfigDialog();
    void UpdateMenuState();
};

// DLL Export Functions
extern "C" {
    BOOL __declspec(dllexport) isUnicode();
    CONST WCHAR* __declspec(dllexport) getName();
    VOID __declspec(dllexport) setInfo(NppData notepadPlusData);
    CONST WCHAR* __declspec(dllexport) getFuncsArray(INT* nbF);
    BOOL __declspec(dllexport) messageProc(UINT msg, WPARAM wParam, LPARAM lParam);
    LONGLONG __declspec(dllexport) beNotificationProc(SCNotification* notifyCode);
}

struct NppData {
    HWND _nppHandle;
    HWND _scintillaMainHandle;
    HWND _scintillaSecondHandle;
};

struct SCNotification {
    struct Sci_NotifyHeader {
        void* hwndFrom;
        UINT_PTR idFrom;
        UINT code;
    } nmhdr;
    int position;
    int ch;
    int modifiers;
    int modificationType;
    const char* text;
    int length;
    int linesAdded;
    int message;
    UINT_PTR wParam;
    LONG_PTR lParam;
    int line;
    int foldLevelNow;
    int foldLevelPrev;
    int margin;
    int listType;
    int x;
    int y;
    int token;
    int annotationLinesAdded;
    int updated;
    int listCompletionMethod;
};
