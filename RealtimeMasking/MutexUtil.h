#pragma once

/*=====================================================================
*
* File:        MutexUtil.h
* Namespace:   MutexUtils
* Description: Implementation of mutex, event, semaphore, etc.
*
* First Writer:     Xiao Rong (Microsoft Research Asia)
* First Write Data: 
* 
* Modify Data:      September, 2007 -- January, 2008
* Modified by:      Zhu Huaiyi
*
*====================================================================*/

#define EVENT_ABANDONED		-0x080L
#define EVENT_TIMEOUT		-0x0102L
#define EVENT_FAIL			-1
#include <Windows.h>
#include <iostream>
#include <map>
#include <stack>
#include <assert.h>

namespace MutexUtils
{

class CMutex
{
private:
	CRITICAL_SECTION mutex_;
	// = Disallow copying and assignment.
	CMutex (const CMutex &);
	void operator= (const CMutex &);

public:
	CMutex (void)	{InitializeCriticalSection (&mutex_);}
	~CMutex (void)	{DeleteCriticalSection (&mutex_);}
	void Lock (void)	{EnterCriticalSection (&mutex_); }
	void UnLock (void)	{LeaveCriticalSection (&mutex_); }
	//BOOL trylock(void) {return TryEnterCriticalSection(&mutex_);	}
};

class CAutoMutexLock
{
public:
	CAutoMutexLock(CMutex& mutex): m_mutex(mutex)
	{
		m_mutex.Lock();
	}
	~CAutoMutexLock()
	{
		m_mutex.UnLock();
	}

private:
	CMutex& m_mutex;
};

template <class T>
class CMultiMutexT
{
private:
	class CRefMutex : public CMutex
	{
		int m_iRef;
	public:
		CRefMutex() : m_iRef(0) {}
		void IncRef () { m_iRef++;	}
		int  UnLock()
		{ 
			CMutex::UnLock();
			return --m_iRef;		
		}
	};
	typedef std::map<T,CRefMutex*> MultiMap;
	typedef typename MultiMap::iterator   MultiMapIter;
	typedef std::stack<CRefMutex*> MutexStack;

	MultiMap	m_multiMap;
	MutexStack  m_freeStack;

	CMutex		m_mapMutex;
public:
	~CMultiMutexT (void)	
	{
		m_mapMutex.Lock();	
		for ( MultiMapIter iter = m_multiMap.begin(); iter!=m_multiMap.end(); iter++)
		{
			delete iter->second;
		}

		while(!m_freeStack.empty())
		{
			delete m_freeStack.top();
			m_freeStack.pop();
		}
		m_mapMutex.UnLock();
	}
	bool Lock (T tvalue)	
	{
		CRefMutex* pMutex=NULL;
		m_mapMutex.Lock();
		MultiMapIter iter = m_multiMap.find(tvalue);
		if ( iter==m_multiMap.end())
		{
			if ( m_freeStack.empty() )
			{
				pMutex = new CRefMutex;
			}
			else
			{
				pMutex = m_freeStack.top();
				m_freeStack.pop();
			}
			if ( pMutex)
			{
				m_multiMap.insert(std::make_pair(tvalue,pMutex));
			}
		}
		else if ( iter->second)
		{
			pMutex = iter->second;
		}

		if (pMutex)
		{
			pMutex->IncRef();
		}
		m_mapMutex.UnLock();
		if (pMutex)
			pMutex->Lock();
		return pMutex==NULL;
	}
	bool UnLock (T tvalue)	
	{ 
		CRefMutex* pMutex=NULL;
		m_mapMutex.Lock();
		MultiMapIter iter = m_multiMap.find(tvalue);
		if ( iter!=m_multiMap.end())
		{
			pMutex = iter->second;
		}
		if (pMutex &&	pMutex->UnLock()==0)
		{
			m_freeStack.push(pMutex);
			m_multiMap.erase(iter);
		}
		m_mapMutex.UnLock();

		return pMutex==NULL;
	}
};

class CEvent
{
protected:
	HANDLE	m_handle;
public:
	CEvent() : m_handle(NULL) {}
	CEvent( const HANDLE& handle) : m_handle(handle) {}
	operator HANDLE() const { return m_handle; }
	HANDLE Create(	LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset,	BOOL bInitialState,	LPCTSTR lpName )
	{
		if ( m_handle!=NULL )
		{
			Close();
		}
		m_handle = CreateEvent(lpEventAttributes, bManualReset, bInitialState, lpName);
		return m_handle;
	}
	CEvent Dup() const { return CEvent(m_handle);	}
	HANDLE Create()	{ return Create(NULL, TRUE, FALSE, NULL);	}
	
