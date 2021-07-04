// ControlDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "RealtimeMasking.h"
#include "ControlDlg.h"

#include "Parameters.h"
#include "RealtimeMaskingDlg.h"


// CControlDlg 对话框

IMPLEMENT_DYNAMIC(CControlDlg, CDialog)

CControlDlg::CControlDlg(CRealtimeMaskingDlg* pParent /*=NULL*/)
	: CDialog(CControlDlg::IDD, NULL)
{
	parent = pParent;
}

CControlDlg::~CControlDlg()
{
}

void CControlDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CControlDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CControlDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON_REALTIME, &CControlDlg::OnBnClickedButtonRealtime)
	ON_BN_CLICKED(IDC_BUTTON_DEFAULT, &CControlDlg::OnBnClickedButtonDefault)
	ON_BN_CLICKED(IDC_BUTTON_OFFTIME, &CControlDlg::OnBnClickedButtonOfftime)
	ON_BN_CLICKED(IDC_BUTTON_STOP2, &CControlDlg::OnBnClickedButtonStop2)
END_MESSAGE_MAP()


// CControlDlg 消息处理程序

void CControlDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	//OnOK();
}

BOOL CControlDlg::IsPowerOfTwo(int num)
{
	if( num <= 0)return FALSE;
	while( (num & 0x1) == 0 ) num >>= 1;
	if( num == 1) return TRUE;
	return FALSE;
}

BOOL CControlDlg::IsInRange(int num, int low, int high)
{
	return (num >= low && num <= high);
}

BOOL CControlDlg::IsInRange(double num, double low, double high)
{
	return (num > low && num < high);
}

BOOL CControlDlg::IsInRange(float num, float low, float high)
{
	return (num > low && num < high);
}

BOOL CControlDlg::UpdateUIParameters()
{
	this->paras = parent->maskingParameters;
	this->deviceid1 = parent->deviceIndex01;
	this->deviceid2 = parent->deviceIndex23;
	this->outputsamplingrate = parent->outputSamplingRate;

	CEdit* pEdit;
	CString str;

	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_IN_SAMPLE_RATE);
	str.Format(_T("%d"),paras.samplingRate);
	pEdit->SetWindowTextW(str);

	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_DEV_ID1);
	str.Format(_T("%d"),deviceid1);
	pEdit->SetWindowTextW(str);

	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_DEV_ID2);
	str.Format(_T("%d"),deviceid2);
	pEdit->SetWindowTextW(str);

	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_FFT);
	str.Format(_T("%d"),paras.frameSize);
	pEdit->SetWindowTextW(str);

	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_LOCAL_FFT);
	str.Format(_T("%d"),paras.localizationFFTSize);
	pEdit->SetWindowTextW(str);

	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_ATTENUATION);
	str.Format(_T("%lf"),paras.realtimeAttenuation);
	pEdit->SetWindowTextW(str);

	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_DUET_SIZE);
	str.Format(_T("%d"),paras.maskingGraphSize);
	pEdit->SetWindowTextW(str);

	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_ZOOMIN_SIZE);
	str.Format(_T("%d"),paras.maskingGraphZoomInSize);
	pEdit->SetWindowTextW(str);

	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_SEPARATION_NUM);
	str.Format(_T("%d"),paras.nOutputChannels);
	pEdit->SetWindowTextW(str);

	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_OUT_SAMPLE_RATE);
	str.Format(_T("%d"),outputsamplingrate);
	pEdit->SetWindowTextW(str);

	return TRUE;
}

