////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek, 2008.
// -------------------------------------------------------------------------
//  File name:   MultiThread_Containers.h
//  Version:     v1.00
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __MultiThread_Containters_h__
#define __MultiThread_Containters_h__
#pragma once

#include "Pipe.h"
#include "StlUtils.h"

#include <queue>
#include <set>
#include <algorithm>

namespace CryMT
{
	//////////////////////////////////////////////////////////////////////////
	// Thread Safe wrappers on the standard STL containers.
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Multi-Thread safe queue container, can be used instead of std::vector.
	//////////////////////////////////////////////////////////////////////////
	template <class T, class Alloc = std::allocator<T> >
	class queue
	{
	public:		
		typedef	T	value_type;
		typedef	std::vector<T, Alloc>	container_type;
		typedef	CryAutoCriticalSection AutoLock;

		//////////////////////////////////////////////////////////////////////////
		// std::queue interface
		//////////////////////////////////////////////////////////////////////////
		const T& front() const		{ AutoLock lock(m_cs); return v.front(); };
		const T& back() const { AutoLock lock(m_cs); 	return v.back(); }
		void	push(const T& x)	{ AutoLock lock(m_cs); return v.push_back(x); };
		void reserve(const size_t n) { AutoLock lock(m_cs); v.reserve(n); };
		// classic pop function of queue should not be used for thread safety, use try_pop instead
		//void	pop()							{ AutoLock lock(m_cs); return v.erase(v.begin()); };

		CryCriticalSection& get_lock() const { return m_cs; }

		bool   empty() const { AutoLock lock(m_cs); return v.empty(); }
		int    size() const  { AutoLock lock(m_cs); return v.size(); }
		void   clear() { AutoLock lock(m_cs); v.clear(); }
		void   free_memory() { AutoLock lock(m_cs); stl::free_container(v); }

		template <class Func>
		void   sort( const Func &compare_less ) { AutoLock lock(m_cs); std::sort( v.begin(),v.end(),compare_less ); }

		//////////////////////////////////////////////////////////////////////////
		bool try_pop( T& returnValue )
		{
			AutoLock lock(m_cs); 
			if (!v.empty())
			{
				returnValue = v.front();
				v.erase(v.begin());
				return true;
			}
			return false;
		};

		//////////////////////////////////////////////////////////////////////////
		bool try_remove( const T& value )
		{
			AutoLock lock(m_cs);
			if(!v.empty())
			{
				typename container_type::iterator it = std::find(v.begin(), v.end(), value);
				if(it != v.end())
				{
					v.erase(it);
					return true;
				}
			}
			return false;
		};

		template<typename Sizer>
		void GetMemoryUsage(Sizer *pSizer) const
		{
			pSizer->AddObject(v);
		}
	private:
		container_type v;
		mutable	CryCriticalSection m_cs;
	};

	//////////////////////////////////////////////////////////////////////////
	// Multi-Thread safe vector container, can be used instead of std::vector.
	//////////////////////////////////////////////////////////////////////////
	template <class T>
	class vector
	{
	public:
		typedef	T	value_type;
		typedef	CryAutoCriticalSection AutoLock;

		CryCriticalSection& get_lock() const { return m_cs; }

		void free_memory() { AutoLock lock(m_cs); stl::free_container(v); }

		//////////////////////////////////////////////////////////////////////////
		// std::vector interface
		//////////////////////////////////////////////////////////////////////////
		bool   empty() const { AutoLock lock(m_cs); return v.empty(); }
		int    size() const  { AutoLock lock(m_cs); return v.size(); }
		void   resize( int sz ) { AutoLock lock(m_cs); v.resize( sz ); }
		void   reserve( int sz ) { AutoLock lock(m_cs); v.reserve( sz ); }
		size_t capacity() const { AutoLock lock(m_cs); return v.size(); }
		void   clear() { AutoLock lock(m_cs); v.clear(); }
		T&	   operator[]( size_t pos ) { AutoLock lock(m_cs); return v[pos]; }
		const T& operator[]( size_t pos ) const { AutoLock lock(m_cs); return v[pos]; }
		const T& front() const { AutoLock lock(m_cs); return v.front(); }
		const T& back() const { AutoLock lock(m_cs); 	return v.back(); }
		T& back() { AutoLock lock(m_cs); 	return v.back(); }

