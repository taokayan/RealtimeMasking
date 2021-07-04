
#ifndef SOUNDWRITER_H
#define SOUNDWRITER_H


#include "stdafx.h"
#include <afxwin.h>
#include <mmsystem.h>


namespace SoundWriter
{
	class Sound2File
	{

	public:
		Sound2File();
		virtual ~Sound2File();
	
	protected:
		CString filename;
		__int16 lastdata;
		HANDLE fileHandle;
		int soundDataLength;
		int SAMPLES;
		CRITICAL_SECTION CriticalSection;


	public:
		void FilePrepare(LPCTSTR filename, int kHz);
		void ProcessNext(__int16* data, int nSamples);
		void Close();
	};

}


#endif