	HANDLE Open( DWORD dwDesiredAccess,	BOOL bInheritHandle, LPCTSTR lpName )	{ return OpenEvent(dwDesiredAccess, bInheritHandle, lpName); }
	BOOL Close()	
	{ 
		BOOL bRet = FALSE;
		if ( m_handle!=NULL ) 
		{ 
			bRet = CloseHandle(m_handle); 
			m_handle=NULL;
		} 
		return bRet;
	}
	BOOL Set() { return SetEvent(m_handle); }
	BOOL Reset() { return ResetEvent(m_handle); }
	BOOL IsNull() { return m_handle==NULL;	}
	DWORD Wait(DWORD dwMilliseconds	) { return WaitForSingleObject(m_handle, dwMilliseconds); }
};

class CSemaphore
{
protected:
	HANDLE	m_handle;
public:
	CSemaphore() : m_handle(NULL) {}
	CSemaphore( const HANDLE& handle) : m_handle(handle) {}
	operator HANDLE() const { return m_handle; }
	HANDLE Create(	LPSECURITY_ATTRIBUTES lpEventAttributes,   LONG lInitialCount, LONG lMaximumCount,	LPCTSTR lpName )
	{
		if ( m_handle!=NULL )
		{
			Close();
		}
		m_handle = CreateSemaphore(lpEventAttributes, lInitialCount, lMaximumCount, lpName);
		return m_handle;
	}
	CSemaphore Dup() const { return CSemaphore(m_handle);	}

	HANDLE Open( DWORD dwDesiredAccess,	BOOL bInheritHandle, LPCTSTR lpName )	{ return OpenSemaphore(dwDesiredAccess, bInheritHandle, lpName); }
	BOOL Close()	
	{ 
		BOOL bRet = FALSE;
		if ( m_handle!=NULL ) 
		{ 
			bRet = CloseHandle(m_handle); 
			m_handle=NULL;
		} 
		return bRet;
	}
	BOOL Release(LONG lReleaseCount, LONG& lPreviousCount) { return ReleaseSemaphore(m_handle,lReleaseCount, &lPreviousCount); }
	BOOL Release()  {return ReleaseSemaphore(m_handle,1,NULL);}
	BOOL Reset() { return ResetEvent(m_handle); }
	BOOL IsNull() { return m_handle==NULL;	}
	DWORD Wait(DWORD dwMilliseconds	) { return WaitForSingleObject(m_handle, dwMilliseconds); }
};


//----------------------------------------------------------------------------------------------------------------
// A MultiArg version of WaitForMultipleObjects
// Remarks:
//	Return value >=0		if bWaitAll=TRUE	 the state of all specified objects is signaled
//							if bWaitAll=FALSE	 return the smallest index value of all the signaled objects
//	Return value<=EVENT_ABANDONED and >EVENT_ABANDONED-nHandles 
//							if bWaitAll=TRUE	 the state of all specified objects is signaled, at least one of the objects is an abandoned mutex object
//							if bWaitAll=FALSE	 return the index of the abandoned mutex object that satisfied the wait
//	Return value == EVENT_TIMEOUT		timeout
//----------------------------------------------------------------------------------------------------------------
inline int WaitHandles (BOOL bWaitAll,  DWORD dwMilliseconds, int nHandles,  ...)
{
	va_list argptr;
	HANDLE* arrHandle = new HANDLE[nHandles];
	int iRet = EVENT_FAIL;
	if ( arrHandle!=NULL )
	{
		va_start(argptr, nHandles);
		for ( int i=0; i<nHandles; i++ )
		{
			arrHandle[i]=va_arg(argptr, HANDLE);
		}
		va_end(argptr);
		DWORD WaitRet = WaitForMultipleObjects(nHandles, arrHandle, bWaitAll, dwMilliseconds);
		if ( WaitRet>= WAIT_OBJECT_0 && WaitRet < WAIT_OBJECT_0 + nHandles)	//event signaled
		{
			iRet = WaitRet - WAIT_OBJECT_0;
		}
		else if ( WaitRet>=WAIT_ABANDONED_0 && WaitRet < WAIT_ABANDONED_0 + nHandles)
		{
			iRet = WAIT_ABANDONED_0 - WaitRet + EVENT_ABANDONED;
		}
		else if ( WaitRet == WAIT_TIMEOUT )
		{
			iRet = EVENT_TIMEOUT;
		}
		else if ( WaitRet == WAIT_FAILED)
		{
			iRet = EVENT_FAIL; 
			std::cout << GetLastError() << std::endl;
		}
		else
		{
			assert(FALSE);
		}
		delete[] arrHandle;
	}
	return iRet;	
}

}