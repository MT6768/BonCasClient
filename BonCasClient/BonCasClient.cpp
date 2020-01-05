// BonCasClient.cpp : DLL アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include <WinScard.h>
#include "CasProxy.h"

#include <stdlib.h>
#include <wchar.h>
#include <stdarg.h>
#include <tchar.h>

#ifdef _MANAGED
#pragma managed(push, off)
#endif

DWORD g_dwIP = 0x7F000001UL;
WORD g_wPort = 6900UL;

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
					::GetPrivateProfileString(_T("Server"), _T("IP"), _T("127.0.0.1"), szBuff, 64, szIniPath);
					char szHost[64] = "";
					size_t len = 0;
					::wcstombs_s(&len, szHost, 64, szBuff, 63);
					DWORD ip = ::inet_addr(szHost);
					g_dwIP = ((ip & 0xFF) << 24) + ((ip & 0xFF00) << 8) + ((ip & 0xFF0000) >> 8) + ((ip & 0xFF000000) >> 24);

					g_wPort = ::GetPrivateProfileInt(_T("Server"), _T("Port"), 6900UL, szIniPath);
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
	g_dwIP = dwIP;
	g_wPort = wPort;
	return TRUE;
}

LONG CasLinkConnect(LPSCARDHANDLE phCard, LPDWORD pdwActiveProtocol)
{
	// プロキシインスタンス生成
	CCasProxy *pCasProxy = new CCasProxy(NULL);

	// IP,Port設定
	pCasProxy->Setting(g_dwIP, g_wPort);
	
	// サーバに接続
	if (!pCasProxy->Connect()) {
		delete pCasProxy;
		*phCard = NULL;
		return SCARD_E_READER_UNAVAILABLE;
	}

	// ハンドルに埋め込む
	*phCard = reinterpret_cast<SCARDHANDLE>(pCasProxy);
	if (pdwActiveProtocol)	*pdwActiveProtocol = SCARD_PROTOCOL_T1;

	return SCARD_S_SUCCESS;
}

extern "C" __declspec(dllexport) LONG WINAPI CasLinkConnectA(SCARDCONTEXT hContext, LPCSTR szReader, DWORD dwShareMode, DWORD dwPreferredProtocols, LPSCARDHANDLE phCard, LPDWORD pdwActiveProtocol)
{
	return CasLinkConnect(phCard, pdwActiveProtocol);
}

extern "C" __declspec(dllexport) LONG WINAPI CasLinkConnectW(SCARDCONTEXT hContext, LPCWSTR szReader, DWORD dwShareMode, DWORD dwPreferredProtocols, LPSCARDHANDLE phCard, LPDWORD pdwActiveProtocol)
{
	return CasLinkConnect(phCard, pdwActiveProtocol);
}

extern "C" __declspec(dllexport) LONG WINAPI CasLinkDisconnect(SCARDHANDLE hCard, DWORD dwDisposition)
{
	// サーバから切断
	CCasProxy *pCasProxy = reinterpret_cast<CCasProxy *>(hCard);
	if (pCasProxy)	delete pCasProxy;

	return SCARD_S_SUCCESS;
}

extern "C" __declspec(dllexport) LONG WINAPI CasLinkEstablishContext(DWORD dwScope, LPCVOID pvReserved1, LPCVOID pvReserved2, LPSCARDCONTEXT phContext)
{
	return SCARD_S_SUCCESS;
}

extern "C" __declspec(dllexport) LONG WINAPI CasLinkFreeMemory(SCARDCONTEXT hContext, LPCVOID pvMem)
{
	return SCARD_S_SUCCESS;
}

extern "C" __declspec(dllexport) LONG WINAPI CasLinkListReadersA(SCARDCONTEXT hContext, LPCSTR mszGroups, LPSTR mszReaders, LPDWORD pcchReaders)
{
	static const char szReaderName[] = "BonCasLink Client Card Reader\0\0";

	if(pcchReaders){
		if ((*pcchReaders == SCARD_AUTOALLOCATE) && mszReaders) {
			*((LPCSTR *)mszReaders) = szReaderName;		
			return SCARD_S_SUCCESS;
		}else{
			*pcchReaders = sizeof(szReaderName);
		}
	}

	if (mszReaders)	::CopyMemory(mszReaders, szReaderName, sizeof(szReaderName));

	return SCARD_S_SUCCESS;
}

extern "C" __declspec(dllexport) LONG WINAPI CasLinkListReadersW(SCARDCONTEXT hContext, LPCWSTR mszGroups, LPWSTR mszReaders, LPDWORD pcchReaders)
{
	static const WCHAR szReaderName[] = L"BonCasLink Client Card Reader\0\0";

	if (pcchReaders) {
		if ((*pcchReaders == SCARD_AUTOALLOCATE) && mszReaders) {
			*((LPCWSTR *)mszReaders) = szReaderName;		
			return SCARD_S_SUCCESS;
		}else{
			*pcchReaders = sizeof(szReaderName);
		}
	}

	if (mszReaders)	::CopyMemory(mszReaders, szReaderName, sizeof(szReaderName));

	return SCARD_S_SUCCESS;
}

extern "C" __declspec(dllexport) LONG WINAPI CasLinkTransmit(SCARDHANDLE hCard, LPCSCARD_IO_REQUEST pioSendPci, LPCBYTE pbSendBuffer, DWORD cbSendLength, LPSCARD_IO_REQUEST pioRecvPci, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength)
{
	// サーバにリクエスト送受信
	CCasProxy *pCasProxy = reinterpret_cast<CCasProxy *>(hCard);
	if (!pCasProxy)	return SCARD_E_READER_UNAVAILABLE;

	const DWORD dwRecvLen = pCasProxy->TransmitCommand(pbSendBuffer, cbSendLength, pbRecvBuffer);
	if (pcbRecvLength)	*pcbRecvLength = dwRecvLen;

	return (dwRecvLen)? SCARD_S_SUCCESS : SCARD_E_TIMEOUT;
}

extern "C" __declspec(dllexport) LONG WINAPI CasLinkReleaseContext(SCARDCONTEXT hContext)
{
	return SCARD_S_SUCCESS;
}


#ifdef _MANAGED
#pragma managed(pop)
#endif

