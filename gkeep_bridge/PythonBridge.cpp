#include "PythonBridge.h"
#include <windows.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <optional>

namespace NppGoogleKeepSync {

// Simple JSON parser functions
namespace {
    static std::string extractJsonValue(const std::string& json, const std::string& key) {
        size_t pos = json.find("\"" + key + "\":");
        if (pos == std::string::npos) return "";
        
        pos += key.length() + 3; // Move past key and colon
        size_t start = json.find('"', pos);
        if (start == std::string::npos) return "";
        
        start++; // Skip opening quote
        size_t end = json.find('"', start);
        if (end == std::string::npos) return "";
        
        return json.substr(start, end - start);
    }
    
    static bool extractJsonBool(const std::string& json, const std::string& key) {
        size_t pos = json.find("\"" + key + "\":");
        if (pos == std::string::npos) return false;
        
        pos += key.length() + 3;
        size_t start = json.find_first_of("tf", pos);
        if (start == std::string::npos) return false;
        
        return json.substr(start, 4) == "true";
    }
}

PythonBridge::PythonBridge()
    : m_hChildStdInRd(nullptr)
    , m_hChildStdInWr(nullptr)
    , m_hChildStdOutRd(nullptr)
    , m_hChildStdOutWr(nullptr)
    , m_hProcess(nullptr)
    , m_hThread(nullptr)
    , m_connected(false)
    , m_initialized(false)
{
}

PythonBridge::~PythonBridge()
{
    Shutdown();
}

PythonBridge::PythonBridge(PythonBridge&& other) noexcept
{
    *this = std::move(other);
}

PythonBridge& PythonBridge::operator=(PythonBridge&& other) noexcept
{
    if (this != &other) {
        Shutdown();
        
        m_hChildStdInRd = other.m_hChildStdInRd;
        m_hChildStdInWr = other.m_hChildStdInWr;
        m_hChildStdOutRd = other.m_hChildStdOutRd;
        m_hChildStdOutWr = other.m_hChildStdOutWr;
        m_hProcess = other.m_hProcess;
        m_hThread = other.m_hThread;
        m_connected = other.m_connected;
        m_initialized = other.m_initialized;
        m_python_path = std::move(other.m_python_path);
        m_script_path = std::move(other.m_script_path);
        m_last_error = std::move(other.m_last_error);
        m_callback = std::move(other.m_callback);
        
        other.m_hChildStdInRd = nullptr;
        other.m_hChildStdInWr = nullptr;
        other.m_hChildStdOutRd = nullptr;
        other.m_hChildStdOutWr = nullptr;
        other.m_hProcess = nullptr;
        other.m_hThread = nullptr;
        other.m_connected = false;
        other.m_initialized = false;
    }
    return *this;
}

bool PythonBridge::Initialize(const std::wstring& python_path, const std::wstring& script_path)
{
    if (m_initialized) {
        return true;
    }

    m_python_path = python_path.empty() ? L"python" : python_path;
    m_script_path = script_path;

    if (!StartPythonProcess()) {
        m_last_error = "Failed to start Python process";
        return false;
    }

    m_initialized = true;
    return true;
}

void PythonBridge::Shutdown()
{
    if (m_initialized) {
        StopPythonProcess();
        m_initialized = false;
        m_connected = false;
    }
}

bool PythonBridge::StartPythonProcess()
{
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    // Create pipes for stdin
    if (!CreatePipe(&m_hChildStdInRd, &m_hChildStdInWr, &sa, 0)) {
        m_last_error = "Failed to create stdin pipe";
        return false;
    }
    SetHandleInformation(m_hChildStdInWr, HANDLE_FLAG_INHERIT, 0);

    // Create pipes for stdout
    if (!CreatePipe(&m_hChildStdOutRd, &m_hChildStdOutWr, &sa, 0)) {
        m_last_error = "Failed to create stdout pipe";
        CloseHandle(m_hChildStdInRd);
        CloseHandle(m_hChildStdInWr);
        return false;
    }
    SetHandleInformation(m_hChildStdOutRd, HANDLE_FLAG_INHERIT, 0);

    // Build command line
    std::wstring cmdLine = L"\"" + m_python_path + L"\" \"" + m_script_path + L"\"";
    
    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdInput = m_hChildStdInRd;
    si.hStdOutput = m_hChildStdOutWr;
    si.hStdError = m_hChildStdOutWr;
    si.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessW(nullptr, &cmdLine[0], nullptr, nullptr, TRUE, 
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        DWORD dwErr = GetLastError();
        m_last_error = "Failed to create Python process. Error: " + std::to_string(static_cast<unsigned long>(dwErr));
        CloseHandle(m_hChildStdInRd);
        CloseHandle(m_hChildStdInWr);
        CloseHandle(m_hChildStdOutRd);
        CloseHandle(m_hChildStdOutWr);
        return false;
    }

    m_hProcess = pi.hProcess;
    m_hThread = pi.hThread;
    
    // Close handles we don't need in parent
    CloseHandle(m_hChildStdInRd);
    CloseHandle(m_hChildStdOutWr);

    m_connected = true;
    
    // Wait a moment for Python to initialize
    Sleep(500);
    
    return true;
}

void PythonBridge::StopPythonProcess()
{
    if (m_hProcess) {
        // Send exit command to gracefully shutdown Python
        if (m_connected) {
            std::string exit_cmd = "{\"command\":\"exit\"}\n";
            DWORD written;
            WriteFile(m_hChildStdInWr, exit_cmd.c_str(), static_cast<DWORD>(exit_cmd.length()), &written, nullptr);
            Sleep(100);
        }

        // Terminate if still running
        TerminateProcess(m_hProcess, 0);
        
        CloseHandle(m_hProcess);
        CloseHandle(m_hThread);
        m_hProcess = nullptr;
        m_hThread = nullptr;
    }

    if (m_hChildStdInWr) {
        CloseHandle(m_hChildStdInWr);
        m_hChildStdInWr = nullptr;
    }
    if (m_hChildStdOutRd) {
        CloseHandle(m_hChildStdOutRd);
        m_hChildStdOutRd = nullptr;
    }

    m_connected = false;
}

bool PythonBridge::SendCommand(const std::string& json_command)
{
    if (!m_connected || !m_hChildStdInWr) {
        m_last_error = "Not connected to Python process";
        return false;
    }

    DWORD written;
    BOOL success = WriteFile(m_hChildStdInWr, json_command.c_str(), 
                             static_cast<DWORD>(json_command.length()), &written, nullptr);
    
    if (!success || written != json_command.length()) {
        m_last_error = "Failed to write to Python process";
        return false;
    }

    return true;
}

bool PythonBridge::ReadResponse(std::string& response, DWORD timeout_ms)
{
    if (!m_connected || !m_hChildStdOutRd) {
        m_last_error = "Not connected to Python process";
        return false;
    }

    char buffer[4096];
    DWORD bytesRead;
    response.clear();
    
    DWORD startTime = GetTickCount();
    
    while (true) {
        DWORD available = 0;
        if (!PeekNamedPipe(m_hChildStdOutRd, nullptr, 0, nullptr, &available, nullptr)) {
            break;
        }
        
        if (available > 0) {
            if (ReadFile(m_hChildStdOutRd, buffer, sizeof(buffer) - 1, &bytesRead, nullptr)) {
                buffer[bytesRead] = '\0';
                response += buffer;
                
                // Check if we have a complete JSON (contains newline)
                if (response.find('\n') != std::string::npos) {
                    break;
                }
            } else {
                break;
            }
        } else {
            // Check timeout
            if (GetTickCount() - startTime > timeout_ms) {
                m_last_error = "Timeout waiting for Python response";
                return false;
            }
            Sleep(10);
        }
        
        // Check if process is still alive
        DWORD exitCode;
        if (GetExitCodeProcess(m_hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
            m_last_error = "Python process exited unexpectedly";
            m_connected = false;
            return false;
        }
    }

    return !response.empty();
}

std::string PythonBridge::BuildJsonCommand(const std::string& command, const std::string& params)
{
    std::ostringstream oss;
    oss << "{\"command\":\"" << command << "\",\"params\":";
    if (params.empty()) {
        oss << "{}";
    } else {
        oss << params;
    }
    oss << "}\n";
    return oss.str();
}

std::string PythonBridge::EscapeJsonString(const std::string& input)
{
    std::ostringstream oss;
    for (char c : input) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                if (c >= 0x20 && c <= 0x7E) {
                    oss << c;
                } else {
                    oss << "\\u" << std::hex << std::setfill('0') << std::setw(4) << (static_cast<int>(c) & 0xFF);
                }
        }
    }
    return oss.str();
}

