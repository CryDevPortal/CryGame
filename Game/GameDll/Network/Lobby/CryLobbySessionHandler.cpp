/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: CryLobby session handler implementation.

-------------------------------------------------------------------------
History:
- 08:12:2009 : Created By Ben Johnson

*************************************************************************/
#include "StdAfx.h"
#include "CryLobbySessionHandler.h"
#include "Game.h"
#include "GameLobby.h"
#include "GameLobbyManager.h"

#include <INetwork.h>

#ifdef GAME_IS_CRYSIS2
#include "Frontend/Multiplayer/MPMenuHub.h"
#include "Frontend/ProfileOptions.h"
#include "PlayerProgression.h"
#endif

//-------------------------------------------------------------------------
CCryLobbySessionHandler::CCryLobbySessionHandler()
{
	g_pGame->GetIGameFramework()->SetGameSessionHandler(this);
	m_userQuit = false;
}

//-------------------------------------------------------------------------
CCryLobbySessionHandler::~CCryLobbySessionHandler()
{
	if(g_pGame)
		g_pGame->ClearGameSessionHandler(); // Must clear pointer in game if cry action deletes the handler.
}

//-------------------------------------------------------------------------
void CCryLobbySessionHandler::CreateSession(const SGameStartParams * pGameStartParams)
{
	m_userQuit = false;
	if( !(pGameStartParams->flags & eGSF_LocalOnly) && pGameStartParams->flags & eGSF_Server )
	{
		// Creates a session if necessary then calls StartGameContext
		CGameLobby* pGameLobby = g_pGame->GetGameLobby();
		if (pGameLobby)
		{
			pGameLobby->OnNewGameStartParams(pGameStartParams);
		}
	}
	else
	{
		// User tried to create a game while in multiplayer without specifying server
		CGameLobbyManager *pGameLobbyManager = g_pGame->GetGameLobbyManager();
		if (pGameLobbyManager)
		{
			pGameLobbyManager->LeaveGameSession(CGameLobbyManager::eLSR_Menu);
		}
		g_pGame->GetIGameFramework()->StartGameContext(pGameStartParams);
	}
}
//-------------------------------------------------------------------------
void CCryLobbySessionHandler::JoinSessionFromConsole(CrySessionID session)
{
	CGameLobby* pGameLobby = g_pGame->GetGameLobby();
	if (pGameLobby)
	{
		pGameLobby->JoinServer(session, "JoinSessionFromConsole", CryMatchMakingInvalidConnectionUID, true);
	}
}

//-------------------------------------------------------------------------
void CCryLobbySessionHandler::LeaveSession()
{
	CGameLobbyManager* pGameLobbyManager = g_pGame->GetGameLobbyManager();
	if (pGameLobbyManager)
	{
		pGameLobbyManager->LeaveGameSession(CGameLobbyManager::eLSR_Menu);
	}
}

//-------------------------------------------------------------------------
int CCryLobbySessionHandler::StartSession()
{







	m_userQuit = false;
	return (int) eCLE_Success;
}

//-------------------------------------------------------------------------
int CCryLobbySessionHandler::EndSession()
{
	if (IPlayerProfileManager *pPlayerProfileManager = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager())
	{




#ifdef GAME_IS_CRYSIS2
		CPlayerProgression *pPlayerProgression = CPlayerProgression::GetInstance();
		if (pPlayerProgression)
		{
			pPlayerProgression->OnEndSession();
		}
#endif

		CGameLobbyManager *pLobbyManager = g_pGame->GetGameLobbyManager();
		if (pLobbyManager)
		{
			const unsigned int controllerIndex = pPlayerProfileManager->GetExclusiveControllerDeviceIndex();
			if (pLobbyManager->GetOnlineState(controllerIndex) == eOS_SignedIn)
			{
				CryLog("CCryLobbySessionHandler::EndSession() saving profile");
				//Quitting the session from in game
#ifdef GAME_IS_CRYSIS2
				g_pGame->GetProfileOptions()->SaveProfile(ePR_All);
#endif
			}
			else
			{
				CryLog("CCryLobbySessionHandler::EndSession() not saving as we're signed out");
			}
		}
	}

	return (int) eCLE_Success;
}

