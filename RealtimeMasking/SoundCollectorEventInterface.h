
#pragma once

class ISoundCollectorEvent{
public:
	virtual void processSoundData(const double *soundData, int bufSize, int nSamples, DWORD additionalData) = 0;
};