bool PythonBridge::ExecuteCommand(const std::string& command, const std::string& params_json, 
                                  BridgeResult& result)
{
    std::string json_cmd = BuildJsonCommand(command, params_json);
    
    if (!SendCommand(json_cmd)) {
        result.success = false;
        result.error_message = m_last_error;
        return false;
    }

    std::string response;
    if (!ReadResponse(response)) {
        result.success = false;
        result.error_message = m_last_error;
        return false;
    }

    result.raw_json = response;
    
    // Parse response using simple JSON parsing
    result.success = extractJsonBool(response, "success");
    if (!result.success) {
        result.error_message = extractJsonValue(response, "error");
    }

    return result.success;
}

// Public API methods

BridgeResult PythonBridge::Login(const std::string& email, const std::string& app_password)
{
    BridgeResult result;
    
    std::ostringstream params;
    params << "{\"email\":\"" << EscapeJsonString(email) << "\","
           << "\"app_password\":\"" << EscapeJsonString(app_password) << "\"}";
    
    if (ExecuteCommand("login", params.str(), result) && result.success) {
        // Parse email from response for confirmation
        // Login successful
    }
    
    return result;
}

BridgeResult PythonBridge::CreateNote(const std::string& title, const std::string& text,
                                     bool pinned, const std::string& color,
                                     const std::vector<std::string>& labels)
{
    BridgeResult result;
    
    std::ostringstream params;
    params << "{\"title\":\"" << EscapeJsonString(title) << "\","
           << "\"text\":\"" << EscapeJsonString(text) << "\","
           << "\"pinned\":" << (pinned ? "true" : "false") << ","
           << "\"color\":\"" << EscapeJsonString(color) << "\"";
           
    if (!labels.empty()) {
        params << ",\"labels\":[";
        for (size_t i = 0; i < labels.size(); ++i) {
            if (i > 0) params << ",";
            params << "\"" << EscapeJsonString(labels[i]) << "\"";
        }
        params << "]";
    }
    params << "}";
    
    ExecuteCommand("create_note", params.str(), result);
    return result;
}

