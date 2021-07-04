#pragma once

#include "FFT.h"
#include "MultiThreadOptimization.h"

#include <vector>

struct MaskingResult
{
public:
	MaskingResult();
	int x;
	int y;
};

struct RealtimeMaskingParameters
{
public:
	int maskingGraphZoomInSize;
	int frameBufferTotal;
	int samplingRate;
	int frameSize;
	int localizationFFTSize;
	int maskingGraphSize;
	int nOutputChannels;
	float realtimeAttenuation;
	//---Instrution:---
	/*	int framesBufferTotal,	Each time only the lastest frame is used,
								preserving previous frames for better separation
								typically 1 or 2...

		int framesize,			that must be equal to the number of samples in
								each mono channel, each frame,
								and must be POWER OF TWO
								typically 1024 or 2048 or 4096 or 8192...

		int samplingrate,		typically 44100 or 96000

		int maskinggraphsize,	typically 100, larger size gains more precision but 
								at the same time slows the separating process

		int noutputchannels,	number of output channels that we want to separate into.
								typicall 2
								*/
public:
	RealtimeMaskingParameters();
	void MakeDefault();
	CString ToString() const;
};

class RealtimeMaskingKernel
{
public:

	RealtimeMaskingKernel(void);
	virtual ~RealtimeMaskingKernel(void);

	// reset parameters
	void Reset(const RealtimeMaskingParameters& paras);

	BOOL Process(const float* inputchannel_01, const float* inputchannel_23, BOOL& bResultAlready);
		// both inputchannel_01 and inputchannel_23 must be stereo channels
		// with 2 x fftsize doubles ( 4 x fftsize doubles in total).

	// properties
	const std::vector<CComplex<float> >& GetCurrentSignal(int inputchannelindex);
	const std::vector<CComplex<float> >& GetCurrentSpectrum(int inputchannelindex);
	const std::vector<std::vector<float> >& GetMaskingGraph();
	const std::vector<std::vector<float> >& GetSmoothedMaskingGraph();
	const std::vector<CComplex<float> >& GetCurrentOutput(int outputchannelindex);
	const std::vector<CComplex<float> >& GetCurrentOutputSpectrum(int outputchannelindex);
	const std::vector<MaskingResult>& GetResult();
	const std::vector<std::vector<int> >& GetResultLabelingGraph();
	const int GetSamplingRate();
	const int GetFFTSize();
	const int GetMaskingGraphSize();
	const int GetOutputChannelsTotal();
	const CString& GetDebugString();
	void EmptyDebugString();

protected: // protected methods

	void MakeSpectrum();			// make fft of input signals
	void AccumulateMaskingGraph();	// 
	void SolvePositions();	
									//  
	void MakeOutputs();				// delayed signal subtraction & ifft the spectrums

	// localization algorithm
	BOOL GetPhaseDiff(const std::vector<CComplex<float> > & correlationspectrum,
		float& outSampleDiff, float& outAmplitude);
	BOOL GetPhaseDiff(const std::vector<CComplex<float> > & spectrumLeft,
		const std::vector<CComplex<float> > & spectrumRight, int frequencyIndex, int frequencySpan,
		float& outSampleDiff, float& outAmplitude);
	BOOL GetPhaseDiff(const std::vector<CComplex<float> > & spectrumLeft,
		const std::vector<CComplex<float> > & spectrumRight, int nodeIndex,
		int nodetotal, int lowcut, int highcut, 
		float& outSampleDiff, float& outAmplitude);

	// Image Convolution for Gauss Smoothing
	BOOL ImageConvolution(const std::vector<std::vector<float> > & graphIn, 
		const std::vector<std::vector<float> > &filterIn,
		std::vector<std::vector<float> > &graphOut);

	// Get an Element in an array with float index.
	CComplex<float> GetFloatIndexedElement(const std::vector<CComplex<float> > & arrayData,
		float index);


protected: // attributes

	int nFrames;			// number of frames preserved in buffer
	int nFFTSize;			//
	int nLocalizationFFTSize;
	int nSamplingRate;		// 44100Hz or 96000Hz or 22050Hz or...
	int currentFrameIndex;	// FrameIndex, range from 0 to nFrames-1
	int nMaskingGraphSize;	// 
	int nOutputChannels;	// number of separated sources
	int nFramesPassed;		// number of frames that have been processed
	float gaussCoefficient;// gauss coeffiecient for DUET graph accumulation
	float realtimeAttenuation;
	BOOL bFirstComputePosition;
	int MaskingGraphZoomInSize;
	CString debugString;

	std::vector<std::vector<CComplex<float> > > inputSignals[4]; 
		// 4 input channels by n buffering Frames by n samplesPerFrame, with each sample a float value

	std::vector<std::vector<CComplex<float> > > spectrums[4]; 
		// FFT of inputsignals.
		// 4 input channels by n buffering Frames by n Frequency-Amplitudes Per Frame, 
		// with each frequency a complex value

	std::vector<std::vector<CComplex<float> > > outputSpectrums;
		// Masked Specturms. 
		// n output channels by n samplesPerFrame, with each sample a complex value

	std::vector<std::vector<CComplex<float> > > outputSignals; 
		// IFFT of separated input spectrums
		// n output channels by n samplesPerFrame, with each sample a complex value

	std::vector<std::vector<float> > maskingGraph;
	std::vector<std::vector<float> > smoothedMaskingGraph;
		// n by n masking graph, and it's gauss smoothed form

	std::vector<MaskingResult> results;
		// n output results

	std::vector<std::vector<int> > resultLabelingGraph;
		// result labeling graph, size of n by n

	FFT_T<float> fftEngine;
	MultiQuickSort qsortengine;
};
