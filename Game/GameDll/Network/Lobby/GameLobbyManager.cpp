#include "StdAfx.h"

#include "Game.h"
#include "GameLobbyManager.h"
//#include "PersistantStats.h"
#include "GameLobby.h"
//#include "FrontEnd/FlashFrontEnd.h"
//#include "FrontEnd/WarningsManager.h"

#include "GameCVars.h"
#include "Network/Squad/SquadManager.h"

#include "IPlayerProfiles.h"
//#include "Frontend/Multiplayer/MPMenuhub.h"

#include "MatchMakingHandler.h"

CGameLobbyManager::CGameLobbyManager() : REGISTER_GAME_MECHANISM(CGameLobbyManager)
{
	m_primaryLobby = new CGameLobby(this);
	m_nextLobby = NULL;

	for (int i=0; i<MAX_LOCAL_USERS; ++i)
	{
		m_onlineState[i] = eOS_SignedOut; // PS3 doesn't initialize with a signed out event so start everything signed out.
	}

	m_multiplayer = false;
	
	m_pendingPrimarySessionDelete = false;
	m_pendingNextSessionDelete = false;
	
	m_isCableConnected = true;
	m_isChatRestricted = false;
	m_bMergingIsComplete = false;

	m_signOutTaskID= CryLobbyInvalidTaskID;

	m_pMatchMakingHandler = new CMatchMakingHandler();

	if (m_pMatchMakingHandler->LoadScript())
		m_pMatchMakingHandler->OnInit( this );	

	gEnv->pNetwork->GetLobby()->RegisterEventInterest(eCLSE_OnlineState, CGameLobbyManager::OnlineCallback, this);
	gEnv->pNetwork->GetLobby()->RegisterEventInterest(eCLSE_EthernetState, CGameLobbyManager::EthernetStateCallback, this);
	gEnv->pNetwork->GetLobby()->RegisterEventInterest(eCLSE_ChatRestricted, CGameLobbyManager::ChatRestrictedCallback, this);
}

CGameLobbyManager::~CGameLobbyManager()
{
	gEnv->pNetwork->GetLobby()->UnregisterEventInterest(eCLSE_OnlineState, CGameLobbyManager::OnlineCallback, this);
	gEnv->pNetwork->GetLobby()->UnregisterEventInterest(eCLSE_EthernetState, CGameLobbyManager::EthernetStateCallback, this);
	gEnv->pNetwork->GetLobby()->UnregisterEventInterest(eCLSE_ChatRestricted, CGameLobbyManager::ChatRestrictedCallback, this);

	SAFE_DELETE(m_primaryLobby);
	SAFE_DELETE(m_nextLobby);
	SAFE_DELETE(m_pMatchMakingHandler);
}

CGameLobby* CGameLobbyManager::GetGameLobby() const
{
	CRY_ASSERT(m_primaryLobby);
	return m_primaryLobby;
}

CGameLobby* CGameLobbyManager::GetNextGameLobby() const
{
	return m_nextLobby;
}

bool CGameLobbyManager::IsPrimarySession(CGameLobby *pLobby)
{
	return pLobby == m_primaryLobby;
}

bool CGameLobbyManager::IsNewSession(CGameLobby *pLobby)
{
	return pLobby == m_nextLobby;
}

//when a session host wants to merge sessions it requests to join another one
bool CGameLobbyManager::NewSessionRequest(CGameLobby* pLobby, CrySessionID sessionId)
{
	if(m_nextLobby == NULL)
	{
		m_bMergingIsComplete = false;
		CRY_ASSERT(pLobby == m_primaryLobby);
		m_nextLobby = new CGameLobby(this);
		m_nextLobby->JoinServer(sessionId, "", CryMatchMakingInvalidConnectionUID, false);
		m_nextLobby->SetMatchmakingGame(true);
		CryLogAlways("CGameLobbyManager::NewSessionRequest Success");
		return true;
	}

	CryLogAlways("CGameLobbyManager::NewSessionRequest Failed");
	return false;
}

