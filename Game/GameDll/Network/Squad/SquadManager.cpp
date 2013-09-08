/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Session handler for squads (similar to parties from other popular online shooters)

-------------------------------------------------------------------------
History:
- 05:03:2010 : Created By Ben Parbury

*************************************************************************/

#include "StdAfx.h"
#include "Game.h"
#include "SquadManager.h"

#include "TypeInfo_impl.h"

#include "Utility/CryWatch.h"
#include "Utility/StringUtils.h"
#include "Network/Lobby/GameLobby.h"
#include "Network/Lobby/GameLobbyManager.h"
#ifdef GAME_IS_CRYSIS2
#include "HUD/HUD.h"
#include "HUD/HUDCVars.h"
#include "HUD/HUDUtils.h"
#include "HUD/ControllerButtons.h"
#include "FrontEnd/FlashFrontEnd.h"
#include "FrontEnd/FrontEndDefines.h"
#include "FrontEnd/Multiplayer/MPMenuHub.h"
#include "FrontEnd/PadButtonsDisplay.h"
#include "FrontEnd/WarningsManager.h"
#include "PlayerProgression.h"
#include "DogTagProgression.h"
#include "PlaylistManager.h"
#endif
#include <IPlayerProfiles.h>

#include "Network/Lobby/GameBrowser.h"

#include "Network/GameNetworkUtils.h"
#include "GameRules.h"
#include "HUD/UIManager.h"
#include "HUD/UILobbyMP.h"

#define SQUADMGR_CREATE_SQUAD_RETRY_TIMER		10.f

static int sm_enable = 1;
static int sm_debug = 0;

static uint32 SquadManager_GetCurrentUserIndex()
{
	IPlayerProfileManager *pPlayerProfileManager = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager();
	return pPlayerProfileManager ? pPlayerProfileManager->GetExclusiveControllerDeviceIndex() : 0;
}

//---------------------------------------
CSquadManager::CSquadManager() : REGISTER_GAME_MECHANISM(CSquadManager)
{
	m_squadHandle = CrySessionInvalidHandle;
	m_currentGameSessionId = CrySessionInvalidID;
	m_requestedGameSessionId = CrySessionInvalidID;
	m_inviteSessionId = CrySessionInvalidID;
	m_nameList.Clear();
	m_squadLeader = false;
	m_isNewSquad = false;
	m_squadLeaderId = CryUserInvalidID;
	m_pendingKickUserId = CryUserInvalidID;
	m_leaderLobbyState = eLS_None;
	m_currentTaskId = CryLobbyInvalidTaskID;
	m_bMultiplayerGame = false;
	m_pendingInvite = false;
	m_bSessionStarted = false;
	m_bGameSessionStarted = false;
	m_slotType = eSST_Public;
	m_requestedSlotType = eSST_Public;
	m_inProgressSlotType = eSST_Public;
	m_sessionIsInvalid = false;
	m_leavingSession = false;

	ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
	pLobby->RegisterEventInterest(eCLSE_UserPacket, CSquadManager::UserPacketCallback, this);
	pLobby->RegisterEventInterest(eCLSE_OnlineState, CSquadManager::OnlineCallback, this);
	pLobby->RegisterEventInterest(eCLSE_SessionUserJoin, CSquadManager::JoinUserCallback, this);
	pLobby->RegisterEventInterest(eCLSE_SessionUserLeave, CSquadManager::LeaveUserCallback, this);
	pLobby->RegisterEventInterest(eCLSE_SessionUserUpdate, CSquadManager::UpdateUserCallback, this);
	pLobby->RegisterEventInterest(eCLSE_SessionClosed, CSquadManager::SessionClosedCallback, this);
	pLobby->RegisterEventInterest(eCLSE_ForcedFromRoom, CSquadManager::ForcedFromRoomCallback, this);

	gEnv->pNetwork->AddHostMigrationEventListener(this, "SquadManager");

#if !defined(_RELEASE)
	if (gEnv->pConsole)
	{
		REGISTER_CVAR(sm_enable, sm_enable, VF_CHEAT|VF_CHEAT_NOCHECK, CVARHELP("Enables and disables squad"));
		REGISTER_CVAR(sm_debug, 0, VF_CHEAT, CVARHELP("Enable squad manager debug watches and logs"));
		gEnv->pConsole->AddCommand("sm_create", CmdCreate, VF_CHEAT, CVARHELP("Create a squad session"));
		gEnv->pConsole->AddCommand("sm_leave", CmdLeave, VF_CHEAT, CVARHELP("Leave a squad session"));
		gEnv->pConsole->AddCommand("sm_kick", CmdKick, VF_CHEAT, CVARHELP("Kick a player from the squad"));
	}
#endif
	if(gEnv->IsDedicated())
	{
		sm_enable = 0;
	}

	m_taskQueue.Init(TaskStartedCallback, this);
}

//---------------------------------------
void CSquadManager::Enable(bool enable, bool allowCreate)
{
	//ignore disables when we are already disabled
	if ( enable != ( !sm_enable ) && !enable)
	{
		return;
	}

	sm_enable = enable;

	CryLog("CSquadManager::Enable() enable='%s'", enable ? "true" : "false");
	if (enable)
	{
		if(!m_pendingInvite)
		{
			if(allowCreate)
			{
				if (m_bMultiplayerGame)
				{
					if(m_taskQueue.GetCurrentTask() != CLobbyTaskQueue::eST_Create)	
					{
						if (m_squadHandle == CrySessionInvalidHandle)
						{
							ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
							if (pLobby)
							{
								const bool allowJoinMultipleSessions = pLobby->GetLobbyServiceFlag(eCLS_Online, eCLSF_AllowMultipleSessions);
								if (allowJoinMultipleSessions)	// we could 
								{
									m_taskQueue.AddTask(CLobbyTaskQueue::eST_Create, false);
								}
								else
								{
									CryLog("    online lobby service doesn't support multiple sessions");
								} // allowJoinMultipleSessions
							} // pLobby
						}
						else
						{
							CryLog("    squad already created");
						} // m_squadHandle
					} // task is not create squad
					else
					{
						CryLog("   squad create already in progress");
					}
				} // m_bMultiplayerGame
			} // allowCreate
		} // pendingInvite
	}
	else
	{
		DeleteSession();
	}
}

//---------------------------------------
CSquadManager::~CSquadManager()
{
	if (gEnv->pConsole)
	{
#if !defined(_RELEASE)
		gEnv->pConsole->UnregisterVariable("sm_enable", true);
		gEnv->pConsole->UnregisterVariable("sm_create", true);
		gEnv->pConsole->UnregisterVariable("sm_leave", true);
#endif
	}

	ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
	pLobby->UnregisterEventInterest(eCLSE_UserPacket, CSquadManager::UserPacketCallback, this);
	pLobby->UnregisterEventInterest(eCLSE_OnlineState, CSquadManager::OnlineCallback, this);
	pLobby->UnregisterEventInterest(eCLSE_SessionUserJoin, CSquadManager::JoinUserCallback, this);
	pLobby->UnregisterEventInterest(eCLSE_SessionUserLeave, CSquadManager::LeaveUserCallback, this);
	pLobby->UnregisterEventInterest(eCLSE_SessionUserUpdate, CSquadManager::UpdateUserCallback, this);
	pLobby->UnregisterEventInterest(eCLSE_SessionClosed, CSquadManager::SessionClosedCallback, this);
	pLobby->UnregisterEventInterest(eCLSE_ForcedFromRoom, CSquadManager::ForcedFromRoomCallback, this);
	
	gEnv->pNetwork->RemoveHostMigrationEventListener(this);
}

//---------------------------------------
void CSquadManager::Update(float dt)
{
	if (sm_enable)
	{
#ifndef _RELEASE
		if (sm_debug > 0)
		{
			if (InSquad())
			{
				//if(HaveSquadMates())
				{
					CryWatch("Squad");
					const int nameSize = m_nameList.Size();
					for(int i = 0; i < nameSize; i++)
					{
						CryWatch("\t%d - %s", i + 1, m_nameList.m_sessionNames[i].m_name);
					}
				}
			}
		}
#endif

		if (m_nameList.m_dirty)
		{
#ifdef USE_C2_FRONTEND
			CFlashFrontEnd *pFlashMenu = g_pGame->GetFlashMenu();
			if (pFlashMenu != NULL && pFlashMenu->IsMenuActive(CFlashFrontEnd::eFlM_Menu))
			{
				EFlashFrontEndScreens screen = pFlashMenu->GetCurrentMenuScreen();
				if ((screen == eFFES_play_online) || (screen == eFFES_choose_playlist) || (screen == eFFES_choose_variant))
				{
					bool compactVersion = (screen == eFFES_choose_playlist) || (screen == eFFES_choose_variant);
					m_nameList.m_dirty = false;
					SendSquadMembersToFlash(pFlashMenu->GetFlash(), compactVersion);
				}
			}
#endif //#ifdef USE_C2_FRONTEND
		}

		SPendingGameJoin *pPendingGameJoin = &m_pendingGameJoin;
		if(pPendingGameJoin->IsValid())
		{
			IGameFramework *pFramework = gEnv->pGame->GetIGameFramework();
#ifdef USE_C2_FRONTEND
			CFlashFrontEnd *pFFE = g_pGame->GetFlashMenu();
			if(!pFFE || (!pFramework->StartingGameContext() && !pFFE->IsFlashRenderingDuringLoad()))
#endif //#ifdef USE_C2_FRONTEND
			{
				CryLog("[squad]  calling delayed SquadJoinGame");
				SquadJoinGame(pPendingGameJoin->m_sessionID, pPendingGameJoin->m_isMatchmakingGame, pPendingGameJoin->m_playlistID, pPendingGameJoin->m_restrictRank, pPendingGameJoin->m_requireRank);
				pPendingGameJoin->Invalidate();
			}
		}
	}

	// Have to do this block regardless of sm_enable since we could be trying to delete the squad as a result
	// of being disabled
	m_taskQueue.Update();
}

//---------------------------------------
bool CSquadManager::IsSquadMateByUserId(CryUserID userId)
{
	bool result = false;

	if (InSquad() && userId.IsValid() && gEnv->pNetwork && gEnv->pNetwork->GetLobby())
	{
		SSessionNames *pNamesList = &m_nameList;
		const int numPlayers = pNamesList->Size();
		for (int i = 0; i < numPlayers; ++ i)
		{
			SSessionNames::SSessionName *pPlayer = &pNamesList->m_sessionNames[i];
			if (pPlayer->m_userId == userId)
			{
				result = true;
				break;
			}
		}
	}

	return result;
}

//---------------------------------------
void CSquadManager::GameSessionIdChanged(EGameSessionChange eventType, CrySessionID gameSessionId)
{
	CryLog("CSquadManager::GameSessionIdChanged() eventType=%i", (int) eventType);

	m_currentGameSessionId = gameSessionId;

	if (!HaveSquadMates())
	{
		// If we're not in an active squad (one with more than just us in) then we don't care what the game session does
		CryLog("  not in an active squad");
		return;
	}

	switch (eventType)
	{
	case eGSC_JoinedNewSession:
		{
			if (InCharge())
			{
				// Reserve slots
				CryLog("  joined a new session and we're in charge of the squad, making reservations");
				g_pGame->GetGameLobby()->MakeReservations(&m_nameList, true);
			}
		}
		break;
	case eGSC_LeftSession:
		{
			if ((!InCharge()) && (NetworkUtils_CompareCrySessionId(m_requestedGameSessionId, gameSessionId) == false))
			{
				CryLog("  left game session when we weren't expecting to, leaving the squad");
				DeleteSession();
				m_requestedGameSessionId = CrySessionInvalidID;
			}
		}
		break;
	case eGSC_LobbyMerged:
		{
			// TODO: Tell anyone who didn't make it into the original session that they need to use a different CrySessionID
		}
		break;
	case eGSC_LobbyMigrated:
		{
			// TODO: Tell anyone not in this game session that they need to use a different CrySessionID
		}
		break;
	}
}

