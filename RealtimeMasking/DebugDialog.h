#ifndef DEBUGDIALOG_H
#define DEBUGDIALOG_H

#pragma once


// CDebugDialog �Ի���

class CDebugDialog : public CDialog
{
	DECLARE_DYNAMIC(CDebugDialog)

public:
	CDebugDialog(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CDebugDialog();

// �Ի�������
	enum { IDD = IDD_DIALOG_DEBUG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

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