#include "StdAfx.h"
#include "RealtimeMaskingKernel.h"


MaskingResult::MaskingResult()
{
	x = 0;
	y = 0;
}


//--------------------------class RealtimeMaskingParameters-------------------------
RealtimeMaskingParameters::RealtimeMaskingParameters()
{
	MakeDefault();
}

void RealtimeMaskingParameters::MakeDefault()
{
	maskingGraphZoomInSize = 5;
	frameBufferTotal = 2;
	samplingRate = 44100;
	frameSize = 2048;
	localizationFFTSize = 1024;
	maskingGraphSize = 128;
	nOutputChannels = 4;
	realtimeAttenuation = 0.997;
}

CString RealtimeMaskingParameters::ToString() const
{
	CString str = _T(""),str2;
	str2.Format(_T("FrameBufferTotal = %d\r\n"),frameBufferTotal);
	str += str2;
	str2.Format(_T("InputSamplingRate = %d\r\n"),samplingRate);
	str += str2;
	str2.Format(_T("FrameFFTSize = %d\r\n"),frameSize);
	str += str2;
	str2.Format(_T("LocalizationFFTSize = %d\r\n"),localizationFFTSize);
	str += str2;
	str2.Format(_T("MaskingGraphSize = %d\r\n"),maskingGraphSize);
	str += str2;
	str2.Format(_T("GraphZoomIn = %d X\r\n"),maskingGraphZoomInSize);
	str += str2;
	str2.Format(_T("RealtimeAttenuation = %.2lf X\r\n"),realtimeAttenuation);
	str += str2;
	str2.Format(_T("NOutputChannel = %d\r\n"),nOutputChannels);
	str += str2;
	return str;
}


//--------------------------class RealtimeMaskingKernel--------------------------
RealtimeMaskingKernel::RealtimeMaskingKernel(void)
{
}

RealtimeMaskingKernel::~RealtimeMaskingKernel(void)
{
}

void RealtimeMaskingKernel::Reset(const RealtimeMaskingParameters& paras)
{
	// debugstring;
	debugString += L"RealtimeMaskingKernel::Reset()\r\n";
	debugString += paras.ToString();

	// assign parameters
	nFrames = paras.frameBufferTotal;
	nFFTSize = paras.frameSize;
	nSamplingRate = paras.samplingRate;
	nLocalizationFFTSize = paras.localizationFFTSize;
	nMaskingGraphSize = paras.maskingGraphSize;
	nOutputChannels = paras.nOutputChannels;
	realtimeAttenuation = paras.realtimeAttenuation;
	MaskingGraphZoomInSize = paras.maskingGraphZoomInSize;

	// initialization
	currentFrameIndex = 0;
	nFramesPassed = 0;
	for(int i = 0; i < 4; i++)
	{
		inputSignals[i].resize(nFrames);
		spectrums[i].resize(nFrames);
		for(int j = 0; j < nFrames; j++)
		{
			inputSignals[i][j].resize(nFFTSize);
			spectrums[i][j].resize(nFFTSize);
		}
	}

	// output
	results.resize(nOutputChannels);
	outputSignals.resize(nOutputChannels);
	outputSpectrums.resize(nOutputChannels);
	for(int i = 0; i < nOutputChannels; i++)
	{
		outputSignals[i].resize(nFFTSize);
		outputSpectrums[i].resize(nFFTSize);
		results[i] = MaskingResult();
		results[i].x = results[i].y = 0;
	}

	// masking graph
	gaussCoefficient = 2.0;
	maskingGraph.resize(nMaskingGraphSize);
	smoothedMaskingGraph.resize(nMaskingGraphSize);
	for(int i = 0; i < nMaskingGraphSize; i++)
	{
		maskingGraph[i].resize(nMaskingGraphSize);
		smoothedMaskingGraph[i].resize(nMaskingGraphSize);
		for(int j = 0; j < nMaskingGraphSize; j++)
		{
			maskingGraph[i][j] = 0;
			smoothedMaskingGraph[i][j] = 0;
		}
	}

	// result labeling graph
	bFirstComputePosition = FALSE;
	resultLabelingGraph.resize(nMaskingGraphSize);
	for(int i = 0; i < nMaskingGraphSize; i++)
	{
		resultLabelingGraph[i].resize(nMaskingGraphSize);
		for(int j = 0; j < nMaskingGraphSize; j++)
		{
			resultLabelingGraph[i][j] = -1;
		}
	}

	debugString += L"OK\r\n";
}

