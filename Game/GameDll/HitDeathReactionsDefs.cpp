#include "StdAfx.h"

#include "HitDeathReactionsDefs.h"
#include "HitDeathReactionsSystem.h"
#include "GameCVars.h"
#include <NameCRCHelper.h>
#include "Game.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
SRandomGeneratorFunct::SRandomGeneratorFunct(CMTRand_int32& pseudoRandomGenerator) : m_pseudoRandomGenerator(pseudoRandomGenerator) {} 

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void SReactionParams::GetMemoryUsage(ICrySizer* s) const
{
	s->AddObject(this, sizeof(*this));

	s->AddObject(validationParams);

	s->AddObject(sCustomExecutionFunc);
	s->AddObject(sCustomAISignal);

	//agReaction size
	s->AddObject(&agReaction, sizeof(agReaction));
	s->AddObject(agReaction.sAGInputValue);
	s->AddObject(agReaction.variations);

	// reactionAnim size
	s->AddObject(reactionAnim.get(), sizeof(reactionAnim));
	s->AddObject(reactionAnim->animCRCs);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
SReactionParams::SReactionParams() : orientationSnapAngle(0), reactionOnCollision(NO_COLLISION_REACTION), 
flags(0), bPauseAI(true), endVelocity(ZERO)
{

}

///////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void SReactionParams::Reset()
{
	validationParams.clear();

	// Execution params
	sCustomExecutionFunc.clear();
	sCustomAISignal.clear();
	orientationSnapAngle = 0;
	reactionOnCollision = NO_COLLISION_REACTION;
	flags = 0;
	bPauseAI = true;
	agReaction.Reset();
	if (reactionAnim != NULL)
		reactionAnim->Reset();
	reactionScriptTable = ScriptTablePtr();
	endVelocity.zero();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
SReactionParams::SValidationParams::SValidationParams() : fMinimumSpeedAllowed(0.0f), fMaximumSpeedAllowed(FLT_MAX), 
fMinimumDamageAllowed(0.0f), fMaximumDamageAllowed(FLT_MAX), fProbability(1.0f), 
shotOrigin(eCD_Invalid), movementDir(eCD_Invalid), bAllowOnlyWhenUsingMountedItems(false), destructibleEvent(0),
fMinimumDistance(0.0f), fMaximumDistance(0.0f) 
{

}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void SReactionParams::SValidationParams::Reset()
{
	sCustomValidationFunc.clear();
	fMinimumSpeedAllowed = 0.0f;
	fMaximumSpeedAllowed = std::numeric_limits<float>::max();
	fMinimumDamageAllowed = 0.0f;
	fMaximumDamageAllowed = std::numeric_limits<float>::max();
	healthThresholds.clear();
	fMinimumDistance = 0.0f;
	fMaximumDistance = 0.0f;
	allowedPartIds.clear();
	shotOrigin = eCD_Invalid; 
	movementDir = eCD_Invalid;
	fProbability = 1.0f;
	allowedStances.clear();
	allowedHitTypes.clear();
	allowedProjectiles.clear();
	allowedWeapons.clear();
	bAllowOnlyWhenUsingMountedItems = false;
	destructibleEvent = 0;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void SReactionParams::SValidationParams::GetMemoryUsage(ICrySizer * s) const
{
	s->AddObject(sCustomValidationFunc);
	s->AddObject(allowedPartIds);
	s->AddContainer(allowedStances);
	s->AddObject(allowedHitTypes);
	s->AddObject(allowedProjectiles);
	s->AddObject(allowedWeapons);
	s->AddObject(healthThresholds);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
SReactionParams::SReactionAnim::SReactionAnim()
{
	Reset();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
SReactionParams::SReactionAnim::~SReactionAnim()
{
	// Release reference to current and requested assets on removal
	ReleaseRequestedAnims();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void SReactionParams::SReactionAnim::Reset()
{
	animCRCs.clear();
	iLayer = 0;
	animFlags = DEFAULT_REACTION_ANIM_FLAGS;
	bAdditive = false;
	bNoAnimCamera = false;
	fOverrideTransTimeToAG = -1.0f;

	m_iNextAnimIndex = -1;
	m_nextAnimCRC = 0;
	m_iTimerHandle = 0;
	m_requestedAnimCRC = 0;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
int	SReactionParams::SReactionAnim::GetNextReactionAnimIndex() const
{
	// Lazily check if the requested asset has already been loaded
	UpdateRequestedAnimStatus();

	return m_iNextAnimIndex; // This will be -1 if the first asset hasn't been loaded yet
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
int	SReactionParams::SReactionAnim::GetNextReactionAnimId(const IAnimationSet* pAnimSet) const
{
	CRY_ASSERT(pAnimSet);

	const int iNextAnimIndex = GetNextReactionAnimIndex();

#ifndef _RELEASE
	// Paranoid check to catch logic errors (or CryAnimation releasing assets we have locked)
	if (iNextAnimIndex >= 0)
	{
		const bool bIsAnimLoaded = gEnv->pCharacterManager->CAF_IsLoaded(m_nextAnimCRC);
		CRY_ASSERT_MESSAGE(bIsAnimLoaded, "This anim was expected to have finished streaming!!");
		if (!bIsAnimLoaded)
			CHitDeathReactionsSystem::Warning("%s was expected to have finished streaming!!", const_cast<IAnimationSet*>(pAnimSet)->GetNameByAnimID(pAnimSet->GetAnimIDByCRC(animCRCs[iNextAnimIndex])));
	}
#endif

	return (iNextAnimIndex >= 0) ? pAnimSet->GetAnimIDByCRC(animCRCs[iNextAnimIndex]) : -1; 
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void SReactionParams::SReactionAnim::OnAnimLoaded() const
{
	CRY_ASSERT((m_nextAnimCRC != 0) || (m_iNextAnimIndex == -1) || !g_pGame->GetHitDeathReactionsSystem().IsStreamingEnabled());

	// Release previous one
	if (m_nextAnimCRC != 0)
	{
		gEnv->pCharacterManager->CAF_Release(m_nextAnimCRC);
	}

	// Requested animation has been loaded. Update index
	++m_iNextAnimIndex %= animCRCs.size();
	m_nextAnimCRC = m_requestedAnimCRC;

	// We are no longer waiting for any request
	m_requestedAnimCRC = 0;

	// Remove timer if any
	if (m_iTimerHandle)
	{
		gEnv->pGame->GetIGameFramework()->RemoveTimer(m_iTimerHandle);
		m_iTimerHandle = 0;
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void SReactionParams::SReactionAnim::RequestNextAnim(const IAnimationSet* pAnimSet) const
{
	CRY_ASSERT(pAnimSet);

	// If we are already waiting for a requested anim to load there's no need to request a new one
	if (m_requestedAnimCRC != 0)
		return;

	// Request loading of the next index
	const int iAnimCRCsSize = animCRCs.size();
	if (iAnimCRCsSize > 0)
	{
		int iIndexToRequest = 0;
		if (iAnimCRCsSize > 1)
		{
			if ((m_iNextAnimIndex + 1) >= iAnimCRCsSize)
			{
				// Randomly reshuffle animIDs vector
				// [*DavidR | 22/Sep/2010] Note: We are reusing the seed previously set on 
				// the random generator. Should be deterministic across the network.
				// This shuffling avoids playing the same animation twice in sequence
				SRandomGeneratorFunct randomFunctor(g_pGame->GetHitDeathReactionsSystem().GetRandomGenerator());
				std::random_shuffle(animCRCs.begin(), animCRCs.end() - 1, randomFunctor);
				std::iter_swap(animCRCs.begin(), animCRCs.end() - 1);

				m_iNextAnimIndex = 0;
			}

			iIndexToRequest = m_iNextAnimIndex + 1;
		}

		// Request reference
		if (g_pGame->GetHitDeathReactionsSystem().IsStreamingEnabled())
		{
			const int requestedAnimModelID = pAnimSet->GetAnimIDByCRC(animCRCs[iIndexToRequest]);
			const uint32 requestedAnimCRC = pAnimSet->GetFilePathCRCByAnimID(requestedAnimModelID);
			if (requestedAnimCRC != m_nextAnimCRC)
			{
				m_requestedAnimCRC = requestedAnimCRC;
				gEnv->pCharacterManager->CAF_AddRef(m_requestedAnimCRC);

				// Create a timer to poll when the asset ends streaming
				m_iTimerHandle = gEnv->pGame->GetIGameFramework()->AddTimer(CTimeValue(0.5f), false, functor(*this, &SReactionAnim::OnTimer), NULL);
			}
			else if (requestedAnimCRC != 0)
			{
				// Requested anim is the one we already have loaded. Immediate success :)
				m_iNextAnimIndex = iIndexToRequest;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void SReactionParams::SReactionAnim::ReleaseRequestedAnims()
{
	if (m_requestedAnimCRC != 0)
	{
		gEnv->pCharacterManager->CAF_Release(m_requestedAnimCRC);
		m_requestedAnimCRC = 0;
	}

	if (m_nextAnimCRC != 0)
	{
		gEnv->pCharacterManager->CAF_Release(m_nextAnimCRC);
		m_nextAnimCRC = 0;
	}

	// Remove the timer, since there's no need to poll for the end of the requested assets streaming
	if (m_iTimerHandle)
	{
		gEnv->pGame->GetIGameFramework()->RemoveTimer(m_iTimerHandle);
		m_iTimerHandle = 0;
	}

	m_iNextAnimIndex = -1;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void SReactionParams::SReactionAnim::OnTimer(void* pUserData, IGameFramework::TimerID handler) const
{
	CRY_ASSERT(g_pGame->GetHitDeathReactionsSystem().IsStreamingEnabled());
	CRY_ASSERT(m_requestedAnimCRC != 0);

	m_iTimerHandle = 0;

	UpdateRequestedAnimStatus();

	// If still is not loaded, wait 0.5 seconds for the next
	if (m_requestedAnimCRC != 0)
	{
		m_iTimerHandle = gEnv->pGame->GetIGameFramework()->AddTimer(CTimeValue(0.5f), false, functor(*this, &SReactionAnim::OnTimer), NULL);	
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void SReactionParams::SReactionAnim::UpdateRequestedAnimStatus() const
{	
	if (!g_pGame->GetHitDeathReactionsSystem().IsStreamingEnabled() || 
		((m_requestedAnimCRC != 0) && gEnv->pCharacterManager->CAF_IsLoaded(m_requestedAnimCRC)))
	{
		OnAnimLoaded();
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void SReactionParams::SAnimGraphReaction::Reset()
{
	sAGInputValue.clear();
	variations.clear();
}