//---------------------------------------
void CSquadManager::ReservationsFinished(EReservationResult result)
{
	CryLog("CSquadManager::ReservationsFinished() result=%i", result);

	if (result == eRR_Success)
	{
		if(SquadsSupported())
		{
			CryLog("  reservations for squad successful, sending eGUPD_SquadJoinGame packet!");
			SendSquadPacket(eGUPD_SquadJoinGame);
		}
		else
		{
			CryLog("  host is in a game that does not support squads, sending eGUPD_SquadNotSupported!");
			SendSquadPacket(eGUPD_SquadNotSupported);
		}
	}
	else
	{
		CryLog("  reservations for squad failed, deleting session");
		HandleCustomError("Disconnect", "@ui_menu_squad_error_not_enough_room", true, true);
	}
}

//---------------------------------------
void CSquadManager::SetSquadHandle(CrySessionHandle handle)
{
	CryLog("[tlh] CSquadManager::SetSquadHandle(CrySessionHandle handle = %d) previousHandle=%d", (int)handle, int(m_squadHandle));
	m_squadHandle = handle;

	m_taskQueue.AddTask(CLobbyTaskQueue::eST_SetLocalUserData, false);
}

//---------------------------------------
void CSquadManager::LocalUserDataUpdated()
{
	if (InSquad())
	{
		m_taskQueue.AddTask(CLobbyTaskQueue::eST_SetLocalUserData, false);
	}
}

//---------------------------------------
ECryLobbyError CSquadManager::DoUpdateLocalUserData(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId)
{
	CryLog("CSquadManager::DoUpdateLocalUserData()");
	CRY_ASSERT(m_taskQueue.GetCurrentTask() == CLobbyTaskQueue::eST_SetLocalUserData);

	uint8 localUserData[CRYLOBBY_USER_DATA_SIZE_IN_BYTES] = {0};
	CGameLobby::SetLocalUserData(localUserData);

	uint32 userIndex = SquadManager_GetCurrentUserIndex();

	ECryLobbyError result = pMatchMaking->SessionSetLocalUserData(m_squadHandle, &taskId, userIndex, localUserData, sizeof(localUserData), CSquadManager::UpdateLocalUserDataCallback, this);
	return result;
}

//static---------------------------------------
void CSquadManager::UpdateLocalUserDataCallback( CryLobbyTaskID taskID, ECryLobbyError error, void* arg )
{
	CryLog("CSquadManager::UpdateLocalUserDataCallback(CryLobbyTaskID taskID, ECryLobbyError error = %d, void* arg)", error);

	CSquadManager *pSquadManager = static_cast<CSquadManager*>(arg);
	CRY_ASSERT(pSquadManager);

	pSquadManager->CallbackReceived(taskID, error);
}

//---------------------------------------
void CSquadManager::JoinGameSession(CrySessionID gameSessionId, bool bIsMatchmakingSession)
{
	CryLog("CSquadManager::JoinGameSession(CrySessionID gameSessionId = %d)", (int)+gameSessionId);
	SSessionNames &playerList = m_nameList;
	if (playerList.Size() > 0)
	{
		SSessionNames::SSessionName &localPlayer = playerList.m_sessionNames[0];
		// Reservation will have been made using the squad session UID
		CGameLobby *pGameLobby = g_pGame->GetGameLobby();
		pGameLobby->SetMatchmakingGame(bIsMatchmakingSession);
		pGameLobby->JoinServer(gameSessionId, "Squad Leader", localPlayer.m_conId, false);
	}
}

//---------------------------------------
ECryLobbyError CSquadManager::DoCreateSquad(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId)
{
	CryLog("CSquadManager::CreateSquad()");

	ECryLobbyError result = eCLE_ServiceNotSupported;

	SCrySessionData session;
	session.m_data = NULL;
	session.m_numData = 0;
	session.m_numPublicSlots = SQUADMGR_MAX_SQUAD_SIZE;
	session.m_numPrivateSlots = 0;
	session.m_ranked = false;
	
	m_slotType = eSST_Public;
	m_requestedSlotType = eSST_Public;
	m_inProgressSlotType = eSST_Public;

	IPlayerProfileManager *pPlayerProfileManager = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager();
	if (pPlayerProfileManager)
	{
		cry_strncpy(session.m_name, pPlayerProfileManager->GetCurrentUser(), sizeof(session.m_name));
		uint32 userIndex = SquadManager_GetCurrentUserIndex(); //pad number
		result = pMatchMaking->SessionCreate(&userIndex, 1, CRYSESSION_CREATE_FLAG_INVITABLE | CRYSESSION_CREATE_FLAG_MIGRATABLE, &session, &taskId, CSquadManager::CreateCallback, this);
	}
	else
	{
		result = eCLE_InvalidUser;
	}

	return result;
}

//static---------------------------------------
void CSquadManager::CreateCallback(CryLobbyTaskID taskID, ECryLobbyError error, CrySessionHandle handle, void* arg)
{
	CryLog("CSquadManager::CreateCallback(CryLobbyTaskID taskID, ECryLobbyError error = %d, CrySessionHandle handle = %d, void* arg)", (int)error, (int)handle);

	CSquadManager* pSquadManager = (CSquadManager*)arg;
	CRY_ASSERT(pSquadManager);

	if (pSquadManager->CallbackReceived(taskID, error))
	{
		pSquadManager->SetSquadHandle(handle);
		pSquadManager->m_squadLeader = true;
		pSquadManager->m_isNewSquad = true;
		CryLog("CSquadManager session created %d", (int)handle);

		pSquadManager->m_taskQueue.AddTask(CLobbyTaskQueue::eST_SessionSetLocalFlags, false);

#ifdef USE_C2_FRONTEND
		if (CMPMenuHub *pMPMenus = CMPMenuHub::GetMPMenuHub())
		{
			pMPMenus->SquadCallback(CMPMenuHub::eMSC_CreatedSquad, true, pSquadManager);
		}
#endif //#ifdef USE_C2_FRONTEND

		if (pSquadManager->m_bGameSessionStarted)
		{
			CryLog("  game session has already started, starting a SessionStart task");
			pSquadManager->m_taskQueue.AddTask(CLobbyTaskQueue::eST_SessionStart, false);
		}
	
#ifdef GAME_IS_CRYSIS2
		g_pGame->RefreshRichPresence();
#endif

		// we no longer delete sessions if they are in the process of being created
		// joined, so we need to clean up after them when the task has completed
		if(!sm_enable)
		{
			CryLog("   deleting created session. squad manager is no longer enabled");

			// this will clear out the task queue for us
			pSquadManager->DeleteSession();
		}
	}
	else if(error != eCLE_TimeOut)
	{
		if(gEnv->bMultiplayer)
		{
#ifdef USE_C2_FRONTEND
			CFlashFrontEnd *pFlashMenu = g_pGame->GetFlashMenu();
			EFlashFrontEndScreens screen = pFlashMenu != NULL && pFlashMenu->IsMenuActive(CFlashFrontEnd::eFlM_Menu) ? pFlashMenu->GetCurrentMenuScreen() : eFFES_unknown;
			if(screen != eFFES_main)	// main is the mp loader, anywhere else, we need to disconnect and return the game back to singleplayer
			{
				CMPMenuHub *mpMenuHub = pFlashMenu ? pFlashMenu->GetMPMenu() : NULL;
				if (mpMenuHub)
				{
					mpMenuHub->SetDisconnectError(CMPMenuHub::eDCE_F_CannotCreateSquad, true);
				}
			}
#endif //#ifdef USE_C2_FRONTEND
		}
	}
}

//---------------------------------------
ECryLobbyError CSquadManager::DoJoinSquad(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId)
{
	CryLog("CSquadManager::JoinSquad(CrySessionID squadSessionId = %d)", (int)+m_inviteSessionId);

	uint32 userIndex = SquadManager_GetCurrentUserIndex(); //pad number
	ECryLobbyError result = pMatchMaking->SessionJoin(&userIndex, 1, CRYSESSION_CREATE_FLAG_INVITABLE, m_inviteSessionId, &taskId, CSquadManager::JoinCallback, this);

	if ((result != eCLE_Success) && (result != eCLE_TooManyTasks))
	{
		JoinSessionFinished(taskId, result, CrySessionInvalidHandle);
	}

	return result;
}

//---------------------------------------
void CSquadManager::JoinSessionFinished(CryLobbyTaskID taskID, ECryLobbyError error, CrySessionHandle hdl)
{
	if (CallbackReceived(taskID, error))
	{
		SetSquadHandle(hdl);

		m_squadLeader = false;
		m_isNewSquad = true;

		CryLog("CSquadManager session joined %d", (int)hdl);
		SendSquadPacket(eGUPD_SquadJoin);

#ifdef USE_C2_FRONTEND
		if (CMPMenuHub *pMPMenus = CMPMenuHub::GetMPMenuHub())
		{
			pMPMenus->SquadCallback(CMPMenuHub::eMSC_JoinedSquad, false, this);
		}
#endif //#ifdef USE_C2_FRONTEND

		m_inviteSessionId = CrySessionInvalidID;
	
#ifdef GAME_IS_CRYSIS2
		g_pGame->RefreshRichPresence();
#endif

		if(!sm_enable || m_sessionIsInvalid)
		{
			CryLog("   deleting joined session sm_enable %d sessionIsInvalid %d", sm_enable, m_sessionIsInvalid);
			m_sessionIsInvalid = false;
			DeleteSession();
		}
	}
	else if (error != eCLE_TimeOut)
	{
		// If we timed out then the task will be restarted, if we failed for any other reason we need to go back
		// to our own squad
		m_inviteSessionId = CrySessionInvalidID;
		m_taskQueue.AddTask(CLobbyTaskQueue::eST_Create, false);
		ReportError(error);
	}
}

//static---------------------------------------
void CSquadManager::JoinCallback(CryLobbyTaskID taskID, ECryLobbyError error, CrySessionHandle handle, uint32 ip, uint16 port, void* arg)
{
	CryLog("CSquadManager::JoinCallback(CryLobbyTaskID taskID, ECryLobbyError error, CrySessionHandle handle = %d, uint32 ip, uint16 port, void* arg)", (int)handle);

	CSquadManager* pSquadManager = (CSquadManager*)arg;
	CRY_ASSERT(pSquadManager);

	pSquadManager->m_taskQueue.AddTask(CLobbyTaskQueue::eST_SessionSetLocalFlags, false);

	pSquadManager->JoinSessionFinished(taskID, error, handle);
}

