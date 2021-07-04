
#ifndef CONTROLDLG_H
#define CONTROLDLG_H

#pragma once

#include "RealtimeMaskingDlg.h"
#include "RealtimeMaskingKernel.h"

// CControlDlg 对话框

class CRealtimeMaskingDlg;

class CControlDlg : public CDialog
{
	DECLARE_DYNAMIC(CControlDlg)

public:
	CControlDlg(CRealtimeMaskingDlg* parent);   // 标准构造函数
	virtual ~CControlDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_CONTROL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButtonRealtime();
	afx_msg void OnBnClickedButtonDefault();

protected:
	BOOL GetParametersFromUI(CString& outMessage);
	BOOL UpdateUIParameters();
	BOOL IsPowerOfTwo(int num);
	BOOL IsInRange(int num, int low, int high);
	BOOL IsInRange(double num, double low, double high);
	BOOL IsInRange(float num, float low, float high);

	int deviceid1;
	int deviceid2;
	int outputsamplingrate;
	RealtimeMaskingParameters paras;
	CRealtimeMaskingDlg* parent;

public:
	afx_msg void OnBnClickedButtonOfftime();
public:
	afx_msg void OnBnClickedButtonStop2();

	friend class CRealimeMaskingDlg;
};


#endif