#include "StdAfx.h"
#include "WaveCollector.h"

#include <vector>

CWaveCollector::CWaveCollector(void)
{
	threadHandle = NULL;	
	InitializeCriticalSection(&CriticalSection);
}

CWaveCollector::~CWaveCollector(void)
{
	Reset();
	DeleteCriticalSection(&CriticalSection);
}

void CWaveCollector::Reset()
{
	EnterCriticalSection(&CriticalSection);

	if( threadHandle != NULL)
	{
		bStopSignal = TRUE;
		LeaveCriticalSection(&CriticalSection);
		WaitForMultipleObjects( 1, &threadHandle, TRUE, INFINITE);
		EnterCriticalSection(&CriticalSection);
		//ASSERT(TerminateThread(threadHandle, 0) != 0);
		CloseHandle(threadHandle);
		threadHandle = NULL;
	}
	if( soundData[0].m_hFile != INVALID_HANDLE_VALUE)
		soundData[0].Close();
	if( soundData[1].m_hFile != INVALID_HANDLE_VALUE)
		soundData[1].Close();
	UnregisterAllListeners();

	LeaveCriticalSection(&CriticalSection);
}

BOOL CWaveCollector::Start(LPCTSTR inLeftChannelFile, LPCTSTR inRightChannelFile, int inBufSize, 
						   int inOutputSamplingRate, int inOverlapBufSize, int inRepeatTime)
{
	EnterCriticalSection(&CriticalSection); 

	if( threadHandle != NULL)
	{
		bStopSignal = TRUE;
		LeaveCriticalSection(&CriticalSection);
		WaitForMultipleObjects( 1, &threadHandle, TRUE, INFINITE);
		EnterCriticalSection(&CriticalSection);
		//ASSERT(TerminateThread(threadHandle, 0)!=0);
		CloseHandle(threadHandle);
		threadHandle = NULL;
	}
	if( soundData[0].m_hFile != INVALID_HANDLE_VALUE)
		soundData[0].Close();
	if( soundData[1].m_hFile != INVALID_HANDLE_VALUE)
		soundData[1].Close();

	if( soundData[0].Open(inLeftChannelFile, CFile::modeRead) != TRUE || 
		soundData[1].Open(inRightChannelFile, CFile::modeRead) != TRUE)
	{
		goto FAILED_RETURN;
	}
	
	this->outSamplingRate = inOutputSamplingRate;
	this->bufSize = inBufSize;
	this->overlapBufSize = inOverlapBufSize;

	this->blankBuffer = 1;
	this->repeatTime = inRepeatTime;

	// retrieve sampling rate of current wave file
	soundData[0].Seek(24, CFile::SeekPosition::begin);
	soundData[0].Read(&(this->fileSamplingRate), 4);

	// seek to the start of the data blocks
	soundData[0].Seek(44, CFile::SeekPosition::begin);
	soundData[1].Seek(44, CFile::SeekPosition::begin);

	threadHandle = CreateThread( NULL, 0, 
           CWaveCollector_collection_thread, this, 0, NULL);

	if( threadHandle == NULL) goto FAILED_RETURN; // thread creation failed...

	bStopSignal = FALSE;
	bFinished = FALSE;

SUCCEED_RETURN:
	LeaveCriticalSection(&CriticalSection);
	return TRUE;

FAILED_RETURN:
	LeaveCriticalSection(&CriticalSection);
	return FALSE;
}

void CWaveCollector::RegisterListener(ISoundCollectorEvent *listener, int param)
{
	EnterCriticalSection(&CriticalSection); 
	this->callBackList.push_back(listener);
	this->additionalDataList.push_back(param);
	LeaveCriticalSection(&CriticalSection);
}

void CWaveCollector::UnregisterListener(ISoundCollectorEvent* listener)
{
	EnterCriticalSection(&CriticalSection); 
	for(int i = 0; i < callBackList.size(); i++)
	{
		if( callBackList[i] == listener)
		{
			callBackList[i] = callBackList[callBackList.size() - 1];
			additionalDataList[i] = additionalDataList[additionalDataList.size() - 1];
			callBackList.pop_back();
			additionalDataList.pop_back();
		}
	}
	LeaveCriticalSection(&CriticalSection);
}

void CWaveCollector::UnregisterAllListeners()
{
	EnterCriticalSection(&CriticalSection); 
	callBackList.clear();
	additionalDataList.clear();
	LeaveCriticalSection(&CriticalSection);
}

void CWaveCollector::OnThreadEnd()
{
}

double Quintic_Interpolation(const double& x1, const double& x2, const double& x3,
							 const double& x4, const double& x5, const double& x6,
							 const double& x)