//Next lobby joins (and reserves slots for everyone)
void CGameLobbyManager::NewSessionResponse(CGameLobby* pLobby, CrySessionID sessionId)
{
	CryLogAlways("CGameLobbyManager::NewSessionResponse %d", sessionId != CrySessionInvalidID);

	CRY_ASSERT(pLobby == m_nextLobby);
	CRY_ASSERT(m_primaryLobby && m_nextLobby);

	if(sessionId != CrySessionInvalidID)
	{
		m_bMergingIsComplete = true;
		CompleteMerge(sessionId);
	}
	else
	{
		m_nextLobby->LeaveSession(true);
	}
}

void CGameLobbyManager::CompleteMerge(CrySessionID sessionId)
{
	CryLog("CGameLobbyManager::CompleteMerge()");
	m_primaryLobby->FindGameMoveSession(sessionId);
}

//Hosted session doesn't want to merge anymore (received new players) so cancels the switch
void CGameLobbyManager::CancelMoveSession(CGameLobby* pLobby)
{
	if (m_nextLobby)
	{
		CryLogAlways("CGameLobbyManager::CancelMoveSession");
		CRY_ASSERT(pLobby == m_primaryLobby);
		m_nextLobby->LeaveSession(true);
		m_bMergingIsComplete = false;
	}
}

//When a game lobby session deletes it tells the manager (this)
void CGameLobbyManager::DeletedSession(CGameLobby* pLobby)
{
	CryLog("CGameLobbyManager::DeletedSession() pLobby:%p, primaryLobby:%p, nextLobby:%p", pLobby, m_primaryLobby, m_nextLobby);
	CRY_ASSERT(m_primaryLobby);
	CRY_ASSERT(pLobby == m_primaryLobby || pLobby == m_nextLobby);
	CRY_ASSERT(m_primaryLobby != m_nextLobby);

	// Can't delete the lobby now because we're in the middle of updating it, do the delete a bit later!
	if (pLobby == m_primaryLobby)
	{
		m_pendingPrimarySessionDelete = true;
	}
	else if (pLobby == m_nextLobby)
	{
		m_pendingNextSessionDelete = true;
	}
}

void CGameLobbyManager::DoPendingDeleteSession(CGameLobby *pLobby)
{
	CryLog("CGameLobbyManager::DoPendingDeleteSession() pLobby:%p", pLobby);

	if(pLobby == m_primaryLobby)
	{
		if(m_nextLobby)
		{
			CRY_ASSERT(m_primaryLobby && m_nextLobby);

			SAFE_DELETE(m_primaryLobby);
			m_primaryLobby = m_nextLobby;
			m_nextLobby = NULL;

			m_primaryLobby->SwitchToPrimaryLobby();

			SetPrivateGame(m_primaryLobby, m_primaryLobby->IsPrivateGame());

			CryLog("CGameLobbyManager::DoPendingDeleteSession - Moved to next session");
		}
		else
		{
			CryLog("CGameLobbyManager::DoPendingDeleteSession - No sessions left");
#ifdef USE_C2_FRONTEND
			CFlashFrontEnd *pFlashMenu = g_pGame->GetFlashMenu();
			if (pFlashMenu)
			{
				// Have to go all the way back to main then forward to play_online because the stack may
				// not include play_online (destroyed when we do a level rotation)
				if (IsMultiplayer() && pFlashMenu->IsScreenInStack("game_lobby"))
				{
					if (CMPMenuHub *pMPMenu = CMPMenuHub::GetMPMenuHub())
					{
						pMPMenu->GoToCurrentLobbyServiceScreen(); // go to correct lobby service screen - play_online or play_lan
					}
				}
			}
#endif //#ifdef USE_C2_FRONTEND
			CSquadManager *pSquadManager = g_pGame->GetSquadManager();
			if (pSquadManager)
			{
				pSquadManager->GameSessionIdChanged(CSquadManager::eGSC_LeftSession, CrySessionInvalidID);
			}

			SetPrivateGame(NULL, false);
		}
	}
	else if(pLobby == m_nextLobby)
	{
		CryLog("CGameLobbyManager::DoPendingDeleteSession - Next Lobby deleted");
		SAFE_DELETE(m_nextLobby);
	}

	m_bMergingIsComplete = false;
}

