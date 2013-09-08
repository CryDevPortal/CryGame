#ifndef __NETWORKDEFINES_H__
#define __NETWORKDEFINES_H__

//---------------------------------------------------------------------------

#if !(defined(_XBOX) || defined(_PS3))
#define IMPLEMENT_PC_BLADES 0
#else
#define IMPLEMENT_PC_BLADES 0
#endif 

#if /*!defined(PROFILE) && */!defined(_RELEASE)
#define GFM_ENABLE_EXTRA_DEBUG 0
#else
#define GFM_ENABLE_EXTRA_DEBUG 0
#endif

//---------------------------------------------------------------------------

#endif // __NETWORKDEFINES_H__
