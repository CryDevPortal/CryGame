////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2005.
// -------------------------------------------------------------------------
//  File name:   MultiThread.h
//  Version:     v1.00
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __MultiThread_h__
#define __MultiThread_h__
#pragma once





#define WRITE_LOCK_VAL (1<<16)

typedef volatile int TRWLock;







//as PowerPC operates via cache line reservation, lock variables should reside ion their own cache line
template <class T>
struct SAtomicVar
{
	T val;



	inline operator T()const{return val;}
	inline operator T()volatile const{return val;}
	inline void operator =(const T& rV){val = rV;return *this;}
	inline void Assign(const T& rV){val = rV;}
	inline void Assign(const T& rV)volatile{val = rV;}
	inline T* Addr() {return &val;}
	inline volatile T* Addr() volatile {return &val;}

	inline bool operator<(const T& v)const{return val < v;}
	inline bool operator<(const SAtomicVar<T>& v)const{return val < v.val;}
	inline bool operator>(const T& v)const{return val > v;}
	inline bool operator>(const SAtomicVar<T>& v)const{return val > v.val;}
	inline bool operator<=(const T& v)const{return val <= v;}
	inline bool operator<=(const SAtomicVar<T>& v)const{return val <= v.val;}
	inline bool operator>=(const T& v)const{return val >= v;}
	inline bool operator>=(const SAtomicVar<T>& v)const{return val >= v.val;}
	inline bool operator==(const T& v)const{return val == v;}
	inline bool operator==(const SAtomicVar<T>& v)const{return val == v.val;}
	inline bool operator!=(const T& v)const{return val != v;}
	inline bool operator!=(const SAtomicVar<T>& v)const{return val != v.val;}
	inline T operator*(const T& v)const{return val * v;}
	inline T operator/(const T& v)const{return val / v;}
	inline T operator+(const T& v)const{return val + v;}
	inline T operator-(const T& v)const{return val - v;}

	inline bool operator<(const T& v)volatile const{return val < v;}
	inline bool operator<(const SAtomicVar<T>& v)volatile const{return val < v.val;}
	inline bool operator>(const T& v)volatile const{return val > v;}
	inline bool operator>(const SAtomicVar<T>& v)volatile const{return val > v.val;}
	inline bool operator<=(const T& v)volatile const{return val <= v;}
	inline bool operator<=(const SAtomicVar<T>& v)volatile const{return val <= v.val;}
	inline bool operator>=(const T& v)volatile const{return val >= v;}
	inline bool operator>=(const SAtomicVar<T>& v)volatile const{return val >= v.val;}
	inline bool operator==(const T& v)volatile const{return val == v;}
	inline bool operator==(const SAtomicVar<T>& v)volatile const{return val == v.val;}
	inline bool operator!=(const T& v)volatile const{return val != v;}
	inline bool operator!=(const SAtomicVar<T>& v)volatile const{return val != v.val;}
	inline T operator*(const T& v)volatile const{return val * v;}
	inline T operator/(const T& v)volatile const{return val / v;}
	inline T operator+(const T& v)volatile const{return val + v;}
	inline T operator-(const T& v)volatile const{return val - v;}
}





;

typedef SAtomicVar<int> TIntAtomic;
typedef SAtomicVar<unsigned int> TUIntAtomic;
typedef SAtomicVar<float> TFloatAtomic;

#ifdef __SNC__
	#ifndef __add_db16cycl__
		#define __add_db16cycl__ __db16cyc();
	#endif
#else
	#define USE_INLINE_ASM
	//#define ADD_DB16_CYCLES
	#undef __add_db16cycl__
	#ifdef ADD_DB16_CYCLES
		#define __add_db16cycl__ __asm__ volatile("db16cyc");
	#else	
		#define __add_db16cycl__ 
	#endif
#endif

