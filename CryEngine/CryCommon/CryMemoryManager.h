////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   CryMemoryManager.h
//  Version:     v1.00
//  Created:     27/7/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: Defines functions for CryEngine custom memory manager.
//               See also CryMemoryManager_impl.h, it must be included only once per module.
//               CryMemoryManager_impl.h is included by platform_impl.h
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
 
#ifndef __CryMemoryManager_h__
#define __CryMemoryManager_h__
#pragma once

//////////////////////////////////////////////////////////////////////////
// Define this if you want to use slow debug memory manager in any config.
//////////////////////////////////////////////////////////////////////////
//#define DEBUG_MEMORY_MANAGER
//////////////////////////////////////////////////////////////////////////

// That mean we use node_allocator for all small allocations

#include "platform.h"

#include <stdarg.h>

#ifndef STLALLOCATOR_CLEANUP
#define STLALLOCATOR_CLEANUP
#endif

#define _CRY_DEFAULT_MALLOC_ALIGNMENT 4

#if !defined(PS3) && !defined(CAFE)
	#include <malloc.h>
#endif

#if defined(__cplusplus)
#if defined(PS3) || defined(LINUX) || defined(CAFE)
	#include <new>
#else
	#include <new.h>
#endif
// required for the inclusion of annotated CryXXX Memory Methods
#if defined(__CRYCG__)
  #include "PS3CryCache.h"
#endif
#endif

	#ifdef CRYSYSTEM_EXPORTS
		#define CRYMEMORYMANAGER_API DLL_EXPORT
	#else
		#define CRYMEMORYMANAGER_API DLL_IMPORT
	#endif

#ifdef __cplusplus
//////////////////////////////////////////////////////////////////////////
#ifdef DEBUG_MEMORY_MANAGER
	#ifdef _DEBUG
		#define _DEBUG_MODE
	#endif
#endif //DEBUG_MEMORY_MANAGER

#if defined(_DEBUG) && !defined(PS3) && !defined(LINUX) && !defined(CAFE)
	#include <crtdbg.h>
#endif //_DEBUG

// checks if the heap is valid in debug; in release, this function shouldn't be called
// returns non-0 if it's valid and 0 if not valid
ILINE int IsHeapValid()
{
#if (defined(_DEBUG) && !defined(RELEASE_RUNTIME) && !defined(PS3) && !defined(CAFE)) || (defined(DEBUG_MEMORY_MANAGER))
	return _CrtCheckMemory();
#else
	return true;
#endif
}

#ifdef DEBUG_MEMORY_MANAGER
// Restore debug mode define
	#ifndef _DEBUG_MODE
		#undef _DEBUG
	#endif //_DEBUG_MODE
#endif //DEBUG_MEMORY_MANAGER
//////////////////////////////////////////////////////////////////////////

#endif //__cplusplus

struct ICustomMemoryHeap;
class IGeneralMemoryHeap;
class IPageMappingHeap;

#if (defined(_LIB) && (defined(XENON) || defined(GRINGO))) || defined(PS3)
#define MEMMAN_STATIC
#endif

#ifdef MEMMAN_STATIC
#define MEMMAN_METHOD(...) static __VA_ARGS__
#else
#define MEMMAN_METHOD(...) virtual __VA_ARGS__ = 0
#endif

// Description:
//	 Interfaces that allow access to the CryEngine memory manager.
struct IMemoryManager
{
	typedef unsigned char HeapHandle;
	enum { BAD_HEAP_HANDLE = 0xFF };

	struct SProcessMemInfo
	{
		uint64 PageFaultCount;
		uint64 PeakWorkingSetSize;
		uint64 WorkingSetSize;
		uint64 QuotaPeakPagedPoolUsage;
		uint64 QuotaPagedPoolUsage;
		uint64 QuotaPeakNonPagedPoolUsage;
		uint64 QuotaNonPagedPoolUsage;
		uint64 PagefileUsage;
		uint64 PeakPagefileUsage;