		void push_back( const T& x ) { AutoLock lock(m_cs); return v.push_back(x); }
		void pop_back() { AutoLock lock(m_cs); 	return v.pop_back(); }
		//////////////////////////////////////////////////////////////////////////

		template <class Func>
		void sort( const Func &compare_less ) { AutoLock lock(m_cs); std::sort( v.begin(),v.end(),compare_less ); }

		template <class Iter>
		void append( const Iter &startRange,const Iter &endRange ) { AutoLock lock(m_cs); v.insert( v.end(), startRange,endRange ); }

		void swap( std::vector<T> &vec ) { AutoLock lock(m_cs); v.swap(vec); }

		//////////////////////////////////////////////////////////////////////////
		bool try_pop_front( T& returnValue )
		{
			AutoLock lock(m_cs); 
			if (!v.empty())
			{
				returnValue = v.front();
				v.erase(v.begin());
				return true;
			}
			return false;
		};
		bool try_pop_back( T& returnValue )
		{
			AutoLock lock(m_cs); 
			if (!v.empty())
			{
				returnValue = v.back();
				v.pop_back();
				return true;
			}
			return false;
		};

		//////////////////////////////////////////////////////////////////////////
		template <typename FindFunction,typename KeyType>
		bool find_and_copy( FindFunction findFunc,const KeyType &key,T &foundValue ) const
		{
			AutoLock lock(m_cs);
			if(!v.empty())
			{
				typename std::vector<T>::const_iterator it;
				for (it = v.begin(); it != v.end(); ++it)
				{
					if (findFunc(key,*it))
					{
						foundValue = *it;
						return true;
					}
				}
			}
			return false;
		}

		//////////////////////////////////////////////////////////////////////////
		bool try_remove( const T& value )
		{
			AutoLock lock(m_cs);
			if(!v.empty())
			{
				typename std::vector<T>::iterator it = std::find(v.begin(), v.end(), value);
				if(it != v.end())
				{
					v.erase(it);
					return true;
				}
			}
			return false;
		};

	vector() {}

	vector( const vector<T> &rOther )
	{		
		AutoLock lock1(m_cs); 
		AutoLock lock2(rOther.m_cs); 

		v = rOther.v;
	}

	vector& operator=( const vector<T> &rOther )
	{		
		if( this == &rOther )
			return *this;

		AutoLock lock1(m_cs); 
		AutoLock lock2(rOther.m_cs); 

		v = rOther.v;

		return *this;
	}
	private:
		std::vector<T> v;
		mutable	CryCriticalSection m_cs;
	};


	//////////////////////////////////////////////////////////////////////////
	// Multi-Thread safe set container, can be used instead of std::set.
	// It has limited functionality, but most of it is there.
	//////////////////////////////////////////////////////////////////////////
	template <class T>
	class set
	{
	public:
		typedef	T	value_type;
		typedef T Key;
		typedef typename std::set<T>::size_type	size_type;
		typedef	CryAutoCriticalSection	AutoLock;

		//////////////////////////////////////////////////////////////////////////
		// Methods
		//////////////////////////////////////////////////////////////////////////
		void								clear()																			{ AutoLock lock(m_cs); s.clear(); }
		size_type						count(const Key& _Key) const								{ AutoLock lock(m_cs); return s.count(_Key); }
		bool								empty() const																{ AutoLock lock(m_cs); return s.empty(); }
		size_type						erase(const Key& _Key)											{ AutoLock lock(m_cs); return s.erase(_Key); }

		bool								find(const Key& _Key)												{ AutoLock lock(m_cs); return (s.find(_Key)!=s.end()); }

		bool								pop_front(value_type&	rFrontElement)				{AutoLock lock(m_cs);if (s.empty()){return false;}rFrontElement=*s.begin();s.erase(s.begin());return true;}
		bool								pop_front()																	{AutoLock lock(m_cs);if (s.empty()){return false;}s.erase(s.begin());return true;}

		bool								front(value_type&	rFrontElement)						{AutoLock lock(m_cs);if (s.empty()){return false;}rFrontElement=*s.begin();return true;}

