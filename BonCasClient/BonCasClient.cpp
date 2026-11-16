// BonCasClient.cpp : DLL アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include <WinScard.h>
#include "CasProxy.h"

#include <WS2tcpip.h>

#include <stdlib.h>
#include <wchar.h>
#include <stdarg.h>
#include <tchar.h>

#include <sstream>
#include <string>
#include <map>
#include <set>
#include <memory>
#include <mutex>
#include <limits>
#include <vector>

#ifdef _MANAGED
#pragma managed(push, off)
#endif

#define __SPRITTER_CHAR_A	':'
#define __SPRITTER_CHAR_W	L':'
#define __SPRITTER_CHAR		_T(__SPRITTER_CHAR_A)


using namespace std;

string g_ReaderTableA;
wstring g_ReaderTableW;

typedef basic_string<TCHAR> tstring;

std::wstring convString(const std::string& input)
{
	size_t i;
	wchar_t* buffer = new wchar_t[input.size() + 1];
	mbstowcs_s(&i, buffer, input.size() + 1, input.c_str(), _TRUNCATE);
	std::wstring result = buffer;
	delete[] buffer;
	return result;
}

std::string convString(const std::wstring& input)
{
	size_t i;
	char* buffer = new char[input.size() * MB_CUR_MAX + 1];
	wcstombs_s(&i, buffer, input.size() * MB_CUR_MAX + 1, input.c_str(), _TRUNCATE);
	std::string result = buffer;
	delete[] buffer;
	return result;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch(ul_reason_for_call){
		case DLL_PROCESS_ATTACH:
#ifdef _DEBUG
		// メモリリーク検出有効
			::_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | ::_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)); 
#endif
			{
				TCHAR szDllPath[_MAX_PATH] = _T("");
				if (::GetModuleFileName(hModule, szDllPath, _MAX_PATH) != 0) {
					TCHAR szDrive[_MAX_DRIVE];
					TCHAR szDir[_MAX_DIR];
					TCHAR szFName[_MAX_FNAME];
					_tsplitpath_s(szDllPath, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFName, _MAX_FNAME, NULL, 0);

					TCHAR szIniPath[_MAX_PATH] = _T("");
					_tmakepath_s(szIniPath, _MAX_PATH, szDrive, szDir, szFName, _T("ini"));

					TCHAR szBuff[64] = _T("");
					::GetPrivateProfileString(_T("Server"), _T("IP"), _T("127.0.0.1"), szBuff, _countof(szBuff), szIniPath);
					DWORD wPort = ::GetPrivateProfileInt(_T("Server"), _T("Port"), 6900UL, szIniPath);

					ostringstream ReaderTableA;
					wostringstream ReaderTableW;

#ifdef _UNICODE
					ReaderTableA << convString(wstring(szBuff)) << __SPRITTER_CHAR_A << wPort << '\0';
					ReaderTableW << szBuff << __SPRITTER_CHAR_W << wPort << L'\0';
#else
					ReaderTableA << szBuff << __SPRITTER_CHAR_A << wPort << '\0';
					ReaderTableW << convString(string(szBuff)) << __SPRITTER_CHAR_W << wPort << L'\0';
#endif
					//以降拡張セクション
					for (int i = 0; i < 99; i++)
					{
						TCHAR key[16];
						TCHAR buf[1024];

						_stprintf_s(key, _countof(key), _T("Server%02d"), i);

						if( ::GetPrivateProfileString(_T("Server:Port"), key, NULL, buf, _countof(buf), szIniPath) == 0) break;// Keyが見つからなければ終了

						LPCTSTR pszPortStr = _tcsrchr(buf, __SPRITTER_CHAR);
						int port_num;
						if (pszPortStr == NULL || (port_num = _tstoi(pszPortStr + 1)) < 0 || port_num > USHRT_MAX) continue;// 不正な形式orポート番号なら読み飛ばす

						tstring server_name = tstring(buf, (int)(pszPortStr - buf));

#ifdef _UNICODE
						ReaderTableA << convString(server_name) << __SPRITTER_CHAR_A << port_num << '\0';
						ReaderTableW << server_name << __SPRITTER_CHAR_W << port_num << L'\0';
#else
						ReaderTableA << server_name << __SPRITTER_CHAR_A << port_num << '\0';
						ReaderTableW << convString(server_name) << __SPRITTER_CHAR_W << port_num << L'\0';
#endif
					}
					ReaderTableA << '\0';// 終端子
					ReaderTableW << L'\0';

					g_ReaderTableA = ReaderTableA.str();
					g_ReaderTableW = ReaderTableW.str();
				}
			}
			break;

		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

extern "C" __declspec(dllexport) BOOL WINAPI SetConfig(const DWORD dwIP, const WORD wPort)
{
	return TRUE;
}

