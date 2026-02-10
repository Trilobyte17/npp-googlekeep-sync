// Configuration Dialog Implementation
// ARM64 Windows Compatible

// Notepad++ requires Unicode
#define UNICODE
#define _UNICODE

#include "../include/ConfigDialog.h"
#include "../include/PluginCore.h"
#include <commctrl.h>
#include <string>

#pragma comment(lib, "comctl32.lib")

ConfigDialog::ConfigDialog(HINSTANCE hInstance, HWND hwndParent, PluginConfig& config)
    : m_hInstance(hInstance), m_hwndParent(hwndParent), m_hwndDialog(NULL), m_config(config) {
}

BOOL ConfigDialog::Show() {
    m_tempExclusions = m_config.excludedExtensions;
    
    INT_PTR result = DialogBoxParamW(m_hInstance, MAKEINTRESOURCEW(IDD_CONFIG_DIALOG),
                                      m_hwndParent, DialogProc, (LPARAM)this);
    return (result == IDOK);
}

void ConfigDialog::UpdateStatus(const std::wstring& status) {
    if (m_hwndDialog) {
        SetDlgItemTextW(m_hwndDialog, IDC_STATUS_TEXT, status.c_str());
    }
}

void ConfigDialog::EnableControls(BOOL enable) {
    if (m_hwndDialog) {
        EnableWindow(GetDlgItem(m_hwndDialog, IDC_EDIT_CLIENT_ID), enable);
        EnableWindow(GetDlgItem(m_hwndDialog, IDC_EDIT_CLIENT_SECRET), enable);
        EnableWindow(GetDlgItem(m_hwndDialog, IDC_BUTTON_AUTHENTICATE), enable);
    }
}

INT_PTR CALLBACK ConfigDialog::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ConfigDialog* pDlg = NULL;
    
    if (uMsg == WM_INITDIALOG) {
        pDlg = reinterpret_cast<ConfigDialog*>(lParam);
        SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
        return pDlg->OnInitDialog(hwndDlg);
    } else {
        pDlg = reinterpret_cast<ConfigDialog*>(GetWindowLongPtr(hwndDlg, DWLP_USER));
    }
    
    if (pDlg) {
        switch (uMsg) {
            case WM_COMMAND:
                return pDlg->OnCommand(hwndDlg, LOWORD(wParam), HIWORD(wParam));
            case WM_CLOSE:
                return pDlg->OnClose(hwndDlg);
        }
    }
    
    return FALSE;
}

INT_PTR ConfigDialog::OnInitDialog(HWND hwndDlg) {
    m_hwndDialog = hwndDlg;
    
    // Set window icon
    HICON hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(100));
    if (hIcon) {
        SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }
    
    // Load current values
    LoadValues();
    
    // Set dialog title
    SetWindowTextW(hwndDlg, L"Google Keep Sync - Configuration");
    
    // Center dialog on parent
    RECT rcParent, rcDlg;
    GetWindowRect(m_hwndParent, &rcParent);
    GetWindowRect(hwndDlg, &rcDlg);
    
    int x = rcParent.left + ((rcParent.right - rcParent.left) - (rcDlg.right - rcDlg.left)) / 2;
    int y = rcParent.top + ((rcParent.bottom - rcParent.top) - (rcDlg.bottom - rcDlg.top)) / 2;
    
    SetWindowPos(hwndDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    
    return TRUE;
}

INT_PTR ConfigDialog::OnCommand(HWND hwndDlg, WORD id, WORD notify) {
    switch (id) {
        case IDOK:
            SaveValues();
        case IDCANCEL:
            EndDialog(hwndDlg, id);
            return TRUE;
            
        case IDC_BUTTON_AUTHENTICATE:
            OnAuthenticate();
            return TRUE;
            
        case IDC_BUTTON_ADD_EXCL:
            OnAddExclusion();
            return TRUE;
            
        case IDC_BUTTON_REMOVE_EXCL:
            OnRemoveExclusion();
            return TRUE;
            
        case IDC_CHECK_AUTOSYNC:
            if (notify == BN_CLICKED) {
                // Update state
            }
            return TRUE;
    }
    
    return FALSE;
}

INT_PTR ConfigDialog::OnClose(HWND hwndDlg) {
    EndDialog(hwndDlg, IDCANCEL);
    return TRUE;
}

