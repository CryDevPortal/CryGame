#ifndef __CRYTHREADIMPL_WINDOWS_H__
#define __CRYTHREADIMPL_WINDOWS_H__
#pragma once

//#include <IThreadTask.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef XENON
#include <windows.h>
#endif
#include <process.h>

struct SThreadNameDesc
{
	DWORD dwType;
	LPCSTR szName;
	DWORD dwThreadID;
	DWORD dwFlags;
};

THREADLOCAL CrySimpleThreadSelf* CrySimpleThreadSelf::m_Self = NULL;

//////////////////////////////////////////////////////////////////////////
CryEvent::CryEvent(bool bManualReset)
{
	m_handle = (void*)CreateEvent(NULL, bManualReset, FALSE, NULL);
}

//////////////////////////////////////////////////////////////////////////
CryEvent::~CryEvent()
{
	CloseHandle(m_handle);
}

//////////////////////////////////////////////////////////////////////////
void CryEvent::Reset()
{
	ResetEvent(m_handle);
}

//////////////////////////////////////////////////////////////////////////
void CryEvent::Set()
{
	SetEvent(m_handle);
}

//////////////////////////////////////////////////////////////////////////
void CryEvent::Wait() const
{
	WaitForSingleObject(m_handle, INFINITE);
}