BOOL CControlDlg::GetParametersFromUI(CString& msg)
{
	CEdit* pEdit;
	CString str;
	BOOL succ = 1;

	msg = _T("");

	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_IN_SAMPLE_RATE);
	pEdit->GetWindowTextW(str);
	if( swscanf(str,_T("%d"),&paras.samplingRate) != 1 || paras.samplingRate <= 0)
	{
		msg += _T("Error Parsing Input Sampling Rate\r\n");
		succ = 0;
	}

	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_DEV_ID1);
	pEdit->GetWindowTextW(str);
	if( swscanf(str,_T("%d"),&deviceid1) != 1 || deviceid1 < 0)
	{
		msg += _T("Error Parsing device id 1\r\n");
		succ = 0;
	}

	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_DEV_ID2);
	pEdit->GetWindowTextW(str);
	if( swscanf(str,_T("%d"),&deviceid2) != 1 || deviceid2 < 0)
	{
		msg += _T("Error Parsing device id 1\r\n");
		succ = 0;
	}

	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_FFT);
	pEdit->GetWindowTextW(str);
	if( swscanf(str,_T("%d"),&paras.frameSize) != 1 || 
		IsPowerOfTwo(paras.frameSize) == FALSE)
	{
		msg += _T("Error Parsing FFT Size\r\n");
		succ = 0;
	}

	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_LOCAL_FFT);
	pEdit->GetWindowTextW(str);
	if( swscanf(str,_T("%d"),&paras.localizationFFTSize) != 1 ||
		IsPowerOfTwo(paras.localizationFFTSize) == FALSE)
	{
		msg += _T("Error Parsing Localization FFT Size\r\n");
		succ = 0;
	}

	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_ATTENUATION);
	pEdit->GetWindowTextW(str);
	if( swscanf(str,_T("%f"),&paras.realtimeAttenuation) != 1 ||
		IsInRange(paras.realtimeAttenuation, 0.0f, 1.0f) == FALSE)
	{
		msg += _T("Error Parsing Realtime Attenuation\r\n");
		succ = 0;
	}

	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_DUET_SIZE);
	pEdit->GetWindowTextW(str);
	if( swscanf(str,_T("%d"),&paras.maskingGraphSize) != 1 ||
		IsInRange(paras.maskingGraphSize, 16, 256) == FALSE)
	{
		msg += _T("Error Parsing Masking Graph Size\r\n");
		succ = 0;
	}

	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_ZOOMIN_SIZE);
	pEdit->GetWindowTextW(str);
	if( swscanf(str,_T("%d"),&paras.maskingGraphZoomInSize) != 1 ||
		IsInRange(paras.maskingGraphZoomInSize, 1, 32) == FALSE)
	{
		msg += _T("Error Parsing Masking Graph Zoom In Size\r\n");
		succ = 0;
	}

	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_SEPARATION_NUM);
	pEdit->GetWindowTextW(str);
	if( swscanf(str,_T("%d"),&paras.nOutputChannels) != 1 ||
		IsInRange(paras.nOutputChannels, 2, 8) == FALSE)
	{
		msg += _T("Error Parsing separation number\r\n");
		succ = 0;
	}

	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_OUT_SAMPLE_RATE);
	pEdit->GetWindowTextW(str);
	if( swscanf_s(str,_T("%d"),&outputsamplingrate) != 1 ||
		outputsamplingrate < 0)
	{
		msg += _T("Error Parsing output sampling rate\r\n");
		succ = 0;
	}

	return succ;
}

void CControlDlg::OnBnClickedButtonOfftime()
{
	// TODO: 在此添加控件通知处理程序代码
	CString msg;
	if( GetParametersFromUI(msg) == TRUE)
	{
		parent->Reset();
		parent->maskingParameters = paras;
		parent->deviceIndex01 = deviceid1;
		parent->deviceIndex23 = deviceid2;
		parent->outputSamplingRate = outputsamplingrate;
		UpdateUIParameters();
		parent->OnBnClickedButtonOffline();
	}
	else
	{
		MessageBox(msg, _T("Parameter Error"),MB_OK);
	}
}

void CControlDlg::OnBnClickedButtonStop2()
{
	// TODO: 在此添加控件通知处理程序代码
	parent->OnBnClickedButtonStop();
}

void CControlDlg::OnBnClickedButtonRealtime()
{
	// TODO: Add your control notification handler code here
	CString msg;
	if( GetParametersFromUI(msg) == TRUE)
	{
		parent->Reset();
		parent->maskingParameters = paras;
		parent->deviceIndex01 = deviceid1;
		parent->deviceIndex23 = deviceid2;
		parent->outputSamplingRate = outputsamplingrate;
		UpdateUIParameters();
		parent->OnBnClickedButtonTest0();
	}
	else
	{
		MessageBox(msg, _T("Parameter Error"),MB_OK);
	}
}

void CControlDlg::OnBnClickedButtonDefault()
{
	// TODO: Add your control notification handler code here
	parent->Reset();
	parent->MakeDefaultParameters();
	UpdateUIParameters();
}
