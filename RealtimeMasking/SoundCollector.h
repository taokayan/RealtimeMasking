
#pragma once

#include <windows.h>
#include <mmsystem.h>
#include <vector>
#include "SoundCollectorEventInterface.h"
#include "MutexUtil.h"


typedef struct regEntry
{
	ISoundCollectorEvent* p_Event;
	DWORD additionalData;
} regEntry;

class CSoundCollector
{
friend void CALLBACK waveProc(HWAVEIN,UINT,DWORD,DWORD,DWORD);

public:
	CSoundCollector(int nChannels_, int nSamples_, int bufSize_, int deviceID_);
	~CSoundCollector();
	void RegisterListener(ISoundCollectorEvent* p_Event, DWORD additionalData = 0);
	void UnRegisterListener(ISoundCollectorEvent* p_Event);
	void UnRegisterAllListener();
	bool static getDevices(WAVEINCAPS* dev, int &nDevice);
	int static getDeviceNum();
	bool Start();
	bool Stop();

protected:
	bool ProcessNext();
	void Localize();

private:
	MutexUtils::CMutex	 m_mutexSoundCollectorEvent;
	std::vector<regEntry>	m_listSoundCollectorEvent;

	DWORD startTime;
	WAVEHDR *pwhi, whis;
	HWAVEIN hwi;

	BOOL bStart;

	int nChannels;
	int nSamples;
	int bufSize;

	int deviceID;
	int nWaitingThreads;

	__int16 *Buffer;
	double *soundData;

	CRITICAL_SECTION CriticalSection;		
};

void CALLBACK waveProc(
	HWAVEIN hwi,       
	UINT uMsg,         
	DWORD dwInstance,  
	DWORD dwParam1,    
	DWORD dwParam2     );

