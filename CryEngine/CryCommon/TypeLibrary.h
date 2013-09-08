/*************************************************************************
Crytek Source File.
Copyright (C) 2011, Crytek Studios
-------------------------------------------------------------------------
History:
- 03:08:2011		Created by Will Wilson
*************************************************************************/

#pragma once

#ifndef __TYPELIBRARY_H__
#define __TYPELIBRARY_H__

#include "ISoftCodeMgr.h"

#ifdef SOFTCODE_ENABLED

// Internal: Used by SC types to auto-remove themselves from their TypeRegistrar on destruction.
struct InstanceTracker
{
	InstanceTracker()
		: m_pRegistrar()
	{}

	~InstanceTracker()
	{
		if (m_pRegistrar)
			m_pRegistrar->RemoveInstance(m_index);
	}

	void SetRegistrar(ITypeRegistrar* pRegistrar, size_t index)
	{
		m_pRegistrar = pRegistrar;
		m_index = index;
	}

	ITypeRegistrar* m_pRegistrar;
	size_t m_index;
};

#endif


#ifdef SOFTCODE_ENABLED

	// Include this for SEH support
	#include <excpt.h>

	/*
		Exposes a class to a TypeLibrary for registration.
		Usage:
		class MyThing : public IThing
		{
			DECLARE_TYPE(MyThing);
			...
	*/
	#define DECLARE_TYPE(TName) \
		private: \
		friend class TypeRegistrar<TName>; \
		static const size_t __START_MEMBERS = __COUNTER__ + 1; \
		template <size_t IDX> void VisitMember(IExchanger& exchanger) {} \
		void VisitMembers(IExchanger& exchanger) { VisitMember<__START_MEMBERS>(exchanger); } \
		InstanceTracker __instanceTracker

#ifdef SOFTCODE
	#define _EXPORT_TYPE_LIB(Interface) \
	extern "C" ITypeLibrary* GetTypeLibrary() {	return CTypeLibrary<Interface>::Instance(); }
#else
	#define _EXPORT_TYPE_LIB(Interface)
