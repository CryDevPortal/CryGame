/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios
-------------------------------------------------------------------------
History:
- 14:05:2010		Created by Steve Humphreys
*************************************************************************/

#include "StdAfx.h"

#include "Game.h"
#include "GameAchievements.h"

#include "ICryReward.h"
#include "GameRules.h"
#include "Network/Lobby/GameLobbyData.h"
#include "Network/Lobby/GameLobby.h"
#ifdef GAME_IS_CRYSIS2
#include "PersistantStats.h"
#endif
#include "Projectile.h"
#include "WeaponSystem.h"
#ifdef GAME_IS_CRYSIS2
#include "HUD/HUDEventDispatcher.h"
#endif

#include <IPlatformOS.h>

const float THROW_TIME_THRESHOLD = 5.0f;

CGameAchievements::CGameAchievements()
: m_lastPlayerThrownObject(0)
, m_lastPlayerKillBulletId(0)
, m_lastPlayerKillGrenadeId(0)
, m_killsWithOneGrenade(0)
, m_HMGHitType(-1)
, m_gaussBulletHitType(-1)
, m_allowAchievements(true)
{
	m_lastPlayerKillBulletId = 0;
	g_pGame->GetIGameFramework()->RegisterListener(this, "CGameAchievements", eFLPriority_Game);

	CGameLobbyManager *pGameLobbyManager = g_pGame->GetGameLobbyManager();
	CRY_ASSERT(pGameLobbyManager || gEnv->IsEditor());
	if(pGameLobbyManager)
	{
		pGameLobbyManager->AddPrivateGameListener(this);
	}
}

CGameAchievements::~CGameAchievements()
{
	CGameLobbyManager *pGameLobbyManager = g_pGame->GetGameLobbyManager();
	if(pGameLobbyManager)
	{
		pGameLobbyManager->RemovePrivateGameListener(this);
	}

	g_pGame->GetIGameFramework()->UnregisterListener(this);

#ifdef GAME_IS_CRYSIS2
	RemoveHUDEventListeners();
#endif
}

void CGameAchievements::GiveAchievement(ECryGameSDKAchievement achievement)
{
	if(AllowAchievement())
	{
		assert(achievement >= 0 && achievement < eA_NumAchievements);

#ifdef GAME_IS_CRYSIS2
		CPersistantStats::GetInstance()->OnGiveAchievement(achievement);
#endif

		ICryLobby* pLobby = gEnv->pNetwork->GetLobby();
		IPlatformOS* pOS = gEnv->pSystem->GetPlatformOS();
		if(pLobby != NULL && pOS != NULL)
		{
			ICryReward* pReward = pLobby->GetReward();
			if(pReward)
			{
				unsigned int user = 0;
#ifdef GAME_IS_CRYSIS2
				unsigned int user = g_pGame->GetExclusiveControllerDeviceIndex();
#endif
				if(user != IPlatformOS::Unknown_User)
				{
					uint32 achievementId = achievement + ACHIEVEMENT_STARTINDEX;

					ECryRewardError error = pReward->Award(user, achievementId, NULL, NULL, NULL);
					CryLog("Award error %d", error);
					CRY_ASSERT(error == eCRE_Queued);
				}
			}
		}
	}
	else
	{
		CryLog("Not Awarding achievement - have been disabled");
	}
}

bool CGameAchievements::AllowAchievement()
{
	return m_allowAchievements 
#ifdef GAME_IS_CRYSIS2
		&& (g_pGameCVars->g_EPD == 0)
#endif
		;
}

void CGameAchievements::OnActionEvent(const SActionEvent& event)
{
	// assuming that we don't want to detect anything in MP.
	if(event.m_event == eAE_inGame && !gEnv->bMultiplayer)
	{
		CGameRules* pGR = g_pGame->GetGameRules();
		if(pGR)
		{
#ifdef GAME_IS_CRYSIS2
			pGR->RegisterKillListener(this);
#endif
			
			m_HMGHitType = pGR->GetHitTypeId("HMG");
			m_gaussBulletHitType = pGR->GetHitTypeId("gaussBullet");
		}

		m_lastPlayerThrownObject = 0;
		m_lastPlayerKillBulletId = 0;
		m_lastPlayerKillGrenadeId = 0;
		m_killsWithOneGrenade = 0;
	}

	// NB: by the time the eAE_unloadlevel event is sent the game
	//	rules is already null: can't unregister.
}

#ifdef GAME_IS_CRYSIS2
void CGameAchievements::OnHUDEvent(const SHUDEvent& event)
{
	// assuming that we don't want to detect anything in MP.
	if(event.eventType == eHUDEvent_OnEntityScanned && !gEnv->bMultiplayer)
	{
		g_pGame->GetPersistantStats()->IncrementClientStats(EIPS_ScanObject, 1);
	}
}
#endif

