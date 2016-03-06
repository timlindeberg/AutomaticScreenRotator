// ScreenRotator.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define PORT "56789"
#define DEFAULT_BUFLEN 512
#define SETTINGS_FILE "settings"
#define ROTATION_FILE "current_rotation"

#pragma comment(lib, "Ws2_32.lib")

BOOL CtrlHandler(DWORD fdwCtrlType);
SOCKET AcceptConnection();
int Shutdown(char* message = nullptr);
void ReadSettings();
void ReadCurrentRotation();
void SaveCurrentRotation();
void SetRotation(char rotation);
std::pair<DISPLAY_DEVICE, DEVMODE> GetDisplaySettings(int monitor_index);

char current_rotation;
POINTL P_position;
POINTL L_position;
size_t monitor_index;
SOCKET ClientSocket = INVALID_SOCKET, ListenSocket = INVALID_SOCKET;
struct addrinfo* addr_info = nullptr;

int _tmain(int argc, _TCHAR* argv[])
{
    if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE)){
        puts("Failed to set CtrlHandler!");
        return 0;
    }

    ReadSettings();
    printf("Settings: ScreenIndex: %u\nPortrait Position: (%d, %d)\nLandscape Position: (%d, %d)\n",
        monitor_index,
        P_position.x, P_position.y,
        L_position.x, L_position.y);

    ReadCurrentRotation();
    printf("Current rotation: %c\n", current_rotation);

    ClientSocket = AcceptConnection();

    char recvbuf[DEFAULT_BUFLEN];
    int res;
    do {
        res = recv(ClientSocket, recvbuf, DEFAULT_BUFLEN, 0);
        if (res > 0){
            SetRotation(recvbuf[0]);
            printf("Got data: %s\n", recvbuf);
        }
    } while (res > 0);

    return Shutdown();
}

BOOL CtrlHandler(DWORD fdwCtrlType){
    Shutdown();
    return false;
}

int Shutdown(char* message){
    send(ClientSocket, "0", 1, 0);

    if (message != nullptr)
        puts(message);
    if (addr_info != nullptr)
        freeaddrinfo(addr_info);
    if (ListenSocket != INVALID_SOCKET)
        closesocket(ListenSocket);
    if (ClientSocket != INVALID_SOCKET)
        closesocket(ClientSocket);

    WSACleanup();
    puts("Shutting down!");
    return 0;
}

void ReadSettings(){
    std::ifstream file;
    file.open(SETTINGS_FILE);

    file >> monitor_index;

    file >> P_position.x;
    file >> P_position.y;

    file >> L_position.x;
    file >> L_position.y;
    file.close();
}

void ReadCurrentRotation(){
    std::ifstream file;
    file.open(ROTATION_FILE);
    current_rotation = file.get();
    file.close();
}

void SaveCurrentRotation(){
    std::ofstream file;
    file.open(ROTATION_FILE);
    file << current_rotation;
    file.close();
}

void SetRotation(char rotation){
    if (rotation == current_rotation)
        return;

    current_rotation = rotation;
    SaveCurrentRotation();

    auto display_settings = GetDisplaySettings(monitor_index);
    DISPLAY_DEVICE dd = display_settings.first;
    DEVMODE dm = display_settings.second;

    dm.dmDisplayOrientation = rotation == 'L' ? DMDO_DEFAULT : DMDO_90;
    dm.dmPosition           = rotation == 'L' ? L_position : P_position;
    std::swap(dm.dmPelsWidth, dm.dmPelsHeight);

    int flags = CDS_UPDATEREGISTRY;
    if (ChangeDisplaySettingsEx(dd.DeviceName, &dm, nullptr, flags, nullptr) != 0)
        puts("Failed to apply display settings");
}

std::pair<DISPLAY_DEVICE, DEVMODE> GetDisplaySettings(int monitor_index){
    DISPLAY_DEVICE dd;
    dd.cb = sizeof(dd);

    EnumDisplayDevices(nullptr, 2, &dd, monitor_index);

    DEVMODE dm;

    // initialize the DEVMODE structure
    ZeroMemory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);

    int res = EnumDisplaySettingsEx(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm, EDS_RAWMODE);
    if (res == 0){
        puts("Failed to extract display settings.");
        return {};
    }

    return { dd, dm };
}

SOCKET AcceptConnection(){
    int wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    if (WSAStartup(wVersionRequested, &wsaData) != 0)
        return Shutdown("WSAStartup failed!");

    struct addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    int res = getaddrinfo(NULL, PORT, &hints, &addr_info);
    if (res != 0)
        return Shutdown("Getaddrinfo failed!");

    ListenSocket = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol);
    if (ListenSocket == INVALID_SOCKET)
        return Shutdown("Error at socket()");

    res = bind(ListenSocket, addr_info->ai_addr, (int)addr_info->ai_addrlen);
    if (res == SOCKET_ERROR)
        return Shutdown("Bind failed!");

    freeaddrinfo(addr_info);
    addr_info = nullptr;

    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
        return Shutdown("Listen failed!");

    puts("Started listening!");

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET)
        return Shutdown("Accept failed");

    puts("Accepted connection!");
    return ClientSocket;
}

