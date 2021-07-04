
// Fast Fourier Transform by Tao Ka Yan (Du,Jia'en)

// FFT1.cpp : Defines the entry point for the console application.
//
#ifndef FFT_H
#define FFT_H

#include "stdafx.h"
#include <windows.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <process.h>

#include "MultiThreadOptimization.h"

#ifndef PI
#define PI 3.1415926535897932384626434
#endif

template < typename T >
class CComplex
{
public:
	T re,im;
	CComplex();
	CComplex(const T& Re);
	CComplex(const T& Re, const T& Im);
	inline CComplex<T> operator+(const CComplex<T>& d2) const;
	inline CComplex<T> operator*(const CComplex<T>& d2) const;
	inline CComplex<T> operator-(const CComplex<T>& d2) const;
	inline CComplex<T> operator/(const CComplex<T>& d2) const;
	inline CComplex<T> operator-() const;
	inline double GetAngle() const; //return an angle between -PI to PI
	inline T GetAbsSqr() const;
	inline T GetAbsSqrt() const;
};

template < typename T >
CComplex<T>::CComplex()
{
	re = im = 0;
}

template < typename T >
CComplex<T>::CComplex(const T& Re)
{
	re = Re;
	im = 0;
}

template < typename T >
CComplex<T>::CComplex(const T& Re, const T& Im)
{
	re = Re;
	im = Im;
}

template < typename T >
CComplex<T> CComplex<T>::operator +(const CComplex<T>& d2) const
{
	return CComplex(re + d2.re, im + d2.im);
}

template < typename T >
CComplex<T> CComplex<T>::operator -(const CComplex<T>& d2) const
{
	return CComplex(re - d2.re, im - d2.im);
}

template < typename T >
CComplex<T> CComplex<T>::operator -() const
{
	return CComplex(-re,-im);
}

template < typename T >
CComplex<T> CComplex<T>::operator *(const CComplex<T>& d2) const
{
	return CComplex( re * d2.re - im * d2.im, re * d2.im + im * d2.re);
}

template < typename T >
CComplex<T> CComplex<T>::operator /(const CComplex<T>& d2) const
{
	T s = d2.re * d2.re + d2.im * d2.im;
	T u = (re * d2.re + im * d2.im) / s;
	T v = (im * d2.re - re * d2.im) / s;
	return CComplex(u,v);
}

template < typename T >
double CComplex<T>::GetAngle() const
{
	return atan2(im,re);
}

template < typename T >
T CComplex<T>::GetAbsSqr() const
{
	return re * re + im * im;
}

template < typename T >
T CComplex<T>::GetAbsSqrt() const
{
	return sqrt(re * re + im * im);
}


//
// -------------------------class FFT_T-----------------------------------
//

template < typename T >
class FFT_T
{
public:
	FFT_T();
	~FFT_T();
	BOOL FFT(const CComplex<T> *In, int Size, CComplex<T> *Out, BOOL bInverse = FALSE);
protected:
	static void Iteration(int degree, int startanglestride, CComplex<T>* ModulusOut, CComplex<T>* pangle);
	std::vector<CComplex<T> > angle, angleinverse;
	std::vector<int> permutation;
	int currentDegree;
	CComplex<T>* currentModulus;
	CComplex<T>* currentAngles;
	Multithread<2> dualthread;
	CRITICAL_SECTION criticalsection;
};


template < typename T >
FFT_T<T>::FFT_T()
{
	InitializeCriticalSection(&criticalsection);
}

template < typename T >
FFT_T<T>::~FFT_T()
{
	DeleteCriticalSection(&criticalsection);
}

