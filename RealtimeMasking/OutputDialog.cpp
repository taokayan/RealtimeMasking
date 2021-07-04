
// OutputDialog.cpp : 实现文件
//

#include "stdafx.h"
#include "RealtimeMasking.h"
#include "OutputDialog.h"


// COutputDialog 对话框

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


// COutputDialog 消息处理程序

void COutputDialog::OnBnClickedButtonClear()
{
	// TODO: 在此添加控件通知处理程序代码
	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_OUTPUT);
	pEdit->SetWindowText(_T(""));
}

void COutputDialog::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
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