//-------------------------------------------------------------------------
void CCryLobbySessionHandler::OnUserQuit()
{
	m_userQuit = true;

	if (g_pGame->GetIGameFramework()->StartedGameContext() == false)
	{
		g_pGame->GetGameLobbyManager()->LeaveGameSession(CGameLobbyManager::eLSR_Menu);
	}
}

//-------------------------------------------------------------------------
void CCryLobbySessionHandler::OnGameShutdown()
{
	const CGame::EHostMigrationState  migrationState = (g_pGame ? g_pGame->GetHostMigrationState() : CGame::eHMS_NotMigrating);

	CryLog("CCryLobbySessionHandler::OnGameShutdown(), m_userQuit=%s, migrationState=%d", (m_userQuit ? "true" : "false"), migrationState);

#ifdef USE_C2_FRONTEND
	CMPMenuHub* pMPMenu = NULL;
	CFlashFrontEnd *pFlashFrontEnd = g_pGame ? g_pGame->GetFlashMenu() : NULL;
	if (pFlashFrontEnd)
	{
		pMPMenu = pFlashFrontEnd->GetMPMenu();
	}
#endif //#ifdef USE_C2_FRONTEND

	if(m_userQuit)
	{
		LeaveSession();
	}

#ifdef USE_C2_FRONTEND
	if (pMPMenu)
	{
		// If we're still on the loading screen, clear it
		pMPMenu->ClearLoadingScreen();
	}
#endif //#ifdef USE_C2_FRONTEND
}

//-------------------------------------------------------------------------
CrySessionHandle CCryLobbySessionHandler::GetGameSessionHandle() const
{
	CrySessionHandle result = CrySessionInvalidHandle;

	CGameLobby* pGameLobby = g_pGame->GetGameLobby();
	if (pGameLobby)
	{
		result = pGameLobby->GetCurrentSessionHandle();
	}

	return result;
}

//-------------------------------------------------------------------------
bool CCryLobbySessionHandler::ShouldCallMapCommand( const char *pLevelName, const char *pGameRules )
{
	bool result = false;

	CGameLobby* pGameLobby = g_pGame->GetGameLobby();
	if (pGameLobby)
	{
		result = pGameLobby->ShouldCallMapCommand(pLevelName, pGameRules);
	}

	return result;
}

//-------------------------------------------------------------------------
bool CCryLobbySessionHandler::ShouldMigrateNub() const
{
	bool bResult = true;

	CGameLobby* pGameLobby = g_pGame->GetGameLobby();
	if (pGameLobby)
	{
		bResult = (pGameLobby->GetState() == eLS_Game);
	}

	return bResult;
}

//-------------------------------------------------------------------------
bool CCryLobbySessionHandler::IsMultiplayer() const
{
	bool result = false;

	CGameLobbyManager *pLobbyManager = g_pGame->GetGameLobbyManager();
	if (pLobbyManager)
	{
		result = pLobbyManager->IsMultiplayer();
	}

	return result;
}

//-------------------------------------------------------------------------
int CCryLobbySessionHandler::GetNumberOfExpectedClients()
{
	int result = 0;

	CGameLobby* pGameLobby = g_pGame->GetGameLobby();
	if (pGameLobby)
	{
		result = pGameLobby->GetNumberOfExpectedClients();
	}

	return result;
}

//-------------------------------------------------------------------------
bool CCryLobbySessionHandler::IsGameSessionMigrating() const
{
	return g_pGame->IsGameSessionHostMigrating();
}

//-------------------------------------------------------------------------
bool CCryLobbySessionHandler::IsMidGameLeaving() const
{
	bool result = false;

	const CGameLobby* pGameLobby = g_pGame->GetGameLobby();
	if (pGameLobby)
	{
		result = pGameLobby->IsMidGameLeaving();
	}

	return result;
}
