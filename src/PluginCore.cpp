// Main Plugin Core Implementation
// ARM64 Windows Compatible
// Updated to use PythonBridge instead of GoogleKeepClient/OAuth

#include "../include/PluginCore.h"
#include "../include/PluginInterface.h"
#include "../include/ConfigDialog.h"
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

// Helper function to extract value from JSON response
std::string extractJsonValue(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) {
        // Try with : delimiter
        search = "\"" + key + "\":";
        pos = json.find(search);
        if (pos == std::string::npos) return "";
    }
    
    size_t valuePos = json.find(":", pos);
    if (valuePos == std::string::npos) return "";
    valuePos++;
    
    // Skip whitespace
    while (valuePos < json.size() && json[valuePos] == ' ') valuePos++;
    if (valuePos >= json.size()) return "";
    
    // Check for string value
    if (json[valuePos] == '"') {
        valuePos++;
        size_t endPos = json.find("\"", valuePos);
        if (endPos == std::string::npos) return "";
        return json.substr(valuePos, endPos - valuePos);
    }
    
    // Check for numeric/boolean value
    size_t endPos = valuePos;
    while (endPos < json.size() && json[endPos] != ',' && json[endPos] != '}' && json[endPos] != ']') {
        endPos++;
    }
    std::string value = json.substr(valuePos, endPos - valuePos);
    // Trim whitespace
    while (!value.empty() && value[0] == ' ') value.erase(0, 1);
    while (!value.empty() && value[value.size()-1] == ' ') value.erase(value.size()-1);
    return value;
}

// FileSyncManager implementation
FileSyncManager::FileSyncManager() : m_autoSyncEnabled(TRUE) {}

FileSyncManager::~FileSyncManager() {
    Shutdown();
}

BOOL FileSyncManager::Initialize(const PluginConfig& config) {
    m_config = config;
    m_autoSyncEnabled = config.autoSyncEnabled;
    
    // Initialize Python bridge for Google Keep
    m_keepBridge = std::make_unique<NppGoogleKeepSync::PythonBridge>();
    
    // Get path to keep_bridge.py relative to plugin DLL
    wchar_t pluginPath[MAX_PATH];
    GetModuleFileNameW(NULL, pluginPath, MAX_PATH);
    std::wstring pluginDir(pluginPath);
    size_t pos = pluginDir.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        pluginDir = pluginDir.substr(0, pos);
    }
    
    std::wstring pythonScript = pluginDir + L"\\keep_bridge.py";
    
    // Initialize Python bridge
    if (!m_keepBridge->Initialize(L"python", pythonScript)) {
        std::wstring error = L"Failed to initialize Python bridge:\n" + 
                            std::wstring(m_keepBridge->GetLastError().begin(), m_keepBridge->GetLastError().end()) +
                            L"\n\nMake sure Python is in PATH and keep_bridge.py is in the plugin folder.";
        MessageBoxW(NULL, error.c_str(), L"Python Bridge Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }
    
    // Auto-login with stored credentials if available
    if (!m_config.email.empty() && !m_config.appPassword.empty()) {
        std::string email(m_config.email.begin(), m_config.email.end());
        std::string password(m_config.appPassword.begin(), m_config.appPassword.end());
        m_keepBridge->Login(email, password);
    }
    
    // Load mappings
    LoadMappings();
    
    return TRUE;
}

void FileSyncManager::Shutdown() {
    SaveMappings();
    if (m_keepBridge) {
        m_keepBridge->Shutdown();
    }
    m_keepBridge.reset();
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
    
    if (!m_keepBridge) {
        return FALSE;
    }
    
    // Login with stored credentials if not authenticated
    auto status = m_keepBridge->GetStatus();
    if (!status.success || status.raw_json.find("\"authenticated\":true") == std::string::npos) {
        // Try to login with stored credentials
        if (!m_config.email.empty() && !m_config.appPassword.empty()) {
            std::string email(m_config.email.begin(), m_config.email.end());
            std::string password(m_config.appPassword.begin(), m_config.appPassword.end());
            auto loginResult = m_keepBridge->Login(email, password);
            if (!loginResult.success) {
                MessageBoxW(NULL, L"Failed to authenticate with Google Keep. Please check your credentials.", L"Sync Failed", MB_OK | MB_ICONWARNING);
                return FALSE;
            }
        } else {
            MessageBoxW(NULL, L"Not authenticated with Google Keep. Please configure login in plugin settings.", L"Sync Failed", MB_OK | MB_ICONWARNING);
            return FALSE;
        }
    }
    
    std::wstring content = ReadFileContents(filePath);
    if (content.empty()) return FALSE;
    
    // Generate title from filename
    size_t lastSlash = filePath.find_last_of(L"/\\");
    size_t lastDot = filePath.find_last_of(L".");
    std::wstring title = filePath.substr(lastSlash + 1, 
                                         lastDot - lastSlash - 1);
    
    // Create note title with prefix
    std::wstring keepTitle = L"Notepad++ Sync: " + title;
    
    NoteMapping mapping = GetMapping(filePath);
    
    // Convert to UTF-8 for Python bridge
    std::string utf8Title(keepTitle.begin(), keepTitle.end());
    std::string utf8Content(content.begin(), content.end());
    
    BOOL result = FALSE;
    
    if (mapping.keepNoteId.empty()) {
        // Create NEW note on first sync
        auto createResult = m_keepBridge->CreateNote(utf8Title, utf8Content);
        if (createResult.success) {
            // Extract note ID from response
            std::string id = extractJsonValue(createResult.raw_json, "id");
            if (!id.empty()) {
                mapping.keepNoteId = std::wstring(id.begin(), id.end());
            }
            result = TRUE;
        }
    } else {
        // Update EXISTING note on subsequent syncs
        std::string noteId(mapping.keepNoteId.begin(), mapping.keepNoteId.end());
        auto updateResult = m_keepBridge->UpdateNote(noteId, utf8Title, utf8Content);
        result = updateResult.success;
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
    std::ifstream file(m_mappingsFile);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            // Parse CSV line: filePath,keepNoteId,lastSyncHash,status,timestamp
            size_t pos1 = line.find(',');
            if (pos1 == std::string::npos) continue;
            size_t pos2 = line.find(',', pos1 + 1);
            if (pos2 == std::string::npos) continue;
            size_t pos3 = line.find(',', pos2 + 1);
            if (pos3 == std::string::npos) continue;
            size_t pos4 = line.find(',', pos3 + 1);
            if (pos4 == std::string::npos) continue;
            
            NoteMapping mapping;
            mapping.filePath = std::wstring(line.begin(), line.begin() + pos1);
            mapping.keepNoteId = std::wstring(line.begin() + pos1 + 1, line.begin() + pos2);
            mapping.lastSyncHash = std::wstring(line.begin() + pos2 + 1, line.begin() + pos3);
            
            std::string statusStr(line.begin() + pos3 + 1, line.begin() + pos4);
            if (statusStr == "SYNCED") mapping.status = SyncStatus::SYNCED;
            else if (statusStr == "FAILED") mapping.status = SyncStatus::FAILED;
            else if (statusStr == "PENDING") mapping.status = SyncStatus::PENDING;
            else mapping.status = SyncStatus::DISABLED;
            
            m_mappings[mapping.filePath] = mapping;
        }
        file.close();
    }
}