template < typename T >
BOOL FFT_T<T>::FFT(const CComplex<T> *ModulusIn,int degree, CComplex<T> *ModulusOut, BOOL bInverse)
{
	CComplex<T> Temp,Temp2,w,w2;

	EnterCriticalSection(&criticalsection);


	long a,b,c,x,y,count;
//	T alpha;

	_ASSERT(degree == 1 << int( log(double(degree)) / log(2.0) + 1e-12 ));

	// permutation
	int *ppermu;
	if( permutation.size() != degree)
	{
		permutation.resize(degree);
		ppermu = &(permutation[0]);
		count=1;a=1;b=degree>>1;x=0;
		ppermu[0]=0;
		while(count<degree)

		{
			ppermu[count]=ppermu[count-a]+b;
			x++;
			count++;
			if(x==a)
			{
				a<<=1;
				x=0;
				b>>=1;
			}
		}
	}
	ppermu = &(permutation[0]);
	for(int i = 0; i < degree; i++)
	{
		ModulusOut[i] = ModulusIn[ppermu[i]];
	}

	// init angles
	if( angle.size() != degree)
	{
		CComplex<T>* pangle;
		angle.resize(degree);
		angleinverse.resize(degree);
		pangle = &(angle[0]);

		int quartersize = degree / 4;
		T oneoversize = ((T)(1.0)) / degree; 
		for(count = 0; count < quartersize; count++)
		{
			T beta;
			beta = (T)(2.0 * PI * count * oneoversize);
			pangle[count].re = cos(beta);
			pangle[count].im = sin(beta);
			pangle[count + quartersize].re = -pangle[count].im;
			pangle[count + quartersize].im = pangle[count].re;
			pangle[count + 2 * quartersize].re = -pangle[count].re;
			pangle[count + 2 * quartersize].im = -pangle[count].im;
			pangle[count + 3 * quartersize].re = pangle[count].im;
			pangle[count + 3 * quartersize].im = -pangle[count].re;
		}
		for(count = 0; count < degree; count++)
		{
			angleinverse[count].re = pangle[count].re;
			angleinverse[count].im = -pangle[count].im;
		}
	}


	//Fast Flourier Transform

	//
	// Iterations
	//

	{	
		this->currentAngles = bInverse == TRUE? &(angleinverse[0]) : &(angle[0]);
		this->currentDegree = degree;
		this->currentModulus = ModulusOut;

		dualthread.Lock();

		// thread 0
		MULTITHREAD_DECLARE_BEGIN(T0, arg);
		FFT_T<T>* This = (FFT_T<T>*)arg;
		This->Iteration(This->currentDegree / 2, This->currentDegree / 2, This->currentModulus, This->currentAngles);
		MULTITHREAD_DECLARE_END();
		MULTITHREAD_ASSIGN_TASK(dualthread, 0, T0, this);

		// thread 1
		MULTITHREAD_DECLARE_BEGIN(T1, arg);
		FFT_T<T>* This = (FFT_T<T>*)arg;
		This->Iteration(This->currentDegree / 2, This->currentDegree / 2, 
			This->currentModulus + (This->currentDegree / 2), This->currentAngles);
		MULTITHREAD_DECLARE_END();
		MULTITHREAD_ASSIGN_TASK(dualthread, 1, T1, this);

		dualthread.Run();
		dualthread.Unlock();
	}


	//
	// combination of results
	//

	{
		dualthread.Lock();

		// thread 0: left half combination
		MULTITHREAD_DECLARE_BEGIN(C0, arg);
		FFT_T<T>* This = (FFT_T<T>*)arg;
		int half = This->currentDegree / 2;
		int quarter = This->currentDegree / 4;
		for(int i = 0; i < quarter; i++)
		{
			CComplex<T> Temp = This->currentModulus[i];
			CComplex<T> Temp2 = This->currentAngles[i] * This->currentModulus[i + half];
			This->currentModulus[i] = Temp + Temp2;
			This->currentModulus[i + half] = Temp - Temp2;
		}
		MULTITHREAD_DECLARE_END();
		MULTITHREAD_ASSIGN_TASK(dualthread, 0, C0, this);

		// thread 1: right half combine
		MULTITHREAD_DECLARE_BEGIN(C1, arg);
		FFT_T<T>* This = (FFT_T<T>*)arg;
		int half = This->currentDegree / 2;
		int quarter = This->currentDegree / 4;
		for(int i = quarter; i < half; i++)
		{
			CComplex<T> Temp = This->currentModulus[i];
			CComplex<T> Temp2 = This->currentAngles[i] * This->currentModulus[i + half];
			This->currentModulus[i] = Temp + Temp2;
			This->currentModulus[i + half] = Temp - Temp2;
		}
		MULTITHREAD_DECLARE_END();
		MULTITHREAD_ASSIGN_TASK(dualthread, 1, C1, this);

		dualthread.Run();
		dualthread.Unlock();
	}

	LeaveCriticalSection(&criticalsection);
	return TRUE;

}

template<typename T>
void FFT_T<T>::Iteration(int degree, int AngleStep, CComplex<T>* ModulusOut, CComplex<T>* pangle)
{
	int AnglePos = 0;
	int a = 2;
	int b, count, c, x, y;
	while(a <= degree)
	{
		b=a>>1;
		count=0,c=0,x=0;
		AnglePos = 0;		
		for(;;)
		{
			if(count==b)
			{
				c+=a;
				if(c>=degree)break;
				count=0;
				x = c;
				AnglePos = 0;
			}
			y=x+b;
			CComplex<T> Temp = ModulusOut[x];
			CComplex<T> Temp2 = pangle[AnglePos] * ModulusOut[y];
			
			ModulusOut[x].re = Temp.re + Temp2.re;
			ModulusOut[x].im = Temp.im + Temp2.im;
			ModulusOut[y].re = Temp.re - Temp2.re;
			ModulusOut[y].im = Temp.im - Temp2.im;
			
			AnglePos += AngleStep;
			count++;
			x++;
		}
		a <<= 1;
		AngleStep >>= 1;
	}
}

#endif // FFT_H