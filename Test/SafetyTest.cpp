#include <winsock2.h>
#include <windows.h>
#include <winscard.h>
#include "SmartSock.h"
#include "CasProxy.h"
#include <cassert>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

struct Listener {
    SOCKET socket;
    WORD port;
    Listener() {
        socket = ::socket(AF_INET, SOCK_STREAM, 0);
        assert(socket != INVALID_SOCKET);
        sockaddr_in address = {};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        assert(!bind(socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)));
        int size = sizeof(address);
        assert(!getsockname(socket, reinterpret_cast<sockaddr*>(&address), &size));
        port = ntohs(address.sin_port);
        assert(!listen(socket, 4));
    }
    SOCKET accept() {
        fd_set read;
        FD_ZERO(&read);
        FD_SET(socket, &read);
        timeval timeout = {5, 0};
        assert(select(0, &read, NULL, NULL, &timeout) == 1);
        SOCKET client = ::accept(socket, NULL, NULL);
        assert(client != INVALID_SOCKET);
        DWORD ms = 3000;
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&ms), sizeof(ms));
        return client;
    }
    ~Listener() { closesocket(socket); }
};

struct InspectSocket : CSmartSock {
    DWORD timeout(int option) {
        DWORD value = 0;
        int size = sizeof(value);
        assert(!getsockopt(m_Socket, SOL_SOCKET, option, reinterpret_cast<char*>(&value), &size));
        return value;
    }
};

void socketTests() {
    Listener listener;
    std::thread server([&] {
        SOCKET client = listener.accept();
        char request;
        assert(recv(client, &request, 1, 0) == 1);
        Sleep(300);
        closesocket(client);
    });
    InspectSocket socket;
    assert(socket.Connect(0x7f000001, listener.port, 1000));
    BYTE value = 1;
    assert(socket.SendOnce(&value, 1, 120) == 1);
    assert(socket.timeout(SO_SNDTIMEO) == 120);
    const auto start = GetTickCount64();
    assert(!socket.RecvOnce(&value, 1, 80));
    assert(socket.GetLastError() == CSmartSock::EC_TIMEOUT);
    assert(socket.timeout(SO_RCVTIMEO) == 80);
    assert(GetTickCount64() - start < 2000);
    server.join();

    // A synchronous connect must be usable by Send/Recv too.
    Listener syncListener;
    std::thread syncServer([&] {
        SOCKET client = syncListener.accept();
        assert(send(client, "x", 1, 0) == 1);
        closesocket(client);
    });
    InspectSocket sync;
    assert(sync.Connect(0x7f000001, syncListener.port, 0));
    assert(sync.Recv(&value, 1, 1000) && value == 'x');
    syncServer.join();
}

void backoffTests() {
    Listener failed;
    // Bind without listen: reserve the port while making connections fail.
    closesocket(failed.socket);
    failed.socket = ::socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(failed.port);
    assert(!bind(failed.socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)));
    CCasProxy bad(NULL);
    bad.Setting(0x7f000001, failed.port);
    assert(!bad.Connect());
    const auto start = GetTickCount64();
    assert(!bad.Connect());
    assert(GetTickCount64() - start < 500);
    Listener healthy;
    std::thread server([&] { closesocket(healthy.accept()); });
    CCasProxy good(NULL);
    good.Setting(0x7f000001, healthy.port);
    assert(good.Connect());
    server.join();
}

template<class F> F load(HMODULE dll, const char* name) {
    auto address = GetProcAddress(dll, name);
    assert(address);
    return reinterpret_cast<F>(address);
}

