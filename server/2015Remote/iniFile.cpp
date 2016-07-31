#include "StdAfx.h"
#include "iniFile.h"


iniFile::iniFile(void)
{

	ContructIniFile();
}


BOOL iniFile::ContructIniFile()
{
	char  szFilePath[MAX_PATH] = {0};
	char* FindPoint = NULL;
	int  len;
	
	::GetModuleFileName(NULL, szFilePath, sizeof(szFilePath));    


	FindPoint = strrchr(szFilePath,'.');
	if (FindPoint!=NULL)
	{
		*FindPoint = '\0';     
		strcat(szFilePath,".ini");    
	}

	m_IniFilePath = szFilePath;                                    //��ֵ���ļ��� �鿴 һ����Ա����  IniFileName

	HANDLE hFile = CreateFileA(m_IniFilePath,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);   //ͬ��  �첽   

	if (hFile==INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}


	ULONG ulLow = GetFileSize(hFile,NULL);

	if (ulLow>0)
	{
		CloseHandle(hFile);

		return FALSE;
	}

	CloseHandle(hFile);
	
	WritePrivateProfileString("Settings", "ListenPort","2356",m_IniFilePath);
	WritePrivateProfileString("Settings", "MaxConnection","10000",m_IniFilePath);

	return TRUE;
}
int iniFile::GetInt(CString MainKey,CString SubKey)   //"Setting" "ListenPort"
{
	return ::GetPrivateProfileInt(MainKey, SubKey,0,m_IniFilePath);
}


BOOL iniFile::SetInt(CString MainKey,CString SubKey,int Data)//8888
{
	CString strData;
	strData.Format("%d", Data);  //2356  
	return ::WritePrivateProfileString(MainKey, SubKey,strData,m_IniFilePath);
}

iniFile::~iniFile(void)
{

}