BOOL RealtimeMaskingKernel::Process(const float *inputchannel_01, const float *inputchannel_23, BOOL& bResultAlready)
{
	//
	bResultAlready = FALSE;


	// Assign inputSignals
	for(int i = 0; i < nFFTSize; i++)
	{
		inputSignals[0][currentFrameIndex][i] = CComplex<float>(inputchannel_01[i * 2]);
		inputSignals[1][currentFrameIndex][i] = CComplex<float>(inputchannel_01[i * 2 + 1]);
		inputSignals[2][currentFrameIndex][i] = CComplex<float>(inputchannel_23[i * 2]);
		inputSignals[3][currentFrameIndex][i] = CComplex<float>(inputchannel_23[i * 2 + 1]);
	}

	// Translate signal into spectrum
	MakeSpectrum();

	// DUET graph Accumulation
	AccumulateMaskingGraph();

	// Realtime Attenuation
	for(int y = 0; y < maskingGraph.size(); y++)
	{
		for(int x = 0; x < maskingGraph[y].size(); x++)
		{
			maskingGraph[y][x] *= realtimeAttenuation;
		}
	}

	// Solve Positions and separate the sound
	if( nFramesPassed % 4 == 0)
	{
		this->SolvePositions();	// Update positions once several frames 
	}
	MakeOutputs();
	bResultAlready = TRUE;

	// Increase current frame index
	currentFrameIndex = (currentFrameIndex + 1) % nFrames;
	nFramesPassed++;

SUCCEED_RETURN:
	// succeed in processing one frame
	return TRUE;
}

// transform signals to spectrums via FFT
void RealtimeMaskingKernel::MakeSpectrum()
{
	// single thread
	for(int i = 0; i < 4; i++)
	{
		this->fftEngine.FFT(&(this->inputSignals[i][this->currentFrameIndex][0]), this->nFFTSize, 
			&(this->spectrums[i][this->currentFrameIndex][0]), FALSE);
	}
}

void RealtimeMaskingKernel::AccumulateMaskingGraph()
{
	//// Algorithm: sound localization by every frequency interval

	float ratio = this->nSamplingRate / (float)(this->nLocalizationFFTSize);
	//float ratio = this->nLocalizationFFTSize * 96000.0 /(float)(this->nSamplingRate) / 4096.0;
	int frequencyintervals[] = {
		1000.0 / ratio,
		2000.0 / ratio,
		3000.0 / ratio};
	int pitchtotal = 24;

	std::vector<CComplex<float> > tempsignal[4];
	std::vector<CComplex<float> > tempspectrum[4];
	for(int i = 0; i < 4; i++)
	{
		tempsignal[i].resize(nLocalizationFFTSize);
		tempspectrum[i].resize(nLocalizationFFTSize);
	}
	for(int t = 0; t < nFFTSize / nLocalizationFFTSize ; t++)
	{
		///
		// initialization
		///
		for(int j = 0; j < 4; j++)
		{
			int baseIndex = t * nLocalizationFFTSize / 2;
			for(int i = 0; i < nLocalizationFFTSize; i++)
			{
				// hamming windowed fft 
				float k = 0.53836 - 
					0.46164 * cos(2 * 3.1415926535898 * i / (nLocalizationFFTSize - 1.0));
				//k = 1;
				tempsignal[j][i] = GetCurrentSignal(j)[baseIndex + i] * k;
			}
			fftEngine.FFT(&(tempsignal[j][0]),nLocalizationFFTSize,&(tempspectrum[j][0]));
		}

		//
		// sound localization and graph accumulation
		//

		// via frequency interval
		for(int i = 0; i < sizeof(frequencyintervals) / sizeof(int) - 1 ; i++)
		{
			float sampleDiff[2];
			float avgAmplitude[2];
			if( GetPhaseDiff(tempspectrum[0],tempspectrum[1],
				frequencyintervals[i], frequencyintervals[i+1] - frequencyintervals[i], 
				sampleDiff[0], avgAmplitude[0]) == TRUE && avgAmplitude[0] > 0 &&
				GetPhaseDiff(tempspectrum[2],tempspectrum[3],
				frequencyintervals[i], frequencyintervals[i+1] - frequencyintervals[i], 
				sampleDiff[1], avgAmplitude[1]) == TRUE && avgAmplitude[1] > 0)
			{
				int px[2];
				px[0] = sampleDiff[0] * MaskingGraphZoomInSize + nMaskingGraphSize / 2;
				px[1] = sampleDiff[1] * MaskingGraphZoomInSize + nMaskingGraphSize / 2;
				float val;
				val = pow(avgAmplitude[0] * avgAmplitude[1], 0.5f);// / (t * t + 1);
				if(val > 0 && px[0] >= 0 && px[1] >= 0 && px[0] < nMaskingGraphSize && px[1] < nMaskingGraphSize)
					maskingGraph[px[1]][px[0]] += val;
			}
		}

		//// via pitch
		//for(int i = 0; i < pitchtotal; i++)
		//{
		//	float sampleDiff[2];
		//	float avgAmplitude[2];
		//	if( GetPhaseDiff(tempspectrum[0],tempspectrum[1],
		//		i, pitchtotal, 1000.0 / ratio, 8000.0 / ratio,
		//		sampleDiff[0], avgAmplitude[0]) == TRUE && avgAmplitude[0] > 0 &&
		//		GetPhaseDiff(tempspectrum[2],tempspectrum[3],
		//		i, pitchtotal, 1000.0/ ratio, 8000.0 / ratio,
		//		sampleDiff[1], avgAmplitude[1]) == TRUE && avgAmplitude[1] > 0)
		//	{
		//		int px[2];
		//		px[0] = sampleDiff[0] * MaskingGraphZoomInSize + nMaskingGraphSize / 2;
		//		px[1] = sampleDiff[1] * MaskingGraphZoomInSize + nMaskingGraphSize / 2;
		//		float val;
		//		val = pow(avgAmplitude[0] * avgAmplitude[1], 0.25);// / (t * t + 1);
		//		if(val > 0 && px[0] >= 0 && px[1] >= 0 && px[0] < nMaskingGraphSize && px[1] < nMaskingGraphSize)
		//		maskingGraph[px[1]][px[0]] += val;
		//	}
		//}
	}
}

