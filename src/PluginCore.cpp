// Main Plugin Core Implementation
// ARM64 Windows Compatible

#include "../include/PluginCore.h"
#include "../include/PluginInterface.h"
#include <wincrypt.h>
#include <sstream>
#include <fstream>
#include <shlobj.h>
#include <iomanip>

#pragma comment(lib, "shell32.lib")

// Static instance
GoogleKeepSyncPlugin& GoogleKeepSyncPlugin::Instance() {
    static GoogleKeepSyncPlugin instance;
    return instance;
}

// FileSyncManager implementation
FileSyncManager::FileSyncManager() : m_autoSyncEnabled(TRUE) {}

FileSyncManager::~FileSyncManager() {
    Shutdown();
}

BOOL FileSyncManager::Initialize(const PluginConfig& config) {
    m_config = config;
    m_autoSyncEnabled = config.autoSyncEnabled;
    
    // Initialize Keep client
    m_keepClient = std::make_unique<GoogleKeepClient>();
    if (!m_config.accessToken.empty()) {
        m_keepClient->Initialize(m_config.accessToken);
    }
    
    // Load mappings
    LoadMappings();
    
    return TRUE;
}

void FileSyncManager::Shutdown() {
    SaveMappings();
    m_keepClient.reset();
}

void FileSyncManager::SetAutoSync(BOOL enabled) {
    m_autoSyncEnabled = enabled;
}

BOOL FileSyncManager::IsAutoSyncEnabled() const {
    return m_autoSyncEnabled;
}

std::wstring FileSyncManager::CalculateFileHash(const std::wstring& filePath) {
    // Read file and calculate MD5 hash
    std::ifstream file(filePath, std::ios::binary);
    if (!file) return L"";
    
    HCRYPTPROV hProv;
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        return L"";
    }
    
    HCRYPTHASH hHash;
    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return L"";
    }
    
    std::vector<BYTE> buffer(8192);
    while (file.good()) {
        file.read((char*)buffer.data(), buffer.size());
        if (file.gcount() > 0) {
            CryptHashData(hHash, buffer.data(), (DWORD)file.gcount(), 0);
        }
    }
    
    BYTE hash[16];
    DWORD hashLen = 16;
    BOOL result = CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0);
    
    std::wostringstream hashStr;
    if (result) {
        for (DWORD i = 0; i < hashLen; i++) {
            hashStr << std::hex << std::setw(2) << std::setfill(L'0') << hash[i];
        }
    }
    
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    
    return hashStr.str();
}

std::wstring FileSyncManager::ReadFileContents(const std::wstring& filePath) {
    std::wifstream file(filePath);
    if (!file) return L"";
    
    std::wostringstream content;
    content << file.rdbuf();
    return content.str();
}

BOOL FileSyncManager::SyncFile(const std::wstring& filePath, BOOL force) {
    if (!force && !ShouldSync(filePath)) {
        return FALSE;
    }
    
    if (!m_keepClient || !m_keepClient->IsAuthenticated()) {
        return FALSE;
    }
    
    std::wstring content = ReadFileContents(filePath);
    if (content.empty()) return FALSE;
    
    // Generate title from filename
    size_t lastSlash = filePath.find_last_of(L"/\\");
    size_t lastDot = filePath.find_last_of(L".");
    std::wstring title = filePath.substr(lastSlash + 1, 
                                         lastDot - lastSlash - 1);
    
    NoteMapping mapping = GetMapping(filePath);
    GoogleKeepClient::KeepNote note;
    
    BOOL result;
    if (mapping.keepNoteId.empty()) {
        // Create new note
        std::vector<std::wstring> labels;
        if (m_config.syncFileMetadata) {
            labels.push_back(L"Notepad++");
            labels.push_back(L"Auto-Sync");
        }
        
        result = m_keepClient->CreateNote(title, content, labels, note);
        if (result) {
            mapping.keepNoteId = note.id;
        }
    } else {
        // Update existing note
        result = m_keepClient->UpdateNote(mapping.keepNoteId, title, content, note);
    }
    
    if (result) {
        mapping.filePath = filePath;
        mapping.lastSyncHash = CalculateFileHash(filePath);
        GetSystemTimeAsFileTime(&mapping.lastSyncTime);
        mapping.status = SyncStatus::SYNCED;
        SetMapping(filePath, mapping);
    } else {
        mapping.status = SyncStatus::FAILED;
    }
    
    return result;
}

BOOL FileSyncManager::ShouldSync(const std::wstring& filePath) {
    if (!m_autoSyncEnabled) return FALSE;
    
    // Check excluded extensions
    size_t dotPos = filePath.find_last_of(L".");
    if (dotPos != std::wstring::npos) {
        std::wstring ext = filePath.substr(dotPos + 1);
        for (const auto& excluded : m_config.excludedExtensions) {
            if (_wcsicmp(ext.c_str(), excluded.c_str()) == 0) {
                return FALSE;
            }
        }
    }
    
    NoteMapping mapping = GetMapping(filePath);
    std::wstring currentHash = CalculateFileHash(filePath);
    
    return mapping.lastSyncHash != currentHash;
}