{
	double a,b,c,d,e,f,P,Q,R;
	double xx,xxx,xxxx,xxxxx;
	f = x3;
	e = (x4 - x2) * 0.5;
	d = (x4 - 2 * x3 + x2) / 2;
	P = x4 - d - e - f;
	Q = (x5 - x3) * 0.5 - 2 * d - e;
	R = (x5 - 2 * x4 + x3) - 2 * d;
	a = -0.5 * (-12 * P + 6 * Q - R);
	b = -15 * P + 7 * Q - R;
	c = -0.5 * (-20 * P + 8 * Q - R);
	xx = x * x;
	xxx = xx * x;
	xxxx = xxx * x;
	xxxxx = xxxx * x;
	double r = a * xxxxx + b * xxxx + c * xxx + d * xx + e * x + f;
	return r;
}

float Cubic_Interpolation(float x1,float x2,float x3,float x4,float x)
{
	float r;
	float a,b,c,d;
	d = x2;
	c = (x3 - x1) / 2.0;
	float a_plus_b = x3 - c - d;
	float _3a_plus_2b = (x4 - x2) / 2.0 - c;
	a = _3a_plus_2b - 2 * a_plus_b;
	b = 3 * a_plus_b - _3a_plus_2b;
	r = a * x * x * x + b * x * x + c * x + d; 

	return r;
}


DWORD WINAPI CWaveCollector_collection_thread(LPVOID lpParam)
{
	CWaveCollector* This = (CWaveCollector*)lpParam;
	std::vector<__int16> signal[2];
	std::vector<double> normalsignal;

	int extraSamples = 8; // extraSamples are used for interpolation
	int nSamplesRead = This->bufSize * This->fileSamplingRate / This->outSamplingRate + extraSamples;

	signal[0].resize(nSamplesRead);
	signal[1].resize(nSamplesRead);
	normalsignal.resize(This->bufSize * 2);

	EnterCriticalSection(&This->CriticalSection); 
	
	while(1)
	{
		if( This->bStopSignal == TRUE)
		{
			break;
		}

		// read signals from files
		int nRead0,nRead1;
		nRead0 = This->soundData[0].Read(&(signal[0][0]), 2 * nSamplesRead);
		nRead1 = This->soundData[1].Read(&(signal[1][0]), 2 * nSamplesRead);
		if( (nRead0 == 0 || nRead1 == 0) && This->blankBuffer == 0 && This->repeatTime == 0)
		{
			break; // all stop
		}
		else if( (nRead0 == 0 || nRead1 == 0) && This->blankBuffer == 0)
		{
			//roll back;
			This->repeatTime--;
			This->blankBuffer = 1;
			This->soundData[0].Seek(44, CFile::SeekPosition::begin);
			This->soundData[1].Seek(44, CFile::SeekPosition::begin);
			continue;
		}
		else if(nRead0 == 0 || nRead1 == 0)
		{
			// empty buffer read 
			This->blankBuffer--;
		}
		else if(nRead0 != 2 * nSamplesRead || nRead1 != 2 * nSamplesRead)
		{
			// unfull buffer read
			This->soundData[0].SeekToEnd();
			This->soundData[1].SeekToEnd();
		}
		else
		{
			// full buffer read
			This->soundData[0].Seek(-extraSamples * 2 + This->overlapBufSize, CFile::SeekPosition::current);
			This->soundData[1].Seek(-extraSamples * 2 + This->overlapBufSize, CFile::SeekPosition::current);
		}
		memset((char*)(&(signal[0][0])) + nRead0, 0, 2 * nSamplesRead - nRead0);
		memset((char*)(&(signal[1][0])) + nRead0, 0, 2 * nSamplesRead - nRead1);


		// cubic interpolation , normalize and rearrange all sound data
		for(int i = 0; i < This->bufSize; i++)
		{
			double x = double(i) *  This->fileSamplingRate / This->outSamplingRate + 2.0;
			int ix = (int)x;
			x = x - ix;

			normalsignal[i * 2] = 
				Cubic_Interpolation(signal[0][ix - 1],signal[0][ix],signal[0][ix + 1],signal[0][ix + 2],x) / 32768.0;
			normalsignal[i * 2 + 1] = 
				Cubic_Interpolation(signal[1][ix - 1],signal[1][ix],signal[1][ix + 1],signal[1][ix + 2],x) / 32768.0;
		}

		// for each call back object, invoke its callback function
		for(int i = 0; i < This->callBackList.size(); i++)
		{
			This->callBackList[i]->processSoundData(&(normalsignal[0]), 
				This->bufSize, This->outSamplingRate, This->additionalDataList[i]);
		}

		LeaveCriticalSection(&This->CriticalSection);
		Yield() // implicit yield 
		EnterCriticalSection(&This->CriticalSection); 
	}

	This->bFinished = TRUE;
	LeaveCriticalSection(&This->CriticalSection);


	return 0;
}

BOOL CWaveCollector::IsFinished()
{
	BOOL r;
	EnterCriticalSection(&CriticalSection); 
	r = bFinished;
	LeaveCriticalSection(&CriticalSection);
	return r;
}
