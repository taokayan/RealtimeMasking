#ifndef OUTPUTDIALOG_H
#define OUTPUTDIALOG_H

#pragma once


// COutputDialog �Ի���

class COutputDialog : public CDialog
{
	DECLARE_DYNAMIC(COutputDialog)

public:
	COutputDialog(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~COutputDialog();

// �Ի�������
	enum { IDD = IDD_DIALOG_OUTPUT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

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