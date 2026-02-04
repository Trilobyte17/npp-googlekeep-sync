// Google Keep REST API Integration
// ARM64 Windows Compatible

#pragma once

#include "PluginInterface.h"
#include <memory>
#include <functional>

// HTTP client for REST API calls
class IHttpClient {
public:
    virtual ~IHttpClient() = default;
    virtual BOOL Initialize() = 0;
    virtual std::string Post(const std::wstring& url, const std::string& body, 
                              const std::vector<std::pair<std::wstring, std::wstring>>& headers) = 0;
    virtual std::string Get(const std::wstring& url,
                             const std::vector<std::pair<std::wstring, std::wstring>>& headers) = 0;
    virtual std::string Patch(const std::wstring& url, const std::string& body,
                               const std::vector<std::pair<std::wstring, std::wstring>>& headers) = 0;
    virtual void Shutdown() = 0;
};

// OAuth 2.0 Authentication Manager
class OAuthManager {
public:
    OAuthManager(const std::wstring& clientId, const std::wstring& clientSecret);
    
    BOOL InitializeLocalServer();  // Launches localhost listener for OAuth callback
    std::wstring GetAuthorizationUrl();
    BOOL ExchangeCodeForToken(const std::wstring& authCode, std::wstring& outAccessToken, 
                               std::wstring& outRefreshToken);
    BOOL RefreshAccessToken(std::wstring& outAccessToken);
    
    static BOOL GeneratePKCEChallenge(std::wstring& outVerifier, std::wstring& outChallenge);
    
private:
    std::wstring m_clientId;
    std::wstring m_clientSecret;
    std::wstring m_codeVerifier;
    std::wstring m_codeChallenge;
    std::wstring m_tokenEndpoint;
};

// Google Keep API Client
class GoogleKeepClient {
public:
    GoogleKeepClient();
    ~GoogleKeepClient();
    
    BOOL Initialize(const std::wstring& accessToken);
    void SetAccessToken(const std::wstring& accessToken);
    
    // Note operations
    struct KeepNote {
        std::wstring id;
        std::wstring title;
        std::wstring content;
        std::vector<std::wstring> labels;
        std::wstring createdTime;
        std::wstring modifiedTime;
        BOOL trashed;
    };
    
    BOOL CreateNote(const std::wstring& title, const std::wstring& content,
                    const std::vector<std::wstring>& labels, KeepNote& outNote);
    BOOL UpdateNote(const std::wstring& noteId, const std::wstring& title,
                    const std::wstring& content, KeepNote& outNote);
    BOOL GetNote(const std::wstring& noteId, KeepNote& outNote);
    BOOL ListNotes(std::vector<KeepNote>& outNotes, const std::wstring& filter = L"");
    BOOL DeleteNote(const std::wstring& noteId);
    
    // OAuth integration
    BOOL Authenticate(const std::wstring& clientId, const std::wstring& clientSecret,
                      HWND hwndParent);
    BOOL IsAuthenticated() const;
    std::wstring GetRefreshToken() const;
    
private:
    std::unique_ptr<IHttpClient> m_httpClient;
    std::wstring m_accessToken;
    std::unique_ptr<OAuthManager> m_oAuthManager;
    
    std::wstring EscapeJsonString(const std::wstring& input);
    std::string BuildCreateNoteRequest(const std::wstring& title, const std::wstring& content,
                                        const std::vector<std::wstring>& labels);
    std::string BuildUpdateNoteRequest(const std::wstring& title, const std::wstring& content);
};

// Callback for async HTTP operations
using HttpCompletionCallback = std::function<void(const std::string& response, BOOL success)>;
