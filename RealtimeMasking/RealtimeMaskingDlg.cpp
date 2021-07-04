// RealtimeMaskingDlg.cpp : implementation file
//

#include "stdafx.h"
#include "RealtimeMasking.h"
#include "RealtimeMaskingDlg.h"

#include "Parameters.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CRealtimeMaskingDlg dialog




CRealtimeMaskingDlg::CRealtimeMaskingDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRealtimeMaskingDlg::IDD, pParent), 
	statusBitmap(STATUS_BITMAP_WIDTH,STATUS_BITMAP_HEIGHT,PixelFormat32bppRGB)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	// critical section
	InitializeCriticalSection(&CriticalSection);

	Reset();
}

CRealtimeMaskingDlg::~CRealtimeMaskingDlg()
{
	Reset();
	DeleteCriticalSection(&CriticalSection);
}

void CRealtimeMaskingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CRealtimeMaskingDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDOK, &CRealtimeMaskingDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CRealtimeMaskingDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_TEST0, &CRealtimeMaskingDlg::OnBnClickedButtonTest0)
	ON_BN_CLICKED(IDC_BUTTON_OFFLINE, &CRealtimeMaskingDlg::OnBnClickedButtonOffline)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CRealtimeMaskingDlg::OnBnClickedButtonStop)
END_MESSAGE_MAP()


// CRealtimeMaskingDlg message handlers

BOOL CRealtimeMaskingDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	//pDebugDialog.reset(new CDebugDialog());
	//pDebugDialog->Create(IDD_DIALOG_DEBUG, this);
	//pDebugDialog->ShowWindow(SW_SHOW);

	pOutputDialog.reset(new COutputDialog());
	pOutputDialog->Create(IDD_DIALOG_OUTPUT, this);
	pOutputDialog->ShowWindow(SW_SHOW);

	pControlDialog.reset(new CControlDlg(this));
	pControlDialog->Create(IDD_DIALOG_CONTROL, this);
	pControlDialog->ShowWindow(SW_SHOW);
	pControlDialog->OnBnClickedButtonDefault();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CRealtimeMaskingDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRealtimeMaskingDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}

	Refresh();
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRealtimeMaskingDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CRealtimeMaskingDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	//OnOK(); // never invoke CDialog::OnOk()
}

// ================================================================================================================== //

void CRealtimeMaskingDlg::Reset()
{

	// reset wav file collectors

	waveCollector01.Reset();
	waveCollector23.Reset();

	pSoundCollector01.reset();
	pSoundCollector23.reset();

	// clear buffer
	this->channelBuffer01.clear();
	this->channelBuffer23.clear();
}

void CRealtimeMaskingDlg::MakeDefaultParameters()
{
	Reset();
	deviceIndex01 = DEFAULT_CHANNEL01_DEVICE_INDEX;
	deviceIndex23 = DEFAULT_CHANNEL23_DEVICE_INDEX;
	outputSamplingRate = DEFAULT_OUTPUT_SAMPLING_RATE;
	maskingParameters.MakeDefault();
}