//////////////////////////////////////////////////////////////////////////
bool CryEvent::Wait( const uint32 timeoutMillis ) const
{
	if (WaitForSingleObject(m_handle, timeoutMillis) == WAIT_TIMEOUT)
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////
// CryLock_WinMutex
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CryLock_WinMutex::CryLock_WinMutex() : m_hdl(CreateMutex(NULL, FALSE, NULL)) {}
CryLock_WinMutex::~CryLock_WinMutex()
{
	CloseHandle(m_hdl);
}

//////////////////////////////////////////////////////////////////////////
void CryLock_WinMutex::Lock()
{
	WaitForSingleObject(m_hdl, INFINITE);
}

//////////////////////////////////////////////////////////////////////////
void CryLock_WinMutex::Unlock()
{
	ReleaseMutex(m_hdl);
}

//////////////////////////////////////////////////////////////////////////
bool CryLock_WinMutex::TryLock()
{
	return WaitForSingleObject(m_hdl, 0) != WAIT_TIMEOUT;
}

//////////////////////////////////////////////////////////////////////////
// CryLock_CritSection
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CryLock_CritSection::CryLock_CritSection()
{
	InitializeCriticalSection((CRITICAL_SECTION*)&m_cs);
}

//////////////////////////////////////////////////////////////////////////
CryLock_CritSection::~CryLock_CritSection()
{
	DeleteCriticalSection((CRITICAL_SECTION*)&m_cs);
}

//////////////////////////////////////////////////////////////////////////
void CryLock_CritSection::Lock()
{
	EnterCriticalSection((CRITICAL_SECTION*)&m_cs);
}

//////////////////////////////////////////////////////////////////////////
void CryLock_CritSection::Unlock()
{
	LeaveCriticalSection((CRITICAL_SECTION*)&m_cs);
}

//////////////////////////////////////////////////////////////////////////
bool CryLock_CritSection::TryLock()
{
	return TryEnterCriticalSection((CRITICAL_SECTION*)&m_cs) != FALSE;
}

//////////////////////////////////////////////////////////////////////////
// most of this is taken from http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
//////////////////////////////////////////////////////////////////////////
CryConditionVariable::CryConditionVariable()
{
	m_waitersCount = 0;
	m_wasBroadcast = 0;
	m_sema = CreateSemaphore(NULL, 0, 0x7fffffff, NULL);
	InitializeCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
	m_waitersDone = CreateEvent(NULL, FALSE, FALSE, NULL);
}

//////////////////////////////////////////////////////////////////////////
CryConditionVariable::~CryConditionVariable()
{
	CloseHandle(m_sema);
	DeleteCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
	CloseHandle(m_waitersDone);
}

//////////////////////////////////////////////////////////////////////////
void CryConditionVariable::Wait( LockType& lock )
{
	EnterCriticalSection( (CRITICAL_SECTION*)&m_waitersCountLock );
	m_waitersCount ++;
	LeaveCriticalSection( (CRITICAL_SECTION*)&m_waitersCountLock );

	SignalObjectAndWait( lock._get_win32_handle(), m_sema, INFINITE, FALSE );

	EnterCriticalSection( (CRITICAL_SECTION*)&m_waitersCountLock );
	m_waitersCount --;
	bool lastWaiter = m_wasBroadcast && m_waitersCount == 0;
	LeaveCriticalSection( (CRITICAL_SECTION*)&m_waitersCountLock );

	if (lastWaiter)
		SignalObjectAndWait( m_waitersDone, lock._get_win32_handle(), INFINITE, FALSE );
	else
		WaitForSingleObject( lock._get_win32_handle(), INFINITE );
}

//////////////////////////////////////////////////////////////////////////
bool CryConditionVariable::TimedWait( LockType& lock, uint32 millis )
{
	EnterCriticalSection( (CRITICAL_SECTION*)&m_waitersCountLock );
	m_waitersCount ++;
	LeaveCriticalSection( (CRITICAL_SECTION*)&m_waitersCountLock );

	bool ok = true;
	if (WAIT_TIMEOUT == SignalObjectAndWait( lock._get_win32_handle(), m_sema, millis, FALSE ))
		ok = false;

	EnterCriticalSection( (CRITICAL_SECTION*)&m_waitersCountLock );
	m_waitersCount --;
	bool lastWaiter = m_wasBroadcast && m_waitersCount == 0;
	LeaveCriticalSection( (CRITICAL_SECTION*)&m_waitersCountLock );

	if (lastWaiter)
		SignalObjectAndWait( m_waitersDone, lock._get_win32_handle(), INFINITE, FALSE );
	else
		WaitForSingleObject( lock._get_win32_handle(), INFINITE );

	return ok;
}

//////////////////////////////////////////////////////////////////////////
void CryConditionVariable::NotifySingle()
{
	EnterCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
	bool haveWaiters = m_waitersCount > 0;
	LeaveCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
	if (haveWaiters)
		ReleaseSemaphore(m_sema, 1, 0);
}

//////////////////////////////////////////////////////////////////////////
void CryConditionVariable::Notify()
{
	EnterCriticalSection( (CRITICAL_SECTION*)&m_waitersCountLock );
	bool haveWaiters = false;
	if (m_waitersCount > 0)
	{
		m_wasBroadcast = 1;
		haveWaiters = true;
	}
	if (haveWaiters)
	{
		ReleaseSemaphore( m_sema, m_waitersCount, 0 );
		LeaveCriticalSection( (CRITICAL_SECTION*)&m_waitersCountLock );
		WaitForSingleObject( m_waitersDone, INFINITE );
		m_wasBroadcast = 0;
	}
	else
	{
		LeaveCriticalSection( (CRITICAL_SECTION*)&m_waitersCountLock );
	}
}

//////////////////////////////////////////////////////////////////////////
CrySemaphore::CrySemaphore(int nMaximumCount)
{
	m_Semaphore = (void*)CreateSemaphore(NULL,0,nMaximumCount, NULL);
}

//////////////////////////////////////////////////////////////////////////
CrySemaphore::~CrySemaphore()
{
	CloseHandle((HANDLE)m_Semaphore);
}

//////////////////////////////////////////////////////////////////////////
void CrySemaphore::Acquire()
{
	WaitForSingleObject((HANDLE)m_Semaphore,INFINITE);
}

//////////////////////////////////////////////////////////////////////////
void CrySemaphore::Release()
{
		ReleaseSemaphore((HANDLE)m_Semaphore,1,NULL);
}

//////////////////////////////////////////////////////////////////////////
CryFastSemaphore::CryFastSemaphore(int nMaximumCount) :
	m_Semaphore(nMaximumCount),
	m_nCounter(0)
{
}

//////////////////////////////////////////////////////////////////////////
CryFastSemaphore::~CryFastSemaphore()
{	
}

//////////////////////////////////////////////////////////////////////////
void CryFastSemaphore::Acquire()
{
	int nCount = ~0;
	do
	{
		nCount = *const_cast<volatile int*>(&m_nCounter);
	}while( CryInterlockedCompareExchange( alias_cast<volatile long*>(&m_nCounter), nCount - 1, nCount) != nCount );
	
	// if the count would have been 0 or below, go to kernel semaphore
	if( (nCount - 1)  < 0 )
		m_Semaphore.Acquire();
}

//////////////////////////////////////////////////////////////////////////
void CryFastSemaphore::Release()
{
	int nCount = ~0;
	do
	{
		nCount = *const_cast<volatile int*>(&m_nCounter);
	}while( CryInterlockedCompareExchange( alias_cast<volatile long*>(&m_nCounter), nCount + 1, nCount) != nCount );
	
	// wake up kernel semaphore if we have waiter
	if( nCount < 0 )
		m_Semaphore.Release();
}

//////////////////////////////////////////////////////////////////////////
CrySimpleThreadSelf::CrySimpleThreadSelf()
	: m_thread(NULL)
	, m_threadId(0)
{
}

//////////////////////////////////////////////////////////////////////////
void CrySimpleThreadSelf::WaitForThread()
{
	assert(m_thread);
	if( GetCurrentThreadId() != m_threadId )
	{
		WaitForSingleObject( (HANDLE)m_thread, INFINITE );
	}
}

//////////////////////////////////////////////////////////////////////////
CrySimpleThreadSelf::~CrySimpleThreadSelf()
{
	if(m_thread)
		CloseHandle(m_thread);
}

//////////////////////////////////////////////////////////////////////////
void CrySimpleThreadSelf::StartThread(unsigned (__stdcall *func)(void*), void* argList)
{



	m_thread = (void*)_beginthreadex( NULL, 0, func, argList, CREATE_SUSPENDED, &m_threadId );

	assert(m_thread);
	ResumeThread((HANDLE)m_thread);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CryInterlockedPushEntrySList( SLockFreeSingleLinkedListHeader& list,  SLockFreeSingleLinkedListEntry &element )
{
	STATIC_CHECK(sizeof(SLockFreeSingleLinkedListHeader) == sizeof(SLIST_HEADER), CRY_INTERLOCKED_SLIST_HEADER_HAS_WRONG_SIZE);
	STATIC_CHECK(sizeof(SLockFreeSingleLinkedListEntry) == sizeof(SLIST_ENTRY), CRY_INTERLOCKED_SLIST_ENTRY_HAS_WRONG_SIZE);		



	assert( ((int)&element) & (MEMORY_ALLOCATION_ALIGNMENT-1) && "LockFree SingleLink List Entry has wrong Alignment" );
	assert( ((int)&list) & (MEMORY_ALLOCATION_ALIGNMENT-1) && "LockFree SingleLink List Header has wrong Alignment" );
	InterlockedPushEntrySList( alias_cast<PSLIST_HEADER>(&list), alias_cast<PSLIST_ENTRY>(&element) );

}

//////////////////////////////////////////////////////////////////////////
void* CryInterlockedPopEntrySList(  SLockFreeSingleLinkedListHeader& list )
{	
	STATIC_CHECK(sizeof(SLockFreeSingleLinkedListHeader) == sizeof(SLIST_HEADER), CRY_INTERLOCKED_SLIST_HEADER_HAS_WRONG_SIZE);



	assert( ((int)&list) & (MEMORY_ALLOCATION_ALIGNMENT-1) && "LockFree SingleLink List Header has wrong Alignment" );
	return reinterpret_cast<void*>(InterlockedPopEntrySList(alias_cast<PSLIST_HEADER>(&list)));	

}

//////////////////////////////////////////////////////////////////////////
void CryInitializeSListHead(SLockFreeSingleLinkedListHeader& list)
{
#if !defined(XENON)
	assert( ((int)&list) & (MEMORY_ALLOCATION_ALIGNMENT-1) && "LockFree SingleLink List Header has wrong Alignment" );
#endif
	STATIC_CHECK(sizeof(SLockFreeSingleLinkedListHeader) == sizeof(SLIST_HEADER), CRY_INTERLOCKED_SLIST_HEADER_HAS_WRONG_SIZE);
	InitializeSListHead(alias_cast<PSLIST_HEADER>(&list));
}

//////////////////////////////////////////////////////////////////////////
void* CryInterlockedFlushSList(SLockFreeSingleLinkedListHeader& list)
{
#if !defined(XENON)
	assert( ((int)&list) & (MEMORY_ALLOCATION_ALIGNMENT-1) && "LockFree SingleLink List Header has wrong Alignment" );
#endif
	STATIC_CHECK(sizeof(SLockFreeSingleLinkedListHeader) == sizeof(SLIST_HEADER), CRY_INTERLOCKED_SLIST_HEADER_HAS_WRONG_SIZE);
	return InterlockedFlushSList(alias_cast<PSLIST_HEADER>(&list));	
}

#endif //__CRYTHREADIMPL_WINDOWS_H__
