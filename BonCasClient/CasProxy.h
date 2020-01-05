// CasProxy.h: CCasProxy クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////


#pragma once


#include "SmartSock.h"


class CCasProxy
{
public:
	enum
	{
		// 互換部
		CPEI_ENTERPROCESS,		// プロセス開始通知
		CPEI_EXITPROCESS,		// プロセス終了通知
		CPEI_GETSERVERIP,		// サーバIP取得
		CPEI_GETSERVERPORT,		// サーバポート取得
		CPEI_CONNECTSUCCESS,	// 接続完了
		CPEI_CONNECTFAILED,		// 接続失敗
		CPEI_DISCONNECTED,		// 接続切断

		// 拡張
		CPEI_CHECKREJECTPROCESS,// 除外プロセスか問い合わせ(lParam=プロセス名格納グローバルATOM)
		CPEI_EXTENDCOMMAND,		// 拡張コマンド(lParam=コマンド格納グローバルATOM, 戻り=コマンド実行結果文字列格納グローバルATOM)
		CPEI_DELETERETURNATOM	// 拡張コマンド戻り値のATOMを削除
	};
	
	CCasProxy(const HWND hProxyHwnd);
	~CCasProxy(void);

	const void Setting(const DWORD dwIP, const WORD wPort);
	const BOOL Connect(void);
	const DWORD TransmitCommand(const BYTE *pSendData, const DWORD dwSendSize, BYTE *pRecvData);
/*
	static void SendEnterProcessEvent(const HWND hProxyHwnd);
	static void SendExitProcessEvent(const HWND hProxyHwnd);
	static DWORD SendProxyEvent2(const HWND hProxyHwnd, WPARAM wParam, LPARAM lParam);
*/
protected:
//	const DWORD SendProxyEvent(WPARAM wParam) const;

	const HWND m_hProxyHwnd;
	CSmartSock m_Socket;
	DWORD	m_dwIP;
	WORD	m_wPort;

	static DWORD dwErrorDelayTime;
};
