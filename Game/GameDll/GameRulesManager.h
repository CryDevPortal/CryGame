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

#ifndef _GameRulesManager_h_
#define _GameRulesManager_h_

#if _MSC_VER > 1000
# pragma once
#endif

#include "IGameRulesManager.h"

class CGameRulesManager : public IGameRulesManager
{
private:
	static CGameRulesManager  *s_pInstance;

	typedef CryFixedStringT<32>	TFixedString_32;
	typedef CryFixedStringT<64>	TFixedString_64;

	struct SGameRulesData{
		TFixedString_64 m_rulesXMLPath;
		TFixedString_64 m_defaultHud;
		bool m_bIsTeamGame;

		//ctor
		SGameRulesData() : m_rulesXMLPath(""), m_defaultHud(""), m_bIsTeamGame(false) {}
	};

	typedef std::map<TFixedString_32, SGameRulesData> TDataMap;
	TDataMap m_rulesData;

public:
	static CGameRulesManager  *GetInstance(bool create = true);

	CGameRulesManager ();
	virtual ~CGameRulesManager ();

	void Init();

	// Summary
	//	 Returns the path for the gamerules XML description.
	const char *GetXmlPath(const char *gameRulesName) const;

	// Summary
	//	 Returns the default HUDState name path for the given gamerules.
	const char *GetDefaultHud(const char *gameRulesName) const;

	// Summary
	//	Returns the number of game rules
	int GetRulesCount();

	// Summary
	//	Returns the name of the nth GameRules (shouldn't be used at gametime)
	const char* GetRules(int index);

	// Summary
	//	Determines if the specified gameRules is a team game (very expensive, only use when the rules change)
	bool IsTeamGame(const char *gameRulesName) const;

	// Summary
	// Determaines if game rules are valid
	bool IsValidGameRules(const char *gameRulesName) const;
};

#endif // _GameRulesManager_h_