namespace {
struct Card {
    CCasProxy proxy;
    SCARDCONTEXT context;
    std::mutex io;
    bool active = true;
    explicit Card(SCARDCONTEXT owner) : proxy(NULL), context(owner) {}
};
std::mutex registryLock;
std::set<SCARDCONTEXT> contexts;
std::map<SCARDHANDLE, std::shared_ptr<Card>> cards;
std::map<const void*, SCARDCONTEXT> allocations;
SCARDCONTEXT nextContext = 1;
SCARDHANDLE nextCard = 1;

// Never reuse tokens: stale handles cannot refer to a new card/context.
template<class T> T token(T& next) {
    if (!next || next == (std::numeric_limits<T>::max)()) throw std::bad_alloc();
    return next++;
}

template<class C> LONG listReaders(SCARDCONTEXT context, const std::basic_string<C>& names,
                                 C* output, LPDWORD length) {
    if (!length) return SCARD_E_INVALID_PARAMETER;
    std::lock_guard<std::mutex> guard(registryLock);
    if (!contexts.count(context)) return SCARD_E_INVALID_HANDLE;
    const DWORD capacity = *length;
    *length = static_cast<DWORD>(names.size());
    if (!output) return SCARD_S_SUCCESS;
    if (capacity == SCARD_AUTOALLOCATE) {
        *reinterpret_cast<C**>(output) = NULL;
        // Use a common allocation type for FreeMemory and ReleaseContext.
        std::unique_ptr<BYTE[]> bytes(new BYTE[names.size() * sizeof(C)]);
        CopyMemory(bytes.get(), names.data(), names.size() * sizeof(C));
        allocations.emplace(bytes.get(), context);
        *reinterpret_cast<C**>(output) = reinterpret_cast<C*>(bytes.release());
        return SCARD_S_SUCCESS;
    }
    if (capacity < names.size()) return SCARD_E_INSUFFICIENT_BUFFER;
    CopyMemory(output, names.data(), names.size() * sizeof(C));
    return SCARD_S_SUCCESS;
}

template<class C> LONG connectReader(SCARDCONTEXT context, const C* name,
    const std::basic_string<C>& names, LPSCARDHANDLE result, LPDWORD protocol) {
    if (!result) return SCARD_E_INVALID_PARAMETER;
    *result = 0;
    if (!name) return SCARD_E_INVALID_PARAMETER;
    std::unique_lock<std::mutex> guard(registryLock);
    if (!contexts.count(context)) return SCARD_E_INVALID_HANDLE;
    for (const C* entry = names.c_str(); *entry; entry += std::char_traits<C>::length(entry) + 1) {
        if (std::basic_string<C>(entry) != name) continue;
        const std::basic_string<C> endpoint(entry);
        const auto separator = endpoint.rfind(static_cast<C>(':'));
        if (separator == std::basic_string<C>::npos) return SCARD_E_READER_UNAVAILABLE;
        const std::basic_string<C> host = endpoint.substr(0, separator);
        const std::basic_string<C> portText = endpoint.substr(separator + 1);
        unsigned long port = 0;
        for (C digit : portText) {
            if (digit < '0' || digit > '9' || port > 6553) return SCARD_E_READER_UNAVAILABLE;
            port = port * 10 + digit - '0';
        }
        if (!port || port > 65535) return SCARD_E_READER_UNAVAILABLE;
        std::string address;
        for (C ch : host) {
            if ((ch < '0' || ch > '9') && ch != '.') return SCARD_E_READER_UNAVAILABLE;
            address.push_back(static_cast<char>(ch));
        }
        IN_ADDR ip = {};
        if (InetPtonA(AF_INET, address.c_str(), &ip) != 1) return SCARD_E_READER_UNAVAILABLE;
        auto card = std::make_shared<Card>(context);
        card->proxy.Setting(ntohl(ip.S_un.S_addr), static_cast<WORD>(port));
        guard.unlock();
        const BOOL connected = card->proxy.Connect();
        guard.lock();
        if (!contexts.count(context)) return SCARD_E_INVALID_HANDLE;
        if (!connected) return SCARD_E_READER_UNAVAILABLE;
        const SCARDHANDLE handle = token(nextCard);
        cards.emplace(handle, card);
        *result = handle;
        if (protocol) *protocol = SCARD_PROTOCOL_T1;
        return SCARD_S_SUCCESS;
    }
    return SCARD_E_READER_UNAVAILABLE;
}
}

// No C++ exception may escape the DLL's C ABI.
#define API extern "C" __declspec(dllexport) LONG WINAPI
#define TRY_API try {
#define END_API } catch (const std::bad_alloc&) { return SCARD_E_NO_MEMORY; } \
    catch (...) { return SCARD_F_INTERNAL_ERROR; }