		uint64 TotalPhysicalMemory;
		int64  FreePhysicalMemory;

		uint64 TotalVideoMemory;
		int64 FreeVideoMemory;
	};

#ifdef MEMMAN_STATIC
	static IMemoryManager* GetInstance();
#else
	virtual ~IMemoryManager(){}
#endif

	MEMMAN_METHOD( bool GetProcessMemInfo( SProcessMemInfo &minfo ) );

	// Description:
	//	 Used to add memory block size allocated directly from the crt or OS to the memory manager statistics.
	MEMMAN_METHOD( void FakeAllocation( long size ) );

	// Initialise the level heap.
	MEMMAN_METHOD( void InitialiseLevelHeap() );

	// Switch the default heap to the level heap.
	MEMMAN_METHOD( void SwitchToLevelHeap() );

	// Switch the default heap to the global heap.
	MEMMAN_METHOD( void SwitchToGlobalHeap() );

	// Enable the global heap for this thread only. Returns previous heap selection, which must be passed to LocalSwitchToHeap.
	MEMMAN_METHOD( int LocalSwitchToGlobalHeap() );

	// Enable the level heap for this thread only. Returns previous heap selection, which must be passed to LocalSwitchToHeap.
	MEMMAN_METHOD( int LocalSwitchToLevelHeap() );

	// Switch to a specific heap for this thread only. Usually used to undo a previous LocalSwitchToGlobalHeap
	MEMMAN_METHOD( void LocalSwitchToHeap(int heap) );

	// Fetch the violation status of the level heap
	MEMMAN_METHOD( bool GetLevelHeapViolationState(bool& usingLevelHeap, size_t& numAllocs, size_t& allocSize) );

	//////////////////////////////////////////////////////////////////////////
	// Heap Tracing API
	MEMMAN_METHOD( HeapHandle TraceDefineHeap( const char *heapName,size_t size,const void* pBase ) );
	MEMMAN_METHOD( void TraceHeapAlloc( HeapHandle heap,void* mem, size_t size, size_t blockSize, const char *sUsage,const char *sNameHint=0 ) );
	MEMMAN_METHOD( void TraceHeapFree( HeapHandle heap,void* mem, size_t blockSize ) );
	MEMMAN_METHOD( void TraceHeapSetColor( uint32 color ) );
	MEMMAN_METHOD( uint32 TraceHeapGetColor() );
	MEMMAN_METHOD( void TraceHeapSetLabel( const char *sLabel ) );
	//////////////////////////////////////////////////////////////////////////

	// Retrieve access to the MemReplay implementation class.
	MEMMAN_METHOD( struct IMemReplay* GetIMemReplay() );

	// Create an instance of ICustomMemoryHeap
	MEMMAN_METHOD( ICustomMemoryHeap* const CreateCustomMemoryHeapInstance(bool const bCanUseGPUMemory) );

	MEMMAN_METHOD( IGeneralMemoryHeap* CreateGeneralExpandingMemoryHeap(size_t upperLimit, size_t reserveSize, const char* sUsage) );
	MEMMAN_METHOD( IGeneralMemoryHeap* CreateGeneralMemoryHeap(void* base, size_t sz,const char *sUsage) );

	MEMMAN_METHOD( IPageMappingHeap* CreatePageMappingHeap(size_t addressSpace, const char* sName) );
};

#undef MEMMAN_METHOD

// Global function implemented in CryMemoryManager_impl.h
#ifdef MEMMAN_STATIC
inline IMemoryManager* CryGetIMemoryManager()
{
	extern IMemoryManager g_memoryManager;
	return &g_memoryManager;
}
#else
IMemoryManager *CryGetIMemoryManager();
#endif