void CRealtimeMaskingDlg::processSoundData(const double *soundData, int bufSize, int nSamples,
										   DWORD additionalData)
{
	EnterCriticalSection(&CriticalSection); 

	/// ===== duplicate sound data=====
	std::vector<float> tempData;
	std::vector<float> overlapBuffer01, overlapBuffer23;

	if( bufSize != maskingKernel.GetFFTSize())
	{
		goto PROCESS_SOUND_DATA_END;
	}

	this->nFramePassed++;

	tempData.resize(maskingKernel.GetFFTSize() * 2,0);
	for(int i = 0; i < maskingKernel.GetFFTSize() * 2; i++)
	{
		tempData[i] = (float)soundData[i];
	}

	if( additionalData == 01) 
	{
		// data related to channel01
		channelBuffer01.push_back(tempData);
	}
	else	
	{
		// data related to channel23
		channelBuffer23.push_back(tempData);
	}

	if( channelBuffer01.size() < 2 || channelBuffer23.size() < 2)
	{
		// not both channel01 and channel23 have enough data, cannot process
		goto PROCESS_SOUND_DATA_END;
	}

	/// ===== masking kernel processing =====
	BOOL bResultAlready;

	// make 50% overlaped data
	overlapBuffer01.resize(maskingKernel.GetFFTSize() * 2);
	overlapBuffer23.resize(maskingKernel.GetFFTSize() * 2);
	for(int i = 0; i < maskingKernel.GetFFTSize(); i++)
	{
		overlapBuffer01[i] = channelBuffer01[0][i + maskingKernel.GetFFTSize()];
		overlapBuffer23[i] = channelBuffer23[0][i + maskingKernel.GetFFTSize()];
		overlapBuffer01[i + maskingKernel.GetFFTSize()] = channelBuffer01[1][i];
		overlapBuffer23[i + maskingKernel.GetFFTSize()] = channelBuffer23[1][i];
	}

	for(int t = 0; t < 2; t++)
	{
		if( t == 0)
		{
			// process original data;
			maskingKernel.Process( &(channelBuffer01[0][0]), &(channelBuffer23[0][0]), bResultAlready);
		}
		else
		{
			// process 50% overlap data
			maskingKernel.Process( &(overlapBuffer01[0]),&(overlapBuffer23[0]), bResultAlready);
		}

		// append output message
		if( maskingKernel.GetDebugString().Compare(L"") != 0)
		{
			pOutputDialog->AppendMessage(CString(maskingKernel.GetDebugString()));
			maskingKernel.EmptyDebugString();
		}

		// ====== write wave files =====
		if( bResultAlready )
		{
			// write source signals
			for(int i = 0; i < 4; i++)
			{
				std::vector<__int16> datatowrite;
				datatowrite.resize( maskingKernel.GetFFTSize() / 2);
				for(int j = 0; j < datatowrite.size(); j++)
				{
					datatowrite[j] = maskingKernel.GetCurrentSignal(i)[j + maskingKernel.GetFFTSize() / 4].re * 32768;
				}
				this->waveWriter[maskingKernel.GetOutputChannelsTotal() + i].ProcessNext(&(datatowrite[0]), datatowrite.size());
			}
			// write separated signals
			std::vector<__int16> datatowrite;
			double multiple = maskingKernel.GetSamplingRate() / (double)outputSamplingRate;
			datatowrite.resize(maskingKernel.GetFFTSize() / multiple / 2);
			for(int i = 0; i < maskingKernel.GetOutputChannelsTotal(); i++)
			{
				for(int j = 0; j < datatowrite.size(); j++)
				{
					datatowrite[j] = 
						maskingKernel.GetCurrentOutput(i)[j * multiple + maskingKernel.GetFFTSize() / 4].re * 32768.0 * 2.0;
				}
				this->waveWriter[i].ProcessNext(&(datatowrite[0]), datatowrite.size());
			}
		}

	}

	// pop buffer
	channelBuffer01.pop_front();
	channelBuffer23.pop_front();

	// ============================= drawings =========================== 
	if(statusBitmap.EasyLock() == TRUE)
	{
		// erase
		int height = statusBitmap.GetHeight();
		int width = statusBitmap.GetWidth();
		statusBitmap.EasyErase(0,0,width,height);
		
		int x0,y0;

		// draw oringin signal
		x0 = y0 = 0;
		for(int i = 0; i < 256; i++)
		{
			float value = maskingKernel.GetCurrentSignal(
				i & 0x1)[i *(maskingKernel.GetSamplingRate() / outputSamplingRate) / 2].re;
			if( (i & 0x1) == 0)
				statusBitmap.EasySetPixel(x0 + i, y0 + value * 30.0f + 32, 0xFF0000/*red*/);
			else
				statusBitmap.EasySetPixel(x0 + i, y0 +value * 30.0f + 32, 0x00FF00/*green*/);
		}
		x0 = 256;
		for(int i = 0; i < 256; i++)
		{
			float value = maskingKernel.GetCurrentSignal(
				1+(i & 0x1))[i *(maskingKernel.GetSamplingRate() / outputSamplingRate) /2].re;
			if( (i & 0x1) == 0)
				statusBitmap.EasySetPixel(x0 + i, y0 + value * 30.0f + 32, 0x00FFFF/*light blue*/);
			else
				statusBitmap.EasySetPixel(x0 + i, y0 + value * 30.0f + 32, 0xFF00FF/*purple*/);
		}

		// draw spectrum
		x0 = 0, y0 = 64;
		for(int i = 1; i < 512; i++)
		{
			float amplitude[3] = {
				maskingKernel.GetCurrentSpectrum(0)[i-1].GetAbsSqrt() * 0.1,
				maskingKernel.GetCurrentSpectrum(0)[i].GetAbsSqrt() * 0.1,
				maskingKernel.GetCurrentSpectrum(0)[i+1].GetAbsSqrt() * 0.1};
			DWORD color;
			if( amplitude[1] < amplitude[0] && amplitude[1] < amplitude[2])
				color = 0x007F00;
			else color = 0xFFFF00;
			if( amplitude[1] >= 32)amplitude[1] = 31;
			if( amplitude[1] <= 0 )amplitude[1] = 1;
			for(int y = 0; y < amplitude[1]; y++)
			{
				statusBitmap.EasySetPixel(x0 + i, y0 + 32 - y, color);
			}
		}

		// draw DUET graph
		double max = 0.1;
		for(int y = 0; y < maskingKernel.GetMaskingGraphSize(); y++)
		{
			//const double* row = &(maskingKernel.GetSmoothedMaskingGraph()[y][0]);
			const float* row = &(maskingKernel.GetMaskingGraph()[y][0]);
			int xsize = maskingKernel.GetMaskingGraphSize();
			for(int x = 0; x < xsize; x++)
			{
				if( max < row[x])
					max = row[x];
			}
		}
		x0 = 0, y0 = 96;
		for(int y = 0; y < maskingKernel.GetMaskingGraphSize(); y++)
		{
			int xsize = maskingKernel.GetMaskingGraphSize();
			for(int x = 0; x < xsize; x++)
			{
				//int value = maskingKernel.GetSmoothedMaskingGraph()[y][x] / max * 255.0;
				int value = maskingKernel.GetMaskingGraph()[y][x] / max * 255.0;
				if( value < 0 )value = 0;
				if( value > 255) value = 255;
				value = (value << 16) | (value << 8 ) | value;
				statusBitmap.EasyFill(x0 + x , y0 + y , 1, 1, value);
				if( x == 0 || y == 0 || x == xsize - 1 || y == maskingKernel.GetMaskingGraphSize() - 1)
				{
					// draw boundary
					statusBitmap.EasySetPixel(x0 + x , y0 + y, 0x7F7F7F);
				}
			}
		}	
		
		// draw results
		COLORREF colortemplate[] = {0xFF0000,0x00FF00,0x0000FF,0xFFFF00,0xFF00FF,0x00FFFF,
			0x7F7F00,0x7F007F,0x007F7F};
		std::vector<COLORREF> labelcolor;
		labelcolor.resize(maskingKernel.GetOutputChannelsTotal() + 1);
		labelcolor[0] = 0xFFFFFF;
		for(int i = 0; i < maskingKernel.GetOutputChannelsTotal(); i++)
		{
			labelcolor[ i + 1] = colortemplate[i % (sizeof(colortemplate) / sizeof(COLORREF))];
		}
		for(int i = 0; i < maskingKernel.GetResult().size(); i++)
		{
			int x = maskingKernel.GetResult()[i].x ;
			int y = maskingKernel.GetResult()[i].y ;
			COLORREF col = labelcolor[i + 1];
			statusBitmap.EasySetPixel(x0 + x - 4, y0 + y - 1 , col);
			statusBitmap.EasySetPixel(x0 + x - 4, y0 + y, col);
			statusBitmap.EasySetPixel(x0 + x - 4, y0 + y + 1 , col);
			statusBitmap.EasySetPixel(x0 + x + 4, y0 + y - 1 , col);
			statusBitmap.EasySetPixel(x0 + x + 4, y0 + y, col);
			statusBitmap.EasySetPixel(x0 + x + 4, y0 + y + 1 , col);
			statusBitmap.EasySetPixel(x0 + x - 1, y0 + y - 4 , col);
			statusBitmap.EasySetPixel(x0 + x, y0 + y - 4 , col);
			statusBitmap.EasySetPixel(x0 + x + 1, y0 + y - 4 , col);
			statusBitmap.EasySetPixel(x0 + x - 1, y0 + y + 4 , col);
			statusBitmap.EasySetPixel(x0 + x, y0 + y + 4 , col);
			statusBitmap.EasySetPixel(x0 + x + 1, y0 + y + 4 , col);
		}

		// draw output spectrums
		x0 = maskingKernel.GetMaskingGraphSize(), y0 = 96;
		for(int i = 0; i < maskingKernel.GetOutputChannelsTotal(); i++)
		{
			const int height = 30;
			const int width = 500 - x0;
			for(int j = 0; j < width; j++)
			{
				double value = maskingKernel.GetCurrentOutputSpectrum(i)[j].GetAbsSqrt() * 0.1;
				if( value > height)value = height;
				for(int k = 0; k <= value; k++)
				{
					statusBitmap.EasySetPixel(x0 + j, y0 + i * height + height - k, labelcolor[i+1]);
				}
			}
		}

		statusBitmap.EasyUnlock();
		Refresh();	
	}

PROCESS_SOUND_DATA_END:
	LeaveCriticalSection(&CriticalSection);
	return;
}