		bool								insert(const value_type& _Val)							{ AutoLock lock(m_cs); return s.insert(_Val).second; }
		size_type						max_size() const														{ AutoLock lock(m_cs); return s.max_size(); }
		size_type						size() const																{ AutoLock lock(m_cs); return s.size(); }
		void								swap(set& _Right)														{ AutoLock lock(m_cs); s.swap(_Right); }
	private:
		std::set<value_type>				s;
		mutable	CryCriticalSection	m_cs;
	};

	//////////////////////////////////////////////////////////////////////////
	// Producer/Consumer Queue for 1 to 1 thread communication
	// Realized with only volatile variables and memory barriers
	// *warning* this producer/consumer queue is only thread safe in a 1 to 1 situation
	// and doesn't provide any yields or similar to prevent spinning
	//////////////////////////////////////////////////////////////////////////
	template<typename T>
	class SingleProducerSingleConsumerQueue
	{
	public:
		SingleProducerSingleConsumerQueue() :
			m_arrBuffer(NULL), m_nBufferSize(NULL), m_nProducerIndex(0), m_nComsumerIndex(0) {}

		SingleProducerSingleConsumerQueue( T *arrBuffer, uint32 nBufferSize ) :
			m_arrBuffer(arrBuffer), m_nBufferSize(nBufferSize), m_nProducerIndex(0), m_nComsumerIndex(0) 
		{
			assert(IsPowerOfTwo(nBufferSize));
		}
		
		void SetBuffer( T *arrBuffer, uint32 nBufferSize )
		{
			assert(IsPowerOfTwo(nBufferSize));
			m_arrBuffer = arrBuffer;
			m_nBufferSize = nBufferSize;			
		}

		void Push( const T &rObj );
		void Pop( T *pResult );
	private:	
		void Regular_Push( const T &rObj );
		void Regular_Pop( T *pResult );
	
		void SPU_Push( const T &rObj );
		void SPU_Pop( T *pResult );
	
		T *m_arrBuffer;
		uint32 m_nBufferSize;
	
		volatile uint32 m_nProducerIndex _ALIGN(16);
		volatile uint32 m_nComsumerIndex _ALIGN(16);
	} _ALIGN(128);

	///////////////////////////////////////////////////////////////////////////////
	template<typename T>
	inline void SingleProducerSingleConsumerQueue<T>::Push( const T &rObj )
	{
		assert(m_arrBuffer != NULL);
		assert(m_nBufferSize != 0);
		



		Regular_Push(rObj);

	}

	///////////////////////////////////////////////////////////////////////////////
	template<typename T>
	inline void SingleProducerSingleConsumerQueue<T>::Regular_Push( const T &rObj )
	{
		// busy-loop if queue is full
		int iter = 0;
		while(m_nProducerIndex - m_nComsumerIndex == m_nBufferSize) { Sleep(iter++ > 10 ? 1 : 0); }

		m_arrBuffer[m_nProducerIndex % m_nBufferSize] = rObj;
		MemoryBarrier();
		m_nProducerIndex += 1;
		MemoryBarrier();
	}

	///////////////////////////////////////////////////////////////////////////////
	template<typename T>
	inline void SingleProducerSingleConsumerQueue<T>::SPU_Push( const T &rObj )
	{









































	}

	///////////////////////////////////////////////////////////////////////////////
	template<typename T>
	inline void SingleProducerSingleConsumerQueue<T>::Pop( T *pResult )
	{
		assert(m_arrBuffer != NULL);
		assert(m_nBufferSize != 0);




		return Regular_Pop(pResult);

	}

	///////////////////////////////////////////////////////////////////////////////
	template<typename T>
	inline void SingleProducerSingleConsumerQueue<T>::Regular_Pop( T *pResult )
	{
		// busy-loop if queue is empty
		int iter = 0;
		while(m_nProducerIndex - m_nComsumerIndex == 0) { Sleep(iter++ > 10 ? 1 : 0); }		

		*pResult = m_arrBuffer[m_nComsumerIndex % m_nBufferSize];
		MemoryBarrier();
		m_nComsumerIndex += 1;
		MemoryBarrier();
	}
	

	///////////////////////////////////////////////////////////////////////////////
	template<typename T>
	inline void SingleProducerSingleConsumerQueue<T>::SPU_Pop( T *pResult )
	{










































	}

