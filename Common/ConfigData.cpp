// ConfigData.cpp: ConfigData �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ConfigData.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// �萔��`
//////////////////////////////////////////////////////////////////////

// �Z�N�V�����̒�`
static const TCHAR SectionArray[][256] =
{
	TEXT("General")		// CONFSECT_GENERAL
};


//////////////////////////////////////////////////////////////////////
// CConfigFile �̃C���v�������e�[�V����
//////////////////////////////////////////////////////////////////////

CConfigData::CConfigFile::CConfigFile()
{
	// �ݒ�t�@�C���̃p�X�����\�z����
	TCHAR szPath[_MAX_PATH + 1] = {TEXT('\0')};
	TCHAR szDrive[_MAX_DRIVE + 1] = {TEXT('\0')};
	TCHAR szDir[_MAX_DIR + 1] = {TEXT('\0')};
	TCHAR szFile[_MAX_FNAME + 1] = {TEXT('\0')};

	::GetModuleFileName(NULL, szPath, sizeof(szPath) - 1);
	::_tsplitpath(szPath, szDrive, szDir, szFile, NULL);
	::_tmakepath(m_szConfigPath, szDrive, szDir, szFile, TEXT("ini"));
}

CConfigData::CConfigFile::~CConfigFile()
{

}

LPCTSTR CConfigData::CConfigFile::GetPath(void)
{
	// �ݒ�t�@�C���̃p�X��Ԃ�
	return m_szConfigPath;	
}

LPCTSTR CConfigData::CConfigFile::GetSection(const UINT nSection)
{
	// �Z�N�V��������Ԃ�
	return SectionArray[nSection];
}


//////////////////////////////////////////////////////////////////////
// CConfigData �̃C���v�������e�[�V����
//////////////////////////////////////////////////////////////////////

CConfigData::CConfigFile CConfigData::m_ConfigFile;


//////////////////////////////////////////////////////////////////////
// CConfigBool �̃C���v�������e�[�V����
//////////////////////////////////////////////////////////////////////

CConfigBool::CConfigBool()
	: m_bDefault(FALSE)
	, m_bValue(FALSE)
{

}

CConfigBool::CConfigBool(const UINT nSection, LPCTSTR lpszKeyName, const BOOL bDefault)
	: m_bDefault(FALSE)
	, m_bValue(FALSE)
{
	RegisterKey(nSection, lpszKeyName, bDefault);
}

void CConfigBool::RegisterKey(const UINT nSection, LPCTSTR lpszKeyName, const BOOL bDefault)
{
	// �L�[���ۑ�
	m_csKeyName = lpszKeyName;
	m_nSection = nSection;

	// �f�[�^���ۑ�
	m_bDefault = bDefault;

	// �f�[�^�ǂݏo��
	Load();
}

CConfigBool::~CConfigBool()
{
	// �f�[�^�ۑ�
	Save();
}

void CConfigBool::Load(void)
{
	// �f�[�^���[�h
	m_bValue = (::GetPrivateProfileInt(m_ConfigFile.GetSection(m_nSection), m_csKeyName, (m_bDefault)? 1 : 0, m_ConfigFile.GetPath()) == 0)? FALSE : TRUE;
}

void CConfigBool::Save(void)
{
	// �f�[�^�Z�[�u
	::WritePrivateProfileString(m_ConfigFile.GetSection(m_nSection), m_csKeyName, (m_bValue)? TEXT("1") : TEXT("0"), m_ConfigFile.GetPath());
}

CConfigBool::operator BOOL () const
{
	// �L���X�g���Z�q
	return m_bValue;
}

CConfigBool::operator BOOL & ()
{
	// �L���X�g���Z�q
	return m_bValue;
}

CConfigBool::operator BOOL * ()
{
	// �L���X�g���Z�q
	return &m_bValue;
}

const BOOL & CConfigBool::operator = (const BOOL bValue)
{
	// ������Z�q
	return (m_bValue = bValue);
}


//////////////////////////////////////////////////////////////////////
// CConfigDword �̃C���v�������e�[�V����
//////////////////////////////////////////////////////////////////////

CConfigDword::CConfigDword()
	: m_dwDefault(0UL)
	, m_dwRangeMax(0xFFFFFFFFUL)
	, m_dwRangeMin(0UL)
	, m_dwValue(0UL)
{

}

