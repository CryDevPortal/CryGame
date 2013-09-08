////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   birdEnum.h
//  Version:     v1.00
//  Created:     7/2010 by Luciano Morpurgo
//  Compilers:   Visual C++ 7.0
//  Description: Bird Namespace with enums and others
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __birdenum_h__
#define __birdenum_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "StdAfx.h"

namespace Bird
{
	typedef enum
	{
		FLYING,
		TAKEOFF,
		LANDING,
		ON_GROUND,
		DEAD
	} EStatus;

	enum
	{
		ANIM_FLY,
		ANIM_TAKEOFF,
		ANIM_WALK,
		ANIM_IDLE,
	};

} // namespace


#endif // __birdenum_h__