//static---------------------------------------
void CSquadManager::SessionChangeSlotTypeCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pArg)
{
	CryLog("CSquadManager::SessionUpdateSlotCallback(CryLobbyTaskID %d ECryLobbyError %d)", taskID, error);

	CSquadManager *pSquadManager = (CSquadManager*)pArg;
	pSquadManager->SessionChangeSlotTypeFinished(taskID, error);	
}

//---------------------------------------
ECryLobbyError CSquadManager::DoLeaveSquad(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId)
{
	CryLog("CSquadManager::LeaveSquad()");

	ECryLobbyError result = pMatchMaking->SessionDelete(m_squadHandle, &taskId, CSquadManager::DeleteCallback, this);

	m_pendingGameJoin.Invalidate();

	if ((result != eCLE_Success) && (result != eCLE_TooManyTasks))
	{
		SquadSessionDeleted(m_currentTaskId, result);
	}
	return result;
}

//---------------------------------------
void CSquadManager::CleanUpSession()
{
	if (m_squadHandle != CrySessionInvalidHandle)
	{
#ifdef USE_C2_FRONTEND
		if (CMPMenuHub *pMPMenus = CMPMenuHub::GetMPMenuHub())
		{
			pMPMenus->SquadCallback(CMPMenuHub::eMSC_LeftSquad, m_squadLeader, this);
		}
#endif //#ifdef USE_C2_FRONTEND
	}

	// finish off the deletion...
	m_squadHandle = CrySessionInvalidHandle;

	m_squadLeaderId = CryUserInvalidID;
	m_squadLeader = false;

	m_requestedGameSessionId = CrySessionInvalidID;
	m_bSessionStarted = false;

	CRY_ASSERT_MESSAGE(m_nameList.Size() <= 1, "[tlh] SANITY FAIL! i thought all remote connections should've been removed by the LeaveUserCallback by this point, so it should only be me in this list...? need rethink.");
	m_nameList.Clear();

	// Make sure we tell the GameLobby that we've left our current squad (need to do this after ensuring that the names list is empty
	OnSquadLeaderChanged();
}

//----------------------------------------
void CSquadManager::SquadSessionDeleted(CryLobbyTaskID taskID, ECryLobbyError error)
{
	bool wasSquadLeader = m_squadLeader;

	CallbackReceived(taskID, error);

	if (error != eCLE_TimeOut)
	{
		m_leavingSession = false;

		CleanUpSession();		// If we didn't timeout, cleanup (if something went wrong then we need to start from scratch)

#ifdef USE_C2_FRONTEND
		if (CMPMenuHub *pMPMenus = CMPMenuHub::GetMPMenuHub())
		{
			pMPMenus->SquadCallback(CMPMenuHub::eMSC_LeftSquad, !wasSquadLeader, this);
		}
#endif //#ifdef USE_C2_FRONTEND

#ifdef GAME_IS_CRYSIS2
		g_pGame->RefreshRichPresence();
#endif

		if (sm_enable)
		{
		  	if(!m_pendingInvite)
			{
					// ...now join the next session (if there's one set)
					if (m_inviteSessionId != CrySessionInvalidID)
					{
							m_taskQueue.AddTask(CLobbyTaskQueue::eST_Join, false);
					}
					else
					{
							// ...else need to recreate our own squad session - shouldn't ever be not in a squad
							m_taskQueue.AddTask(CLobbyTaskQueue::eST_Create, false);
					}
			}
		}
	}
}

//static---------------------------------------
void CSquadManager::DeleteCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* arg)
{

	CryLog("CSquadManager::DeleteCallback(CryLobbyTaskID taskID, ECryLobbyError error = %d, void* arg)", error);

	CSquadManager*  pSquadManager = (CSquadManager*) arg;
	CRY_ASSERT(pSquadManager);

	pSquadManager->SquadSessionDeleted(taskID, error);

}

//---------------------------------------
void CSquadManager::InviteAccepted(ECryLobbyService service, uint32 user, CrySessionID id)
{
	CryLog("[Invite] SquadManager::InviteAccepted service %d user %d id %d", service, user, (int)+id);

	//If we accepted an invite, we will be joining an online game (XLive) - make sure the lobbyservice is set to it
	CGameLobby::SetLobbyService(eCLS_Online);

	//Notify UI that this one is being pulled into a lobby (change menu to JoinMP)
	NOTIFY_UI_LOBBY(InviteAccepted());

	CLobbyTaskQueue::ESessionTask currentTask = m_taskQueue.GetCurrentTask();

	m_pendingInvite = false;
	m_inviteSessionId = id;

	if ((m_squadHandle == CrySessionInvalidHandle) && (!m_taskQueue.HasTaskInProgress()))
	{
		m_taskQueue.Reset();
	}
	else if(currentTask == CLobbyTaskQueue::eST_Create || currentTask == CLobbyTaskQueue::eST_Join)
	{
		CryLog("[invite] currently %s a session , adding delete task for when done", (currentTask == CLobbyTaskQueue::eST_Create) ? "creating" : "joining");
		m_taskQueue.AddTask(CLobbyTaskQueue::eST_Delete, false);
	}
	else
	{
		DeleteSession();
	}
	m_taskQueue.AddTask(CLobbyTaskQueue::eST_Join, false);
}

//static---------------------------------------
void CSquadManager::JoinUserCallback(UCryLobbyEventData eventData, void *arg)
{
	CSquadManager* pSquadManager = (CSquadManager*)arg;
	CRY_ASSERT(pSquadManager);
	if(eventData.pSessionUserData->session == pSquadManager->m_squadHandle)
	{
		pSquadManager->JoinUser(&eventData.pSessionUserData->data);
	}
}

//static---------------------------------------
void CSquadManager::LeaveUserCallback(UCryLobbyEventData eventData, void *arg)
{
	CSquadManager* pSquadManager = (CSquadManager*)arg;
	CRY_ASSERT(pSquadManager);
	if(eventData.pSessionUserData->session == pSquadManager->m_squadHandle)
	{
		pSquadManager->LeaveUser(&eventData.pSessionUserData->data);
	}
}

//static---------------------------------------
void CSquadManager::UpdateUserCallback(UCryLobbyEventData eventData, void *arg)
{
	CSquadManager* pSquadManager = (CSquadManager*)arg;
	CRY_ASSERT(pSquadManager);
	if(eventData.pSessionUserData->session == pSquadManager->m_squadHandle)
	{
		pSquadManager->UpdateUser(&eventData.pSessionUserData->data);
	}
}

//static---------------------------------------
void CSquadManager::OnlineCallback(UCryLobbyEventData eventData, void *arg)
{
	CryLog("CSquadManager::OnlineCallback(UCryLobbyEventData eventData, void *arg)");

	if(eventData.pOnlineStateData)
	{
		CSquadManager* pSquadManager = (CSquadManager*)arg;
		CRY_ASSERT(pSquadManager);

		const EOnlineState onlineState = eventData.pOnlineStateData->m_curState;
		uint32 user = eventData.pOnlineStateData->m_user;

		CryLog("    onlineState=%i, currentTask=%i", int(onlineState), int(pSquadManager->m_taskQueue.GetCurrentTask()));

		if(user == SquadManager_GetCurrentUserIndex())	// only do this for the game user
		{
			if (onlineState == eOS_SignedIn)
			{
				ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
				bool allowSquadCreate = (pSquadManager->m_bMultiplayerGame) && (sm_enable) && (pSquadManager->m_squadHandle == CrySessionInvalidHandle) && (pLobby && pLobby->GetLobbyServiceFlag(eCLS_Online, eCLSF_AllowMultipleSessions));

				// ensure we can actually create a squad at this point in time
				if (allowSquadCreate)
				{
					CryLog("    finished signing in, calling CreateSquad()");
					pSquadManager->m_taskQueue.AddTask(CLobbyTaskQueue::eST_Create, false);
				}
			}
			else if (onlineState == eOS_SignedOut)
			{
				CryLog("SquadManager: onlineState: eOS_SignedOut, Deleting and disabling");
				pSquadManager->DeleteSession();
				pSquadManager->Enable(false, false);
			}
		}
	}
}

//static--------------------------------------------
void CSquadManager::UpdateOfflineState(CSquadManager *pSquadManager)
{
	CryLog("[SquadManager] UpdateOflineState");

	if (pSquadManager->m_squadHandle != CrySessionInvalidHandle)
	{
			CryLog("    signed out and not currently in a task");
			pSquadManager->CleanUpSession();
	}
}

//static---------------------------------------
void CSquadManager::UserPacketCallback(UCryLobbyEventData eventData, void *userParam)
{
	if(!sm_enable)
		return;

	CryLog("[tlh] CSquadManager::UserPacketCallback(UCryLobbyEventData, void)");

	if(eventData.pUserPacketData)
	{
		CSquadManager* pSquadManager = (CSquadManager*)userParam;
		CRY_ASSERT(pSquadManager);

		if (eventData.pUserPacketData->session != CrySessionInvalidHandle)
		{
			const CrySessionHandle  lobbySessionHandle = g_pGame->GetGameLobby()->GetCurrentSessionHandle();

			if (eventData.pUserPacketData->session == pSquadManager->m_squadHandle)
			{
				pSquadManager->ReadSquadPacket(&eventData.pUserPacketData);
			}
		}
		else
		{
			CryLog("[tlh]   err: session handle in packet data is INVALID");
		}
	}
	else
	{
		CryLog("[tlh]   err: packet data is NULL");
	}
}