CConfigDword::CConfigDword(const UINT nSection, LPCTSTR lpszKeyName, const DWORD dwDefault, const DWORD dwRangeMax, const DWORD dwRangeMin)
	: m_dwDefault(0UL)
	, m_dwRangeMax(0xFFFFFFFFUL)
	, m_dwRangeMin(0UL)
	, m_dwValue(0UL)
{
	RegisterKey(nSection, lpszKeyName, dwDefault, dwRangeMax, dwRangeMin);
}

CConfigDword::~CConfigDword()
{
	// �f�[�^�ۑ�
	Save();
}

void CConfigDword::RegisterKey(const UINT nSection, LPCTSTR lpszKeyName, const DWORD dwDefault, const DWORD dwRangeMax, const DWORD dwRangeMin)
{
	// �L�[���ۑ�
	m_csKeyName = lpszKeyName;
	m_nSection = nSection;

	// �f�[�^���ۑ�
	m_dwDefault = dwDefault;
	m_dwRangeMax = dwRangeMax;
	m_dwRangeMin = dwRangeMin;

	// �f�[�^�ǂݏo��
	Load();
}

void CConfigDword::Load(void)
{
	// �f�[�^���[�h
	DWORD dwValue = ::GetPrivateProfileInt(m_ConfigFile.GetSection(m_nSection), m_csKeyName, m_dwDefault, m_ConfigFile.GetPath());

	// �N���b�v
	if(dwValue > m_dwRangeMax)dwValue = m_dwRangeMax;
	if(dwValue < m_dwRangeMin)dwValue = m_dwRangeMin;

	m_dwValue = dwValue;
}

void CConfigDword::Save(void)
{
	// �f�[�^�Z�[�u
	DWORD dwValue = m_dwValue;

	// �N���b�v
	if(dwValue > m_dwRangeMax)dwValue = m_dwRangeMax;
	if(dwValue < m_dwRangeMin)dwValue = m_dwRangeMin;

	CString csValue;
	csValue.Format(TEXT("%lu"), dwValue);

	::WritePrivateProfileString(m_ConfigFile.GetSection(m_nSection), m_csKeyName, csValue, m_ConfigFile.GetPath());
}

CConfigDword::operator DWORD () const
{
	// �L���X�g���Z�q
	return m_dwValue;
}

CConfigDword::operator DWORD & ()
{
	// �L���X�g���Z�q
	return m_dwValue;
}

CConfigDword::operator DWORD * ()
{
	// �L���X�g���Z�q
	return &m_dwValue;
}

const DWORD & CConfigDword::operator = (const DWORD dwValue)
{
	// ������Z�q
	return (m_dwValue = dwValue);
}


//////////////////////////////////////////////////////////////////////
// CConfigString �̃C���v�������e�[�V����
//////////////////////////////////////////////////////////////////////

CConfigString::CConfigString()
	: CString()
	, m_dwLengthMax(1024UL)
{

}

CConfigString::CConfigString(const UINT nSection, LPCTSTR lpszKeyName, LPCTSTR lpszDefault, const DWORD dwLengthMax)
	: CString()
	, m_dwLengthMax(1024UL)
{
	RegisterKey(nSection, lpszKeyName, lpszDefault, dwLengthMax);
}

CConfigString::~CConfigString()
{
	// �f�[�^�ۑ�
	Save();
}

void CConfigString::RegisterKey(const UINT nSection, LPCTSTR lpszKeyName, LPCTSTR lpszDefault, const DWORD dwLengthMax)
{
	// �L�[���ۑ�
	m_csKeyName = lpszKeyName;
	m_nSection = nSection;

	// �f�[�^���ۑ�
	m_csDefault = (lpszDefault)?lpszDefault : TEXT("");
	m_dwLengthMax = dwLengthMax;

	// �f�[�^�ǂݏo��
	Load();
}

void CConfigString::Load(void)
{
	// �f�[�^���[�h
	DWORD dwLength = ::GetPrivateProfileString(m_ConfigFile.GetSection(m_nSection), m_csKeyName, m_csDefault, GetBuffer(m_dwLengthMax + 1), m_dwLengthMax, m_ConfigFile.GetPath());
	ReleaseBuffer();

	if(!dwLength || !GetLength()){
		*this = m_csDefault;
		}
}