#if !defined(__SPU__)
	void CrySpinLock(volatile int *pLock,int checkVal,int setVal);
  void CryReleaseSpinLock(volatile int*, int);
	#if !defined(PS3)
		long   CryInterlockedIncrement( int volatile *lpAddend );
		long   CryInterlockedDecrement( int volatile *lpAddend );
		long   CryInterlockedExchangeAdd(long volatile * lpAddend, long Value);
		long	 CryInterlockedCompareExchange(long volatile * dst, long exchange, long comperand);
		void*  CryInterlockedExchangePointer(void* volatile *dst, void* value);
		void*	 CryInterlockedCompareExchangePointer(void* volatile * dst, void* exchange, void* comperand);
		int64  CryInterlockedCompareExchange64( volatile int64 *addr, int64 exchange, int64 comperand );
	#endif
	void*  CryCreateCriticalSection();
  void   CryCreateCriticalSectionInplace(void*);
	void   CryDeleteCriticalSection( void *cs );
  void   CryDeleteCriticalSectionInplace( void *cs );
	void   CryEnterCriticalSection( void *cs );
	bool   CryTryCriticalSection( void *cs );
	void   CryLeaveCriticalSection( void *cs );

#ifdef WIN32
  void*  CryCreateCriticalSectionWithSpinCount(int spinCount);
#endif











































































































































































































































































































































ILINE void CrySpinLock(volatile int *pLock,int checkVal,int setVal)
{ 





#ifdef _CPU_X86
# ifdef __GNUC__
	register int val;
	__asm__ __volatile__ (
		"0:     mov %[checkVal], %%eax\n"
		"       lock cmpxchg %[setVal], (%[pLock])\n"
		"       jnz 0b"
		: "=m" (*pLock)
		: [pLock] "r" (pLock), "m" (*pLock),
		  [checkVal] "m" (checkVal),
		  [setVal] "r" (setVal)
		: "eax", "cc", "memory"
		);
# else //!__GNUC__
	__asm
	{
		mov edx, setVal
		mov ecx, pLock
Spin:
		// Trick from Intel Optimizations guide
#ifdef _CPU_SSE
		pause
#endif 
		mov eax, checkVal
		lock cmpxchg [ecx], edx
		jnz Spin
	}
# endif //!__GNUC__
#else // !_CPU_X86



























































































	// NOTE: The code below will fail on 64bit architectures!
	while(_InterlockedCompareExchange((volatile long*)pLock,setVal,checkVal)!=checkVal) ;

#endif

}

ILINE void CryReleaseSpinLock(volatile int *pLock,int setVal)
{ 
  *pLock = setVal;



}

//////////////////////////////////////////////////////////////////////////
ILINE void CryInterlockedAdd(volatile int *pVal, int iAdd)
{
#ifdef _CPU_X86
# ifdef __GNUC__
	__asm__ __volatile__ (
		"        lock add %[iAdd], (%[pVal])\n"
		: "=m" (*pVal)
		: [pVal] "r" (pVal), "m" (*pVal), [iAdd] "r" (iAdd)
		);
# else
	__asm
	{
		mov edx, pVal
		mov eax, iAdd
		lock add [edx], eax
	}
# endif
#else

































	// NOTE: The code below will fail on 64bit architectures!




#if defined(_WIN64)
  _InterlockedExchangeAdd((volatile long*)pVal,iAdd);
#else
  InterlockedExchangeAdd((volatile long*)pVal,iAdd);
#endif

#endif
}

#endif //__SPU__

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// CryInterlocked*SList Function, these are specialized C-A-S
// functions for single-linked lists which prevent the A-B-A problem there
// there are implemented in the platform specific CryThread_*.h files
// TODO clean up the interlocked function the same was the CryThread_* header are

//TODO somehow get their real size on WIN (without including windows.h...)
//NOTE: The sizes are verifyed at compile-time in the implementation functions, but this is still ugly

#if !defined(_SPU_JOB) || defined(JOB_LIB_COMP)
#if defined(WIN64) || defined(GRINGO)
_MS_ALIGN(16)
#elif defined(WIN32)
_MS_ALIGN(4)


#endif
struct SLockFreeSingleLinkedListEntry
{
	SLockFreeSingleLinkedListEntry *pNext;
}



;

#if defined(WIN64) || defined(GRINGO)
_MS_ALIGN(16)
#elif defined(WIN32)
_MS_ALIGN(8)


#endif
struct SLockFreeSingleLinkedListHeader
{
	SLockFreeSingleLinkedListEntry *pNext;
}



;


