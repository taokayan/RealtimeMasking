
#include "stdafx.h"
#pragma comment(lib,"winmm.lib")

#include <windows.h>
#include <vector>
#include <algorithm>
#include "SoundCollectorEventInterface.h"
#include "MutexUtil.h"
#include "SoundCollector.h" 


	CSoundCollector::CSoundCollector(int nChannels_, int Samples_, int bufSize_, int deviceID_)
	{
		pwhi = 0;
		hwi = 0;
		InitializeCriticalSection(&CriticalSection);

		nSamples = Samples_;
		nChannels = nChannels_;
		bufSize = bufSize_;
		bStart = FALSE;

		this->Buffer = new __int16[nChannels * bufSize];
		this->soundData = new double[nChannels * bufSize];

		deviceID = deviceID_;
		nWaitingThreads = 0;
		
		m_listSoundCollectorEvent.clear();
	}

	CSoundCollector::~CSoundCollector()
	{
		UnRegisterAllListener();
		Stop();
		DeleteCriticalSection(&CriticalSection);
		delete[] Buffer;
		delete[] soundData;
	}

	void CSoundCollector::RegisterListener(ISoundCollectorEvent *p_Event, DWORD additionalData)
	{
		MutexUtils::CAutoMutexLock lock(m_mutexSoundCollectorEvent);		

		int n = static_cast<int>(m_listSoundCollectorEvent.size());
		BOOL found = FALSE;
		for (int i = 0; i < n; ++i)
		{
			if (m_listSoundCollectorEvent[i].p_Event == p_Event)
			{
				found = TRUE; break;
			}
		}
		if ( ! found)
		{
			regEntry entry;
			entry.p_Event = p_Event;
			entry.additionalData = additionalData;
			m_listSoundCollectorEvent.push_back(entry);
		}
	}

	void CSoundCollector::UnRegisterListener(ISoundCollectorEvent *p_Event)
	{
		MutexUtils::CAutoMutexLock lock(m_mutexSoundCollectorEvent);
		
		for (std::vector<regEntry>::iterator pos = m_listSoundCollectorEvent.begin();
				pos != m_listSoundCollectorEvent.end(); ++pos)
		{
			if ( (*pos).p_Event == p_Event )
			{
				m_listSoundCollectorEvent.erase(pos);
				return;
			}
		}
	}

	void CSoundCollector::UnRegisterAllListener()
	{
		MutexUtils::CAutoMutexLock lock(m_mutexSoundCollectorEvent);
		m_listSoundCollectorEvent.clear();
	}

	bool CSoundCollector::getDevices(WAVEINCAPS* dev, int &nDevice)
	{
		nDevice = waveInGetNumDevs();
		for (int i = 0; i < nDevice; ++i){
			int res = waveInGetDevCaps(i, &dev[i], sizeof(WAVEINCAPS));
			if (res != MMSYSERR_NOERROR){
				nDevice = 0;
				return false;
			}
		}
		return true;
	}

	int CSoundCollector::getDeviceNum()
	{
		return waveInGetNumDevs();
	}

	bool CSoundCollector::Start()
	{

		WAVEFORMATEX wfx;

		const int Bits = 16;
		memset(&wfx,0,sizeof(WAVEFORMATEX));
		wfx.wFormatTag=WAVE_FORMAT_PCM;
		wfx.nChannels=nChannels;
		wfx.wBitsPerSample=Bits;
		wfx.nSamplesPerSec=nSamples;
		wfx.nBlockAlign=Bits*nChannels/8;
		wfx.nAvgBytesPerSec=nSamples*Bits*nChannels/8;
		wfx.cbSize=0;

		nWaitingThreads++;
		MMRESULT r = waveInOpen(&hwi, deviceID, &wfx, (DWORD)waveProc, (DWORD)this,
			CALLBACK_FUNCTION);

		pwhi=&whis;
		pwhi->dwFlags=0;
		pwhi->dwLoops=0;
		pwhi->dwBytesRecorded=0;
		pwhi->dwBufferLength=bufSize*sizeof(__int16) * nChannels;
		pwhi->lpData=(char *)Buffer;

		waveInStart(hwi);

		bStart = TRUE;

		ProcessNext();

		return true;
	}

	bool CSoundCollector::Stop()
	{
		UINT time = GetTickCount();
		bStart = FALSE;
		while(nWaitingThreads > 0)
		{
			Sleep(1);
		}
		return true;
	}

	bool CSoundCollector::ProcessNext()
	{
		if( !bStart )
			return false;
		//waveInUnprepareHeader(hwi,pwhi,sizeof(WAVEHDR));
		pwhi=&whis;
		pwhi->dwFlags=0;
		pwhi->dwLoops=0;
		MMRESULT r;
		r = waveInPrepareHeader(hwi,pwhi,sizeof(WAVEHDR));
		r = waveInAddBuffer(hwi,pwhi,sizeof(WAVEHDR));
		nWaitingThreads++;
		return true;
	}



	void CALLBACK waveProc(HWAVEIN hwi,UINT uMsg,DWORD dwInstance,
		DWORD wParam,DWORD lParam)
	{

		CSoundCollector* pWave = (CSoundCollector*)dwInstance; // "this" pointer

		// single threading starts here.
		EnterCriticalSection(&pWave->CriticalSection); 

		WAVEHDR* hdr=(WAVEHDR*)lParam;

		__int16* pData=(__int16*)pWave->Buffer;

		// copy sound datas for use
		for(int i = 0; i < pWave->bufSize * pWave->nChannels; i++)
		{
			pWave->soundData[i] = (double)(pData[i]) / (double)32768;
		}


		{
			MutexUtils::CAutoMutexLock lock(pWave->m_mutexSoundCollectorEvent);
			int count = static_cast<int>(pWave->m_listSoundCollectorEvent.size());
			for (int i = 0; i < count; ++i)
			{
				if (pWave != NULL && pWave->m_listSoundCollectorEvent[i].p_Event != NULL)
					pWave->m_listSoundCollectorEvent[i].p_Event->processSoundData(pWave->soundData, pWave->bufSize, pWave->nSamples, pWave->m_listSoundCollectorEvent[i].additionalData);
			}
		}
		pWave->nWaitingThreads--;

		if( pWave->bStart == FALSE)
			goto END;

		pWave->ProcessNext();  // so that it can be always running.

END:
		// single threading ends here
		LeaveCriticalSection(&pWave->CriticalSection);
		return;
	}

	//