class STraceHeapAllocatorAutoColor
{
public:
	explicit STraceHeapAllocatorAutoColor( uint32 color ) { m_color = CryGetIMemoryManager()->TraceHeapGetColor(); CryGetIMemoryManager()->TraceHeapSetColor(color); }
	~STraceHeapAllocatorAutoColor() { CryGetIMemoryManager()->TraceHeapSetColor(m_color); };
protected:
	uint32 m_color;
	STraceHeapAllocatorAutoColor() {};
};

#define TRACEHEAP_AUTOCOLOR(color) STraceHeapAllocatorAutoColor _auto_color_(color);


// Summary:
//		Structure filled by call to CryModuleGetMemoryInfo().
struct CryModuleMemoryInfo
{
	uint64 requested;
	// Total amount of memory allocated.
	uint64 allocated;
	// Total amount of memory freed.
	uint64 freed;
	// Total number of memory allocations.
	int num_allocations;
	// Allocated in CryString.
	uint64 CryString_allocated;
	// Allocated in STL.
	uint64 STL_allocated;
	// Amount of memory wasted in pools in stl (not usefull allocations).
	uint64 STL_wasted;
};

struct CryReplayInfo
{
	uint64 uncompressedLength;
	uint64 writtenLength;
	uint32 trackingSize;
	const char* filename;
};

//////////////////////////////////////////////////////////////////////////
// Extern declarations of globals inside CrySystem.
//////////////////////////////////////////////////////////////////////////
#if !defined(__SPU__)

#ifdef __cplusplus 
extern "C" {
#endif //__cplusplus


CRYMEMORYMANAGER_API void *CryMalloc(size_t size, size_t& allocated, size_t alignment);
CRYMEMORYMANAGER_API void *CryRealloc(void *memblock,size_t size, size_t& allocated,size_t& oldsize, size_t alignment);
CRYMEMORYMANAGER_API size_t CryFree(void *p, size_t alignment);
CRYMEMORYMANAGER_API size_t CryGetMemSize(void *p, size_t size);
CRYMEMORYMANAGER_API int  CryStats(char *buf);
CRYMEMORYMANAGER_API void CryFlushAll();
CRYMEMORYMANAGER_API void CryCleanup();
CRYMEMORYMANAGER_API int  CryGetUsedHeapSize();
CRYMEMORYMANAGER_API int  CryGetWastedHeapSize();
CRYMEMORYMANAGER_API void *CrySystemCrtMalloc(size_t size);
CRYMEMORYMANAGER_API void *CrySystemCrtRealloc(void * p, size_t size);
CRYMEMORYMANAGER_API size_t CrySystemCrtFree(void *p);
CRYMEMORYMANAGER_API size_t CrySystemCrtSize(void *p);
CRYMEMORYMANAGER_API void CryGetIMemoryManagerInterface( void **pIMemoryManager );

// This function is local in every module
/*CRYMEMORYMANAGER_API*/ void CryGetMemoryInfoForModule(CryModuleMemoryInfo * pInfo);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //!defined(__SPU__)

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Cry Memory Manager accessible in all build modes.
//////////////////////////////////////////////////////////////////////////
#if !defined(USING_CRY_MEMORY_MANAGER)
	#define USING_CRY_MEMORY_MANAGER
#endif

#ifndef _LIB
#define CRY_MEM_USAGE_API extern "C" DLL_EXPORT
#else //_USRDLL
#define CRY_MEM_USAGE_API
#endif //_USRDLL

#include "CryMemReplay.h"

#if CAPTURE_REPLAY_LOG
#define CRYMM_INLINE inline
#else
#define CRYMM_INLINE ILINE
#endif

#if defined(NOT_USE_CRY_MEMORY_MANAGER)
	#if !defined(__SPU__)
		CRYMM_INLINE void* CryModuleMalloc(size_t size)
		{
			MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CryMalloc);
			void* ptr = malloc(size);
			MEMREPLAY_SCOPE_ALLOC(ptr, size, 0);
			return ptr;
		}
		
		CRYMM_INLINE void* CryModuleRealloc(void *memblock, size_t size)
		{
			MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CryMalloc);
			void* ret = realloc(memblock,size);
			MEMREPLAY_SCOPE_REALLOC(memblock, ret, size, 0);
			return ret;
		}
		
		CRYMM_INLINE void  CryModuleFree(void *memblock)
		{
			MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CryMalloc);
			free(memblock);
			MEMREPLAY_SCOPE_FREE(memblock);
		}
		
		CRYMM_INLINE void  CryModuleMemalignFree( void *memblock )
		{
			MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CryMalloc);
			free(memblock);
			MEMREPLAY_SCOPE_FREE(memblock);
		}
		

















			CRYMM_INLINE void* CryModuleReallocAlign( void *memblock, size_t size,size_t alignment )
			{
				MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CryMalloc);
				void* ret = realloc(memblock,size);
				MEMREPLAY_SCOPE_REALLOC(memblock, ret, size, alignment);
				return ret;
			}
			
			CRYMM_INLINE void* CryModuleMemalign( size_t size,size_t alignment )
			{
				MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CryMalloc);
				void* ret = malloc(size);
				MEMREPLAY_SCOPE_ALLOC(ret, size, alignment);
				return ret;
			}











	#endif //__SPU__
