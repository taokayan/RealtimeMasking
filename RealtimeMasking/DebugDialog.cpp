// DebugDialog.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "RealtimeMasking.h"
#include "DebugDialog.h"


// CDebugDialog �Ի���

IMPLEMENT_DYNAMIC(CDebugDialog, CDialog)

CDebugDialog::CDebugDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CDebugDialog::IDD, pParent)
{
	debugMessage = _T("");
}

CDebugDialog::~CDebugDialog()
{
}

void CDebugDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDebugDialog, CDialog)
	ON_BN_CLICKED(IDOK, &CDebugDialog::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR, &CDebugDialog::OnBnClickedButtonClear)
END_MESSAGE_MAP()


// CDebugDialog ��Ϣ�������

void CDebugDialog::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	//OnOK();
}

void CDebugDialog::OnBnClickedButtonClear()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	debugMessage = _T("");
	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_DEBUG);
	pEdit->SetWindowText(debugMessage);
}

void CDebugDialog::SetMessage(const CString &msg)
{
	debugMessage = msg;
	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_DEBUG);
	pEdit->SetWindowText(debugMessage);
	pEdit->LineScroll(pEdit->GetLineCount());
}

void CDebugDialog::AppendMessage(const CString& msg)
{
	CString s;
	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_DEBUG);
	pEdit->GetWindowText(s);
	SetMessage(s + msg);
}