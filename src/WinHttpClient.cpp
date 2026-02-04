// Windows HTTP Client Implementation for ARM64
// Uses WinHTTP API for secure, native HTTP requests

#include "../include/GoogleKeepAPI.h"
#include <winhttp.h>
#include <string>
#include <vector>
#include <sstream>

#pragma comment(lib, "winhttp.lib")

class WinHttpClient : public IHttpClient {
public:
    WinHttpClient() : m_hSession(NULL) {}
    
    BOOL Initialize() override {
        // Use WinHTTP session for ARM64 Windows
        // WinHTTP is preferred over WinINET for server-side and automated scenarios
        m_hSession = WinHttpOpen(
            L"NppGoogleKeepSync/1.0 (ARM64; Windows)",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            WINHTTP_FLAG_ASYNC
        );
        return m_hSession != NULL;
    }
    
    void Shutdown() override {
        if (m_hSession) {
            WinHttpCloseHandle(m_hSession);
            m_hSession = NULL;
        }
    }
    
    std::string Post(const std::wstring& url, const std::string& body,
                     const std::vector<std::pair<std::wstring, std::wstring>>& headers) override {
        return SendRequest(L"POST", url, body, headers);
    }
    
    std::string Get(const std::wstring& url,
                    const std::vector<std::pair<std::wstring, std::wstring>>& headers) override {
        return SendRequest(L"GET", url, "", headers);
    }
    
    std::string Patch(const std::wstring& url, const std::string& body,
                      const std::vector<std::pair<std::wstring, std::wstring>>& headers) override {
        // PATCH requires manual method override via X-HTTP-Method-Override header for some servers
        // Or use custom verbs in WinHTTP
        return SendRequest(L"PATCH", url, body, headers);
    }
    
private:
    HINTERNET m_hSession;
    
    std::string SendRequest(const std::wstring& method, const std::wstring& url,
                            const std::string& body,
                            const std::vector<std::pair<std::wstring, std::wstring>>& headers) {
        URL_COMPONENTS urlComp = {0};
        urlComp.dwStructSize = sizeof(URL_COMPONENTS);
        
        wchar_t hostName[256] = {0};
        wchar_t urlPath[2048] = {0};
        
        urlComp.lpszHostName = hostName;
        urlComp.dwHostNameLength = 256;
        urlComp.lpszUrlPath = urlPath;
        urlComp.dwUrlPathLength = 2048;
        
        if (!WinHttpCrackUrl(url.c_str(), 0, 0, &urlComp)) {
            return "";
        }
        
        // Convert port
        INTERNET_PORT port = urlComp.nPort;
        if (port == 0) {
            port = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? INTERNET_DEFAULT_HTTPS_PORT 
                                                               : INTERNET_DEFAULT_HTTP_PORT;
        }
        
        // Connect to server
        HINTERNET hConnect = WinHttpConnect(m_hSession, hostName, port, 0);
        if (!hConnect) {
            return "";
        }
        
        // Create request
        DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect,
            method.c_str(),
            urlPath,
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            flags
        );
        
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            return "";
        }
        
        // Add headers
        for (const auto& header : headers) {
            std::wstring headerLine = header.first + L": " + header.second;
            WinHttpAddRequestHeaders(hRequest, headerLine.c_str(), (ULONG)-1L,
                                     WINHTTP_ADDREQ_FLAG_ADD);
        }
        
        // Default content-type for JSON
        if (method == L"POST" || method == L"PATCH") {
            WinHttpAddRequestHeaders(hRequest, L"Content-Type: application/json", (ULONG)-1L,
                                     WINHTTP_ADDREQ_FLAG_ADD);
        }
        
        BOOL result = WinHttpSendRequest(
            hRequest,
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0,
            (LPVOID)body.c_str(),
            (DWORD)body.length(),
            (DWORD)body.length(),
            0
        );
        
        if (!result) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            return "";
        }
        
        result = WinHttpReceiveResponse(hRequest, NULL);
        if (!result) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            return "";
        }
        
        // Read response
        std::string response;
        DWORD dwSize = 0;
        DWORD dwDownloaded = 0;
        
        do {
            dwSize = 0;
            WinHttpQueryDataAvailable(hRequest, &dwSize);
            
            if (dwSize > 0) {
                std::vector<char> buffer(dwSize + 1);
                WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded);
                buffer[dwDownloaded] = '\0';
                response.append(buffer.data());
            }
        } while (dwSize > 0);
        
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        
        return response;
    }
};

// Factory function
std::unique_ptr<IHttpClient> CreateHttpClient() {
    return std::make_unique<WinHttpClient>();
}
