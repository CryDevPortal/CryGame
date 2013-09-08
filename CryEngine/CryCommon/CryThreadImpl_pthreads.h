/////////////////////////////////////////////////////////////////////////////
//
// Crytek Source File
// Copyright (C), Crytek Studios, 2001-2006.
//
// History:
// Jun 20, 2006: Created by Sascha Demetrio
//
/////////////////////////////////////////////////////////////////////////////

#include "CryThread_pthreads.h"

#ifndef __SPU__
THREADLOCAL CrySimpleThreadSelf
	*CrySimpleThreadSelf::m_Self = NULL;
#endif
// vim:ts=2


//////////////////////////////////////////////////////////////////////////
// CryEvent(Timed) implementation
//////////////////////////////////////////////////////////////////////////
void CryEventTimed::Reset()
{
	m_lockNotify.Lock();
	m_flag = false;
	m_lockNotify.Unlock();
}

//////////////////////////////////////////////////////////////////////////
void CryEventTimed::Set()
{
	m_lockNotify.Lock();
	m_flag = true;
	m_cond.Notify();
	m_lockNotify.Unlock();
}

//////////////////////////////////////////////////////////////////////////
void CryEventTimed::Wait()
{
	m_lockNotify.Lock();
	if (!m_flag)
		m_cond.Wait(m_lockNotify);
  if (!m_bManualReset)
	m_flag	=	false;
	m_lockNotify.Unlock();
}

//////////////////////////////////////////////////////////////////////////
bool CryEventTimed::Wait( const uint32 timeoutMillis )
{
	bool bResult = true;
	m_lockNotify.Lock();
	if (!m_flag)
		bResult = m_cond.TimedWait(m_lockNotify,timeoutMillis);
	m_flag	=	false;
	m_lockNotify.Unlock();
	return bResult;
}

///////////////////////////////////////////////////////////////////////////////
// CryCriticalSection implementation
///////////////////////////////////////////////////////////////////////////////
typedef CryLockT<CRYLOCK_RECURSIVE> TCritSecType;

void  CryDeleteCriticalSection( void *cs )
{
	delete ((TCritSecType *)cs);
}

void  CryEnterCriticalSection( void *cs )
{
	((TCritSecType*)cs)->Lock();
}

bool  CryTryCriticalSection( void *cs )
{
	return false;
}

void  CryLeaveCriticalSection( void *cs )
{
	((TCritSecType*)cs)->Unlock();
}

void  CryCreateCriticalSectionInplace(void* pCS)
{
	new (pCS) TCritSecType;
}

void CryDeleteCriticalSectionInplace( void *)
{
}

void* CryCreateCriticalSection()
{
	return (void*) new TCritSecType;
}

#if !defined(__SPU__)
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CryInterlockedPushEntrySList( SLockFreeSingleLinkedListHeader& list,  SLockFreeSingleLinkedListEntry &element )
{
	assert( ((int)&element) & (MEMORY_ALLOCATION_ALIGNMENT-1) && "LockFree SingleLink List Entry has wrong Alignment" );

	STATIC_CHECK(sizeof(SLockFreeSingleLinkedListHeader) == sizeof(SLockFreeSingleLinkedListEntry*), CRY_INTERLOCKED_SLIST_HEADER_HAS_WRONG_SIZE);
	STATIC_CHECK(sizeof(SLockFreeSingleLinkedListEntry) == sizeof(SLockFreeSingleLinkedListEntry*), CRY_INTERLOCKED_SLIST_ENTRY_HAS_WRONG_SIZE);		

	SLockFreeSingleLinkedListEntry *pCurrentTop = NULL;
	volatile void *pHeader = alias_cast<volatile void*>(&list);









}

//////////////////////////////////////////////////////////////////////////
void* CryInterlockedPopEntrySList(  SLockFreeSingleLinkedListHeader& list )
{
	STATIC_CHECK(sizeof(SLockFreeSingleLinkedListHeader) == sizeof(SLockFreeSingleLinkedListEntry*), CRY_INTERLOCKED_SLIST_HEADER_HAS_WRONG_SIZE);

	struct SFreeList{ SFreeList *pNext; };
	SLockFreeSingleLinkedListEntry *pCurrentTop = NULL;
	SLockFreeSingleLinkedListEntry *pNext = NULL;
	volatile void *pHeader = alias_cast<volatile void*>(&list);












	return pCurrentTop;
}

//////////////////////////////////////////////////////////////////////////
void CryInitializeSListHead(SLockFreeSingleLinkedListHeader& list)
{
#if !defined(__SPU__)
	STATIC_CHECK(sizeof(SLockFreeSingleLinkedListHeader) == sizeof(SLockFreeSingleLinkedListEntry*), CRY_INTERLOCKED_SLIST_HEADER_HAS_WRONG_SIZE);
	list.pNext = NULL;
#endif 
}

//////////////////////////////////////////////////////////////////////////
void* CryInterlockedFlushSList(SLockFreeSingleLinkedListHeader& list)
{
	STATIC_CHECK(sizeof(SLockFreeSingleLinkedListHeader) == sizeof(SLockFreeSingleLinkedListEntry*), CRY_INTERLOCKED_SLIST_HEADER_HAS_WRONG_SIZE);
	STATIC_CHECK(sizeof(SLockFreeSingleLinkedListHeader) == sizeof(SLockFreeSingleLinkedListEntry*), CRY_INTERLOCKED_SLIST_HEADER_HAS_WRONG_SIZE);

	struct SFreeList{ SFreeList *pNext; };
	SLockFreeSingleLinkedListEntry *pCurrentTop = NULL;
	SLockFreeSingleLinkedListEntry *pNext = NULL;
	volatile void *pHeader = alias_cast<volatile void*>(&list);










	return pCurrentTop;
}






















#endif