void CConfigString::Save(void)
{
	// �f�[�^�Z�[�u
	*this = Left(m_dwLengthMax);
	::WritePrivateProfileString(m_ConfigFile.GetSection(m_nSection), m_csKeyName, (CString &)*this, m_ConfigFile.GetPath());
}

const CString& CConfigString::operator = (const CString& stringSrc)
{
	// ������Z�q
	(CString &)*this = stringSrc;
	return (CString &)*this;
}

const CString& CConfigString::operator = (TCHAR ch)
{
	// ������Z�q
	(CString &)*this = ch;
	return (CString &)*this;
}

const CString& CConfigString::operator = (const unsigned char* psz)
{
	// ������Z�q
	(CString &)*this = psz;
	return (CString &)*this;
}

const CString& CConfigString::operator = (LPCWSTR lpsz)
{
	// ������Z�q
	(CString &)*this = lpsz;
	return (CString &)*this;
}

const CString& CConfigString::operator = (LPCSTR lpsz)
{
	// ������Z�q
	(CString &)*this = lpsz;
	return (CString &)*this;
}

//////////////////////////////////////////////////////////////////////
// CConfigStringArray �̃C���v�������e�[�V����
//////////////////////////////////////////////////////////////////////

CConfigStringArray::CConfigStringArray()
	: CStringArray()
{

}

CConfigStringArray::CConfigStringArray(const UINT nSection, LPCTSTR lpszKeyNameBase)
	: CStringArray()
{
	RegisterKey(nSection, lpszKeyNameBase, NULL, 1024);
}

CConfigStringArray::~CConfigStringArray()
{
	// �f�[�^�ۑ�
	Save();
}

void CConfigStringArray::RegisterKey(const UINT nSection, LPCTSTR lpszKeyName, LPCTSTR lpszDefault, const DWORD dwLengthMax)
{
	// �L�[���ۑ�
	m_csKeyName = lpszKeyName;
	m_nSection = nSection;

	// �f�[�^�ǂݏo��
	Load();
}

void CConfigStringArray::Load(void)
{
	// �f�[�^���[�h
	CString csKey(m_csKeyName), csValue;
	csKey.AppendFormat(TEXT("_Count"));

	int i, iCount = ::GetPrivateProfileInt(m_ConfigFile.GetSection(m_nSection), csKey, 0, m_ConfigFile.GetPath());
	RemoveAll();
	for(i=0; i<iCount; i++){
		csKey = m_csKeyName;
		csKey.AppendFormat(TEXT("_%d"), i);
		DWORD dwLength = ::GetPrivateProfileString(m_ConfigFile.GetSection(m_nSection), csKey, TEXT(""), csValue.GetBuffer(_MAX_PATH + 1), _MAX_PATH, m_ConfigFile.GetPath());
		if(dwLength>0){
			Add(csValue);
			}
		}
}

void CConfigStringArray::Save(void)
{
	// �f�[�^�Z�[�u
	CString csKey(m_csKeyName), csValue;
	csKey.AppendFormat(TEXT("_Count"));

	csValue.Format(TEXT("%lu"), GetCount());
	::WritePrivateProfileString(m_ConfigFile.GetSection(m_nSection), csKey, csValue, m_ConfigFile.GetPath());

	for(int i=0; i<GetCount(); i++){
		csKey = m_csKeyName;
		csKey.AppendFormat(TEXT("_%d"), i);
		::WritePrivateProfileString(m_ConfigFile.GetSection(m_nSection), csKey, GetAt(i), m_ConfigFile.GetPath());
	}
	
	// �s�v�ݒ�̍폜
	TCHAR szSectionValues[1024];
	GetPrivateProfileSection(m_ConfigFile.GetSection(m_nSection), szSectionValues, 1024, m_ConfigFile.GetPath());
	WCHAR *p = szSectionValues;
	int keyLen = m_csKeyName.GetLength();
	while(*p != '\0' && p < szSectionValues + 1024) {
		if (_tcsnicmp(p, m_csKeyName, keyLen) == 0) {
			int no = _tcstol(p + keyLen + 1, NULL, 0);
			if (no >= GetCount()) {
				TCHAR *pk = _tcschr(p, _T('='));
				TCHAR szKey[32];
				_tcsncpy_s(szKey, 32, p, pk - p);
				WritePrivateProfileString(m_ConfigFile.GetSection(m_nSection), szKey, NULL, m_ConfigFile.GetPath());
			}
		}
		p += _tcslen(p) + 1;
	}
}