//---------------------------------------
void CSquadManager::SendSquadPacket(GameUserPacketDefinitions packetType, SCryMatchMakingConnectionUID connectionUID /*=CryMatchMakingInvalidConnectionUID*/)
{
	CryLog("[tlh] CSquadManager::SendSquadPacket(packetType = '%d', ...)", packetType);

	ICryLobbyService *pLobbyService = gEnv->pNetwork->GetLobby()->GetLobbyService(eCLS_Online);
	if (!pLobbyService)
	{
		CryLog("    failed to find online lobby service");
		return;
	}
	ICryMatchMaking *pMatchmaking = pLobbyService->GetMatchMaking();

	CCryLobbyPacket packet;

	switch(packetType)
	{
	case eGUPD_SquadJoin:
		{
			CryLog("[tlh]   sending packet of type 'eGUPD_SquadJoin'");
			CRY_ASSERT(!m_squadLeader);

			const uint32 MaxBufferSize = CryLobbyPacketHeaderSize + CryLobbyPacketUINT32Size;
			if (packet.CreateWriteBuffer(MaxBufferSize))
			{
				packet.StartWrite(packetType, true);
				packet.WriteUINT32(GameLobbyData::GetVersion());
			}
		}
		break;
	case eGUPD_SquadJoinGame:
		{
			CryLog("[tlh]   sending packet of type 'eGUPD_SquadJoinGame'");
			CRY_ASSERT(m_squadLeader);

			uint32 passwordTransportSize = 0;
#if USE_CRYLOBBY_GAMESPY
			uint32 passwordLength = 0;
			ICVar* pPasswordCVar = gEnv->pConsole->GetCVar("sv_password");
			if (pPasswordCVar != NULL)
			{
				passwordLength = strlen(pPasswordCVar->GetString());
			}

			passwordTransportSize = CryLobbyPacketUINT8Size + passwordLength;
#endif // USE_CRYLOBBY_GAMESPY

			const uint32 MaxBufferSize = CryLobbyPacketHeaderSize + pMatchmaking->GetSessionIDSizeInPacket() + CryLobbyPacketBoolSize + CryLobbyPacketUINT32Size + CryLobbyPacketUINT32Size + CryLobbyPacketUINT32Size + passwordTransportSize;
			if (packet.CreateWriteBuffer(MaxBufferSize))
			{
				packet.StartWrite(packetType, true);
				ECryLobbyError error = pMatchmaking->WriteSessionIDToPacket(m_currentGameSessionId, &packet);

				bool bIsMatchmakingGame = false;

				CGameLobby *pGameLobby = g_pGame->GetGameLobby();
				if (pGameLobby)
				{
					bIsMatchmakingGame = pGameLobby->IsMatchmakingGame();
				}

				packet.WriteBool(bIsMatchmakingGame);

				uint32 playlistId = GameLobbyData::GetPlaylistId();
				packet.WriteUINT32(playlistId);

#ifdef GAME_IS_CRYSIS2
				CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
#endif

				int restrictRank = 0;
				int requireRank = 0;

#ifdef GAME_IS_CRYSIS2

				if(pPlaylistManager)
				{
					const int activeVariant = pPlaylistManager->GetActiveVariantIndex();
					const SGameVariant *pGameVariant = (activeVariant >= 0) ? pPlaylistManager->GetVariant(activeVariant) : NULL;
					restrictRank = pGameVariant ? pGameVariant->m_restrictRank : 0;
					requireRank = pGameVariant ? pGameVariant->m_requireRank : 0;
				}
#endif

				packet.WriteUINT32(restrictRank);
				packet.WriteUINT32(requireRank);

#if USE_CRYLOBBY_GAMESPY
				packet.WriteUINT8(passwordLength);
				packet.WriteString(pPasswordCVar->GetString(), passwordLength + 1);
				CryLog("[Password]: sending eGUPD_SquadJoinGame with password '%s'", pPasswordCVar->GetString());
#endif // USE_CRYLOBBY_GAMESPY

				CRY_ASSERT(error == eCLE_Success);
			}
		}
		break;
	case eGUPD_SquadLeaveGame:
		{
			CryLog("CSquadManager::SendSquadPacket() sending packet of type 'eGUPD_SquadLeaveGame'");
			m_requestedGameSessionId = CrySessionInvalidID;

			const uint32 MaxBufferSize = CryLobbyPacketHeaderSize;
			if (packet.CreateWriteBuffer(MaxBufferSize))
			{
				packet.StartWrite(packetType, true);
			}
		}
		break;
	case eGUPD_SquadNotSupported:
		{
			CryLog("CSquadManager::SendSquadPacket sending packet of type 'eGUPD_SquadNotSupported'");
			CRY_ASSERT(m_squadLeader);

			const uint32 MaxBufferSize = CryLobbyPacketHeaderSize;
			if (packet.CreateWriteBuffer(MaxBufferSize))
			{
				packet.StartWrite(packetType, true);
			}

			break;
		}
		case eGUPD_SquadNotInGame:
		{
			CryLog("CSquadManager::SendSquadPacket sending packet of type 'eGUPD_SquadNotInGame'");
			const uint32 MaxBufferSize = CryLobbyPacketHeaderSize;
			if (packet.CreateWriteBuffer(MaxBufferSize))
			{
				packet.StartWrite(packetType, true);
			}
			break;
		}
		case eGUPD_SquadDifferentVersion:
		{
			CryLog("CSquadManager::SendSquadPacket sending packet of type 'eGUPD_SquadDifferentVersion'");
			const uint32 MaxBufferSize = CryLobbyPacketHeaderSize;
			if (packet.CreateWriteBuffer(MaxBufferSize))
			{
				packet.StartWrite(packetType, true);
			}
			break;
		}
		case eGUPD_SquadKick:
		{
			CryLog("CSquadManager::SendSquadPacket sending packet of type 'eGUPD_SquadKick'");
			const uint32 MaxBufferSize = CryLobbyPacketHeaderSize;
			if (packet.CreateWriteBuffer(MaxBufferSize))
			{
				packet.StartWrite(packetType, true);
			}
			break;
		}
	}

	CRY_ASSERT_MESSAGE(packet.GetWriteBuffer() != NULL, "Haven't written any data");
	CRY_ASSERT_MESSAGE(packet.GetWriteBufferPos() == packet.GetReadBufferSize(), "Packet size doesn't match data size");
	CRY_ASSERT_MESSAGE(packet.GetReliable(), "Unreliable packet sent");

	if(m_squadLeader)
	{
		if(connectionUID != CryMatchMakingInvalidConnectionUID)
		{
			pMatchmaking->SendToClient(&packet, m_squadHandle, connectionUID);
		}
		else
		{
			NetworkUtils_SendToAll(&packet, m_squadHandle, m_nameList, true);
		}
	}
	else
	{
		pMatchmaking->SendToServer(&packet, m_squadHandle);
	}
}

//---------------------------------------
void CSquadManager::ReadSquadPacket(SCryLobbyUserPacketData** ppPacketData)
{
	SCryLobbyUserPacketData* pPacketData = (*ppPacketData);
	CCryLobbyPacket* pPacket = pPacketData->pPacket;
	CRY_ASSERT_MESSAGE(pPacket->GetReadBuffer() != NULL, "No packet data");

	uint32 packetType = pPacket->StartRead();
	CryLog("CSquadManager::ReadSquadPacket() packetType = '%d'", packetType);

	switch(packetType)
	{
	case eGUPD_SquadJoin:
		{
			CryLog("  reading packet of type 'eGUPD_SquadJoin'");
			CRY_ASSERT(m_squadLeader);

			const unsigned int nameIndex = m_nameList.Find(pPacketData->connection);
			if (nameIndex != SSessionNames::k_unableToFind)
			{
				m_nameList.m_sessionNames[nameIndex].m_bFullyConnected = true;
			}

			uint32 clientVersion = pPacket->ReadUINT32();
			uint32 ownVersion = GameLobbyData::GetVersion();

			if (clientVersion == ownVersion)
			{
				if (m_currentGameSessionId != CrySessionInvalidID)
				{
					if(SquadsSupported())
					{
						CryLog("   host is in a session that does support squads, sending, eGUPD_SquadJoinGame");
						SendSquadPacket(eGUPD_SquadJoinGame,  pPacketData->connection);
					}
					else
					{
						CryLog("   host is in a session that does not support squads, sending, eGUPD_SquadNotSupported");
						SendSquadPacket(eGUPD_SquadNotSupported, pPacketData->connection);
					}
				}
				else
				{
					SendSquadPacket(eGUPD_SquadNotInGame, pPacketData->connection);
				}
			}
			else
			{
				SendSquadPacket(eGUPD_SquadDifferentVersion, pPacketData->connection);
			}
		}
		break;
	case eGUPD_SquadJoinGame:
		{
			CryLog("  reading packet of type 'eGUPD_SquadJoinGame'");
			CRY_ASSERT(!m_squadLeader);

			ICryLobbyService *pLobbyService = gEnv->pNetwork->GetLobby()->GetLobbyService(eCLS_Online);
			if (pLobbyService)
			{
				ICryMatchMaking *pMatchmaking = pLobbyService->GetMatchMaking();
				CrySessionID requestedGameSessionID = pMatchmaking->ReadSessionIDFromPacket(pPacket);
				
				const bool bIsMatchmakingGame = pPacket->ReadBool();
				const uint32 playlistId = pPacket->ReadUINT32();
				const int restrictRank = (int)pPacket->ReadUINT32();
				const int requireRank = (int)pPacket->ReadUINT32();

#if USE_CRYLOBBY_GAMESPY
				const uint8 passwordLength = pPacket->ReadUINT8();

				if ((passwordLength > 0) && (passwordLength <= MATCHMAKING_SESSION_PASSWORD_MAX_LENGTH))
				{
					char password[MATCHMAKING_SESSION_PASSWORD_MAX_LENGTH + 1];
					pPacket->ReadString((char*)&password, passwordLength + 1);
					CryLog("[Password]: received eGUPD_SquadJoinGame with password '%s'", password);

					ICVar* pPasswordCVar = gEnv->pConsole->GetCVar("sv_password");
					if (pPasswordCVar != NULL)
					{
						pPasswordCVar->Set(password);
					}
				}
#endif // USE_CRYLOBBY_GAMESPY

				IGameFramework *pFramework = gEnv->pGame->GetIGameFramework();
#ifdef USE_C2_FRONTEND
				CFlashFrontEnd *pFFE = g_pGame->GetFlashMenu();
				if(!pFFE || (!pFramework->StartingGameContext() && !pFFE->IsFlashRenderingDuringLoad()))
				{	
					CryLog("[squad]  calling squad join game straight away");
					SquadJoinGame(requestedGameSessionID, bIsMatchmakingGame, playlistId, restrictRank, requireRank);
				}
				else
#endif //#ifdef USE_C2_FRONTEND
				{
					CryLog("[squad]  delaying squad join game call");
					m_pendingGameJoin.Set(requestedGameSessionID, bIsMatchmakingGame, playlistId, restrictRank, requireRank, true);
				}
			}
		}
		break;
	case eGUPD_SquadLeaveGame:
		{
			CryLog("  reading 'eGUPD_SquadLeaveGame' packet");
			m_requestedGameSessionId = CrySessionInvalidID;
			CGameLobbyManager *pGameLobbyManager = g_pGame->GetGameLobbyManager();
			if (pGameLobbyManager)
			{
				pGameLobbyManager->LeaveGameSession(CGameLobbyManager::eLSR_ReceivedSquadLeavingFromSquadHost);

#ifdef USE_C2_FRONTEND
				CFlashFrontEnd *pFlashFrontEnd = g_pGame->GetFlashMenu();
				if (pFlashFrontEnd)
				{
					pFlashFrontEnd->ClearPendingChangedSettings();
				}
#endif //#ifdef USE_C2_FRONTEND

				m_pendingGameJoin.Invalidate();
			}
		}
		break;
	case eGUPD_SquadNotSupported:
		{
			CryLog("  reading 'eGUPD_SquadNotSupported' packet");
			RequestLeaveSquad();
			g_pGame->AddGameWarning("SquadNotSupported", NULL);
#ifdef GAME_IS_CRYSIS2
			g_pGame->GetWarnings()->AddWarning("SquadNotSupported", NULL);
#endif
		}
		break;
	case eGUPD_SquadNotInGame:
		{
			CryLog("  reading 'eGUPD_SquadNotInGame' packet");
			m_requestedGameSessionId = CrySessionInvalidID;
			CGameLobbyManager *pGameLobbyManager = g_pGame->GetGameLobbyManager();
			if (pGameLobbyManager)
			{
				pGameLobbyManager->LeaveGameSession(CGameLobbyManager::eLSR_AcceptingInvite);
			}
		}
		break;
	case eGUPD_SquadDifferentVersion:
		{
			CryLog("  reading 'eGUPD_SquadDifferentVersion' packet");
			RequestLeaveSquad();
			g_pGame->AddGameWarning("WrongSquadVersion", NULL);
#ifdef GAME_IS_CRYSIS2
			g_pGame->GetWarnings()->AddWarning("WrongVersion", NULL);
#endif
		}
		break;
	case eGUPD_SquadKick:
		{
			CryLog("  reading 'eGUPD_SquadKick' packet");

			ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
			if (pLobby)
			{
				ICryMatchMaking *pMatchmaking = pLobby->GetMatchMaking();
				if (pMatchmaking)
				{
					CrySessionID currentSessionId = pMatchmaking->SessionGetCrySessionIDFromCrySessionHandle(m_squadHandle);
					CTimeValue now = gEnv->pTimer->GetFrameStartTime();

					SKickedSession session;
					session.m_sessionId = currentSessionId;
					session.m_timeKicked = now;

					// If the user has changed, reset the list
					IPlayerProfileManager *pPlayerProfileManager = g_pGame->GetIGameFramework()->GetIPlayerProfileManager();
					if (pPlayerProfileManager)
					{
						const char *pCurrentUser = pPlayerProfileManager->GetCurrentUser();
						if (strcmp(pCurrentUser, m_kickedSessionsUsername.c_str()))
						{
							m_kickedSessionsUsername = pCurrentUser;
							m_kickedSessions.clear();
						}
					}

					const int numKickedSessions = m_kickedSessions.size();
					if (numKickedSessions < SQUADMGR_NUM_STORED_KICKED_SESSION)
					{
						m_kickedSessions.push_back(session);
					}
					else
					{
						int slotToUse = 0;
						CTimeValue oldestTime = now;
						for (int i = 0; i < numKickedSessions; ++ i)
						{
							if (m_kickedSessions[i].m_timeKicked < oldestTime)
							{
								oldestTime = m_kickedSessions[i].m_timeKicked;
								slotToUse = i;
							}
						}

						m_kickedSessions[slotToUse] = session;
					}
				}
			}

			RequestLeaveSquad();
#ifdef GAME_IS_CRYSIS2
			g_pGame->GetWarnings()->AddWarning("KickedFromSquad", NULL);
#endif

			CGameLobby *pGameLobby = g_pGame->GetGameLobby();
			if (pGameLobby != NULL && pGameLobby->IsPrivateGame())
			{
				g_pGame->GetGameLobbyManager()->LeaveGameSession(CGameLobbyManager::eLSR_KickedFromSquad);
			}
		}
		break;
	}

	CRY_ASSERT_MESSAGE(pPacket->GetReadBufferSize() == pPacket->GetReadBufferPos(), "Haven't read all the data");
}

