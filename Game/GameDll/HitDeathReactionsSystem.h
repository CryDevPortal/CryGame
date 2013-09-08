/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
Description: 
This file holds shared data and functionality across all actor using the 
HitDeathReactions system
-------------------------------------------------------------------------
History:
- 22:2:2010	13:01 : Created by David Ramos
*************************************************************************/
#pragma once
#ifndef __HIT_DEATH_REACTIONS_SYSTEM_H
#define __HIT_DEATH_REACTIONS_SYSTEM_H

#include "HitDeathReactionsDefs.h"
#include "CustomReactionFunctions.h"
#include <VectorMap.h>

struct ICharacterModel;
class CActor;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class CHitDeathReactionsSystem
{
public:
	static void														Warning(const char* szFormat, ...);


	CHitDeathReactionsSystem();
	~CHitDeathReactionsSystem();

	void																	OnToggleGameMode();
	void																	Reset();

	ProfileId															GetReactionParamsForActor(const CActor& actor, ReactionsContainerConstPtr& pHitReactions, ReactionsContainerConstPtr& pDeathReactions, ReactionsContainerConstPtr& pCollisionReactions, SHitDeathReactionsConfigConstPtr& pHitDeathReactionsConfig);
	void																	RequestReactionAnimsForActor(const CActor& actor, uint32 requestFlags);
	void																	ReleaseReactionAnimsForActor(const CActor& actor, uint32 requestFlags);

	void																	Reload();
	void																	PreloadData();

	void																	DumpHitDeathReactionsAssetUsage() const;

	ILINE bool														IsStreamingEnabled() const { return static_cast<uint8>(GetStreamingPolicy()) != 0U; }

	void																	GetMemoryUsage(ICrySizer * s) const;

	ILINE CCustomReactionFunctions&				GetCustomReactionFunctions() { return m_customReactionFunctions; }
	ILINE const CCustomReactionFunctions&	GetCustomReactionFunctions() const { return m_customReactionFunctions; }

	CMTRand_int32&												GetRandomGenerator() { return m_pseudoRandom; }

private:
	// private types
	struct SReactionsProfile
	{
		typedef std::map<EntityId, uint32> entitiesUsingProfileContainer;

		SReactionsProfile() : timerId(0), iRefCount(0) {}
		SReactionsProfile(ReactionsContainerConstPtr pHitReactions, ReactionsContainerConstPtr pDeathReactions, ReactionsContainerConstPtr pCollisionReactions, ScriptTablePtr pHitAndDeathReactionsTable, SHitDeathReactionsConfigConstPtr	pHitDeathReactionsConfig) : 
		pHitReactions(pHitReactions), pDeathReactions(pDeathReactions), pCollisionReactions(pCollisionReactions), pHitAndDeathReactionsTable(pHitAndDeathReactionsTable), pHitDeathReactionsConfig(pHitDeathReactionsConfig), timerId(0), iRefCount(0) {}
		~SReactionsProfile();

		void				GetMemoryUsage(ICrySizer * s) const;
		ILINE bool	IsValid() const { return !(pHitReactions.expired() || pDeathReactions.expired() || pCollisionReactions.expired() || pHitDeathReactionsConfig.expired()); }

		ScriptTablePtr												pHitAndDeathReactionsTable;

		ReactionsContainerConstWeakPtr				pHitReactions;
		ReactionsContainerConstWeakPtr				pDeathReactions;
		ReactionsContainerConstWeakPtr				pCollisionReactions;

		SHitDeathReactionsConfigConstWeakPtr	pHitDeathReactionsConfig;

		entitiesUsingProfileContainer					entitiesUsingProfile;
		int																		iRefCount;

		IGameFramework::TimerID								timerId;
	};

	struct SFailSafeProfile
	{
		ReactionsContainerConstPtr				pHitReactions;
		ReactionsContainerConstPtr				pDeathReactions;
		ReactionsContainerConstPtr				pCollisionReactions;

		SHitDeathReactionsConfigConstPtr	pHitDeathReactionsConfig;
	};

	struct SPredGetMemoryUsage;
	struct SPredGetAnims;
	struct SPredRequestAnims;


	typedef std::map<ProfileId, SReactionsProfile>					ProfilesContainer;
	typedef ProfilesContainer::value_type										ProfilesContainersItem;

	typedef VectorMap<string, ScriptTablePtr>								FileToScriptTableMap;


	// Private methods
	void								ExecuteHitDeathReactionsScripts(bool bForceReload);
	ProfileId						GetActorProfileId(const CActor& actor) const;
	ScriptTablePtr			LoadReactionsScriptTable(const CActor& actor) const;
	ScriptTablePtr			LoadReactionsScriptTable(const char* szReactionsDataFile) const;
	bool								LoadHitDeathReactionsParams(const CActor& actor, ScriptTablePtr pHitDeathReactionsTable, ReactionsContainerPtr pHitReactions, ReactionsContainerPtr pDeathReactions, ReactionsContainerPtr pCollisionReactions);
	bool								LoadHitDeathReactionsConfig(const CActor& actor, ScriptTablePtr pHitDeathReactionsTable, SHitDeathReactionsConfigPtr pHitDeathReactionsConfig);
	void								LoadReactionsParams(const CActor& actor, IScriptTable* pHitDeathReactionsTable, const char* szReactionParamsName, bool bDeathReactions, ReactionId baseReactionId, ReactionsContainer& reactions);

	void								GetReactionParamsFromScript(const CActor& actor, const ScriptTablePtr pScriptTable, SReactionParams& reactionParams, ReactionId reactionId) const;
	bool								GetValidationParamsFromScript( const ScriptTablePtr pScriptTable, SReactionParams::SValidationParams& validationParams, const CActor& actor, ReactionId reactionId) const;
	void								GetReactionAnimParamsFromScript(const CActor& actor, ScriptTablePtr pScriptTable, SReactionParams::SReactionAnim& reactionAnim) const;
	ILINE	uint8					GetStreamingPolicy() const { return m_streamingEnabled; }

	void								PreProcessStanceParams(SmartScriptTable pReactionTable) const;
	void								FillAllowedPartIds(const CActor& actor, const ScriptTablePtr pScriptTable, SReactionParams::SValidationParams& validationParams) const;
	ECardinalDirection	GetCardinalDirectionFromString(const char* szCardinalDirection) const;

	ILINE bool					FlagsValidateLocking(uint32 flags) const { return flags == ((eRRF_Alive | eRRF_AIEnabled) | (!gEnv->bMultiplayer * eRRF_OutFromPool)); }
	void								OnRequestAnimsTimer(void* pUserData, IGameFramework::TimerID handler);
	void								OnReleaseAnimsTimer(void* pUserData, IGameFramework::TimerID handler);


	ProfilesContainer 							m_reactionProfiles;
	SFailSafeProfile								m_failSafeProfile;

	CCustomReactionFunctions				m_customReactionFunctions;

	mutable FileToScriptTableMap		m_reactionsScriptTableCache;

	mutable CMTRand_int32						m_pseudoRandom;

	uint8														m_streamingEnabled;

#ifndef _RELEASE
	std::map<ProfileId, string>			m_profileIdToReactionFileMap; // debug attribute

	class CHitDeathReactionsDebugWidget;
	CHitDeathReactionsDebugWidget*	m_pWidget;
#endif // #ifndef _RELEASE
};

#endif // __HIT_DEATH_REACTIONS_SYSTEM_H