// localization algorithm
BOOL RealtimeMaskingKernel::GetPhaseDiff(const std::vector<CComplex<float> > & inCorrSpectrum,
										 float& outSampleDiff, float& outAmplitude)
{
	// initialization
	const int multiple = 1;//2;//(1 << int(log((float)MaskingGraphZoomInSize) / log(2.0) + 1e-7));

	std::vector<CComplex<float> > corrSpectrum;
	std::vector<CComplex<float> > corrSignal;

	int fftsize = inCorrSpectrum.size() * multiple;
	corrSpectrum.resize(fftsize);
	corrSignal.resize(fftsize);

	corrSpectrum[0] = 0;
	for(int i = 1; i <= fftsize / 2; i++)
	{
		if ( i < inCorrSpectrum.size() / 2 )
		{
			corrSpectrum[i] = inCorrSpectrum[i];
		}
		else
		{
			corrSpectrum[i] = 0;
		}
		corrSpectrum[fftsize - i].re = corrSpectrum[i].re;
		corrSpectrum[fftsize - i].im = -corrSpectrum[i].im;
	}

	// transform back to time-signal space
	fftEngine.FFT(&(corrSpectrum[0]), fftsize, &(corrSignal[0]),1);

	// calculate the number of samples differed by 2 channels
	const int maxdiff = this->nMaskingGraphSize * multiple / 2 / MaskingGraphZoomInSize + 1;
	float max = 0;
	float secondmax = 0;
	float left = -1,right = -1;
	float avg = 0;
	int maxindex = -maxdiff - 2;
	for(int i = -maxdiff; i <= maxdiff; i++)
	{
		avg += corrSignal[(i + fftsize) % fftsize].re;
		if( corrSignal[(i + fftsize) % fftsize].re > corrSignal[(i + fftsize - 1) % fftsize].re &&
			corrSignal[(i + fftsize) % fftsize].re > corrSignal[(i + fftsize + 1) % fftsize].re)
		{
			if( max < corrSignal[(i + fftsize) % fftsize].re)
			{
				maxindex = i;
				secondmax = max;
				max = corrSignal[(i + fftsize) % fftsize].re;
				left = corrSignal[(i - 1 + fftsize) % fftsize].re;
				right = corrSignal[(i + 1 + fftsize) % fftsize].re;
			}
			else if( secondmax < corrSignal[(i + fftsize) % fftsize].re )
			{
				secondmax = corrSignal[(i + fftsize) % fftsize].re;
			}
		}
	}
	avg /= maxdiff * 2 + 1;
	max = sqrt(max);
	left = sqrt(left);
	right = sqrt(right);
	outSampleDiff = maxindex + (-left + right) / (2.0 * (-left + 2 * max - right));
	outSampleDiff /= (float)multiple;
	
	// formal way
	//outAmplitude = max;

	// alternative
	outAmplitude = max - avg;

	return TRUE;
}