//---------------------------------------
void CSquadManager::OnSquadLeaderChanged()
{
	CryLog("[tlh] CSquadManager::OnSquadLeaderChanged()");

	// Need to tell the game lobby who our squad leader is
	ICryLobbyService *pLobbyService = gEnv->pNetwork->GetLobby()->GetLobbyService(eCLS_Online);
	if (pLobbyService)
	{
		ICryMatchMaking *pMatchMaking = pLobbyService->GetMatchMaking();

		SCryMatchMakingConnectionUID hostUID = pMatchMaking->GetHostConnectionUID(m_squadHandle);
		int hostIdx = m_nameList.Find(hostUID);
		if (hostIdx != SSessionNames::k_unableToFind)
		{
			CryUserID hostUserId = m_nameList.m_sessionNames[hostIdx].m_userId;
			CRY_ASSERT_MESSAGE(hostUserId.IsValid(), "Failed to find a valid hostUserId, probably attempting to use squads on a LAN!");
			if (hostUserId.IsValid())
			{
				CryLog("[tlh]   calling GameLobby::OnSquadChanged()");
				m_squadLeaderId = hostUserId;
				g_pGame->GetGameLobby()->OnSquadChanged();
				m_isNewSquad = false;
			}
		}
		else
		{
			m_squadLeaderId = CryUserInvalidID;
			g_pGame->GetGameLobby()->OnSquadChanged();
		}

		// Need to tell flash that the squad leader has changed - set the name list as dirty
		m_nameList.m_dirty = true;
	}
}

//---------------------------------------
void CSquadManager::JoinUser(SCryUserInfoResult* user)
{
	CryLog("CSquadManager::JoinUser() user='%s'", user->m_userName);

	m_nameList.Insert(user->m_userID, user->m_conID, user->m_userName, user->m_userData, user->m_isDedicated);

	if(m_isNewSquad)
	{
		OnSquadLeaderChanged();
	}

	// if we're the leader and we're on the variant screen, then refresh so
	// options are updated, (e.g. solo gets disabled if necessary)
	if(m_squadLeader)
	{
#ifdef USE_C2_FRONTEND
		CFlashFrontEnd *pFlashFrontEnd = g_pGame->GetFlashMenu();
		if(pFlashFrontEnd != NULL && pFlashFrontEnd->GetCurrentMenuScreen() == eFFES_choose_variant)
		{
			pFlashFrontEnd->RefreshCurrentScreen();
		}
#endif //#ifdef USE_C2_FRONTEND
	}
}

//---------------------------------------
void CSquadManager::UpdateUser(SCryUserInfoResult* user)
{
	CryLog("CSquadManager::UpdateUser(SCryUserInfoResult* user)");
	m_nameList.Update(user->m_userID, user->m_conID, user->m_userName, user->m_userData, user->m_isDedicated, false);
}

//---------------------------------------
void CSquadManager::LeaveUser(SCryUserInfoResult* user)
{
	CryLog("CSquadManager::LeaveUser(), user='%s'", user->m_userName);
	m_nameList.Remove(user->m_conID);
}

#ifdef GAME_IS_CRYSIS2
//------------------------------------------------------------------------
void CSquadManager::GetDogtagInfoByChannel(int channelId, CDogtag::STagInfo &dogtagInfo)
{
	CrySessionHandle hdl = m_squadHandle;

	if(hdl != CrySessionInvalidHandle)
	{
		SCryMatchMakingConnectionUID temp = gEnv->pNetwork->GetLobby()->GetMatchMaking()->GetConnectionUIDFromGameSessionHandleAndChannelID(hdl, channelId);
		SSessionNames::SSessionName *pPlayer = m_nameList.GetSessionName(temp, true);
		if (pPlayer && g_pGame->GetGameLobby())
		{
			g_pGame->GetGameLobby()->GetDogtagInfoBySessionName(pPlayer, dogtagInfo);
		}
		else
		{
			CryLog("CSquadManager::GetDogtagInfoByChannel() failed to find player for channelId %i", channelId);
		}
	}
	else
	{
		CryLog("CSquadManager::GetDogtagInfoByChannel invalid session handle, not retrieving dogtag info");
	}
}
#endif

//-------------------------------------------------------------------------
void CSquadManager::ShowGamercardForSquadChannel(int channelId)
{
	if (g_pGame->GetGameLobby())
	{
		if (InSquad() && gEnv->pNetwork && gEnv->pNetwork->GetLobby())
		{
			CryUserID userId = GetUserIDFromChannelID(channelId);
			if (userId.IsValid())
			{
				g_pGame->GetGameLobby()->ShowGamercardByUserId(userId);
			}
		}
	}
}


//---------------------------------------
CryUserID CSquadManager::GetUserIDFromChannelID(int channelId)
{
	CryUserID userId = CryUserInvalidID;

	if (gEnv->pNetwork && gEnv->pNetwork->GetLobby())
	{
		if (ICryMatchMaking* pMatchMaking = gEnv->pNetwork->GetLobby()->GetMatchMaking())
		{
			SCryMatchMakingConnectionUID conId = pMatchMaking->GetConnectionUIDFromGameSessionHandleAndChannelID(m_squadHandle, channelId);
			SSessionNames::SSessionName *pSessionName = m_nameList.GetSessionName(conId, true);
			if (pSessionName)
			{
				userId = pSessionName->m_userId;
			}
		}
	}

#if !defined(_RELEASE)
	if (userId == CryUserInvalidID)
	{
		CryLog("Failed to get CryUserID for squad channel %d", channelId);
	}
#endif

	return userId;
}

//-------------------------------------------------------------------------
#ifdef USE_C2_FRONTEND
void CSquadManager::SendSquadMembersToFlash(IFlashPlayer *pFlashPlayer, bool compactVersion/*=false*/)
{
	if (!pFlashPlayer)
		return;

	CryFixedStringT<DISPLAY_NAME_LENGTH> leaderName;
	int size = 0;

	if(InSquad())
	{
		const int NUM_ELEMENTS_PER_PUSH = 5;
		const int MAX_PUSH_ELEMENTS = SQUADMGR_MAX_SQUAD_SIZE * NUM_ELEMENTS_PER_PUSH; 
		size = m_nameList.Size();
		if(size)
		{
			CryFixedArray<SFlashSquadPlayerInfo, SQUADMGR_MAX_SQUAD_SIZE> playerInfos;

			CryUserID squadLeader = m_squadLeaderId;

			size = MIN(size, SQUADMGR_MAX_SQUAD_SIZE);
			for (int nameIdx(0); (nameIdx<size); nameIdx++)
			{
				SSessionNames::SSessionName &pPlayer = m_nameList.m_sessionNames[nameIdx];

				SFlashSquadPlayerInfo playerInfo;

				uint32 channelId = pPlayer.m_conId.m_uid;
				bool isLocal = (nameIdx==0);

				playerInfo.m_rank = pPlayer.m_userData[eLUD_Rank];
				pPlayer.GetDisplayName(playerInfo.m_nameString);
				playerInfo.m_conId = channelId;
				playerInfo.m_isLocal = isLocal;	// Currently local player is always index 0
				if (squadLeader.IsValid() && (squadLeader == pPlayer.m_userId))
				{
					playerInfo.m_isSquadLeader = true;
					leaderName = playerInfo.m_nameString.c_str();
				}
				else
				{
					playerInfo.m_isSquadLeader = false;
				}

				playerInfo.m_reincarnations = pPlayer.GetReincarnations();

				playerInfos.push_back(playerInfo);
			}

#ifndef _RELEASE
			if (g_pGame->GetHUD() && g_pGame->GetHUD()->GetCVars()->menu_useTestData)
			{
				const int useSize = SQUADMGR_MAX_SQUAD_SIZE;

				for (int nameIdx(size); (nameIdx<useSize); nameIdx++)
				{
					SFlashSquadPlayerInfo playerInfo;
					playerInfo.m_rank = nameIdx;
					playerInfo.m_reincarnations = (cry_rand()%8) ? 0 : (cry_rand()%10);
					playerInfo.m_nameString.Format("Player%d", nameIdx);
					playerInfo.m_conId = 100+nameIdx;
					playerInfo.m_isLocal = false;
					playerInfo.m_isSquadLeader = false;

					playerInfos.push_back(playerInfo);
				}
			}
#endif
			size = playerInfos.size();

			IFlashVariableObject *pPushArray = NULL;
			FE_FLASHVAROBJ_REG(pFlashPlayer, FRONTEND_SUBSCREEN_PATH_SET("m_namesList"), pPushArray);

			pPushArray->ClearElements();

			for (int playerIdx(0); (playerIdx<size); ++playerIdx)
			{
				IFlashVariableObject *pNewObject = NULL;
				if (pFlashPlayer->CreateObject("Object", NULL, 0, pNewObject))
				{
					const bool bRankValid = (playerInfos[playerIdx].m_rank > 0);
					SFlashVarValue rankInt(playerInfos[playerIdx].m_rank);
					SFlashVarValue rankNull("");

					pNewObject->SetMember("Name", playerInfos[playerIdx].m_nameString.c_str());
					pNewObject->SetMember("Rank", bRankValid ? rankInt : rankNull);
					pNewObject->SetMember("Reincarnations", playerInfos[playerIdx].m_reincarnations);
					pNewObject->SetMember("ConId", playerInfos[playerIdx].m_conId);
					pNewObject->SetMember("Local", playerInfos[playerIdx].m_isLocal);
					pNewObject->SetMember("SquadLeader", playerInfos[playerIdx].m_isSquadLeader);
					pPushArray->PushBack(pNewObject);

					FE_FLASHOBJ_SAFERELEASE(pNewObject);
				}
			}
			FE_FLASHOBJ_SAFERELEASE(pPushArray);
		}
	}

	pFlashPlayer->Invoke0(FRONTEND_SUBSCREEN_PATH_SET("UpdateSquadMembersList"));

	// Status Message
	IFlashVariableObject *pFlashStatusMessage = NULL;
	FE_FLASHVAROBJ_REG(pFlashPlayer, FRONTEND_SUBSCREEN_PATH_SET("PlayerList.StatusMessage"), pFlashStatusMessage);
	if (compactVersion)
	{
		if (size==2)
			pFlashStatusMessage->SetText(CHUDUtils::LocalizeString("@ui_menu_squadlist_plus_member", CryStringUtils::toString(size-1)));
		else if (size>3)
			pFlashStatusMessage->SetText(CHUDUtils::LocalizeString("@ui_menu_squadlist_plus_members", CryStringUtils::toString(size-1)));
		else
			pFlashStatusMessage->SetText("");
	}
	else if (InSquad())
	{
		if (m_squadLeader==false)
			pFlashStatusMessage->SetText("@ui_menu_squadlist_waiting_for_leader");
		else
			pFlashStatusMessage->SetText("@ui_menu_squadlist_invite_others");
	}
	else
	{
		pFlashStatusMessage->SetText("");
	}
	FE_FLASHOBJ_SAFERELEASE(pFlashStatusMessage);


	// Squad Title
	IFlashVariableObject *pSquadName = NULL;
	FE_FLASHVAROBJ_REG(pFlashPlayer, FRONTEND_SUBSCREEN_PATH_SET("PlayerList.SquadName"), pSquadName);
	if (!leaderName.empty())
	{
		pSquadName->SetText(CHUDUtils::LocalizeString("@ui_menu_squadlist_title_name", leaderName.c_str()));
	}
	else
	{
		pSquadName->SetText("@ui_menu_squadlist_title");
	}
	FE_FLASHOBJ_SAFERELEASE(pSquadName);
}
#endif //#ifdef USE_C2_FRONTEND