#ifdef GAME_IS_CRYSIS2
void CGameAchievements::OnEntityKilled(const HitInfo &hitInfo)
{
	// target must be an AI
	IActor* pTarget = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(hitInfo.targetId);
	if(!pTarget || pTarget->IsPlayer())
		return;

	// shooter might be null, if this is a collision, but will be checked otherwise
	IActor* pShooter = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(hitInfo.shooterId);

	switch(CategoriseHit(hitInfo.type))
	{	
		case eCHT_Bullet:
		{
			// ignore AI shots
			if(!pShooter || !pShooter->IsPlayer())
				break;

			CProjectile* pProjectile = g_pGame->GetWeaponSystem()->GetProjectile(hitInfo.projectileId);
			assert(pProjectile);
			const CTimeValue& spawnTime = pProjectile ? pProjectile->GetSpawnTime() : 0.0f;
			if(spawnTime == 0.0f)
				break;

			if(hitInfo.projectileId == m_lastPlayerKillBulletId
				&& spawnTime == m_lastPlayerKillBulletSpawnTime)
			{
				// same projectile as previously, trigger the 'two kills one bullet' objective
				GiveAchievement(eA_SPFeature_OneBulletTwoKills);
			}
			else
			{
				// save for later
				m_lastPlayerKillBulletId = hitInfo.projectileId;
				m_lastPlayerKillBulletSpawnTime = spawnTime;
			}
		}
		break;

		case eCHT_Grenade:
		{
			if(!pShooter || !pShooter->IsPlayer())
				break;

			CProjectile* pGrenade = g_pGame->GetWeaponSystem()->GetProjectile(hitInfo.projectileId);
			const CTimeValue& spawnTime = pGrenade ? pGrenade->GetSpawnTime() : 0.0f;
			if(spawnTime == 0.0f)
				break;

			if(hitInfo.projectileId == m_lastPlayerKillGrenadeId
				&& spawnTime == m_lastPlayerKillGrenadeSpawnTime)
			{
				if(++m_killsWithOneGrenade == 3)
				{
					GiveAchievement(eA_SPFeature_OneGrenadeThreeKills);
				}
			}
			else
			{
				// save for later
				m_lastPlayerKillGrenadeId = hitInfo.projectileId;
				m_lastPlayerKillGrenadeSpawnTime = spawnTime;
				m_killsWithOneGrenade = 1;
			}
			
		}
		break;

		case eCHT_Collision:
		{
			CTimeValue now = gEnv->pTimer->GetFrameStartTime();
			if(hitInfo.weaponId != 0 
				&& hitInfo.weaponId == m_lastPlayerThrownObject
				&& (now-m_lastPlayerThrownTime) < THROW_TIME_THRESHOLD)
			{
				// AI was killed by an object the player threw in the last x seconds...
				g_pGame->GetPersistantStats()->IncrementClientStats(EIPS_ThrownObjectKill, 1);
			}
		}
		break;

		default:
		break;
	}
}
#endif

void CGameAchievements::PlayerThrewObject(EntityId object, bool ai)
{
	if(gEnv->bMultiplayer)
		return;

	if(ai)
	{
#ifdef GAME_IS_CRYSIS2
		g_pGame->GetPersistantStats()->IncrementClientStats(EIPS_GrabAndThrow, 1);
#endif
	}

	m_lastPlayerThrownObject = object;
	m_lastPlayerThrownTime = gEnv->pTimer->GetFrameStartTime();
}

#ifdef GAME_IS_CRYSIS2
void CGameAchievements::AddHUDEventListeners()
{
	CHUDEventDispatcher::AddHUDEventListener(this, "OnEntityScanned");
}

void CGameAchievements::RemoveHUDEventListeners()
{
	CHUDEventDispatcher::RemoveHUDEventListener(this);
}
#endif

CGameAchievements::ECategorisedHitType CGameAchievements::CategoriseHit(int hitType)
{
	ECategorisedHitType outType;

#ifdef GAME_IS_CRYSIS2
	if(hitType == CGameRules::EHitType::Collision)
	{
		outType = eCHT_Collision;
	}
	else if(hitType == CGameRules::EHitType::Frag)
	{
		outType = eCHT_Grenade;
	}
	else if(hitType == CGameRules::EHitType::Bullet
				|| hitType == CGameRules::EHitType::HeavyBullet
				|| hitType == m_HMGHitType
				|| hitType == m_gaussBulletHitType)
	{
		outType = eCHT_Bullet;
	}
	else
#endif
	{
		outType = eCHT_Other;
	}

	return outType;
}

void CGameAchievements::SetPrivateGame(const bool privateGame)
{
	m_allowAchievements = !privateGame;
}