BOOL RealtimeMaskingKernel::GetPhaseDiff(const std::vector<CComplex<float> > & spectrumLeft,
		const std::vector<CComplex<float> > & spectrumRight, int nodeIndex,
		int nodetotal, int lowcut, int highcut, 
		float& outSampleDiff, float& outAmplitude)
{
	// init
	std::vector<CComplex<float> > corrspectrum;
	int fftsize = spectrumLeft.size();
	corrspectrum.resize(fftsize);
	for(int i = 0; i < fftsize; i++)
	{
		corrspectrum[i] = 0;
	}
	if( lowcut < 1)lowcut = 1;
	if( highcut > fftsize / 2) highcut = fftsize/2;

	// pitch filter
	for(int i = lowcut; i < highcut; i++)
	{
		float p = log((float)i) / log(2.0) * nodetotal + nodetotal - nodeIndex;
		p -= (int)(p / nodetotal) * nodetotal; // equivalent to p = p % nodetotal
		if( p > 0.0 && p < 2.0 )
		{
			float k = cos(fabs(1.0 - p) * 3.141592654 * 0.5);
			CComplex<float> f0 = spectrumLeft[i];
			CComplex<float> f1 = spectrumRight[fftsize - i];
			corrspectrum[i] = f0 * f1;
			corrspectrum[i].re *= k;
			corrspectrum[i].im *= k;
			corrspectrum[fftsize - i].re = corrspectrum[i].re;
			corrspectrum[fftsize - i].im = -corrspectrum[i].im;
		}
	}
	return GetPhaseDiff(corrspectrum,outSampleDiff,outAmplitude);
}

BOOL RealtimeMaskingKernel::GetPhaseDiff(const std::vector<CComplex<float> > & spectrumLeft,
		const std::vector<CComplex<float> > & spectrumRight, int frequencyIndex, int frequencySpan,
		float& outSampleDiff, float& outAmplitude)
{
	// initialization
	std::vector<CComplex<float> > corrspectrum;
	int fftsize = spectrumLeft.size();
	corrspectrum.resize(fftsize);
	for(int i = 0; i < fftsize; i++)
	{
		corrspectrum[i].re = corrspectrum[i].im = 0;
	}

	// calculate amplitude of the specified frequency interval
	int lo = frequencyIndex / 1.2;
	int hi =  (frequencyIndex + frequencySpan) * 1.2;
	if( lo < 1)lo = 1;
	if( hi > fftsize/2) hi = fftsize / 2;
	for(int i = lo; i < hi; i++)
	{
		float k = 0.53836 - 
			0.46164 * cos(2 * 3.1415926535898 * (i - lo) / (hi - lo - 1.0));
		CComplex<float> f0 = spectrumLeft[i];
		CComplex<float> f1 = spectrumRight[fftsize - i];
		corrspectrum[i] = f0 * f1 * CComplex<float>(k);
		corrspectrum[fftsize - i].re = corrspectrum[i].re;
		corrspectrum[fftsize - i].im = -corrspectrum[i].im;
	}

	return GetPhaseDiff(corrspectrum,outSampleDiff,outAmplitude);
}


struct RealtimeMaskingKernel_MappingStruct
{
	int x;
	int y;
	float val;
};

int RealtimeMaskingKernel_Compare(const void* a, const void* b)
{
	float p = ((const RealtimeMaskingKernel_MappingStruct*)a)->val;
	float q = ((const RealtimeMaskingKernel_MappingStruct*)b)->val;
	if( p > q )return 1;
	if( p < q )return -1;
	return 0;
}