BridgeResult PythonBridge::UpdateNote(const std::string& note_id,
                                     const std::optional<std::string>& title,
                                     const std::optional<std::string>& text,
                                     const std::optional<bool>& pinned,
                                     const std::optional<std::string>& color,
                                     const std::optional<std::vector<std::string>>& labels)
{
    BridgeResult result;
    
    std::ostringstream params;
    params << "{\"id\":\"" << EscapeJsonString(note_id) << "\"";
    
    if (title.has_value()) {
        params << ",\"title\":\"" << EscapeJsonString(title.value()) << "\"";
    }
    if (text.has_value()) {
        params << ",\"text\":\"" << EscapeJsonString(text.value()) << "\"";
    }
    if (pinned.has_value()) {
        params << ",\"pinned\":" << (pinned.value() ? "true" : "false");
    }
    if (color.has_value()) {
        params << ",\"color\":\"" << EscapeJsonString(color.value()) << "\"";
    }
    if (labels.has_value()) {
        params << ",\"labels\":[";
        const auto& label_list = labels.value();
        for (size_t i = 0; i < label_list.size(); ++i) {
            if (i > 0) params << ",";
            params << "\"" << EscapeJsonString(label_list[i]) << "\"";
        }
        params << "]";
    }
    params << "}";
    
    ExecuteCommand("update_note", params.str(), result);
    return result;
}

BridgeResult PythonBridge::GetStatus()
{
    BridgeResult result;
    ExecuteCommand("status", "", result);
    return result;
}