API CasLinkConnectA(SCARDCONTEXT context, LPCSTR name, DWORD, DWORD,
                   LPSCARDHANDLE result, LPDWORD protocol) {
    TRY_API
    return connectReader(context, name, g_ReaderTableA, result, protocol);
    END_API
}
API CasLinkConnectW(SCARDCONTEXT context, LPCWSTR name, DWORD, DWORD,
                   LPSCARDHANDLE result, LPDWORD protocol) {
    TRY_API
    return connectReader(context, name, g_ReaderTableW, result, protocol);
    END_API
}
API CasLinkEstablishContext(DWORD, LPCVOID, LPCVOID, LPSCARDCONTEXT result) {
    TRY_API
    if (!result) return SCARD_E_INVALID_PARAMETER;
    *result = 0;
    std::lock_guard<std::mutex> guard(registryLock);
    const SCARDCONTEXT context = token(nextContext);
    contexts.insert(context);
    *result = context;
    return SCARD_S_SUCCESS;
    END_API
}
API CasLinkDisconnect(SCARDHANDLE handle, DWORD) {
    TRY_API
    std::shared_ptr<Card> card;
    {
        std::lock_guard<std::mutex> guard(registryLock);
        auto it = cards.find(handle);
        if (it == cards.end()) return SCARD_E_INVALID_HANDLE;
        card = it->second;
        cards.erase(it);
    }
    std::lock_guard<std::mutex> io(card->io);
    card->active = false;
    return SCARD_S_SUCCESS;
    END_API
}
API CasLinkReleaseContext(SCARDCONTEXT context) {
    TRY_API
    std::vector<std::shared_ptr<Card>> retired;
    {
        std::lock_guard<std::mutex> guard(registryLock);
        if (!contexts.count(context)) return SCARD_E_INVALID_HANDLE;
        // Reserve before modifying state so allocation failure leaves it intact.
        retired.reserve(cards.size());
        contexts.erase(context);
        for (auto it = cards.begin(); it != cards.end();) {
            if (it->second->context == context) {
                retired.push_back(it->second);
                it = cards.erase(it);
            } else ++it;
        }
        for (auto it = allocations.begin(); it != allocations.end();) {
            if (it->second == context) {
                delete[] static_cast<const BYTE*>(it->first);
                it = allocations.erase(it);
            } else ++it;
        }
    }
    for (auto& card : retired) {
        std::lock_guard<std::mutex> io(card->io);
        card->active = false;
    }
    return SCARD_S_SUCCESS;
    END_API
}
API CasLinkFreeMemory(SCARDCONTEXT context, LPCVOID memory) {
    TRY_API
    std::lock_guard<std::mutex> guard(registryLock);
    if (!contexts.count(context)) return SCARD_E_INVALID_HANDLE;
    auto it = allocations.find(memory);
    if (it == allocations.end() || it->second != context) return SCARD_E_INVALID_PARAMETER;
    delete[] static_cast<const BYTE*>(memory);
    allocations.erase(it);
    return SCARD_S_SUCCESS;
    END_API
}
API CasLinkListReadersA(SCARDCONTEXT context, LPCSTR, LPSTR output, LPDWORD length) {
    TRY_API
    return listReaders(context, g_ReaderTableA, output, length);
    END_API
}
API CasLinkListReadersW(SCARDCONTEXT context, LPCWSTR, LPWSTR output, LPDWORD length) {
    TRY_API
    return listReaders(context, g_ReaderTableW, output, length);
    END_API
}
API CasLinkTransmit(SCARDHANDLE handle, LPCSCARD_IO_REQUEST, LPCBYTE send, DWORD sendLength,
                    LPSCARD_IO_REQUEST recvPci, LPBYTE recv, LPDWORD recvLength) {
    TRY_API
    if (!send || !sendLength || sendLength > 255 || !recvLength || !recv)
        return SCARD_E_INVALID_PARAMETER;
    if (recvPci && recvPci->cbPciLength < sizeof(SCARD_IO_REQUEST))
        return SCARD_E_INSUFFICIENT_BUFFER;
    std::shared_ptr<Card> card;
    {
        std::lock_guard<std::mutex> guard(registryLock);
        auto it = cards.find(handle);
        if (it == cards.end()) return SCARD_E_INVALID_HANDLE;
        card = it->second;
    }
    std::lock_guard<std::mutex> io(card->io);
    if (!card->active) return SCARD_E_INVALID_HANDLE;
    BYTE response[255];
    const DWORD length = card->proxy.TransmitCommand(send, sendLength, response);
    const DWORD capacity = *recvLength;
    *recvLength = length;
    if (!length) return SCARD_E_TIMEOUT;
    if (capacity < length) return SCARD_E_INSUFFICIENT_BUFFER;
    CopyMemory(recv, response, length);
    if (recvPci) {
        recvPci->dwProtocol = SCARD_PROTOCOL_T1;
        recvPci->cbPciLength = sizeof(SCARD_IO_REQUEST);
    }
    return SCARD_S_SUCCESS;
    END_API
}

#ifdef _MANAGED
#pragma managed(pop)
#endif
