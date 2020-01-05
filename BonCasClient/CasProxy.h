// CasProxy.h: CCasProxy �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////


#pragma once


#include "SmartSock.h"


class CCasProxy
{
public:
	enum
	{
		// �݊���
		CPEI_ENTERPROCESS,		// �v���Z�X�J�n�ʒm
		CPEI_EXITPROCESS,		// �v���Z�X�I���ʒm
		CPEI_GETSERVERIP,		// �T�[�oIP�擾
		CPEI_GETSERVERPORT,		// �T�[�o�|�[�g�擾
		CPEI_CONNECTSUCCESS,	// �ڑ�����
		CPEI_CONNECTFAILED,		// �ڑ����s
		CPEI_DISCONNECTED,		// �ڑ��ؒf

		// �g��
		CPEI_CHECKREJECTPROCESS,// ���O�v���Z�X���₢���킹(lParam=�v���Z�X���i�[�O���[�o��ATOM)
		CPEI_EXTENDCOMMAND,		// �g���R�}���h(lParam=�R�}���h�i�[�O���[�o��ATOM, �߂�=�R�}���h���s���ʕ�����i�[�O���[�o��ATOM)
		CPEI_DELETERETURNATOM	// �g���R�}���h�߂�l��ATOM���폜
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
