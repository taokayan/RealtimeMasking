
#ifndef MULTITREADOPTIMIZATION_H
#define MULTITREADOPTIMIZATION_H

#include <process.h>
#include <windows.h>
#include <errno.h>

class ThreadInstance
{
public:
	HANDLE eventstart;
	HANDLE eventfinished;
	void (*pfun)(void *);
	void* funarg;

public:
	ThreadInstance()
	{
		do{
			eventstart = CreateEvent(NULL,TRUE,FALSE,NULL);
		}while(eventstart == NULL);
		do{
			eventfinished = CreateEvent(NULL,TRUE,FALSE,NULL);
		}while(eventfinished == NULL);
		pfun = NULL;
		funarg = NULL;
	}
	~ThreadInstance()
	{
		CloseHandle(eventstart);
		CloseHandle(eventfinished);
	}

	// set new task
	void setfunc(void (*funIn)(void*), void* argIn)
	{
		pfun = funIn;
		funarg = argIn;
	}

	// run
	static void __cdecl run(void* arg)
	{
		ThreadInstance* paras = (ThreadInstance*)arg;
		while(TRUE)
		{
			// wait for a new task
			WaitForSingleObject(paras->eventstart, INFINITE);
			//_ASSERT_EXPR(paras->pfun != NULL, L"thread does not have run address");
			ResetEvent(paras->eventstart);

			if( paras->pfun != NULL)
			{
				// run the function body
				paras->pfun(paras->funarg);
			}

			// set finished signal
			paras->pfun = NULL;
			while( SetEvent(paras->eventfinished) == FALSE);
		}
		return;
	}

};

template <int ThreadTotal>
class Multithread
{

protected:
	ThreadInstance ti[ThreadTotal];
	HANDLE handle[ThreadTotal];
	CRITICAL_SECTION criticalsection;

public:
	// bound a task to a thread
	void SetThreadTask(int threadIndex, void (*funIn)(void*), void* argIn)
	{
		ti[threadIndex].setfunc(funIn, argIn);
	}

	Multithread()
	{
		for(int i = 0; i < ThreadTotal; i++)
		{
			do{
				try{handle[i] = (HANDLE)_beginthread(ThreadInstance::run,0,&(ti[i]));}
				catch(...){}
			}while(handle[i] == (HANDLE)-1L || handle[i] == (HANDLE)EINVAL || handle[i] == (HANDLE)EACCES );		
		}
		InitializeCriticalSection(&criticalsection);
	}

	~Multithread()
	{
		for(int i = 0; i < ThreadTotal; i++)
		{
			CloseHandle(handle[i]);
		}
	}

	void Lock()
	{
		EnterCriticalSection(&criticalsection);
	}

	void Unlock()
	{
		LeaveCriticalSection(&criticalsection);
	}

	// run multithreads simultaneously
	BOOL Run()
	{
		DWORD r;
		for(int i = 0; i < ThreadTotal; i++)
		{
			ResetEvent(ti[i].eventfinished);
			SetEvent(ti[i].eventstart);
		}
		for(int i = 0; i < ThreadTotal; i++)
		{
			do
			{
				r = WaitForSingleObject(ti[i].eventfinished,INFINITE);
			}while(r == WAIT_FAILED);
		}
		return TRUE;
	}
};

#define MULTITHREAD_DECLARE_BEGIN(ThreadName, ArgumentName) \
	class ThreadName \
	{ \
	public: \
		static void run(void* ArgumentName) \
		{

#define MULTITHREAD_DECLARE_END() }};

#define MULTITHREAD_ASSIGN_TASK(MultithreadInstance, ThreadAssignIndex, ThreadName, ArgumentValue) \
	MultithreadInstance.SetThreadTask((ThreadAssignIndex), (void (*)(void*))ThreadName::run, (void*)(ArgumentValue))



class MultiQuickSort
{
	Multithread<2> multithread;
public:
	BOOL MultiQSort(void* base_addr, int elementtotal, int sizeofelement, int (*compare)(const void* , const void* ));
};

#endif