void RealtimeMaskingKernel::SolvePositions()
{
	// Aim: find all the local/global maxinums in the DUET graph 
	// Algorithm: simple discrete gradient algorithm 
	// complexity: O(N * N * logN), where N = nMaskingGraphSize		
	
	// gauss graph smoothing...
	std::vector<std::vector<float> > filter;
	const int filtersize = 51;
	filter.resize(filtersize);
	for(int y = 0; y < filtersize; y++)
	{
		filter[y].resize(filtersize);
		for(int x = 0; x < filtersize; x++)
		{
			int dy = y - filtersize / 2;
			int dx = x - filtersize / 2;
			filter[y][x] = exp(-(dx * dx + dy * dy) / (2 * gaussCoefficient * gaussCoefficient));
		}
	}
	ImageConvolution(maskingGraph, filter, smoothedMaskingGraph);
	
	// initialization
	std::vector<RealtimeMaskingKernel_MappingStruct> sortedGraph;
	std::vector<RealtimeMaskingKernel_MappingStruct> donatedGraph;
	sortedGraph.resize(nMaskingGraphSize * nMaskingGraphSize);
	donatedGraph.resize(nMaskingGraphSize * nMaskingGraphSize);
	float globalmax;
	for(int y = 0, p = 0; y < nMaskingGraphSize; y++)
	{
		for(int x = 0; x < nMaskingGraphSize; x++, p++)
		{
			sortedGraph[p].x = x;
			sortedGraph[p].y = y;
			sortedGraph[p].val = smoothedMaskingGraph[y][x]; // ready to sort by val
			donatedGraph[p].x = x;
			donatedGraph[p].y = y;
			donatedGraph[p].val = smoothedMaskingGraph[y][x]; // ready to donate
		}
	}

	// sort by amplitude
	qsortengine.MultiQSort(&(sortedGraph[0]), sortedGraph.size(), 
		sizeof(RealtimeMaskingKernel_MappingStruct), 
		RealtimeMaskingKernel_Compare);
	globalmax = sortedGraph[sortedGraph.size() -1].val;

	// amplitude donation
	for(int i = 0; i < nMaskingGraphSize * nMaskingGraphSize; i++)
	{
		int cx = sortedGraph[i].x;
		int cy = sortedGraph[i].y;
		RealtimeMaskingKernel_MappingStruct maxneighbor = sortedGraph[i];
		float donatevalue = donatedGraph[cy * nMaskingGraphSize + cx].val;
		if( cx > 0 && cx < nMaskingGraphSize - 1 && cy > 0 && cy < nMaskingGraphSize - 1)
		{
			// find max in all the neighborhoods
			for(int dy = -1; dy <= 1; dy++)
			{
				for(int dx = -1; dx <= 1; dx++)
				{
					if( smoothedMaskingGraph[cy + dy][cx + dx] > maxneighbor.val)
					{
						maxneighbor.x = cx + dx;
						maxneighbor.y = cy + dy;
						maxneighbor.val = smoothedMaskingGraph[cy + dy][cx + dx];
					}
				}
			}
			// donation
			donatedGraph[maxneighbor.y * nMaskingGraphSize + maxneighbor.x].val += donatevalue;
			donatedGraph[cy * nMaskingGraphSize + cx].val -= donatevalue;
		}
	}

	// sort by donated amplitude
	qsortengine.MultiQSort(&(donatedGraph[0]), donatedGraph.size(),
		sizeof(RealtimeMaskingKernel_MappingStruct),
		RealtimeMaskingKernel_Compare);

	// modify gauss coefficient via the result of classification
	if( nOutputChannels >= 2)
	{
		const float maxGauss = 10.0;
		const float minGauss = 0.5;
		const float deltaGauss = 0.015;
		const float threshold = 0.85;
		float sumn_1 = 0, sumn = 0, sumall = 1e-10;
		int i;
		for(i = 0; i < nOutputChannels - 1; i++)
		{
			sumn_1 += donatedGraph[donatedGraph.size() - i - 1].val;
			sumn += donatedGraph[donatedGraph.size() - i - 1].val;
			sumall += donatedGraph[donatedGraph.size() - i - 1].val;
		}
		sumn += donatedGraph[donatedGraph.size() - i - 1].val;
		sumall += donatedGraph[donatedGraph.size() - i - 1].val;
		i++;
		for(; i < nOutputChannels * 10; i++)
		{
			sumall += donatedGraph[donatedGraph.size() - i - 1].val;
		}
		if( sumn / sumall < threshold)
		{
			// increase gauss
			gaussCoefficient *= (1+deltaGauss);
		}
		if( sumn_1 / sumall > threshold)
		{
			// decrease gauss
			gaussCoefficient /= (1+deltaGauss);
		}
		if( gaussCoefficient > maxGauss) gaussCoefficient = maxGauss;
		if( gaussCoefficient < minGauss) gaussCoefficient = minGauss;
	}

	// result assignments
	if( bFirstComputePosition == TRUE)
	{
		// assign results
		int p = donatedGraph.size() - 1;
		for(int i = 0; i < this->nOutputChannels; i++,p--)
		{
			this->results[i].x = donatedGraph[p].x;
			this->results[i].y = donatedGraph[p].y;
		}
	}
	else
	{
		// solve permutation
		std::vector<RealtimeMaskingKernel_MappingStruct> distance;
		distance.resize(nOutputChannels * nOutputChannels);
		int p = 0;
		for(int i = 0 ; i < nOutputChannels; i++)
		{
			for(int j = 0; j < nOutputChannels; j++,p++)
			{
				distance[p].x = i;
				distance[p].y = j;
				int curx = donatedGraph[donatedGraph.size() - 1 - j].x;
				int cury = donatedGraph[donatedGraph.size() - 1 - j].y;
				int prevx = results[i].x;
				int prevy = results[i].y;
				distance[p].val = (curx - prevx)*(curx-prevx) + (cury - prevy)*(cury-prevy);
			}
		}
		qsortengine.MultiQSort(&(distance[0]), distance.size(),sizeof(RealtimeMaskingKernel_MappingStruct),
			RealtimeMaskingKernel_Compare);
		std::vector<BOOL> prevMark, currMark;
		prevMark.resize(nOutputChannels);
		currMark.resize(nOutputChannels);
		for(int i = 0; i < nOutputChannels; i++)
		{
			prevMark[i] = currMark[i] = 0;
		}
		for(int i = 0; i < distance.size(); i++)
		{
			if( prevMark[distance[i].x] == 0 && currMark[distance[i].y] == 0)
			{
				prevMark[distance[i].x] = currMark[distance[i].y] = 1;
				results[distance[i].x].x = donatedGraph[donatedGraph.size() - 1 - distance[i].y].x;
				results[distance[i].x].y = donatedGraph[donatedGraph.size() - 1 - distance[i].y].y;
			}
		}
	}
}