//-------------------------------------------------------------------------
bool CSquadManager::GetSquadCommonDLCs(uint32 &commonDLCs)
{
	if(InSquad())
	{
		int numPlayers = m_nameList.Size();
		if(numPlayers)
		{
			commonDLCs = ~0;
			for (int i=0; i<numPlayers; ++i)
			{
				SSessionNames::SSessionName &player = m_nameList.m_sessionNames[i];
				commonDLCs &= player.m_userData[eLUD_LoadedDLCs];
			}
			return true;
		}
	}
	commonDLCs = 0;
	return false;
}

//-------------------------------------------------------------------------
void CSquadManager::TaskStartedCallback( CLobbyTaskQueue::ESessionTask task, void *pArg )
{
	CryLog("CSquadManager::TaskStartedCallback(task=%u)", task);
	INDENT_LOG_DURING_SCOPE();

	CSquadManager *pSquadManager = static_cast<CSquadManager*>(pArg);

	ECryLobbyError result = eCLE_ServiceNotSupported;
	bool bMatchMakingTaskStarted = false;
	CryLobbyTaskID taskId = CryLobbyInvalidTaskID;

	ICryLobby *pCryLobby = gEnv->pNetwork->GetLobby();
	if (pCryLobby != NULL && pCryLobby->GetLobbyService(eCLS_Online))
	{
		ICryMatchMaking *pMatchMaking = pCryLobby->GetLobbyService(eCLS_Online)->GetMatchMaking();
		if (pMatchMaking)
		{
			switch (task)
			{
			case CLobbyTaskQueue::eST_Create:
				if (sm_enable && (pSquadManager->m_squadHandle == CrySessionInvalidHandle))
				{
					CryLog("    creating a squad");
					bMatchMakingTaskStarted = true;
					result = pSquadManager->DoCreateSquad(pMatchMaking, taskId);
				}
				break;

			case CLobbyTaskQueue::eST_Join:
				if (sm_enable && (pSquadManager->m_inviteSessionId != CrySessionInvalidID) && (pSquadManager->m_squadHandle == CrySessionInvalidHandle))
				{
					CryLog("    joining a squad");
					bMatchMakingTaskStarted = true;
					result = pSquadManager->DoJoinSquad(pMatchMaking, taskId);
				}
				break;

			case CLobbyTaskQueue::eST_Delete:
				if (pSquadManager->m_squadHandle != CrySessionInvalidHandle)
				{
					CryLog("    deleting current squad");
					bMatchMakingTaskStarted = true;
					result = pSquadManager->DoLeaveSquad(pMatchMaking, taskId);
				}
				else
				{
					pSquadManager->m_leavingSession = false;
				}
				break;

			case CLobbyTaskQueue::eST_SetLocalUserData:
				{
					if (sm_enable && (pSquadManager->m_squadHandle != CrySessionInvalidHandle))
					{
						CryLog("    setting local user data");
						bMatchMakingTaskStarted = true;
						result = pSquadManager->DoUpdateLocalUserData(pMatchMaking, taskId);
					}
				}
				break;

			case CLobbyTaskQueue::eST_SessionStart:
				{
					if (sm_enable && (pSquadManager->m_squadHandle != CrySessionInvalidHandle))
					{
						if (!pSquadManager->m_bSessionStarted)
						{
							CryLog("    starting session");
							bMatchMakingTaskStarted = true;
							result = pSquadManager->DoStartSession(pMatchMaking, taskId);
						}
					}
				}
				break;

			case CLobbyTaskQueue::eST_SessionEnd:						// Deliberate fall-through
				{
					if (pSquadManager->m_squadHandle != CrySessionInvalidHandle)
					{
						if (pSquadManager->m_bSessionStarted)
						{
							CryLog("    ending session");
							bMatchMakingTaskStarted = true;
							result = pSquadManager->DoEndSession(pMatchMaking, taskId);
						}
					}
				}
				break;
			case CLobbyTaskQueue::eST_SessionUpdateSlotType:
				{
					if(sm_enable && (pSquadManager->m_squadHandle != CrySessionInvalidHandle))	// don't really need to do this if disabled or no session
					{
						if(pSquadManager->m_slotType != pSquadManager->m_requestedSlotType)
						{
							CryLog("    session update slot type");
							bMatchMakingTaskStarted = true;
							result = pSquadManager->DoSessionChangeSlotType(pMatchMaking, taskId);
						}
					}
					break;
				}
			case CLobbyTaskQueue::eST_SessionSetLocalFlags:
				{
					if (sm_enable && (pSquadManager->m_squadHandle != CrySessionInvalidHandle))
					{
						CryLog("    setting local session flags");
						bMatchMakingTaskStarted = true;
						result = pSquadManager->DoSessionSetLocalFlags(pMatchMaking, taskId);
					}
					break;
				}
			}
		}
	}

	if (bMatchMakingTaskStarted)
	{
		if (result == eCLE_Success)
		{
			pSquadManager->m_currentTaskId = taskId;
		}
		else if(result == eCLE_SuccessInvalidSession)
		{
			pSquadManager->m_taskQueue.TaskFinished();
		}
		else if (result == eCLE_TooManyTasks)
		{
			CryLog("  too many tasks, restarting next frame");
			pSquadManager->m_taskQueue.RestartTask();
		}
		else
		{
			ReportError(result);
			pSquadManager->m_taskQueue.TaskFinished();
		}
	}
	else
	{
		// Task failed to start
		CryLog("  ERROR: task failed to start, we're in an incorrect state sm_enable=%i, squadHandle=%u, sessionStarted=%s",
			sm_enable, pSquadManager->m_squadHandle, pSquadManager->m_bGameSessionStarted ? "true" : "false");

		pSquadManager->m_taskQueue.TaskFinished();
	}
}

//-------------------------------------------------------------------------
void CSquadManager::TaskFinished()
{
	CryLog("CSquadManager::TaskFinished() task=%u", m_taskQueue.GetCurrentTask());

	m_taskQueue.TaskFinished();
}

//static---------------------------------------
void CSquadManager::ReportError(ECryLobbyError error)
{
	CryLogAlways("CSquadManager::ReportError(ECryLobbyError error = %d)", error);

	if(error != eCLE_Success)
	{
			switch(error)
			{
			case eCLE_InsufficientPrivileges:
				g_pGame->AddGameWarning("InsufficientPrivileges", NULL);
				break;
			case eCLE_InternetDisabled:
				g_pGame->AddGameWarning("NetworkDisonnected", NULL);
				break;
			case eCLE_UserNotSignedIn:
				g_pGame->AddGameWarning("NotSignedIn", NULL);
				break;
			case eCLE_CableNotConnected:
				{
					//Should never happen, but just handle it to be sure
					CGameLobbyManager *pGameLobbyManager = g_pGame->GetGameLobbyManager();
					if (pGameLobbyManager)
					{
						pGameLobbyManager->SetCableConnected(false);
					}
					g_pGame->AddGameWarning("CableUnplugged", NULL);
					break;
				}
			default:
				g_pGame->AddGameWarning("SquadManagerError", NULL);
				break;
			}
#ifdef USE_C2_FRONTEND
		CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
		if(menu)
		{
			CMPMenuHub *mpMenuHub = menu->GetMPMenu();
			if (mpMenuHub)
			{
				CMPMenuHub::EDisconnectError disconnectError = CMPMenuHub::eDCE_None;
				CGameLobbyManager *pGameLobbyManager = g_pGame->GetGameLobbyManager();
				const bool  onlineLobby = (gEnv->pNetwork->GetLobby()->GetLobbyServiceType() == eCLS_Online);

				if ((error == eCLE_UserNotSignedIn) && onlineLobby)
				{
					disconnectError = CMPMenuHub::eDCE_F_UserNotSignedIn;
				}
				else if ((error == eCLE_InsufficientPrivileges) && onlineLobby)
				{
					disconnectError = CMPMenuHub::eDCE_F_InsufficientPrivileges;
				}
				else if(error == eCLE_CableNotConnected)
				{
					// Should never really happen as you can't be online at this point - might as well handle though.
					// as far as I'm aware, we can only get this error on PS3 at the moment,
					// it occurs if you try to select the online game mode without the ethernet
					// cable connected, if this happens, we need to send them back to the select screen,
					// otherwise we leave the user in a very bad state, because they will have
					// failed to create their squad, all the while trying to do various other online
					// things that may eventually explode
					
					if (pGameLobbyManager)
					{
						pGameLobbyManager->SetCableConnected(false);
					}

					disconnectError = CMPMenuHub::eDCE_F_CableNotConnected;
				}
				else if(error == eCLE_InternetDisabled)
				{
					// Should never really happen as you can't be online at this point - might as well handle though.
					disconnectError = CMPMenuHub::eDCE_F_InternetDisabled;
				}

				if(disconnectError != CMPMenuHub::eDCE_None)
				{
					mpMenuHub->SetDisconnectError(disconnectError, true);
				}
				else
				{
					if(!pGameLobbyManager || pGameLobbyManager->IsCableConnected())
					{
						CGameLobby::ShowErrorDialog(error, NULL, NULL, NULL);
					}
				}
			}
		}
#endif //#ifdef USE_C2_FRONTEND
	}
}

