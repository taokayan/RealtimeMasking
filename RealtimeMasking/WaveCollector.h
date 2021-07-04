#pragma once

#include "stdafx.h"
#include <vector>


class CWaveCollector
{
public:
	CWaveCollector(void);
	virtual ~CWaveCollector(void);

	void Reset();
	void RegisterListener(ISoundCollectorEvent* listener, int inAdditionalData);
	void UnregisterListener(ISoundCollectorEvent* listener);
	void UnregisterAllListeners();

	BOOL Start(LPCTSTR inLeftChannelFile, LPCTSTR inRightChannelFile, int inBufsize, int inOutputSamplingRate,
		int inOverlapBufSize = 0,int inRepeatTime = 1);
	// (filename, filename, buffer size in samples, output sampling rate) 
	BOOL IsFinished();

protected:
	CFile soundData[2];
	int bufSize;
	int outSamplingRate;
	int overlapBufSize;
	int fileSamplingRate;
	std::vector<ISoundCollectorEvent*> callBackList; 
	std::vector<int> additionalDataList;
	HANDLE threadHandle;

	friend DWORD WINAPI CWaveCollector_collection_thread( LPVOID lpParam);
	void OnThreadEnd();

	int blankBuffer;
	int repeatTime;
	bool bFinished;

	// critical section
	CRITICAL_SECTION CriticalSection;
	BOOL bStopSignal;
};

DWORD WINAPI CWaveCollector_collection_thread( LPVOID lpParam);
