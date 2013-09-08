////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  Created:     05/08/2011 by Will Wilson
//  Description: Interface to manage SoftCode module loading and patching
// -------------------------------------------------------------------------
//  History: Created by Will Wilson.
//
////////////////////////////////////////////////////////////////////////////

#ifndef __ISOFTCODE_MGR_H_
#define __ISOFTCODE_MGR_H_

#pragma once

// Provides the generic interface for exchanging member values between SoftCode modules,
struct IExchangeValue
{
	virtual ~IExchangeValue() {}

	// Allocates a new IExchangeValue with the underlying type
	virtual IExchangeValue* Clone() const = 0;
	// Returns the size of the underlying type (to check compatibility)
	virtual size_t GetSizeOf() const = 0;
};

template <typename T>
struct ExchangeValue : public IExchangeValue
{
	ExchangeValue(T& value)
		: m_value(value)
	{}

	virtual IExchangeValue* Clone() const { return new ExchangeValue(*this); }
	virtual size_t GetSizeOf() const { return sizeof(m_value); }

	T m_value;
};

// Interface for performing an exchange of instance data
struct IExchanger
{
	virtual ~IExchanger() {}

	template <typename T>
	void Visit(const char* name, T& instance);

	// True if data is being read from instance members
	virtual bool IsLoading() const = 0;
	// True if existing member data should be preserved (use to restore old instances)
	virtual bool PreserveUnknownMembers() const = 0;

	virtual size_t InstanceCount() const = 0;

	virtual bool BeginInstance(void* pInstance) = 0;
	virtual bool SetValue(const char* name, IExchangeValue& value) = 0;
	virtual IExchangeValue* GetValue(const char* name, void* pTarget, size_t targetSize) = 0;
};

template <typename T>
void IExchanger::Visit(const char* name, T& value)
{
	if (IsLoading())
	{
		IExchangeValue* pValue = GetValue(name, &value, sizeof(value));
		if (pValue)
		{
			ExchangeValue<T>* pTypedValue = static_cast<ExchangeValue<T>*>(pValue);
			value = pTypedValue->m_value;
		}
		// If member is new and setup in the constructor - we don't want to destroy the value!
// 		else if (!PreserveUnknownMembers())	// No value found, set to default value
// 		{
// 			value = T();
// 		}
	}
	else	// Saving
	{
		// If this member is stored
		if (SetValue(name, ExchangeValue<T>(value)))
		{
			// Set the original value to the default value (to allow safe destruction)
			value = T();
		}
	}
}

struct ITypeRegistrar
{
	virtual ~ITypeRegistrar() {}

	virtual const char* GetName() const = 0;

	// Creates an instance of the type
	virtual void* CreateInstance() = 0;

#ifdef SOFTCODE_ENABLED
	// How many active instances exist of this type?
	virtual size_t InstanceCount() const = 0;
	// Used to remove a tracked instance from the Registrar
	virtual void RemoveInstance(size_t index) = 0;
// 	// Retrieve the instances
// 	virtual size_t GetInstances(void** ppInstances, size_t& count) const = 0;
	// Exchanges the instance state with the given exchanger data set
	virtual bool ExchangeInstances(IExchanger& exchanger) = 0;
	// Destroys all tracked instances of this type
	virtual bool DestroyInstances() = 0;
#endif
};

struct ITypeLibrary
{
	virtual ~ITypeLibrary() {}

	virtual const char* GetName() = 0;
	virtual bool CreateInstance(const char* typeName, void** ppInstance) = 0;

#ifdef SOFTCODE_ENABLED
	virtual void SetOverride(ITypeLibrary* pOverrideLib) = 0;

		// Fills in the supplied type list if large enough, and sets count to number of types
	virtual size_t GetTypes(ITypeRegistrar** ppRegistrar, size_t& count) const = 0;
#endif
};

struct ISoftCodeListener
{
	virtual ~ISoftCodeListener() {}

	// Called when an instance is replaced to allow managing systems to fixup pointers
	virtual void InstanceReplaced(void* pOldInstance, void* pNewInstance) = 0;
};

/// Interface for ...
struct ISoftCodeMgr
{
	virtual ~ISoftCodeMgr() {}

	// Used to register built-in libraries on first use
	virtual void RegisterLibrary(ITypeLibrary* pLib) = 0;

	// Loads any new SoftCode modules
	virtual void LoadNewModules() = 0;

	virtual void AddListener(const char* libraryName, ISoftCodeListener* pListener, const char* listenerName) = 0;
	virtual void RemoveListener(const char* libraryName, ISoftCodeListener* pListener) = 0;

	// To be called regularly to poll for library updates
	virtual void PollForNewModules() = 0;
	
	/// Frees this instance from memory
	//virtual void Release() = 0;
};

#endif // __ISOFTCODE_MGR_H_