void FileSyncManager::SaveMappings() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::ofstream file(m_mappingsFile);
    if (file.is_open()) {
        for (const auto& pair : m_mappings) {
            const NoteMapping& mapping = pair.second;
            std::string filePath(mapping.filePath.begin(), mapping.filePath.end());
            std::string noteId(mapping.keepNoteId.begin(), mapping.keepNoteId.end());
            std::string hash(mapping.lastSyncHash.begin(), mapping.lastSyncHash.end());
            
            std::string statusStr;
            switch (mapping.status) {
                case SyncStatus::SYNCED: statusStr = "SYNCED"; break;
                case SyncStatus::FAILED: statusStr = "FAILED"; break;
                case SyncStatus::PENDING: statusStr = "PENDING"; break;
                default: statusStr = "DISABLED"; break;
            }
            
            file << filePath << "," << noteId << "," << hash << "," << statusStr << "\n";
        }
        file.close();
    }
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
    wchar_t filePath[MAX_PATH];
    SendMessageW(m_hwndNpp, NPPM_GETFULLCURRENTPATH, MAX_PATH, (LPARAM)filePath);
    
    if (m_syncManager) {
        m_syncManager->SyncFile(filePath, TRUE);
    }
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
                L"Using App Password authentication\n"
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
        GetPrivateProfileStringW(L"Credentials", L"Email", L"", buffer, 1024, iniPath.c_str());
        m_config.email = buffer;
        
        GetPrivateProfileStringW(L"Credentials", L"AppPassword", L"", buffer, 1024, iniPath.c_str());
        m_config.appPassword = buffer;
    }
}

void GoogleKeepSyncPlugin::SaveConfig() {
    wchar_t configPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, configPath))) {
        std::wstring iniPath = std::wstring(configPath) + L"\\Notepad++\\plugins\\config\\GoogleKeepSync.ini";
        
        WritePrivateProfileStringW(L"Settings", L"AutoSync", 
                                   m_config.autoSyncEnabled ? L"1" : L"0", iniPath.c_str());
        WritePrivateProfileStringW(L"Credentials", L"Email", m_config.email.c_str(), iniPath.c_str());
        WritePrivateProfileStringW(L"Credentials", L"AppPassword", m_config.appPassword.c_str(), iniPath.c_str());
    }
}

LoginResult GoogleKeepSyncPlugin::Login(const std::wstring& email, const std::wstring& appPassword) {
    LoginResult result;
    result.success = false;
    
    if (!m_syncManager || !m_syncManager->GetBridge()) {
        result.error_message = L"Sync manager not initialized";
        return result;
    }
    
    std::string emailUtf8(email.begin(), email.end());
    std::string passwordUtf8(appPassword.begin(), appPassword.end());
    
    auto loginResult = m_syncManager->GetBridge()->Login(emailUtf8, passwordUtf8);
    
    if (loginResult.success) {
        // Save credentials
        m_config.email = email;
        m_config.appPassword = appPassword;
        SaveConfig();
        result.success = true;
    } else {
        result.error_message = std::wstring(loginResult.error_message.begin(), loginResult.error_message.end());
    }
    
    return result;
}

void GoogleKeepSyncPlugin::CreateMenu() {
    // Menu creation handled by Notepad++ plugin framework
    // The menu items are defined in DllMain.cpp
}

void GoogleKeepSyncPlugin::ShowConfigDialog() {
    ConfigDialog dlg(g_hInstance, m_hwndNpp, m_config);
    dlg.Show();
}

void GoogleKeepSyncPlugin::UpdateMenuState() {
    // Update menu checkmarks/state
}