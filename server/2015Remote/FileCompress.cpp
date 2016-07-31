// FileCompress.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "2015Remote.h"
#include "FileCompress.h"
#include "afxdialogex.h"


// CFileCompress �Ի���

IMPLEMENT_DYNAMIC(CFileCompress, CDialog)

CFileCompress::CFileCompress(CWnd* pParent /*=NULL*/,ULONG n)
	: CDialog(CFileCompress::IDD, pParent)
	, m_EditRarName(_T(""))
{
	m_ulType = n;
}

CFileCompress::~CFileCompress()
{
}

void CFileCompress::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_RARNAME, m_EditRarName);
}


BEGIN_MESSAGE_MAP(CFileCompress, CDialog)
END_MESSAGE_MAP()


// CFileCompress ��Ϣ�������


BOOL CFileCompress::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString strTips;
	switch(m_ulType)
	{
	case 1:
		{
			strTips = "������ѹ���ļ�����";
			SetDlgItemText(IDC_STATIC, strTips);
			break;
		}
	case 2:
		{
			strTips = "�������ѹ�ļ��У�";
			SetDlgItemText(IDC_STATIC, strTips);
			break;
		}
	case 3:
		{
			strTips = "������Զ��ѹ���ļ�����";
			SetDlgItemText(IDC_STATIC, strTips);
			break;
		}
	case 4:
		{
			strTips = "������Զ�̽�ѹ�ļ��У�";
			SetDlgItemText(IDC_STATIC, strTips);
		}
	}


	UpdateData(TRUE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣: OCX ����ҳӦ���� FALSE
}