void CRealtimeMaskingDlg::Refresh()
{
	CDC* pdc = this->GetDC();
	Gdiplus::Graphics graphics((HDC)(*pdc));
	graphics.DrawImage(&statusBitmap, 2,2);
	this->ReleaseDC(pdc);
}

void CRealtimeMaskingDlg::OnBnClickedButtonTest0()
{
	// TODO: 在此添加控件通知处理程序代码
	Reset();
	
	// masking kernel initialization
	maskingKernel.Reset(maskingParameters);
	nFramePassed = 0;

	// sound card collector init
	pSoundCollector01.reset(new CSoundCollector(2, maskingParameters.samplingRate,
		maskingParameters.frameSize, deviceIndex01)); //channel 01
	pSoundCollector01->RegisterListener(this, 01);
	pSoundCollector23.reset(new CSoundCollector(2, maskingParameters.samplingRate,
		maskingParameters.frameSize, deviceIndex23));	//channel 23
	pSoundCollector23->RegisterListener(this, 23);

	// wave writer init
	waveWriter.resize(maskingParameters.nOutputChannels + 4);
	for(int i = 0; i < maskingParameters.nOutputChannels; i++)
	{
		CString name;
		name.Format(_T("Outputs\\out_%d.wav"),i+1);
		waveWriter[i].FilePrepare(name, outputSamplingRate);
	}
	for(int i = 0; i < 4; i++)
	{
		CString name;
		name.Format(_T("Outputs\\%d.wav"), i+1);
		waveWriter[maskingParameters.nOutputChannels + i].FilePrepare(name, maskingParameters.samplingRate);
	}

	// start all realtime collectors
	pSoundCollector01->Start();
	pSoundCollector23->Start();
	nFramePassed = 0;
}