void ConfigDialog::LoadValues() {
    // Client ID
    SetDlgItemTextW(m_hwndDialog, IDC_EDIT_CLIENT_ID, m_config.clientId.c_str());
    
    // Client Secret
    SetDlgItemTextW(m_hwndDialog, IDC_EDIT_CLIENT_SECRET, m_config.clientSecret.c_str());
    
    // Auto-sync
    CheckDlgButton(m_hwndDialog, IDC_CHECK_AUTOSYNC, 
                   m_config.autoSyncEnabled ? BST_CHECKED : BST_UNCHECKED);
    
    // Sync metadata
    CheckDlgButton(m_hwndDialog, IDC_CHECK_SYNC_METADATA,
                   m_config.syncFileMetadata ? BST_CHECKED : BST_UNCHECKED);
    
    // Load exclusions
    HWND hList = GetDlgItem(m_hwndDialog, IDC_LIST_EXCLUSIONS);
    for (const auto& ext : m_tempExclusions) {
        SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)ext.c_str());
    }
}

void ConfigDialog::SaveValues() {
    // Client ID
    wchar_t buffer[1024];
    GetDlgItemTextW(m_hwndDialog, IDC_EDIT_CLIENT_ID, buffer, 1024);
    m_config.clientId = buffer;
    
    // Client Secret
    GetDlgItemTextW(m_hwndDialog, IDC_EDIT_CLIENT_SECRET, buffer, 1024);
    m_config.clientSecret = buffer;
    
    // Auto-sync
    m_config.autoSyncEnabled = IsDlgButtonChecked(m_hwndDialog, IDC_CHECK_AUTOSYNC) == BST_CHECKED;
    
    // Sync metadata
    m_config.syncFileMetadata = IsDlgButtonChecked(m_hwndDialog, IDC_CHECK_SYNC_METADATA) == BST_CHECKED;
    
    // Exclusions
    m_config.excludedExtensions = m_tempExclusions;
    
    // Save to INI
    GoogleKeepSyncPlugin::Instance().SaveConfig();
}

void ConfigDialog::OnAuthenticate() {
    // Get current values first
    wchar_t buffer[1024];
    GetDlgItemTextW(m_hwndDialog, IDC_EDIT_CLIENT_ID, buffer, 1024);
    std::wstring email = buffer;
    
    GetDlgItemTextW(m_hwndDialog, IDC_EDIT_CLIENT_SECRET, buffer, 1024);
    std::wstring appPassword = buffer;
    
    if (email.empty() || appPassword.empty()) {
        MessageBoxW(m_hwndDialog, 
                    L"Please enter both Email and App Password first.\n\n"
                    L"Generate an App Password at: https://myaccount.google.com/apppasswords",
                    L"Login Error", 
                    MB_OK | MB_ICONWARNING);
        return;
    }
    
    UpdateStatus(L"Logging in to Google Keep...");
    EnableControls(FALSE);
    
    // Use PythonBridge to login
    GoogleKeepSyncPlugin& plugin = GoogleKeepSyncPlugin::Instance();
    auto result = plugin.Login(email, appPassword);
    
    if (result.success) {
        UpdateStatus(L"Login successful!");
        EnableControls(TRUE);
    } else {
        std::wstring errorMsg = L"Login failed: " + result.error_message;
        UpdateStatus(L"Login failed!");
        MessageBoxW(m_hwndDialog, errorMsg.c_str(), L"Login Failed", MB_OK | MB_ICONERROR);
        EnableControls(TRUE);
    }
}

void ConfigDialog::OnAddExclusion() {
    wchar_t buffer[256];
    GetDlgItemTextW(m_hwndDialog, IDC_EDIT_EXCLUSION, buffer, 256);
    
    std::wstring ext(buffer);
    if (!ext.empty()) {
        // Remove leading dot if present
        if (ext[0] == L'.') {
            ext = ext.substr(1);
        }
        
        m_tempExclusions.push_back(ext);
        
        HWND hList = GetDlgItem(m_hwndDialog, IDC_LIST_EXCLUSIONS);
        SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)ext.c_str());
        
        SetDlgItemTextW(m_hwndDialog, IDC_EDIT_EXCLUSION, L"");
    }
}

void ConfigDialog::OnRemoveExclusion() {
    HWND hList = GetDlgItem(m_hwndDialog, IDC_LIST_EXCLUSIONS);
    int sel = (int)SendMessageW(hList, LB_GETCURSEL, 0, 0);
    
    if (sel != LB_ERR) {
        SendMessageW(hList, LB_DELETESTRING, sel, 0);
        m_tempExclusions.erase(m_tempExclusions.begin() + sel);
    }
}

// Helper functions
std::wstring GetDlgItemText(HWND hwndDlg, int nIDDlgItem) {
    wchar_t buffer[1024];
    GetDlgItemTextW(hwndDlg, nIDDlgItem, buffer, 1024);
    return std::wstring(buffer);
}

void SetDlgItemText(HWND hwndDlg, int nIDDlgItem, const std::wstring& text) {
    SetDlgItemTextW(hwndDlg, nIDDlgItem, text.c_str());
}
