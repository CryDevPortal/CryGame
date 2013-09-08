/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios
-------------------------------------------------------------------------
History:
- 14:05:2010		Created by Steve Humphreys
*************************************************************************/

#pragma once

#ifndef __GAME_ACHIEVEMENTS_H__
#define __GAME_ACHIEVEMENTS_H__

#include "IGameFramework.h"
//#include "GameRulesTypes.h"
//#include "GameRulesModules/IGameRulesKillListener.h"
#include "../DownloadMgr.h"
//#include "HUD/HUDEventDispatcher.h"
#include "Network/Lobby/GameLobbyManager.h"


//////////////////////////////////////////////////////////////////////////
// Achievements / Trophies
//	For now we assume that achievements are numbered sequentially on
//	both platforms, though potentially starting at different indices.
//	Since this depends on xlast etc it may be necessary to assign
//	an id per achievement for each platform later.
//////////////////////////////////////////////////////////////////////////











//Currently PC achievements are stubbed out in CryNetwork (AFAIK gamespy achievements are not planned for C2)
static const int ACHIEVEMENT_STARTINDEX = 0;


#define Achievement(f) \
	f(eA_CanItRunTheSDK) \
	f(eA_ForeignContaminant) \
	f(eA_MoreThanHuman) \
	f(eA_FalseProphet) \
	f(eA_InternalAffairs) \
	f(eA_IntoTheAbyss) \
	f(eA_OnceAMarine) \
	f(eA_HungOutToDry) \
	f(eA_FireWalker) \
	f(eA_DarkNightOfTheSoul) \
	f(eA_CrossroadsOfTheWorld) \
	f(eA_TheseusAtLast) \
	f(eA_HomeStretch) \
	f(eA_Complete_Campaign) \
	f(eA_Complete_6_Hard) \
	f(eA_Complete_12_Hard) \
	f(eA_Complete_6_Delta) \
	f(eA_Complete_12_Delta) \
	f(eA_Complete_Campaign_Hard) \
	f(eA_Complete_Campaign_Delta) \
	f(eA_SPFeature_StealthKills) \
	f(eA_SPFeature_NYTourist) \
	f(eA_SPFeature_ThrowObject) \
	f(eA_SPFeature_GrabAndThrow) \
	f(eA_SPFeature_Microwave) \
	f(eA_SPFeature_OneBulletTwoKills) \
	f(eA_SPFeature_OneGrenadeThreeKills) \
	f(eA_SPFeature_HeadshotStreak) \
	f(eA_SPFeature_Sliding) \
	f(eA_Level_FoodForThought) \
	f(eA_Level_HoleInOne) \
	f(eA_Level_BandOfBrothers) \
	f(eA_Level_Books) \
	f(eA_Level_JewelThief) \
	f(eA_SPFeature_SpeedCamera) \
	f(eA_MP_SDKWhatSDK) \
	f(eA_MP_WinIHazIt) \
	f(eA_MP_NaniteMight) \
	f(eA_MP_TooledUp) \
	f(eA_MP_TheCleaner) \
	f(eA_MP_CrySpy) \
	f(eA_MP_PlayYourWay) \
	f(eA_MP_Dedication) \
	f(eA_MP_ModernArt) \
	f(eA_MP_SuckItAndSee) \
	f(eA_MP_DoggieStyle) \
	f(eA_MP_ModularDevelopment) \
	f(eA_MP_YouGottaFightForYourRight) \
	f(eA_MP_Crytinerant) \
	f(eA_MP_IAmNotANumber) \


AUTOENUM_BUILDENUMWITHTYPE_WITHNUM(ECryGameSDKAchievement, Achievement, eA_NumAchievements);

class CGameAchievements 
	: 
#ifdef GAME_IS_CRYSIS2
	public IGameRulesKillListener, 
	public IHUDEventListener, 
#endif
	public IGameFrameworkListener,	
	public IPrivateGameListener
{
public:
	CGameAchievements();
	virtual ~CGameAchievements();

	void GiveAchievement(ECryGameSDKAchievement achievement);
	void PlayerThrewObject(EntityId object, bool ai);

#ifdef GAME_IS_CRYSIS2
	void AddHUDEventListeners();
	void RemoveHUDEventListeners();
#endif

	//IGameFrameworkListener
	virtual void OnPostUpdate(float fDeltaTime) {};
	virtual void OnSaveGame(ISaveGame* pSaveGame) {};
	virtual void OnLoadGame(ILoadGame* pLoadGame) {};
	virtual void OnLevelEnd(const char* nextLevel) {};
	virtual void OnActionEvent(const SActionEvent& event);
	//IGameFrameworkListener

#ifdef GAME_IS_CRYSIS2
	// IGameRulesKillListener
	virtual void OnEntityKilledEarly(const HitInfo &hitInfo) {};
	virtual void OnEntityKilled(const HitInfo &hitInfo);
	// ~IGameRulesKillListener

	//IHUDEventListener
	virtual void OnHUDEvent(const SHUDEvent& event);
	//~IHUDEventListener
#endif

	//IPrivateGameListener
	virtual void SetPrivateGame(const bool privateGame);
	//~IPrivateGameListener

protected:
	
	enum ECategorisedHitType
	{
		eCHT_Collision,
		eCHT_Bullet,
		eCHT_Grenade,
		eCHT_Other,
	};
	ECategorisedHitType CategoriseHit(int hitType);
	ILINE bool AllowAchievement();

	// track some things for detecting certain achievements.
	//	NB: DO NOT SERIALIZE... potential for cheating:
	//	- player throws object
	//	- saves
	//	- on load object kills enemy
	//	- repeated loading = achievement
	EntityId m_lastPlayerThrownObject;
	CTimeValue m_lastPlayerThrownTime;

	EntityId m_lastPlayerKillBulletId;
	CTimeValue m_lastPlayerKillBulletSpawnTime;

	EntityId m_lastPlayerKillGrenadeId;
	int m_killsWithOneGrenade;
	CTimeValue m_lastPlayerKillGrenadeSpawnTime;

	// bullet types
	int m_HMGHitType;
	int m_gaussBulletHitType;


	bool m_allowAchievements;
};

#endif // __GAME_ACHIEVEMENTS_H__