//static---------------------------------------
void CSquadManager::HandleCustomError(const char* dialogName, const char* msgPreLoc, const bool deleteSession, const bool returnToMainMenu)
{
	if (deleteSession)
	{
		g_pGame->GetGameLobby()->LeaveSession(true);
	}

#ifdef USE_C2_FRONTEND
	if (CMPMenuHub* mpMenuHub = CMPMenuHub::GetMPMenuHub())
	{
		if (returnToMainMenu)
		{
			mpMenuHub->GoToCurrentLobbyServiceScreen();
		}

		mpMenuHub->ShowErrorDialog(dialogName, msgPreLoc);  // inform user
	}
#endif //#ifdef USE_C2_FRONTEND
}

//-------------------------------------------------------------------------
bool CSquadManager::CallbackReceived( CryLobbyTaskID taskId, ECryLobbyError result )
{
	if (m_currentTaskId == taskId)
	{
		m_currentTaskId = CryLobbyInvalidTaskID;
	}
	else
	{
		CryLog("CSquadManager::CallbackReceived() received callback with an unexpected taskId=%d, expected=%d", taskId, m_currentTaskId);
	}

	if (result == eCLE_TimeOut)
	{
		if(sm_enable || m_taskQueue.GetCurrentTask() == CLobbyTaskQueue::eST_Delete)
		{
			CryLog("CSquadManager::CallbackReceived() task timed out, restarting");
			m_taskQueue.RestartTask();
		}
		else
		{
			CryLog("CSquadManager::CallbackReceived() task timed out, finishing as squad manager is disabled");
			m_taskQueue.TaskFinished();
		}
	}
	else
	{
		m_taskQueue.TaskFinished();

		if (result != eCLE_Success && result != eCLE_SuccessInvalidSession)
		{
			CryLog("CSquadManager::CallbackReceived() task unsuccessful, result=%u", result);
			ReportError(result);
		}
	}

	return (result == eCLE_Success) || (result == eCLE_SuccessInvalidSession);
}

//-------------------------------------------------------------------------
bool CSquadManager::OnPromoteToServer( SHostMigrationInfo& hostMigrationInfo, uint32& state )
{
	if (m_squadHandle == hostMigrationInfo.m_session)
	{
		const unsigned int numUsers = m_nameList.Size();
		for (unsigned int i = 0; i < numUsers; ++ i)
		{
			m_nameList.m_sessionNames[i].m_bFullyConnected = true;
		}

		m_pendingGameJoin.Invalidate();
	}

	return true;
}

//-------------------------------------------------------------------------
bool CSquadManager::OnFinalise( SHostMigrationInfo& hostMigrationInfo, uint32& state )
{
	if (m_squadHandle == hostMigrationInfo.m_session)
	{
		m_squadLeader = hostMigrationInfo.IsNewHost();
		CryLog("CSquadManager::OnFinalise() squad session has changed host, isNewHost=%s", m_squadLeader ? "true" : "false");
		OnSquadLeaderChanged();

#ifdef USE_C2_FRONTEND
		if (CMPMenuHub *pMPMenus = CMPMenuHub::GetMPMenuHub())
		{
			pMPMenus->SquadCallback(CMPMenuHub::eMSC_MigratedSquad, m_squadLeader, this);
		}
#endif //#ifdef USE_C2_FRONTEND
	}

	return true;
}

//-------------------------------------------------------------------------
void CSquadManager::SetMultiplayer( bool multiplayer )
{
	bool shouldBeEnabled = false;

	if (m_bMultiplayerGame != multiplayer)
	{
		m_bMultiplayerGame = multiplayer;
		if (multiplayer)
		{
			// Only enable if we're using online service
			ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
			if (pLobby)
			{
				const bool allowJoinMultipleSessions = pLobby->GetLobbyServiceFlag(eCLS_Online, eCLSF_AllowMultipleSessions);
				if (allowJoinMultipleSessions)
				{
					shouldBeEnabled = true;
				}
			}
		}
	}

	//If we are in multiplayer, always enable the squadmanager
	Enable(multiplayer, false);
}

//-------------------------------------------------------------------------
void CSquadManager::TellMembersToLeaveGameSession()
{
	CryLog("CSquadManager::TellMembersToLeaveGameSession()");
	SendSquadPacket(eGUPD_SquadLeaveGame);
}

//-------------------------------------------------------------------------
void CSquadManager::RequestLeaveSquad()
{
	CryLog("CSquadManager::RequestLeaveSquad()");
	DeleteSession();
}

#if !defined(_RELEASE)
//static---------------------------------------
void CSquadManager::CmdCreate(IConsoleCmdArgs* pCmdArgs)
{
	CSquadManager *pSquadManager = g_pGame->GetSquadManager();
	pSquadManager->m_taskQueue.AddTask(CLobbyTaskQueue::eST_Create, false);
}

//static---------------------------------------
void CSquadManager::CmdLeave(IConsoleCmdArgs* pCmdArgs)
{
	CSquadManager *pSquadManager = g_pGame->GetSquadManager();
	pSquadManager->DeleteSession();
}

//static---------------------------------------
void CSquadManager::CmdKick(IConsoleCmdArgs* pCmdArgs)
{
	if (pCmdArgs->GetArgCount() == 2)
	{
		const char *pPlayerName = pCmdArgs->GetArg(1);
		CSquadManager *pSquadManager = g_pGame->GetSquadManager();

		if (pSquadManager->m_squadLeader)
		{
			SSessionNames *pSessionNames = &pSquadManager->m_nameList;
			const int numMembers = pSessionNames->Size();
			for (int i = 0; i < numMembers; ++ i)
			{
				SSessionNames::SSessionName &player = pSessionNames->m_sessionNames[i];
				if (!stricmp(player.m_name, pPlayerName))
				{
					pSquadManager->KickPlayer(player.m_userId);
				}
			}
		}
		else
		{
			CryLog("Only the squad leader can kick players");
		}
	}
	else
	{
		CryLog("Invalid format, use 'sm_kick <playername>'");
	}
}
#endif

//-------------------------------------------------------------------------
void CSquadManager::OnGameSessionStarted()
{
	m_bGameSessionStarted = true;
	m_taskQueue.AddTask(CLobbyTaskQueue::eST_SessionStart, false);
}

//-------------------------------------------------------------------------
void CSquadManager::OnGameSessionEnded()
{
	m_bGameSessionStarted = false;
	m_taskQueue.AddTask(CLobbyTaskQueue::eST_SessionEnd, false);
}

//-------------------------------------------------------------------------
void CSquadManager::DeleteSession()
{
	CryLog("CSquadManager::DeleteSession()");

	// We want to leave the current session, clear/cancel any pending tasks that aren't related
	// to deleting the session

	CLobbyTaskQueue::ESessionTask currentTask = m_taskQueue.GetCurrentTask();

	// need to let create/join finish their tasks before deleting, or we can get the session
	// in a bad state, they will initiate delete session on completion if need be
	if(currentTask == CLobbyTaskQueue::eST_Create)
	{
		CryLog(" cannot delete session, currently creating");
		return;
	}

	// join session is marked as invalid so we don't stay with it 
	// upon completion
	if(currentTask == CLobbyTaskQueue::eST_Join)	
	{
		CryLog(" cannot delete session, currently joining");
		m_sessionIsInvalid = true;
		return;
	}

	m_pendingGameJoin.Invalidate();

	if(m_squadHandle != CrySessionInvalidHandle)	// don't bother trying to add a delete if we're not already in a session
	{	
		m_leavingSession = true;

		CLobbyTaskQueue::ESessionTask firstRequiredTask = CLobbyTaskQueue::eST_Delete;
		if (m_bSessionStarted)
		{
			firstRequiredTask = CLobbyTaskQueue::eST_SessionEnd;
		}

		if (currentTask != firstRequiredTask)
		{
			if (m_currentTaskId != CryLobbyInvalidTaskID)
			{
				ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
				if (pLobby)
				{
					ICryLobbyService *pLobbyService = pLobby->GetLobbyService(eCLS_Online);
					if (pLobbyService)
					{
						ICryMatchMaking *pMatchMaking = pLobbyService->GetMatchMaking();
						if (pMatchMaking)
						{
							pMatchMaking->CancelTask(m_currentTaskId);
							m_currentTaskId = CryLobbyInvalidTaskID;
						}
					}
				}
			}
			m_taskQueue.Reset();

			if (m_bSessionStarted)
			{
				m_taskQueue.AddTask(CLobbyTaskQueue::eST_SessionEnd, false);
			}
			m_taskQueue.AddTask(CLobbyTaskQueue::eST_Delete, false);
		}
		else
		{
			m_taskQueue.ClearNotStartedTasks();
			// We're already doing the first required task, if this task is a eST_SessionEnd then we need
			// to follow it with a eST_Delete
			if (firstRequiredTask == CLobbyTaskQueue::eST_SessionEnd)
			{
				m_taskQueue.AddTask(CLobbyTaskQueue::eST_Delete, false);
			}
		}
	}
}

//-------------------------------------------------------------------------
ECryLobbyError CSquadManager::DoStartSession( ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId )
{
	CryLog("CSquadManager::DoStartSession()");
	CRY_ASSERT(m_taskQueue.GetCurrentTask() == CLobbyTaskQueue::eST_SessionStart);

	ECryLobbyError result = eCLE_ServiceNotSupported;

	result = pMatchMaking->SessionStart(m_squadHandle, &taskId, SessionStartCallback, this);
	return result;
}

//-------------------------------------------------------------------------
void CSquadManager::SessionStartCallback( CryLobbyTaskID taskID, ECryLobbyError error, void* pArg )
{
	CryLog("CSquadManager::SessionStartCallback()");

	CSquadManager *pSquadManager = static_cast<CSquadManager*>(pArg);

	if (pSquadManager->CallbackReceived(taskID, error))
	{
		pSquadManager->m_bSessionStarted = true;
	}
}

//-------------------------------------------------------------------------
ECryLobbyError CSquadManager::DoEndSession( ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId )
{
	CryLog("CSquadManager::DoEndSession()");
	CRY_ASSERT(m_taskQueue.GetCurrentTask() == CLobbyTaskQueue::eST_SessionEnd);

	ECryLobbyError result = eCLE_ServiceNotSupported;
	result = pMatchMaking->SessionEnd(m_squadHandle, &taskId, SessionEndCallback, this);
	return result;
}

//-------------------------------------------------------------------------
void CSquadManager::SessionEndCallback( CryLobbyTaskID taskID, ECryLobbyError error, void* pArg )
{
	CryLog("CSquadManager::SessionEndCallback()");

	CSquadManager *pSquadManager = static_cast<CSquadManager*>(pArg);

	pSquadManager->CallbackReceived(taskID, error);

	// If we get any result other than a timeout (which will try again), we need to reset the started flag
	if (error != eCLE_TimeOut)
	{
		pSquadManager->m_bSessionStarted = false;
	}
}

