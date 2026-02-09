#pragma once

/**
 * PythonBridge - C++ to Python bridge for Google Keep integration
 * 
 * This class replaces OAuthManager and GoogleKeepClient functionality,
 * providing a simple interface to communicate with the Python keep_bridge.py
 * script via JSON inter-process communication.
 * 
 * Uses App Password authentication instead of OAuth.
 */

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>
#include <windows.h>

namespace NppGoogleKeepSync {

// Forward declarations
struct KeepNote;

/**
 * Result structure for bridge operations
 */
struct BridgeResult {
    bool success = false;
    std::string error_message;
    std::string raw_json;
};

/**
 * Simple note structure for C++ side
 */
struct KeepNote {
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

/**
 * PythonBridge class - manages Python subprocess and JSON communication
 */
class PythonBridge {
public:
    PythonBridge();
    ~PythonBridge();

    // Non-copyable
    PythonBridge(const PythonBridge&) = delete;
    PythonBridge& operator=(const PythonBridge&) = delete;

    // Movable
    PythonBridge(PythonBridge&& other) noexcept;
    PythonBridge& operator=(PythonBridge&& other) noexcept;

    /**
     * Initialize the bridge - starts Python subprocess
     * @param python_path Path to Python executable (empty for default 'python')
     * @param script_path Path to keep_bridge.py script
     * @return true if initialization succeeded
     */
    bool Initialize(const std::wstring& python_path, const std::wstring& script_path);

    /**
     * Shutdown the bridge - terminates Python subprocess
     */
    void Shutdown();

    /**
     * Check if bridge is currently connected to Python process
     */
    bool IsConnected() const { return m_connected; }

    // Authentication (using App Password instead of OAuth)
    
    /**
     * Login with email and Google App Password
     * @param email Google account email
     * @param app_password Google App Password (16-character code)
     * @return BridgeResult with success status
     */
    BridgeResult Login(const std::string& email, const std::string& app_password);

    /**
     * Check current authentication status
     * @return BridgeResult with auth info in raw_json
     */
    BridgeResult GetStatus();

    /**
     * Logout - clears stored credentials
     */
    void Logout();

    // Note operations

    /**
     * Create a new note in Google Keep
     * @param title Note title
     * @param text Note content
     * @param pinned Pin the note
     * @param color Note color (e.g., "RED", "BLUE", "YELLOW", "GREEN", "DEFAULT")
     * @param labels Array of label names
     * @return BridgeResult with created note info
     */
    BridgeResult CreateNote(const std::string& title, const std::string& text,
                           bool pinned = false, const std::string& color = "DEFAULT",
                           const std::vector<std::string>& labels = {});

    /**
     * Update an existing note in Google Keep
     * @param note_id Google Keep note ID
     * @param title New title (empty to keep current)
     * @param text New text content (empty to keep current)
     * @param pinned Pin state (nullopt to keep current)
     * @param color Note color (empty to keep current)
     * @param labels Array of label names (empty to keep current)
     * @return BridgeResult with updated note info
     */
    BridgeResult UpdateNote(const std::string& note_id,
                           const std::optional<std::string>& title = std::nullopt,
                           const std::optional<std::string>& text = std::nullopt,
                           const std::optional<bool>& pinned = std::nullopt,
                           const std::optional<std::string>& color = std::nullopt,
                           const std::optional<std::vector<std::string>>& labels = std::nullopt);

    /**
     * Sync notes with Google Keep
     * @return BridgeResult with sync status
     */
    BridgeResult Sync();

    /**
     * List notes with optional filtering
     * @param all Include archived notes
     * @param limit Maximum number of notes to return
     * @param query Search query string
     * @param labels Filter by labels
     * @return BridgeResult with note list in raw_json
     */
    BridgeResult ListNotes(bool all = false, int limit = 100, 
                           const std::string& query = "",
                           const std::vector<std::string>& labels = {});

    /**
     * Get a specific note by ID
     * @param note_id Google Keep note ID
     * @return BridgeResult with full note content in raw_json
     */
    BridgeResult GetNote(const std::string& note_id);

    /**
     * Delete/archive a note
     * @param note_id Note to delete
     * @param permanent If true, permanently delete; otherwise archive
     * @return BridgeResult with deletion status
     */
    BridgeResult DeleteNote(const std::string& note_id, bool permanent = false);

    // Utility methods

    /**
     * Parse note list from JSON response
     * @param json_response Raw JSON from ListNotes
     * @return Vector of KeepNote structures
     */
    std::vector<KeepNote> ParseNoteList(const std::string& json_response);

    /**
     * Parse single note from JSON response
     * @param json_response Raw JSON from GetNote
     * @return KeepNote structure
     */
    KeepNote ParseNote(const std::string& json_response);

    /**
     * Set callback for async operation completion
     */
    using Callback = std::function<void(const BridgeResult&)>;
    void SetCallback(Callback callback) { m_callback = callback; }

    /**
     * Get last error message
     */
    const std::string& GetLastError() const { return m_last_error; }

private:
    // Process handles
    HANDLE m_hChildStdInRd = nullptr;
    HANDLE m_hChildStdInWr = nullptr;
    HANDLE m_hChildStdOutRd = nullptr;
    HANDLE m_hChildStdOutWr = nullptr;
    HANDLE m_hProcess = nullptr;
    HANDLE m_hThread = nullptr;

    // State
    bool m_connected = false;
    bool m_initialized = false;
    std::wstring m_python_path;
    std::wstring m_script_path;
    std::string m_last_error;
    Callback m_callback;

    // Internal methods
    bool StartPythonProcess();
    void StopPythonProcess();
    bool SendCommand(const std::string& json_command);
    bool ReadResponse(std::string& response, DWORD timeout_ms = 30000);
    bool ExecuteCommand(const std::string& command, const std::string& params_json, 
                        BridgeResult& result);
    std::string BuildJsonCommand(const std::string& command, const std::string& params);
    std::string EscapeJsonString(const std::string& input);
};

} // namespace NppGoogleKeepSync