void RealtimeMaskingKernel::MakeOutputs()
{
	//
	// Algorithm: Delayed signal substraction & reconstruction via time-frequency masking
	// Complexity: O(N * log N * S) for each frame, where 
	//             N = FFT Size, 
	//             S = number of output channels
	// Author: Tao,Ka Yan (Du,Jia en) 
	//
	// -----------------------------------------------------------------------------------

	//
	// Substraction of delayed original signals....
	//
	std::vector<std::vector<CComplex<float> > > subtractedsignal[2];
	std::vector<std::vector<CComplex<float> > > subtractedspectrum[2];
		// 2 input channel pairs * n Output Channels * FFTSize
	
	for(int i = 0; i < 2; i++)
	{
		subtractedsignal[i].resize(nOutputChannels);
		subtractedspectrum[i].resize(nOutputChannels);
		for(int j = 0; j < nOutputChannels; j++)
		{
			subtractedsignal[i][j].resize(nFFTSize);
			subtractedspectrum[i][j].resize(nFFTSize);

			float delay = i == 0?
				results[j].x - nMaskingGraphSize / 2 : results[j].y - nMaskingGraphSize / 2;
			delay = delay / MaskingGraphZoomInSize;

			int blanksize = fabs(delay) + 3;
			for(int k = 0; k < blanksize; k++)
			{
				subtractedsignal[i][j][k] = 0;
				subtractedsignal[i][j][nFFTSize - k - 1] = 0;
			}
			for(int k = blanksize; k < nFFTSize - blanksize; k++)
			{
				// signal subtraction
				// hamming windowed fft 
				float coef = 0.53836 - 
					0.46164 * cos(2 * 3.1415926535898 * k / (nFFTSize - 1.0));

				subtractedsignal[i][j][k] = 
					(GetFloatIndexedElement(GetCurrentSignal(i * 2), k + delay) - 
					GetFloatIndexedElement(GetCurrentSignal(i * 2 + 1), k) )
					* CComplex<float>(coef);
			}
		}
	}

	// transfer subtracted signals into subtracted spectrum
	for(int j = 0; j < nOutputChannels; j++)
	{
		this->fftEngine.FFT(&(subtractedsignal[0][j][0]),nFFTSize,&(subtractedspectrum[0][j][0]));
		this->fftEngine.FFT(&(subtractedsignal[1][j][0]),nFFTSize,&(subtractedspectrum[1][j][0]));

		// multi-thread supported...
		//this->multifftengine.FFTs(&(subtractedsignal[0][j][0]),&(subtractedspectrum[0][j][0]),FALSE,
		//	&(subtractedsignal[1][j][0]),&(subtractedspectrum[1][j][0]),FALSE, nFFTSize);
	}

	//
	// reconstruction via time-frequency masking
	//
	const int lowfreqcutoff = 5;
	for(int i = 0; i < nOutputChannels; i++)
	{
		for(int j = 1; j < nFFTSize; j++)
		{
			outputSpectrums[i][j] = 0;
		}
	}
	std::vector<float> avgamplitude;
	avgamplitude.resize(nOutputChannels);
	for(int i = lowfreqcutoff; i <= nFFTSize / 8; i++)
	{
		//CComplex<float> avg = (GetCurrentSpectrum(0)[i] + GetCurrentSpectrum(1)[i] +
		//	GetCurrentSpectrum(2)[i] + GetCurrentSpectrum(3)[i]) * CComplex<float>(0.25);
		CComplex<float> avg = GetCurrentSpectrum(0)[i];

		// find out which channel depresses the sound most...
		int minchannelindex = 0;
		for(int j = 0; j < nOutputChannels; j++)
		{
			avgamplitude[j] = (subtractedspectrum[0][j][i].GetAbsSqr() + 1e-50) 
				* (subtractedspectrum[1][j][i].GetAbsSqr() + 1e-50);
			if( avgamplitude[j] < avgamplitude[minchannelindex])
			{
				minchannelindex = j;
			}
		}
		for(int j = 0; j < nOutputChannels; j++)
		{
			outputSpectrums[j][i] = CComplex<float>(0);
			outputSpectrums[j][nFFTSize - i] = CComplex<float>(0);
		}
		outputSpectrums[minchannelindex][i] = avg;
		outputSpectrums[minchannelindex][nFFTSize - i].re = outputSpectrums[minchannelindex][i].re;
		outputSpectrums[minchannelindex][nFFTSize - i].im = -outputSpectrums[minchannelindex][i].im;
	}

	//
	// IFFT back to time-signal space
	//
	for(int i = 0; i < nOutputChannels; i++)
	{
		fftEngine.FFT(&(outputSpectrums[i][0]), nFFTSize, &(outputSignals[i][0]), TRUE);
		for(int j = 0; j < nFFTSize; j++)
		{
			outputSignals[i][j].re /= nFFTSize;
		}
	}
}

