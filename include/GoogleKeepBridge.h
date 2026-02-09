// Google Keep Bridge Integration
// ARM64 Windows Compatible
// Uses PythonBridge instead of OAuth/GoogleKeepClient

#pragma once

#include <string>
#include <vector>

// Simplified note structure compatible with PythonBridge
struct KeepNoteSimple {
    std::string id;
    std::string title;
    std::string text;
    bool pinned = false;
    bool archived = false;
    std::string color;
    std::vector<std::string> labels;
    std::string created_timestamp;
    std::string edited_timestamp;
};

// Bridge result structure
struct BridgeResult {
    bool success = false;
    std::string error_message;
    std::string raw_json;
};

// Forward declaration of PythonBridge
namespace NppGoogleKeepSync {
    class PythonBridge;
}

// Main bridge interface class
class GoogleKeepBridge {
public:
    GoogleKeepBridge();
    ~GoogleKeepBridge();
    
    BOOL Initialize();
    void Shutdown();
    
    // Authentication (using email/app password instead of OAuth)
    BOOL Login(const std::string& email, const std::string& app_password);
    BOOL Logout();
    BOOL IsAuthenticated() const;
    
    // Note operations
    BOOL SyncFileToKeep(const std::string& filepath, const std::string& content, std::string& outNoteId);
    BOOL UpdateNoteInKeep(const std::string& noteId, const std::string& content);
    BOOL GetNoteFromKeep(const std::string& noteId, KeepNoteSimple& outNote);
    BOOL DeleteNoteFromKeep(const std::string& noteId, BOOL permanent = FALSE);
    BOOL ListNotesFromKeep(std::vector<KeepNoteSimple>& outNotes, 
                           const std::string& query = "", 
                           int limit = 100);
    
    // File sync operations
    struct FileSyncInfo {
        std::string filepath;
        std::string noteId;
        std::string lastSyncHash;
        std::string lastSyncTime;
        BOOL synced = FALSE;
    };
    
    BOOL SyncFile(const FileSyncInfo& fileInfo);
    BOOL GetSyncStatus(const std::string& filepath, FileSyncInfo& outInfo);
    
    // Configuration
    struct BridgeConfig {
        std::string email;
        std::string app_password;  // Google App Password (16-character)
        BOOL auto_sync = TRUE;
        std::vector<std::string> excluded_extensions;
    };
    
    void SetConfig(const BridgeConfig& config);
    BridgeConfig GetConfig() const;
    
    // Error handling
    std::string GetLastError() const;
    
private:
    std::unique_ptr<NppGoogleKeepSync::PythonBridge> m_pythonBridge;
    BridgeConfig m_config;
    std::string m_lastError;
    
    BOOL EnsureAuthenticated();
    std::string GenerateNoteTitleFromFilename(const std::string& filepath);
    std::string CalculateFileHash(const std::string& filepath);
    
    // Private helpers for PythonBridge interaction
    BOOL ExecutePythonCommand(const std::string& command, const std::string& params, BridgeResult& result);
    std::vector<KeepNoteSimple> ParseNotesFromJson(const std::string& json);
    KeepNoteSimple ParseNoteFromJson(const std::string& json);
    
    // File sync mapping
    std::vector<FileSyncInfo> m_syncMappings;
    
    BOOL LoadMappings();
    BOOL SaveMappings();
    BOOL FindMapping(const std::string& filepath, FileSyncInfo& outInfo);
    BOOL UpdateMapping(const FileSyncInfo& info);
};