#else //NOT_USE_CRY_MEMORY_MANAGER

/////////////////////////////////////////////////////////////////////////
// Extern declarations,used by overriden new and delete operators.
//////////////////////////////////////////////////////////////////////////
extern "C"
{
	void* CryModuleMalloc(size_t size) throw();
	void* CryModuleRealloc(void *memblock,size_t size) throw();
	void  CryModuleFree(void *ptr) throw();
	void* CryModuleMemalign(size_t size, size_t alignment) throw() ;
	void* CryModuleReallocAlign(void *memblock, size_t size,size_t alignment) throw();
	void  CryModuleMemalignFree(void*) throw() ;
	void* CryModuleCalloc(size_t a, size_t b) throw();
}

CRYMM_INLINE void* CryModuleCRTMalloc(size_t s)
{
	MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CrtMalloc);
	void* ret = CryModuleMalloc(s);
	MEMREPLAY_SCOPE_ALLOC(ret, s, 0);
	return ret;
}

CRYMM_INLINE void* CryModuleCRTRealloc(void* p, size_t s)
{
	MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CrtMalloc);
	void* ret = CryModuleRealloc(p, s);
	MEMREPLAY_SCOPE_REALLOC(p, ret, s, 0);
	return ret;
}

CRYMM_INLINE void CryModuleCRTFree(void* p)
{
	MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CrtMalloc);
	CryModuleFree(p);
	MEMREPLAY_SCOPE_FREE(p);
}

#define malloc   CryModuleCRTMalloc
#define realloc  CryModuleCRTRealloc
#define free     CryModuleCRTFree
#define calloc	 CryModuleCalloc

#endif //NOT_USE_CRY_MEMORY_MANAGER

#if !defined(__SPU__)
CRY_MEM_USAGE_API void CryModuleGetMemoryInfo( CryModuleMemoryInfo *pMemInfo );
#endif

#if !defined(NOT_USE_CRY_MEMORY_MANAGER)
	#if defined(_LIB) && !defined(NEW_OVERRIDEN)






























































































