// Configuration Dialog Header
// ARM64 Windows Compatible

#pragma once

#include <windows.h>
#include "PluginInterface.h"

// Dialog resource IDs
#define IDD_CONFIG_DIALOG       100
#define IDC_EDIT_CLIENT_ID      101
#define IDC_EDIT_CLIENT_SECRET  102
#defineIDC_CHECK_AUTOSYNC  103
#define IDC_CHECK_SYNC_METADATA 104
#define IDC_BUTTON_AUTHENTICATE 105
#define IDC_STATUS_TEXT         106
#define IDC_LIST_EXCLUSIONS     107
#define IDC_BUTTON_ADD_EXCL     108
#define IDC_BUTTON_REMOVE_EXCL  109
#define IDC_EDIT_EXCLUSION      110

class ConfigDialog {
public:
    ConfigDialog(HINSTANCE hInstance, HWND hwndParent, PluginConfig& config);
    
    BOOL Show();
    void UpdateStatus(const std::wstring& status);
    void EnableControls(BOOL enable);
    
private:
    static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR OnInitDialog(HWND hwndDlg);
    INT_PTR OnCommand(HWND hwndDlg, WORD id, WORD notify);
    INT_PTR OnClose(HWND hwndDlg);
    
    void LoadValues();
    void SaveValues();
    void OnAuthenticate();
    void OnAddExclusion();
    void OnRemoveExclusion();
    
    HINSTANCE m_hInstance;
    HWND m_hwndParent;
    HWND m_hwndDialog;
    PluginConfig& m_config;
    
    std::vector<std::wstring> m_tempExclusions;
};

// Helper functions
std::wstring GetDlgItemText(HWND hwndDlg, int nIDDlgItem);
void SetDlgItemText(HWND hwndDlg, int nIDDlgItem, const std::wstring& text);
