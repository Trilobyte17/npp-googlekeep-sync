// DLL Entry Point for Notepad++ Google Keep Sync Plugin
// ARM64 Windows Compatible

#include "../include/PluginCore.h"
#include "../include/PluginInterface.h"

// Notification codes (from Notepad++ SDK)
#define NPPN_FILEBEFORESAVE 2001
#define NPPN_BUFFERSAVED    2002
#define NPPN_FILEDELETED    2003
#define NPPN_FILEOPENED     2005
#define NPPN_BUFFERACTIVATED 2008
#define NPPN_FILECLOSED     2004

// Plugin handle and data
HINSTANCE g_hInstance = NULL;
HWND g_hwndNpp = NULL;
NppData g_nppData = {0};

// Function array for Notepad++
struct FuncItem {
    WCHAR _itemName[64];
    void (*_pFunc)(void*);
    INT _cmdID;
    BOOL _init2Check;
    void* _pShKey;
};

FuncItem g_funcItems[] = {
    {L"&Sync Now", [](void*) { GoogleKeepSyncPlugin::Instance().OnSyncNow(); }, 0, FALSE, NULL},
    {L"&Toggle Auto-Sync", [](void*) { GoogleKeepSyncPlugin::Instance().OnToggleAutoSync(); }, 0, FALSE, NULL},
    {L"&Configure...", [](void*) { GoogleKeepSyncPlugin::Instance().OnConfigure(); }, 0, FALSE, NULL},
    {L"&About", [](void*) { GoogleKeepSyncPlugin::Instance().OnAbout(); }, 0, FALSE, NULL}
};

const INT NB_FUNC = sizeof(g_funcItems) / sizeof(FuncItem);

// DLL entry point
BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD reason, LPVOID lpReserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            g_hInstance = hInstance;
            DisableThreadLibraryCalls(hInstance);
            break;
            
        case DLL_PROCESS_DETACH:
            GoogleKeepSyncPlugin::Instance().Terminate();
            break;
    }
    return TRUE;
}

// Notepad++ Plugin Interface Functions - exported with extern "C" and __declspec(dllexport)
extern "C" {

__declspec(dllexport) BOOL APIENTRY isUnicode() {
    return TRUE;
}

__declspec(dllexport) CONST WCHAR* APIENTRY getName() {
    return GoogleKeepSyncPlugin::Instance().GetName();
}

__declspec(dllexport) VOID APIENTRY setInfo(NppData notepadPlusData) {
    g_nppData = notepadPlusData;
    g_hwndNpp = notepadPlusData._nppHandle;
    GoogleKeepSyncPlugin::Instance().Init(g_hInstance, g_hwndNpp);
}

__declspec(dllexport) CONST WCHAR* APIENTRY getFuncsArray(INT* nbF) {
    *nbF = NB_FUNC;
    return (WCHAR*)g_funcItems;
}

__declspec(dllexport) BOOL APIENTRY messageProc(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_NOTIFY:
            if (lParam) {
                SCNotification* notify = (SCNotification*)lParam;
                beNotificationProc(notify);
            }
            break;
    }
    return TRUE;
}

__declspec(dllexport) LONGLONG APIENTRY beNotificationProc(SCNotification* notifyCode) {
    if (!notifyCode) return 0;
    
    // Handle Notepad++ Notifications
    if (notifyCode->nmhdr.code >= NPPN_FILEBEFORESAVE && 
        notifyCode->nmhdr.code <= NPPN_FILECLOSED + 100) {
        // Get file path from Notepad++ via message
        wchar_t filePath[MAX_PATH] = {0};
        ::SendMessage(g_hwndNpp, NPPM_GETFULLCURRENTPATH, MAX_PATH, (LPARAM)filePath);
        
        switch (notifyCode->nmhdr.code) {
            case NPPN_FILEBEFORESAVE:
                GoogleKeepSyncPlugin::Instance().OnFileBeforeSave(filePath);
                break;
            case NPPN_BUFFERSAVED:
                GoogleKeepSyncPlugin::Instance().OnFileSaved(filePath);
                break;
            case NPPN_BUFFERACTIVATED:
                GoogleKeepSyncPlugin::Instance().OnBufferActivated(filePath);
                break;
            case NPPN_FILECLOSED:
                GoogleKeepSyncPlugin::Instance().OnFileClosed(filePath);
                break;
        }
    }
    
    return 0;
}

} // extern "C"
