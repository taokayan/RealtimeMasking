// RealtimeMaskingDlg.h : header file
//
#ifndef REALTIMEMASKINGDLG_H
#define REALTIMEMASKINGDLG_H

#pragma once

#include "EasyBitmap.h"
#include "RealtimeMaskingKernel.h"
#include "ControlDlg.h"
#include "DebugDialog.h"
#include "OutputDialog.h"

#include "WaveCollector.h"
#include "SoundWriter.h"

#include <queue>

class CControlDlg;

// CRealtimeMaskingDlg dialog
class CRealtimeMaskingDlg : public CDialog, ISoundCollectorEvent
{

	//--------------------MFC Built In---------------------------
// Construction
public:
	CRealtimeMaskingDlg(CWnd* pParent = NULL);	// standard constructor
	virtual ~CRealtimeMaskingDlg();

// Dialog Data
	enum { IDD = IDD_REALTIMEMASKING_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

protected:
	HICON m_hIcon;
	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()


public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedButtonTest0();
	afx_msg void OnBnClickedButtonOffline();
	afx_msg void OnBnClickedButtonStop();



	//------------------------Kernel Data--------------------------
public:
	void Refresh();
	void Reset();

protected:

	// Critical Section prevents more than one thread entering functions at the same time
	CRITICAL_SECTION CriticalSection;	

	// A Bitmap displaying current status
	EasyBitmap statusBitmap;
	int nFramePassed;

	//
	// --- For Sound Collection Module ---
	//
	// realtime sound card collector instances
	std::auto_ptr<CSoundCollector> pSoundCollector01;
	std::auto_ptr<CSoundCollector> pSoundCollector23;

	// offline wave file collector
	CWaveCollector waveCollector01;
	CWaveCollector waveCollector23;
	int deviceIndex01;
	int deviceIndex23;
	std::deque<std::vector<float> > channelBuffer01;
	std::deque<std::vector<float> > channelBuffer23;

	// wave output instances
	std::vector<SoundWriter::Sound2File> waveWriter;
	int outputSamplingRate;

	// helper
	FFT_T<float> fftEngine;

	// implement ISoundCollectorEvent
	virtual void processSoundData(const double *soundData, int bufSize, int nSamples,
		DWORD additionalData);
	void MakeDefaultParameters();

	// masking kernel
	RealtimeMaskingKernel maskingKernel;
	RealtimeMaskingParameters maskingParameters;

	// ------------------- UIs ------------------
	std::auto_ptr<CDebugDialog> pDebugDialog;
	std::auto_ptr<COutputDialog> pOutputDialog;
	std::auto_ptr<CControlDlg> pControlDialog;

	friend class CControlDlg;
};


#endif 