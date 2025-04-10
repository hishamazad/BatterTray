#include <windows.h>
#include <shellapi.h>
#include <wininet.h>
#include <strsafe.h>
#include "resource.h"
#include <shobjidl.h>
#include <shlguid.h>
#include <shlobj.h>

#pragma comment(lib, "wininet.lib")

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_SEND 1
#define ID_TRAY_SETTINGS 2
#define ID_TRAY_EXIT 3
#define ID_TIMER_PUSH 1001
#define ID_TRAY_STARTUP 4

NOTIFYICONDATA nid = {};
HMENU hTrayMenu;
HINSTANCE hInst;
HWND hMainWnd;

TCHAR g_url[256] = L"https://www.postb.in/1744257667496-8851193876471";
UINT g_interval = 60;
bool IsInStartup() {
    HKEY hKey;
    WCHAR path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        WCHAR value[MAX_PATH];
        DWORD size = sizeof(value);
        LONG result = RegQueryValueExW(hKey, L"BatterTray", NULL, NULL, (LPBYTE)value, &size);
        RegCloseKey(hKey);
        return (result == ERROR_SUCCESS && wcscmp(value, path) == 0);
    }
    return false;
}

void SetStartup(bool enable) {
    HKEY hKey;
    WCHAR path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {

        if (enable) {
            RegSetValueExW(hKey, L"BatterTray", 0, REG_SZ, (BYTE*)path, (DWORD)((wcslen(path) + 1) * sizeof(WCHAR)));
        }
        else {
            RegDeleteValueW(hKey, L"BatterTray");
        }

        RegCloseKey(hKey);
    }
}

void UpdateTrayIcon(int batteryLevel) {
    HICON icon;
    if (batteryLevel < 30)
        icon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_BATTERTRAYLOW));
    else if (batteryLevel < 90)
        icon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_BATTERTRAYHALF));
    else
        icon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_BATTERTRAY));

    nid.hIcon = icon;
    StringCchPrintf(nid.szTip, ARRAYSIZE(nid.szTip), L"Battery: %d%%", batteryLevel);
    nid.uFlags = NIF_ICON | NIF_TIP;
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}
void SendBatteryStatus() {
    SYSTEM_POWER_STATUS status;
    char computerName[MAX_COMPUTERNAME_LENGTH + 1] = { 0 };
    DWORD size = sizeof(computerName);

    if (GetComputerNameA(computerName, &size) && GetSystemPowerStatus(&status)) {
        char url[512];
        sprintf_s(url,
            "%ws?datatype=CPU%%20Temperature&data=%d&note=%s",
            g_url, status.BatteryLifePercent, computerName);

        HINTERNET hInternet = InternetOpen(L"BatteryTray", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (hInternet) {
            HINTERNET hUrl = InternetOpenUrlA(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE, 0);
            if (hUrl) {
                InternetCloseHandle(hUrl);
            }
            InternetCloseHandle(hInternet);
        }
        UpdateTrayIcon(status.BatteryLifePercent);

    }
}

INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        SetDlgItemText(hDlg, IDC_EDIT_URL, g_url);
        SetDlgItemInt(hDlg, IDC_EDIT_INTERVAL, g_interval, FALSE);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            GetDlgItemText(hDlg, IDC_EDIT_URL, g_url, sizeof(g_url) / sizeof(TCHAR));
            BOOL translated;
            UINT interval = GetDlgItemInt(hDlg, IDC_EDIT_INTERVAL, &translated, FALSE);
            if (translated && interval >= 10) {
                g_interval = interval;
                SetTimer(hMainWnd, ID_TIMER_PUSH, g_interval * 1000, NULL);
            }
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        SetTimer(hWnd, ID_TIMER_PUSH, g_interval * 1000, NULL);
        break;

    case WM_TIMER:
        if (wParam == ID_TIMER_PUSH) {
            SendBatteryStatus();
        }
        break;

    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hWnd);

            CheckMenuItem(hTrayMenu, ID_TRAY_STARTUP,
                MF_BYCOMMAND | (IsInStartup() ? MF_CHECKED : MF_UNCHECKED));

            TrackPopupMenu(hTrayMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
        }
        break;


    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_TRAY_SEND:
            SendBatteryStatus();
            break;
        case ID_TRAY_SETTINGS:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_SETTINGS), hWnd, SettingsDlgProc);
            break;
        case ID_TRAY_STARTUP:
                SetStartup(!IsInStartup());
                break;
        case ID_TRAY_EXIT:
            Shell_NotifyIcon(NIM_DELETE, &nid);
            PostQuitMessage(0);
            break;
        }
        break;

    case WM_DESTROY:
        KillTimer(hWnd, ID_TIMER_PUSH);
        Shell_NotifyIcon(NIM_DELETE, &nid);
        DestroyMenu(hTrayMenu);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void AddToStartupIfMissing() {
    WCHAR exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    PWSTR startupPath = NULL;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Startup, 0, NULL, &startupPath))) {
        WCHAR shortcutPath[MAX_PATH];
        swprintf_s(shortcutPath, MAX_PATH, L"%s\\BatterTray.lnk", startupPath);

        if (GetFileAttributes(shortcutPath) != INVALID_FILE_ATTRIBUTES) {
            CoTaskMemFree(startupPath);
            return;
        }

        CoInitialize(NULL);
        IShellLinkW* pLink = nullptr;

        if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&pLink))) {
            pLink->SetPath(exePath);
            pLink->SetDescription(L"BatterTray");

            IPersistFile* pFile;
            if (SUCCEEDED(pLink->QueryInterface(IID_IPersistFile, (void**)&pFile))) {
                pFile->Save(shortcutPath, TRUE);
                pFile->Release();
            }
            pLink->Release();
        }

        CoUninitialize();
        CoTaskMemFree(startupPath);
    }
}


int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine, int nCmdShow) {

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    hInst = hInstance;
    AddToStartupIfMissing();

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"BatteryTrayApp";
    RegisterClass(&wc);

    hMainWnd = CreateWindowEx(0, wc.lpszClassName, L"Battery Tray", 0,
        0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hMainWnd;
    nid.uID = 1;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_BATTERTRAY));
    StringCchCopy(nid.szTip, ARRAYSIZE(nid.szTip), L"Battery Tray");
    Shell_NotifyIcon(NIM_ADD, &nid);

    hTrayMenu = CreatePopupMenu();
    AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_SEND, L"Send Battery Now");
    AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_SETTINGS, L"Settings");
    AppendMenu(hTrayMenu, MF_STRING | (IsInStartup() ? MF_CHECKED : 0), ID_TRAY_STARTUP, L"Start with Windows");
    AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");

    SendBatteryStatus();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