//-------------------------------------------------------------------------
void CSquadManager::SessionClosedCallback( UCryLobbyEventData eventData, void *userParam )
{
	CSquadManager *pSquadManager = static_cast<CSquadManager*>(userParam);
	CRY_ASSERT(pSquadManager);
	if (pSquadManager)
	{
		SCryLobbySessionEventData *pEventData = eventData.pSessionEventData;
		
		if ((pSquadManager->m_squadHandle == pEventData->session) && (pEventData->session != CrySessionInvalidHandle))
		{
			CryLog("CSquadManager::SessionClosedCallback() received SessionClosed event, leaving session");
			pSquadManager->RequestLeaveSquad();
		}
	}	
}

//-------------------------------------------------------------------------
void CSquadManager::ForcedFromRoomCallback(UCryLobbyEventData eventData, void *pArg)
{
	CSquadManager *pSquadManager = static_cast<CSquadManager*>(pArg);
	if (pSquadManager)
	{
		SCryLobbyForcedFromRoomData *pEventData = eventData.pForcedFromRoomData;

		CryLog("CSquadManager::ForcedFromRoomCallback session %d reason %d", (int)pEventData->m_session, pEventData->m_why);

		if (pEventData->m_session != CrySessionInvalidHandle && pSquadManager->m_squadHandle == pEventData->m_session)
		{
			// any of the other reasons will result in sign out, disconnect, thus
			// handled appropriately by the other events out there
			CryLog("[squad] received eCLSE_ForcedFromRoom event with reason %d, leaving session", pEventData->m_why);
			pSquadManager->RequestLeaveSquad();
		}
	}
}

//-------------------------------------------------------------------------
bool CSquadManager::OnTerminate( SHostMigrationInfo& hostMigrationInfo, uint32& state )
{
	if (m_squadHandle == hostMigrationInfo.m_session)
	{
		CryLog("CSquadManager::OnTerminate() host migration failed, leaving session");
		RequestLeaveSquad();
	}	

	return true;
}

//-------------------------------------------------------------------------
bool CSquadManager::OnReset(SHostMigrationInfo& hostMigrationInfo, uint32& state)
{
	return true;
}

//-------------------------------------------------------------------------
bool CSquadManager::SquadsSupported()
{
	bool result = true;

#ifdef GAME_IS_CRYSIS2
	CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
	bool result = true;

	if(pPlaylistManager)
	{
		int activeVariant = pPlaylistManager->GetActiveVariantIndex();
		if(activeVariant >= 0)
		{
			const SGameVariant *pGameVariant = pPlaylistManager->GetVariant(activeVariant);
			if(pGameVariant)
			{
				result = pGameVariant->m_allowSquads;
			}
		}
	}
#endif

	return result;
}

//-------------------------------------------------------------------------
void CSquadManager::SessionChangeSlotType(ESessionSlotType type)
{
	CryLog("[squad] SessionChangeSlotType set slots to be %s", (type == eSST_Public) ? "public" : "private");

	m_requestedSlotType = type;
	m_taskQueue.AddTask(CLobbyTaskQueue::eST_SessionUpdateSlotType, false);
}

//-------------------------------------------------------------------------
ECryLobbyError CSquadManager::DoSessionChangeSlotType(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId)
{
	ECryLobbyError error = eCLE_Success;

	uint32 numPublicSlots = (m_requestedSlotType == eSST_Public) ? SQUADMGR_MAX_SQUAD_SIZE	: 0;
	uint32 numPrivateSlots = (m_requestedSlotType == eSST_Private) ? SQUADMGR_MAX_SQUAD_SIZE	: 0;

	CryLog("[squad] DoSessionChangeSlotType numPublicSlots %d numPrivateSlots %d squadHandle %d", numPublicSlots, numPrivateSlots, GetSquadSessionHandle());
	error = pMatchMaking->SessionUpdateSlots(GetSquadSessionHandle(), numPublicSlots, numPrivateSlots, &taskId, CSquadManager::SessionChangeSlotTypeCallback, this);

	if(error == eCLE_Success)
	{
		m_inProgressSlotType = m_requestedSlotType;
	}

	return error;
}

//-------------------------------------------------------------------------
void CSquadManager::SessionChangeSlotTypeFinished(CryLobbyTaskID taskID, ECryLobbyError error)
{
	if (CallbackReceived(taskID, error))
	{
		CryLog("[squad] SessionChangeSlotTypeFinished succeeded in changing slot status to %s", (m_inProgressSlotType == eSST_Public) ? "public" : "private");
		
		m_slotType = m_inProgressSlotType;
#ifdef GAME_IS_CRYSIS2
		g_pGame->RefreshRichPresence();
#endif
	}
	else if (error != eCLE_TimeOut)
	{
		CryLog("[squad] SessionChangeSlotTypeFinished could not set slot type to %s, failed with error code %d",  (m_inProgressSlotType == eSST_Public) ? "public" : "private", error);
	}
}

//-------------------------------------------------------------------------
ECryLobbyError CSquadManager::DoSessionSetLocalFlags(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId)
{
	ECryLobbyError result = pMatchMaking->SessionSetLocalFlags(GetSquadSessionHandle(), CRYSESSION_LOCAL_FLAG_HOST_MIGRATION_CAN_BE_HOST, &taskId, CSquadManager::SessionSetLocalFlagsCallback, this);

	return result;
}

//-------------------------------------------------------------------------
void CSquadManager::SessionSetLocalFlagsCallback( CryLobbyTaskID taskID, ECryLobbyError error, CrySessionHandle h, uint32 flags, void* pArg )
{
	CryLog("CSquadManager::SessionSetLocalFlagsCallback() error=%u", error);

	CSquadManager *pSquadManager = static_cast<CSquadManager*>(pArg);
	pSquadManager->CallbackReceived(taskID, error);
}

//-------------------------------------------------------------------------
void CSquadManager::LeftGameSessionInProgress()
{
	CryLog("CSquadManager::LeftGameSessionInProgress()");

	if (HaveSquadMates())
	{
		if (InCharge())
		{
			CryLog("  currently in charge of an active squad, leave it");
			RequestLeaveSquad();
		}
		else
		{
			if (NetworkUtils_CompareCrySessionId(m_requestedGameSessionId, m_currentGameSessionId))
			{
				CryLog("  currently a member in a squad, leaving game that squad leader is in, leave squad too");
				RequestLeaveSquad();
			}
		}
	}
}

//-------------------------------------------------------------------------
void CSquadManager::KickPlayer( CryUserID userId )
{
	if (m_squadLeader)
	{
		if (!(m_squadLeaderId == userId))		// CryUserID doesn't support !=  :-(
		{
			int index = m_nameList.FindByUserId(userId);
			if (index != SSessionNames::k_unableToFind)
			{
				SSessionNames::SSessionName &player = m_nameList.m_sessionNames[index];
				m_pendingKickUserId = player.m_userId;
				CryLog("CSquadManager::KickPlayer() requested kick '%s' uid=%u", player.m_name, player.m_conId.m_uid);
#ifdef GAME_IS_CRYSIS2
				g_pGame->GetWarnings()->AddWarning("KickPlayerFromSquad", player.m_name, this);
#endif
			}
		}
	}
}

//-------------------------------------------------------------------------
bool CSquadManager::AllowedToJoinSession( CrySessionID sessionId )
{
	IPlayerProfileManager *pPlayerProfileManager = g_pGame->GetIGameFramework()->GetIPlayerProfileManager();
	if (pPlayerProfileManager)
	{
		const char *pCurrentUser = pPlayerProfileManager->GetCurrentUser();
		if (strcmp(pCurrentUser, m_kickedSessionsUsername.c_str()))
		{
			// If the user has changed since our banned list was created, allow the join
			// Note: The list will be reset when we're next kicked, by resetting it here we stop the user
			// just logging out and back in again to bypass the ban
			return true;
		}
	}
	
	const int numKickedSessions = m_kickedSessions.size();
	for (int i = 0; i < numKickedSessions; ++ i)
	{
		if (NetworkUtils_CompareCrySessionId(m_kickedSessions[i].m_sessionId, sessionId))
		{
			CTimeValue now = gEnv->pTimer->GetFrameStartTime();
			float timeDiff = (now - m_kickedSessions[i].m_timeKicked).GetSeconds();

			if (timeDiff < 1800.f)		// 30 mins
			{
				return false;
			}
		}
	}

	return true;
}

//-------------------------------------------------------------------------
void CSquadManager::RemoveFromBannedList(CrySessionID sessionId)
{
	const int numKickedSessions = m_kickedSessions.size();
	for (int i = 0; i < numKickedSessions; ++ i)
	{
		if (NetworkUtils_CompareCrySessionId(m_kickedSessions[i].m_sessionId, sessionId))
		{
			m_kickedSessions.removeAt(i);
			break;
		}
	}
}

//-------------------------------------------------------------------------
bool CSquadManager::IsEnabled()
{
	return sm_enable != 0;
}

//-------------------------------------------------------------------------
void CSquadManager::SquadJoinGame(CrySessionID sessionID, bool isMatchmakingGame, uint32 playlistID, int restrictRank, int requireRank)
{
	CryLog("SquadJoinGame isMatchmakingGame %d, playlistID %d, restrictRank %d requireRank %d", isMatchmakingGame, playlistID, restrictRank, requireRank);
	
	m_requestedGameSessionId = sessionID;

	const char*  pWarning = NULL;  // a NULL warning means allowed to join
#ifdef GAME_IS_CRYSIS2
	CPlayerProgression *pPlayerProgression = CPlayerProgression::GetInstance();

	const int  displayRank = (pPlayerProgression ? pPlayerProgression->GetData(EPP_DisplayRank) : 0);
	const int  reincarnations = (pPlayerProgression ? pPlayerProgression->GetData(EPP_Reincarnate) : 0);
	
	if (displayRank)
	{
	 	pWarning = ((!pWarning && restrictRank && ((displayRank > restrictRank) || (reincarnations > 0))) ? "RankTooHigh" : pWarning);
	 	pWarning = ((!pWarning && requireRank && ((displayRank < requireRank) && (reincarnations == 0))) ? "RankTooLow" : pWarning);
	}
#else
	const int  displayRank = 0;
#endif 

	if (!pWarning)	
	{
		if (isMatchmakingGame)
		{
#ifdef GAME_IS_CRYSIS2
			g_pGame->GetPlaylistManager()->ChoosePlaylistById(playlistID);
#endif
		}

		JoinGameSession(m_requestedGameSessionId, isMatchmakingGame);

#ifdef USE_C2_FRONTEND
		CFlashFrontEnd *pFlashFrontEnd = g_pGame->GetFlashMenu();
		if (pFlashFrontEnd)
		{
			pFlashFrontEnd->ClearPendingChangedSettings();
		}
#endif //#ifdef USE_C2_FRONTEND
	}
	else
	{
		CRY_ASSERT(pWarning);
		CryLog("  my rank (%d) out of variant rank range (%d to %d), leaving squad with warning \"%s\"", displayRank, requireRank, restrictRank, pWarning);
		RequestLeaveSquad();
		g_pGame->AddGameWarning("SquadManagerError", NULL);
		
#ifdef GAME_IS_CRYSIS2
		g_pGame->GetWarnings()->AddWarning(pWarning, NULL);		
#endif
	}
}
