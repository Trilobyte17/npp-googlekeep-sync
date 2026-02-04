// Google Keep API Client Implementation

#include "../include/GoogleKeepAPI.h"
#include "../include/PluginInterface.h"
#include <sstream>
#include <iomanip>

// JSON helper functions (in production, use a proper JSON library like nlohmann/json)
std::wstring GoogleKeepClient::EscapeJsonString(const std::wstring& input) {
    std::wstring output;
    for (wchar_t c : input) {
        switch (c) {
            case L'\\': output += L"\\\\"; break;
            case L'\"': output += L"\\\""; break;
            case L'\b': output += L"\\b"; break;
            case L'\f': output += L"\\f"; break;
            case L'\n': output += L"\\n"; break;
            case L'\r': output += L"\\r"; break;
            case L'\t': output += L"\\t"; break;
            default:
                if (c < 0x20) {
                    std::wostringstream oss;
                    oss << L"\\u" << std::hex << std::setw(4) << std::setfill(L'0') << (int)c;
                    output += oss.str();
                } else {
                    output += c;
                }
        }
    }
    return output;
}

std::string GoogleKeepClient::BuildCreateNoteRequest(const std::wstring& title,
                                                      const std::wstring& content,
                                                      const std::vector<std::wstring>& labels) {
    std::wostringstream json;
    json << L"{";
    json << L"\"title\":\"" << EscapeJsonString(title) << L"\",";
    json << L"\"body\":[{":" << L"\"text\":\"" << EscapeJsonString(content) << L"\"}";
    json << L"]}";
    
    // Add labels if any
    if (!labels.empty()) {
        json << L"," << L"\"labels\":[";
        for (size_t i = 0; i < labels.size(); i++) {
            if (i > 0) json << L",";
            json << L"{\"name\":\"" << EscapeJsonString(labels[i]) << L"\"}";
        }
        json << L"]";
    }
    
    json << L"}";
    
    std::wstring ws = json.str();
    return std::string(ws.begin(), ws.end());
}

std::string GoogleKeepClient::BuildUpdateNoteRequest(const std::wstring& title,
                                                      const std::wstring& content) {
    // Build update mask and fields
    std::wostringstream json;
    json << L"{";
    json << L"\"title\":\"" << EscapeJsonString(title) << L"\",";
    json << L"\"body\":[{":" << L"\"text\":\"" << EscapeJsonString(content) << L"\"}";
    json << L"]}";
    json << L"}";
    
    std::wstring ws = json.str();
    return std::string(ws.begin(), ws.end());
}

GoogleKeepClient::GoogleKeepClient() : m_httpClient(nullptr) {}

GoogleKeepClient::~GoogleKeepClient() {
    if (m_httpClient) {
        m_httpClient->Shutdown();
    }
}

BOOL GoogleKeepClient::Initialize(const std::wstring& accessToken) {
    // Create HTTP client
    // Note: In full implementation, this would use the factory function
    // from WinHttpClient.cpp
    m_accessToken = accessToken;
    return TRUE;
}

void GoogleKeepClient::SetAccessToken(const std::wstring& accessToken) {
    m_accessToken = accessToken;
}

BOOL GoogleKeepClient::CreateNote(const std::wstring& title, const std::wstring& content,
                                   const std::vector<std::wstring>& labels, KeepNote& outNote) {
    if (m_accessToken.empty()) return FALSE;
    
    std::wstring url = KEEP_API_BASE_URL;
    url += L"notes/notes";
    
    std::string body = BuildCreateNoteRequest(title, content, labels);
    
    std::vector<std::pair<std::wstring, std::wstring>> headers;
    headers.push_back({L"Authorization", L"Bearer " + m_accessToken});
    headers.push_back({L"Content-Type", L"application/json"});
    
    // Response would be parsed here
    // std::string response = m_httpClient->Post(url, body, headers);
    // Parse response and populate outNote
    
    // Mock successful response for now
    static int noteCounter = 1;
    std::wostringstream noteId;
    noteId << L"note_" << noteCounter++;
    outNote.id = noteId.str();
    outNote.title = title;
    outNote.content = content;
    outNote.labels = labels;
    outNote.createdTime = L"2025-01-01T00:00:00Z";
    outNote.modifiedTime = L"2025-01-01T00:00:00Z";
    outNote.trashed = FALSE;
    
    return TRUE;
}

BOOL GoogleKeepClient::UpdateNote(const std::wstring& noteId, const std::wstring& title,
                                   const std::wstring& content, KeepNote& outNote) {
    if (m_accessToken.empty()) return FALSE;
    
    std::wstring url = KEEP_API_BASE_URL;
    url += L"notes/";
    url += noteId;
    
    std::string body = BuildUpdateNoteRequest(title, content);
    
    std::vector<std::pair<std::wstring, std::wstring>> headers;
    headers.push_back({L"Authorization", L"Bearer " + m_accessToken});
    headers.push_back({L"Content-Type", L"application/json"});
    
    // Response handling
    outNote.id = noteId;
    outNote.title = title;
    outNote.content = content;
    outNote.modifiedTime = L"2025-01-01T00:00:00Z";
    
    return TRUE;
}

BOOL GoogleKeepClient::GetNote(const std::wstring& noteId, KeepNote& outNote) {
    if (m_accessToken.empty()) return FALSE;
    
    std::wstring url = KEEP_API_BASE_URL;
    url += L"notes/";
    url += noteId;
    
    std::vector<std::pair<std::wstring, std::wstring>> headers;
    headers.push_back({L"Authorization", L"Bearer " + m_accessToken});
    
    // GET request and parse
    
    outNote.id = noteId;
    return TRUE;
}

BOOL GoogleKeepClient::ListNotes(std::vector<KeepNote>& outNotes, const std::wstring& filter) {
    if (m_accessToken.empty()) return FALSE;
    
    std::wstring url = KEEP_API_BASE_URL;
    url += L"notes";
    if (!filter.empty()) {
        url += L"?filter=" + filter;
    }
    
    std::vector<std::pair<std::wstring, std::wstring>> headers;
    headers.push_back({L"Authorization", L"Bearer " + m_accessToken});
    
    // Parse list response
    outNotes.clear();
    return TRUE;
}

BOOL GoogleKeepClient::DeleteNote(const std::wstring& noteId) {
    if (m_accessToken.empty()) return FALSE;
    
    std::wstring url = KEEP_API_BASE_URL;
    url += L"notes/";
    url += noteId;
    url += L":trash";
    
    std::vector<std::pair<std::wstring, std::wstring>> headers;
    headers.push_back({L"Authorization", L"Bearer " + m_accessToken});
    
    // POST to trash endpoint
    return TRUE;
}

BOOL GoogleKeepClient::Authenticate(const std::wstring& clientId, const std::wstring& clientSecret,
                                     HWND hwndParent) {
    m_oAuthManager = std::make_unique<OAuthManager>(clientId, clientSecret);
    
    std::wstring authUrl = m_oAuthManager->GetAuthorizationUrl();
    
    // Open browser for authentication
    ShellExecuteW(hwndParent, L"open", authUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
    
    // Show dialog with instructions and local server callback handling
    // This dialog would host the HTTP server to receive the OAuth callback
    
    // For now, simplified flow - user would paste auth code
    return TRUE;
}

BOOL GoogleKeepClient::IsAuthenticated() const {
    return !m_accessToken.empty();
}

std::wstring GoogleKeepClient::GetRefreshToken() const {
    if (m_oAuthManager) {
        // Return stored refresh token
    }
    return L"";
}