void dllTests(const char* source, bool releaseDuringTransmit) {
    Listener listener;
    std::atomic<bool> transmitting(false);
    std::thread server([&] {
        SOCKET client = listener.accept();
        for (int i = 0; i != 3; ++i) {
            BYTE length = 0, request[255];
            assert(recv(client, reinterpret_cast<char*>(&length), 1, MSG_WAITALL) == 1);
            assert(length == 1);
            assert(recv(client, reinterpret_cast<char*>(request), length, MSG_WAITALL) == length);
            if (i == 2) { transmitting = true; Sleep(150); }
            const char reply[] = {2, static_cast<char>(0x90), 0};
            assert(send(client, reply, sizeof(reply), 0) == sizeof(reply));
        }
        closesocket(client);
    });
    char temp[MAX_PATH], full[MAX_PATH];
    assert(GetTempPathA(MAX_PATH, temp));
    const std::string directory = std::string(temp) + "BonCasSafety-" + std::to_string(GetCurrentProcessId());
    assert(CreateDirectoryA(directory.c_str(), NULL));
    const std::string dllPath = directory + "\\BonCasClient.dll";
    const std::string iniPath = directory + "\\BonCasClient.ini";
    assert(GetFullPathNameA(source, MAX_PATH, full, NULL));
    assert(CopyFileA(full, dllPath.c_str(), FALSE));
    {
        std::ofstream config(iniPath);
        config << "[Server]\nIP=127.0.0.1\nPort=" << listener.port
            << "\n[Server:Port]\nServer00=127.0.0.1:1\n";
    }
    HMODULE dll = LoadLibraryA(dllPath.c_str());
    assert(dll);
    auto establish = load<decltype(&SCardEstablishContext)>(dll, "CasLinkEstablishContext");
    auto release = load<decltype(&SCardReleaseContext)>(dll, "CasLinkReleaseContext");
    auto listA = load<decltype(&SCardListReadersA)>(dll, "CasLinkListReadersA");
    auto listW = load<decltype(&SCardListReadersW)>(dll, "CasLinkListReadersW");
    auto freeMemory = load<decltype(&SCardFreeMemory)>(dll, "CasLinkFreeMemory");
    auto connectA = load<decltype(&SCardConnectA)>(dll, "CasLinkConnectA");
    auto connectW = load<decltype(&SCardConnectW)>(dll, "CasLinkConnectW");
    auto disconnect = load<decltype(&SCardDisconnect)>(dll, "CasLinkDisconnect");
    auto transmit = load<decltype(&SCardTransmit)>(dll, "CasLinkTransmit");
    SCARDCONTEXT context = 0, other = 0;
    assert(establish(SCARD_SCOPE_SYSTEM, NULL, NULL, NULL) == SCARD_E_INVALID_PARAMETER);
    assert(establish(SCARD_SCOPE_SYSTEM, NULL, NULL, &context) == SCARD_S_SUCCESS);
    assert(establish(SCARD_SCOPE_SYSTEM, NULL, NULL, &other) == SCARD_S_SUCCESS && other != context);
    DWORD length = 0;
    assert(listA(context, NULL, NULL, &length) == SCARD_S_SUCCESS && length > 2);
    char guard[2] = {'X', 'Y'};
    DWORD capacity = 1;
    assert(listA(context, NULL, guard, &capacity) == SCARD_E_INSUFFICIENT_BUFFER);
    assert(capacity == length && guard[0] == 'X' && guard[1] == 'Y');
    wchar_t wideGuard[2] = {L'X', L'Y'};
    capacity = 1;
    assert(listW(context, NULL, wideGuard, &capacity) == SCARD_E_INSUFFICIENT_BUFFER);
    assert(wideGuard[0] == L'X' && wideGuard[1] == L'Y');
    std::vector<char> names(length);
    assert(listA(context, NULL, names.data(), &length) == SCARD_S_SUCCESS);
    const std::string expected = "127.0.0.1:" + std::to_string(listener.port);
    assert(expected == names.data());
    assert(std::string(names.data() + expected.size() + 1) == "127.0.0.1:1");
    assert(names[length - 1] == 0 && names[length - 2] == 0);
    wchar_t* wideNames = NULL;
    DWORD automatic = SCARD_AUTOALLOCATE;
    assert(listW(context, NULL, reinterpret_cast<wchar_t*>(&wideNames), &automatic) == SCARD_S_SUCCESS);
    assert(freeMemory(other, wideNames) == SCARD_E_INVALID_PARAMETER);
    assert(freeMemory(context, wideNames) == SCARD_S_SUCCESS);
    assert(freeMemory(context, wideNames) == SCARD_E_INVALID_PARAMETER);
    SCARDHANDLE card = 123;
    DWORD protocol = 0;
    assert(connectA(context, NULL, 0, 0, &card, NULL) == SCARD_E_INVALID_PARAMETER && card == 0);
    assert(connectA(context, "missing", 0, 0, &card, NULL) == SCARD_E_READER_UNAVAILABLE);
    const std::wstring wideExpected(expected.begin(), expected.end());
    assert(connectW(context, wideExpected.c_str(), SCARD_SHARE_SHARED, SCARD_PROTOCOL_T1,
        &card, &protocol) == SCARD_S_SUCCESS && protocol == SCARD_PROTOCOL_T1);
    BYTE command[256] = {0xA4};
    BYTE response[3] = {0xAA, 0xBB, 0xCC};
    length = 3;
    assert(transmit(card, NULL, command, 256, NULL, response, &length) == SCARD_E_INVALID_PARAMETER);
    assert(transmit(card, NULL, command, 0, NULL, response, &length) == SCARD_E_INVALID_PARAMETER);
    length = 1;
    assert(transmit(card, NULL, command, 1, NULL, response, &length) == SCARD_E_INSUFFICIENT_BUFFER);
    assert(length == 2 && response[0] == 0xAA && response[1] == 0xBB && response[2] == 0xCC);
    length = 3;
    assert(transmit(card, NULL, command, 1, NULL, response, &length) == SCARD_S_SUCCESS);
    assert(length == 2 && response[0] == 0x90 && response[1] == 0 && response[2] == 0xCC);
    std::thread call([&] {
        DWORD capacity = sizeof(response);
        assert(transmit(card, NULL, command, 1, NULL, response, &capacity) == SCARD_S_SUCCESS);
    });
    const auto deadline = GetTickCount64() + 4000;
    while (!transmitting && GetTickCount64() < deadline) Sleep(1);
    assert(transmitting);
    if (releaseDuringTransmit) assert(release(context) == SCARD_S_SUCCESS);
    else assert(disconnect(card, SCARD_LEAVE_CARD) == SCARD_S_SUCCESS);
    call.join();
    assert(disconnect(card, SCARD_LEAVE_CARD) == SCARD_E_INVALID_HANDLE);
    if (!releaseDuringTransmit) assert(release(context) == SCARD_S_SUCCESS);
    assert(release(context) == SCARD_E_INVALID_HANDLE);
    length = 0;
    assert(listA(context, NULL, NULL, &length) == SCARD_E_INVALID_HANDLE);
    assert(release(other) == SCARD_S_SUCCESS);
    server.join();
    assert(FreeLibrary(dll));
    assert(DeleteFileA(iniPath.c_str()));
    assert(DeleteFileA(dllPath.c_str()));
    assert(RemoveDirectoryA(directory.c_str()));
}

int main(int argc, char** argv) {
    assert(argc == 2);
    WSADATA data;
    assert(!WSAStartup(MAKEWORD(2, 2), &data));
    socketTests();
    backoffTests();
    dllTests(argv[1], true);
    dllTests(argv[1], false);
    WSACleanup();
    std::cout << "Transport, buffer, legacy INI, and handle lifetime tests passed\n";
}