BOOL RealtimeMaskingKernel::ImageConvolution(const std::vector<std::vector<float> > & graphIn,
										const std::vector<std::vector<float> > & filterIn,
										std::vector<std::vector<float> > & graphOut)
{
	int w1,h1,w2,h2,w,h;
	int power;
	int size;

	w1 = graphIn[0].size();
	h1 = graphIn.size();
	w2 = filterIn[0].size();
	h2 = filterIn.size();
	w = w1 + w2;
	h = h1 + h2;

	// align width, height into power of 2;
	//power = 1;
	//while( (1 << power) < w ) power++;
	//w = (1 << power);
	//power = 1;
	//while( (1 << power) < h ) power++;
	//h = (1 << power);

	size = w * h;
	power = 1;
	while( ( 1 << power) < size) power++;
	size = (1 << power);


	std::vector<CComplex<float>> graph0, graph1;
	std::vector<CComplex<float>> spectrum0, spectrum1;
	graph0.resize(size);
	graph1.resize(size);
	spectrum0.resize(size);
	spectrum1.resize(size);

	for(int y = 0; y < h1; y++)
	{
		for(int x = 0; x < w1; x++)
		{
			graph0[y * w + x].re = graphIn[y][x];
		}
	}

	for(int y = 0; y < h2; y++)
	{
		for(int x = 0; x < w2; x++)
		{
			graph1[size - y * w - x - 1].re = filterIn[y][x];
		}
	}

	// single thread implementation
	fftEngine.FFT(&(graph0[0]), graph0.size(), &(spectrum0[0]));
	fftEngine.FFT(&(graph1[0]), graph1.size(), &(spectrum1[0]));

	// Convolution
	for(int i = 0; i < spectrum0.size(); i++)
	{
		spectrum0[i] = spectrum0[i] *  spectrum1[i];
	}

	// IFFT
	this->fftEngine.FFT(&(spectrum0[0]), spectrum0.size(), &(graph0[0]), TRUE);

	for(int y = 0; y < h1; y++)
	{
		for(int x = 0; x < w1; x++)
		{
			graphOut[y][x] = graph0[(size + x - (w2 >> 1) + 
				(y - (h2 >> 1)) * w ) % size].re / size;
		}
	}

	return TRUE;
}