void CRealtimeMaskingDlg::OnBnClickedCancel()
{
	Reset();
	pSoundCollector01.reset();
	pSoundCollector23.reset();
	CDialog::OnCancel();
}

void CRealtimeMaskingDlg::OnBnClickedButtonOffline()
{
	// TODO: Add your control notification handler code here
	Reset();
	
	// masking kernel initialization
	maskingKernel.Reset(maskingParameters);
	nFramePassed = 0;

	// wave writer init
	waveWriter.resize(maskingParameters.nOutputChannels + 4);
	for(int i = 0; i < maskingParameters.nOutputChannels; i++)
	{
		CString name;
		name.Format(_T("Outputs\\out_%d.wav"),i+1); // for separation results
		waveWriter[i].FilePrepare(name, outputSamplingRate);
	}
	for(int i = 0; i < 4; i++)
	{
		CString name;
		name.Format(_T("Outputs\\%d.wav"), i+1); // for original recording sound
		waveWriter[maskingParameters.nOutputChannels + i].FilePrepare(name, maskingParameters.samplingRate);
	}

	// wave collector init & start
	waveCollector01.Reset();
	waveCollector23.Reset();
	waveCollector01.RegisterListener(this, 01);
	waveCollector23.RegisterListener(this, 23);
	waveCollector01.Start(_T("SampleData\\1.wav"), _T("SampleData\\2.wav"), maskingParameters.frameSize, maskingParameters.samplingRate);
	waveCollector23.Start(_T("SampleData\\3.wav"), _T("SampleData\\4.wav"), maskingParameters.frameSize, maskingParameters.samplingRate);
	nFramePassed = 0;
}

void CRealtimeMaskingDlg::OnBnClickedButtonStop()
{
	// TODO: Add your control notification handler code here
	Reset();
	for(int i = 0; i < this->waveWriter.size(); i++)
	{
		waveWriter[i].Close();
	}
}