void PythonBridge::Logout()
{
    // Clear any cached state
    m_last_error.clear();
    // Note: Actual credential clearing is handled by Python side
    // by removing the auth.json file
}

BridgeResult PythonBridge::Sync()
{
    BridgeResult result;
    ExecuteCommand("sync", "", result);
    return result;
}

BridgeResult PythonBridge::ListNotes(bool all, int limit, 
                                      const std::string& query,
                                      const std::vector<std::string>& labels)
{
    BridgeResult result;
    
    std::ostringstream params;
    params << "{\"all\":" << (all ? "true" : "false") << ","
           << "\"limit\":" << limit;
    
    if (!query.empty()) {
        params << ",\"query\":\"" << EscapeJsonString(query) << "\"";
    }
    
    if (!labels.empty()) {
        params << ",\"labels\":[";
        for (size_t i = 0; i < labels.size(); ++i) {
            if (i > 0) params << ",";
            params << "\"" << EscapeJsonString(labels[i]) << "\"";
        }
        params << "]";
    }
    
    params << "}";
    
    ExecuteCommand("list", params.str(), result);
    return result;
}

BridgeResult PythonBridge::GetNote(const std::string& note_id)
{
    BridgeResult result;
    
    std::ostringstream params;
    params << "{\"id\":\"" << EscapeJsonString(note_id) << "\"}";
    
    ExecuteCommand("get", params.str(), result);
    return result;
}

BridgeResult PythonBridge::DeleteNote(const std::string& note_id, bool permanent)
{
    BridgeResult result;
    
    std::ostringstream params;
    params << "{\"id\":\"" << EscapeJsonString(note_id) << "\","
           << "\"permanent\":" << (permanent ? "true" : "false") << "}";
    
    ExecuteCommand("delete", params.str(), result);
    return result;
}

std::vector<KeepNote> PythonBridge::ParseNoteList(const std::string& json_response)
{
    std::vector<KeepNote> notes;
    size_t array_start = json_response.find("\"notes\":[");
    if (array_start == std::string::npos) return notes;
    size_t obj_start = json_response.find('{', array_start);
    if (obj_start == std::string::npos) return notes;
    int depth = 0;
    size_t search_pos = obj_start;
    while (search_pos < json_response.length()) {
        char c = json_response[search_pos];
        if (c == '{') {
            if (depth == 0) obj_start = search_pos;
            depth++;
        } else if (c == '}') {
            depth--;
            if (depth == 0) {
                std::string note_json = json_response.substr(obj_start, search_pos - obj_start + 1);
                notes.push_back(ParseNote(note_json));
            }
        }
        search_pos++;
        if (notes.size() >= 1000) break;
    }
    return notes;
}

KeepNote PythonBridge::ParseNote(const std::string& json_response)
{
    KeepNote note;
    
    // Simple parsing for single note
    note.id = extractJsonValue(json_response, "id");
    note.title = extractJsonValue(json_response, "title");
    note.text = extractJsonValue(json_response, "text");
    
    // Parse boolean values
    std::string pinned_str = extractJsonValue(json_response, "pinned");
    note.pinned = (pinned_str == "true");
    
    std::string archived_str = extractJsonValue(json_response, "archived");
    note.archived = (archived_str == "true");
    
    note.color = extractJsonValue(json_response, "color");
    note.created_timestamp = extractJsonValue(json_response, "timestamp");
    
    // Parse labels array - simplified
    size_t labels_start = json_response.find("\"labels\":[");
    if (labels_start != std::string::npos) {
        labels_start += 10; // Skip "\"labels\":["
        size_t labels_end = json_response.find("]", labels_start);
        if (labels_end != std::string::npos) {
            std::string labels_str = json_response.substr(labels_start, labels_end - labels_start);
            size_t start = 0;
            while (true) {
                size_t quote1 = labels_str.find('"', start);
                if (quote1 == std::string::npos) break;
                size_t quote2 = labels_str.find('"', quote1 + 1);
                if (quote2 == std::string::npos) break;
                note.labels.push_back(labels_str.substr(quote1 + 1, quote2 - quote1 - 1));
                start = quote2 + 1;
            }
        }
    }
    
    return note;
}

} // namespace NppGoogleKeepSync