NoteMapping FileSyncManager::GetMapping(const std::wstring& filePath) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_mappings.find(filePath);
    if (it != m_mappings.end()) {
        return it->second;
    }
    return NoteMapping();
}

BOOL FileSyncManager::SetMapping(const std::wstring& filePath, const NoteMapping& mapping) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mappings[filePath] = mapping;
    return TRUE;
}

void FileSyncManager::LoadMappings() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Get config directory
    wchar_t configPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, configPath))) {
        m_mappingsFile = std::wstring(configPath) + L"\\Notepad++\\GoogleKeepSync.mappings";
    }
    
    // Load from file (simple CSV format)
    // Implementation omitted for brevity
}

void FileSyncManager::SaveMappings() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    // Save to file
}

// GoogleKeepSyncPlugin implementation
BOOL GoogleKeepSyncPlugin::Init(HINSTANCE hInstance, HWND hwndNpp) {
    m_hInstance = hInstance;
    m_hwndNpp = hwndNpp;
    
    // Load configuration
    LoadConfig();
    
    // Initialize sync manager
    m_syncManager = std::make_unique<FileSyncManager>();
    m_syncManager->Initialize(m_config);
    
    // Create menu
    CreateMenu();
    
    return TRUE;
}

void GoogleKeepSyncPlugin::Terminate() {
    if (m_syncManager) {
        m_syncManager->Shutdown();
    }
}

void GoogleKeepSyncPlugin::OnFileBeforeSave(const std::wstring& filePath) {
    // File is about to be saved
}

void GoogleKeepSyncPlugin::OnFileSaved(const std::wstring& filePath) {
    if (m_syncManager && m_config.autoSyncEnabled) {
        m_syncManager->SyncFile(filePath, FALSE);
    }
}

void GoogleKeepSyncPlugin::OnFileClosed(const std::wstring& filePath) {
    // Cleanup if needed
}

void GoogleKeepSyncPlugin::OnBufferActivated(const std::wstring& filePath) {
    // Could show sync status in UI
}

void GoogleKeepSyncPlugin::OnSyncNow() {
    // Get current file
    // Call sync
}

void GoogleKeepSyncPlugin::OnConfigure() {
    ShowConfigDialog();
}

void GoogleKeepSyncPlugin::OnToggleAutoSync() {
    m_config.autoSyncEnabled = !m_config.autoSyncEnabled;
    if (m_syncManager) {
        m_syncManager->SetAutoSync(m_config.autoSyncEnabled);
    }
}

void GoogleKeepSyncPlugin::OnAbout() {
    MessageBoxW(m_hwndNpp, 
                L"Google Keep Sync Plugin v1.0.0\n\n"
                L"Automatically sync Notepad++ files to Google Keep.\n\n"
                L"ARM64 Windows Edition\n"
                L"\u00a9 2025",
                L"About Google Keep Sync",
                MB_OK);
}

void GoogleKeepSyncPlugin::LoadConfig() {
    // Load from INI file
    wchar_t configPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, configPath))) {
        std::wstring iniPath = std::wstring(configPath) + L"\\Notepad++\\plugins\\config\\GoogleKeepSync.ini";
        
        m_config.autoSyncEnabled = GetPrivateProfileIntW(L"Settings", L"AutoSync", 1, iniPath.c_str()) != 0;
        
        wchar_t buffer[1024];
        GetPrivateProfileStringW(L"OAuth", L"ClientId", L"", buffer, 1024, iniPath.c_str());
        m_config.clientId = buffer;
        
        GetPrivateProfileStringW(L"OAuth", L"ClientSecret", L"", buffer, 1024, iniPath.c_str());
        m_config.clientSecret = buffer;
        
        GetPrivateProfileStringW(L"OAuth", L"RefreshToken", L"", buffer, 1024, iniPath.c_str());
        m_config.refreshToken = buffer;
    }
}

void GoogleKeepSyncPlugin::SaveConfig() {
    wchar_t configPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, configPath))) {
        std::wstring iniPath = std::wstring(configPath) + L"\\Notepad++\\plugins\\config\\GoogleKeepSync.ini";
        
        WritePrivateProfileStringW(L"Settings", L"AutoSync", 
                                   m_config.autoSyncEnabled ? L"1" : L"0", iniPath.c_str());
        WritePrivateProfileStringW(L"OAuth", L"ClientId", m_config.clientId.c_str(), iniPath.c_str());
        WritePrivateProfileStringW(L"OAuth", L"ClientSecret", m_config.clientSecret.c_str(), iniPath.c_str());
        WritePrivateProfileStringW(L"OAuth", L"RefreshToken", m_config.refreshToken.c_str(), iniPath.c_str());
    }
}

void GoogleKeepSyncPlugin::CreateMenu() {
    // Menu creation handled by Notepad++ plugin framework
}

void GoogleKeepSyncPlugin::ShowConfigDialog() {
    // Create and show configuration dialog
    // Would use CreateDialog or DialogBox
}

void GoogleKeepSyncPlugin::UpdateMenuState() {
    // Update menu checkmarks/state
}
