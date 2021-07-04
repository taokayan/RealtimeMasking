#ifndef DEBUGDIALOG_H
#define DEBUGDIALOG_H

#pragma once


// CDebugDialog 对话框

class CDebugDialog : public CDialog
{
	DECLARE_DYNAMIC(CDebugDialog)

public:
	CDebugDialog(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CDebugDialog();

// 对话框数据
	enum { IDD = IDD_DIALOG_DEBUG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
public:
	afx_msg void OnBnClickedButtonClear();


public:
	void SetMessage(const CString& msg);
	void AppendMessage(const CString& msg);
	
protected:
	CString debugMessage;
};

#endif