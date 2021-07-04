
// OutputDialog.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "RealtimeMasking.h"
#include "OutputDialog.h"


// COutputDialog �Ի���

IMPLEMENT_DYNAMIC(COutputDialog, CDialog)

COutputDialog::COutputDialog(CWnd* pParent /*=NULL*/)
	: CDialog(COutputDialog::IDD, pParent)
{

}

COutputDialog::~COutputDialog()
{
}

void COutputDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(COutputDialog, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR, &COutputDialog::OnBnClickedButtonClear)
	ON_BN_CLICKED(IDOK, &COutputDialog::OnBnClickedOk)
END_MESSAGE_MAP()


// COutputDialog ��Ϣ�������

void COutputDialog::OnBnClickedButtonClear()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_OUTPUT);
	pEdit->SetWindowText(_T(""));
}

void COutputDialog::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	//OnOK();
}

void COutputDialog::SetMessage(const CString& msg)
{
	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_OUTPUT);
	pEdit->SetWindowText(msg);
	pEdit->LineScroll(pEdit->GetLineCount());
}

void COutputDialog::AppendMessage(const CString& msg)
{
	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_OUTPUT);
	CString s;
	pEdit->GetWindowText(s);
	SetMessage(s + msg);
}