	//////////////////////////////////////////////////////////////////////////
	// Producer/Consumer Queue for N to 1 thread communication
	// lockfree implemenation, to copy with multiple producers, 
	// a internal producer refcount is managed, the queue is empty
	// as soon as there are no more producers and no new elements
	//////////////////////////////////////////////////////////////////////////
	template<typename T>
	class N_ProducerSingleConsumerQueue
	{
	public:
		N_ProducerSingleConsumerQueue() :
			m_arrBuffer(NULL), m_arrStates(NULL), m_nBufferSize(0), m_nProducerIndex(0), m_nComsumerIndex(0), m_nRunning(0), m_nProducerCount(0) 
		{}

		N_ProducerSingleConsumerQueue( T *arrBuffer, volatile uint32 *arrStateBuffer, uint32 nBufferSize ) :
			m_arrBuffer(arrBuffer), m_arrStates(arrStateBuffer), m_nBufferSize(nBufferSize), m_nProducerIndex(0), m_nComsumerIndex(0), m_nRunning(0), m_nProducerCount(0) 
		{
			assert(IsPowerOfTwo(nBufferSize));
		}
		
		void Push( const T &rObj );
		bool Pop( T *pResult );
		
		// needs to be called before using, assumes that there is at least one producer
		// so the first one doesn't need to call AddProducer, but he has to deregister itself
		void SetRunningState()
		{
			memset((void*)m_arrStates, 0, sizeof(uint32)*m_nBufferSize);
			m_nRunning = 1;
			m_nProducerCount = 1;
		}
		
		void SetBuffer( T *arrBuffer, volatile uint32 *arrStateBuffer, uint32 nBufferSize )
		{
			assert(IsPowerOfTwo(nBufferSize));
			m_arrBuffer = arrBuffer;
			m_arrStates = arrStateBuffer;
			m_nBufferSize = nBufferSize;			
		}

		void AddProducer();
		void RemoveProducer();
	private:	
		void Regular_Push( const T &rObj );
		bool Regular_Pop( T *pResult );
	
		void SPU_Push( const T &rObj );
		bool SPU_Pop( T *pResult );
	
		T *m_arrBuffer;
		volatile uint32 *m_arrStates;
		uint32 m_nBufferSize;
	
		volatile uint32 m_nProducerIndex;
		volatile uint32 m_nComsumerIndex;
		volatile uint32 m_nRunning;
		volatile uint32 m_nProducerCount;
	} _ALIGN(128);

	///////////////////////////////////////////////////////////////////////////////
	template<typename T>
	inline void N_ProducerSingleConsumerQueue<T>::AddProducer()
	{
		assert( m_arrBuffer != NULL );
		assert( m_arrStates != NULL );
		assert( m_nBufferSize != 0 );












		CryInterlockedIncrement((volatile int*)&m_nProducerCount);

	}
	
	///////////////////////////////////////////////////////////////////////////////
	template<typename T>
	inline void N_ProducerSingleConsumerQueue<T>::RemoveProducer( )
	{
		assert( m_arrBuffer != NULL );
		assert( m_arrStates != NULL );
		assert( m_nBufferSize != 0 );















		if( CryInterlockedDecrement((volatile int*)&m_nProducerCount) == 0 )
			m_nRunning = 0;

	}

	///////////////////////////////////////////////////////////////////////////////
	template<typename T>
	inline void N_ProducerSingleConsumerQueue<T>::Push( const T &rObj )
	{
		assert( m_arrBuffer != NULL );
		assert( m_arrStates != NULL );
		assert( m_nBufferSize != 0 );




		Regular_Push(rObj);

	}

	///////////////////////////////////////////////////////////////////////////////
	template<typename T>
	inline void N_ProducerSingleConsumerQueue<T>::Regular_Push( const T &rObj )
	{
		uint32 nProducerIndex;
		uint32 nComsumerIndex;
		
		do
		{
			nProducerIndex = m_nProducerIndex;
			nComsumerIndex = m_nComsumerIndex;
			
			if( nProducerIndex - nComsumerIndex == m_nBufferSize)
				continue;
				
			if(CryInterlockedCompareExchange( alias_cast<volatile long*>(&m_nProducerIndex), nProducerIndex + 1, nProducerIndex ) == nProducerIndex )
				break;
		}while(true);		

		m_arrBuffer[nProducerIndex % m_nBufferSize] = rObj;
		MemoryBarrier();
		m_arrStates[nProducerIndex % m_nBufferSize] = 1;
		MemoryBarrier();
	}

