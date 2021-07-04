
#include "stdafx.h"
#include "MultiThreadOptimization.h"

BOOL MultiQuickSort::MultiQSort(void* base_addr, int elementtotal, int sizeofelement, 
		int (*compare)(const void* , const void* ) )
{
	std::vector<BYTE> tempdata;
	tempdata.resize(elementtotal * sizeofelement);

	BYTE* base = (BYTE*)base_addr;
	BYTE* pointer = &(tempdata[0]);

	memcpy(pointer, base, elementtotal * sizeofelement);

	int half = elementtotal / 2;

	void* threaddata0[] = {pointer, (void*)half, (void*)sizeofelement, (void*)compare};
	void* threaddata1[] = {pointer + half * sizeofelement, 
		(void*)(elementtotal - half), (void*)sizeofelement, (void*)compare};

	multithread.Lock();
	MULTITHREAD_DECLARE_BEGIN(T0, arg);
	void** args = (void**)arg;
	qsort(args[0], (UINT)args[1], (UINT)args[2], (int (*)(const void* , const void* ))args[3]);
	MULTITHREAD_DECLARE_END();

	MULTITHREAD_ASSIGN_TASK(multithread, 0, T0, threaddata0);
	MULTITHREAD_ASSIGN_TASK(multithread, 1, T0, threaddata1);
	multithread.Run();
	multithread.Unlock();

	int p = 0;
	int q = half;
	int i = 0;
	while( p < half && q < elementtotal )
	{
		if( compare(pointer + p * sizeofelement, pointer + q * sizeofelement) > 0 )
		{
			memcpy(base + i * sizeofelement, pointer + q * sizeofelement, sizeofelement);
			q++;
		}
		else
		{
			memcpy(base + i * sizeofelement, pointer + p * sizeofelement, sizeofelement);
			p++;
		}
		i++;
	}
	while( p < half)
	{
		memcpy(base + i * sizeofelement, pointer + p * sizeofelement, sizeofelement);
		p++; i++;
	}
	while( q < elementtotal)
	{
		memcpy(base + i * sizeofelement, pointer + q * sizeofelement, sizeofelement);
		q++; i++;
	}

#ifdef _DEBUG
	for(i = 0; i < elementtotal - 1; i++)
	{
		int r = compare(base + (i+1) * sizeofelement, base + (i) * sizeofelement);
		_ASSERT_EXPR(r >= 0,L"Error in Qsort");
	}
#endif
	return TRUE;
}