// push a element atomically onto a single linked list
void CryInterlockedPushEntrySList( SLockFreeSingleLinkedListHeader& list, SLockFreeSingleLinkedListEntry &element );

// push a element atomically from a single linked list
void* CryInterlockedPopEntrySList(  SLockFreeSingleLinkedListHeader& list );

// initialzied the lock-free single linked list
void CryInitializeSListHead(SLockFreeSingleLinkedListHeader& list);
  
// flush the whole list
void* CryInterlockedFlushSList(SLockFreeSingleLinkedListHeader& list);
#endif


//special define to guard SPU driver compilation
#if !defined(JOB_LIB_COMP) && !defined(_SPU_JOB)

ILINE void CryReadLock(volatile int *rw, bool yield)
{
	CryInterlockedAdd(rw,1);
#ifdef NEED_ENDIAN_SWAP




















	volatile char *pw=(volatile char*)rw+1;







	{
		for(;*pw;);
	}

#else
	volatile char *pw=(volatile char*)rw+2;







	{
		for(;*pw;);
	}
#endif
}

ILINE void CryReleaseReadLock(volatile int* rw)
{
	CryInterlockedAdd(rw,-1);
}

ILINE void CryWriteLock(volatile int* rw)
{
	CrySpinLock(rw,0,WRITE_LOCK_VAL);
}

ILINE void CryReleaseWriteLock(volatile int* rw)
{
	CryInterlockedAdd(rw,-WRITE_LOCK_VAL);
}

//////////////////////////////////////////////////////////////////////////
struct ReadLock
{
	ILINE ReadLock(volatile int &rw)
	{
		CryInterlockedAdd(prw=&rw,1);
#ifdef NEED_ENDIAN_SWAP









		volatile char *pw=(volatile char*)&rw+1; for(;*pw;);

#else
		volatile char *pw=(volatile char*)&rw+2; for(;*pw;);
#endif
	}
	ILINE ReadLock(volatile int &rw, bool yield)
	{
		CryReadLock(prw=&rw, yield);
	}
	~ReadLock()
	{
		CryReleaseReadLock(prw);
	}
private:
	volatile int *prw;
};

struct ReadLockCond
{
	ILINE ReadLockCond(volatile int &rw,int bActive)
	{
		if (bActive)
		{
			CryInterlockedAdd(&rw,1);
			bActivated = 1;
#ifdef NEED_ENDIAN_SWAP









			volatile char *pw=(volatile char*)&rw+1; for(;*pw;);

#else
			volatile char *pw=(volatile char*)&rw+2; for(;*pw;);
#endif
		}
		else
		{
			bActivated = 0;
		}
		prw = &rw; 
	}
	void SetActive(int bActive=1) { bActivated = bActive; }
	void Release() { CryInterlockedAdd(prw,-bActivated); }
	~ReadLockCond()
	{



		CryInterlockedAdd(prw,-bActivated);
	}

private:
	volatile int *prw;
	int bActivated;
};

//////////////////////////////////////////////////////////////////////////
struct WriteLock
{
	ILINE WriteLock(volatile int &rw) { CryWriteLock(&rw); prw=&rw; }
	~WriteLock() { CryReleaseWriteLock(prw); }
private:
	volatile int *prw;
};

//////////////////////////////////////////////////////////////////////////
struct WriteAfterReadLock
{
	ILINE WriteAfterReadLock(volatile int &rw) { CrySpinLock(&rw,1,WRITE_LOCK_VAL+1); prw=&rw; }
	~WriteAfterReadLock() { CryInterlockedAdd(prw,-WRITE_LOCK_VAL); }
private:
	volatile int *prw;
};

//////////////////////////////////////////////////////////////////////////
struct WriteLockCond
{
	ILINE WriteLockCond(volatile int &rw,int bActive=1)
	{
		if (bActive)
			CrySpinLock(&rw,0,iActive=WRITE_LOCK_VAL);
		else 
			iActive = 0;
		prw = &rw; 
	}
	ILINE WriteLockCond() { prw=&(iActive=0); }
	~WriteLockCond() { 



		CryInterlockedAdd(prw,-iActive); 
	}
	void SetActive(int bActive=1) { iActive = -bActive & WRITE_LOCK_VAL; }
	void Release() { CryInterlockedAdd(prw,-iActive); }
	volatile int *prw;
	int iActive;
};

