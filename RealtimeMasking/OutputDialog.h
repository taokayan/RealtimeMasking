#ifndef OUTPUTDIALOG_H
#define OUTPUTDIALOG_H

#pragma once


// COutputDialog 对话框

class COutputDialog : public CDialog
{
	DECLARE_DYNAMIC(COutputDialog)

public:
	COutputDialog(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~COutputDialog();

// 对话框数据
	enum { IDD = IDD_DIALOG_OUTPUT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonClear();
public:
	afx_msg void OnBnClickedOk();

public:
	void SetMessage(const CString& msg);
	void AppendMessage(const CString& msg);

};

#endif