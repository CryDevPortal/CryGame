#ifndef BUCKETALLOCATORPOLICY_H
#define BUCKETALLOCATORPOLICY_H

#ifdef WIN32
#include <windows.h>
#endif





#define BUCKET_ALLOCATOR_DEFAULT_MAX_SEGMENTS 8


namespace BucketAllocatorDetail
{

	struct AllocHeader
	{
		AllocHeader* volatile next;

#ifdef BUCKET_ALLOCATOR_TRAP_DOUBLE_DELETES
		uint32 magic;
#endif
	};

	struct SyncPolicyLocked
	{
#if defined(XENON) || defined(_WIN32)
		typedef SLIST_HEADER FreeListHeader;


#endif

#if defined(XENON) || defined(_WIN32)

		union Lock
		{
			volatile int lock;
			uint8 padding[128];
		} BUCKET_ALIGN(128);

		Lock m_cleanupLock;
		Lock m_refillLock;

		struct CleanupReadLock
		{
			ILINE CleanupReadLock(SyncPolicyLocked& policy)
			{
				int iter=0;
				while (policy.m_cleanupLock.lock)
					//Sleep 0 is yield and a potential deadlock if the thread holding the lock is preempted
					Sleep((++iter>100)?1:0);
			}
			~CleanupReadLock() {}

		private:
			CleanupReadLock(const CleanupReadLock&);
			CleanupReadLock& operator = (const CleanupReadLock&);
		};

		struct CleanupWriteLock
		{
			CleanupWriteLock(SyncPolicyLocked& policy) : m_policy(policy) { CrySpinLock(&policy.m_cleanupLock.lock, 0, 1); }
			~CleanupWriteLock() { CryReleaseSpinLock(&m_policy.m_cleanupLock.lock, 0); }

		private:
			SyncPolicyLocked& m_policy;
		};

		struct RefillLock
		{
			RefillLock(SyncPolicyLocked& policy) : m_policy(policy) { CrySpinLock(&m_policy.m_refillLock.lock, 0, 1); }
			~RefillLock() { CryReleaseSpinLock(&m_policy.m_refillLock.lock, 0); }

		private:
			SyncPolicyLocked& m_policy;
		};
		




























































#endif


















































































































#if defined(WIN32) || defined (GRINGO)

		static void PushOnto(FreeListHeader& list, AllocHeader* ptr)
		{
			InterlockedPushEntrySList(&list, reinterpret_cast<PSLIST_ENTRY>(ptr));
		}

		static void PushListOnto(FreeListHeader& list, AllocHeader* head, AllocHeader* tail)
		{
			AllocHeader* item = head;
			AllocHeader* next;

			while (item)
			{
				next = item->next;
				item->next = NULL;

				PushOnto(list, item);

				item = next;
			}
		}

		static AllocHeader* PopOff(FreeListHeader& list)
		{
			return reinterpret_cast<AllocHeader*>(InterlockedPopEntrySList(&list));
		}

		static AllocHeader* PopListOff(FreeListHeader& list)
		{
			return reinterpret_cast<AllocHeader*>(InterlockedFlushSList(&list));
		}

		ILINE static bool IsFreeListEmpty(FreeListHeader& list)
		{
			return QueryDepthSList(&list) == 0;
		}

#endif

	};

	struct SyncPolicyUnlocked
	{
		typedef AllocHeader* FreeListHeader;

		struct CleanupReadLock { CleanupReadLock(SyncPolicyUnlocked&) {} };
		struct CleanupWriteLock { CleanupWriteLock(SyncPolicyUnlocked&) {} };
		struct RefillLock { RefillLock(SyncPolicyUnlocked&) {} };

		ILINE static void PushOnto(FreeListHeader& list, AllocHeader* ptr)
		{
			ptr->next = list;
			list = ptr;
		}

		ILINE static void PushListOnto(FreeListHeader& list, AllocHeader* head, AllocHeader* tail)
		{
			tail->next = list;
			list = head;
		}

		ILINE static AllocHeader* PopOff(FreeListHeader& list)
		{
			AllocHeader* top = list;
			if (top)
				list = *(AllocHeader**)(&top->next); // cast away the volatile
			return top;
		}

		ILINE static AllocHeader* PopListOff(FreeListHeader& list)
		{
			AllocHeader* pRet = list;
			list = NULL;
			return pRet;
		}

		ILINE static bool IsFreeListEmpty(FreeListHeader& list)
		{
			return list == NULL;
		}
	};

	template <size_t Size, typename SyncingPolicy, bool FallbackOnCRT = true, size_t MaxSegments = BUCKET_ALLOCATOR_DEFAULT_MAX_SEGMENTS>
	struct DefaultTraits
	{
		enum
		{
			MaxSize = 512,
			
#ifdef BUCKET_ALLOCATOR_PACK_SMALL_SIZES
			NumBuckets = 32 / 4 + (512 - 32) / 8,
#else
			NumBuckets = 512 / MEMORY_ALLOCATION_ALIGNMENT,
#endif

			PageLength = 64 * 1024,
			SmallBlockLength = 1024,
			SmallBlocksPerPage = 64,

			NumGenerations = 4,
			MaxNumSegments = MaxSegments,

			NumPages = Size / PageLength,

			FallbackOnCRTAllowed = FallbackOnCRT,
		};

		typedef SyncingPolicy SyncPolicy;

		static uint8 GetBucketForSize(size_t sz)
		{
#ifdef BUCKET_ALLOCATOR_PACK_SMALL_SIZES
			if (sz <= 32)
			{
				const int alignment = 4;
				size_t alignedSize = (sz + (alignment - 1)) & ~(alignment - 1);
				return alignedSize / alignment - 1;
			}
			else
			{
				const int alignment = 8;
				size_t alignedSize = (sz + (alignment - 1)) & ~(alignment - 1);
				alignedSize -= 32;
				return alignedSize / alignment + 7;
			}
#else
			const int alignment = MEMORY_ALLOCATION_ALIGNMENT;
			size_t alignedSize = (sz + (alignment - 1)) & ~(alignment - 1);
			return alignedSize / alignment - 1;
#endif
		}

		static size_t GetSizeForBucket(uint8 bucket)
		{
			size_t sz;

#ifdef BUCKET_ALLOCATOR_PACK_SMALL_SIZES
			if (bucket <= 7)
				sz = (bucket + 1) * 4;
			else
				sz = (bucket - 7) * 8 + 32;
#else
			sz = (bucket + 1) * MEMORY_ALLOCATION_ALIGNMENT;
#endif

#ifdef BUCKET_ALLOCATOR_TRAP_DOUBLE_DELETES
			return sz < sizeof(AllocHeader) ? sizeof(AllocHeader) : sz;
#else
			return sz;
#endif
		}

		static size_t GetGenerationForStability(uint8 stability)
		{
			return 3 - stability / 64;
		}
	};
}

#endif