#if !defined(PS3) && !defined(LINUX) && !defined(CAFE)
ILINE int64 CryInterlockedCompareExchange64( volatile int64 *addr, int64 exchange, int64 compare )
{
	return _InterlockedCompareExchange64( (volatile int64*)addr, exchange,compare);
}
#endif


//////////////////////////////////////////////////////////////////////////


































































































































struct Lock_dummy_param{};

struct NoLock {
	NoLock(unsigned int& rw) {}
	NoLock(int& rw, int bActive=1) {}
	NoLock(volatile int& rw, int bActive=1) {}
	NoLock(Lock_dummy_param) {}
  NoLock() { 



  }
	void SetActive(int = 0) {}




};

#if defined(EXCLUDE_PHYSICS_THREAD) 
# if EMBED_PHYSICS_AS_FIBER
   ILINE void SolveContention() { 
#if !defined(__SPU__)
		 JobManager::Fiber::SwitchFiberDirect();
#endif 
	 }
  ILINE void SpinLock(volatile int *pLock,int checkVal,int setVal) { *(int*)pLock=setVal; } 
	ILINE void AtomicAdd(volatile int *pVal, int iAdd) {	*(int*)pVal+=iAdd; }
	ILINE void AtomicAdd(volatile unsigned int *pVal, int iAdd) { *(unsigned int*)pVal+=iAdd; }
  ILINE void JobSpinLock(volatile int *pLock,int checkVal,int setVal) { 
    while(CryInterlockedCompareExchange((long volatile * )pLock,(long)setVal,(long)checkVal)!=checkVal) SolveContention(); 
  } 
# else
	 ILINE void SpinLock(volatile int *pLock,int checkVal,int setVal) { *(int*)pLock=setVal; } 
	 ILINE void AtomicAdd(volatile int *pVal, int iAdd) {	*(int*)pVal+=iAdd; }
	 ILINE void AtomicAdd(volatile unsigned int *pVal, int iAdd) { *(unsigned int*)pVal+=iAdd; }
   ILINE void JobSpinLock(volatile int *pLock,int checkVal,int setVal) { CrySpinLock(pLock,checkVal,setVal); } 
# endif 
#else
	ILINE void SpinLock(volatile int *pLock,int checkVal,int setVal) { CrySpinLock(pLock,checkVal,setVal); } 
	ILINE void AtomicAdd(volatile int *pVal, int iAdd) {	CryInterlockedAdd(pVal,iAdd); }
	ILINE void AtomicAdd(volatile unsigned int *pVal, int iAdd) { CryInterlockedAdd((volatile int*)pVal,iAdd); }

  ILINE void JobSpinLock(volatile int *pLock,int checkVal,int setVal) { SpinLock(pLock,checkVal,setVal); } 
#endif

ILINE void JobAtomicAdd(volatile int *pVal, int iAdd) {	CryInterlockedAdd(pVal,iAdd); }
ILINE void JobAtomicAdd(volatile unsigned int *pVal, int iAdd) { CryInterlockedAdd((volatile int*)pVal,iAdd); }


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Definitions of default locking primitives for all platforms except PS3 
#if !defined(PS3) 
  typedef ReadLock JobReadLock;
  typedef ReadLockCond JobReadLockCond;
  typedef WriteLock JobWriteLock;
  typedef WriteLockCond JobWriteLockCond;
  typedef ReadLock JobReadLockPlatf1;
  typedef WriteLock JobWriteLockPlatf1;

	#define ReadLockPlatf0 ReadLock
	#define WriteLockPlatf0 WriteLock
	#define ReadLockCondPlatf0 ReadLockCond
	#define WriteLockCondPlatf0 WriteLockCond
	#define ReadLockPlatf1 NoLock
	#define WriteLockPlatf1 NoLock
	#define ReadLockCondPlatf1 NoLock
	#define WriteLockCondPlatf1 NoLock
// Definitions of ps3 locking primitives






































































#endif
#endif//JOB_LIB_COMP


#endif // __MultiThread_h__
