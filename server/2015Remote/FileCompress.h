#pragma once


// CFileCompress �Ի���

class CFileCompress : public CDialog
{
	DECLARE_DYNAMIC(CFileCompress)

public:
	CFileCompress(CWnd* pParent = NULL,ULONG n = 0);   // ��׼���캯��
	virtual ~CFileCompress();

// �Ի�������
	enum { IDD = IDD_DIALOG_FILE_COMPRESS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	CString m_EditRarName;
	ULONG   m_ulType;
	virtual BOOL OnInitDialog();
};