	///////////////////////////////////////////////////////////////////////////////
	template<typename T>
	inline void N_ProducerSingleConsumerQueue<T>::SPU_Push( const T &rObj )
	{


















































	}

	///////////////////////////////////////////////////////////////////////////////
	template<typename T>
	inline bool N_ProducerSingleConsumerQueue<T>::Pop( T *pResult )
	{
		assert( m_arrBuffer != NULL );
		assert( m_arrStates != NULL );
		assert( m_nBufferSize != 0 );




		return Regular_Pop(pResult);

	}

	///////////////////////////////////////////////////////////////////////////////
	template<typename T>
	inline bool N_ProducerSingleConsumerQueue<T>::Regular_Pop( T *pResult )
	{
		// busy-loop if queue is empty
		int iter = 0;
		while(m_nRunning && m_nProducerIndex - m_nComsumerIndex == 0) { Sleep(iter++ > 10 ? 1 : 0); }

		if( m_nRunning == 0 && m_nProducerIndex - m_nComsumerIndex == 0)
			return false;
			
		iter = 0;
		while(m_arrStates[m_nComsumerIndex % m_nBufferSize] == 0 )  { Sleep(iter++ > 10 ? 1 : 0); }
		
		*pResult = m_arrBuffer[m_nComsumerIndex % m_nBufferSize];
		MemoryBarrier();
		m_arrStates[m_nComsumerIndex % m_nBufferSize] = 0;
		MemoryBarrier();
		m_nComsumerIndex += 1;
		MemoryBarrier();

		return true;
	}
	


	///////////////////////////////////////////////////////////////////////////////
	template<typename T>
	inline bool N_ProducerSingleConsumerQueue<T>::SPU_Pop( T *pResult )
	{



		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	// Class implementation are in the platform specific headers
	// ex Xenon_MT.h
	//////////////////////////////////////////////////////////////////////////

#if !defined(XENON)
	///////////////////////////////////////////////////////////////////////////////
	// 
	// Multi-thread safe lock-less FIFO queue container for passing pointers between threads.
	// The queue only stores pointers to T, it does not copy the contents of T.
	//
	//////////////////////////////////////////////////////////////////////////
	template <class T, class Alloc = std::allocator<T> >
	class CLocklessPointerQueue
	{
	public:
		explicit CLocklessPointerQueue(size_t reserve = 32) { m_lockFreeQueue.reserve(reserve); };
		~CLocklessPointerQueue() {};

		// Check's if queue is empty.
		bool	empty() const;

		// Pushes item to the queue, only pointer is stored, T contents are not copied.
		void	push( T* ptr );
		// pop can return NULL, always check for it before use.
		T*    pop();

	private:
		typedef typename Alloc::template rebind<T*> Alloc_rebind;
		queue<T*, typename Alloc_rebind::other> m_lockFreeQueue;
	};

	//////////////////////////////////////////////////////////////////////////
	template <class T, class Alloc>
	inline bool CLocklessPointerQueue<T, Alloc>::empty() const
	{
		return m_lockFreeQueue.empty();
	}

	//////////////////////////////////////////////////////////////////////////
	template <class T, class Alloc>
	inline void CLocklessPointerQueue<T, Alloc>::push( T* ptr )
	{
		m_lockFreeQueue.push(ptr);
	}

	//////////////////////////////////////////////////////////////////////////
	template <class T, class Alloc>
	inline T* CLocklessPointerQueue<T, Alloc>::pop()
	{
		T* val = NULL;
		m_lockFreeQueue.try_pop(val);
		return val;
	}

#endif
}; // namespace CryMT

namespace stl
{
	template <typename T> void free_container(CryMT::vector<T>& v)
	{
		v.free_memory();
	}
	template <typename T> void free_container(CryMT::queue<T>& v)
	{
		v.free_memory();
	}
}






#endif // __MultiThread_Containters_h__
