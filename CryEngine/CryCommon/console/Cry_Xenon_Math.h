//////////////////////////////////////////////////////////////////////
//
//	Crytek Common Source code
//
//	File:Cry_Math.h
//	Description: Common math class for Xenon
//
//	History:
//	-Dec 12,2007: Created by Alexey
//
//////////////////////////////////////////////////////////////////////


#ifndef CRY_XENON_MATH
#define CRY_XENON_MATH
#pragma once 

//=============================================================================

struct XMVec4A
{
	static ILINE XMVECTOR LoadVec4 (const void *pV)
	{
		return XMLoadVector4A(pV);
	}
	static ILINE XMVECTOR LoadVec3 (const void *pV)
	{
		return XMLoadVector3A(pV);
	}
	static ILINE void StoreVec4 (void *pDest, const XMVECTOR& xm)
	{
		XMStoreVector4A(pDest, xm);
	}
	static ILINE void StoreVec3 (void *pDest, const XMVECTOR& xm)
	{
		XMStoreVector3A(pDest, xm);
	}

	static void *New(size_t s)
	{
		unsigned char *p = ::new unsigned char[s + 16];
		if (p)
		{
			unsigned char offset = (unsigned char)(16 - ((UINT_PTR)p & 15));
			p += offset;
			p[-1] = offset;
		}
		return p;
	}
	static void Del(void *p)
	{
		if(p)
		{
			unsigned char* pb = static_cast<unsigned char*>(p);
			pb -= pb[-1];
			::delete [] pb;
		}
	}
};

struct XMVec4
{
	static ILINE XMVECTOR LoadVec4 (const void *pV)
	{
		return XMLoadVector4(pV);
	}
	static ILINE XMVECTOR LoadVec3 (const void *pV)
	{
		return XMLoadVector3(pV);
	}
  static ILINE XMVECTOR LoadVec2 (const void *pV)
  {
    return XMLoadVector2(pV);
  }
	static ILINE void StoreVec4 (void *pDest, const XMVECTOR& xm)
	{
		XMStoreVector4(pDest, xm);
	}
	static ILINE void StoreVec3 (void *pDest, const XMVECTOR& xm)
	{
		XMStoreVector3(pDest, xm);
	}
	static void *New(size_t s)
	{
		unsigned char *p = ::new unsigned char[s];
		return p;
	}
	static void Del(void *p)
	{
		if(p)
			::delete [] (unsigned char*)p;
	}

	NULL_STRUCT_INFO
};












#endif