#endif

	// Internal: Outputs the specialized method template for the member at index
	#define _SOFT_MEMBER_VISITOR(member, index) \
		template <> void VisitMember<index>(IExchanger& exchanger) { exchanger.Visit(#member, member); VisitMember<index+1>(exchanger); }

	/*
		Used to expose a class member to SoftCoding (to allow run-time member exchange)
		If SoftCode is disabled this does nothing and simple emits the member.
		Usage: std::vector<string> SOFT(m_myStrings);
	*/
	#define SOFT(member) \
		member; \
		_SOFT_MEMBER_VISITOR(member, __COUNTER__)

	// The SoftCode exception filter, needs to be implemented in each module
	SC_API int SoftCodeExceptionFilter(DWORD exCode);

	#define SOFTCODE_TRY __try
	#define SOFTCODE_ON_FAIL(exp) __except(SoftCodeExceptionFilter(GetExceptionCode())) { exp; }

	#define SOFTCODE_TRY2(exp, ok) \
		[&]() { ok = false; \
		__try { exp; ok = true; } \
		__except(SoftCodeExceptionFilter(GetExceptionCode())) { }; }()

#else	// !SOFTCODE_ENABLED ...

	#define DECLARE_TYPE(TName) \
			private: \
			friend class TypeRegistrar<TName>;

	#define _EXPORT_TYPE_LIB(Interface)

	#define SOFT(member) member

	#define SOFTCODE_TRY
	#define SOFTCODE_ON_FAIL(exp)

	#define SOFTCODE_TRY2(exp, ok) do { exp; ok = true; } while (false)

#endif

/*
	Implements registration for a type to a TypeLibrary.
	Usage:
		// MyThing.cpp
		DECLARE_TYPE(MyThing);
*/
#define IMPLEMENT_TYPE(TName) \
	static TypeRegistrar<TName> s_typeRegistrar(#TName)

/*
	Provides the singleton for the TypeLibrary implementation.
	Also exports the accessors function for SoftCode builds.
	Usage:
	// ThingLibrary.cpp
	IMPLEMENT_TYPELIB(IThing, "Things")
*/
#define IMPLEMENT_TYPELIB(Interface, Name) \
	_EXPORT_TYPE_LIB(Interface) \
	template <>	CTypeLibrary<Interface>* CTypeLibrary<Interface>::Instance() \
	{ \
		static CTypeLibrary<Interface> s_instance(Name); \
		return &s_instance;	\
	}

/*
	Internal: Used to register a type with a TypeLibrary.
	Also provides instance construction (factory) access.
	For SC builds it also provides copying and instance tracking.
*/
template <typename T>
class TypeRegistrar : public ITypeRegistrar
{
public:
	TypeRegistrar(const char* name)
		: m_name(name)
	{
		typedef typename T::TLibrary TLib;
		TLib::Instance()->RegisterType(this);
	}

	virtual const char* GetName() const { return m_name; }

	virtual void* CreateInstance()
	{
		T* pInstance = NULL;

		SOFTCODE_TRY
		{
			pInstance = ConstructInstance();
			RegisterInstance(pInstance);
		}
		SOFTCODE_ON_FAIL(pInstance = NULL)
		
		return pInstance;
	}

#ifdef SOFTCODE_ENABLED
	virtual size_t InstanceCount() const
	{
		return m_instances.size();
	}

	virtual void RemoveInstance(size_t index)
	{
		std::swap(m_instances[index], m_instances.back());
		m_instances.pop_back();
	}

	virtual bool ExchangeInstances(IExchanger& exchanger)
	{
		if (exchanger.IsLoading())
		{
			const size_t instanceCount = exchanger.InstanceCount();

			// Ensure we have the correct number of instances
			if (m_instances.size() != instanceCount)
			{
				// TODO: Destroy any existing instances
				for (size_t i = 0; i < instanceCount; ++i)
				{
					if (!CreateInstance())
						return false;
				}
			}
		}

		//SOFTCODE_TRY
		{
			for (TInstanceVec::iterator iter(m_instances.begin()); iter != m_instances.end(); ++iter)
			{
				T* pInstance = *iter;
				if (exchanger.BeginInstance(pInstance))
				{
					// Exchanges the members of pInstance as defined in T
					// Should also exchange members of parent types
					pInstance->VisitMembers(exchanger);
				}
			}
		}
		//SOFTCODE_ON_FAIL(return false)

		return true;
	}

// 	virtual size_t GetInstances(void** ppInstances, size_t& count) const
// 	{
// 		size_t returnedCount = 0;
// 
// 		if (ppInstances && count >= m_instances.size())
// 		{
// 			returnedCount = m_instances.size();
// 			memcpy(*ppInstances, &(m_instances.front()), returnedCount * sizeof(void*))
// 		}
// 		count = m_instances.size();
// 		return returnedCount;
// 	}

	virtual bool DestroyInstances()
	{
		SOFTCODE_TRY
		{
			while (!m_instances.empty())
			{
				delete m_instances.back();
				// NOTE: No need to pop_back() as already done by the InstanceTracker via RemoveInstance()
			}
		}
		SOFTCODE_ON_FAIL(return false)

		return true;
	}
#endif

private:
	void RegisterInstance(T* pInstance)
	{
#ifdef SOFTCODE_ENABLED
		size_t index = m_instances.size();
		pInstance->__instanceTracker.SetRegistrar(this, index);
		m_instances.push_back(pInstance);
#endif
	}

	// Needed to avoid C2712 due to lack of stack unwind within SEH try blocks
	T* ConstructInstance()
	{
		return new T();
	}

private:
	const char* m_name;										// Name of the type

#ifdef SOFTCODE_ENABLED
	typedef std::vector<T*> TInstanceVec;
	TInstanceVec m_instances;				// Tracks the active instances (SC only)
#endif
};

/*
	Provides factory creation support for a set of types that 
	derive from a single interface T. Users need to provide a 
	specialization of the static CTypeLibrary<T>* Instance() member
	in a cpp file to provide the singleton instance.
*/
template <typename T>
class CTypeLibrary
	#ifdef SOFTCODE_ENABLED
		: public ITypeLibrary
	#endif
{
public:
	CTypeLibrary(const char* name)
		: m_name(name)
#ifdef SOFTCODE_ENABLED
		,m_pOverrideLib()
		,m_overrideActive()
		,m_registered()
#endif
	{
	}

	// Implemented in the export cpp
	static CTypeLibrary<T>* Instance();

	void RegisterType(ITypeRegistrar* pType)
	{
		m_typeMap[pType->GetName()] = pType;
	}

	// The global identifier for this library module
	/*virtual*/ const char* GetName() { return m_name; }

	// Generic creation function
	/*virtual*/ bool CreateInstance(const char* typeName, void** ppInstance)
	{
#ifdef SOFTCODE_ENABLED
		RegisterWithSoftCode();

		// If override is enabled and the override can create the instance
		if (m_pOverrideLib && m_pOverrideLib->CreateInstance(typeName, ppInstance))
			return true;
#endif

		TTypeMap::const_iterator typeIter(m_typeMap.find(typeName));
		if (typeIter != m_typeMap.end())
		{
			ITypeRegistrar* pRegistrar = typeIter->second;
			*ppInstance = pRegistrar->CreateInstance();

			return (*ppInstance != NULL);	// TODO: Just return directly
		}

		return false;
	}

#ifdef SOFTCODE_ENABLED
	// Indicates CreateInstance requests should be forwarded to the specified lib
	virtual void SetOverride(ITypeLibrary* pOverrideLib)
	{
		m_pOverrideLib = pOverrideLib;
	}

	virtual size_t GetTypes(ITypeRegistrar** ppTypes, size_t& count) const
	{
		size_t returnedCount = 0;

		if (ppTypes && count >= m_typeMap.size())
		{
			for (TTypeMap::const_iterator iter(m_typeMap.begin()); iter != m_typeMap.end(); ++iter)
			{
				*ppTypes = iter->second;
				++ppTypes;
				++returnedCount;
			}
		}

		count = m_typeMap.size();
		return returnedCount;
	}

	// Inform the Mgr of this Library and allow it to set an override
	inline void RegisterWithSoftCode()
	{
		// Only register built-in types, SC types are handled directly by 
		// the SoftCodeMgr, so there's no need to auto-register.
#ifndef SOFTCODE
		if (!m_registered)
		{
			if (ISoftCodeMgr* pSoftCodeMgr = gEnv->pSoftCodeMgr)
				pSoftCodeMgr->RegisterLibrary(this);

			m_registered = true;
		}
#endif
	}
#endif

	// Utility creation function, bypasses virtual overhead
	T* CreateInstance(const char* typeName)
	{
		T* pInstance = NULL;

		// Call create function directly (avoiding one virtual call)
		CTypeLibrary<T>::CreateInstance(typeName, reinterpret_cast<void**>(&pInstance));

		return pInstance;
	}

private:
	typedef std::map<string, ITypeRegistrar*> TTypeMap;
	TTypeMap m_typeMap;

	// The name for this TypeLibrary used during SC registration
	const char* m_name;

#ifdef SOFTCODE_ENABLED
	// Used to patch in a new TypeLib at run-time
	ITypeLibrary* m_pOverrideLib;
	// True when the owning object system enables the override
	bool m_overrideActive;
	// True when registration with SoftCodeMgr has been attempted
	bool m_registered;
#endif
};

#endif