//Handles online state changes - when you sign out it returns you
void CGameLobbyManager::OnlineCallback(UCryLobbyEventData eventData, void *arg)
{
	if (g_pGameCVars->g_ProcessOnlineCallbacks == 0)
		return;

	if(eventData.pOnlineStateData)
	{
		CGameLobbyManager *pLobbyManager = static_cast<CGameLobbyManager*>(arg);
		CRY_ASSERT(eventData.pOnlineStateData->m_user < MAX_LOCAL_USERS);
		pLobbyManager->m_onlineState[eventData.pOnlineStateData->m_user] = eventData.pOnlineStateData->m_curState;

		IPlayerProfileManager *pPlayerProfileManager = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager();
		uint32 userIndex = pPlayerProfileManager ? pPlayerProfileManager->GetExclusiveControllerDeviceIndex() : 0;

#ifndef _RELEASE
		const char *pEventType = "eOS_Unknown";
		if (eventData.pOnlineStateData->m_curState == eOS_SignedOut)
		{
			pEventType = "eOS_SignedOut";
		}
		else if (eventData.pOnlineStateData->m_curState == eOS_SigningIn)
		{
			pEventType = "eOS_SigningIn";
		}
		else if (eventData.pOnlineStateData->m_curState == eOS_SignedIn)
		{
			pEventType = "eOS_SignedIn";
		}
		CryLog("[GameLobbyManager] OnlineCallback: eventType=%s, user=%u, currentUser=%u", pEventType, eventData.pOnlineStateData->m_user, userIndex);

		ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
		if (g_pGameCVars->autotest_enabled && pLobby != NULL && (pLobby->GetLobbyServiceType() == eCLS_LAN))
		{
			// Don't care about signing out if we're in the autotester and in LAN mode
			return;
		}
#endif




		{
			EOnlineState onlineState = eventData.pOnlineStateData->m_curState;
			if(onlineState == eOS_SignedOut)
			{
				if(eventData.pOnlineStateData->m_reason != eCLE_CyclingForInvite)
				{
#ifdef USE_C2_FRONTEND
					CFlashFrontEnd *pFlashFrontEnd = g_pGame->GetFlashMenu();
					CMPMenuHub *pMPMenuHub = pFlashFrontEnd ? pFlashFrontEnd->GetMPMenu() : NULL;

					if(pMPMenuHub)
					{
#if INCLUDE_DEDICATED_LEADERBOARDS
						pFlashFrontEnd->ClearDelaySessionLeave();
#endif
						pMPMenuHub->OnlineStateChanged(eventData.pOnlineStateData->m_curState, eventData.pOnlineStateData->m_reason, eventData.pOnlineStateData->m_serviceConnected);
					}
#endif //#ifdef USE_C2_FRONTEND

#if !defined(XENON) && !defined(PS3)
					IPlayerProfileManager *pPPM = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager();
					if(pPPM)
					{
						pPPM->ClearOnlineAttributes();
					}
#endif

#ifdef GAME_IS_CRYSIS2
					CWarningsManager *pWM = g_pGame->GetWarnings();
					if(pWM)
					{
						pWM->RemoveWarning("ChatRestricted");
					}
#endif
				}

				pLobbyManager->LeaveGameSession(eLSR_SignedOut);
			}					
		}
	}
}

void CGameLobbyManager::EthernetStateCallback(UCryLobbyEventData eventData, void *arg)
{
	if(eventData.pEthernetStateData)
	{
		CryLog("[GameLobbyManager] EthernetStateCallback state %d", eventData.pEthernetStateData->m_curState);

		CGameLobbyManager *pGameLobbyManager = (CGameLobbyManager*)arg;
		CRY_ASSERT(pGameLobbyManager);

#ifdef GAME_IS_CRYSIS2
		if(g_pGame->HasExclusiveControllerIndex())
		{
#ifdef USE_C2_FRONTEND
			CFlashFrontEnd *pFlashFrontEnd = g_pGame->GetFlashMenu();
			CMPMenuHub *pMPMenuHub = pFlashFrontEnd ? pFlashFrontEnd->GetMPMenu() : NULL;
#endif //#ifdef USE_C2_FRONTEND
			ECableState newState = eventData.pEthernetStateData->m_curState;

#ifdef USE_C2_FRONTEND
			if(pMPMenuHub)
			{
				pMPMenuHub->EthernetStateChanged(newState);
			}
#endif //#ifdef USE_C2_FRONTEND

			// cable has been removed, clear dialog
			if(newState == eCS_Unplugged || newState == eCS_Disconnected)
			{
#ifdef GAME_IS_CRYSIS2
				CWarningsManager *pWM = g_pGame->GetWarnings();
				if(pWM)
				{
					pWM->RemoveWarning("ChatRestricted");
				}
#endif
			}
		}
#endif

		pGameLobbyManager->m_isCableConnected = (eventData.pEthernetStateData->m_curState == eCS_Connected) ? true : false;
	}
}

