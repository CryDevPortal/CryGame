/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2012.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 

	-------------------------------------------------------------------------
	History:
	- 06:07:2012  : Created by Michiel Meesters

*************************************************************************/

#ifndef _IGameRulesManager_h_
#define _IGameRulesManager_h_

#if _MSC_VER > 1000
# pragma once
#endif

#include "IGameFramework.h"

class IGameRulesManager
{
public:
	virtual ~IGameRulesManager() {}

	virtual void Init() = 0;

	virtual int GetRulesCount() = 0;

	virtual const char* GetRules(int index) = 0;
};

#endif // _IGameRulesManager_h_