CComplex<float> RealtimeMaskingKernel::GetFloatIndexedElement(
	const std::vector<CComplex<float> > &arrayData, float index)
{
	int i = (int)index;
	float x = index - i;
	const float& x1 = arrayData[i-1].re;
	const float& x2 = arrayData[i].re;
	const float& x3 = arrayData[i+1].re;
	const float& x4 = arrayData[i+2].re;	
	const float& y1 = arrayData[i-1].im;
	const float& y2 = arrayData[i].im;
	const float& y3 = arrayData[i+1].im;
	const float& y4 = arrayData[i+2].im;

	// cubic interpolation
	float re,im;
	float a,b,c,d;
	float xx = x*x;
	float xxx = xx*x;

	d = x2;
	c = (x3 - x1) * 0.5;
	float a_plus_b = x3 - c - d;
	float _3a_plus_2b = (x4 - x2) * 0.5 - c;
	a = _3a_plus_2b - a_plus_b * 2;
	b = a_plus_b * 3 - _3a_plus_2b;
	re = a * xxx + b * xx + c * x + d; 

	d = y2;
	c = (y3 - y1) *0.5;
	a_plus_b = y3 - c - d;
	_3a_plus_2b = (y4 - y2) * 0.5 - c;
	a = _3a_plus_2b - a_plus_b * 2;
	b = a_plus_b * 3 - _3a_plus_2b;
	im = a * xxx + b * xx + c * x + d; 

	return CComplex<float>(re,im);
//	return arrayData[i];
}


//
// properties, GET methods
//

const std::vector<CComplex<float> >& RealtimeMaskingKernel::GetCurrentSignal(int inputchannelindex)
{
	return inputSignals[inputchannelindex][currentFrameIndex];
}

const std::vector<CComplex<float> >& RealtimeMaskingKernel::GetCurrentSpectrum(int inputchannelindex)
{
	return spectrums[inputchannelindex][currentFrameIndex];
}

const std::vector<CComplex<float> >& RealtimeMaskingKernel::GetCurrentOutput(int outputchannelindex)
{
	return outputSignals[outputchannelindex];
}

const std::vector<CComplex<float> >& RealtimeMaskingKernel::GetCurrentOutputSpectrum(int outputchannelindex)
{
	return outputSpectrums[outputchannelindex];
}

const std::vector<std::vector<float> >& RealtimeMaskingKernel::GetMaskingGraph()
{
	return maskingGraph;
}

const std::vector<std::vector<float> >& RealtimeMaskingKernel::GetSmoothedMaskingGraph()
{
	return smoothedMaskingGraph;
}

const std::vector<MaskingResult>& RealtimeMaskingKernel::GetResult()
{
	return results;
}

const std::vector<std::vector<int> >& RealtimeMaskingKernel::GetResultLabelingGraph()
{
	return resultLabelingGraph;
}

const int RealtimeMaskingKernel::GetSamplingRate()
{
	return nSamplingRate;
}

const int RealtimeMaskingKernel::GetFFTSize()
{
	return nFFTSize;
}

const int RealtimeMaskingKernel::GetMaskingGraphSize()
{
	return nMaskingGraphSize;
}

const int RealtimeMaskingKernel::GetOutputChannelsTotal()
{
	return nOutputChannels;
}

const CString& RealtimeMaskingKernel::GetDebugString()
{
	return debugString;
}

void RealtimeMaskingKernel::EmptyDebugString()
{
	debugString = L"";
}