void CGameLobbyManager::ChatRestrictedCallback(UCryLobbyEventData eventData, void *arg)
{
	SCryLobbyChatRestrictedData *pChatRestrictedData = eventData.pChatRestrictedData;
	if(pChatRestrictedData)
	{
		CryLog("[GameLobbyManager] ChatRestrictedCallback user %d isChatRestricted %d", pChatRestrictedData->m_user, pChatRestrictedData->m_chatRestricted);

		CGameLobbyManager *pGameLobbyManager = (CGameLobbyManager*)arg;
		CRY_ASSERT(pGameLobbyManager);
	
		IPlayerProfileManager *pPlayerProfileManager = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager();
		uint32 userIndex = pPlayerProfileManager ? pPlayerProfileManager->GetExclusiveControllerDeviceIndex() : 0;

		if(pChatRestrictedData->m_user == userIndex)
		{
			pGameLobbyManager->m_isChatRestricted = pChatRestrictedData->m_chatRestricted;
		}
	}
}

void CGameLobbyManager::Update( float dt )
{
	if (m_primaryLobby)
	{
		m_primaryLobby->Update(dt);
	}
	if (m_nextLobby)
	{
		m_nextLobby->Update(dt);
	}

	m_pMatchMakingHandler->Update();

	// Do these in reverse order because the next lobby become the primary lobby if the current primary is deleted
	if (m_pendingNextSessionDelete)
	{
		DoPendingDeleteSession(m_nextLobby);
		m_pendingNextSessionDelete = false;
	}
	if (m_pendingPrimarySessionDelete)
	{
		if (m_primaryLobby->GetState() == eLS_None)
		{
			DoPendingDeleteSession(m_primaryLobby);
		}
		else
		{
			CryLog("CGameLobbyManager::Update() primary lobby deletion requested but it's no longer in the eLS_None state, aborting the delete");
		}
		m_pendingPrimarySessionDelete = false;
	}
}

//-------------------------------------------------------------------------
void CGameLobbyManager::LeaveGameSession(ELeavingSessionReason reason)
{
	CryLog("CGameLobbyManager::LeaveGameSession() reason=%i multiplayer %d", (int) reason, gEnv->bMultiplayer);
	// Tell the game lobby that we want to quit
	// Note: If we're leaving in a group (because the squad is quitting) then we need to do special behaviour:
	//					If we're the squad session host then we need to tell the rest of the squad to quit
	//					If we're the game session host then the rest of the squad has to quit before we do

	if(gEnv->bMultiplayer || reason == eLSR_SwitchGameType)
	{
		if (IsLobbyMerging())
		{
			CancelMoveSession(m_primaryLobby);
		}

		bool canLeaveNow = true;
		CSquadManager *pSquadManager = g_pGame->GetSquadManager();

		ELobbyState lobbyState = m_primaryLobby->GetState();
		if (lobbyState != eLS_Game)
		{
			// If we're in a squad and we're the game session host then we need to be careful about leaving
			if (pSquadManager != NULL && pSquadManager->HaveSquadMates())
			{
				if (reason == eLSR_Menu)
				{
					// User requested quit
					//	- If we're not the squad session host then we're the only person leaving so we can do it straight away
					//	- If we're not the game session host then there are no ordering issues so again, we can just leave
					if (pSquadManager->InCharge())
					{
						if (m_primaryLobby->IsServer())
						{
							CryLog("  we're trying to leave but we're the squad and game host, need to leave last");
							pSquadManager->TellMembersToLeaveGameSession();
							canLeaveNow = false;
						}
						else
						{
							CryLog("  we're not the game host, can leave now");
							pSquadManager->TellMembersToLeaveGameSession();
						}
					}
				}
				else if (reason == eLSR_ReceivedSquadLeavingFromSquadHost)
				{
					// Squad leader requested quit
					if (m_primaryLobby->IsServer())
					{
						canLeaveNow = false;
					}
				}
				else if (reason == eLSR_ReceivedSquadLeavingFromGameHost)
				{
					pSquadManager->TellMembersToLeaveGameSession();
				}
			}
		}
		else
		{
			CryLog("  leaving a game that is not in the lobby state, tell the squad manager");
			pSquadManager->LeftGameSessionInProgress();
		}

		if (canLeaveNow)
		{
			m_primaryLobby->LeaveSession(true);
		}
		else
		{
			m_primaryLobby->LeaveAfterSquadMembers();
		}
	}
}