//#error hi2

					#if !defined(XENON) && !defined(CAFE)
						//ILINE void * __cdecl operator new   (size_t size) throw () { return CryModuleMalloc(size, eCryModule); }
					#endif // !defined(XENON)

					CRYMM_INLINE void * __cdecl operator new   (size_t size)
					{
						MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CrtNew);
						void* ret = CryModuleMalloc(size);
						MEMREPLAY_SCOPE_ALLOC(ret, size, 0);
						return ret;
					}

					#if !defined(XENON) && !defined(CAFE)
						//ILINE void * __cdecl operator new[] (size_t size) throw () { return CryModuleMalloc(size, eCryModule); }
					#endif // !defined(XENON)

					CRYMM_INLINE void * __cdecl operator new[] (size_t size)
					{
						MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CrtNewArray);
						void* ret = CryModuleMalloc(size);
						MEMREPLAY_SCOPE_ALLOC(ret, size, 0);
						return ret;
					}
					
					CRYMM_INLINE void __cdecl operator delete  (void *p) throw ()
					{
						MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CrtNew);
						CryModuleFree(p);
						MEMREPLAY_SCOPE_FREE(p);
					}
					
					CRYMM_INLINE void __cdecl operator delete[](void *p) throw ()
					{
						MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CrtNewArray);
						CryModuleFree(p);
						MEMREPLAY_SCOPE_FREE(p);
					}


	#endif // defined(_LIB) && !defined(NEW_OVERRIDEN)
#endif // !defined(NOT_USE_CRY_MEMORY_MANAGER)

// Need for our allocator to avoid deadlock in cleanup
void*  CryCrtMalloc(size_t size);
size_t CryCrtFree(void *p);
// wrapper for _msize on PC
size_t CryCrtSize(void * p);


#if !defined( NOT_USE_CRY_MEMORY_MANAGER) && !defined(JOB_LIB_COMP) // && !defined(_STLP_BEGIN_NAMESPACE) // Avoid non STLport version
#include "CryMemoryAllocator.h"




#endif

//////////////////////////////////////////////////////////////////////////
class ScopedSwitchToGlobalHeap
{
#if USE_LEVEL_HEAP
public:
	ILINE ScopedSwitchToGlobalHeap()
		: m_old(CryGetIMemoryManager()->LocalSwitchToGlobalHeap())
	{
	}

	ILINE ~ScopedSwitchToGlobalHeap()
	{
		CryGetIMemoryManager()->LocalSwitchToHeap(m_old);
	}

private:
	ScopedSwitchToGlobalHeap(const ScopedSwitchToGlobalHeap&);
	ScopedSwitchToGlobalHeap& operator = (const ScopedSwitchToGlobalHeap&);

private:
	int m_old;
#else
public:
	ILINE ScopedSwitchToGlobalHeap() {}
#endif
};

class CondScopedSwitchToGlobalHeap
{
#if USE_LEVEL_HEAP
public:
	ILINE CondScopedSwitchToGlobalHeap(bool cond)
	: m_old(0)
	, m_cond(cond)
	{
		IF (cond, 0)
			m_old = CryGetIMemoryManager()->LocalSwitchToGlobalHeap();
	}

	ILINE ~CondScopedSwitchToGlobalHeap()
	{
		IF (m_cond, 0)
			CryGetIMemoryManager()->LocalSwitchToHeap(m_old);
	}

private:
	CondScopedSwitchToGlobalHeap(const CondScopedSwitchToGlobalHeap&);
	CondScopedSwitchToGlobalHeap& operator = (const CondScopedSwitchToGlobalHeap&);

private:
	int m_old;
	bool m_cond;
#else
public:
	ILINE CondScopedSwitchToGlobalHeap(bool) {}
#endif
};

class ScopedSwitchToLevelHeap
{
#if USE_LEVEL_HEAP
public:
	ILINE ScopedSwitchToLevelHeap()
		: m_old(CryGetIMemoryManager()->LocalSwitchToLevelHeap())
	{
	}

	ILINE ~ScopedSwitchToLevelHeap()
	{
		CryGetIMemoryManager()->LocalSwitchToHeap(m_old);
	}

private:
	ScopedSwitchToLevelHeap(const ScopedSwitchToLevelHeap&);
	ScopedSwitchToLevelHeap& operator = (const ScopedSwitchToLevelHeap&);

private:
	int m_old;
#else
public:
	ILINE ScopedSwitchToLevelHeap() {}
#endif
};























#endif // __CryMemoryManager_h__
