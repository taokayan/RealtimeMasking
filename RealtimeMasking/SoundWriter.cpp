

#include "stdafx.h"
#include "SoundWriter.h"

#include <windows.h>
#include <string.h>

#include <math.h>

namespace SoundWriter
{
	Sound2File::Sound2File()
	{
		InitializeCriticalSection(&CriticalSection);
		fileHandle = 0;
	}

	Sound2File::~Sound2File()
	{
		EnterCriticalSection(&CriticalSection);
		Close();
		LeaveCriticalSection(&CriticalSection);
	}

	void Sound2File::FilePrepare(LPCTSTR name, int kHz)
	{
		EnterCriticalSection(&CriticalSection);
		Close();

		filename = name;
		SAMPLES = kHz;
		soundDataLength = 0;
		lastdata = 0;

		fileHandle = CreateFile( filename, GENERIC_WRITE, FILE_SHARE_READ, NULL, 
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		DWORD r;
		__int32 i32;
		__int16 i16;
		WriteFile(fileHandle, "RIFF", 4, &r, NULL);//header
		i32 = 0;
		WriteFile(fileHandle, &i32, 4, &r, NULL);	//filesize-8
		WriteFile(fileHandle, "WAVE", 4, &r, NULL);	//"WAVE"
		WriteFile(fileHandle, "fmt ",4,&r,NULL);	//"fmt "
		i32 = 16;
		WriteFile(fileHandle, &i32, 4, &r, NULL);	//Int32 = 16
		i16 = 0;
		WriteFile(fileHandle, &i16, 2, &r, NULL);	//Int16 = 0;
		i16 = 1;
		WriteFile(fileHandle, &i16, 2, &r, NULL);	//1 channel
		i32 = SAMPLES;
		WriteFile(fileHandle, &i32, 4, &r, NULL);	//Int32=nSamplesPerSec
		i32 = SAMPLES * 2;
		WriteFile(fileHandle, &i32, 4, &r, NULL);	//Int32=nAvgBytesPerSec
		i16 = 1;//??
		WriteFile(fileHandle, &i16, 2, &r, NULL);	//Int16 = nBlockAlign ??
		i16 = 16;
		WriteFile(fileHandle, &i16, 2, &r, NULL);	//Int16 = wBitsPerSample
		WriteFile(fileHandle, "data", 4, &r, NULL);
		i32 = 0;
		WriteFile(fileHandle, &i32, 4,&r, NULL);		//Int32 = the length of the data in bytes

		LeaveCriticalSection(&CriticalSection);
	}

	void Sound2File::ProcessNext(__int16* pData, int nSamples)
	{

		EnterCriticalSection(&CriticalSection);
		
		if( fileHandle != NULL)
		{
			// smooth the data
			double avgbegin = 0;
			double avgend = 0;
			int i;
			for(i = 0; i < nSamples / 100 + 1; i++)
			{
				avgbegin += pData[i];
				avgend += pData[nSamples - 1 - i];
			}
			avgbegin /= i;
			avgend /= i;
			float delta = avgbegin - lastdata;
			for(i = 0; i < nSamples; i++)
			{
				float k = cos( i/(float)nSamples * 3.14159265358979324) * 0.5 + 0.5;
				pData[i] = pData[i] - delta * k;
			}
			lastdata = avgend;


			// Write to File
			DWORD r;
			WriteFile(fileHandle, pData, nSamples * 2, &r, NULL);
			soundDataLength += nSamples *2;
		}

		LeaveCriticalSection(&CriticalSection);
	}

	void Sound2File::Close()
	{

		EnterCriticalSection(&CriticalSection);
		if( fileHandle != NULL)
		{
		
			// shut down file
			SetFilePointer(fileHandle, 0, NULL, FILE_BEGIN);


			DWORD r;
			__int32 i32;
			__int16 i16;
			WriteFile(fileHandle, "RIFF", 4, &r, NULL);//header
			i32 = 44 + soundDataLength - 8;
			WriteFile(fileHandle, &i32, 4, &r, NULL);	//filesize-8
			WriteFile(fileHandle, "WAVE", 4, &r, NULL);	//"WAVE"
			WriteFile(fileHandle, "fmt ",4,&r,NULL);	//"fmt "
			i32 = 16;
			WriteFile(fileHandle, &i32, 4, &r, NULL);	//Int32 = 16
			i16 = 1;
			WriteFile(fileHandle, &i16, 2, &r, NULL);	//Int16 = 0;
			i16 = 1;
			WriteFile(fileHandle, &i16, 2, &r, NULL);	//1 channel
			i32 = SAMPLES;
			WriteFile(fileHandle, &i32, 4, &r, NULL);	//Int32=nSamplesPerSec
			i32 = SAMPLES * 2;
			WriteFile(fileHandle, &i32, 4, &r, NULL);	//Int32=nAvgBytesPerSec
			i16 = 2;//??
			WriteFile(fileHandle, &i16, 2, &r, NULL);	//Int16 = nBlockAlign ??
			i16 = 16;
			WriteFile(fileHandle, &i16, 2, &r, NULL);	//Int16 = wBitsPerSample
			WriteFile(fileHandle, "data", 4, &r, NULL);
			i32 = soundDataLength;// - BUFSIZE * 2;
			WriteFile(fileHandle, &i32, 4, &r, NULL);		//Int32 = the length of the data in bytes

			CloseHandle(fileHandle);

			fileHandle = NULL;
		}

		LeaveCriticalSection(&CriticalSection);
	}

}