//-------------------------------------------------------------------------
bool CGameLobbyManager::HaveActiveLobby(bool includeLeaving/*=true*/) const
{
	if (m_nextLobby)
	{
		return true;
	}
	else if (m_primaryLobby)
	{
		if ((includeLeaving) || (m_primaryLobby->GetState() != eLS_Leaving))
		{
			if (m_primaryLobby->GetState() != eLS_None)
			{
				return true;
			}
		}
	}
	return false;
}

//-------------------------------------------------------------------------
void CGameLobbyManager::MoveUsers(CGameLobby *pFromLobby)
{
	CryLog("[GameLobbyManager] MoveUsers pFromLobby %p pToLobby %p", pFromLobby, m_nextLobby);

	if(m_nextLobby)
	{
		m_nextLobby->MoveUsers(pFromLobby);
	}
}

//------------------------------------------------------------------------
void CGameLobbyManager::SetMultiplayer(const bool multiplayer)
{
	m_multiplayer = multiplayer;
	if(!multiplayer)
	{
		SetPrivateGame(NULL, false);	//can't be a private game anymore
	}
}

//------------------------------------------------------------------------
void CGameLobbyManager::AddPrivateGameListener(IPrivateGameListener* pListener)
{
	CRY_ASSERT(pListener);

	stl::push_back_unique(m_privateGameListeners, pListener);

	bool privateGame = m_primaryLobby ? m_primaryLobby->IsPrivateGame() : false;
	pListener->SetPrivateGame(privateGame);
}

//------------------------------------------------------------------------
void CGameLobbyManager::RemovePrivateGameListener(IPrivateGameListener* pListener)
{
	stl::find_and_erase(m_privateGameListeners, pListener);
}

//------------------------------------------------------------------------
void CGameLobbyManager::SetPrivateGame(CGameLobby *pLobby, const bool privateGame)
{
#ifndef _RELEASE
	if(privateGame)
	{
		CRY_ASSERT_MESSAGE(IsPrimarySession(pLobby), "PrivateGame logic is broken!");
	}
#endif

	if(pLobby == NULL || IsPrimarySession(pLobby))
	{
		if(m_privateGameListeners.size())
		{
			TPrivateGameListenerVec::iterator iter = m_privateGameListeners.begin();
			while (iter != m_privateGameListeners.end())
			{
				(*iter)->SetPrivateGame(privateGame);
				++iter;
			}
		}
	}
}

ECryLobbyError CGameLobbyManager::DoUserSignOut()
{
	CryLog("CGameLobbyManager::DoUserSignOut");
	ECryLobbyError error = eCLE_Success;

	if(m_signOutTaskID == CryLobbyInvalidTaskID)
	{
		ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
		ICryLobbyService *pLobbyService = pLobby ? pLobby->GetLobbyService() : NULL;

		if(pLobbyService)
		{
#ifdef GAME_IS_CRYSIS2
			error = pLobbyService->UserSignOut(g_pGame->GetExclusiveControllerDeviceIndex(), &m_signOutTaskID, UserSignoutCallback, this);
#endif
		}
	}
	else
	{
		CryLog("  not starting signout task as we already have one in progress");
	}

	return error;
}

// static
void CGameLobbyManager::UserSignoutCallback(CryLobbyTaskID taskID, ECryLobbyError error, void *pArg)
{
	CryLog("UserSignoutCallback error %d", error);

	CGameLobbyManager *pLobbyMgr =(CGameLobbyManager*)pArg;

	if(pLobbyMgr && pLobbyMgr->m_signOutTaskID == taskID)
	{
		pLobbyMgr->m_signOutTaskID = CryLobbyInvalidTaskID;
	}
}
