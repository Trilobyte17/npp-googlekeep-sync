// Main Plugin Core
// ARM64 Windows Compatible

#pragma once

#include "PluginInterface.h"
#include "gkeep_bridge/PythonBridge.h"
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
    std::unique_ptr<NppGoogleKeepSync::PythonBridge> m_keepBridge;
    
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
    std::unique_ptr<NppGoogleKeepSync::PythonBridge> m_keepBridge;
    
    // Menu handles
    HMENU m_hPluginMenu;
    INT m_funcCount;
    
    void CreateMenu();
    void ShowConfigDialog();
    void UpdateMenuState();
};

// DLL Export Functions
extern "C" {
    BOOL APIENTRY isUnicode();
    CONST WCHAR* APIENTRY getName();
    VOID APIENTRY setInfo(NppData notepadPlusData);
    CONST WCHAR* APIENTRY getFuncsArray(INT* nbF);
    BOOL APIENTRY messageProc(UINT msg, WPARAM wParam, LPARAM lParam);
    LONGLONG APIENTRY beNotificationProc(SCNotification* notifyCode);
}
