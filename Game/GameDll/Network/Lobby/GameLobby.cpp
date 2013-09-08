#include "StdAfx.h"

#include "GameRules.h"
#include "GameLobby.h"
#include "GameLobbyManager.h"
#include "GameBrowser.h"
#include "GameServerLists.h"

#include "ICryLobbyUI.h"
#include "ICryMatchMaking.h"
#include "ICryVoice.h"
#include "IGameFramework.h"
#include "IGameRulesSystem.h"
#include "IGameSessionHandler.h"
#include "ICryStats.h"

#include "Game.h"
#ifdef GAME_IS_CRYSIS2
#include "FrontEnd/FlashFrontEnd.h"
#include "FrontEnd/FrontEndDefines.h"
#include "FrontEnd/Multiplayer/MPMenuHub.h"
#include "FrontEnd/Multiplayer/UIServerList.h"
#include "FrontEnd/Multiplayer/UIScoreboard.h"
#endif
#include "Network/Squad/SquadManager.h"
#include "Endian.h"

#include "GameCVars.h"
#include "Utility/StringUtils.h"
#include "Utility/CryWatch.h"
#include "Utility/CryHash.h"
#ifdef GAME_IS_CRYSIS2
#include "GameCodeCoverage/GameCodeCoverageTracker.h"
#include "PlayerProgression.h"
#include "DogTagProgression.h"
#include "GameRules.h"
#include "GameRulesModules/GameRulesModulesManager.h"
#include "GameRulesModules/IGameRulesStateModule.h"
#include "GameRulesModules/IGameRulesRoundsModule.h"
#include "GameRulesModules/IGameRulesPlayerStatsModule.h"
#include "HUD/HUD.h"
#include "HUD/HUDEventWrapper.h"
#include "HUD/HUDUtils.h"
#include "HUD/HUDCVars.h"

#include "PlaylistManager.h"
#include "Frontend/ProfileOptions.h"
#endif

#include "TypeInfo_impl.h"

#include "Network/GameNetworkUtils.h"
#ifdef GAME_IS_CRYSIS2
#include "PersistantStats.h"
#endif

#include "Player.h"
#include "MatchMakingTelemetry.h"
#include "MatchMakingHandler.h"
#ifdef GAME_IS_CRYSIS2
#include "Network/RecordingSystem.h"
#include "FrontEnd/WarningsManager.h"
#include "UI/UIManager.h"
#endif

#include <IPlatformOS.h>






	#define GAME_LOBBY_DO_ENSURE_BEST_HOST		0
	#define GAME_LOBBY_DO_LOBBY_MERGING				1
	#define GAME_LOBBY_IGNORE_BAD_SERVERS_LIST 0


#define GAME_LOBBY_DEDICATED_SERVER_DOES_MERGING 0

#define MAX_WRITEUSERDATA_USERS 6


#define ORIGINAL_MATCHMAKING_DESC "C2 Release Matchmaking"

#if !defined(_RELEASE)
static int gl_debug = 0;
static int gl_voteDebug = 0;
static int gl_voip_debug = 0;
static int gl_dummyUserlist = 0;
static int gl_dummyUserlistNumTeams = 3;
static int gl_debugBadServersList = 0;
static int gl_debugBadServersTestPerc = 50;
static int gl_debugForceLobbyMigrations = 0;
static float gl_debugForceLobbyMigrationsTimer = 12.5f;
static int gl_debugLobbyRejoin = 0;
static float gl_debugLobbyRejoinTimer = 25.f;
static float gl_debugLobbyRejoinRandomTimer = 2.f;

static int gl_debugLobbyBreaksGeneral = 0;
static int gl_debugLobbyHMAttempts = 0;
static int gl_debugLobbyHMTerminations = 0;
static int gl_debugLobbyBreaksHMShard = 0;
static int gl_debugLobbyBreaksHMHints = 0;
static int gl_debugLobbyBreaksHMTasks = 0;
static int gl_lobbyForceShowTeams = 0;

static int gl_searchSimulatorEnabled = 0;
#endif

static float gl_initialTime = 10.0f;
static float gl_findGameTimeoutBase = 3.0f;
static float gl_findGameTimeoutPerPlayer = 5.0f;
static float gl_findGameTimeoutRandomRange = 5.0f;
static float gl_svReturnToLobbyWait = 10.0f;
static float gl_slotReservationTimeout = 5.0f;
static float gl_leaveGameTimeout = 10.f;
static int gl_ignoreBadServers = GAME_LOBBY_IGNORE_BAD_SERVERS_LIST;
static int gl_allowLobbyMerging = 1;
static int gl_allowEnsureBestHostCalls = 1;
static float gl_timeTillEndOfGameForNoMatchMaking = 120.f;
static float gl_timeBeforeStartOfGameForNoMatchMaking = 5.f;
static float gl_skillChangeUpdateDelayTime = 3.f;
static float gl_gameTimeRemainingRestrictLoad = 30.0f;
static float gl_startTimerMinTimeAfterPlayerJoined = 10.f;
static int gl_startTimerMaxPlayerJoinResets = 8;

static int gl_findGameNumJoinRetries = 2;
static float gl_findGamePingMultiplyer = 2.f;
static float gl_findGamePlayerMultiplyer = 1.f;
static float gl_findGameLobbyMultiplyer = 0.25f;
static float gl_findGameSkillMultiplyer = 1.f;
static float gl_findGameLanguageMultiplyer = 0.5f;
static float gl_findGameRandomMultiplyer = 0.5f;
static float gl_findGamePingScale = 300.f;
static float gl_findGameIdealPlayerCount = 6.f;

static float gl_hostMigrationEnsureBestHostDelayTime = 10.f;
static float gl_hostMigrationEnsureBestHostOnStartCountdownDelayTime = 3.f;
static float gl_hostMigrationEnsureBestHostOnReturnToLobbyDelayTime = 5.f;
static float gl_hostMigrationEnsureBestHostGameStartMinimumTime = 4.f;

#if INCLUDE_DEDICATED_LEADERBOARDS
static int gl_maxGetOnlineDataRetries = 10;
#endif

#ifndef _RELEASE
static int gl_skipPreSearch = 0;
#endif

#ifndef _RELEASE
	#define DEBUG_TEAM_BALANCING 1
#else
	#define DEBUG_TEAM_BALANCING 0
#endif

#define DEBUG_TEAM_BALANCING_USERS DEBUG_TEAM_BALANCING
#define DEBUG_TEAM_BALANCING_CLANS DEBUG_TEAM_BALANCING
#define DEBUG_TEAM_BALANCING_SQUADS DEBUG_TEAM_BALANCING

#if DEBUG_TEAM_BALANCING
	#define TEAM_BALANCING_LOG(...)	CryLog(__VA_ARGS__);
#else
	#define TEAM_BALANCING_LOG(...) {}
#endif

#if DEBUG_TEAM_BALANCING_USERS
	#define TEAM_BALANCING_USERS_LOG(...)	CryLog(__VA_ARGS__);
#else
	#define TEAM_BALANCING_USERS_LOG(...) {}
#endif

#if DEBUG_TEAM_BALANCING_CLANS
	#define TEAM_BALANCING_CLANS_LOG(...)	CryLog(__VA_ARGS__);
#else
	#define TEAM_BALANCING_CLANS_LOG(...) {}
#endif

#if DEBUG_TEAM_BALANCING_SQUADS
	#define TEAM_BALANCING_SQUADS_LOG(...)	CryLog(__VA_ARGS__);
#else
	#define TEAM_BALANCING_SQUADS_LOG(...) {}
#endif

#define VOTING_EXTRA_DEBUG_OUTPUT  (0 && !defined(_RELEASE))

#if VOTING_EXTRA_DEBUG_OUTPUT
	#define VOTING_DBG_LOG(...)		CryLog(__VA_ARGS__);
	#define VOTING_DBG_WATCH(...)	CryWatch(__VA_ARGS__);
#else
	#define VOTING_DBG_LOG(...)		{}
	#define VOTING_DBG_WATCH(...)	{}
#endif

#ifndef _RELEASE
static bool s_testing = false;

void CGameLobby::CmdTestTeamBalancing(IConsoleCmdArgs *pArgs)
{
	// Function for testing the team balancing

	s_testing = true;

	struct STestUserId : public SCryUserID
	{
		STestUserId(int id)
		{
			m_id = id;
		}

		virtual bool operator == (const SCryUserID &other) const
		{
			const STestUserId &rhs = static_cast<const STestUserId&>(other);
			return m_id == rhs.m_id;
		}

		virtual bool operator < (const SCryUserID &other) const
		{
			const STestUserId &rhs = static_cast<const STestUserId&>(other);
			return m_id < rhs.m_id;
		}

		int m_id;
	};

	CGameLobby *pLobby = g_pGame->GetGameLobby();

#define ADD_FAKE_USER(name, skill) \
	{ \
		SCryUserInfoResult fakeUser;	\
		fakeUser.m_conID.m_uid = nextUserId;	\
		fakeUser.m_conID.m_sid = nextUserId;	\
		fakeUser.m_userID = new STestUserId(nextUserId);	\
		cry_sprintf(fakeUser.m_userName, sizeof(fakeUser.m_userName), name);	\
		memset(fakeUser.m_userData, 0, sizeof(fakeUser.m_userData));	\
		fakeUser.m_userData[eLUD_SkillRank1] = skill & 0xff;	\
		fakeUser.m_userData[eLUD_SkillRank2] = (skill >> 8) & 0xff;	\
		pLobby->InsertUser(&fakeUser); \
		++ nextUserId; \
	}

#define REMOVE_FAKE_USERS \
	{ \
		while(nextUserId > 1) \
		{ \
			--nextUserId; \
			SCryUserInfoResult fakeUser;	\
			fakeUser.m_conID.m_uid = nextUserId;	\
			fakeUser.m_conID.m_sid = nextUserId;	\
			pLobby->RemoveUser(&fakeUser); \
		} \
	}

#define ADD_TO_SQUAD(userId, hostId) \
	{ \
		SCryMatchMakingConnectionUID userUID; \
		userUID.m_uid = userId; \
		userUID.m_sid = userId; \
		SSessionNames::SSessionName *pPlayer = pLobby->m_nameList.GetSessionName(userUID, false); \
		SCryUserInfoResult updateInfo; \
		updateInfo.m_conID = pPlayer->m_conId; \
		updateInfo.m_userID = pPlayer->m_userId; \
		memcpy(updateInfo.m_userName, pPlayer->m_name, CRYLOBBY_USER_NAME_LENGTH); \
		memcpy(updateInfo.m_userData, pPlayer->m_userData, CRYLOBBY_USER_DATA_SIZE_IN_BYTES); \
		updateInfo.m_userData[eLUD_SquadId1] = uint8(hostId & 0xFF); \
		updateInfo.m_userData[eLUD_SquadId2] = uint8((hostId >> 8) & 0xFF); \
		pLobby->UpdateUser(&updateInfo); \
	}

#define ADD_TO_CLAN(userId, clanName) \
	{ \
		SCryMatchMakingConnectionUID userUID; \
		userUID.m_uid = userId; \
		userUID.m_sid = userId; \
		SSessionNames::SSessionName *pPlayer = pLobby->m_nameList.GetSessionName(userUID, false); \
		SCryUserInfoResult updateInfo; \
		updateInfo.m_conID = pPlayer->m_conId; \
		updateInfo.m_userID = pPlayer->m_userId; \
		memcpy(updateInfo.m_userName, pPlayer->m_name, CRYLOBBY_USER_NAME_LENGTH); \
		memcpy(updateInfo.m_userData, pPlayer->m_userData, CRYLOBBY_USER_DATA_SIZE_IN_BYTES); \
		memset(&updateInfo.m_userData[eLUD_ClanTag1], 0, 4); \
		for (int i = 0; i < 4; ++ i) \
		{ \
			updateInfo.m_userData[int(eLUD_ClanTag1) + i] = clanName[i]; \
			if (!clanName[i]) \
				break; \
		} \
		pLobby->UpdateUser(&updateInfo); \
	}

#define UPDATE_SKILL(userId, skill) \
	{ \
		SCryMatchMakingConnectionUID userUID; \
		userUID.m_uid = userId; \
		userUID.m_sid = userId; \
		SSessionNames::SSessionName *pPlayer = pLobby->m_nameList.GetSessionName(userUID, false); \
		SCryUserInfoResult updateInfo; \
		updateInfo.m_conID = pPlayer->m_conId; \
		updateInfo.m_userID = pPlayer->m_userId; \
		memcpy(updateInfo.m_userName, pPlayer->m_name, CRYLOBBY_USER_NAME_LENGTH); \
		memcpy(updateInfo.m_userData, pPlayer->m_userData, CRYLOBBY_USER_DATA_SIZE_IN_BYTES); \
		updateInfo.m_userData[eLUD_SkillRank1] = skill & 0xff;	\
		updateInfo.m_userData[eLUD_SkillRank2] = (skill >> 8) & 0xff;	\
		pLobby->UpdateUser(&updateInfo); \
	}

	int nextUserId = 1;

	ADD_FAKE_USER("fake1", 2);
	ADD_FAKE_USER("fake2", 12);
	ADD_FAKE_USER("fake3", 2);
	ADD_FAKE_USER("fake4", 6);
	ADD_FAKE_USER("fake5", 3);
	ADD_FAKE_USER("fake6", 1);
	ADD_FAKE_USER("fake7", 1);
	ADD_FAKE_USER("fake8", 5);
	ADD_FAKE_USER("fake9", 18);
	ADD_FAKE_USER("fake10", 18);
	ADD_FAKE_USER("fake11", 4);
	ADD_FAKE_USER("fake12", 7);

	ADD_TO_SQUAD(2, 1);
	ADD_TO_SQUAD(1, 1);

	ADD_TO_CLAN(1, "bob");
	ADD_TO_CLAN(5, "bob");
	ADD_TO_CLAN(7, "bob");
	ADD_TO_CLAN(10, "fred");
	ADD_TO_CLAN(11, "fred");
	ADD_TO_CLAN(12, "fred");

	UPDATE_SKILL(5, 10);
	UPDATE_SKILL(7, 2);

	pLobby->BalanceTeams();
	
	// Remove fake users
	REMOVE_FAKE_USERS;

	s_testing = false;

#undef ADD_FAKE_USER
#undef ADD_TO_SQUAD
#undef ADD_TO_CLAN
#undef UPDATE_SKILL
#undef REMOVE_FAKE_USERS
}

//-------------------------------------------------------------------------
void CGameLobby::CmdTestMuteTeam(IConsoleCmdArgs *pArgs)
{
	if (pArgs->GetArgCount() == 3)
	{
		uint8 teamId = uint8(atoi(pArgs->GetArg(1)));
		bool mute = (atoi(pArgs->GetArg(2)) != 0);

		CGameLobby *pLobby = g_pGame->GetGameLobby();

		CryLog("%s all players on team %i", (mute ? "Muting" : "Un-Muting"), teamId);
		pLobby->MutePlayersOnTeam(teamId, mute);
	}
	else
	{
		CryLog("usage: g_testMuteTeam <teamId (1 or 2)> <mute (1 or 0)>");
	}
}

//-------------------------------------------------------------------------
void CGameLobby::CmdCallEnsureBestHost(IConsoleCmdArgs *pArgs)
{
#if GAME_LOBBY_DO_ENSURE_BEST_HOST
	CGameLobby *pLobby = g_pGame->GetGameLobby();
	if (pLobby)
	{
		if (!g_pGameCVars->g_hostMigrationUseAutoLobbyMigrateInPrivateGames)
		{
			// Force set the g_hostMigrationUseAutoLobbyMigrateInPrivateGames cvar so that the command works in private games
			g_pGameCVars->g_hostMigrationUseAutoLobbyMigrateInPrivateGames = 1;
		}

		pLobby->m_taskQueue.AddTask(CLobbyTaskQueue::eST_EnsureBestHost, true);
	}
#endif
}

//-------------------------------------------------------------------------
void CGameLobby::CmdFillReservationSlots(IConsoleCmdArgs *pArgs)
{
	CGameLobby *pLobby = g_pGame->GetGameLobby();
	if (pLobby)
	{
		const float currentTime = gEnv->pTimer->GetAsyncCurTime();
		for (int i = 0; i < ARRAY_COUNT(pLobby->m_slotReservations); ++ i)
		{
			pLobby->m_slotReservations[i].m_con.m_uid = (i + 1);
			pLobby->m_slotReservations[i].m_con.m_sid = (i + 1);
			pLobby->m_slotReservations[i].m_timeStamp = currentTime;
		}
	}
}

#endif

//-------------------------------------------------------------------------

namespace GameLobbyUtils
{
	bool IsValidMap(CGameLobby *pGameLobby, ILevelInfo *pLevelInfo, const char *pGameRules, string &outResult)
	{
		outResult = PathUtil::GetFileName(pLevelInfo->GetName());

		CryFixedStringT<32> mapName = pGameLobby->GetValidMapForGameRules(outResult.c_str(), pGameRules, false);

		return (!mapName.empty());
	}
};

/// Used by console auto completion.
struct SGLLevelNameAutoComplete : public IConsoleArgumentAutoComplete
{
	virtual int GetCount() const 
	{ 
		ILevelSystem* pLevelSystem = g_pGame->GetIGameFramework()->GetILevelSystem();

		const int numLevels = pLevelSystem->GetLevelCount();

		int numMPLevels = 0;

		CGameLobby *pGameLobby = g_pGame->GetGameLobby();
		if (pGameLobby)
		{
			const char *pGameRules = pGameLobby->GetCurrentGameModeName(NULL);
			if (pGameRules)
			{
				for (int i = 0; i < numLevels; ++ i)
				{
					ILevelInfo* pLevelInfo = pLevelSystem->GetLevelInfo(i);
					static string strResult;
					if (GameLobbyUtils::IsValidMap(pGameLobby, pLevelInfo, pGameRules, strResult))
					{
						++ numMPLevels;
					}
				}
			}
		}

		return numMPLevels;
	}

	virtual const char* GetValue( int nIndex ) const 
	{
		ILevelSystem* pLevelSystem = g_pGame->GetIGameFramework()->GetILevelSystem();

		const int numLevels = pLevelSystem->GetLevelCount();

		int numFoundMPLevels = 0;

		// This is slow but it's only used in response to the user hitting tab while doing a gl_map command on the console
		CGameLobby *pGameLobby = g_pGame->GetGameLobby();
		if (pGameLobby)
		{
			const char *pGameRules = pGameLobby->GetCurrentGameModeName(NULL);
			if (pGameRules)
			{
				for (int i = 0; i < numLevels; ++ i)
				{
					ILevelInfo* pLevelInfo = pLevelSystem->GetLevelInfo(i);
					static string strResult;

					if (GameLobbyUtils::IsValidMap(pGameLobby, pLevelInfo, pGameRules, strResult))
					{
						++ numFoundMPLevels;
						if (numFoundMPLevels > nIndex)
						{
							return strResult.c_str();
						}
					}
				}
			}
		}
		return "";
	};
};
// definition and declaration must be separated for devirtualization
SGLLevelNameAutoComplete gl_LevelNameAutoComplete;

//-------------------------------------------------------------------------
void CGameLobby::CmdDumpValidMaps(IConsoleCmdArgs *pArgs)
{
	ICVar *pGameRulesCVar = gEnv->pConsole->GetCVar("sv_gamerules");
	if (pGameRulesCVar)
	{
		const char *pGameRules = pGameRulesCVar->GetString();
		IGameRulesSystem *pGameRulesSystem = g_pGame->GetIGameFramework()->GetIGameRulesSystem();

		pGameRules = pGameRulesSystem->GetGameRulesName(pGameRules);
		if (pGameRules)
		{
			CGameLobby *pGameLobby = g_pGame->GetGameLobby();
			if (pGameLobby)
			{
				ILevelSystem* pLevelSystem = g_pGame->GetIGameFramework()->GetILevelSystem();
				const int numLevels = pLevelSystem->GetLevelCount();

				for (int i = 0; i < numLevels; ++ i)
				{
					ILevelInfo* pLevelInfo = pLevelSystem->GetLevelInfo(i);
					static string strResult;

					if (GameLobbyUtils::IsValidMap(pGameLobby, pLevelInfo, pGameRules, strResult))
					{
						// This is a reply to a console command, should be CryLogAlways
						CryLogAlways(strResult.c_str());
					}
				}
			}
		}
	}
}

//-------------------------------------------------------------------------

#define ONLINE_STATS_FRAGMENTED_PACKET_SIZE 1175	// need breathing room, was getting fragmented packets again :(

//-------------------------------------------------------------------------
#ifdef USE_C2_FRONTEND
void CGameLobby::SFlashChatMessage::SendChatMessageToFlash(IFlashPlayer *pFlashPlayer, bool isInit)
{
	if (m_message.empty()==false)
	{
		SFlashVarValue pArgs[] = {
			m_name.c_str(),
			m_message.c_str(),
			CHAT_MESSAGE_POSTFIX,
			m_local,
			isInit };

			pFlashPlayer->Invoke(FRONTEND_SUBSCREEN_PATH_SET("AddChatMessage"), pArgs, ARRAY_COUNT(pArgs));
	}
}
#endif //#ifdef USE_C2_FRONTEND
//-------------------------------------------------------------------------

static uint32 GameLobby_GetCurrentUserIndex()
{
	IPlayerProfileManager *pPlayerProfileManager = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager();
	return pPlayerProfileManager ? pPlayerProfileManager->GetExclusiveControllerDeviceIndex() : 0;
}


volatile bool CGameLobby::s_bShouldBeSearching = false;
int CGameLobby::s_currentMMSearchID = 0;

#if !defined(_RELEASE)
unsigned int CGameLobby::s_mainThreadHandle;
#endif

//-------------------------------------------------------------------------
CGameLobby::CGameLobby( CGameLobbyManager* pMgr )
{
	CryLog("CGameLobby::CGameLobby()");

#if !defined(_RELEASE)
	s_mainThreadHandle = CryGetCurrentThreadId();
#endif

#ifndef _RELEASE
	gEnv->pConsole->AddCommand("g_testTeamBalancing", CmdTestTeamBalancing);
	gEnv->pConsole->AddCommand("g_testMuteTeam", CmdTestMuteTeam);
	gEnv->pConsole->AddCommand("g_callEnsureBestHost", CmdCallEnsureBestHost);
	gEnv->pConsole->AddCommand("g_fillReservationSlots", CmdFillReservationSlots);
#endif
	gEnv->pConsole->AddCommand("gl_advancePlaylist", CmdAdvancePlaylist);
	gEnv->pConsole->AddCommand("gl_dumpValidMaps", CmdDumpValidMaps);

	m_gameStartParams = NULL;

	m_hasReceivedMapCommand = false;
	
	m_startTimer = gl_initialTime;
	m_findGameTimeout = GetFindGameTimeout();
	m_lastUserListUpdateTime = 0.f;
	m_timeTillCallToEnsureBestHost = -1.f;
	m_hasReceivedSessionQueryCallback = false;
	m_hasReceivedStartCountdownPacket = false;
	m_hasReceivedPlaylistSync = false;
	m_gameHadStartedWhenPlaylistRotationSynced = false;
	m_startTimerCountdown = false;
	m_initialStartTimerCountdown = false;
	m_joinCommand[0] = '\0';
	m_findGameResults.clear();
	m_currentSessionId = CrySessionInvalidID;
	m_nextSessionId = CrySessionInvalidID;
	m_autoVOIPMutingType = eLAVT_start;
	m_localVoteStatus = eLVS_awaitingCandidates;
	VOTING_DBG_LOG("[tlh] set m_localVoteStatus [1] to eLVS_awaitingCandidates");
	m_shouldFindGames = false;
	m_privateGame = false;
	m_passwordGame = false;
	CRY_ASSERT(pMgr);
	m_gameLobbyMgr = pMgr;
	m_reservationList = NULL;
	m_squadReservation = false;
	m_bMigratedSession = false;
	m_bSessionStarted = false;
	m_bCancelling = false;
	m_bQuitting = false;
	m_bNeedToSetAsElegibleForHostMigration = false;
	m_bPlaylistHasBeenAdvancedThroughConsole = false;
	m_networkedVoteStatus = eLNVS_NotVoted;
	m_lastActiveStatus = eAS_Lobby;
	m_timeTillUpdateSession = 0.f;
	m_bSkipCountdown = false;
	m_bServerUnloadRequired = false;
	m_bChoosingGamemode = false;
	m_bHasUserList = false;
	m_isMidGameLeaving = false;
	m_findGameNumRetries = 0;
	m_numPlayerJoinResets = 0;
	m_bRetryIfPassworded = false;

	for (int i=0; i<ARRAY_COUNT(m_slotReservations); i++)
	{
		m_slotReservations[i].m_con = CryMatchMakingInvalidConnectionUID;
		m_slotReservations[i].m_timeStamp = 0.f;
	}

	ClearChatMessages();

	m_votingFlashInfo.Reset();
	m_votingCandidatesFlashInfo.Reset();
	m_bMatchmakingSession = false;
	m_bWaitingForGameToFinish = false;
	m_allowRemoveUsers = true;
	m_bHasReceivedVoteOptions = false;

	// SInternalLobbyData
	m_server = false;
	m_state = eLS_None;
	m_requestedState = eLS_None;
	m_currentSession = CrySessionInvalidHandle;
	m_nameList.Clear();
	m_sessionFavouriteKeyId = INVALID_SESSION_FAVOURITE_ID;
	m_endGameResponses = 0;
	m_sessionUserDataDirty = false;
	m_needsTeamBalancing = false;
	m_squadDirty = false;
	m_leftVoteChoice.Reset();
	m_rightVoteChoice.Reset();
	m_highestLeadingVotesSoFar = 0;
	m_leftHadHighestLeadingVotes = false;
	m_votingEnabled = false;
	m_votingClosed = false;
	m_leftWinsVotes = false;
	m_isLeaving = false;
	m_pendingConnectSessionId = CrySessionInvalidID;
	m_leaveGameTimeout = 0.f;
	m_pendingReservationId = CryMatchMakingInvalidConnectionUID;
	m_currentTaskId = CryLobbyInvalidTaskID;
	m_stateHasChanged = false;
	m_playListSeed = 0;

	memset(&m_sessionData, 0, sizeof(m_sessionData));
	memset(&m_userData, 0, sizeof(m_userData));
	memset(&m_detailedServerInfo, 0, sizeof(m_detailedServerInfo));

	ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
	pLobby->RegisterEventInterest(eCLSE_UserPacket, CGameLobby::MatchmakingSessionUserPacketCallback, this);
	pLobby->RegisterEventInterest(eCLSE_RoomOwnerChanged, CGameLobby::MatchmakingSessionRoomOwnerChangedCallback, this);
	pLobby->RegisterEventInterest(eCLSE_SessionUserJoin, CGameLobby::MatchmakingSessionJoinUserCallback, this);
	pLobby->RegisterEventInterest(eCLSE_SessionUserLeave, CGameLobby::MatchmakingSessionLeaveUserCallback, this);
	pLobby->RegisterEventInterest(eCLSE_SessionUserUpdate, CGameLobby::MatchmakingSessionUpdateUserCallback, this);
	pLobby->RegisterEventInterest(eCLSE_SessionClosed, CGameLobby::MatchmakingSessionClosedCallback, this);
	pLobby->RegisterEventInterest(eCLSE_KickedFromSession, CGameLobby::MatchmakingSessionKickedCallback, this);
	pLobby->RegisterEventInterest(eCLSE_ForcedFromRoom, CGameLobby::MatchmakingForcedFromRoomCallback, this);
	pLobby->RegisterEventInterest(eCLSE_SessionRequestInfo, CGameLobby::MatchmakingSessionDetailedInfoRequestCallback, this);
	gEnv->pNetwork->AddHostMigrationEventListener(this, "GameLobby");

	REGISTER_CVAR(gl_initialTime, gl_initialTime, 0, "How long you spend in the lobby on a newly created lobby");
	REGISTER_CVAR(gl_findGameTimeoutBase, gl_findGameTimeoutBase, 0, "How long to wait for results when finding a game");
	REGISTER_CVAR(gl_findGameTimeoutPerPlayer, gl_findGameTimeoutPerPlayer, 0, "Extension to findGameTimeout for each player in session");
	REGISTER_CVAR(gl_findGameTimeoutRandomRange, gl_findGameTimeoutRandomRange, 0, "Randomization for the findGameTimeout");
	REGISTER_CVAR(gl_leaveGameTimeout, gl_leaveGameTimeout, 0, "Timeout for waiting for other players to leave the game before leaving ourselves");
	REGISTER_CVAR(gl_ignoreBadServers, gl_ignoreBadServers, 0, "Don't ignore bad servers (ones we have failed to connect to before)");
	REGISTER_CVAR(gl_allowLobbyMerging, gl_allowLobbyMerging, 0, "Set to 0 to stop matchmaking games from attempting to merge");
	REGISTER_CVAR(gl_allowEnsureBestHostCalls, gl_allowEnsureBestHostCalls, 0, "Set to 0 to stop the game doing pushed lobby-migrations");
	REGISTER_CVAR(gl_timeTillEndOfGameForNoMatchMaking, gl_timeTillEndOfGameForNoMatchMaking, 0, "Amount of game time remaining in which no matchmaking should occur");
	REGISTER_CVAR(gl_timeBeforeStartOfGameForNoMatchMaking, gl_timeBeforeStartOfGameForNoMatchMaking , 0, "Amount of time at the end of the start countdown before we disable matchmaking (enabled after InGame is hit)");
	REGISTER_CVAR(gl_skillChangeUpdateDelayTime, gl_skillChangeUpdateDelayTime, 0, "Amount of time after detecting a change in skill ranking, before we call SessionUpdate");
	REGISTER_CVAR(gl_gameTimeRemainingRestrictLoad, gl_gameTimeRemainingRestrictLoad, 0, "Don't start loading a level if there's only a limited amount of time remaining");
	REGISTER_CVAR(gl_startTimerMinTimeAfterPlayerJoined , gl_startTimerMinTimeAfterPlayerJoined , 0, "Minimum time before a game can start after a player has joined");
	REGISTER_CVAR(gl_startTimerMaxPlayerJoinResets, gl_startTimerMaxPlayerJoinResets, 0, "Amount of times the start timer can be reset due to players joining before they are ignored");

	REGISTER_CVAR(gl_findGameNumJoinRetries, gl_findGameNumJoinRetries, 0, "Number of times to retry joining before creating our own game");
	REGISTER_CVAR(gl_findGamePingMultiplyer, gl_findGamePingMultiplyer, 0, "Multiplyer for ping submetric");
	REGISTER_CVAR(gl_findGamePlayerMultiplyer, gl_findGamePlayerMultiplyer, 0, "Multiplyer for player submetric");
	REGISTER_CVAR(gl_findGameLobbyMultiplyer, gl_findGameLobbyMultiplyer, 0, "Multiplyer for lobby state submetric");
	REGISTER_CVAR(gl_findGameSkillMultiplyer, gl_findGameSkillMultiplyer, 0, "Multiplyer for skill submetric");
	REGISTER_CVAR(gl_findGameLanguageMultiplyer, gl_findGameLanguageMultiplyer, 0, "Multiplyer for language submetric");
	REGISTER_CVAR(gl_findGameRandomMultiplyer , gl_findGameRandomMultiplyer , 0, "Multiplyer for random submetric");
	REGISTER_CVAR(gl_findGamePingScale, gl_findGamePingScale, 0, "Amount to divide the ping by before clamping to 0->1");
	REGISTER_CVAR(gl_findGameIdealPlayerCount, gl_findGameIdealPlayerCount, 0, "Minimum number of players required for full score in the player submetric");

	REGISTER_CVAR(gl_hostMigrationEnsureBestHostDelayTime, gl_hostMigrationEnsureBestHostDelayTime, 0, "Time after a player joins before we call ensurebesthost");
	REGISTER_CVAR(gl_hostMigrationEnsureBestHostOnStartCountdownDelayTime, gl_hostMigrationEnsureBestHostOnStartCountdownDelayTime, 0, "Time after the game countdown starts before we call ensurebesthost");
	REGISTER_CVAR(gl_hostMigrationEnsureBestHostOnReturnToLobbyDelayTime, gl_hostMigrationEnsureBestHostOnReturnToLobbyDelayTime, 0, "Time after the game countdown starts before we call ensurebesthost");
	REGISTER_CVAR(gl_hostMigrationEnsureBestHostGameStartMinimumTime, gl_hostMigrationEnsureBestHostGameStartMinimumTime, 0, "Minimum amount of time before the game starts that we're allowed to check for a better host");

#if INCLUDE_DEDICATED_LEADERBOARDS
	REGISTER_CVAR(gl_maxGetOnlineDataRetries, gl_maxGetOnlineDataRetries, 0, "Max number of retries allowed before server gives up trying to get user's online data");
#endif

#if !defined(_RELEASE)
	REGISTER_CVAR(gl_debug, gl_debug, 0, "Turn on some debugging");
	REGISTER_CVAR(gl_voteDebug, gl_voteDebug, 0, "Turn on some map vote debugging");
	REGISTER_CVAR(gl_voip_debug, gl_voip_debug, 0, "Turn on game lobby voice debug");
	REGISTER_CVAR(gl_skipPreSearch, gl_skipPreSearch, 0, "Just create game before start searching");
	REGISTER_CVAR(gl_slotReservationTimeout, gl_slotReservationTimeout, 0, "How long it takes for slot reservations to time out");
	REGISTER_CVAR(gl_dummyUserlist, gl_dummyUserlist, 0, "Number of debug dummy users to display.");
	REGISTER_CVAR(gl_dummyUserlistNumTeams, gl_dummyUserlistNumTeams, 0, "Sets the number of teams to split dummy players across, 0: Team 0,  1: Team 1,  2: Team 1&2,  3: Team 0,1&2.");
	REGISTER_CVAR(gl_debugBadServersList, gl_debugBadServersList, 0, "Set alternate servers to be bad");
	REGISTER_CVAR(gl_debugBadServersTestPerc, gl_debugBadServersTestPerc, 0, "Percentage chance of setting a bad server (if gl_debugBadServersList=1)");
	REGISTER_CVAR(gl_debugForceLobbyMigrations, gl_debugForceLobbyMigrations, 0, "1=Force a lobby migration every gl_debugForceLobbyMigrationsTimer seconds");
	REGISTER_CVAR(gl_debugForceLobbyMigrationsTimer, gl_debugForceLobbyMigrationsTimer, 0, "Time between forced lobby migrations, 2=Start the leave lobby timer after a host migration starts");
	REGISTER_CVAR(gl_debugLobbyRejoin, gl_debugLobbyRejoin, 0, "1=Leave the lobby and rejoin after gl_debugLobbyRejoinTimer seconds");
	REGISTER_CVAR(gl_debugLobbyRejoinTimer, gl_debugLobbyRejoinTimer, 0, "Time till auto leaving the session");
	REGISTER_CVAR(gl_debugLobbyRejoinRandomTimer, gl_debugLobbyRejoinRandomTimer, 0, "Random element to leave game timer");
	REGISTER_CVAR(gl_lobbyForceShowTeams, gl_lobbyForceShowTeams, 0, "Force the lobby to display teams all the time");

	REGISTER_CVAR(gl_debugLobbyBreaksGeneral, gl_debugLobbyBreaksGeneral, 0, "Counter: General lobby breaks");
	REGISTER_CVAR(gl_debugLobbyHMAttempts, gl_debugLobbyHMAttempts, 0, "Counter: host migration attempts");
	REGISTER_CVAR(gl_debugLobbyHMTerminations, gl_debugLobbyHMTerminations, 0, "Counter: host migration terminations");
	REGISTER_CVAR(gl_debugLobbyBreaksHMShard, gl_debugLobbyBreaksHMShard, 0, "Counter: Host Migration sharding detected in lobby");
	REGISTER_CVAR(gl_debugLobbyBreaksHMHints, gl_debugLobbyBreaksHMHints, 0, "Counter: Host Migration hinting error detected");
	REGISTER_CVAR(gl_debugLobbyBreaksHMTasks, gl_debugLobbyBreaksHMTasks, 0, "Counter: Host Migration task error detected");

#if defined(USE_SESSION_SEARCH_SIMULATOR)
	REGISTER_CVAR(gl_searchSimulatorEnabled, gl_searchSimulatorEnabled, 0, "Enable/Disable the Session Search Simulator for testing Matchmaking");
	gEnv->pConsole->RegisterString("gl_searchSimulatorFilepath", NULL, 0,"Set the source XML file for the Session Search Simulator");
#endif //defined(USE_SESSION_SEARCH_SIMULATOR)

	m_timeTillAutoLeaveLobby = 0.f;
	m_failedSearchCount = 0;
#endif //!defined(_RELEASE)

#if ENABLE_CHAT_MESSAGES
	gEnv->pConsole->AddCommand("gl_say", CmdChatMessage, 0, CVARHELP("Send a chat message"));
	gEnv->pConsole->AddCommand("gl_teamsay", CmdChatMessageTeam, 0, CVARHELP("Send a chat message to team"));
#endif

	gEnv->pConsole->AddCommand("gl_StartGame", CmdStartGame, 0, CVARHELP("force start a game"));
	gEnv->pConsole->AddCommand("gl_Map", CmdSetMap, 0, CVARHELP("Set map for the lobby"));
	gEnv->pConsole->AddCommand("gl_GameRules", CmdSetGameRules, 0, CVARHELP("Set the game rules for the lobby"));
	gEnv->pConsole->AddCommand("gl_Vote", CmdVote, 0, CVARHELP("Vote for next map in lobby (left or right)"));
	gEnv->pConsole->RegisterAutoComplete("gl_Map", &gl_LevelNameAutoComplete);

	m_isTeamGame = false;
#ifdef GAME_IS_CRYSIS2
	m_DLCServerStartWarningId = g_pGame->GetWarnings()->GetWarningId("DLCServerStartWarning");
#else
	m_DLCServerStartWarningId = 0;
#endif
	m_badServersHead = 0;

	m_taskQueue.Init(CGameLobby::TaskStartedCallback, this);

	m_startTimerLength = gl_initialTime;

	m_profanityTask = CryLobbyInvalidTaskID;

#if INCLUDE_DEDICATED_LEADERBOARDS
	m_WriteUserDataTaskID = CryLobbyInvalidTaskID;
	m_writeUserDataTimer = g_pGameCVars->g_writeUserDataInterval;	// alow first tick
#endif

#if !defined(_RELEASE)
	if (gl_debugForceLobbyMigrations)
	{
		gEnv->pConsole->GetCVar("net_hostHintingNATTypeOverride")->Set(1);
		gEnv->pConsole->GetCVar("net_hostHintingActiveConnectionsOverride")->Set(1);
		CryLog("setting net_hostHintingActiveConnectionsOverride to 1");
	}
#endif
}

//-------------------------------------------------------------------------
CGameLobby::~CGameLobby()
{
	CryLog("CGameLobby::~CGameLobby() canceling any pending tasks, Lobby: %p", this);
	CancelAllLobbyTasks();

	SAFE_DELETE(m_gameStartParams);

	ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
	pLobby->UnregisterEventInterest(eCLSE_UserPacket, CGameLobby::MatchmakingSessionUserPacketCallback, this);
	pLobby->UnregisterEventInterest(eCLSE_RoomOwnerChanged, CGameLobby::MatchmakingSessionRoomOwnerChangedCallback, this);
	pLobby->UnregisterEventInterest(eCLSE_SessionUserJoin, CGameLobby::MatchmakingSessionJoinUserCallback, this);
	pLobby->UnregisterEventInterest(eCLSE_SessionUserLeave, CGameLobby::MatchmakingSessionLeaveUserCallback, this);
	pLobby->UnregisterEventInterest(eCLSE_SessionUserUpdate, CGameLobby::MatchmakingSessionUpdateUserCallback, this);
	pLobby->UnregisterEventInterest(eCLSE_SessionClosed, CGameLobby::MatchmakingSessionClosedCallback, this);
	pLobby->UnregisterEventInterest(eCLSE_KickedFromSession, CGameLobby::MatchmakingSessionKickedCallback, this);
	pLobby->UnregisterEventInterest(eCLSE_ForcedFromRoom, CGameLobby::MatchmakingForcedFromRoomCallback, this);
	pLobby->UnregisterEventInterest(eCLSE_SessionRequestInfo, CGameLobby::MatchmakingSessionDetailedInfoRequestCallback, this);
	gEnv->pNetwork->RemoveHostMigrationEventListener(this);

	m_gameLobbyMgr = NULL;
}

//-------------------------------------------------------------------------
void CGameLobby::SvFinishedGame(const float dt)
{
	m_bServerUnloadRequired = true;

	CRY_ASSERT(m_server);
	if(m_state == eLS_Game)
	{
		SetState(eLS_EndSession);
		m_startTimer = 0.0f;
	}
	else if(m_state == eLS_GameEnded)
	{
		if(m_startTimer > 10.0f)
		{
			gEnv->pConsole->ExecuteString("unload", false, true);
			SetState(eLS_PostGame);
		}
		else
		{
			m_startTimer += dt;
		}
	}
}

//-------------------------------------------------------------------------
void CGameLobby::SetState(ELobbyState state)
{
	if(state != m_state)
	{
		CryLog("CGameLobby(%p)::SetState %d", this, state);
		INDENT_LOG_DURING_SCOPE();
		EnterState(m_state, state);
		m_stateHasChanged = true;
	}
}

//-------------------------------------------------------------------------
void CGameLobby::Update(float dt)
{
#if !defined(_RELEASE)
	if(gl_debug)
	{
		CryWatch("gameRules=%s, map=%s, private=%s passworded=%s", m_currentGameRules.c_str(), m_currentLevelName.c_str(), (IsPrivateGame()?"true":"false"), (IsPasswordedGame()?"true":"false"));
		CryWatch("StartedGameContext %d", g_pGame->GetIGameFramework()->StartedGameContext());
#ifdef GAME_IS_CRYSIS2
		if (CPlaylistManager* pPlaylistMan=g_pGame->GetPlaylistManager())
		{
			CryWatch("playlist set=%s, id=%u", (pPlaylistMan->HavePlaylistSet() ? "true" : "false"), (pPlaylistMan->GetCurrentPlaylist() ? pPlaylistMan->GetCurrentPlaylist()->id : 0));
		}
		else
		{
			CryWatch("playlist [manager=NULL]");
		}
#endif
	}

	if (gl_voip_debug)
	{
		switch (m_autoVOIPMutingType)
		{
			case eLAVT_off:					CryWatch("Current mute state eLAVT_off");											break;
			case eLAVT_allButParty:	CryWatch("Current mute state eLAVT_allButParty");							break;
			case eLAVT_all:					CryWatch("Current mute state eLAVT_all");											break;
			default:								CryWatch("Current mute state %d", (int)m_autoVOIPMutingType);	break;
		};
	}
#endif

#ifdef _DEBUG
	static float white[] = {1.0f,1.0f,1.0f,1.0f};
	float ypos = 200.0f;
	if (gEnv->pRenderer)
	{
		if (gl_debugForceLobbyMigrations)
		{
			gEnv->pRenderer->Draw2dLabel(100, ypos, 3.0f, white, false, "HOST MIGRATION:");
			ypos += 30.0f;
			gEnv->pRenderer->Draw2dLabel(125, ypos, 2.0f, white, false, "Attempts (%i) / Terminations (%i)", gl_debugLobbyHMAttempts, gl_debugLobbyHMTerminations);
			ypos += 20.0f;
		}

		if (gl_debugLobbyBreaksGeneral)
		{
			gEnv->pRenderer->Draw2dLabel(125, ypos, 2.0f, white, false, "HOST MIGRATION BREAKS DETECTED (%i)", gl_debugLobbyBreaksGeneral);
			ypos += 20.0f;
		}
		if (gl_debugLobbyBreaksHMShard)
		{
			gEnv->pRenderer->Draw2dLabel(125, ypos, 2.0f, white, false, "HOST MIGRATION SHARD BREAKS DETECTED (%i)", gl_debugLobbyBreaksHMShard);
			ypos += 20.0f;
		}
		if (gl_debugLobbyBreaksHMHints)
		{
			gEnv->pRenderer->Draw2dLabel(125, ypos, 2.0f, white, false, "HOST MIGRATION HINTS BREAKS DETECTED (%i)", gl_debugLobbyBreaksHMHints);
			ypos += 20.0f;
		}
		if (gl_debugLobbyBreaksHMTasks)
		{
			gEnv->pRenderer->Draw2dLabel(125, ypos, 2.0f, white, false, "HOST MIGRATION TASKS BREAKS DETECTED (%i)", gl_debugLobbyBreaksHMTasks);
			ypos += 20.0f;
		}
	}
#endif

#if INCLUDE_DEDICATED_LEADERBOARDS
	if(AllowOnlineAttributeTasks())
	{
		TickOnlineAttributeTasks();
		TickWriteUserData(dt);
	}
#endif

	UpdateState();

	m_taskQueue.Update();

	m_nameList.Tick(dt);

#if !defined(_RELEASE)
	if(gl_voip_debug)
	{
		const int nameSize = m_nameList.Size();
		for(int i = 0; i < nameSize; i++)
		{
			CryWatch("\t%d - %s [uid:%u] mute:%d", i + 1, m_nameList.m_sessionNames[i].m_name, m_nameList.m_sessionNames[i].m_conId.m_uid, m_nameList.m_sessionNames[i].m_muted);
			CryUserID userId = m_nameList.m_sessionNames[i].m_userId;
			if (userId.IsValid())
			{
				ICryVoice *pCryVoice = gEnv->pNetwork->GetLobby()->GetVoice();
				uint32 userIndex = GameLobby_GetCurrentUserIndex();
				if (pCryVoice)
				{
					CryWatch("\t\tIsMuted %d IsExMuted %d", pCryVoice->IsMuted(userIndex, userId)?1:0, pCryVoice->IsMutedExternally(userIndex, userId)?1:0);
				}
			}
		}
	}
#endif

	if (m_isLeaving)
	{
		m_leaveGameTimeout -= dt;
		if (m_leaveGameTimeout <= 0.f)
		{
			CryLog("CGameLobby::Update() leave game timeout has occurred");
			LeaveSession(true);
		}
		return;
	}

	if(m_state == eLS_GameEnded)
	{
		if(m_endGameResponses >= (int)m_nameList.Size() - 1)
		{
			if (m_bServerUnloadRequired)
			{
				gEnv->pConsole->ExecuteString("unload", false, true);
			}
			SetState(eLS_PostGame);
		}
	}

	if (m_server)
	{
		if (g_pGame->GetHostMigrationState() == CGame::eHMS_NotMigrating)
		{
			const EActiveStatus currentStatus = GetActiveStatus(m_state);
			if (m_lastActiveStatus != currentStatus)
			{
				m_taskQueue.AddTask(CLobbyTaskQueue::eST_Update, true);
				m_lastActiveStatus = currentStatus;
			}
		}
	}

	if(m_state == eLS_PostGame)
	{
		/*CFlashFrontEnd *pFlashFrontEnd = g_pGame->GetFlashMenu();
		if (pFlashFrontEnd)
		{
		pFlashFrontEnd->ScheduleInitialize(CFlashFrontEnd::eFlM_Menu, eFFES_game_lobby, eFFES_game_lobby);
		}*/
		SetState(eLS_Lobby);
	}

	if(m_state == eLS_Lobby)
	{
		CRY_ASSERT(m_currentSession != CrySessionInvalidHandle);

		eHostMigrationState hostMigrationState = eHMS_Unknown;
		ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
		if (pLobby)
		{
			ICryMatchMaking *pMatchmaking = pLobby->GetMatchMaking();
			if (pMatchmaking)
			{
				hostMigrationState = pMatchmaking->GetSessionHostMigrationState(m_currentSession);
			}
		}

#ifdef GAME_IS_CRYSIS2
		CRecordingSystem *pRecordingSystem = g_pGame->GetRecordingSystem();
		if (pRecordingSystem && pRecordingSystem->IsPlayingHighlightsReel())
		{
#ifdef USE_C2_FRONTEND
			CFlashFrontEnd *pFlashMenu = g_pGame->GetFlashMenu();
			if (pFlashMenu)
			{
				IFlashPlayer *pFlashPlayer = pFlashMenu->GetFlash();
				if (pFlashPlayer)
				{
					pFlashPlayer->Invoke(FRONTEND_SUBSCREEN_PATH_SET("UpdateHighlightSize"), NULL, 0);
				}
			}
#endif //#ifdef USE_C2_FRONTEND
		}
#endif

#if !defined(_RELEASE)
		if(gl_debug)
		{
			CryWatch("Session %d", m_currentSession);
			CryWatch("Server %d, Private %d, Online %d, Password %d", m_server, IsPrivateGame(), IsOnlineGame(), IsPasswordedGame());
			CryWatch("Hash %d", GameLobbyData::ConvertGameRulesToHash("HashTest"));
			if(!m_server)
			{
				CryWatch("JoinCommand %s", m_joinCommand);
			}

			if(m_userData[eLDI_Map].m_int32 != 0)
			{
				CryWatch("%s", GameLobbyData::GetMapFromHash(m_userData[eLDI_Map].m_int32));
			}

			if(m_userData[eLDI_Gamemode].m_int32 != 0)
			{
				CryWatch("%s", GameLobbyData::GetGameRulesFromHash(m_userData[eLDI_Gamemode].m_int32));
			}

			CryWatch("RequiredDLCs %d", m_userData[eLDI_RequiredDLCs].m_int32);
			CryWatch("Playlist %d", m_userData[eLDI_Playlist].m_int32);
			CryWatch("Variant %d", m_userData[eLDI_Variant].m_int32);
			CryWatch("Version %d", m_userData[eLDI_Version].m_int32);

			const int nameSize = m_nameList.Size();
			for(int i = 0; i < nameSize; i++)
			{
				CryWatch("\t%d - %s [sid:%llu uid:%u]", i + 1, m_nameList.m_sessionNames[i].m_name, m_nameList.m_sessionNames[i].m_conId.m_sid, m_nameList.m_sessionNames[i].m_conId.m_uid);
			}
		}

		if ((gl_debugLobbyRejoin == 1) || ((gl_debugLobbyRejoin == 2) && m_migrationStarted))
		{
			m_timeTillAutoLeaveLobby -= dt;
			if (m_timeTillAutoLeaveLobby <= 0.f)
			{
				LeaveSession(true);
			}
		}
#endif

		if (m_squadDirty && (m_nameList.Size() > 0))
		{
			bool bCanUpdateUserData = false;
			uint32 squadLeaderUID = 0;
			CryUserID pSquadLeaderId = g_pGame->GetSquadManager()->GetSquadLeader();
			SCryMatchMakingConnectionUID squadLeaderConID;
			if (pSquadLeaderId == CryUserInvalidID)
			{
				bCanUpdateUserData = true;
			}
			else
			{
				// Need to make sure we know who our squad leader is before updating the user data
				if (GetConnectionUIDFromUserID(pSquadLeaderId, squadLeaderConID))
				{
					bCanUpdateUserData = true;
				}
			}
			if (bCanUpdateUserData)
			{
				m_squadDirty = false;
				m_taskQueue.AddTask(CLobbyTaskQueue::eST_SetLocalUserData, true);
			}
		}
		const float prevStartTimer = m_startTimer;
		if (m_server)
		{
			bool  sendCountdownPacket = false;

			const int players = m_nameList.Size();
			const int playersNeeded = g_pGame->GetCVars()->g_minplayerlimit - players;
			bool bIsOnlineGame = IsOnlineGame();
			bool bIsPassworded = IsPasswordedGame();
			const bool countdownStarted = gEnv->IsDedicated() || g_pGameCVars->gl_skip || m_bSkipCountdown || (((m_privateGame==false || IsPasswordedGame()) && (IsOnlineGame() || g_pGameCVars->gl_enableOfflineCountdown)) && (playersNeeded <= 0));
			if(countdownStarted != m_startTimerCountdown)
			{
				m_startTimerCountdown = countdownStarted;
				if(countdownStarted)
				{
					m_startTimerLength = gl_initialTime;
					m_startTimer = m_startTimerLength;
					m_initialStartTimerCountdown = true;

					if (!CheckDLCRequirements())
					{
						return;
					}
				}

				sendCountdownPacket = true;

				if (m_timeTillCallToEnsureBestHost < gl_hostMigrationEnsureBestHostOnStartCountdownDelayTime)
				{
					m_timeTillCallToEnsureBestHost = gl_hostMigrationEnsureBestHostOnStartCountdownDelayTime;
				}
			}

			if (g_pGameCVars->gl_skip || m_bSkipCountdown)
			{
				m_startTimer = -1.f;
				m_bSkipCountdown = false;
			}

			if (m_votingEnabled)
			{
				if (countdownStarted && !m_votingClosed && ((m_startTimer - dt) <= g_pGameCVars->gl_votingCloseTimeBeforeStart))
				{
					SvCloseVotingAndDecideWinner();
					sendCountdownPacket = true;
				}
			}

			if (sendCountdownPacket)
			{
				SendPacket(eGUPD_LobbyStartCountdownTimer);
			}

			if(countdownStarted)
			{
				// Detect if the timer cvars have been changed
				const float timerLength = (m_initialStartTimerCountdown ? gl_initialTime : g_pGameCVars->gl_time);
				if (timerLength != m_startTimerLength)
				{
					const float diff = (timerLength - m_startTimerLength);
					m_startTimer += diff;
					m_startTimerLength = timerLength;
					SendPacket(eGUPD_UpdateCountdownTimer);
				}

				if (m_startTimer < 0.0f)
				{
					bool bCanStartSession = true;

					// Don't try to start the session mid-migration (delay call until after the migration has finished)
					if (hostMigrationState != eHMS_Idle)
					{
						bCanStartSession = false;
						CryLog("CGameLobby::Update() trying to start a game mid-migration, delaying (state=%u)", hostMigrationState);
					}

					if (bCanStartSession)
					{
						if (m_votingEnabled)
						{
							AdvanceLevelRotationAccordingToVotingResults();
						}

						SetState(eLS_JoinSession);
					}
				}
				else
				{
					m_startTimer -= dt;
				}
			}

			if (m_needsTeamBalancing && (g_pGameCVars->g_autoAssignTeams != 0))
			{
				// Don't balance teams mid migration
				if (hostMigrationState == eHMS_Idle)
				{
					BalanceTeams();
					m_needsTeamBalancing = false;
				}
			}
		}
		else
		{
			if(m_startTimerCountdown)
			{
				m_startTimer -= dt;
			}
		}

		if(m_startTimerCountdown)
		{
			if (m_votingEnabled)
			{
				CRY_ASSERT(g_pGameCVars->gl_checkDLCBeforeStartTime < g_pGameCVars->gl_time);
				CRY_ASSERT(g_pGameCVars->gl_checkDLCBeforeStartTime > gl_initialTime);
				const float  checkDLCReqsTime = MAX(MIN(g_pGameCVars->gl_checkDLCBeforeStartTime, g_pGameCVars->gl_time), gl_initialTime);

				if ((m_startTimer < checkDLCReqsTime) && ((m_startTimer + dt) >= checkDLCReqsTime))
				{
					CryLog("CGameLobby::Update() calling CheckDLCRequirements() [2]");
					// 1. when host is between rounds and just waits on the scoreboard when the next rotation has unloaded DLC in the vote
					// 2. when client is between rounds and just waits on the scoreboard when the next rotation has unloaded DLC in the vote
					const bool  leftSessionDueToDLCRequirementsFail = !CheckDLCRequirements();
					if (leftSessionDueToDLCRequirementsFail)
					{
						return;
					}
				}
			}
		}

#ifndef _RELEASE
		if (m_votingEnabled && gl_voteDebug)
		{
			if (m_votingCandidatesFlashInfo.tmpWatchInfoIsSet)
			{
				CryWatch("left candidate: %s (%s), num votes = %d", m_leftVoteChoice.m_levelName.c_str(), m_leftVoteChoice.m_gameRules.c_str(), m_leftVoteChoice.m_numVotes);
				CryWatch("right candidate: %s (%s), num votes = %d", m_rightVoteChoice.m_levelName.c_str(), m_rightVoteChoice.m_gameRules.c_str(), m_rightVoteChoice.m_numVotes);
				CryWatch("most votes so far: %d (%s)", m_highestLeadingVotesSoFar, (m_leftHadHighestLeadingVotes ? "left" : "right"));
			}
			if (m_votingFlashInfo.tmpWatchInfoIsSet)
			{
				if (!m_votingFlashInfo.localHasVoted)
				{
					if (!m_votingFlashInfo.votingClosed)
					{
						if (m_votingFlashInfo.localHasCandidates)
						{
							CryWatch("Cast your vote NOW!");
						}
						else
						{
							CryWatch("Awaiting candidate info...");
						}
					}
				}
				else
				{
					CryWatch("You voted %s", (m_votingFlashInfo.localVotedLeft ? "LEFT" : "RIGHT"));
				}
				if (m_votingFlashInfo.votingClosed)
				{
					if (m_votingFlashInfo.votingDrawn)
					{
						CryWatch("Voting was DRAWN, picking winner at RANDOM...");
					}
					CryWatch("Next map selected... Next: %s", (m_votingFlashInfo.leftWins ? "LEFT" : "RIGHT"));
				}
			}
			VOTING_DBG_WATCH("LOCAL vote status: %d", m_localVoteStatus);
		}
#endif

		const static float k_soundOffset = 0.5f;
		if(floorf(prevStartTimer + k_soundOffset) != floorf(m_startTimer + k_soundOffset))
		{
			const float time = m_initialStartTimerCountdown ? gl_initialTime : g_pGameCVars->gl_time;

#ifdef GAME_IS_CRYSIS2
			if(time > 0.0f)
			{
				m_lobbyCountdown.Play(0, "timeLeft", clamp(m_startTimer/time, 0.0f, 1.0f));
			}
#endif
		}

#if !defined(_RELEASE)
		if(gl_debug)
		{
			if (m_startTimerCountdown)
			{
				CryWatch("Auto start %.0f", m_startTimer);
			}
			else
			{
				CryWatch("Waiting for players");
			}
		}
#endif

		if (!m_connectedToDedicatedServer)
		{
			if (m_bHasUserList && (m_nameList.Size() == 1) && (m_server == false) && (gEnv->pNetwork->GetLobby()->GetMatchMaking()->GetSessionHostMigrationState(m_currentSession) == eHMS_Idle))
			{
				CryLog("GAME LOBBY HAS DETECTED A BROKEN STATE, BAILING (we're the only one in the lobby and we're not the host)");
#if !defined(_RELEASE)
				++gl_debugLobbyBreaksGeneral;
#endif

				LeaveSession(true);
				if (m_bMatchmakingSession && m_gameLobbyMgr->IsPrimarySession(this))
				{
					m_taskQueue.AddTask(CLobbyTaskQueue::eST_FindGame, false);
				}
			}
		}

		if(m_gameLobbyMgr->IsPrimarySession(this))
		{
#ifdef USE_C2_FRONTEND
			CFlashFrontEnd *pFlashMenu = g_pGame->GetFlashMenu();
			if (pFlashMenu && pFlashMenu->IsMenuActive(CFlashFrontEnd::eFlM_Menu))
			{
				IFlashPlayer *pFlashPlayer = pFlashMenu->GetFlash();
				EFlashFrontEndScreens screen = pFlashMenu->GetCurrentMenuScreen();
				if (IsGameLobbyScreen(screen))
				{
					if (m_sessionUserDataDirty)
					{
						m_sessionUserDataDirty = false;

						if (!m_server)
						{
							CMPMenuHub*  pMPMenuHub = pFlashMenu->GetMPMenu();
							CUIScoreboard*  pScoreboard = (pMPMenuHub ? pMPMenuHub->GetScoreboardUI() : NULL);

							if (!pScoreboard || (pScoreboard->ShowGameLobbyEndGame() == false))
							{
								CryLog("CGameLobby::Update() calling CheckDLCRequirements() [5]");
								// 1. when between rounds, client hammers A to get to lobby whilst server is still on victory screen
								// 2. client joining a lobby with unloaded DLC in the vote
								// 3. client accepting invitation to a match using unloaded DLC (either from a host in a lobby or a host in a game)
								// 4. client being in a private lobby when the host changes map to a DLC map the client doesn't have loaded
								const bool  leftSessionDueToDLCRequirementsFail = !CheckDLCRequirements();
								if (leftSessionDueToDLCRequirementsFail)
								{
									return;
								}
							}
						}

						SendSessionDetailsToFlash(pFlashPlayer);
					}

					float currTime  = gEnv->pTimer->GetCurrTime();
					if (m_nameList.m_dirty || (m_lastUserListUpdateTime < currTime - 0.1f))
					{
						m_lastUserListUpdateTime = currTime;

						if (m_nameList.m_dirty)
							m_nameList.m_dirty = false;

						SendUserListToFlash(pFlashPlayer);
					}
				}

				UpdateStatusMessage(pFlashPlayer);
			}
#endif //#ifdef USE_C2_FRONTEND

#if GAME_LOBBY_DO_ENSURE_BEST_HOST
			if ((!gEnv->IsDedicated()) && m_server)
			{
				if (m_timeTillCallToEnsureBestHost > 0.f)
				{
					m_timeTillCallToEnsureBestHost -= dt;
					if (m_timeTillCallToEnsureBestHost <= 0.f)
					{
						m_taskQueue.AddTask(CLobbyTaskQueue::eST_EnsureBestHost, true);
					}
				}
			}
#endif
		}

		if (m_server && (hostMigrationState == eHMS_Idle))
		{
			if (m_timeTillUpdateSession > 0.f)
			{
				m_timeTillUpdateSession -= dt;
				if (m_timeTillUpdateSession <= 0.f)
				{
					CryLog("CGameLobby::Update() change in average skill ranking detected");
					m_taskQueue.AddTask(CLobbyTaskQueue::eST_Update, true);
				}
			}
		}
	}
	else if(m_state == eLS_FindGame)
	{
		if(m_gameLobbyMgr->IsPrimarySession(this))
		{
#ifdef USE_C2_FRONTEND
			CFlashFrontEnd *pFlashMenu = g_pGame->GetFlashMenu();
			if (pFlashMenu && pFlashMenu->IsMenuActive(CFlashFrontEnd::eFlM_Menu))
			{
				IFlashPlayer *pFlashPlayer = pFlashMenu->GetFlash();
				UpdateStatusMessage(pFlashPlayer);
			}
#endif //#ifdef USE_C2_FRONTEND
		}
	}
}

void CGameLobby::UpdateDebugString()
{
	if (g_pGame->GetIGameFramework()->StartedGameContext())
	{
#ifdef GAME_IS_CRYSIS2
		g_pGame->AppendCrashDebugMessage(" Game");
		if (m_server)
		{
			g_pGame->AppendCrashDebugMessage(" Server");
		}
		g_pGame->AppendCrashDebugMessage(" ");
		g_pGame->AppendCrashDebugMessage(GetCurrentLevelName());
		g_pGame->AppendCrashDebugMessage(" ");
		g_pGame->AppendCrashDebugMessage(GetCurrentGameModeName());
#endif
		return;
	}
	if(m_state == eLS_Lobby)
	{
#ifdef GAME_IS_CRYSIS2
		g_pGame->AppendCrashDebugMessage(" Lobby");
#endif
	}
}

//-------------------------------------------------------------------------
// if returns false then the user will have been returned to the main menu and the session will have been left
bool CGameLobby::CheckDLCRequirements()
{
	bool  checkOk = true;

#ifdef GAME_IS_CRYSIS2
	CDLCManager*  pDLCManager = g_pGame->GetDLCManager();
	uint32  dlcRequirements = 0;

	if (m_votingEnabled)
	{
		if (m_hasReceivedStartCountdownPacket || m_server)
		{
			if (!m_votingCandidatesFlashInfo.leftLevelMapPath.empty() && !m_votingCandidatesFlashInfo.rightLevelMapPath.empty())
			{
				dlcRequirements |= pDLCManager->GetRequiredDLCsForLevel(m_votingCandidatesFlashInfo.leftLevelMapPath);
				dlcRequirements |= pDLCManager->GetRequiredDLCsForLevel(m_votingCandidatesFlashInfo.rightLevelMapPath);
			}
		}
	}
	else
	{
		if (!m_server && m_hasReceivedSessionQueryCallback)
		{
			dlcRequirements |= m_userData[eLDI_RequiredDLCs].m_int32;
		}
	}

	if ((dlcRequirements > 0) && !CDLCManager::MeetsDLCRequirements(dlcRequirements, pDLCManager->GetLoadedDLCs()))
	{
#ifdef USE_C2_FRONTEND
		if (CMPMenuHub* pMPMenu=CMPMenuHub::GetMPMenuHub())
		{
			pMPMenu->GoToCurrentLobbyServiceScreen();

			if (m_state != eLS_Leaving)
			{
				if (CGameLobbyManager* pGameLobbyManager=g_pGame->GetGameLobbyManager())
				{
					pGameLobbyManager->LeaveGameSession(CGameLobbyManager::eLSR_Menu);
				}
			}
		}
#endif //#ifdef USE_C2_FRONTEND

		g_pGame->GetWarnings()->AddWarning("DLCUnavailable", NULL);

		checkOk = false;
	}
#endif

	return checkOk;
}

//---------------------------------------
bool CGameLobby::ShouldCheckForBestHost()
{
	bool result = false;

	if ((!gEnv->IsDedicated()) && (IsMatchmakingGame() || g_pGameCVars->g_hostMigrationUseAutoLobbyMigrateInPrivateGames) && gl_allowEnsureBestHostCalls)
	{
		if ((!m_gameLobbyMgr->IsLobbyMerging()) && (!m_startTimerCountdown || (m_startTimer > gl_hostMigrationEnsureBestHostGameStartMinimumTime)))
		{
			ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
			if (pLobby)
			{
				ICryMatchMaking *pMatchmaking = pLobby->GetMatchMaking();
				if (pMatchmaking)
				{
					eHostMigrationState migrationState = pMatchmaking->GetSessionHostMigrationState(m_currentSession);

					if (migrationState == eHMS_Idle)
					{
						result = true;
					}
				}
			}
		}
	}

	return result;
}

//---------------------------------------
// [static]
void CGameLobby::MatchmakingEnsureBestHostCallback( CryLobbyTaskID taskID, ECryLobbyError error, void* arg )
{
	ENSURE_ON_MAIN_THREAD;

	CGameLobby *pLobby = static_cast<CGameLobby *>(arg);

	CryLog("CGameLobby::MatchmakingEnsureBestHostCallback error %d Lobby: %p", (int)error, pLobby);
	// Should call this but we're not currently waiting for the callback
	pLobby->NetworkCallbackReceived(taskID, error);

#ifndef _RELEASE
	if (gl_debugForceLobbyMigrations)
	{
		ICryMatchMaking *pMatchMaking = gEnv->pNetwork->GetLobby()->GetMatchMaking();
		eHostMigrationState migrationState = pMatchMaking->GetSessionHostMigrationState(pLobby->m_currentSession);
		if ((migrationState == eHMS_Idle) && (pLobby->m_server))
		{
			pLobby->m_timeTillCallToEnsureBestHost = gl_debugForceLobbyMigrationsTimer;
		}
	}
#endif
}

//-------------------------------------------------------------------------
#ifdef USE_C2_FRONTEND
bool CGameLobby::IsGameLobbyScreen(EFlashFrontEndScreens screen)
{
	if ( (screen == eFFES_game_lobby)
				|| (screen == eFFES_matchsettings)
				|| (screen == eFFES_matchsettings_change_map)
				|| (screen == eFFES_matchsettings_change_gametype) )
	{
		return true;
	}

	return false;
}

//-------------------------------------------------------------------------
void CGameLobby::UpdateStatusMessage(IFlashPlayer *pFlashPlayer)
{
	CRY_ASSERT(m_gameLobbyMgr->IsPrimarySession(this));

	if (!pFlashPlayer)
		return;

	CryFixedStringT<64> statusString;

	bool updateMatchmaking = false;
	bool bShowOnPlayOnlineScreen = false;

	const ELobbyState lobbyState = m_state;

	const int players = m_nameList.Size();
	const int playersNeeded = g_pGame->GetCVars()->g_minplayerlimit - players;

	if (m_bMatchmakingSession)
	{
		switch (lobbyState)
		{
			case eLS_FindGame:
			{
				statusString = "@ui_menu_gamelobby_searching";
				updateMatchmaking = true;
				break;
			}
			case eLS_Initializing:
			{
				statusString = "@ui_menu_gamelobby_joining";
				updateMatchmaking = true;
				bShowOnPlayOnlineScreen = true;
				break;
			}
			case eLS_Leaving:
			{
				statusString = "@ui_menu_gamelobby_joining";
				updateMatchmaking = true;
				break;
			}
			case eLS_Lobby:						// Deliberate fall-through
			case eLS_EndSession:
			{
				if (m_startTimerCountdown)
				{
					float timeToVote = m_startTimer - g_pGameCVars->gl_votingCloseTimeBeforeStart;
					if (m_votingEnabled && m_votingClosed==false && timeToVote>=0.f)
					{
						float timeLeft = MAX(0.f, timeToVote+1.f);	// Round up, don't show 0 sec
						statusString.Format("%0.f", timeLeft);
						if (timeLeft < 2.f)
						{
							statusString = CHUDUtils::LocalizeString("@ui_menu_gamelobby_votecountdown_single", statusString.c_str());
						}
						else
						{
							statusString = CHUDUtils::LocalizeString("@ui_menu_gamelobby_votecountdown", statusString.c_str());
						}
					}
					else
					{
						float timeLeft = MAX(0.f, m_startTimer+1.f);	// Round up, don't show 0 sec
						statusString.Format("%0.f", timeLeft);
						if (timeLeft < 2.f)
						{
							statusString = CHUDUtils::LocalizeString("@ui_menu_gamelobby_autostart_single", statusString.c_str());
						}
						else
						{
							statusString = CHUDUtils::LocalizeString("@ui_menu_gamelobby_autostart", statusString.c_str());
						}
					}
				}
				else if (playersNeeded > 0)
				{
					if (playersNeeded > 1)
					{
						statusString.Format("%d", playersNeeded);
						statusString = CHUDUtils::LocalizeString("@ui_menu_gamelobby_waiting_for_x_players", statusString.c_str());
					}
					else
					{
						statusString = "@ui_menu_gamelobby_waiting_for_player";
					}
				}
				break;
			}
			case eLS_JoinSession:			// Deliberate fall-through
			case eLS_Game:						// Deliberate fall-through
			case eLS_PreGame:
			{
				statusString = "@ui_menu_gamelobby_starting_game";
				break;
			}
		}
	}
	else
	{
		if (lobbyState == eLS_Lobby)
		{
			if(m_server)
			{
				const bool countdownStarted = ((m_privateGame==false || IsPasswordedGame()) && (IsOnlineGame() || g_pGameCVars->gl_enableOfflineCountdown)) && (playersNeeded <= 0);

				if ((m_privateGame==true && !IsPasswordedGame()) || (!IsOnlineGame() && !g_pGameCVars->gl_enableOfflineCountdown))
				{
					statusString = "@ui_menu_gamelobby_select_start_match";
				}
				else if (countdownStarted)
				{
					float timeLeft = MAX(0.f, m_startTimer+1.f);	// Round up, don't show 0 sec
					statusString.Format("%0.f", timeLeft);
					if (timeLeft < 2.f)
					{
						statusString = CHUDUtils::LocalizeString("@ui_menu_gamelobby_autostart_single", statusString.c_str());
					}
					else
					{
						statusString = CHUDUtils::LocalizeString("@ui_menu_gamelobby_autostart", statusString.c_str());
					}
				}
				else if (playersNeeded > 0)
				{
					if (playersNeeded > 1)
					{
						statusString.Format("%d", playersNeeded);
						statusString = CHUDUtils::LocalizeString("@ui_menu_gamelobby_waiting_for_x_players", statusString.c_str());
					}
					else
					{
						statusString = "@ui_menu_gamelobby_waiting_for_player";
					}
				}
			}
			else
			{
				if ((m_privateGame==true && !IsPasswordedGame()) || (!IsOnlineGame() && !g_pGameCVars->gl_enableOfflineCountdown))
				{
					statusString = "@ui_menu_gamelobby_waiting_for_server";
				}
				else if (m_startTimerCountdown)
				{
					float timeLeft = MAX(0.f, m_startTimer+1.f);	// Round up, don't show 0 sec
					statusString.Format("%0.f", timeLeft);
					if (timeLeft < 2.f)
					{
						statusString = CHUDUtils::LocalizeString("@ui_menu_gamelobby_autostart_single", statusString.c_str());
					}
					else
					{
						statusString = CHUDUtils::LocalizeString("@ui_menu_gamelobby_autostart", statusString.c_str());
					}
				}
				else if (playersNeeded > 0)
				{
					if (playersNeeded > 1)
					{
						statusString.Format("%d", playersNeeded);
						statusString = CHUDUtils::LocalizeString("@ui_menu_gamelobby_waiting_for_x_players", statusString.c_str());
					}
					else
					{
						statusString = "@ui_menu_gamelobby_waiting_for_player";
					}
				}
			}
		}
		else if(lobbyState == eLS_Game)
		{
			statusString = "@ui_menu_gamelobby_starting_game";
		}
	}

	CFlashFrontEnd *pFlashMenu = g_pGame->GetFlashMenu();
	if (pFlashMenu != NULL && pFlashMenu->IsMenuActive(CFlashFrontEnd::eFlM_Menu))
	{
		if (IsGameLobbyScreen(pFlashMenu->GetCurrentMenuScreen()))
		{
			if (updateMatchmaking)
			{
				UpdateMatchmakingDetails(pFlashPlayer, statusString.c_str());
			}
			else
			{
				pFlashPlayer->Invoke1(FRONTEND_SUBSCREEN_PATH_SET("UpdateStatusMessage"), statusString.c_str());
			}
		}
		else
		{
			bool bShowMessage = bShowOnPlayOnlineScreen;
			if (!bShowMessage)
			{
				const EFlashFrontEndScreens currentScreen = pFlashMenu->GetCurrentMenuScreen();
				if ((currentScreen != eFFES_play_online) && (currentScreen != eFFES_play_lan))
				{
					bShowMessage = true;
				}
			}

			pFlashPlayer->Invoke1("_root.setTopRightCaption", bShowMessage ? statusString.c_str() : "");
		}
	}
}
#endif //#ifdef USE_C2_FRONTEND

//static------------------------------------------------------------------------
bool CGameLobby::SortPlayersByTeam(const SFlashLobbyPlayerInfo &elem1, const SFlashLobbyPlayerInfo &elem2)
{
	if (elem1.m_teamId!=0 && elem2.m_teamId==0)
	{
		return true;
	}
	else if (elem2.m_teamId!=0 && elem1.m_teamId==0)
	{
		return false;
	}
	else if (elem1.m_onLocalTeam==true && elem2.m_onLocalTeam==false)
	{
		return true;
	}
	else if (elem1.m_onLocalTeam==false && elem2.m_onLocalTeam==true)
	{
		return false;
	}
	else
	{
		if (elem1.m_teamId < elem2.m_teamId)
			return true;
		else
			return false;
	}
}

//-------------------------------------------------------------------------
void CGameLobby::ClearChatMessages()
{
#if ENABLE_CHAT_MESSAGES
	m_chatMessagesIndex = 0;

	for (int i=0; i<NUM_CHATMESSAGES_STORED; ++i)
	{
		m_chatMessagesArray[i].Clear();
	}
#endif
}

//-------------------------------------------------------------------------
#ifdef USE_C2_FRONTEND
void CGameLobby::SendChatMessagesToFlash(IFlashPlayer *pFlashPlayer)
{
#if ENABLE_CHAT_MESSAGES

	IFlashVariableObject *textInputObj = NULL;
	FE_FLASHVAROBJ_REG(pFlashPlayer, FRONTEND_SUBSCREEN_PATH_SET("RightPane.TextChat.TextChatEntry"), textInputObj);

	SFlashVarValue maxChars(MAX_CHATMESSAGE_LENGTH-1);
	textInputObj->SetMember("maxChars", maxChars);

	FE_FLASHOBJ_SAFERELEASE(textInputObj);

	int index = m_chatMessagesIndex+1;
	if (index >= NUM_CHATMESSAGES_STORED)
		index = 0;

	int startIndex = index;

	do
	{
		m_chatMessagesArray[index].SendChatMessageToFlash(pFlashPlayer, true);

		++index;

		if (index >= NUM_CHATMESSAGES_STORED)
			index = 0;
	} while(index != startIndex);
#endif
}

//-------------------------------------------------------------------------
void CGameLobby::SendUserListToFlash(IFlashPlayer *pFlashPlayer)
{
	CRY_ASSERT(m_gameLobbyMgr->IsPrimarySession(this));

	if (!pFlashPlayer)
		return;

	CSquadManager* pSquadMgr = g_pGame->GetSquadManager();

	const SSessionNames *sessionNames = &m_nameList;
	int size = sessionNames->Size();

	bool useSquadUserlist = false;
	const ELobbyState lobbyState = m_state;

	if (m_bMatchmakingSession)
	{
		switch (lobbyState)
		{
			case eLS_FindGame:		// Deliberate fall-through
			case eLS_Initializing:
			case eLS_Leaving:
				{
					if (size == 0)
					{
						useSquadUserlist = true;
						sessionNames = pSquadMgr->GetSessionNames();
						size = sessionNames->Size();
					}
				}
				break;
		}
	}

	CryUserID pSquadLeaderId = pSquadMgr->GetSquadLeader();

	int localTeam = 0;

	bool useTeams = (g_pGameCVars->g_autoAssignTeams != 0) && (m_isTeamGame == true);
	if (m_privateGame == false || IsPasswordedGame())
	{
		if (m_startTimerCountdown==false || (m_votingEnabled && m_votingClosed==false))
		{
			useTeams = false;
		}
	}

#if !defined(_RELEASE)
	useTeams |= (gl_lobbyForceShowTeams != 0);
#endif

	const int MAX_PLAYERS = MAX_PLAYER_LIMIT;
	CryFixedArray<SFlashLobbyPlayerInfo, MAX_PLAYERS> playerInfos;
	int numPushedPlayers = 0;

	for (int nameIdx(0); (nameIdx < size) && (numPushedPlayers < MAX_PLAYERS); ++nameIdx)
	{
		const SSessionNames::SSessionName *pPlayer = &sessionNames->m_sessionNames[nameIdx];

		if (pPlayer->m_isDedicated)
		{
			continue;
		}

		if (pPlayer->m_name[0] == 0)
		{
			continue;
		}

		SFlashLobbyPlayerInfo playerInfo;

		if (useSquadUserlist)	// If using the squad list, 
		{
			uint32 channelId = pPlayer->m_conId.m_uid;
			bool isLocal = (nameIdx==0);

			CryUserID userId = pSquadMgr->GetUserIDFromChannelID(channelId);
			bool isSquadLeader = (pSquadLeaderId.IsValid() && (pSquadLeaderId == userId));

			playerInfo.m_rank = (uint8)pPlayer->m_userData[eLUD_Rank];
			playerInfo.m_reincarnations = pPlayer->GetReincarnations();
			pPlayer->GetDisplayName(playerInfo.m_nameString);

			playerInfo.m_teamId = 0;
			playerInfo.m_onLocalTeam = isLocal;
			playerInfo.m_conId = channelId; // Squad channel id, NOT game channel id
			playerInfo.m_isLocal = isLocal;	// Currently local player is always index 0
			playerInfo.m_isSquadMember = pSquadMgr->IsSquadMateByUserId(userId);
			playerInfo.m_isSquadLeader = isSquadLeader;
			playerInfo.m_voiceState = (uint8)eLVS_off;
			playerInfo.m_entryType = eLET_Squad;
		}
		else
		{
			uint32 channelId = pPlayer->m_conId.m_uid;
			bool isLocal = (nameIdx==0);
			if (isLocal)
			{
				localTeam = pPlayer->m_teamId;
			}

			CryUserID userId = GetUserIDFromChannelID(channelId);
			uint8 voiceState = (uint8)GetVoiceState(channelId);
			bool isSquadLeader = (pSquadLeaderId.IsValid() && (pSquadLeaderId == userId));

			playerInfo.m_rank = (uint8)pPlayer->m_userData[eLUD_Rank];
			playerInfo.m_reincarnations = pPlayer->GetReincarnations();
			pPlayer->GetDisplayName(playerInfo.m_nameString);

			if (useTeams)
			{
				playerInfo.m_teamId = pPlayer->m_teamId;
				playerInfo.m_onLocalTeam = (playerInfo.m_teamId == localTeam);
			}
			else
			{
				playerInfo.m_teamId = 0;
				playerInfo.m_onLocalTeam = isLocal;
			}

			playerInfo.m_conId = channelId;
			playerInfo.m_isLocal = isLocal;	// Currently local player is always index 0
			playerInfo.m_isSquadMember = pSquadMgr->IsSquadMateByUserId(userId);
			playerInfo.m_isSquadLeader = isSquadLeader;
			playerInfo.m_voiceState = voiceState;
			playerInfo.m_entryType = eLET_Lobby;
		}

		playerInfos.push_back(playerInfo);
		++ numPushedPlayers;
	}

#ifndef _RELEASE
	if (gl_dummyUserlist)
	{
		const int useSize = MIN(size+gl_dummyUserlist, MAX_PLAYERS);

		for (int nameIdx(size); (nameIdx<useSize); nameIdx++)
		{
			SFlashLobbyPlayerInfo playerInfo;

			playerInfo.m_rank = nameIdx;
			playerInfo.m_reincarnations = (nameIdx%5) ? 0 : nameIdx;
			playerInfo.m_nameString.Format("WWWWWWWWWWWWWW%.2d WWWW ", nameIdx);


			if (useTeams)
			{
				if (gl_dummyUserlistNumTeams==1)
					playerInfo.m_teamId = 1;
				else if (gl_dummyUserlistNumTeams==2)
					playerInfo.m_teamId = 1 + (nameIdx % 2);
				else if (gl_dummyUserlistNumTeams==3)
					playerInfo.m_teamId = (nameIdx % 3);
				else
					playerInfo.m_teamId = 0;
			}
			else
			{
				playerInfo.m_teamId = 0;
			}
			playerInfo.m_conId = 100+nameIdx;
			playerInfo.m_isLocal = false;
			playerInfo.m_isSquadMember = (nameIdx%4) ? false : true;
			playerInfo.m_isSquadLeader = ((nameIdx%4) || (nameIdx!=4)) ? false : true;
			playerInfo.m_onLocalTeam = (playerInfo.m_teamId == localTeam);
			playerInfo.m_voiceState = nameIdx%6;
			playerInfo.m_entryType = eLET_Lobby;

			playerInfos.push_back(playerInfo);
		}
	}
#endif

	std::sort(playerInfos.begin(), playerInfos.end(), SortPlayersByTeam); // Sort players by team

	IFlashVariableObject *pPushArray = NULL, *pMaxTeamPlayers = NULL;
	FE_FLASHVAROBJ_REG(pFlashPlayer, FRONTEND_SUBSCREEN_PATH_SET("m_namesList"), pPushArray);
	FE_FLASHVAROBJ_REG(pFlashPlayer, FRONTEND_SUBSCREEN_PATH_SET("m_maxTeamPlayers"), pMaxTeamPlayers);

	pPushArray->ClearElements();

	int currentTeamCount = 0;
	int currentTeam = -1;

	int maxUIPlayersPerTeam = 6;

	SFlashVarValue maxTeamPlayersValue = pMaxTeamPlayers->ToVarValue();
	if (maxTeamPlayersValue.IsDouble())	// Flash var is a double
	{
		maxUIPlayersPerTeam = (int)maxTeamPlayersValue.GetDouble();	// PC UI might be able to show more..
	}

	size = playerInfos.size();
	for (int playerIdx(0); (playerIdx<size); ++playerIdx)
	{
		if (playerInfos[playerIdx].m_teamId != currentTeam)
		{
			currentTeam = playerInfos[playerIdx].m_teamId;
			currentTeamCount = 0;
		}

		++currentTeamCount;
		if ((currentTeam != 0) && (currentTeamCount > maxUIPlayersPerTeam))
		{
			continue;
		}

		IFlashVariableObject *pNewObject = NULL;
		if (pFlashPlayer->CreateObject("Object", NULL, 0, pNewObject))
		{
			const bool bRankValid = (playerInfos[playerIdx].m_rank > 0);
			SFlashVarValue rankInt(playerInfos[playerIdx].m_rank);
			SFlashVarValue rankNull("");

			pNewObject->SetMember("Name", playerInfos[playerIdx].m_nameString.c_str());
			pNewObject->SetMember("Rank", bRankValid ? rankInt : rankNull);
			pNewObject->SetMember("Reincarnations", playerInfos[playerIdx].m_reincarnations);
			pNewObject->SetMember("TeamId", playerInfos[playerIdx].m_teamId);
			pNewObject->SetMember("ConId", playerInfos[playerIdx].m_conId);
			pNewObject->SetMember("Local", playerInfos[playerIdx].m_isLocal);
			pNewObject->SetMember("SquadMember", playerInfos[playerIdx].m_isSquadMember);
			pNewObject->SetMember("SquadLeader", playerInfos[playerIdx].m_isSquadLeader);
			pNewObject->SetMember("VoiceState", playerInfos[playerIdx].m_voiceState);
			pNewObject->SetMember("EntryType", (int)playerInfos[playerIdx].m_entryType);
			pPushArray->PushBack(pNewObject);

			FE_FLASHOBJ_SAFERELEASE(pNewObject);
		}
	}
	FE_FLASHOBJ_SAFERELEASE(pPushArray);
	FE_FLASHOBJ_SAFERELEASE(pMaxTeamPlayers);

	pFlashPlayer->Invoke1(FRONTEND_SUBSCREEN_PATH_SET("UpdatePlayerListNames"), (useTeams ? localTeam : 0));
}
#endif //#ifdef USE_C2_FRONTEND

const char* CGameLobby::GetMapDescription(const char* levelFileName, CryFixedStringT<64>* pOutLevelDescription)
{
	CRY_ASSERT(pOutLevelDescription);

	ILevelInfo* pLevelInfo = gEnv->pGame->GetIGameFramework()->GetILevelSystem()->GetLevelInfo(levelFileName);
	if(pLevelInfo)
	{
		CryFixedStringT<32> levelPath;
		string strLevelName(levelFileName);
		levelPath.Format("%s/%s.xml", pLevelInfo->GetPath(), PathUtil::GetFileName(strLevelName).c_str());

		XmlNodeRef mapInfo = GetISystem()->LoadXmlFromFile(levelPath.c_str());
		if(mapInfo)
		{
			XmlNodeRef imageNode = mapInfo->findChild("DescriptionText");
			if(imageNode)
			{
				if (const char* text = imageNode->getAttr("text"))
				{
					*pOutLevelDescription = text;
				}
			}
		}
	}

	return pOutLevelDescription->c_str();
}

#ifdef USE_C2_FRONTEND
const char* CGameLobby::GetMapImageName(const char* levelFileName, CryFixedStringT<128>* pOutLevelImageName)
{
	CRY_ASSERT(pOutLevelImageName);

	return pOutLevelImageName->c_str();
}

//-------------------------------------------------------------------------
void CGameLobby::UpdateMatchmakingDetails(IFlashPlayer *pFlashPlayer, const char* statusMessage)
{
	// Show searching panel.
	pFlashPlayer->Invoke1(FRONTEND_SUBSCREEN_PATH_SET("UpdateMatchmakingDetails"), statusMessage);
}

//-------------------------------------------------------------------------
void CGameLobby::SendSessionDetailsToFlash(IFlashPlayer *pFlashPlayer, const char* levelOverride/*=0*/)
{
	CRY_ASSERT(m_gameLobbyMgr->IsPrimarySession(this));

	if (!pFlashPlayer)
		return;

	CRecordingSystem *pRecordingSystem = g_pGame->GetRecordingSystem();
	bool bShowingHighlights = pRecordingSystem ? pRecordingSystem->IsPlayingHighlightsReel() : false;

	const ELobbyState lobbyState = m_state;

	if (m_bMatchmakingSession)
	{
		switch (lobbyState)
		{
			case eLS_FindGame:		// Deliberate fall-through
			case eLS_Leaving:
				{
					UpdateMatchmakingDetails(pFlashPlayer, "@ui_menu_gamelobby_searching");
					return;
				}
				break;
			case eLS_Initializing:
				{
					UpdateMatchmakingDetails(pFlashPlayer, "@ui_menu_gamelobby_joining");
					return;
				}
				break;
		};
	}

	bool votingEnabled = (m_votingEnabled && m_votingFlashInfo.localHasCandidates);
	if (votingEnabled==false)
	{
		// If we don't have any session details, or are matchmaking and don't have playlist details - show fetching info status instead of level details.
		if (!m_server && (!m_hasReceivedSessionQueryCallback || (m_bMatchmakingSession && (!m_hasReceivedPlaylistSync || !m_hasReceivedStartCountdownPacket))))
		{
			UpdateMatchmakingDetails(pFlashPlayer, "@ui_menu_gamelobby_fetching");
		}
		else
		{
			// No voting - display single map image with gamemode and variant.
			CryFixedWStringT<32> variantName;
			CryFixedWStringT<32> gameModeName;

			CryFixedStringT<32> tempString;

#ifdef GAME_IS_CRYSIS2
			CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
			if (pPlaylistManager)
			{
				const SGameVariant* pVariant = pPlaylistManager->GetVariant(pPlaylistManager->GetActiveVariantIndex());
				if (pVariant)
				{
					tempString = pVariant->m_localName.c_str();
				}
			}
			variantName = CHUDUtils::LocalizeStringW(tempString.c_str());

			if (m_currentGameRules.empty())
			{
				gameModeName.clear();
				CryLog("CGameLobby::SendSessionDetailsToFlash() We're in trouble, our currentGameRules is empty. Hide an invalid string ID, but nothing good is going to be onscreen");
			}
			else
			{
				tempString.Format("@ui_rules_%s", m_currentGameRules.c_str());
				gameModeName = CHUDUtils::LocalizeStringW(tempString.c_str());
			}
#endif

			if (levelOverride)
			{
				m_uiOverrideLevel = levelOverride;
			}

			CryFixedWStringT<64> fullModeName;
			CryFixedStringT<128> flashLevelImageName;
			CryFixedStringT<64> flashLevelDescription;
			CryFixedStringT<32> levelFileName = m_uiOverrideLevel.empty() ? m_currentLevelName.c_str() : m_uiOverrideLevel.c_str();
			GetMapImageName(levelFileName.c_str(), &flashLevelImageName);

			bool bUsingLevelOverride = m_uiOverrideLevel.empty()==false;
			if (bUsingLevelOverride)
			{
				GetMapDescription(levelFileName.c_str(), &flashLevelDescription);
				fullModeName.Format(L"%ls", gameModeName.c_str());
			}
			else
			{
				fullModeName.Format(L"%ls - %ls", gameModeName.c_str(), variantName.c_str());
			}

			levelFileName = PathUtil::GetFileName(levelFileName.c_str()).c_str();
			levelFileName = g_pGame->GetMappedLevelName(levelFileName.c_str());

			bool allowMapCycling = m_server && m_bMatchmakingSession==false && m_bChoosingGamemode==false && bUsingLevelOverride==false;

			SFlashVarValue pArgs[] = { fullModeName.c_str()
				, levelFileName.c_str()
					,	bShowingHighlights ? "$BackBuffer" : flashLevelImageName.c_str()
					, allowMapCycling
					, bUsingLevelOverride
					, flashLevelDescription.c_str() };

			pFlashPlayer->Invoke(FRONTEND_SUBSCREEN_PATH_SET("UpdateSessionDetails"), pArgs, ARRAY_COUNT(pArgs));
		}
	}
	else
	{
		// Voting enabled - either show voting options or if voting has finished, the winner.
		SFlashVarValue pArgs[] = { m_votingFlashInfo.votingClosed
			, m_votingFlashInfo.votingDrawn
				,	m_votingFlashInfo.leftWins
				, m_votingFlashInfo.localHasVoted
				, m_votingFlashInfo.localVotedLeft
				, m_votingFlashInfo.leftNumVotes
				, m_votingFlashInfo.rightNumVotes
				, bShowingHighlights ? "$BackBuffer" : m_votingCandidatesFlashInfo.leftLevelImage.c_str()
				, m_votingCandidatesFlashInfo.leftLevelName.c_str()
				, m_votingCandidatesFlashInfo.leftRulesName.c_str()
				, bShowingHighlights ? "$BackBuffer" : m_votingCandidatesFlashInfo.rightLevelImage.c_str()
				, m_votingCandidatesFlashInfo.rightLevelName.c_str()
				, m_votingCandidatesFlashInfo.rightRulesName.c_str()
				, m_votingFlashInfo.votingStatusMessage.c_str()
		};
		pFlashPlayer->Invoke(FRONTEND_SUBSCREEN_PATH_SET("UpdateSessionVotingDetails"), pArgs, ARRAY_COUNT(pArgs));
	}
}
#endif //#ifdef USE_C2_FRONTEND

//-------------------------------------------------------------------------
ECryLobbyError CGameLobby::DoQuerySession(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId)
{
	CryLog("CGameLobby::DoQuerySession() pLobby=%p", this);
	CRY_ASSERT(m_taskQueue.GetCurrentTask() == CLobbyTaskQueue::eST_Query);

#if !USE_CRYLOBBY_GAMESPY	// on GameSpy servers can call this too, to get thier serverId used for ServerImageCache
	CRY_ASSERT(!m_server);
#endif

	ECryLobbyError result = pMatchMaking->SessionQuery(m_currentSession, &taskId, CGameLobby::MatchmakingSessionQueryCallback, this);

	return result;
}

//-------------------------------------------------------------------------
void CGameLobby::UpdateState()
{
	//done in update to handle callbacks from network thread
	if(m_stateHasChanged)
	{
		if (m_state == eLS_PreGame)
		{
			SetState(eLS_Game);
		}

		m_stateHasChanged = false;
	}
}

//-------------------------------------------------------------------------
bool CGameLobby::CheckRankRestrictions()
{
	bool bAllowedToJoin = true;

#ifdef GAME_IS_CRYSIS2
	if (!gEnv->IsDedicated())
	{
		CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
		if (pPlaylistManager)
		{
			int activeVariant = pPlaylistManager->GetActiveVariantIndex();
			const SGameVariant *pGameVariant = (activeVariant >= 0) ? pPlaylistManager->GetVariant(activeVariant) : NULL;
			const int localRank = CPlayerProgression::GetInstance()->GetData(EPP_Rank);
			const int reincarnations = CPlayerProgression::GetInstance()->GetData(EPP_Reincarnate);

			const char*  pWarning = NULL;  // a NULL warning means everything's OK and we're allowed to join

			{
				int restrictRank = pGameVariant ? pGameVariant->m_restrictRank : 0;
				if (restrictRank)
				{
					if (localRank > restrictRank || reincarnations > 0)
					{
						pWarning = "RankTooHigh";
					}
				}
			}

			if (!pWarning)
			{
				int requireRank = pGameVariant ? pGameVariant->m_requireRank : 0;
				if (requireRank)
				{
					if (localRank < requireRank)
					{
						pWarning = "RankTooLow";
					}
				}
			}

			// alright, all checks are done... time to see if we're allowed

			bAllowedToJoin = (pWarning == NULL);
			if (!bAllowedToJoin)
			{
				LeaveSession(true);
				g_pGame->GetWarnings()->AddWarning(pWarning, NULL);
			}
		}
	}
#endif

	return bAllowedToJoin;
}

//-------------------------------------------------------------------------
void CGameLobby::EnterState(ELobbyState prevState, ELobbyState newState)
{
	bool isPrimarySession = m_gameLobbyMgr->IsPrimarySession(this);
	m_state = newState;

	CryLog("[CG] CGameLobby::EnterState() entering state %i from %i Lobby: %p", int(newState), int(prevState), this);
	INDENT_LOG_DURING_SCOPE();

	switch(newState)
	{
	case eLS_JoinSession:
		{
			bool bAllowedToJoin = CheckRankRestrictions();
			if (bAllowedToJoin)
			{
				m_taskQueue.AddTask(CLobbyTaskQueue::eST_SessionStart, false);
				SetLocalVoteStatus(eLVS_awaitingCandidates);		// Reset our vote ready for the next round

#if defined (TRACK_MATCHMAKING)
				CMatchmakingTelemetry* pMMTel = g_pGame->GetMatchMakingTelemetry();
				if( pMMTel != NULL && m_bMatchmakingSession && m_currentSessionId )
				{
					pMMTel->AddEvent( SMMLaunchGameEvent( m_currentSessionId ) );
				}
#endif
				if( CMatchMakingHandler* pMMhandler = m_gameLobbyMgr->GetMatchMakingHandler() )
				{
					pMMhandler->OnLeaveMatchMaking();
				}

				if (m_server)
				{
					SendPacket(eGUPD_UnloadPreviousLevel, CryMatchMakingInvalidConnectionUID);
				}
			}
			break;
		}
	case eLS_EndSession:
		{
			if (m_server)
			{
				SendPacket(eGUPD_LobbyEndGame);
			}
			m_taskQueue.AddTask(CLobbyTaskQueue::eST_SessionEnd, false);
			break;
		}
	case eLS_PreGame:
		{
#ifdef USE_C2_FRONTEND
			CFlashFrontEnd *pFFE = g_pGame->GetFlashMenu();
			if (pFFE)
			{
				pFFE->Execute(CFlashFrontEnd::eFECMD_load_with_movie_mp, m_currentLevelName.c_str()); 
			}
#endif //#ifdef USE_C2_FRONTEND

			OnGameStarting();

			break;
		}
	case eLS_Game:
		{
			// Force godmode to be off!
			ICVar *pCVar = gEnv->pConsole->GetCVar("g_godmode");
			if (pCVar)
			{
				if (pCVar->GetIVal() != 0)
				{
					CryLog("CGameLobby::EnterState(eLS_Game), g_godmode is set! resetting to 0");
					pCVar->Set(0);
				}
			}

			if (m_server)
			{
				m_loadingGameRules = m_currentGameRules;
				m_loadingLevelName = m_currentLevelName;
			}

			// Because we are using the lobby, we can set a hint which allows the loading system to queue the client map load a lot sooner
			ICryLobby* pLobby = gEnv->pNetwork->GetLobby();
			if (pLobby)
			{
				pLobby->SetCryEngineLoadHint(GetLoadingLevelName(), GetLoadingGameModeName());
			}

			if(m_server)
			{
				// does this need to check for uninitialised data if dedicated?
				m_taskQueue.AddTask(CLobbyTaskQueue::eST_Update, true);

#if INCLUDE_DEDICATED_LEADERBOARDS
				ICryStats *pStats = gEnv->pNetwork->GetLobby()->GetStats();
				if((!pStats) || (pStats->GetLeaderboardType() == eCLLT_P2P))
				{
					SendPacket(eGUPD_LobbyGameHasStarted);	//informs clients who are in the lobby straight away
				}
				else
				{
					SSessionNames *sessionNames = &m_nameList;
					uint32 count = sessionNames->Size();

					for(uint32 i = 0; i < count; ++i)
					{
						SSessionNames::SSessionName *pSessionName = &sessionNames->m_sessionNames[i];
						if(pSessionName->m_onlineStatus != eOAS_Uninitialised)
						{
							CryLog("[GameLobby] sending game has started to user %s", pSessionName->m_name);
							SendPacket(eGUPD_LobbyGameHasStarted, pSessionName->m_conId);
						}
						else
						{
							CryLog("[GameLobby] not sending game has started to user %s as we don't have their online stats yet", pSessionName->m_name);
						}
					}
					
					CryLog("[GameLobby] using dedicated leader boards, only send to users whose data we have already obtained");
				}
#else
				SendPacket(eGUPD_LobbyGameHasStarted);	//informs clients who are in the lobby straight away
#endif
				m_taskQueue.AddTask(CLobbyTaskQueue::eST_StartGameContext, true);
			}
			else
			{
				ICVar *pCVarMaxPlayers = gEnv->pConsole->GetCVar("sv_maxplayers");
				if (pCVarMaxPlayers)
				{
					uint32 maxPlayers = m_sessionData.m_numPublicSlots + m_sessionData.m_numPrivateSlots;
					pCVarMaxPlayers->Set((int) maxPlayers);
				}

#ifdef GAME_IS_CRYSIS2
				CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
				if (pPlaylistManager)
				{
					pPlaylistManager->SetModeOptions();
				}
#endif

				g_pGame->GetIGameFramework()->ExecuteCommandNextFrame(m_joinCommand);
			}

#ifdef USE_C2_FRONTEND
			CFlashFrontEnd *pFFE = g_pGame->GetFlashMenu();
			if (pFFE)
			{
				pFFE->Execute(CFlashFrontEnd::eFECMD_load_with_movie_mp, m_currentLevelName.c_str()); 
			}
#endif //#ifdef USE_C2_FRONTEND
		}
		break;

	case eLS_PostGame:
		{
			if( gEnv && gEnv->pCryPak )
			{
				gEnv->pCryPak->DisableRuntimeFileAccess(false);
			}
		}
		break;

	case eLS_Lobby:
		{
			CryLog(" entered lobby state (eLS_Lobby), m_server=%s Lobby: %p", m_server ? "true" : "false", this);


			ResetLevelOverride();

			if (m_bHasReceivedVoteOptions == false)
			{
				VOTING_DBG_LOG("[tlh] set m_localVoteStatus [2] to eLVS_awaitingCandidates");
				SetLocalVoteStatus(eLVS_awaitingCandidates);
				m_votingFlashInfo.Reset();
				m_votingCandidatesFlashInfo.Reset();
			}

#ifdef GAME_IS_CRYSIS2
			m_lobbyCountdown.SetSignal("LobbyCountdown");
#endif

			ClearChatMessages();

			if(prevState == eLS_PostGame)
			{
				CRY_ASSERT(m_server);

				if (m_votingEnabled)
				{
					SvResetVotingForNextElection();
				}

				if (m_timeTillCallToEnsureBestHost < gl_hostMigrationEnsureBestHostOnReturnToLobbyDelayTime)
				{
					m_timeTillCallToEnsureBestHost = gl_hostMigrationEnsureBestHostOnReturnToLobbyDelayTime;
				}
			}

			if(!m_server)
			{
				m_taskQueue.AddTask(CLobbyTaskQueue::eST_Query, true);
			}
			else
			{
				m_initialStartTimerCountdown = (prevState != eLS_PostGame);
				m_startTimerLength = m_initialStartTimerCountdown ? gl_initialTime : g_pGameCVars->gl_time;
				m_startTimer = (g_pGameCVars->gl_skip == 0) ? m_startTimerLength : 0.f;
				SendPacket(eGUPD_LobbyStartCountdownTimer);
			}

			m_sessionUserDataDirty = true;
			m_nameList.m_dirty = true;

			if (prevState == eLS_Initializing)
			{
				m_hasReceivedSessionQueryCallback = false;
				m_hasReceivedStartCountdownPacket = false;
				m_hasReceivedPlaylistSync = false;
				m_gameHadStartedWhenPlaylistRotationSynced = false;

				if(!m_server)
				{
					if(isPrimarySession)
					{
#ifdef USE_C2_FRONTEND
						CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
						if(menu)
						{
							if (menu->IsScreenInStack("game_lobby") == false)
							{
								menu->Execute(CFlashFrontEnd::eFECMD_goto, "game_lobby");
							}
						}
#endif //#ifdef USE_C2_FRONTEND
					}
				}
				else
				{
					m_votingEnabled = false;
					CRY_ASSERT_MESSAGE((!g_pGameCVars->gl_enablePlaylistVoting || g_pGameCVars->gl_experimentalPlaylistRotationAdvance), "The gl_enablePlaylistVoting cvar currently requires gl_experimentalPlaylistRotationAdvance to also be set");

					if (g_pGameCVars->gl_experimentalPlaylistRotationAdvance)
					{
						SvInitialiseRotationAdvanceAtFirstLevel();  // (will set data->Get()->m_votingEnabled as required)
					}

					if (m_votingEnabled)
					{
						SvResetVotingForNextElection();
					}

					CryLog("CGameLobby::EnterState() eLS_Lobby: calling CheckDLCRequirements() [4]");
					// 1. when host starts a new playlist which has unloaded DLC on the first vote
					const bool  leftSessionDueToDLCRequirementsFail = !CheckDLCRequirements();
					if (leftSessionDueToDLCRequirementsFail)
					{
						break;  // (early break from eLS_Lobby case)
					}
				}
			}

			if(prevState == eLS_PostGame)
			{
				CRY_ASSERT(m_server);

				if (!m_votingEnabled)
				{
					UpdateLevelRotation();
					m_taskQueue.AddTask(CLobbyTaskQueue::eST_Update, true);
				}
			}

			// Un-mute everyone
			MutePlayersOnTeam(0, false);
			MutePlayersOnTeam(1, false);
			MutePlayersOnTeam(2, false);

			// Refresh squad user data incase progression information has updated
			if (CSquadManager *pSquadManager = g_pGame->GetSquadManager())
			{
				pSquadManager->LocalUserDataUpdated();
			}

			m_taskQueue.AddTask(CLobbyTaskQueue::eST_SetLocalUserData, true);

#ifndef _RELEASE
			m_timeTillAutoLeaveLobby = gl_debugLobbyRejoinTimer;
			float randomElement = ((float) (g_pGame->GetRandomNumber() & 0xffff)) / ((float) 0xffff);
			m_timeTillAutoLeaveLobby += (gl_debugLobbyRejoinRandomTimer * randomElement);

			m_migrationStarted = false;
#endif

			// got here by a map command
			if (m_hasReceivedMapCommand)
			{
				CryLog("  entered lobby state - already have game start params, starting session");
				SetState(eLS_JoinSession);
			}
			else if (!m_pendingLevelName.empty())
			{
				CryLog("  entered lobby state - already have pending level name");
				m_currentLevelName = m_pendingLevelName.c_str();
				m_pendingLevelName.clear();

				ICVar *pCVar = gEnv->pConsole->GetCVar("sv_gamerules");
				if (pCVar)
				{
					pCVar->Set(m_currentGameRules.c_str());
				}

				CryFixedStringT<128> command;
				command.Format("map %s s nb", GetCurrentLevelName());
				gEnv->pConsole->ExecuteString(command.c_str(), false, true);
			}
		}
		break;

	case eLS_None:
		{
			if (isPrimarySession)
			{
				ILevelRotation *pLevelRotation=g_pGame->GetIGameFramework()->GetILevelSystem()->GetLevelRotation();
				if (pLevelRotation->GetLength() > 0)
				{
					pLevelRotation->Reset();
				}

#ifdef GAME_IS_CRYSIS2
				CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
				if (pPlaylistManager)
				{
					pPlaylistManager->ClearCurrentPlaylist();
				}
#endif
			}

			// Task queue should already be empty but it'll get updated once before we are actually deleted so make
			// sure nothing is going to start!
			m_taskQueue.Reset();

			ClearChatMessages();

			m_allowRemoveUsers = true;
			m_nameList.Clear();
			m_sessionFavouriteKeyId = INVALID_SESSION_FAVOURITE_ID;

			m_currentGameRules = "";
			m_currentLevelName = "";

			m_gameLobbyMgr->DeletedSession(this);
		}
		break;

	case eLS_FindGame:
		{
			CRY_ASSERT((prevState == eLS_None) || (prevState == eLS_Leaving) || (prevState == eLS_Initializing));
			CRY_ASSERT(m_currentSession == CrySessionInvalidHandle);

#if defined (TRACK_MATCHMAKING)
			if( CMatchmakingTelemetry* pMMTel = g_pGame->GetMatchMakingTelemetry() )
			{
				pMMTel->BeginMatchMakingTranscript( CMatchmakingTelemetry::eMMTelTranscript_QuickMatch );
			}
#endif
			FindGameEnter();
		}
		break;

	case eLS_GameEnded:
		{
			if (m_hasReceivedMapCommand)
			{
				// If we're ready to start the next game, don't wait to be told to continue
				SetState(eLS_Lobby);
			}
		}
		break;

	case eLS_Initializing:
		{
			ResetLocalVotingData();
		}
		break;

	case eLS_Leaving:
		{
			const char* pNextFrameCommand = g_pGame->GetIGameFramework()->GetNextFrameCommand();
			if(pNextFrameCommand)
			{
				if(!strcmp(pNextFrameCommand, m_joinCommand))
				{
					CryLog("[lobby] clear join command from CryAction");
					g_pGame->GetIGameFramework()->ClearNextFrameCommand();
				}
			}

			ResetLocalVotingData();
			CancelAllLobbyTasks();
			if (m_bSessionStarted)
			{
				// Need to end the session before deleting it
				m_taskQueue.AddTask(CLobbyTaskQueue::eST_SessionEnd, false);
			}
			m_taskQueue.AddTask(CLobbyTaskQueue::eST_Delete, false);
		}
		break;

	default:
		CRY_ASSERT_MESSAGE(false, "CGameLobby::EnterState Unknown state");
		break;
	}

	if (isPrimarySession)
	{
#ifdef USE_C2_FRONTEND
		CFlashFrontEnd *pFlashMenu = g_pGame->GetFlashMenu();
		if (pFlashMenu != NULL && pFlashMenu->IsMenuActive(CFlashFrontEnd::eFlM_Menu))
		{
			IFlashPlayer *pFlashPlayer = pFlashMenu->GetFlash();
			UpdateStatusMessage(pFlashPlayer);
		}
#endif //#ifdef USE_C2_FRONTEND
	}
}

//---------------------------------------
void CGameLobby::OnGameStarting()
{
	CryLog("[CG] CGameLobby::OnGameStarting() Lobby: %p", this);

	// Sort out voice muting
	CryUserID localUser = gEnv->pNetwork->GetLobby()->GetLobbyService()->GetUserID(GameLobby_GetCurrentUserIndex());
	SCryMatchMakingConnectionUID localUID;
	if (localUser.IsValid() && GetConnectionUIDFromUserID(localUser, localUID))
	{
		int userIndex = m_nameList.Find(localUID);
		if (userIndex != SSessionNames::k_unableToFind)
		{
			const uint8 teamId = m_nameList.m_sessionNames[userIndex].m_teamId;
			CryLog("    we're on team %i Lobby: %p", teamId, this);
			if (teamId)
			{
				const uint8 otherTeamId = (3 - teamId);
				if(m_isTeamGame)
				{
					CryLog("    muting players on teams 0 and %i Lobby: %p", otherTeamId, this);
					// Mute players not on a team or on the other team, un-mute those on our team
					MutePlayersOnTeam(0, true);
					MutePlayersOnTeam(teamId, false);
					MutePlayersOnTeam(otherTeamId, true);
				}
				else
				{
					CryLog("    ummuting all players Lobby: %p", this);
					MutePlayersOnTeam(0, false);
					MutePlayersOnTeam(1, false);
					MutePlayersOnTeam(2, false);
				}
			}
			else
			{
				CryLog("    we're not on a team Lobby: %p", this);
				// Not been given a team, either we're a mid-game joiner or this isn't a team game
				if ((!m_hasValidGameRules) || m_isTeamGame)
				{
					CryLog("        this is either a team game or unknown gamemode, muting all players, will unmute when teams have been assigned Lobby: %p", this);
					// Team game, mute all players and unmute our team once we know what that is!
					MutePlayersOnTeam(0, true);
					MutePlayersOnTeam(1, true);
					MutePlayersOnTeam(2, true);
				}
				else
				{
					CryLog("        this is a non-team game Lobby: %p", this);
				}
			}
		}
		else
		{
			CryLog("    not received the users list yet Lobby: %p", this);
		}
	}
}

//-------------------------------------------------------------------------
void CGameLobby::UpdateLevelRotation()
{
	CRY_ASSERT(m_server);
#ifdef GAME_IS_CRYSIS2
	ILevelRotation*  pLevelRotation = g_pGame->GetPlaylistManager()->GetLevelRotation();
	const int  len = pLevelRotation->GetLength();
	if (len > 0)
	{
		m_currentLevelName = pLevelRotation->GetNextLevel();

		// game modes can have aliases, so we get the correct name here
		IGameRulesSystem *pGameRulesSystem = g_pGame->GetIGameFramework()->GetIGameRulesSystem();
		const char *gameRulesName = pGameRulesSystem ? pGameRulesSystem->GetGameRulesName(pLevelRotation->GetNextGameRules()) : pLevelRotation->GetNextGameRules();

		if (stricmp(m_currentGameRules.c_str(), gameRulesName))
		{
			GameRulesChanged(gameRulesName);
		}
		const int numSettings = pLevelRotation->GetNextSettingsNum();
		for(int i = 0; i < numSettings; i++)
		{
			gEnv->pConsole->ExecuteString(pLevelRotation->GetNextSetting(i));
		}

		if (!pLevelRotation->Advance())
		{
			pLevelRotation->First();
		}
	}
#endif
}

//-------------------------------------------------------------------------
void CGameLobby::SvInitialiseRotationAdvanceAtFirstLevel()
{
	CryLog("CGameLobby::SvInitialiseRotationAdvanceAtFirstLevel()");

	CRY_ASSERT(m_server);

#ifdef GAME_IS_CRYSIS2
	if (CPlaylistManager* plMgr=g_pGame->GetPlaylistManager())
	{
		if (plMgr->HavePlaylistSet())
		{
			ILevelRotation*  pRot = plMgr->GetLevelRotation();
			CRY_ASSERT(pRot);

			m_votingEnabled = (g_pGameCVars->gl_enablePlaylistVoting && (IsPrivateGame()==false || IsPasswordedGame()) && (IsOnlineGame() || g_pGameCVars->gl_enableOfflinePlaylistVoting) && (pRot->GetLength() >= g_pGameCVars->gl_minPlaylistSizeToEnableVoting));

			{
				pRot->First();

				CryFixedStringT<128>  consoleCmd;

				ChangeGameRules(pRot->GetNextGameRules());
				ChangeMap(pRot->GetNextLevel());

				// this block is a slightly modified version of pRot->ChangeLevel() - so other "settings" from the rotation level info entry are set, and the "next" index is updated. will also set sv_gamerules, etc. but we don't use those anymore
				{
					const int  numSettings = pRot->GetNextSettingsNum();
					for (int i=0; i<numSettings; i++)
					{
						CRY_TODO(26,05,2010, "Need to think about the security of this. If it doesn't go through the general white-list filtering that system.cfg (etc.) settings go through, then it'll need its own white-list. Or it might be best to have its own anyway. (The PlaylistManager might be a good place for it.)");
						gEnv->pConsole->ExecuteString(pRot->GetNextSetting(i));
					}

					if (!m_votingEnabled)
					{
						if (!pRot->Advance())
						{
							pRot->First();
						}
					}
				}
			}
		}
	}
#endif
}

//-------------------------------------------------------------------------
void CGameLobby::SvResetVotingForNextElection()
{
	CryLog("CGameLobby::SvResetVotingForNextElection()");

	CRY_ASSERT(m_server);
	CRY_ASSERT(m_votingEnabled);

#ifdef GAME_IS_CRYSIS2
	CPlaylistManager*  plMgr = g_pGame->GetPlaylistManager();
	CRY_ASSERT(plMgr);
	CRY_ASSERT(plMgr->HavePlaylistSet());
	ILevelRotation*  pLevelRotation = plMgr->GetLevelRotation();
	CRY_ASSERT(pLevelRotation);
#endif

	m_votingClosed = false;
	m_leftVoteChoice.Reset();
	m_rightVoteChoice.Reset();
	m_highestLeadingVotesSoFar = 0;
	m_leftHadHighestLeadingVotes = false;

	SetLocalVoteStatus(eLVS_notVoted);
	VOTING_DBG_LOG("[tlh] set m_localVoteStatus [3] to eLVS_notVoted");

	UpdateVoteChoices();

#ifdef USE_C2_FRONTEND
	UpdateVotingCandidatesFlashInfo();
	UpdateVotingInfoFlashInfo();
#endif //#ifdef USE_C2_FRONTEND
}

//-------------------------------------------------------------------------
void CGameLobby::SvCloseVotingAndDecideWinner()
{
	CryLog("CGameLobby::SvCloseVotingAndDecideWinner()");

	CRY_ASSERT(m_server);
	CRY_ASSERT(m_votingEnabled);
	CRY_ASSERT(!m_votingClosed);

	m_votingClosed = true;

	if (m_leftVoteChoice.m_numVotes != m_rightVoteChoice.m_numVotes)
	{
		m_leftWinsVotes = (m_leftVoteChoice.m_numVotes > m_rightVoteChoice.m_numVotes);
	}
	else
	{
		CryLog(".. voting is drawn at %d a piece", m_leftVoteChoice.m_numVotes);

		if (m_highestLeadingVotesSoFar > 0)
		{
			CryLog(".... the [%s] choice was the first to get to %d, so setting that as the winner!", (m_leftHadHighestLeadingVotes ? "left" : "right"), m_highestLeadingVotesSoFar);
			m_leftWinsVotes = m_leftHadHighestLeadingVotes;
		}
		else
		{
			m_leftWinsVotes = (cry_frand() >= 0.5f);
			CryLog(".... neither choice was ever winning (there were probably no votes), so picking at random... chose [%s]", (m_leftWinsVotes ? "left" : "right"));
		}
	}

	UpdateRulesAndMapFromVoting();
}

//-------------------------------------------------------------------------
void CGameLobby::AdvanceLevelRotationAccordingToVotingResults()
{
#ifdef GAME_IS_CRYSIS2
	ILevelRotation*  pLevelRotation = g_pGame->GetPlaylistManager()->GetLevelRotation();
	CRY_ASSERT(pLevelRotation);

	if (m_server)
	{
		CRY_ASSERT(m_votingEnabled);
		// If the voting hasn't closed yet, close it now (possible if the start timer length was changed using the console)
		if (!m_votingClosed)
		{
			SvCloseVotingAndDecideWinner();
		}

		if (!m_leftWinsVotes)
		{
			// if right wins we need to advance the rotation BEFORE updating the rotation and session, so we skip over the losing (left) choice
			if (!pLevelRotation->Advance())
			{
				pLevelRotation->First();
			}
		}

		UpdateLevelRotation();

		if (m_leftWinsVotes)
		{
			// if left wins we need to advance the rotation AFTER updating the rotation, so we skip over the losing (right) choice next time
			if (!pLevelRotation->Advance())
			{
				pLevelRotation->First();
			}
		}

		m_taskQueue.AddTask(CLobbyTaskQueue::eST_Update, true);
	}
	else
	{
		// Advance the rotation twice for the client so it moves onto the next pair of levels ready for the next vote.
		// Note that the server does actually Advance twice too (above), but one of those times is buried inside its call to UpdateLevelRotation().

		if (!pLevelRotation->Advance())
		{
			pLevelRotation->First();
		}

		if (!pLevelRotation->Advance())
		{
			pLevelRotation->First();
		}
	}
#endif

}

//-------------------------------------------------------------------------
void CGameLobby::StartGame()
{
#ifdef GAME_IS_CRYSIS2
	CCCPOINT(GameLobby_StartGame);
#endif

	if (m_gameStartParams)
	{
		SGameContextParams gameContextParams;
		gameContextParams.levelName = m_gameStartParams->levelName.c_str();
		gameContextParams.gameRules = m_gameStartParams->gameModeName.c_str();
		gameContextParams.demoPlaybackFilename = m_gameStartParams->demoPlaybackFilename.c_str();
		gameContextParams.demoRecorderFilename = m_gameStartParams->demoRecorderFilename.c_str();

		// Give the gamerules a quick sanity check
		if (!stricmp(gameContextParams.gameRules, "SinglePlayer"))
		{
			gameContextParams.gameRules = "TeamInstantAction";
		}

		SGameStartParams gameStartParams;
		gameStartParams.flags = m_gameStartParams->flags;
		gameStartParams.port = m_gameStartParams->port;
		gameStartParams.maxPlayers = m_gameStartParams->maxPlayers;
		gameStartParams.hostname = m_gameStartParams->hostname.c_str();
		gameStartParams.connectionString = m_gameStartParams->connectionString.c_str();
		gameStartParams.pContextParams = &gameContextParams;
		gameStartParams.session = m_currentSession;

#ifdef GAME_IS_CRYSIS2
		CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
		if (pPlaylistManager)
		{
			pPlaylistManager->SetModeOptions();
		}
#endif

		m_loadingGameRules = m_currentGameRules = m_gameStartParams->gameModeName.c_str();
		m_loadingLevelName = m_currentLevelName = m_gameStartParams->levelName.c_str();

		g_pGame->GetIGameFramework()->StartGameContext(&gameStartParams);

		SAFE_DELETE(m_gameStartParams);
	}
	else
	{
		ICVar *pCVar = gEnv->pConsole->GetCVar("sv_gamerules");
		if (pCVar)
		{
			pCVar->Set(m_currentGameRules.c_str());
		}

#ifdef GAME_IS_CRYSIS2
		CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
		if (pPlaylistManager)
		{
			pPlaylistManager->IgnoreCVarChanges(true);

			pCVar = gEnv->pConsole->GetCVar("sv_maxplayers");
			if (pCVar)
			{
				uint32 maxPlayers = m_sessionData.m_numPublicSlots + m_sessionData.m_numPrivateSlots;
				pCVar->Set((int) maxPlayers);
			}

			pPlaylistManager->IgnoreCVarChanges(false);
		}
#endif

		CryFixedStringT<128> command;
		command.Format("map %s s nb", GetCurrentLevelName());
		gEnv->pConsole->ExecuteString(command.c_str());

		m_hasReceivedMapCommand = false;
	}
}

//-------------------------------------------------------------------------
void CGameLobby::OnNewGameStartParams( const SGameStartParams* pGameStartParams )
{
#ifdef GAME_IS_CRYSIS2
	CCCPOINT(GameLobby_CreateSession);
#endif

	const bool bIsMigratingSession = ((pGameStartParams->flags & eGSF_HostMigrated) != 0);

	if (bIsMigratingSession)
	{
		// Migrating sessions just need to call a matchmaking function, no longer any need for
		// the game start params
		m_taskQueue.AddTask(CLobbyTaskQueue::eST_Migrate, false);
		return;
	}

	CryFixedStringT<32> validString = GetValidMapForGameRules(pGameStartParams->pContextParams->levelName, pGameStartParams->pContextParams->gameRules, false);
	if (validString.empty() && (g_pGameCVars->autotest_enabled == 0))
	{
		// This is responding to input on the console so yes, it should be a CryLogAlways
		CryLogAlways("Map '%s' does not have a valid setup for gamerules '%s'", pGameStartParams->pContextParams->levelName, pGameStartParams->pContextParams->gameRules);
		return;
	}

	SAFE_DELETE(m_gameStartParams);
	m_gameStartParams = new SLobbyGameStartParams();

	m_gameStartParams->levelName = pGameStartParams->pContextParams->levelName;
	m_gameStartParams->gameModeName = pGameStartParams->pContextParams->gameRules;

	m_currentLevelName = m_gameStartParams->levelName;
	m_currentGameRules = m_gameStartParams->gameModeName;

	// Store the game start params, needs to be done else strings are lost from stack before callback comes back.
	m_gameStartParams->flags = pGameStartParams->flags;
	m_gameStartParams->port = pGameStartParams->port;
	m_gameStartParams->maxPlayers = pGameStartParams->maxPlayers;
	m_gameStartParams->hostname = pGameStartParams->hostname;
	m_gameStartParams->connectionString = pGameStartParams->connectionString;
	if (pGameStartParams->pContextParams)
	{
		m_gameStartParams->levelName = pGameStartParams->pContextParams->levelName;
		m_gameStartParams->gameModeName = pGameStartParams->pContextParams->gameRules;
		m_gameStartParams->demoPlaybackFilename = pGameStartParams->pContextParams->demoPlaybackFilename;
		m_gameStartParams->demoRecorderFilename = pGameStartParams->pContextParams->demoRecorderFilename;
	}

	const bool bIsInSession = (m_currentSession != CrySessionInvalidHandle);
	const bool bIsServer = (m_server);

	if (bIsInSession)
	{
		if (bIsServer)
		{
			if (m_bSessionStarted && (m_state == eLS_Game))
			{
				// Can now start the game context
				m_taskQueue.AddTask(CLobbyTaskQueue::eST_StartGameContext, true);
			}
			else
			{
				// We're the host of this session, begin starting the game
				if (m_state == eLS_Lobby)
				{
					SetState(eLS_JoinSession);
				}
			}
		}
		else
		{
			// We're in a session and we're not the host, leave this one and create our own
			LeaveSession(true);
			SetMatchmakingGame(false);
			m_taskQueue.AddTask(CLobbyTaskQueue::eST_Create, false);
		}
	}
	else
	{
		if(!m_taskQueue.HasTaskInProgress() || m_taskQueue.GetCurrentTask() != CLobbyTaskQueue::eST_Create)
		{
			m_taskQueue.AddTask(CLobbyTaskQueue::eST_Create, true);

			// Show create session dialog whether we can start the create now or not
#ifdef USE_C2_FRONTEND
			CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
			CMPMenuHub *mpMenuHub = menu ? menu->GetMPMenu() : NULL;
			if (mpMenuHub)
			{
				mpMenuHub->ShowLoadingDialog("CreateSession");
			}
#endif //#ifdef USE_C2_FRONTEND

			if (gEnv->IsDedicated())
			{
				UpdatePrivatePasswordedGame();
			}
		}
		else
		{
			CryLog("create session is already in progress, not adding a new one");
		}
	}
}

//-------------------------------------------------------------------------
bool CGameLobby::ShouldCallMapCommand( const char * pLevelName, const char *pGameRules )
{
	const bool bIsInSession = (m_currentSession != CrySessionInvalidHandle);
	const bool bIsServer = (m_server);

	m_currentLevelName = pLevelName;
	m_currentGameRules = pGameRules;

	// Always allow map command for singleplayer
	if(!gEnv->bMultiplayer && !gEnv->IsDedicated())
		return true;

	if (bIsInSession)
	{
		if (bIsServer)
		{
			if (m_bSessionStarted && (m_state == eLS_Game))
			{
				return true;
			}
			else
			{
				// We're the host of this session, begin starting the game
				if (m_state == eLS_Lobby)
				{
					SetState(eLS_JoinSession);
				}
			}
		}
		else
		{
			// We're in a session and we're not the host, leave this one and create our own
			LeaveSession(true);
			SetMatchmakingGame(false);
			m_taskQueue.AddTask(CLobbyTaskQueue::eST_Create, false);
		}
	}
	else
	{
		if(!m_taskQueue.HasTaskInProgress() || m_taskQueue.GetCurrentTask() != CLobbyTaskQueue::eST_Create)
		{
			m_taskQueue.AddTask(CLobbyTaskQueue::eST_Create, true);

			// TODO: UI show loading dialog

			if (gEnv->IsDedicated())
			{
				UpdatePrivatePasswordedGame();
			}
		}
		else
		{
			CryLog("create session is already in progress, not adding a new one");
		}
	}
	
	m_hasReceivedMapCommand = true;
	return false;
}

//-------------------------------------------------------------------------
void CGameLobby::OnMapCommandIssued()
{
	IGameFramework *pFramework = g_pGame->GetIGameFramework();
	if (pFramework->StartedGameContext() || pFramework->StartingGameContext())
	{
		// Already in a started session, need to end it before the next one loads
		gEnv->pConsole->ExecuteString("unload", false, false);
		SetState(eLS_EndSession);
		m_bServerUnloadRequired = false;
	}
}

//-------------------------------------------------------------------------
void CGameLobby::SetupSessionData()
{
	m_sessionData.m_numData = 0;

	m_userData[eLDI_Gamemode].m_id = LID_MATCHDATA_GAMEMODE;
	m_userData[eLDI_Gamemode].m_type = eCLUDT_Int32;
	m_userData[eLDI_Gamemode].m_int32 = GameLobbyData::ConvertGameRulesToHash(m_currentGameRules.c_str());
	++m_sessionData.m_numData;

	m_userData[eLDI_Version].m_id = LID_MATCHDATA_VERSION;
	m_userData[eLDI_Version].m_type = eCLUDT_Int32;
	m_userData[eLDI_Version].m_int32 = GameLobbyData::GetVersion();
	++m_sessionData.m_numData;

	m_userData[eLDI_Playlist].m_id = LID_MATCHDATA_PLAYLIST;
	m_userData[eLDI_Playlist].m_type = eCLUDT_Int32;
	m_userData[eLDI_Playlist].m_int32 = GameLobbyData::GetPlaylistId();
	++m_sessionData.m_numData;

	m_userData[eLDI_Variant].m_id = LID_MATCHDATA_VARIANT;
	m_userData[eLDI_Variant].m_type = eCLUDT_Int32;
	m_userData[eLDI_Variant].m_int32 = GameLobbyData::GetVariantId();
	++m_sessionData.m_numData;

	m_userData[eLDI_RequiredDLCs].m_id = LID_MATCHDATA_REQUIRED_DLCS;
	m_userData[eLDI_RequiredDLCs].m_type = eCLUDT_Int32;
#ifdef GAME_IS_CRYSIS2
	m_userData[eLDI_RequiredDLCs].m_int32 = g_pGame->GetDLCManager()->GetRequiredDLCs();
#else
	m_userData[eLDI_RequiredDLCs].m_int32 = 0;
#endif
	++m_sessionData.m_numData;

#if USE_CRYLOBBY_GAMESPY
	m_userData[eLDI_Official].m_id = LID_MATCHDATA_OFFICIAL;
	m_userData[eLDI_Official].m_type = eCLUDT_Int32;
	m_userData[eLDI_Official].m_int32 = GameLobbyData::GetOfficialServer();
	++m_sessionData.m_numData;

	m_userData[eLDI_Region].m_id = LID_MATCHDATA_REGION;
	m_userData[eLDI_Region].m_type = eCLUDT_Int32;
	m_userData[eLDI_Region].m_int32 = 0;
	++m_sessionData.m_numData;

	m_userData[eLDI_FavouriteID].m_id = LID_MATCHDATA_FAVOURITE_ID;
	m_userData[eLDI_FavouriteID].m_type = eCLUDT_Int32;
	m_userData[eLDI_FavouriteID].m_int32 = INVALID_SESSION_FAVOURITE_ID;
	++m_sessionData.m_numData;
#endif

	m_userData[eLDI_Language].m_id = LID_MATCHDATA_LANGUAGE;
	m_userData[eLDI_Language].m_type = eCLUDT_Int32;
	m_userData[eLDI_Language].m_int32 = GetCurrentLanguageId();
	++m_sessionData.m_numData;

	m_userData[eLDI_Map].m_id = LID_MATCHDATA_MAP;
	m_userData[eLDI_Map].m_type = eCLUDT_Int32;
	m_userData[eLDI_Map].m_int32 = GameLobbyData::ConvertMapToHash(m_currentLevelName.c_str());
	++m_sessionData.m_numData;

	m_userData[eLDI_Skill].m_id = LID_MATCHDATA_SKILL;
	m_userData[eLDI_Skill].m_type = eCLUDT_Int32;
#ifdef GAME_IS_CRYSIS2
	m_userData[eLDI_Skill].m_int32 = CPlayerProgression::GetInstance()->GetData(EPP_SkillRank);
#else
	m_userData[eLDI_Skill].m_int32 = 0;
#endif
	++m_sessionData.m_numData;

	m_userData[eLDI_Active].m_id = LID_MATCHDATA_ACTIVE;
	m_userData[eLDI_Active].m_type = eCLUDT_Int32;
	m_userData[eLDI_Active].m_int32 = 0;
	++m_sessionData.m_numData;

	m_sessionData.m_data = m_userData;

	ICVar* pMaxPlayers = gEnv->pConsole->GetCVar("sv_maxplayers");

	if(!m_privateGame)
	{
		m_sessionData.m_numPublicSlots = pMaxPlayers ? pMaxPlayers->GetIVal() : MAX_PLAYER_LIMIT;
		m_sessionData.m_numPrivateSlots = 0;
	}
	else
	{
		m_sessionData.m_numPublicSlots = 0; 
		m_sessionData.m_numPrivateSlots = pMaxPlayers ? pMaxPlayers->GetIVal() : MAX_PLAYER_LIMIT;
	}
	
	memset( m_sessionData.m_name, 0, MAX_SESSION_NAME_LENGTH );
	ICVar* pServerNameCVar = gEnv->pConsole->GetCVar("sv_servername");
	if (pServerNameCVar)
	{
		if (pServerNameCVar->GetFlags() & VF_MODIFIED)
		{
			cry_strncpy(m_sessionData.m_name, pServerNameCVar->GetString(), MAX_SESSION_NAME_LENGTH);
		}
		else
		{
			IPlatformOS::TUserName userName;

			IPlayerProfileManager *pProfileManager = g_pGame->GetIGameFramework()->GetIPlayerProfileManager();
			if (pProfileManager)
			{
				const uint32 userIndex = pProfileManager->GetExclusiveControllerDeviceIndex();

				IPlatformOS *pPlatformOS = GetISystem()->GetPlatformOS();
				if (!pPlatformOS->UserGetOnlineName(userIndex, userName))
				{
					pPlatformOS->UserGetName(userIndex, userName);
				}
			}

			cry_strncpy(m_sessionData.m_name, userName.c_str(), MAX_SESSION_NAME_LENGTH);
		}
	}
	else
	{
		cry_strncpy(m_sessionData.m_name, "default servername", MAX_SESSION_NAME_LENGTH);
	}

	m_sessionData.m_ranked = false;

	if( CMatchMakingHandler* pMMHandler = m_gameLobbyMgr->GetMatchMakingHandler() )
	{
		pMMHandler->AdjustCreateSessionData( &m_sessionData, eLDI_Num );
	}

}

//-------------------------------------------------------------------------
ECryLobbyError CGameLobby::DoCreateSession(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId)
{
	CryLog("CGameLobby::DoCreateSession() pLobby=%p", this);
	CRY_ASSERT(m_taskQueue.GetCurrentTask() == CLobbyTaskQueue::eST_Create);

	ECryLobbyError result = eCLE_ServiceNotSupported;

	uint32 users = GameLobby_GetCurrentUserIndex();

	SetupSessionData();

	
	m_nameList.Clear();
	m_sessionFavouriteKeyId = INVALID_SESSION_FAVOURITE_ID;

	SetState(eLS_Initializing);
	result = pMatchMaking->SessionCreate(&users, 1, GetSessionCreateFlags(), &m_sessionData, &taskId, CGameLobby::MatchmakingSessionCreateCallback, this);
	CryLog("CGameLobby::CreateSession() error=%i, Lobby: %p", result, this);

	// If the error is too many tasks, we'll try again next frame so don't need to clear the dialog
	if ((result != eCLE_Success) && (result != eCLE_TooManyTasks))
	{
		if (!m_bMatchmakingSession)
		{
#ifdef USE_C2_FRONTEND
			CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
			CMPMenuHub *mpMenuHub = menu ? menu->GetMPMenu() : NULL;
			if (mpMenuHub)
			{
				mpMenuHub->HideLoadingDialog("CreateSession");
			}
#endif //#ifdef USE_C2_FRONTEND
		}
	}

	return result;
}

//-------------------------------------------------------------------------
ECryLobbyError CGameLobby::DoMigrateSession(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId)
{
	CryLog("CGameLobby::DoMigrateSession() pLobby=%p", this);
	CRY_ASSERT(m_taskQueue.GetCurrentTask() == CLobbyTaskQueue::eST_Migrate);

	uint32 users = GameLobby_GetCurrentUserIndex();

	ECryLobbyError result = eCLE_ServiceNotSupported;

	result = pMatchMaking->SessionMigrate(m_currentSession, &users, 1, GetSessionCreateFlags(), &m_sessionData, &taskId, CGameLobby::MatchmakingSessionMigrateCallback, this);

	return result;
}

//-------------------------------------------------------------------------
uint32 CGameLobby::GetSessionCreateFlags() const
{
	uint32 flags = CRYSESSION_CREATE_FLAG_NUB | CRYSESSION_LOCAL_FLAG_CAN_SEND_SERVER_PING;

	if (!gEnv->IsDedicated())
	{
		flags |= CRYSESSION_CREATE_FLAG_MIGRATABLE;
	}

	if (!m_privateGame || IsPasswordedGame())
	{
		flags |= CRYSESSION_CREATE_FLAG_SEARCHABLE;
	}

	return flags;
}

//-------------------------------------------------------------------------
void CGameLobby::SetPrivateGame( bool enable )
{
	CryLog("CGameLobby::SetPrivateGame(enable=%s)", (enable?"true":"false"));
	m_privateGame = enable;
	m_gameLobbyMgr->SetPrivateGame(this, enable);
}

//-------------------------------------------------------------------------
CryUserID CGameLobby::GetHostUserId()
{
	CryUserID result = CryUserInvalidID;

	ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
	if (pLobby)
	{
		ICryMatchMaking *pMatchmaking = pLobby->GetMatchMaking();
		if (pMatchmaking)
		{
			SCryMatchMakingConnectionUID hostUID = pMatchmaking->GetHostConnectionUID(m_currentSession);
			GetUserIDFromConnectionUID(hostUID, result);
		}
	}

	return result;
}

//-------------------------------------------------------------------------
const char* CGameLobby::GetCurrentGameModeName(const char* unknownStr/*="Unknown"*/)
{
	if (!m_currentGameRules.empty())
	{
		return m_currentGameRules.c_str();
	}
	return unknownStr;
}

//-------------------------------------------------------------------------
void CGameLobby::CancelLobbyTask(CLobbyTaskQueue::ESessionTask taskType)
{
	CryLog("CancelLobbyTask %d", taskType);

	// If this is the task that's running, might need to cancel the matchmaking task
	if (m_taskQueue.GetCurrentTask() == taskType)
	{
		CRY_ASSERT_MESSAGE(taskType != CLobbyTaskQueue::eST_Create && taskType != CLobbyTaskQueue::eST_Join && taskType != CLobbyTaskQueue::eST_Delete, "Trying to cancel a lobby task that shouldn't be canceled");

		CryLobbyTaskID networkTaskId = m_currentTaskId;
		CryLog("currentTaskId %d", m_currentTaskId);
		if (networkTaskId != CryLobbyInvalidTaskID)
		{
			ICryLobby *lobby = gEnv->pNetwork->GetLobby();
			if (lobby != NULL && lobby->GetLobbyService())
			{
				CryLog("CGameLobby::CancelLobbyTask taskType=%u, networkTaskId=%u Lobby: %p", taskType, networkTaskId, this);

				lobby->GetLobbyService()->GetMatchMaking()->CancelTask(networkTaskId);
				m_currentTaskId = CryLobbyInvalidTaskID;
			}
		}
	}

	// Now remove the task from our queue
	m_taskQueue.CancelTask(taskType);
}

//-------------------------------------------------------------------------
void CGameLobby::CancelAllLobbyTasks()
{
	CryLog("CGameLobby::CancelAllLobbyTasks Lobby: %p", this);

	if (m_taskQueue.HasTaskInProgress())
	{
		CancelLobbyTask(m_taskQueue.GetCurrentTask());
	}

	m_taskQueue.Reset();

#ifdef USE_C2_FRONTEND
	CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
	CMPMenuHub *mpMenuHub = menu ? menu->GetMPMenu() : NULL;
	if (mpMenuHub)
	{
		mpMenuHub->SearchComplete();
	}
#endif //#ifdef USE_C2_FRONTEND

	if (m_gameLobbyMgr->IsPrimarySession(this))
	{
		CGameBrowser *pGameBrowser = g_pGame->GetGameBrowser();

		if ( pGameBrowser )
		{
			pGameBrowser->CancelSearching();
		}
		
		CGameLobby::s_bShouldBeSearching = true;
	}
}

//-------------------------------------------------------------------------
void CGameLobby::SetCurrentSession(CrySessionHandle h)
{
	m_currentSession = h;

	if (h == CrySessionInvalidHandle)  // there's no point in holding on to an invalid id if the handle is being made invalid... (in fact not doing this was causing bugs such as a new squad member being dragged into a game that the leader was no longer in when re-joining the squad)
	{
		SetCurrentId(CrySessionInvalidID, false, false);
	}
	else
	{
		if (!m_server)
		{
			m_startTimerCountdown = false;
			SendPacket(eGUPD_ReservationClientIdentify);	//Always identify (otherwise need to know if we have a reservation)
		}
	}
}

//-------------------------------------------------------------------------
void CGameLobby::SetLocalUserData(uint8 * localUserData)
{
#ifdef GAME_IS_CRYSIS2
	CPlayerProgression* pPlayerProgression = CPlayerProgression::GetInstance();
	localUserData[eLUD_Rank] = pPlayerProgression->GetData(EPP_DisplayRank);
	localUserData[eLUD_LoadedDLCs] = (uint8)g_pGame->GetDLCManager()->GetLoadedDLCs();
#else
	localUserData[eLUD_Rank] = 0;
	localUserData[eLUD_LoadedDLCs] = 0;
#endif

#ifdef GAME_IS_CRYSIS2
	//Dogtags
	CDogTagProgression* pDogTagProgression = CDogTagProgression::GetInstance();
	uint8 special = pDogTagProgression->GetCurrentDogTagSpecial();
	uint8 style = pDogTagProgression->GetCurrentDogTagSkinStyle();
	CRY_ASSERT(special < 16);
	CRY_ASSERT(style < 16);
	localUserData[eLUD_DogtagStyle] = style | (special << 4);
	localUserData[eLUD_DogtagId] = pDogTagProgression->GetCurrentDogTagSkinId();
#endif

	// Reincarnation is first 4 bits, leave voting bits alone.
	uint8 reincarnationAndVote = localUserData[eLUD_ReincarnationsAndVoteChoice];
#ifdef GAME_IS_CRYSIS2
	uint8 numReincarnations = (uint8)pPlayerProgression->GetData(EPP_Reincarnate);
#else
	uint8 numReincarnations = 0;
#endif
	CRY_ASSERT( (numReincarnations & 0xF0)==0 );

	reincarnationAndVote = (reincarnationAndVote & 0xF);
	reincarnationAndVote |= ((numReincarnations << 4) & 0xF0);

	localUserData[eLUD_ReincarnationsAndVoteChoice] = reincarnationAndVote;


	// ClanTag
#ifdef GAME_IS_CRYSIS2
	CProfileOptions *pProfileOptions = g_pGame->GetProfileOptions();
	if (pProfileOptions)
	{
		CryFixedStringT<32> option("MP/OperativeStatus/ClanTag");
		if(pProfileOptions->IsOption(option.c_str()))
		{
			CryFixedStringT<CLAN_TAG_LENGTH> clanTag(pProfileOptions->GetOptionValue(option.c_str()).c_str() ); // 4 chars + terminator
			char *str = clanTag.begin();
			int length = clanTag.length();

			for(int i = 0, idx = eLUD_ClanTag1; i < length; i++, idx++)
			{
				localUserData[idx] = str[i];
			}
		}
	}
#endif

#ifdef GAME_IS_CRYSIS2
	int skillRank = CPlayerProgression::GetInstance()->GetData(EPP_SkillRank);
#else
	int skillRank = 0;
#endif
	skillRank = CLAMP(skillRank, 0, 0xFFFF);		// Should be impossible to get outside this range, but just to be sure ...
	localUserData[eLUD_SkillRank1] = (skillRank & 0xFF);
	localUserData[eLUD_SkillRank2] = ((skillRank & 0xFF00) >> 8);
}

//-------------------------------------------------------------------------
ECryLobbyError CGameLobby::DoUpdateLocalUserData(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId)
{
	CryLog("CGameLobby::DoUpdateLocalUserData() pLobby=%p", this);
	CRY_ASSERT(m_taskQueue.GetCurrentTask() == CLobbyTaskQueue::eST_SetLocalUserData);

	uint8 localUserData[CRYLOBBY_USER_DATA_SIZE_IN_BYTES] = {0};

	SetLocalUserData(localUserData);

	// Voting is the last 4 bits, leave reincarnation bits alone.
	uint8 reincarnationAndVote = localUserData[eLUD_ReincarnationsAndVoteChoice];
	uint8 voteChoice = (uint8)m_networkedVoteStatus;
	CRY_ASSERT( (voteChoice & 0xF0)==0 );

	reincarnationAndVote = (reincarnationAndVote & 0xF0);
	reincarnationAndVote |= (voteChoice & 0xF);

	localUserData[eLUD_ReincarnationsAndVoteChoice] = reincarnationAndVote;

	uint32 squadLeaderUID = 0;
	CryUserID pSquadLeaderId = g_pGame->GetSquadManager()->GetSquadLeader();
	if (pSquadLeaderId.IsValid())
	{
		SCryMatchMakingConnectionUID conId;
		if (GetConnectionUIDFromUserID(pSquadLeaderId, conId))
		{
			squadLeaderUID = conId.m_uid;
			m_squadDirty = false;
		}
	}
	else
	{
		m_squadDirty = false;
	}
	localUserData[eLUD_SquadId1] = (squadLeaderUID & 0xFF);
	localUserData[eLUD_SquadId2] = ((squadLeaderUID & 0xFF00) >> 8);

	ICryMatchMaking *pMatchmaking = gEnv->pNetwork->GetLobby()->GetLobbyService()->GetMatchMaking();
	uint32 userIndex = GameLobby_GetCurrentUserIndex();
	ECryLobbyError result = pMatchmaking->SessionSetLocalUserData(m_currentSession, &taskId, userIndex, localUserData, sizeof(localUserData), MatchmakingLocalUserDataCallback, this);

	return result;
}

//-------------------------------------------------------------------------
void CGameLobby::SetCurrentId(CrySessionID id, bool isCreator, bool isMigratedSession)
{
	CryLog("[tlh] CGameLobby::SetCurrentId(CrySessionID id = %d)", (int)+id);

	m_currentSessionId = id;
	m_bMigratedSession = isMigratedSession;
}


//-------------------------------------------------------------------------
// Connect to a server
bool CGameLobby::JoinServer(CrySessionID sessionId, const char *sessionName, const SCryMatchMakingConnectionUID &reservationId, bool bRetryIfPassworded )
{
	CryLog("CGameLobby::JoinServer(CrySessionID sessionId = %d, sessionName = %s)", (int)+sessionId, sessionName);

	if (!sessionId)
	{
		CryLog("  NULL sessionId requested");
		return false;	
	}
	
	if (NetworkUtils_CompareCrySessionId(m_currentSessionId, sessionId))
	{
		CryLog("  already in requested session");
		return false;	
	}

	m_pendingConnectSessionId = sessionId;
	m_pendingConnectSessionName = sessionName ? sessionName : " ";
	m_pendingReservationId = reservationId;

	m_bRetryIfPassworded = bRetryIfPassworded;

	// Show joining dialog regardless of whether we can join immediately
	if (m_gameLobbyMgr->IsPrimarySession(this) && !m_bMatchmakingSession)
	{
#ifdef USE_C2_FRONTEND
		CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
		CMPMenuHub *mpMenuHub = menu ? menu->GetMPMenu() : NULL;
		if (mpMenuHub)
		{
			CryLog("JoinServer %s", m_pendingConnectSessionName.c_str());
			mpMenuHub->ShowLoadingDialog("JoinSession");
		}
#endif //#ifdef USE_C2_FRONTEND
	}

	if (m_currentSession != CrySessionInvalidHandle)
	{
		LeaveSession(false);
	}

	m_taskQueue.AddTask(CLobbyTaskQueue::eST_Join, false);

#if !defined(_RELEASE)
	if (gl_debugForceLobbyMigrations)
	{
		gEnv->pConsole->GetCVar("net_hostHintingActiveConnectionsOverride")->Set(2);
		CryLog("  setting net_hostHintingActiveConnectionsOverride to 2");
	}
#endif

	return true;
}

//-------------------------------------------------------------------------
ECryLobbyError CGameLobby::DoJoinServer(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId)
{
	CryLog("CGameLobby::DoJoinServer() pLobby=%p", this);
	CRY_ASSERT(m_taskQueue.GetCurrentTask() == CLobbyTaskQueue::eST_Join);

	ECryLobbyError joinResult = eCLE_ServiceNotSupported;

#ifdef GAME_IS_CRYSIS2
	// Request a server join on the lobby system.
	CCCPOINT (GameLobby_JoinServer);
#endif

	uint32 user = GameLobby_GetCurrentUserIndex();

	SetState(eLS_Initializing);

	m_shouldFindGames = false;

	m_server = false;
	m_connectedToDedicatedServer = false;
	CryLog("JoinServer - clear the name list Lobby: %p", this);
	m_nameList.Clear();

	joinResult = pMatchMaking->SessionJoin(&user, 1, CRYSESSION_CREATE_FLAG_SEARCHABLE | CRYSESSION_CREATE_FLAG_NUB, m_pendingConnectSessionId, &taskId, CGameLobby::MatchmakingSessionJoinCallback, this);

	if ((joinResult != eCLE_Success) && (joinResult != eCLE_TooManyTasks))
	{
		CryLog("CGameLobby::DoJoinServer() failed to join server, error=%i", joinResult);
		JoinServerFailed(joinResult, m_pendingConnectSessionId);
		m_pendingConnectSessionId = CrySessionInvalidID;
		m_pendingConnectSessionName.clear();
	
		// Hide joining dialog
		if (m_gameLobbyMgr->IsPrimarySession(this) && !m_bMatchmakingSession)
		{
#ifdef USE_C2_FRONTEND
			CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
			CMPMenuHub *mpMenuHub = menu ? menu->GetMPMenu() : NULL;
			if (mpMenuHub)
			{
				mpMenuHub->HideLoadingDialog("JoinSession");
			}
#endif //#ifdef USE_C2_FRONTEND
		}

		if( m_bMatchmakingSession )
		{
			if( CMatchMakingHandler* pMMHandler = m_gameLobbyMgr->GetMatchMakingHandler() )
			{
				pMMHandler->GameLobbyJoinFinished( joinResult );
			}
		}
	}

	return joinResult;
}

//-------------------------------------------------------------------------
ECryLobbyError CGameLobby::DoUpdateSession(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId)
{
	CryLog("CGameLobby::DoUpdateSession() pLobby=%p", this);
	CRY_ASSERT(m_taskQueue.GetCurrentTask() == CLobbyTaskQueue::eST_Update);

	CRY_ASSERT(m_currentSession != CrySessionInvalidHandle);
	CRY_ASSERT(m_server);

	const EActiveStatus activeStatus = GetActiveStatus(m_state);
	m_lastActiveStatus = activeStatus;
	const int averageSkill = CalculateAverageSkill();
	m_lastUpdatedAverageSkill = averageSkill;

	uint32	numData = 0;

	m_userData[eLDI_Gamemode].m_int32 = GameLobbyData::ConvertGameRulesToHash(m_currentGameRules.c_str());
	++numData;
	m_userData[eLDI_Map].m_int32 = GameLobbyData::ConvertMapToHash(m_currentLevelName.c_str());
	++numData;
	m_userData[eLDI_Active].m_int32 = (int32)activeStatus;
	++numData;
	m_userData[eLDI_Version].m_int32 = GameLobbyData::GetVersion();
	++numData;
#ifdef GAME_IS_CRYSIS2
	m_userData[eLDI_RequiredDLCs].m_int32 = g_pGame->GetDLCManager()->GetRequiredDLCs();
#else
	m_userData[eLDI_RequiredDLCs].m_int32 = 0;
#endif
	++numData;
	m_userData[eLDI_Playlist].m_int32 = GameLobbyData::GetPlaylistId();
	++numData;
	m_userData[eLDI_Variant].m_int32 = GameLobbyData::GetVariantId();
	++numData;
	m_userData[eLDI_Skill].m_int32 = averageSkill;
	++numData;
	m_userData[eLDI_Language].m_int32 = GetCurrentLanguageId();
	++numData;
#if USE_CRYLOBBY_GAMESPY
	m_userData[eLDI_Official].m_int32 = GameLobbyData::GetOfficialServer();
	++numData;
	m_userData[eLDI_Region].m_int32 = 0;
	++numData;
	m_userData[eLDI_FavouriteID].m_int32 = INVALID_SESSION_FAVOURITE_ID;
	++numData;
#endif

	if( CMatchMakingHandler* pMMHandler = m_gameLobbyMgr->GetMatchMakingHandler() )
	{
		pMMHandler->AdjustCreateSessionData( &m_sessionData, eLDI_Num );
	}

	ECryLobbyError result = pMatchMaking->SessionUpdate(m_currentSession, &m_userData[0], numData, &taskId, CGameLobby::MatchmakingSessionUpdateCallback, this);
	return result;
}

//-------------------------------------------------------------------------
// Delete the current session.
ECryLobbyError CGameLobby::DoDeleteSession(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId)
{
	CryLog("CGameLobby::DoDeleteSession() pLobby=%p", this);
	CRY_ASSERT(m_taskQueue.GetCurrentTask() == CLobbyTaskQueue::eST_Delete);

	ECryLobbyError result = eCLE_ServiceNotSupported;

	// Request a server delete in the lobby system.
	result = pMatchMaking->SessionDelete(m_currentSession, &taskId, CGameLobby::MatchmakingSessionDeleteCallback, this);

	if ((result != eCLE_Success) && (result != eCLE_TooManyTasks))
	{
		FinishDelete(result);
	}

	return result;
}

//-------------------------------------------------------------------------
// Migrate callback.
void CGameLobby::MatchmakingSessionMigrateCallback(CryLobbyTaskID taskID, ECryLobbyError error, CrySessionHandle h, void* arg)
{
	ENSURE_ON_MAIN_THREAD;

	CGameLobby *pLobby = static_cast<CGameLobby *>(arg);
	if (pLobby)
	{
		if (pLobby->NetworkCallbackReceived(taskID, error))
		{
			CRY_ASSERT(pLobby->m_currentSession == h);

			pLobby->m_server = true;
			pLobby->m_connectedToDedicatedServer = false;
			pLobby->GameRulesChanged(pLobby->m_currentGameRules.c_str());

#if defined (TRACK_MATCHMAKING)
			if( CMatchmakingTelemetry* pMMTel = g_pGame->GetMatchMakingTelemetry() )
			{
				pMMTel->AddEvent( SMMBecomeHostEvent() );
			}
#endif
		}
	}
}

//-------------------------------------------------------------------------
// Create callback.
// Starts the server game.
void CGameLobby::MatchmakingSessionCreateCallback(CryLobbyTaskID taskID, ECryLobbyError error, CrySessionHandle h, void* arg)
{
	ENSURE_ON_MAIN_THREAD;

	CGameLobby *pLobby = static_cast<CGameLobby *>(arg);
#ifdef USE_C2_FRONTEND
	CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
	CMPMenuHub *mpMenuHub = menu ? menu->GetMPMenu() : NULL;
#endif //#ifdef USE_C2_FRONTEND

	CryLog("CGameLobby::MatchmakingSessionCreateCallback() result=%i Lobby: %p", error, pLobby);
	if (pLobby->NetworkCallbackReceived(taskID, error))
	{
		ICryMatchMaking *pMatchmaking = gEnv->pNetwork->GetLobby()->GetLobbyService()->GetMatchMaking();
		CryLog("  calling SetCurrentSession() h=%u Lobby: %p", h, pLobby);
		
		pLobby->m_server = true;
		pLobby->m_connectedToDedicatedServer = false;
		pLobby->m_findGameNumRetries = 0;
		
		pLobby->SetCurrentSession(h);
		pLobby->SetCurrentId(pMatchmaking->SessionGetCrySessionIDFromCrySessionHandle(h), true, false);

		if(!pLobby->m_bCancelling)
		{
			pLobby->m_taskQueue.AddTask(CLobbyTaskQueue::eST_SessionSetLocalFlags, false);
			pLobby->m_bNeedToSetAsElegibleForHostMigration = false;

			if(pLobby->m_bMatchmakingSession)
			{
				//If this is not set, we haven't created a session from 'Find Game', and so
				//	don't want to search for more lobbies
				CryLog("  > Setting ShouldBeSearching to TRUE to look for further lobbies to merge / join");
				CGameLobby::s_bShouldBeSearching = true;
				pLobby->m_shouldFindGames = true;
			}
			else
			{
				CryLog("  > Setting ShouldBeSearching to FALSE as we're not a matchmaking session");
				CGameLobby::s_bShouldBeSearching = false;
			}

			if ((pLobby->m_gameStartParams == NULL) || !(pLobby->m_gameStartParams->flags & eGSF_HostMigrated))
			{
				if(pLobby->IsPrivateGame())
				{
					CSquadManager *pSquadManager = g_pGame->GetSquadManager();
					if(pSquadManager)
					{
						pSquadManager->SessionChangeSlotType(CSquadManager::eSST_Private);
					}
				}

#ifdef GAME_IS_CRYSIS2
				CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
				uint32 playListSeed = (uint32)(gEnv->pTimer->GetFrameStartTime(ITimer::ETIMER_UI).GetValue());
				playListSeed &= 0x7FFFFFFF;	// Make sure this doesn't become negative when converted to int32
				if (pPlaylistManager)
				{
					ILevelRotation* pLevelRotation = pPlaylistManager->GetLevelRotation();
					if (pLevelRotation != NULL && pLevelRotation->IsRandom())
					{
						pLevelRotation->Shuffle(playListSeed);
					}
				}
				pLobby->m_playListSeed = playListSeed;
#endif

				pLobby->SetState(eLS_Lobby);
				pLobby->GameRulesChanged(pLobby->m_currentGameRules.c_str());

#ifdef GAME_IS_CRYSIS2
				if (pPlaylistManager)
				{
					if (pPlaylistManager->GetActiveVariantIndex() == -1)
					{
						// Set default variant
						int defaultVariant = pPlaylistManager->GetDefaultVariant();
						if (defaultVariant != -1)
						{
							pPlaylistManager->ChooseVariant(defaultVariant);
						}
					}

					pPlaylistManager->SetModeOptions();
				}
#endif

				cry_strncpy(pLobby->m_detailedServerInfo.m_motd, g_pGameCVars->g_messageOfTheDay->GetString(), DETAILED_SESSION_INFO_MOTD_SIZE);
				cry_strncpy(pLobby->m_detailedServerInfo.m_url, g_pGameCVars->g_serverImageUrl->GetString(), DETAILED_SESSION_INFO_URL_SIZE);

#if USE_CRYLOBBY_GAMESPY
				if (!gEnv->IsDedicated())
				{
					if (gEnv->pNetwork->GetLobby()->GetLobbyServiceType() != eCLS_LAN)
					{
						pLobby->m_taskQueue.AddTask(CLobbyTaskQueue::eST_Query, true);
					}
				}
#endif

#if defined (TRACK_MATCHMAKING)
				//we have made a session, record this successful decison
				if( CMatchmakingTelemetry* pMMTel = g_pGame->GetMatchMakingTelemetry() )
				{
					if( pMMTel->GetCurrentTranscriptType() == CMatchmakingTelemetry::eMMTelTranscript_QuickMatch )
					{
						pMMTel->AddEvent( SMMChosenSessionEvent( pLobby->m_sessionData.m_name, pLobby->m_currentSessionId, ORIGINAL_MATCHMAKING_DESC, true, pLobby->s_currentMMSearchID, pLobby->m_gameLobbyMgr->IsPrimarySession(pLobby) ) );
					}
				}
#endif

			}

#ifdef USE_C2_FRONTEND
			if (mpMenuHub && !pLobby->m_bMatchmakingSession)
			{
				mpMenuHub->HideLoadingDialog("CreateSession");
			}

			if(menu != NULL && menu->IsScreenInStack("game_lobby")==false)
			{
				menu->Execute(CFlashFrontEnd::eFECMD_goto, "game_lobby");
			}
#endif //#ifdef USE_C2_FRONTEND

			pLobby->InformSquadManagerOfSessionId();
			pLobby->RefreshCustomiseEquipment();
		}
	}
	else
	{
		if (error == eCLE_UserNotSignedIn)
		{
			if (gEnv->IsDedicated())
			{
				CryFatalError("User not signed in - please check dedicated.cfg credentials, and sv_bind in system.cfg is correct (if needed)");
			}
		}
	}
}

//-------------------------------------------------------------------------
// Do the actual game-connect on successful find.
// arg is CGameLobby*
void CGameLobby::MatchmakingSessionJoinCallback(CryLobbyTaskID taskID, ECryLobbyError error, CrySessionHandle h, uint32 ip, uint16 port, void* arg)
{
	ENSURE_ON_MAIN_THREAD;

	CGameLobby *pLobby = static_cast<CGameLobby *>(arg);

	CryLog("CGameLobby::MatchmakingSessionJoinCallback() result=%i Lobby: %p", error, pLobby);
	if (pLobby->NetworkCallbackReceived(taskID, error))
	{
		CryLog("CGameLobby::MatchmakingSessionJoinCallback");
		CryLog("  calling SetCurrentSession() h=%u Lobby: %p", h, pLobby);

		ICryMatchMaking *pMatchmaking = gEnv->pNetwork->GetLobby()->GetLobbyService()->GetMatchMaking();
		
		// set session info
		pLobby->SetCurrentSession(h);
		pLobby->SetCurrentId(pMatchmaking->SessionGetCrySessionIDFromCrySessionHandle(h), false, false);

		pLobby->m_allowRemoveUsers = true;

		if(!pLobby->m_bCancelling)
		{
			// need the name for loading screen
			pLobby->m_currentSessionName = pLobby->m_pendingConnectSessionName;

			// Clear pending join info
			pLobby->m_pendingConnectSessionId = CrySessionInvalidID;
			pLobby->m_pendingConnectSessionName.clear();

			pLobby->m_bNeedToSetAsElegibleForHostMigration = true;

			pLobby->SetState(eLS_Lobby);

			if(pLobby->m_gameLobbyMgr->IsNewSession(pLobby))
			{
				CGameLobby *pLobby2 = g_pGame->GetGameLobby();
				SSessionNames *pSessionNames = &pLobby2->m_nameList;
				if (pSessionNames->Size() > 1)
				{
					CryLog("    making reservations Lobby: %p", pLobby);
					//Make reservations for our primary (the one we are hosting) namelist
					pLobby->MakeReservations(pSessionNames, false);
				}
				else
				{
					CryLog("    no need to make reservations, continuing with merge Lobby: %p", pLobby);
					pLobby->m_gameLobbyMgr->CompleteMerge(pLobby->m_currentSessionId);
				}
			}

			cry_sprintf(pLobby->m_joinCommand, sizeof(pLobby->m_joinCommand), "connect <session>%d,%d.%d.%d.%d:%d", h, ((uint8*)&ip)[0], ((uint8*)&ip)[1], ((uint8*)&ip)[2], ((uint8*)&ip)[3], port);

			pLobby->InformSquadManagerOfSessionId();
			
			pLobby->CancelDetailedServerInfoRequest();	// don't want this anymore
		}

		pLobby->m_findGameNumRetries = 0;
	}
	else if (error != eCLE_TimeOut)
	{
		bool bJoinFailed = true;
		if (error == eCLE_PasswordIncorrect)
		{
#ifdef USE_C2_FRONTEND
			TFlashFrontEndPtr pFlashFrontEnd = g_pGame->GetFlashFrontEnd();
			if (pFlashFrontEnd)
			{
				CMPMenuHub *pMenuHub = pFlashFrontEnd->GetMPMenu();
				if (pMenuHub)
				{
					pMenuHub->ServerRequiresPassword(pLobby->m_pendingConnectSessionId);
				}
			}
#endif //#ifdef USE_C2_FRONTEND

			if (pLobby->m_bRetryIfPassworded)
			{
				bJoinFailed = false;

#ifdef USE_C2_FRONTEND
				CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
				CMPMenuHub *mpMenuHub = menu ? menu->GetMPMenu() : NULL;
				if (mpMenuHub)
				{
					mpMenuHub->HideLoadingDialog("JoinSession");
				}
#endif //#ifdef USE_C2_FRONTEND

#ifdef GAME_IS_CRYSIS2
				g_pGame->GetWarnings()->AddInputFieldWarning("ServerListPassword", pLobby, 24, NULL, NULL, true);
#endif
			}
		}

		if (bJoinFailed)
		{
			pLobby->JoinServerFailed(error, pLobby->m_pendingConnectSessionId);
		}

		pLobby->m_allowRemoveUsers = true;
	}
	pLobby->m_bRetryIfPassworded = false;

	if( pLobby->m_bMatchmakingSession )
	{
		if( CMatchMakingHandler* pMMHandler = pLobby->m_gameLobbyMgr->GetMatchMakingHandler() )
		{
			pMMHandler->GameLobbyJoinFinished( error );
		}
	}
}

//-------------------------------------------------------------------------
void CGameLobby::JoinServerFailed(ECryLobbyError error, CrySessionID serverSessionId)
{
	CryLog("GameLobby session join error %d", (int)error);

	// don't add full servers to the bad list, they won't be returned
	// if still full the next time we search, it was likely that we just tried to
	// join at the same time as someone else into the remaining slot,
	// if its empty next time, then we might still have a chance
	// of joining
	if(error != eCLE_SessionFull)
	{
		if(serverSessionId != CrySessionInvalidID)
		{
			if((int)m_badServers.size() > m_badServersHead)
			{
				m_badServers[m_badServersHead] = serverSessionId;
			}
			else
			{
				m_badServers.push_back(serverSessionId);
			}
			m_badServersHead = (m_badServersHead + 1) % k_maxBadServers;
#if defined (TRACK_MATCHMAKING)
			if( CMatchmakingTelemetry* pMMTel = g_pGame->GetMatchMakingTelemetry() )
			{
				pMMTel->AddEvent( SMMServerConnectFailedEvent( serverSessionId, m_gameLobbyMgr->IsPrimarySession(this), error ) );
			}
#endif
		}
	}

	ICVar *pCVar = gEnv->pConsole->GetCVar("sv_password");
	if (pCVar != NULL && ((pCVar->GetFlags() & VF_WASINCONFIG) == 0))
	{
		pCVar->Set("");
	}

	if(m_gameLobbyMgr->IsNewSession(this))
	{
		m_gameLobbyMgr->NewSessionResponse(this, CrySessionInvalidID);
	}
	else //if IsPrimarySession
	{
		if (!m_bCancelling)
		{
			CSquadManager *pSquadManager = g_pGame->GetSquadManager();
			bool isSquadMember = pSquadManager != NULL && pSquadManager->InSquad() && !pSquadManager->InCharge() && !pSquadManager->IsLeavingSquad();
			
			if(isSquadMember)
			{
				CryLog("Failed to join GAME session our squad is in and we're a squad member, leaving squad...");
				pSquadManager->RequestLeaveSquad();
				ShowErrorDialog(error, NULL, NULL, this);	//auto removes Join Session Dialog
#ifdef USE_C2_FRONTEND
				CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
				if(menu != NULL && menu->IsScreenInStack("game_lobby")==true)
				{
					if (CMPMenuHub *pMPMenu = CMPMenuHub::GetMPMenuHub())
					{
						pMPMenu->GoToCurrentLobbyServiceScreen(); // go to correct lobby service screen - play_online or play_lan
					}
				}
#endif //#ifdef USE_C2_FRONTEND
			}
			else
			{
				if( ! m_bMatchmakingSession )
				{
					ShowErrorDialog(error, NULL, NULL, this);	//auto removes Join Session Dialog
				}
			}
		}
	}

}

//-------------------------------------------------------------------------
bool CGameLobby::IsBadServer(CrySessionID sessionId)
{
#ifndef _RELEASE
	if (gl_debugLobbyRejoin)
	{
		// Ignore bad servers if we're doing a rejoin test
		return false;
	}
#endif

	if(!gl_ignoreBadServers)
	{
		const int size = m_badServers.size();
		for(int i = 0; i < size; i++)
		{
			//Compare the CrySessionId (not the smart pointers)
			if((*m_badServers[i].get()) == (*sessionId.get()))
			{
				return true;
			}
		}
	}

	return false;
}

//-------------------------------------------------------------------------
// Callback from deleting session
// arg is CGameLobby*
void CGameLobby::MatchmakingSessionDeleteCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* arg)
{
	ENSURE_ON_MAIN_THREAD;

	CGameLobby *pLobby = static_cast<CGameLobby *>(arg);
	pLobby->NetworkCallbackReceived(taskID, error);

	if (error != eCLE_TimeOut)
	{
		pLobby->FinishDelete(error);

		CSquadManager *pSquadManager = g_pGame->GetSquadManager();
		if(pSquadManager)
		{
			pSquadManager->SessionChangeSlotType(CSquadManager::eSST_Public);	// make all slots public again if necessary
		}
	}
}

//-------------------------------------------------------------------------
void CGameLobby::FinishDelete(ECryLobbyError result)
{
	CryLog("CGameLobby::FinishDelete() lobby:%p result from delete = %d", this, (int) result);

	CrySessionID oldSessionID = m_currentSessionId;
	
	SetCurrentSession(CrySessionInvalidHandle);
	m_server = false;
	m_connectedToDedicatedServer = false;
	m_votingEnabled = false;
	m_bWaitingForGameToFinish = false;
	m_isLeaving = false;
	m_bHasUserList = false;
	m_bHasReceivedVoteOptions = false;

	m_reservationList = NULL;
	m_squadReservation = false;
	for (int i = 0; i < MAX_RESERVATIONS; ++ i)
	{
		m_slotReservations[i].m_con = CryMatchMakingInvalidConnectionUID;
		m_slotReservations[i].m_timeStamp = 0.f;
	}

	m_nameList.Clear();
	m_sessionFavouriteKeyId = INVALID_SESSION_FAVOURITE_ID;
	for (int i = 0; i < MAX_PLAYER_GROUPS; ++ i)
	{
		m_playerGroups[i].Reset();
	}
	m_previousGameScores.clear();

	m_shouldFindGames = false;

	if (!m_gameLobbyMgr->IsMultiplayer())
	{
		m_taskQueue.Reset();
	}

	CryLog("Clearing internal m_userData and m_sessionData in CGameLobby::FinishDelete()");
	memset(&m_sessionData, 0, sizeof(m_sessionData));
	memset(&m_userData, 0, sizeof(m_userData));

#ifdef USE_C2_FRONTEND
	ResetFlashInfos();
#endif //#ifdef USE_C2_FRONTEND

	m_startTimerCountdown = false;
	m_initialStartTimerCountdown = false;
	m_bCancelling = false;
	m_bQuitting = false;

	m_findGameNumRetries = 0;
	m_numPlayerJoinResets = 0;

	if (!m_gameLobbyMgr->IsLobbyMerging())
	{
#ifdef GAME_IS_CRYSIS2
		if (CPlaylistManager* pPlaylistManager=g_pGame->GetPlaylistManager())
		{
			if (ILevelRotation* pLevelRotation=pPlaylistManager->GetLevelRotation())
			{
				pLevelRotation->First();  // leave level rotation in a sensible state after leaving in case we decide to head right back into the lobby to start a new session on this same 'list
			}
		}
#endif
	}

	// Remove any tasks that require a valid session handle (since we don't have one any more)
	m_taskQueue.ClearInternalSessionTasks();


	const int numTasks = m_taskQueue.GetCurrentQueueSize();
	if ((numTasks == 0) || ((numTasks == 1) && (m_taskQueue.GetCurrentTask() == CLobbyTaskQueue::eST_Delete)))
	{
#ifndef _RELEASE
		if (gl_debugLobbyRejoin)
		{
			SetState(eLS_FindGame);
		}
		else
#endif
		{
			SetState(eLS_None);

			//We think this means we're actually done and given up on matchmaking
			if( CMatchMakingHandler* pMMhandler = m_gameLobbyMgr->GetMatchMakingHandler() )
			{
				pMMhandler->OnLeaveMatchMaking();
			}

#if defined (TRACK_MATCHMAKING)
			//probably can't actually be merging here
			if( ! m_gameLobbyMgr->IsLobbyMerging() )
			{
				CMatchmakingTelemetry* pMMTel = g_pGame->GetMatchMakingTelemetry();

				if( pMMTel != NULL && pMMTel->GetCurrentTranscriptType() == CMatchmakingTelemetry::eMMTelTranscript_QuickMatch )
				{
					CTelemetryCollector		*pTC=static_cast<CTelemetryCollector*>( g_pGame->GetITelemetryCollector() );
					pTC->SetAbortedSessionId( m_currentSessionName.c_str(), oldSessionID );
	
					pMMTel->AddEvent( SMMLeaveMatchMakingEvent() );
					pMMTel->EndMatchmakingTranscript( CMatchmakingTelemetry::eMMTelTranscript_QuickMatch, true );
				}
			}
#endif

		}
	}

	m_currentSessionName.clear();	// don't need this anymore

	m_hasReceivedMapCommand = false;

	SAFE_DELETE(m_gameStartParams);
}

//static-------------------------------------------------------------------------
void CGameLobby::MatchmakingSessionUpdateCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* arg)
{
	ENSURE_ON_MAIN_THREAD;

	CRY_ASSERT(error == eCLE_Success);

	CGameLobby *pLobby = static_cast<CGameLobby *>(arg);
	if (pLobby->NetworkCallbackReceived(taskID, error))
	{
		pLobby->m_sessionUserDataDirty = true;

		// Tell clients that we've updated the session info
		pLobby->SendPacket(eGUPD_LobbyUpdatedSessionInfo);
	}
}

//-------------------------------------------------------------------------
void CGameLobby::InsertUser(SCryUserInfoResult* user)
{
	TEAM_BALANCING_USERS_LOG("CGameLobby::InsertUser() new player %s, uid=%u, sid=%llX, lobby:%p", user->m_userName, user->m_conID.m_uid, user->m_conID.m_sid, this);

	if (user->m_isDedicated)
	{
		m_connectedToDedicatedServer = true;
		CryLog("  user is dedicated, ignoring");
		return;
	}

#ifdef GAME_IS_CRYSIS2
	CAudioSignalPlayer::JustPlay("LobbyPlayerJoin");
#endif

	if (m_state == eLS_Leaving)
	{
		CryLog("CGameLobby::InsertUser() '%s' joining while we're in leaving state, ignoring, lobby=%p", user->m_userName, this);
		return;
	}

	// We might already have a dummy entry for this player (possible if we receive a SetTeam message before the user joined callback)
	bool foundByUserId = false;
	int playerIndex = m_nameList.Find(user->m_conID);
	
	if(playerIndex == SSessionNames::k_unableToFind)
	{
		CryLog("[GameLobby] Could not find user %s by ConnectionID, checking for CryUserID", user->m_userName);
		
		playerIndex = m_nameList.FindByUserId(user->m_userID);
		if(playerIndex != SSessionNames::k_unableToFind)
		{
			CryLog("[GameLobby] Found %s from CryUserID", user->m_userName);
			foundByUserId = true;
		}
	}

	if (playerIndex == SSessionNames::k_unableToFind)
	{
		if(m_nameList.Size() >= MAX_SESSION_NAMES)
		{
			CryLog("[GameLobby] More players than supported, possibly because of a merge, try to remove an invalid entry");
			bool success = m_nameList.RemoveEntryWithInvalidConnection();
			if(!success)
			{
				CRY_ASSERT_MESSAGE(0, string().Format("No players to remove, we have maxed out the session list with %d players", m_nameList.Size()));
			}
		}

		TEAM_BALANCING_USERS_LOG("    unknown player, adding");
		playerIndex = m_nameList.Insert(user->m_userID, user->m_conID, user->m_userName, user->m_userData, user->m_isDedicated);
	}
	else
	{
		TEAM_BALANCING_USERS_LOG("    already had a dummy entry for this player");
		CRY_ASSERT(!m_server);		// Shouldn't be possible to get here on the server
		m_nameList.Update(user->m_userID, user->m_conID, user->m_userName, user->m_userData, user->m_isDedicated, foundByUserId);
	}

	if (FindGroupByConnectionUID(&user->m_conID))
	{
		CRY_ASSERT_MESSAGE(false, "CGameLobby::InsertUser() Duplicate call, should never happen");
		CryLog("CGameLobby::InsertUser() Duplicate call, should never happen Lobby: %p", this);
		return;
	}

	SSessionNames::SSessionName *pPlayer = &m_nameList.m_sessionNames[playerIndex];
	if (pPlayer)
	{
		UpdatePlayerGroup(pPlayer, 0);
	}

	// Sort out voice
	if (user->m_userID.IsValid())
	{
		SSessionNames::SSessionName *pPlayer2 = m_nameList.GetSessionNameByUserId(user->m_userID);
		if (pPlayer2)
		{
			CryUserID localUser = gEnv->pNetwork->GetLobby()->GetLobbyService()->GetUserID(GameLobby_GetCurrentUserIndex());
			const bool bIsLocalUser = localUser.IsValid() && (localUser == user->m_userID);
			if (bIsLocalUser==false)
			{
				SetAutomaticMutingStateForPlayer(pPlayer2, m_autoVOIPMutingType);
			}

			if (m_state == eLS_Game)
			{
				if (m_isTeamGame || !m_hasValidGameRules)
				{
					// If we're a team game, mute the player till we know what team they're on
					MutePlayerBySessionName(pPlayer2, true, SSessionNames::SSessionName::MUTE_REASON_WRONG_TEAM);
				}
			}
		}
	}

	// Sort out squads
	CSquadManager *pSquadMgr = g_pGame->GetSquadManager();
	if(pSquadMgr)
	{
		if(IsPrivateGame() && IsServer())
		{
			if(pSquadMgr->InSquad() && pSquadMgr->InCharge() && pSquadMgr->HaveSquadMates())
			{
				const SSessionNames *pSessionNames = pSquadMgr->GetSessionNames();
				if(pSessionNames)
				{
					const SSessionNames::SSessionName *pSessionName = pSessionNames->GetSessionNameByUserId(user->m_userID);
					if(pSessionName)
					{
						if(pPlayer)
						{
							CryLog("  setting user %s game lobby user data from squad data as intrim measure", user->m_userName);
							pPlayer->SetUserData(pSessionName->m_userData);
						}
					}
				}
			}
		}

		CryUserID squadLeader = pSquadMgr->GetSquadLeader();
		if ((m_state == eLS_Lobby) && squadLeader.IsValid() && (squadLeader == user->m_userID))
		{
			// User joining is our squad leader, mark ourselves as in a squad
			m_taskQueue.AddTask(CLobbyTaskQueue::eST_SetLocalUserData, true);
		}
	}

	//if anyone joins - don't want to move session
	if(m_gameLobbyMgr->IsPrimarySession(this))
	{
		if (m_server)
		{
			// Reset the "search for better host" timer to give the network a chance to get NAT details
			m_timeTillCallToEnsureBestHost = gl_hostMigrationEnsureBestHostDelayTime;

#if !defined(_RELEASE)
			if (gl_debugForceLobbyMigrations)
			{
				// Make sure the active connections is set to 1
				gEnv->pConsole->GetCVar("net_hostHintingActiveConnectionsOverride")->Set(1);
				CryLog("  setting net_hostHintingActiveConnectionsOverride to 1");
			}
#endif

		}
	}

	CheckForSkillChange();

#if INCLUDE_DEDICATED_LEADERBOARDS
	SSessionNames::SSessionName *pSessionName = m_nameList.GetSessionNameByUserId(user->m_userID);

	if(AllowOnlineAttributeTasks())
	{
		int idx = FindWriteUserDataByUserID(user->m_userID);
		if(idx < 0)
		{
			CryLog("  no write user data found for %s, going to retrieve it", user->m_userName);
			AddOnlineAttributeTask(user->m_userID, eOATT_ReadUserData);
		}
		else
		{
			CryLog("  write user data found for %s, setting to session data and sending", user->m_userName);
			SWriteUserData *pData = &m_writeUserData[idx];

			pSessionName->SetOnlineData(pData->m_data, pData->m_numData);
			pSessionName->m_onlineStatus = eOAS_Initialised;
			SendPacket(eGUPD_StartOfGameUserStats, pSessionName->m_conId);		
		}
	}
	else if(m_server)
	{
		if(pSessionName)
		{
			// this won't work for host migration, but its a bit of a hack
			// to allow server/client functionality to still work
			pSessionName->m_onlineStatus = eOAS_Initialised;
		}
	}
#endif

	// If we're the server then we only need 1 player, clients should have 2
	if (m_server || (m_nameList.Size() > 1))
	{
		m_bHasUserList = true;
	}
	if (m_startTimerCountdown && (m_numPlayerJoinResets < gl_startTimerMaxPlayerJoinResets))
	{
		m_startTimer = max(m_startTimer, gl_startTimerMinTimeAfterPlayerJoined);
		m_numPlayerJoinResets ++;
	}

#if defined(TRACK_MATCHMAKING)
	if( CMatchmakingTelemetry* pMMTel = g_pGame->GetMatchMakingTelemetry() )
	{
		if( pMMTel->GetCurrentTranscriptType() == CMatchmakingTelemetry::eMMTelTranscript_QuickMatch )
		{
			IPlayerProfileManager *pPlayerProfileManager = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager();
			uint32 currentUserIndex = pPlayerProfileManager ? pPlayerProfileManager->GetExclusiveControllerDeviceIndex() : 0;	

			CryUserID localUserId = gEnv->pNetwork->GetLobby()->GetLobbyService()->GetUserID( currentUserIndex );
			bool isLocal = (user->m_userID == localUserId);
			pMMTel->AddEvent( SMMPlayerJoinedMMEvent( user, m_currentSessionId, m_nameList.Size(), isLocal ) );
		}
	}
#endif
}

//-------------------------------------------------------------------------
void CGameLobby::UpdateUser( SCryUserInfoResult* user )
{
	CryLog("CGameLobby::UpdateUser() player '%s' uid=%u has updated their user data Lobby: %p", user->m_userName, user->m_conID.m_uid, this);

	if (user->m_isDedicated)
	{
		CryLog("  user is dedicated, ignoring");
		return;
	}

	SSessionNames::SSessionName *pPlayer = m_nameList.GetSessionName(user->m_conID, true);
	if(pPlayer)
	{
		const uint16 previousSkill = pPlayer->GetSkillRank();
		if (pPlayer->m_conId != user->m_conID)
		{
			TEAM_BALANCING_USERS_LOG("CGameLobby::UpdateUser() player '%s' has changed UID", pPlayer->m_name);
			// Player connection UID has changed (probably due to a LIVE migration), need to update the group members
			SPlayerGroup *pPlayerGroup = FindGroupByConnectionUID(&pPlayer->m_conId);
			if (pPlayerGroup)
			{
				const int numMembers = pPlayerGroup->m_members.size();
				for (int i = 0; i < numMembers; ++ i)
				{
					if (pPlayerGroup->m_members[i] == pPlayer->m_conId)
					{
						TEAM_BALANCING_USERS_LOG("    updating group");
						pPlayerGroup->m_members[i] = user->m_conID;
						break;
					}
				}
			}
		}

		m_nameList.Update(user->m_userID, user->m_conID, user->m_userName, user->m_userData, user->m_isDedicated, false);

		UpdatePlayerGroup(pPlayer, previousSkill);

		CheckForVotingChanges(true);

		CheckForSkillChange();
	}
}

//-------------------------------------------------------------------------
void CGameLobby::UpdatePlayerGroup( SSessionNames::SSessionName *pPlayer, uint16 previousSkill)
{
	CRY_ASSERT(pPlayer);
	if (pPlayer)
	{
		bool bNeedsTeamBalancing = false;

		// Determine what sort of group the player should be in
		EPlayerGroupType requestedGroupType = ePGT_Unknown;

		uint32 squadLeaderUID =  pPlayer->m_userData[eLUD_SquadId1] + 
														(pPlayer->m_userData[eLUD_SquadId2] << 8);
		CryFixedStringT<CLAN_TAG_LENGTH> clanTag;
		if (squadLeaderUID != 0)
		{
			requestedGroupType = ePGT_Squad;
		}
		else
		{
			bool bUseClans = true;

#ifdef GAME_IS_CRYSIS2
			CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
			int variantIdx = pPlaylistManager ?	pPlaylistManager->GetActiveVariantIndex() : -1;
			if (variantIdx >= 0)
			{
				bUseClans = pPlaylistManager->GetVariants()[variantIdx].m_allowClans;
			}
#endif

			if (bUseClans)
			{
				pPlayer->GetClanTagName(clanTag);
			}

			if (clanTag.empty())
			{
				requestedGroupType = ePGT_Individual;
			}
			else
			{
				requestedGroupType = ePGT_Clan;
			}
		}

		uint16 currentSkill = pPlayer->GetSkillRank();

		// Now check what group we're in at the moment to see if we need to do anything
		bool bNeedToChangeGroup = true;
		SPlayerGroup *pCurrentGroup = FindGroupByConnectionUID(&pPlayer->m_conId);
		if (pCurrentGroup)
		{
			// Currently in a group, is it the correct group?
			if (pCurrentGroup->m_type == requestedGroupType)
			{
				switch (requestedGroupType)
				{
				case ePGT_Individual:
					bNeedToChangeGroup = false;
					break;
				case ePGT_Squad:
					bNeedToChangeGroup = (squadLeaderUID != pCurrentGroup->m_pSquadLeader.m_uid);
					break;
				case ePGT_Clan:
					bNeedToChangeGroup = (stricmp(pCurrentGroup->m_clanTag.c_str(), clanTag.c_str()) != 0);
					break;
				}
			}
			// If we need to change group, remove us from our previous group, otherwise make sure the skill rank is correct
			if (bNeedToChangeGroup)
			{
				pCurrentGroup->RemoveMember(&pPlayer->m_conId, previousSkill);
			}
			else if (currentSkill != previousSkill)
			{
				pCurrentGroup->m_totalSkill = (pCurrentGroup->m_totalSkill - previousSkill) + currentSkill;
				bNeedsTeamBalancing = true;
			}
		}

		// Check if we need to change group
		if (bNeedToChangeGroup)
		{
			switch (requestedGroupType)
			{
			case ePGT_Individual:
				{
					SPlayerGroup *pPlayerGroup = FindEmptyGroup();
					CRY_ASSERT(pPlayerGroup);
					if (pPlayerGroup)
					{
						pPlayerGroup->InitIndividual(&pPlayer->m_conId, this);
					}
				}
				break;
			case ePGT_Squad:
				{
					// Player is entering a squad or moving to a new squad
					SCryMatchMakingConnectionUID tempUID;
					tempUID.m_uid = squadLeaderUID;
					SSessionNames::SSessionName *pSquadLeader = m_nameList.GetSessionName(tempUID, true);		// Ignore SID since we don't know it
					if (pSquadLeader)
					{
						TEAM_BALANCING_SQUADS_LOG("CGameLobby::UpdatePlayerGroup() playerUID %i requests to have playerUID %i as squad leader", pPlayer->m_conId.m_uid, squadLeaderUID);
						SPlayerGroup *pSquadLeaderGroup = FindGroupByConnectionUID(&pSquadLeader->m_conId);
						if (pSquadLeaderGroup)
						{
							if (pSquadLeaderGroup->m_type == ePGT_Individual)
							{
								TEAM_BALANCING_SQUADS_LOG("    host in own group, converting to squad group");
								pSquadLeaderGroup->InitSquad(&pSquadLeader->m_conId);
							}
							else if (pSquadLeaderGroup->m_type == ePGT_Clan)
							{
								TEAM_BALANCING_SQUADS_LOG("    host in clan group, creating squad group and moving host into it");
								// Need to leave clan group and form a squad group
								pSquadLeaderGroup->RemoveMember(&pSquadLeader->m_conId, this);
								pSquadLeaderGroup = NULL;
							}
						}
						if (!pSquadLeaderGroup)
						{
							// Squad leader wasn't in a group or is no longer in a group, create a new squad group
							pSquadLeaderGroup = FindEmptyGroup();
							CRY_ASSERT(pSquadLeaderGroup);
							if (pSquadLeaderGroup)
							{
								pSquadLeaderGroup->InitSquad(&pSquadLeader->m_conId);
								pSquadLeaderGroup->AddMember(&pSquadLeader->m_conId, this);
							}
						}
						if (pSquadLeaderGroup  && (pPlayer != pSquadLeader))
						{
							pSquadLeaderGroup->AddMember(&pPlayer->m_conId, this);
						}
						TEAM_BALANCING_SQUADS_LOG("    adding player to squad, newMemberCount=%i", pSquadLeaderGroup ? pSquadLeaderGroup->m_numMembers : -1);
					}
				}
				break;
			case ePGT_Clan:
				{
					TEAM_BALANCING_SQUADS_LOG("    player is in clan '%s'", clanTag.c_str());
					SPlayerGroup *pClanGroup = FindGroupByClan(clanTag.c_str());
					if (!pClanGroup)
					{
						TEAM_BALANCING_SQUADS_LOG("        unknown clan, creating new group");
						pClanGroup = FindEmptyGroup();
						CRY_ASSERT(pClanGroup);
						if (pClanGroup)
						{
							pClanGroup->InitClan(clanTag.c_str());
						}
					}
					if (pClanGroup)
					{
						pClanGroup->AddMember(&pPlayer->m_conId, this);
						TEAM_BALANCING_SQUADS_LOG("        added player to clan, newMemberCount=%i", pClanGroup->m_numMembers);
					}
				}
				break;
			}
			bNeedsTeamBalancing = true;
		}

		// If anything has changed (group or skill) then we need to re-balance the teams
		if (bNeedsTeamBalancing)
		{
			m_needsTeamBalancing = true;
		}
	}
}

//-------------------------------------------------------------------------
void CGameLobby::RemoveUser(SCryUserInfoResult* user)
{
	TEAM_BALANCING_USERS_LOG("CGameLobby::RemoveUser() user=%s, uid=%u, sid=%llX allowRemove %d lobby:%p", user->m_userName, user->m_conID.m_uid, user->m_conID.m_sid, m_allowRemoveUsers, this);

	if(m_allowRemoveUsers)
	{
#ifdef GAME_IS_CRYSIS2
		CAudioSignalPlayer::JustPlay("LobbyPlayerLeave");
#endif

		// Remove from group
		SPlayerGroup *pPlayerGroup = FindGroupByConnectionUID(&user->m_conID);
		if (pPlayerGroup)
		{
			pPlayerGroup->RemoveMember(&user->m_conID, this);
			if (m_isTeamGame)
			{
				m_needsTeamBalancing = true;
			}
		}

#if INCLUDE_DEDICATED_LEADERBOARDS
		SSessionNames::SSessionName *pSessionName = m_nameList.GetSessionNameByUserId(user->m_userID);
		if (pSessionName)
		{
			// need to do this before the user is removed from the list
			// otherwise it will fail
			RemoveOnlineAttributeTask(user->m_userID);
			
			if(AllowOnlineAttributeTasks() && !m_gameLobbyMgr->IsLobbyMerging()) // unlikely to end well if we are
			{
				if(pSessionName->m_onlineStatus == eOAS_UpdateComplete)
				{
					int idx = FindWriteUserDataByUserID(pSessionName->m_userId);
					if(idx < 0)
					{
						CryLog("  %s not found in write user data, adding to queue", pSessionName->m_name);
						m_writeUserData.push_back(SWriteUserData(pSessionName->m_userId, pSessionName->m_onlineData, pSessionName->m_onlineDataCount));
					}
					else
					{
						CryLog("  found %s in write user data, updating", pSessionName->m_name);
						SWriteUserData *pData = &m_writeUserData[idx];
						pData->SetData(pSessionName->m_onlineData, pSessionName->m_onlineDataCount);
						pData->m_requeue = pData->m_writing;	// if we're already writing, then we need to requeue on finish so we update correctly
					}
				}
				else
				{
					CryLog("  incomplete user data for %s, not adding to queue for writing", pSessionName->m_name);
				}
			}
		}
#endif

		m_nameList.Remove(user->m_conID);

		CheckForVotingChanges(true);

		// If we've currently got a pending call to SessionEnsureBestHost, delay it to let the host hints settle
		if (m_timeTillCallToEnsureBestHost > 0.f)
		{
			m_timeTillCallToEnsureBestHost = gl_hostMigrationEnsureBestHostDelayTime;
		}

		CheckForSkillChange();
	}
	else
	{
		SSessionNames::SSessionName *pSessionName = m_nameList.GetSessionNameByUserId(user->m_userID);
		if (pSessionName)
		{
			pSessionName->m_conId = CryMatchMakingInvalidConnectionUID;
		}
	}

	if (m_isLeaving)
	{
		CheckCanLeave();
	}
	else if (m_state == eLS_Game)
	{
		CGameRules *pGameRules = g_pGame->GetGameRules();
		if (pGameRules)
		{
			int channelId = (int) user->m_conID.m_uid;
			pGameRules->OnUserLeftLobby(channelId);
		}
	}

#if defined (TRACK_MATCHMAKING)
	if( CMatchmakingTelemetry* pMMTel = g_pGame->GetMatchMakingTelemetry() )
	{
		if( pMMTel->GetCurrentTranscriptType() == CMatchmakingTelemetry::eMMTelTranscript_QuickMatch )
		{
			IPlayerProfileManager *pPlayerProfileManager = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager();
			uint32 currentUserIndex = pPlayerProfileManager ? pPlayerProfileManager->GetExclusiveControllerDeviceIndex() : 0;	

			CryUserID localUserId = gEnv->pNetwork->GetLobby()->GetLobbyService()->GetUserID( currentUserIndex );
			bool isLocal = (user->m_userID == localUserId);
			pMMTel->AddEvent( SMMPlayerLeftMMEvent( user, m_currentSessionId, m_nameList.Size(), isLocal ) );
		}
	}
#endif //defined (TRACK_MATCHMAKING)
}

//-------------------------------------------------------------------------
void CGameLobby::CheckCanLeave()
{
	CryLog("CGameLobby::CheckCanLeave() lobby:%p", this);

	int numPlayersThatNeedToLeaveFirst = 0;
	// Check if we're able to leave yet
	const int numRemainingPlayers = m_nameList.Size();
	// Start from 1 since we don't include ourselves
	for (int i = 1; i < numRemainingPlayers; ++ i)
	{
		SSessionNames::SSessionName &player = m_nameList.m_sessionNames[i];
		if (player.m_bMustLeaveBeforeServer && (player.m_conId != CryMatchMakingInvalidConnectionUID))
		{
			CryLog("    waiting for player %s to leave first", player.m_name);
			++ numPlayersThatNeedToLeaveFirst;
		}
	}
	if (!numPlayersThatNeedToLeaveFirst)
	{
		CryLog("    don't need to wait anymore, leaving next frame");
		m_leaveGameTimeout = 0.f;
	}
}

//static-------------------------------------------------------------------------
void CGameLobby::MatchmakingSessionQueryCallback(CryLobbyTaskID taskID, ECryLobbyError error, SCrySessionSearchResult* session, void* arg)
{
	ENSURE_ON_MAIN_THREAD;

	CGameLobby *pLobby = static_cast<CGameLobby *>(arg);
	CryLog("CGameLobby::MatchmakingSessionQueryCallback() Lobby: %p", pLobby);

	if (pLobby->NetworkCallbackReceived(taskID, error))
	{
#if USE_CRYLOBBY_GAMESPY
		if (session)
		{
			int numData = session->m_data.m_numData;
			for (int i=0; i<numData; ++i)
			{
				SCryLobbyUserData& pUserData = session->m_data.m_data[i];
				if ((pUserData.m_id == LID_MATCHDATA_FAVOURITE_ID) && (eCLUDT_Int32 == pUserData.m_type))
				{
					pLobby->m_sessionFavouriteKeyId = pUserData.m_int32;

					pLobby->CheckGetServerImage();                                           
				}
			}
		}
		if (pLobby->m_server==false)
#endif
		{
			pLobby->RecordReceiptOfSessionQueryCallback();

			if (!pLobby->m_bMatchmakingSession)
			{
#ifdef USE_C2_FRONTEND
				CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
				if(menu)
				{
					CMPMenuHub *mpMenuHub = menu->GetMPMenu();
					if (mpMenuHub)
					{
						mpMenuHub->HideLoadingDialog("JoinSession");
					}
				}
#endif //#ifdef USE_C2_FRONTEND
			}

			if(session)
			{
				memcpy(&pLobby->m_sessionData, &session->m_data, sizeof(pLobby->m_sessionData));
				memcpy(&pLobby->m_userData, session->m_data.m_data, sizeof(pLobby->m_userData));

				CryLog("CGameLobby::MatchmakingSessionQueryCallback() got user data Lobby: %p", pLobby);
				for (int i = 0; i < eLDI_Num; ++ i)
				{
					SCryLobbyUserData &userData = pLobby->m_userData[i];
					CryLog("  i=%i, id=%i, data=%i Lobby: %p", i, userData.m_id, userData.m_int32, pLobby);
				}

				pLobby->m_sessionData.m_data = pLobby->m_userData;
				pLobby->m_sessionUserDataDirty = true;

				const bool bPreviousAllowLoadout = pLobby->AllowCustomiseEquipment();

				const char *pGameRules = GameLobbyData::GetGameRulesFromHash(pLobby->m_userData[eLDI_Gamemode].m_int32);
				pLobby->GameRulesChanged(pGameRules);

				const bool bNewAllowLoadout = pLobby->AllowCustomiseEquipment();

				pLobby->m_currentLevelName = GameLobbyData::GetMapFromHash(pLobby->m_userData[eLDI_Map].m_int32, "");

				CSquadManager *pSquadManager = g_pGame->GetSquadManager();
				if(pSquadManager)
				{
					pSquadManager->SessionChangeSlotType(pLobby->IsPrivateGame() ? CSquadManager::eSST_Private : CSquadManager::eSST_Public);
				}

				if((!pLobby->m_bMatchmakingSession) && (bPreviousAllowLoadout != bNewAllowLoadout))
				{
					pLobby->RefreshCustomiseEquipment();
				}

#if IMPLEMENT_PC_BLADES
				CGameServerLists *pGameServerList = g_pGame->GetGameServerLists();
				if(pGameServerList)
				{
					pGameServerList->Add(CGameServerLists::eGSL_Recent, pLobby->m_sessionData.m_name, pLobby->m_sessionFavouriteKeyId, false);
				}
#endif
			}
		}
	}
}

//static-------------------------------------------------------------------------
void CGameLobby::MatchmakingSessionUserPacketCallback(UCryLobbyEventData eventData, void *arg)
{
	ENSURE_ON_MAIN_THREAD;

	if(eventData.pUserPacketData)
	{
		CGameLobby *lobby = static_cast<CGameLobby *>(arg);
	
		if(lobby->m_currentSession == eventData.pUserPacketData->session && eventData.pUserPacketData->session != CrySessionInvalidHandle)
		{
			lobby->ReadPacket(&eventData.pUserPacketData);
		}
	}
}

static int GetCryLobbyUserTypeSize(ECryLobbyUserDataType type)
{
	switch(type)
	{
		case eCLUDT_Int8:
			return CryLobbyPacketUINT8Size;			
		case eCLUDT_Int16:
			return CryLobbyPacketUINT16Size;
		case eCLUDT_Int32:
		case eCLUDT_Float32:
			return CryLobbyPacketUINT32Size;
		default:
			CRY_ASSERT_MESSAGE(0, string().Format("Unknown data type %d", type));
			break;
	}

	return 0;
}

//static-------------------------------------------------------------------------
void CGameLobby::WriteOnlineAttributeData(CGameLobby *pLobby, CCryLobbyPacket *pPacket, GameUserPacketDefinitions packetType, SCryLobbyUserData *pData, int32 numData, SCryMatchMakingConnectionUID connnectionUID)
{
	CRY_ASSERT(pData);
	CRY_ASSERT(numData >= 0);


	int numSent = 0;
	int i = 0;

	while(numSent < numData)
	{
		int bufferSz = CryLobbyPacketHeaderSize;
		for(i = numSent; i < numData; ++i)
		{
			if(bufferSz >= ONLINE_STATS_FRAGMENTED_PACKET_SIZE)
			{
				break;
			}

			bufferSz += GetCryLobbyUserTypeSize(pData[i].m_type);
		}

		// CreateWriteBuffer will free and recreate each time
		if(pPacket->CreateWriteBuffer(bufferSz))
		{
			pPacket->StartWrite(packetType, true);
			for(; numSent < i; ++numSent)
			{
				pPacket->WriteCryLobbyUserData(&pData[numSent]);
			}

			pLobby->SendPacket(pPacket, packetType, connnectionUID);
		}
	}
}

//---------------------------------------
bool CGameLobby::OnLeaveOrQuit()
{
	bool done = false;

#if INCLUDE_DEDICATED_LEADERBOARDS
	CryLog("CGameLobby::OnLeaveOrQuit");

	if(m_currentSession != CrySessionInvalidHandle)
	{
		SendPacket(eGUPD_UpdateUserStats, CryMatchMakingInvalidConnectionUID);
		done = true;
	}
#endif
	return done;
}

//---------------------------------------
void CGameLobby::SendPacket(GameUserPacketDefinitions packetType, SCryMatchMakingConnectionUID connectionUID /*=CryMatchMakingInvalidConnectionUID*/)
{
#if DEBUG_TEAM_BALANCING
	if (s_testing)	// Used by the test function to make sure we don't try to send packets when we're not in a session!
	{
		return;
	}
#endif

	CCryLobbyPacket packet;

	switch(packetType)
	{
	case eGUPD_LobbyStartCountdownTimer:
		{
			CryLog("[tlh]   sending packet of type 'eGUPD_LobbyStartCountdownTimer'");
			CRY_ASSERT(m_server);

			VOTING_DBG_LOG("[tlh]     whilst vote status = %d", m_localVoteStatus);

			const uint32 MaxBufferSize = CryLobbyPacketHeaderSize +
				(CryLobbyPacketBoolSize * 5) +
				(CryLobbyPacketUINT8Size);
			if (packet.CreateWriteBuffer(MaxBufferSize))
			{
				packet.StartWrite(packetType, true);
				packet.WriteBool(m_startTimerCountdown);
				packet.WriteBool(m_initialStartTimerCountdown);
				packet.WriteUINT8((uint8) m_startTimer);
				packet.WriteBool(m_votingEnabled);
				packet.WriteBool(m_votingClosed);
				packet.WriteBool(m_leftWinsVotes);

				VOTING_DBG_LOG("[tlh]     SENT this data:");
				VOTING_DBG_LOG("[tlh]       m_startTimerCountdown = %d", m_startTimerCountdown);
				VOTING_DBG_LOG("[tlh]       m_startTimer = %d", (uint8) m_startTimer);
				VOTING_DBG_LOG("[tlh]       data->Get()->m_votingEnabled = %d", m_votingEnabled);
				VOTING_DBG_LOG("[tlh]       data->Get()->m_votingClosed = %d", m_votingClosed);
				VOTING_DBG_LOG("[tlh]       data->Get()->m_leftWinsVotes = %d", m_leftWinsVotes);
			}

			if (m_votingEnabled)
			{
#ifdef USE_C2_FRONTEND
				UpdateVotingInfoFlashInfo();
#endif //#ifdef USE_C2_FRONTEND
			}
		}
		break;
	case eGUPD_LobbyGameHasStarted:
		{
			CRY_ASSERT(m_server);
			const uint32 MaxBufferSize = CryLobbyPacketHeaderSize + (3 * CryLobbyPacketUINT32Size) + (CryLobbyPacketBoolSize * 2);
			if (packet.CreateWriteBuffer(MaxBufferSize))
			{
				packet.StartWrite(packetType, true);
#ifdef GAME_IS_CRYSIS2
				CDLCManager* pDLCManager = g_pGame->GetDLCManager();
				packet.WriteUINT32(pDLCManager->GetRequiredDLCs());
#else
				packet.WriteUINT32(0);
#endif

				bool gameStillInProgress = true;

#ifdef GAME_IS_CRYSIS2
				CGameRules *pGameRules = g_pGame->GetGameRules();
				if (pGameRules)
				{
					IGameRulesStateModule *pStateModule = pGameRules->GetStateModule();
					if (pStateModule)
					{
						IGameRulesStateModule::EGR_GameState gameState = pStateModule->GetGameState();
						if (gameState == IGameRulesStateModule::EGRS_PostGame)
						{
							gameStillInProgress = false;
						}
						else if (gameState == IGameRulesStateModule::EGRS_InGame)
						{
							if (pGameRules->IsTimeLimited() && (pGameRules->GetRemainingGameTime() < gl_gameTimeRemainingRestrictLoad))
							{
								gameStillInProgress = false;

								IGameRulesRoundsModule *pRoundsModule = pGameRules->GetRoundsModule();
								if (pRoundsModule)
								{
									if (pRoundsModule->GetRoundsRemaining() != 0)
									{
										gameStillInProgress = true;
									}
								}
							}
						}
					}
				}
#endif

				packet.WriteBool(gameStillInProgress);
				packet.WriteBool(m_votingEnabled);

				const char *pCurrentGameRules = m_gameStartParams ? m_gameStartParams->gameModeName.c_str() : m_currentGameRules.c_str();
				const char *pCurrentLevelName = m_gameStartParams ? m_gameStartParams->levelName.c_str() : m_currentLevelName.c_str();

				uint32 gameRulesHash = GameLobbyData::ConvertGameRulesToHash(pCurrentGameRules);
				uint32 mapHash = GameLobbyData::ConvertMapToHash(pCurrentLevelName);

				packet.WriteUINT32(gameRulesHash);
				packet.WriteUINT32(mapHash);
			}
		}
		break;
	case eGUPD_LobbyEndGame:
		{
			CRY_ASSERT(m_server);
			const uint32 MaxBufferSize = CryLobbyPacketHeaderSize;
			if (packet.CreateWriteBuffer(MaxBufferSize))
			{
				packet.StartWrite(packetType, true);
			}
		}
		break;
	case eGUPD_LobbyEndGameResponse:
		{
			CRY_ASSERT(!m_server);
			const uint32 MaxBufferSize = CryLobbyPacketHeaderSize;
			if (packet.CreateWriteBuffer(MaxBufferSize))
			{
				packet.StartWrite(packetType, true);
			}			
		}
		break;
	case eGUPD_LobbyUpdatedSessionInfo:
		{
			CRY_ASSERT(m_server);
			const uint32 MaxBufferSize = CryLobbyPacketHeaderSize;
			if (packet.CreateWriteBuffer(MaxBufferSize))
			{
				packet.StartWrite(packetType, true);
			}
		}
		break;
	case eGUPD_LobbyMoveSession:
		{
			CRY_ASSERT(m_server);
			CRY_ASSERT(m_nextSessionId != CrySessionInvalidID);
			ICryMatchMaking *pMatchmaking = gEnv->pNetwork->GetLobby()->GetLobbyService()->GetMatchMaking();
			const uint32 MaxBufferSize = CryLobbyPacketHeaderSize + pMatchmaking->GetSessionIDSizeInPacket();
			if (packet.CreateWriteBuffer(MaxBufferSize))
			{
				packet.StartWrite(packetType, true);
				ECryLobbyError error = pMatchmaking->WriteSessionIDToPacket(m_nextSessionId, &packet);
				CRY_ASSERT(error == eCLE_Success);
			}
		}
		break;
	case eGUPD_SetTeam:
		{
			CRY_ASSERT(m_server);

			const uint32 numPlayers = uint32(m_nameList.Size());

			const uint32 MaxBufferSize = CryLobbyPacketHeaderSize + CryLobbyPacketUINT8Size + (numPlayers * (CryLobbyPacketUINT8Size + CryLobbyPacketConnectionUIDSize));
			if (packet.CreateWriteBuffer(MaxBufferSize))
			{
				packet.StartWrite(packetType, true);
				packet.WriteUINT8(uint8(numPlayers));
				for (uint32 i = 0; i < numPlayers; ++ i)
				{
					SSessionNames::SSessionName &player = m_nameList.m_sessionNames[i];
					packet.WriteConnectionUID(player.m_conId);
					packet.WriteUINT8(player.m_teamId);
				}
			}
			break;
		}
	case eGUPD_SendChatMessage:
		{
			CRY_ASSERT(!m_server);

#ifndef _RELEASE
			CryLog("CLIENT Send: eGUPD_SendChatMessage channel:%d team:%d, message:%s", m_chatMessageStore.conId.m_uid, m_chatMessageStore.teamId, m_chatMessageStore.message.c_str());
#endif
			
			uint8 messageLength = m_chatMessageStore.message.size();
			if (messageLength>0 && messageLength<MAX_CHATMESSAGE_LENGTH)
			{
				const uint32 MaxBufferSize = CryLobbyPacketHeaderSize + CryLobbyPacketUINT8Size + CryLobbyPacketUINT8Size + messageLength;
				if (packet.CreateWriteBuffer(MaxBufferSize))
				{
					packet.StartWrite(packetType, true);
					packet.WriteUINT8(m_chatMessageStore.teamId);
					packet.WriteUINT8(messageLength + 1);  // +1 is null teminator
					packet.WriteString(m_chatMessageStore.message.c_str(), messageLength + 1);  // +1 is null teminator
				}
			}
			else
			{
				CryLog("Error, Cannot send empty chat message");
			}

			m_chatMessageStore.Clear();

			break;
		}
	case eGUPD_ChatMessage:
		{
			CRY_ASSERT(m_server);

#ifndef _RELEASE
			CryLog("SERVER SEND to %d: eGUPD_ChatMessage channel:%d team:%d, message:%s", connectionUID.m_uid, m_chatMessageStore.conId.m_uid, m_chatMessageStore.teamId, m_chatMessageStore.message.c_str());
#endif

			uint8 messageLength = m_chatMessageStore.message.size();
			if (messageLength>0 && messageLength<MAX_CHATMESSAGE_LENGTH)
			{
				const uint32 MaxBufferSize = CryLobbyPacketHeaderSize + CryLobbyPacketConnectionUIDSize + CryLobbyPacketUINT8Size + CryLobbyPacketUINT8Size + messageLength;
				if (packet.CreateWriteBuffer(MaxBufferSize))
				{
					packet.StartWrite(packetType, true);
					packet.WriteConnectionUID(m_chatMessageStore.conId);
					packet.WriteUINT8(m_chatMessageStore.teamId);
					packet.WriteUINT8(messageLength + 1);  // +1 is null teminator
					packet.WriteString(m_chatMessageStore.message.c_str(), messageLength + 1);  // +1 is null teminator
				}
			}
			else
			{
				CryLog("Error, Cannot send empty chat message");
			}

			break;
		}
	case eGUPD_ReservationRequest:
		{
			CryLog("  sending packet of type 'eGUPD_ReservationRequest'");

			CRY_ASSERT(!m_server);

			SCryMatchMakingConnectionUID  reservationRequests[MAX_RESERVATIONS];
			const int  numReservationsToRequest = BuildReservationsRequestList(reservationRequests, ARRAY_COUNT(reservationRequests), m_reservationList);

			const SSessionNames*  members = m_reservationList;

			const int  bufferSz = (CryLobbyPacketHeaderSize + CryLobbyPacketUINT8Size + (numReservationsToRequest * CryLobbyPacketConnectionUIDSize));

			if (packet.CreateWriteBuffer(bufferSz))
			{

				packet.StartWrite(packetType, true);

				CryLog("    numReservationsToRequest = %d", numReservationsToRequest);
				packet.WriteUINT8(numReservationsToRequest);

				CryLog("    reservations needed:");
				for (int i=0; i<numReservationsToRequest; i++)
				{
					CryLog("      %02d: {%llu,%d}", (i + 1), reservationRequests[i].m_sid, reservationRequests[i].m_uid);
					packet.WriteConnectionUID(reservationRequests[i]);
				}
			}

			break;
		}
	case eGUPD_ReservationClientIdentify:
		{
			CryLog("  sending packet of type 'eGUPD_ReservationClientIdentify', slot reserved as uid=%u, sid=%llu", m_pendingReservationId.m_uid, m_pendingReservationId.m_sid);
			CRY_ASSERT(!m_server);
	
			ICryMatchMaking *pMatchmaking = gEnv->pNetwork->GetLobby()->GetLobbyService()->GetMatchMaking();
			CRY_ASSERT(pMatchmaking);
			
			const int  bufferSz = CryLobbyPacketHeaderSize + CryLobbyPacketConnectionUIDSize + CryLobbyPacketBoolSize + pMatchmaking->GetSessionIDSizeInPacket();

			if (packet.CreateWriteBuffer(bufferSz))
			{
				packet.StartWrite(packetType, true);

				packet.WriteConnectionUID(m_pendingReservationId);
				m_pendingReservationId = CryMatchMakingInvalidConnectionUID;

#ifdef GAME_IS_CRYSIS2
				CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
				const bool bHavePlaylist = pPlaylistManager ? pPlaylistManager->HavePlaylistSet() : false;
#else
				const bool bHavePlaylist = false;
#endif
				packet.WriteBool(bHavePlaylist);

				CrySessionID sessionIdToSend = CrySessionInvalidID;

				if (m_gameLobbyMgr->IsNewSession(this))
				{
					CGameLobby *pPrimaryGameLobby = m_gameLobbyMgr->GetGameLobby();
					sessionIdToSend = pPrimaryGameLobby->m_currentSessionId;
					if (!sessionIdToSend)
					{
						sessionIdToSend = pPrimaryGameLobby->m_pendingConnectSessionId;
					}
				}

				pMatchmaking->WriteSessionIDToPacket(sessionIdToSend, &packet);
			}

			break;
		}
	case eGUPD_ReservationsMade:
		{
			CryLog("  sending packet of type 'eGUPD_ReservationsMade'");
			CRY_ASSERT(m_server);
			
			const int  bufferSz = (CryLobbyPacketHeaderSize);

			if (packet.CreateWriteBuffer(bufferSz))
			{
				packet.StartWrite(packetType, true);
			}

			break;
		}
	case eGUPD_ReservationFailedSessionFull:
		{
			CryLog("  sending packet of type 'eGUPD_ReservationFailedSessionFull'");
			CRY_ASSERT(m_server);

			const int  bufferSz = (CryLobbyPacketHeaderSize);

			if (packet.CreateWriteBuffer(bufferSz))
			{
				packet.StartWrite(packetType, true);
			}

			break;
		}
	case eGUPD_SetGameVariant:
		{
			CRY_ASSERT(m_server);

#ifdef GAME_IS_CRYSIS2
			CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
			if (pPlaylistManager)
			{
				pPlaylistManager->WriteSetVariantPacket(&packet);
			}
#endif
			break;
		}
	case eGUPD_SyncPlaylistRotation:
		{
			CryLog("[tlh]   sending packet of type 'eGUPD_SyncPlaylistRotation'");
			CRY_ASSERT(m_server);

			int  curNextIdx = 0;

#ifdef GAME_IS_CRYSIS2
			if (CPlaylistManager* pPlaylistManager=g_pGame->GetPlaylistManager())
			{
				CRY_ASSERT(pPlaylistManager->HavePlaylistSet());

				if (ILevelRotation* pRot=pPlaylistManager->GetLevelRotation())
				{
					curNextIdx = pRot->GetNext();
				}
			}
#endif

			const bool  gameHasStarted = (m_state == eLS_Game);

			const uint32  bufferSize = (CryLobbyPacketHeaderSize + CryLobbyPacketUINT32Size + CryLobbyPacketUINT8Size + (2 * CryLobbyPacketBoolSize));

			if (packet.CreateWriteBuffer(bufferSize))
			{
				packet.StartWrite(eGUPD_SyncPlaylistRotation, true);
				CryLog("[tlh]     writing next=%d, started=%d, advancedThruConsole=%d seed=%d", curNextIdx, gameHasStarted, m_bPlaylistHasBeenAdvancedThroughConsole, m_playListSeed);
				packet.WriteUINT32(m_playListSeed);
				packet.WriteUINT8((uint8) curNextIdx);
				packet.WriteBool(gameHasStarted);
				packet.WriteBool(m_bPlaylistHasBeenAdvancedThroughConsole);
			}

			break;
		}
	case eGUPD_UpdateCountdownTimer:
		{
			const int bufferSz = (CryLobbyPacketHeaderSize + CryLobbyPacketUINT8Size);
			if (packet.CreateWriteBuffer(bufferSz))
			{
				packet.StartWrite(packetType, true);
				uint8 startTimer = (uint8) MAX(m_startTimer, 0.f);
				packet.WriteUINT8(startTimer);
			}
			break;
		}
	case eGUPD_RequestAdvancePlaylist:
		{
			const int bufferSz = (CryLobbyPacketHeaderSize);
			if (packet.CreateWriteBuffer(bufferSz))
			{
				packet.StartWrite(packetType, true);
			}
			break;
		}
#if INCLUDE_DEDICATED_LEADERBOARDS
	case eGUPD_StartOfGameUserStats:
		{
			int userIndex = m_nameList.Find(connectionUID);

			if(userIndex != SSessionNames::k_unableToFind)
			{
				CryLog("[GameLobby] sending eGUPD_StartOfGameUserStats");

				SSessionNames::SSessionName *pSessionName = &m_nameList.m_sessionNames[userIndex];
				
				CryLog("[GameLobby] writing eGUPD_StartOfGameUserStats with %d pieces of data", pSessionName->m_onlineDataCount);

				WriteOnlineAttributeData(this, &packet, packetType, pSessionName->m_onlineData, pSessionName->m_onlineDataCount, connectionUID);	
			}
			else
			{
				CryLog("[GameLobby] failed to find user to send user stats to");
			}

			break;
		}

	case eGUPD_EndOfGameUserStats:
		{
			CRY_ASSERT(!gEnv->IsDedicated());	// local player to dedicated server, in trouble if dedicated
			CryLog("[GameLobby] sending eGUPD_EndOfGameUserStats");
			
			if(!gEnv->bServer) // if the server trys to send this, it does not end well
			{
				SCryLobbyUserData onlineData[MAX_ONLINE_STATS_SIZE];
				IPlayerProfileManager *pPlayerProfileManager = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager();
				int32 numData = pPlayerProfileManager->GetOnlineAttributes(onlineData, MAX_ONLINE_STATS_SIZE);

				WriteOnlineAttributeData(this, &packet, packetType, onlineData, numData, CryMatchMakingInvalidConnectionUID);	// this should go to the server
			}
			break;	
		}

	case eGUPD_UpdateUserStats:
		{
			CRY_ASSERT(!gEnv->IsDedicated());	// local player to dedicated server, in trouble if dedicated
			CryLog("[GameLobby] sending eGUPD_UpdateUserStats");
			
			if(!gEnv->bServer) // if the server trys to send this, it does not end well
			{
				SCryLobbyUserData onlineData[MAX_ONLINE_STATS_SIZE];
				IPlayerProfileManager *pPlayerProfileManager = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager();
				int32 numData = pPlayerProfileManager->GetOnlineAttributes(onlineData, MAX_ONLINE_STATS_SIZE);

				WriteOnlineAttributeData(this, &packet, packetType, onlineData, numData, CryMatchMakingInvalidConnectionUID);	// this should go to the server
			}
			break;
		}
	case eGUPD_UpdateUserStatsReceived:
		{
			const int bufferSz = CryLobbyPacketHeaderSize;
			if (packet.CreateWriteBuffer(bufferSz))
			{
				packet.StartWrite(packetType, true);
			}
			break;
		}
	case eGUPD_FailedToReadOnlineData:
		{
			CryLog("   sending packet eGUPD_FailedToReadOnlineData");

			const int bufferSz = CryLobbyPacketHeaderSize;
			if (packet.CreateWriteBuffer(bufferSz))
			{
				packet.StartWrite(packetType, true);
			}
			break;
		}
#endif
	case eGUPD_SyncPlaylistContents:
		{
#ifdef GAME_IS_CRYSIS2
			const SPlaylist *pPlaylist = g_pGame->GetPlaylistManager()->GetCurrentPlaylist();

			const ILevelRotation::TExtInfoId playlistId = pPlaylist->id;

			ILevelRotation *pRotation = g_pGame->GetIGameFramework()->GetILevelSystem()->FindLevelRotationForExtInfoId(playlistId);
			CRY_ASSERT(pRotation);
			if (pRotation)
			{
				int numLevels = pRotation->GetLength();

				const uint32 bufferSize = (CryLobbyPacketHeaderSize + CryLobbyPacketUINT8Size + (CryLobbyPacketUINT32Size * (2 * numLevels)));
				if (packet.CreateWriteBuffer(bufferSize))
				{
					packet.StartWrite(packetType, true);
					packet.WriteUINT8((uint8) numLevels);

					int originalNext = pRotation->GetNext();
					pRotation->First();
					for (int i = 0; i < numLevels; ++ i)
					{
						const char *pGameRules = pRotation->GetNextGameRules();
						const char *pLevel = pRotation->GetNextLevel();

						const uint32 gameRulesHash = GameLobbyData::ConvertGameRulesToHash(pGameRules);
						const uint32 levelHash = GameLobbyData::ConvertMapToHash(pLevel);

						packet.WriteUINT32(gameRulesHash);
						packet.WriteUINT32(levelHash);

						pRotation->Advance();
					}

					// Now we need to put the rotation back where it was
					pRotation->First();
					while (pRotation->GetNext() != originalNext)
					{
						pRotation->Advance();
					}
				}

			}
#endif
			break;
		}
	case eGUPD_SyncExtendedServerDetails:
		{
			CryLog("  writing eGUPD_SyncExtendedServerDetails");

			uint32 bufferSize = (CryLobbyPacketHeaderSize);
#if USE_CRYLOBBY_GAMESPY
			bufferSize += (CryLobbyPacketUINT8Size * DETAILED_SESSION_INFO_MOTD_SIZE) + (CryLobbyPacketUINT8Size * DETAILED_SESSION_INFO_URL_SIZE);
			bufferSize -= 2;	// need to remove null terminators
#endif
			bufferSize += (CryLobbyPacketBoolSize * 2);

			if (packet.CreateWriteBuffer(bufferSize))
			{
				packet.StartWrite(packetType, true);

#if USE_CRYLOBBY_GAMESPY
				char motd[DETAILED_SESSION_INFO_MOTD_SIZE] = {0};
				char url[DETAILED_SESSION_INFO_URL_SIZE] = {0};
				
				strncpy(motd, m_detailedServerInfo.m_motd, DETAILED_SESSION_INFO_MOTD_SIZE);
				strncpy(url, m_detailedServerInfo.m_url, DETAILED_SESSION_INFO_URL_SIZE);

				motd[DETAILED_SESSION_INFO_MOTD_SIZE-1] = 0;
				url[DETAILED_SESSION_INFO_URL_SIZE-1] = 0;

				packet.WriteString(motd, DETAILED_SESSION_INFO_MOTD_SIZE);
				packet.WriteString(url, DETAILED_SESSION_INFO_URL_SIZE);
#endif

				const bool  isPrivate = IsPrivateGame();
				const bool  isPassworded = IsPasswordedGame();
				CryLog("    writing bool isPrivate=%s", (isPrivate?"true":"false"));
				packet.WriteBool(isPrivate);
				CryLog("    writing bool isPassworded=%s", (isPassworded?"true":"false"));
				packet.WriteBool(isPassworded);
			}
			break;
		}
	case eGUPD_UnloadPreviousLevel:
		{
			CryLog("  writing eGUPD_UnloadPreviousLevel");
			const uint32 bufferSize = (CryLobbyPacketHeaderSize);
			if (packet.CreateWriteBuffer(bufferSize))
			{
				packet.StartWrite(packetType, true);
			}
			break;
		}
	}

#if INCLUDE_DEDICATED_LEADERBOARDS
	// eGUPD_StartOfGameUserStats, eGUPD_EndOfGameUserStats, eGUPD_UpdateUserStats have to fragment the packet and send it itself, 
	// this is why the guard is here
	if(packetType != eGUPD_StartOfGameUserStats && packetType != eGUPD_EndOfGameUserStats && packetType != eGUPD_UpdateUserStats)
#endif
	{
		SendPacket(&packet, packetType, connectionUID);
	}
}

void CGameLobby::SendPacket(CCryLobbyPacket *pPacket, GameUserPacketDefinitions packetType, SCryMatchMakingConnectionUID connectionUID)
{
	CRY_ASSERT_TRACE(pPacket->GetWriteBuffer() != NULL, ("Haven't written any data, packetType '%d'", packetType));
	CRY_ASSERT_TRACE(pPacket->GetWriteBufferPos() == pPacket->GetReadBufferSize(), ("Packet size doesn't match data size, packetType '%d'", packetType));
	CRY_ASSERT_TRACE(pPacket->GetReliable(), ("Unreliable packet sent, packetType '%d'", packetType));

	ICryMatchMaking *pMatchmaking = gEnv->pNetwork->GetLobby()->GetLobbyService()->GetMatchMaking();
	const int packetSize = pPacket->GetWriteBufferPos();
	if (packetSize > 0)
	{
		ECryLobbyError result = eCLE_Success;
		if(m_server)
		{
			if(connectionUID != CryMatchMakingInvalidConnectionUID)
			{
				CryLog("Send packet of type to clients(%d) '%d'", connectionUID.m_uid, packetType);
				result = pMatchmaking->SendToClient(pPacket, m_currentSession, connectionUID);
			}
			else
			{
				CryLog("Send packet of type to all clients '%d'", packetType);
				result = NetworkUtils_SendToAll(pPacket, m_currentSession, m_nameList, true);
			}
		}
		else
		{
			CryLog("Send packet of type to server '%d'", packetType);
			result = pMatchmaking->SendToServer(pPacket, m_currentSession);
		}
		if (result != eCLE_Success)
		{
			CryLog("GameLobby::SendPacket ERROR sending the packet %d type %d",(int)result,(int)packetType);
		}
	}
	else
	{
		CryLog("GameLobby::SendPacket ERROR trying to send an invalid sized packet %d typed %d",packetSize,(int)packetType);
	}
}

//static-------------------------------------------------------------------------
void CGameLobby::ReadOnlineAttributeData(CCryLobbyPacket *pPacket, SSessionNames::SSessionName *pSessionName, IPlayerProfileManager *pPlayerProfileManager)
{
#if INCLUDE_DEDICATED_LEADERBOARDS
	SCryLobbyUserData *pData = &pSessionName->m_onlineData[0];
	int32 onlineDataCount =	pPlayerProfileManager->GetOnlineAttributeCount();
	int32 sizeRead = CryLobbyPacketHeaderSize;

	pPlayerProfileManager->GetOnlineAttributesDataFormat(pData, MAX_ONLINE_STATS_SIZE); // need to ensure our format is set
	
	for(int i = pSessionName->m_recvOnlineDataCount; i < onlineDataCount; ++i, ++pSessionName->m_recvOnlineDataCount)
	{
		if(sizeRead >= ONLINE_STATS_FRAGMENTED_PACKET_SIZE)
		{
			break;
		}

		pPacket->ReadCryLobbyUserData(&pData[i]);
		sizeRead += GetCryLobbyUserTypeSize(pData[i].m_type);
	}
#endif
}

//---------------------------------------
void CGameLobby::ReadPacket(SCryLobbyUserPacketData** ppPacketData)
{
	if (m_bCancelling || (GetState() == eLS_Leaving))
	{
		CryLog("CGameLobby::ReadPacket() Received a packet while cancelling, ignoring");
		return;
	}

	SCryLobbyUserPacketData* pPacketData = (*ppPacketData);
	CCryLobbyPacket* pPacket = pPacketData->pPacket;
	CRY_ASSERT_MESSAGE(pPacket->GetReadBuffer() != NULL, "No packet data");

	uint32 packetType = pPacket->StartRead();
	CryLog("Read packet of type '%d' lobby=%p", packetType, this);

	switch(packetType)
	{
	case eGUPD_LobbyStartCountdownTimer:
		{
			CryLog("[tlh]   reading packet of type 'eGUPD_LobbyStartCountdownTimer'");
			CRY_ASSERT(!m_server);

			VOTING_DBG_LOG("[tlh]     whilst vote status = %d", m_localVoteStatus);

			m_hasReceivedStartCountdownPacket = true;

			m_startTimerCountdown = pPacket->ReadBool();
			m_initialStartTimerCountdown = pPacket->ReadBool();
			m_startTimer = (float) pPacket->ReadUINT8();
			m_votingEnabled = pPacket->ReadBool();
			m_votingClosed = pPacket->ReadBool();
			m_leftWinsVotes = pPacket->ReadBool();

			m_startTimerLength = m_initialStartTimerCountdown ? gl_initialTime : g_pGameCVars->gl_time;

			VOTING_DBG_LOG("[tlh]     READ this data:");
			VOTING_DBG_LOG("[tlh]       m_startTimerCountdown = %d", m_startTimerCountdown);
			VOTING_DBG_LOG("[tlh]       m_startTimer = %d", (uint8) m_startTimer);
			VOTING_DBG_LOG("[tlh]       pInternalData->m_votingEnabled = %d", m_votingEnabled);
			VOTING_DBG_LOG("[tlh]       pInternalData->m_votingClosed = %d", m_votingClosed);
			VOTING_DBG_LOG("[tlh]       pInternalData->m_leftWinsVotes = %d", m_leftWinsVotes);

			if (m_votingEnabled)
			{
				CRY_ASSERT(g_pGameCVars->gl_enablePlaylistVoting);

				if (m_localVoteStatus == eLVS_awaitingCandidates)
				{
					SetLocalVoteStatus(eLVS_notVoted);
					VOTING_DBG_LOG("[tlh] set m_localVoteStatus [4] to eLVS_notVoted");
				}

				UpdateVoteChoices();
#ifdef USE_C2_FRONTEND
				UpdateVotingCandidatesFlashInfo();
#endif //#ifdef USE_C2_FRONTEND
				CheckForVotingChanges(false);
#ifdef USE_C2_FRONTEND
				UpdateVotingInfoFlashInfo();
#endif //#ifdef USE_C2_FRONTEND

				if (m_votingClosed)
				{
					UpdateRulesAndMapFromVoting();
				}

				m_bHasReceivedVoteOptions = true;
			}

			m_sessionUserDataDirty = true;

			if (m_bNeedToSetAsElegibleForHostMigration)
			{
				m_taskQueue.AddTask(CLobbyTaskQueue::eST_SessionSetLocalFlags, false);
				m_bNeedToSetAsElegibleForHostMigration = false;
			}
		}
		break;
	case eGUPD_LobbyGameHasStarted:
		{
			CRY_ASSERT(!m_server);
			uint32 requiredDLCs = pPacket->ReadUINT32();
			bool gameStillInProgress = pPacket->ReadBool();
			m_votingEnabled = pPacket->ReadBool();

			uint32 gameRulesHash = pPacket->ReadUINT32();
			uint32 mapHash = pPacket->ReadUINT32();

			m_loadingGameRules = GameLobbyData::GetGameRulesFromHash(gameRulesHash, "");
			m_loadingLevelName = GameLobbyData::GetMapFromHash(mapHash, "");

			if(!IsQuitting())
			{
				if ((!m_bSessionStarted) && (GetState() != eLS_JoinSession))
				{
#ifdef GAME_IS_CRYSIS2
					CDLCManager* pDLCManager = g_pGame->GetDLCManager();
					bool localPlayerMeetsDLCRequirements = CDLCManager::MeetsDLCRequirements(requiredDLCs, pDLCManager->GetLoadedDLCs());
#else
					bool localPlayerMeetsDLCRequirements = true;
#endif
					
					if (!localPlayerMeetsDLCRequirements)
					{
#ifdef USE_C2_FRONTEND
						// Unfortunately the player doesn't meet the DLC requirements so he can't join the game
						if (CMPMenuHub* pMPMenu=CMPMenuHub::GetMPMenuHub())
						{
							pMPMenu->GoToCurrentLobbyServiceScreen();
						}
#endif //#ifdef USE_C2_FRONTEND

						LeaveSession(true);
						
#ifdef GAME_IS_CRYSIS2
						g_pGame->GetWarnings()->AddWarning("DLCUnavailable", NULL);
#endif
					}
					else
					{
						if (gameStillInProgress)
						{
							if (m_votingEnabled && !m_gameHadStartedWhenPlaylistRotationSynced)
							{
								AdvanceLevelRotationAccordingToVotingResults();
							}

							SetState(eLS_JoinSession);
						}
						else
						{
							m_bWaitingForGameToFinish = true;
						}
					}
				}
				else
				{
					CryLog("  received duplicate eGUPD_LobbyGameHasStarted packet, probably due to a host migration");
				}
			}
			else
			{
				CryLog("  in the process quitting, ignoring eGUPD_LobbyGameHasStarted");
			}
		}
		break;

	case eGUPD_LobbyEndGame:
		{
			CRY_ASSERT(!m_server);

			if(!IsQuitting())
			{
				if (m_bWaitingForGameToFinish)
				{
					m_bWaitingForGameToFinish = false;
				}
				else
				{
					m_bHasReceivedVoteOptions = false;
					SetState(eLS_EndSession);
				}
			}
			else
			{
				CryLog("  ignoring lobby end game as in the process of leaving session");
			}
		}
		break;
	case eGUPD_LobbyEndGameResponse:
		{
			CRY_ASSERT(m_server);
			m_endGameResponses++;

			CRY_ASSERT(m_state != eLS_Game);
			CryLog("[tlh] calling SendPacket(eGUPD_SyncPlaylistRotation) [3]");
			SendPacket(eGUPD_SyncPlaylistRotation, pPacketData->connection);
		}
		break;
	case eGUPD_LobbyUpdatedSessionInfo:
		{
			CRY_ASSERT(!m_server);
			m_taskQueue.AddTask(CLobbyTaskQueue::eST_Query, true);
		}
		break;
	case eGUPD_LobbyMoveSession:
		{
			CRY_ASSERT(!m_server);
			CRY_ASSERT(m_gameLobbyMgr->IsPrimarySession(this));

			ICryMatchMaking *pMatchmaking = gEnv->pNetwork->GetLobby()->GetLobbyService()->GetMatchMaking();
			CrySessionID session = pMatchmaking->ReadSessionIDFromPacket(pPacket);

			if(!IsQuitting())
			{
				SSessionNames &playerList = m_nameList;
				unsigned int size = playerList.Size();

				if (size > 0)
				{
					SSessionNames::SSessionName &localPlayer = playerList.m_sessionNames[0];
					SCryMatchMakingConnectionUID localConnection = localPlayer.m_conId;

					m_allowRemoveUsers = false;

					for(unsigned int i = 0; i < size; i++)
					{
						// because we are preserving the list bewteen servers, need to invalidate the
						// connection ids so no the client does not get confused with potential
						// duplication ids
						playerList.m_sessionNames[i].m_conId = CryMatchMakingInvalidConnectionUID;
					}

					// Reservation will have been made using the old session connection UID so we need to save it before leaving
					JoinServer(session, "Merging Session", localConnection, false);

#if defined (TRACK_MATCHMAKING)
					if( CMatchmakingTelemetry* pMMTel = g_pGame->GetMatchMakingTelemetry() )
					{
						pMMTel->AddEvent( SMMMergeRequestedEvent( session ) );
					}
#endif
				}
			}
			else
			{
				CryLog("  not moving to new session as in the process of quitting");
			}
		}
		break;
	case eGUPD_SetTeam:
		{
			CRY_ASSERT(!m_server);

			// Remove any previous placeholder entries
			m_nameList.RemoveBlankEntries();

			const uint32 numPlayersInPacket = uint32(pPacket->ReadUINT8());
			for (uint32 i = 0; i < numPlayersInPacket; ++ i)
			{
				SCryMatchMakingConnectionUID conId = pPacket->ReadConnectionUID();
				uint8 teamId = pPacket->ReadUINT8();

				int playerIndex = m_nameList.Find(conId);
				if (playerIndex == SSessionNames::k_unableToFind)
				{
					CryLog("eGUPD_SetTeam Packet telling about player who doesn't exist yet, uid=%u", conId.m_uid);
					playerIndex = m_nameList.Insert(CryUserInvalidID, conId, "", NULL, false);
				}
				m_nameList.m_sessionNames[playerIndex].m_teamId = teamId;
			}
			m_nameList.m_dirty = true;
			break;
		}
	case eGUPD_SendChatMessage:
		{
			CRY_ASSERT(m_server);

			m_chatMessageStore.Clear();
			m_chatMessageStore.teamId = pPacket->ReadUINT8();

			uint8 messageSize = pPacket->ReadUINT8(); // includes +1 for null terminator
			CRY_ASSERT(messageSize <= MAX_CHATMESSAGE_LENGTH);
			
			if (messageSize <= MAX_CHATMESSAGE_LENGTH)
			{
				char buffer[MAX_CHATMESSAGE_LENGTH];
				pPacket->ReadString(buffer, messageSize);

				m_chatMessageStore.message = buffer;
				m_chatMessageStore.conId = pPacketData->connection;

				SendChatMessageToClients();
			}
			break;
		}
	case eGUPD_ChatMessage:
		{
			CRY_ASSERT(!m_server);

			SChatMessage chatMessage;
			chatMessage.conId = pPacket->ReadConnectionUID();
			chatMessage.teamId = pPacket->ReadUINT8();

			uint8 messageSize = pPacket->ReadUINT8(); // includes +1 for null terminator
			CRY_ASSERT(messageSize <= MAX_CHATMESSAGE_LENGTH);

			if (messageSize <= MAX_CHATMESSAGE_LENGTH)
			{
				char buffer[MAX_CHATMESSAGE_LENGTH];
				pPacket->ReadString(buffer, messageSize);

				chatMessage.message = buffer;

				RecievedChatMessage(&chatMessage);
			}

			break;
		}
	case eGUPD_ReservationRequest:
		{
			CryLog("  reading packet of type 'eGUPD_ReservationRequest'");
			CRY_ASSERT(m_server);

			int  numReservationsRequested = pPacket->ReadUINT8();
			CRY_ASSERT(numReservationsRequested <= MAX_RESERVATIONS);
			numReservationsRequested = MIN(numReservationsRequested, MAX_RESERVATIONS);
			CryLog("    numReservationsRequested = %d", numReservationsRequested);

			SCryMatchMakingConnectionUID  requestedReservations[MAX_RESERVATIONS];
			for (int i=0; i<numReservationsRequested; i++)
			{
				requestedReservations[i] = pPacket->ReadConnectionUID();
			}

			const EReservationResult  resres = DoReservations(numReservationsRequested, requestedReservations);
			switch (resres)
			{
			case eRR_Success:
				{
					CryLog("    sending eGUPD_ReservationsMade packet to leader {%llu,%d}", pPacketData->connection.m_sid, pPacketData->connection.m_uid);
					SendPacket(eGUPD_ReservationsMade, pPacketData->connection);
					break;
				}
			case eRR_Fail:
				{
					CryLog("    sending eGUPD_ReservationFailedSessionFull packet to leader {%llu,%d} because not enough space for him and his squad", pPacketData->connection.m_sid, pPacketData->connection.m_uid);
					SendPacket(eGUPD_ReservationFailedSessionFull, pPacketData->connection);
					break;
				}
			case eRR_NoneNeeded:
				{
					CryLog("    NO RESERVATIONS REQUIRED, sending SUCCESS but this SHOULD NOT HAPPEN");
					SendPacket(eGUPD_ReservationsMade, pPacketData->connection);
					break;
				}
			default:
				{
					CRY_ASSERT(0);
					break;
				}
			}

			break;
		}
	case eGUPD_ReservationClientIdentify:
		{
			CryLog("  reading packet of type 'eGUPD_ReservationClientIdentify'");
			CRY_ASSERT(m_server);
			
			const SCryMatchMakingConnectionUID requestedUID = pPacket->ReadConnectionUID();
			CryLog("    client identifying itself as uid=%u, sid=%llu", requestedUID.m_uid, requestedUID.m_sid);

			const bool bHasPlaylist = pPacket->ReadBool();

			ICryMatchMaking *pMatchmaking = gEnv->pNetwork->GetLobby()->GetLobbyService()->GetMatchMaking();
			CRY_ASSERT(pMatchmaking);
			CrySessionID joinersSessionId = pMatchmaking->ReadSessionIDFromPacket(pPacket);

			// Are two sessions trying to merge into each other? If so, we need to cancel the merge on one side.
			if (BidirectionalMergingRequiresCancel(joinersSessionId))
			{
				FindGameCancelMove();
			}

			const float  timeNow = gEnv->pTimer->GetAsyncCurTime();
			int  reservedCount = 0;
			bool  hadReservation = false;

			for (int i=0; i<ARRAY_COUNT(m_slotReservations); i++)
			{
				SSlotReservation*  res = &m_slotReservations[i];

				if (res->m_con != CryMatchMakingInvalidConnectionUID)
				{
					CryLog("    slot %i reserved for uid=%u, sid=%llu", i, res->m_con.m_uid, res->m_con.m_sid);
					if (res->m_con == requestedUID)
					{
						res->m_con = CryMatchMakingInvalidConnectionUID;
						hadReservation = true;
						CryLog("      removed reservation as it's for this client!");
					}
					else if ((timeNow - res->m_timeStamp) > gl_slotReservationTimeout)
					{
						res->m_con = CryMatchMakingInvalidConnectionUID;
						CryLog("      removed reservation as it's expired");
					}
					else
					{
						reservedCount++;
						CryLog("      reservation valid, will timeout in %f seconds", (timeNow - res->m_timeStamp));
					}
				}
			}

			const int  numPrivate = m_sessionData.m_numPrivateSlots;
			const int  numPublic = m_sessionData.m_numPublicSlots;
			const int  numFilledExc = (m_nameList.Size() - 1);  // NOTE -1 because client in question will be in list already but we don't want them to be included in the calculations
			const int  numEmptyExc = ((numPrivate + numPublic) - numFilledExc);

			CRY_ASSERT(numFilledExc >= 0);

			CryLog("    nums private = %d, public = %d, filled (exc. client) = %d, empty (exc. client) = %d, reserved = %d", numPrivate, numPublic, numFilledExc, numEmptyExc, reservedCount);

			bool  enoughSpace = false;

			if (numEmptyExc > 0)
			{
				if (hadReservation || (numEmptyExc > reservedCount))
				{
					enoughSpace = true;
				}
				else
				{
					CryLog("      client being kicked because all empty spaces have been reserved and they didn't have a reservation");
				}
			}
			else
			{
				CryLog("      client being kicked because there are no empty spaces");
			}

			if (enoughSpace)
			{
				SSessionNames &names = m_nameList;
				int index = names.Find(pPacketData->connection);
				if (index != SSessionNames::k_unableToFind)
				{
					names.m_sessionNames[index].m_bFullyConnected = true;
				}

				SendPacket(eGUPD_SyncExtendedServerDetails, pPacketData->connection);

#ifdef GAME_IS_CRYSIS2
				// Check if we need to sync the playlist
				if (g_pGame->GetPlaylistManager()->HavePlaylistSet() && (!bHasPlaylist))
				{
					SendPacket(eGUPD_SyncPlaylistContents, pPacketData->connection);
				}
				// Send variant info before potentially starting the game
				SendPacket(eGUPD_SetGameVariant, pPacketData->connection);

				if (CPlaylistManager* plMgr=g_pGame->GetPlaylistManager())
				{
					if (plMgr->HavePlaylistSet())
					{
						m_bPlaylistHasBeenAdvancedThroughConsole = false;
						CryLog("[tlh] calling SendPacket(eGUPD_SyncPlaylistRotation) [1]");
						SendPacket(eGUPD_SyncPlaylistRotation, pPacketData->connection);
					}
				}
#endif
				
				if(m_state == eLS_Game)
				{
#if INCLUDE_DEDICATED_LEADERBOARDS
					// delay sending game join until we have the client online stats
					ICryStats *pStats = gEnv->pNetwork->GetLobby()->GetStats();
					if((!pStats) || (pStats->GetLeaderboardType() == eCLLT_P2P) || (index == SSessionNames::k_unableToFind) || (names.m_sessionNames[index].m_onlineStatus == eOAS_Initialised))
					{
						SendPacket(eGUPD_LobbyGameHasStarted, pPacketData->connection);
					}
					else
					{
						CryLog("[GameLobby] delaying send of game join to user %s pStats %p leaderBoardType %d userIndex %d onlineStatus %d", 
							names.m_sessionNames[index].m_name, pStats, pStats ? pStats->GetLeaderboardType() : -1, index, index >= 0 ? names.m_sessionNames[index].m_onlineStatus : -1);
					}
#else
					SendPacket(eGUPD_LobbyGameHasStarted, pPacketData->connection);
#endif
				}
				else if(m_state == eLS_Lobby)
				{
					SendPacket(eGUPD_LobbyStartCountdownTimer, pPacketData->connection);
					SendPacket(eGUPD_SetTeam, pPacketData->connection);		// Temporary fix for team message being lost due to arriving before session join callback
				}
			}
			else
			{
				CryLog("    sending eGUPD_ReservationFailedSessionFull packet to {%llu,%d}", pPacketData->connection.m_sid, pPacketData->connection.m_uid);
				SendPacket(eGUPD_ReservationFailedSessionFull, pPacketData->connection);
			}

			break;
		}
	case eGUPD_ReservationsMade:
		{
			CryLog("  reading packet of type 'eGUPD_ReservationsMade'");
			CRY_ASSERT(!m_server);

			if(m_squadReservation)
			{
				CSquadManager *pSquadManager = g_pGame->GetSquadManager();
				if(pSquadManager->SquadsSupported())
				{
					CryLog("    sending eGUPD_SquadJoinGame packet!");
					pSquadManager->SendSquadPacket(eGUPD_SquadJoinGame);
				}
				else
				{
					CryLog("    squads not supported, sending eGUPD_SquadNotSupported packet!");
					pSquadManager->SendSquadPacket(eGUPD_SquadNotSupported);
				}
			}
			else
			{
				CRY_ASSERT(m_gameLobbyMgr->IsNewSession(this));
				m_gameLobbyMgr->NewSessionResponse(this, m_currentSessionId);
			}
			break;
		}
	case eGUPD_ReservationFailedSessionFull:
		{
			CryLog("  reading packet of type 'eGUPD_ReservationFailedSessionFull'");
			CRY_ASSERT(!m_server);

			if(m_squadReservation)
			{
				if (m_bMatchmakingSession)
				{
					// fix for DT #31667 and #31781
					CryLog("    ... session WAS a matchmaking (Quick Game) session, so leaving session and adding a new Create task to try again WITHOUT showing an error");
					LeaveSession(true);
					//TODO: handle this correctly with new Lua matchmaking
					m_taskQueue.AddTask(CLobbyTaskQueue::eST_Create, true);
				}
				else
				{
					CryLog("    ... session WASN'T a matchmaking (Quick Game) session, so showing error, deleting session and returning to main menu");
					CSquadManager::HandleCustomError("Disconnect", "@ui_menu_squad_error_not_enough_room", true, true);
				}
			}
			else
			{
				if (m_gameLobbyMgr->IsNewSession(this))
				{
					m_gameLobbyMgr->NewSessionResponse(this, CrySessionInvalidID);
				}
				else
				{
					// Tried to join a game where all remaining slots were reserved
					ShowErrorDialog(eCLE_SessionFull, NULL, NULL, NULL);

#ifdef USE_C2_FRONTEND
					if (CMPMenuHub *pMPMenu = CMPMenuHub::GetMPMenuHub())
					{
						pMPMenu->GoToCurrentLobbyServiceScreen();	// Go back to main menu screen
					}
#endif //#ifdef USE_C2_FRONTEND

					LeaveSession(true);
				}
			}
			break;
		}
	case eGUPD_SetGameVariant:
		{
			CRY_ASSERT(!m_server);
#ifdef GAME_IS_CRYSIS2
			CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
			if (pPlaylistManager)
			{
				pPlaylistManager->ReadSetVariantPacket(pPacket);
			}

			// Need to refresh flash
			m_sessionUserDataDirty = true;
#endif
			break;
		}
	case eGUPD_SyncPlaylistRotation:
		{
			CryLog("[tlh]   reading packet of type 'eGUPD_SyncPlaylistRotation'");
			CRY_ASSERT(!m_server);

			const uint32  seed = pPacket->ReadUINT32();
			m_playListSeed = seed;
			const int  curNextIdx = (int) pPacket->ReadUINT8();
			const bool  gameHasStarted = pPacket->ReadBool();
			const bool bAdvancedThroughConsole = pPacket->ReadBool();

			CryLog("[tlh]     reading next=%d, started=%d, advancedThruConsole=%d seed=%d", curNextIdx, gameHasStarted, bAdvancedThroughConsole, seed);

#ifdef GAME_IS_CRYSIS2
			if (CPlaylistManager* pPlaylistManager=g_pGame->GetPlaylistManager())
			{
				CRY_ASSERT(pPlaylistManager->HavePlaylistSet());

				if (pPlaylistManager->GetLevelRotation()->IsRandom())
				{
					pPlaylistManager->GetLevelRotation()->Shuffle(seed);
				}
				pPlaylistManager->AdvanceRotationUntil(curNextIdx);
				m_gameHadStartedWhenPlaylistRotationSynced = gameHasStarted;
				m_hasReceivedPlaylistSync = true;

				// Need to refresh flash
				m_sessionUserDataDirty = true;

				if (bAdvancedThroughConsole)
				{
					m_votingClosed = false;
					m_leftVoteChoice.Reset();
					m_rightVoteChoice.Reset();
					m_highestLeadingVotesSoFar = 0;
					m_leftHadHighestLeadingVotes = false;

					SetLocalVoteStatus(eLVS_notVoted);
				}

				UpdateVoteChoices();
#ifdef USE_C2_FRONTEND
				UpdateVotingCandidatesFlashInfo();
				UpdateVotingInfoFlashInfo();
#endif //#ifdef USE_C2_FRONTEND
				RefreshCustomiseEquipment();
			}
#endif

			break;
		}
	case eGUPD_UpdateCountdownTimer:
		{
			m_startTimer = (float) pPacket->ReadUINT8();
			break;
		}
	case eGUPD_RequestAdvancePlaylist:
		{
			DebugAdvancePlaylist();
			break;
		}
#if INCLUDE_DEDICATED_LEADERBOARDS
	case eGUPD_StartOfGameUserStats:
		{
			CryLog("    reading packet eGUPD_StartOfGameUserStats");
	
			IPlayerProfileManager *pPlayerProfileManager = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager();	
			SSessionNames::SSessionName *pSessionName = &m_nameList.m_sessionNames[0];
			ReadOnlineAttributeData(pPacket, pSessionName, pPlayerProfileManager);
		
			if(pSessionName->m_recvOnlineDataCount == pPlayerProfileManager->GetOnlineAttributeCount())
			{
				CryLog("[GameLobby] received user data for %s, setting to profile", pSessionName->m_name);
				pPlayerProfileManager->SetOnlineAttributes(pPlayerProfileManager->GetCurrentProfile(pPlayerProfileManager->GetCurrentUser()), pSessionName->m_onlineData, pSessionName->m_recvOnlineDataCount);
			}
			break;
		}
	case eGUPD_EndOfGameUserStats:
		{
			CryLog("   reading packet eGUPD_EndOfGameUserStats");

			IPlayerProfileManager *pPlayerProfileManager = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager();
			int userIndex = m_nameList.Find(pPacketData->connection);
			SSessionNames::SSessionName *pSessionName = &m_nameList.m_sessionNames[userIndex];

			CRY_ASSERT_MESSAGE(m_server, "received end of game user stats but we're not a server");

			if(IsPasswordedGame() || IsPrivateGame() || (g_pGame->GetPlaylistManager() && g_pGame->GetPlaylistManager()->IsUsingCustomVariant()))
			{
				CryLog("Returning early from eGUPD_EndOfGameUserStats because it's a private or passworded game (not writing leaderboards or user data)");
				return;
			}
			ReadOnlineAttributeData(pPacket, &m_nameList.m_sessionNames[userIndex], pPlayerProfileManager);

			if(AllowOnlineAttributeTasks())	// server doesn't care if client as well
			{
				if(pSessionName->m_recvOnlineDataCount == pPlayerProfileManager->GetOnlineAttributeCount())
				{
					CryLog("[GameLobby] received all EndOfGame user data for %s", pSessionName->m_name);

					pSessionName->m_onlineDataCount = pSessionName->m_recvOnlineDataCount;
					pPlayerProfileManager->ApplyChecksums(pSessionName->m_onlineData, pSessionName->m_onlineDataCount);

					if(pSessionName->m_onlineStatus == eOAS_WaitingForUpdate)
					{
						bool replaced = false;
						for (TWriteLeaderboardData::iterator it = m_writeLeaderboardData.begin();(!replaced) && (it != m_writeLeaderboardData.end()); ++it)
						{
							if (it->m_userID == pSessionName->m_userId)
							{
								CryLog("[ARC] replacing leaderboard write data for %s", pSessionName->m_name);
								it->Replace(pSessionName->m_onlineData, pSessionName->m_recvOnlineDataCount);
								replaced = true;
							}
						}

						if (!replaced)
						{
							CryLog("[ARC] adding leaderboard write data for %s", pSessionName->m_name);
							if (m_writeLeaderboardData.isfull() == false)
							{
								m_writeLeaderboardData.push_back();
								m_writeLeaderboardData.back().m_userID = pSessionName->m_userId;
								m_writeLeaderboardData.back().Replace(pSessionName->m_onlineData, pSessionName->m_onlineDataCount);
							}
							else
							{
								CryLogAlways("[ARC] unable to add leaderboard write data for %s - leaderboard write data buffer full!", pSessionName->m_name);
							}
						}

						pSessionName->m_onlineStatus = eOAS_Initialised;
					}

					int idx = FindWriteUserDataByUserID(pSessionName->m_userId);
					if(idx < 0)
					{
						CryLog(  "recieved end of game stats for %s and not found in write user data, adding", pSessionName->m_name);
						m_writeUserData.push_back(SWriteUserData(pSessionName->m_userId, pSessionName->m_onlineData, pSessionName->m_onlineDataCount));
					}
					else
					{
						CryLog(  "recieved end of game stats for %s and found in write user data, updating", pSessionName->m_name);
						SWriteUserData *pData = &m_writeUserData[idx];
						pData->SetData(pSessionName->m_onlineData, pSessionName->m_onlineDataCount);
						pData->m_requeue = pData->m_writing;
					}
				}

				// check to see if we're still waiting on anyone
				if(!AnySessionNamesWithStatus(eOAS_WaitingForUpdate))
				{
					CryLog("[GameLobby] finished waiting for client updates, adding stats/leaderboard tasks");
					WritePendingLeaderboardData();
				}
			}

			break;
		}

	case eGUPD_UpdateUserStats:
		{
			CryLog("   reading packet eGUPD_UpdateUserStats");

			IPlayerProfileManager *pPlayerProfileManager = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager();
			int userIndex = m_nameList.Find(pPacketData->connection);
			SSessionNames::SSessionName *pSessionName = &m_nameList.m_sessionNames[userIndex];
	
			CRY_ASSERT_MESSAGE(m_server, "received update user stats from client but we're not a server");

			if(pSessionName->m_onlineStatus != eOAS_Updating)
			{
				CryLog("[GameLobby] received first packet from user %s. indicating they are about to leave", pSessionName->m_name);

				pSessionName->m_onlineStatus = eOAS_Updating;
				pSessionName->m_recvOnlineDataCount = 0;
			}

			ReadOnlineAttributeData(pPacket, pSessionName, pPlayerProfileManager);

			if(pSessionName->m_recvOnlineDataCount == pPlayerProfileManager->GetOnlineAttributeCount())
			{
				CryLog("[GameLobby] received all UpdateUserStats packets for user %s", pSessionName->m_name);

				pSessionName->m_onlineDataCount = pSessionName->m_recvOnlineDataCount;
				pSessionName->m_onlineStatus = eOAS_UpdateComplete;
				pPlayerProfileManager->ApplyChecksums(pSessionName->m_onlineData, pSessionName->m_onlineDataCount);

				SendPacket(eGUPD_UpdateUserStatsReceived, pPacketData->connection);
			}
			break;
		}

	case eGUPD_UpdateUserStatsReceived:
		{
			CryLog("  reading packet eGUPD_UpdateUserStatsReceived");

#ifdef USE_C2_FRONTEND
			CFlashFrontEnd *pFFE = g_pGame->GetFlashMenu();
			if(pFFE)
			{
				if(pFFE->IsDelayingSessionLeave())
				{
					pFFE->DoDelayedSessionLeave();
				}
				else
				{
					CMPMenuHub *pMPMH = pFFE->GetMPMenu();
					if(pMPMH && pMPMH->IsDelayingSessionLeave())
					{
						pMPMH->DoDelayedSessionLeave();
					}
				}
			}
#endif //#ifdef USE_C2_FRONTEND

			break;
		}
	case eGUPD_FailedToReadOnlineData:
		{
			CryLog("  reading packet eGUPD_FailedToReadOnlineData");
		
			LeaveSession(true);

#ifdef USE_C2_FRONTEND
			CMPMenuHub *pMH = CMPMenuHub::GetMPMenuHub();
			if(pMH)
			{
				pMH->SetDisconnectError(CMPMenuHub::eDCE_HostMigrationFailed, true);
			}
#endif //#ifdef USE_C2_FRONTEND
			break;
		}
#endif
	case eGUPD_SyncPlaylistContents:
		{
#ifdef GAME_IS_CRYSIS2
			CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
			const ILevelRotation::TExtInfoId playlistId = pPlaylistManager->CreateCustomPlaylist(PLAYLIST_MANAGER_CUSTOM_NAME);

			ILevelRotation *pRotation = g_pGame->GetIGameFramework()->GetILevelSystem()->CreateNewRotation(playlistId);

			const int numLevels = (int) pPacket->ReadUINT8();
			for (int i = 0; i < numLevels; ++ i)
			{
				const uint32 gameRulesHash = pPacket->ReadUINT32();
				const uint32 levelHash = pPacket->ReadUINT32();

				const char *pGameRules = GameLobbyData::GetGameRulesFromHash(gameRulesHash);
				const char *pLevel = GameLobbyData::GetMapFromHash(levelHash);

				pRotation->AddLevel(pLevel, pGameRules);
			}

			pPlaylistManager->ChoosePlaylistById(playlistId);
#endif

			break;
		}
	case eGUPD_SyncExtendedServerDetails:
		{
			CryLog("  reading eGUPD_SyncExtendedServerDetails");

#if USE_CRYLOBBY_GAMESPY
			pPacket->ReadString(m_detailedServerInfo.m_motd, DETAILED_SESSION_INFO_MOTD_SIZE);
			pPacket->ReadString(m_detailedServerInfo.m_url, DETAILED_SESSION_INFO_URL_SIZE);

			if (!gEnv->IsDedicated())
			{
				CheckGetServerImage();
			}
#endif

			const bool  isPrivate = pPacket->ReadBool();
			CryLog("    read isPrivate=%s, calling SetPrivateGame()", (isPrivate?"true":"false"));
			SetPrivateGame(isPrivate);

			const bool  isPassworded = pPacket->ReadBool();
			CryLog("    read isPassworded=%s, calling SetPasswordedGame()", (isPassworded?"true":"false"));
			SetPasswordedGame(isPassworded);

			break;
		}
	case eGUPD_UnloadPreviousLevel:
		{
			CryLog("  reading eGUPD_UnloadPreviousLevel");
			if (g_pGame->GetIGameFramework()->StartedGameContext() || g_pGame->GetIGameFramework()->StartingGameContext())
			{
				DoBetweenRoundsUnload();
			}
			break;
		}
	default:
		{
			CRY_ASSERT_MESSAGE(0, "Got packet another something - just no idea what");
		}
		break;
	}

	CRY_ASSERT_MESSAGE(pPacket->GetReadBufferSize() == pPacket->GetReadBufferPos(), "Haven't read all the data");
}


//static------------------------------------------------------------------------
const char* CGameLobby::GetValidGameRules(const char* gameRules, bool returnBackup/*=false*/)
{
	if (!gameRules)
		return "";

	bool validRules = false;
	const char * fullGameRulesName = gEnv->pGame->GetIGameFramework()->GetIGameRulesSystem()->GetGameRulesName(gameRules);
	const char *backupGameRules = 0;

#ifdef GAME_IS_CRYSIS2
	CGameRulesModulesManager *pGameRulesModulesManager = CGameRulesModulesManager::GetInstance();
	const int rulesCount = pGameRulesModulesManager->GetRulesCount();
	for(int i = 0; i < rulesCount; i++)
	{
		const char* name = pGameRulesModulesManager->GetRules(i);

		if (!backupGameRules)
			backupGameRules = name;

		if(fullGameRulesName != NULL && (stricmp(name, fullGameRulesName)==0))
		{
			validRules = true;
			break;
		}
	}
#else
	validRules = true;
#endif

	if (validRules)
	{
		return fullGameRulesName;
	}
	else if (returnBackup && backupGameRules)
	{
		CryLog("Failed to find valid gamemode (tried %s), using backup %s", fullGameRulesName?fullGameRulesName:"<null> ", backupGameRules);
		return backupGameRules;
	}

	return "";
}

//static ------------------------------------------------------------------------
CryFixedStringT<32> CGameLobby::GetValidMapForGameRules(const char* inLevel, const char* gameRules, bool returnBackup/*=false*/)
{
	if (!gameRules)
		return "";

	CryFixedStringT<32> levelName = inLevel;

	const char *backupLevelName = 0;
	bool validLevel = false;

	ILevelSystem* pLevelSystem = g_pGame->GetIGameFramework()->GetILevelSystem();
	if (levelName.find("/")==string::npos)
	{
		int j=0;
		const char *loc=0;
		CryFixedStringT<32> tmp;
		IGameRulesSystem *pGameRulesSystem = g_pGame->GetIGameFramework()->GetIGameRulesSystem();
		while(loc=pGameRulesSystem->GetGameRulesLevelLocation(gameRules, j++))
		{
			tmp=loc;
			tmp.append(levelName);

			if (pLevelSystem->GetLevelInfo(tmp.c_str()))
			{
				levelName=tmp;
				break;
			}
		}
	}

	const int levelCount = pLevelSystem->GetLevelCount();
	for(int i = 0; i < levelCount; i++)
	{
		ILevelInfo *pLevelInfo = pLevelSystem->GetLevelInfo(i);
		if (pLevelInfo->SupportsGameType(gameRules))
		{
			const char* name = pLevelInfo->GetName();

			if (!backupLevelName)
				backupLevelName = name;

			if(stricmp(PathUtil::GetFileName(name), levelName.c_str())==0)
			{
				validLevel = true;
				break;
			}
		}
	}

	if (validLevel)
	{
		return levelName;
	}
	else if (returnBackup && backupLevelName)
	{
		CryLog("Failed to find valid level (tried %s) for %s gamemode, using backup %s Lobby: ", levelName.c_str(), gameRules, backupLevelName);
		return backupLevelName;
	}

	return "";
}

void CGameLobby::RecievedChatMessage(const SChatMessage* message)
{
	SSessionNames::SSessionName *player = m_nameList.GetSessionName(message->conId, false);
	if (player)
	{
		CryFixedStringT<DISPLAY_NAME_LENGTH> displayName;
		player->GetDisplayName(displayName);
		if ((m_state == eLS_Game) && g_pGame->GetGameRules())
		{
#ifdef GAME_IS_CRYSIS2
			CryLog("%s %s%s%s", CHUDUtils::LocalizeString((message->teamId!=0)?"@ui_chattype_team":"@ui_chattype_all"), displayName.c_str(),CHAT_MESSAGE_POSTFIX, message->message.c_str());

			IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActorByChannelId(message->conId.m_uid);
			if (pActor)
			{
				SHUDEventWrapper::OnChatMessage( pActor->GetEntityId(), message->teamId, message->message.c_str() );
			}
			else
			{
				CryLog("Failed to find actor for chat message");
			}
#endif
		}
		else
		{
#if ENABLE_CHAT_MESSAGES
			CryLog("%s> %s", displayName.c_str(), message->message.c_str());

			++m_chatMessagesIndex;
			if (m_chatMessagesIndex >= NUM_CHATMESSAGES_STORED)
			{
				m_chatMessagesIndex = 0;
			}

			SSessionNames::SSessionName *localPlayer = &m_nameList.m_sessionNames[0];
			m_chatMessagesArray[m_chatMessagesIndex].Set(displayName.c_str(), message->message.c_str(), player == localPlayer);

#ifdef USE_C2_FRONTEND
			CFlashFrontEnd *pFlashMenu = g_pGame->GetFlashMenu();
			if (pFlashMenu && pFlashMenu->IsMenuActive(CFlashFrontEnd::eFlM_Menu))
			{
				if (IFlashPlayer *pFlashPlayer = pFlashMenu->GetFlash())
				{
					EFlashFrontEndScreens screen = pFlashMenu->GetCurrentMenuScreen();
					if (screen == eFFES_game_lobby)
					{
						m_chatMessagesArray[m_chatMessagesIndex].SendChatMessageToFlash(pFlashPlayer, false);
					}
				}
			}
#endif //#ifdef USE_C2_FRONTEND
#endif
		}
	}
	else
	{
		CryLog("[Unknown] %s> %s", (message->teamId==0)?"":"[team] ", message->message.c_str());
	}
}

void CGameLobby::SendChatMessageToClients()
{
	CRY_ASSERT(m_server);

#ifndef _RELEASE
	CryLog("SERVER SendChatMessageToClients: channel:%d  team:%d, message:%s", m_chatMessageStore.conId.m_uid, m_chatMessageStore.teamId, m_chatMessageStore.message.c_str());
#endif

	const int nameSize = m_nameList.Size();
	for (int i=0; i<nameSize; ++i)
	{
		SSessionNames::SSessionName &player = m_nameList.m_sessionNames[i];

		int playerTeam = player.m_teamId;
		if ((m_state == eLS_Game) && g_pGame->GetGameRules())
		{
			IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActorByChannelId(player.m_conId.m_uid);
			if (pActor)
			{
				playerTeam = g_pGame->GetGameRules()->GetTeam(pActor->GetEntityId());
			}
		}

		if (m_chatMessageStore.teamId==0 || m_chatMessageStore.teamId==playerTeam)
		{
			if (!gEnv->IsDedicated() && i==0) // Assume the server at this point is 0 - only on non-dedicated
			{
				RecievedChatMessage(&m_chatMessageStore);
			}
			else
			{
				SendPacket(eGUPD_ChatMessage, player.m_conId);
			}
		}
	}
	
	m_chatMessageStore.Clear();
}

#if ENABLE_CHAT_MESSAGES
//static-------------------------------------------------------------------------
void CGameLobby::CmdChatMessage(IConsoleCmdArgs* pCmdArgs)
{
	if (pCmdArgs->GetArg(1))
	{
		CGameLobby* pLobby = g_pGame->GetGameLobby();
		if(pLobby)
		{
			pLobby->SendChatMessage(false, pCmdArgs->GetArg(1));
		}
	}
	else
	{
		CryLog("gl_say : you must provide a message");
	}
}

//static-------------------------------------------------------------------------
void CGameLobby::CmdChatMessageTeam(IConsoleCmdArgs* pCmdArgs)
{
	if (pCmdArgs->GetArg(1))
	{
		CGameLobby* pLobby = g_pGame->GetGameLobby();
		if(pLobby)
		{
			pLobby->SendChatMessage(true, pCmdArgs->GetArg(1));
		}
	}
	else
	{
		CryLog("gl_teamsay : you must provide a message");
	}	
}

//static-------------------------------------------------------------------------
void CGameLobby::SendChatMessageTeamCheckProfanityCallback( CryLobbyTaskID taskID, ECryLobbyError error, const char* pString, bool isProfanity, void* pArg )
{
	CGameLobby* pLobby = g_pGame->GetGameLobby();
	if(pLobby)
	{
		pLobby->SendChatMessageCheckProfanityCallback(true, taskID, error, pString, isProfanity, pArg );
	}
}

//static-------------------------------------------------------------------------
void CGameLobby::SendChatMessageAllCheckProfanityCallback( CryLobbyTaskID taskID, ECryLobbyError error, const char* pString, bool isProfanity, void* pArg )
{
	CGameLobby* pLobby = g_pGame->GetGameLobby();
	if(pLobby)
	{
		pLobby->SendChatMessageCheckProfanityCallback(false, taskID, error, pString, isProfanity, pArg );
	}
}

//-------------------------------------------------------------------------
void CGameLobby::SendChatMessageCheckProfanityCallback( const bool team, CryLobbyTaskID taskID, ECryLobbyError error, const char* pString, bool isProfanity, void* pArg )
{
	CryLog("CGameLobby::SendChatMessageCheckProfanityCallback()");
	INDENT_LOG_DURING_SCOPE();
	m_profanityTask = CryLobbyInvalidTaskID;

	if(!isProfanity)
	{
		CryLog("Profanity Check ok for '%s'", pString);

		CGameLobby *pGameLobby = g_pGame->GetGameLobby();
		if (pString && pGameLobby)
		{
			if (pGameLobby->IsCurrentlyInSession() && m_nameList.Size()>0)
			{
				SSessionNames::SSessionName &localPlayer = m_nameList.m_sessionNames[0];

				int teamId = 0;
				if (team)
				{
					teamId = localPlayer.m_teamId;

					if ((m_state == eLS_Game) && g_pGame->GetGameRules())
					{
						IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActorByChannelId(localPlayer.m_conId.m_uid);
						if (pActor)
						{
							teamId = g_pGame->GetGameRules()->GetTeam(pActor->GetEntityId());
						}
					}

					if (teamId == 0)
					{
						CryLog("Send team message : not on a team so sending to all");
					}
				}

				pGameLobby->m_chatMessageStore.Set(localPlayer.m_conId, teamId, pString);

				if (pGameLobby->m_chatMessageStore.message.size()>0)
				{
					CryLog("Sending '%s'", pString);
					if (m_server)
					{
						pGameLobby->SendChatMessageToClients();
					}
					else
					{
						pGameLobby->SendPacket(eGUPD_SendChatMessage);
					}
				}
			}
		}
	}
}

//-------------------------------------------------------------------------
void CGameLobby::SendChatMessage(bool team, const char* message)
{
	CryLog("CGameLobby::SendChatMessage()");
	INDENT_LOG_DURING_SCOPE();

	if(m_profanityTask != CryLobbyInvalidTaskID)
	{
		CryLog("CGameLobby::SendChatMessage() :: Can't send chat message still waiting for profanity check to return!");
		return;
	}

	CGameLobby *pGameLobby = g_pGame->GetGameLobby();
	if (message && pGameLobby)
	{
		if (pGameLobby->IsCurrentlyInSession() && m_nameList.Size()>0)
		{
			SSessionNames::SSessionName &localPlayer = m_nameList.m_sessionNames[0];

			int teamId = 0;

			if (team)
			{
				teamId = localPlayer.m_teamId;

				if ((m_state == eLS_Game) && g_pGame->GetGameRules())
				{
					IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActorByChannelId(localPlayer.m_conId.m_uid);
					if (pActor)
					{
						teamId = g_pGame->GetGameRules()->GetTeam(pActor->GetEntityId());
					}
				}

				if (teamId == 0)
				{
					CryLog("Send team message : not on a team so sending to all");
					team = false;
				}
			}

			CryLobbyCheckProfanityCallback whichFunctor;

			if(team)
			{
				whichFunctor = SendChatMessageTeamCheckProfanityCallback;
			}
			else
			{
				whichFunctor = SendChatMessageAllCheckProfanityCallback;
			}

			// Trigger a task to check the profanity, the callback will actualy send the data is the string isn't profane.
			ICryLobbyService*	pLobbyService = gEnv->pNetwork->GetLobby()->GetLobbyService();
			if ( pLobbyService )
			{
				ECryLobbyError	error =	pLobbyService->CheckProfanity( message, &m_profanityTask, whichFunctor, NULL );
				if(error!=eCLE_Success)
				{
					CryLog("Unable to check for profanity in '%s'", message);
				}
			}
		}
	}
}
#endif

//static-------------------------------------------------------------------------
void CGameLobby::CmdStartGame(IConsoleCmdArgs* pCmdArgs)
{
	CGameLobby *pGameLobby = g_pGame->GetGameLobby();
	if (pGameLobby)
	{
		if(pGameLobby->m_server)
		{
#ifdef GAME_IS_CRYSIS2
			// Check if any players are connected without the required DLC
			const int nameSize = pGameLobby->m_nameList.Size();

			stack_string playersMissingDLC;
			uint32 requiredDLCs = g_pGame->GetDLCManager()->GetRequiredDLCs();
			for (int i=0; i<nameSize; ++i)
			{
				SSessionNames::SSessionName &player = pGameLobby->m_nameList.m_sessionNames[i];
				uint32 loadedDLC = (uint32)player.m_userData[eLUD_LoadedDLCs];
				if (!CDLCManager::MeetsDLCRequirements(requiredDLCs, loadedDLC))
				{
					CryLog("CGameLobby::CmdStartGame: %s does not meet DLC requirements for this game", player.m_name);
					if (!playersMissingDLC.empty())
					{
						playersMissingDLC.append(", ");
					}
					playersMissingDLC.append(player.m_name);
				}
			}
			if (!playersMissingDLC.empty())
			{
				g_pGame->GetWarnings()->AddWarning(pGameLobby->m_DLCServerStartWarningId, pGameLobby, playersMissingDLC.c_str());
			}
			else
			{
				pGameLobby->m_bSkipCountdown = true;
			}
#else
			pGameLobby->m_bSkipCountdown = true;
#endif
		}
	}
}	

//static-------------------------------------------------------------------------
void CGameLobby::CmdSetMap(IConsoleCmdArgs* pCmdArgs)
{
	CGameLobby *pGameLobby = g_pGame->GetGameLobby();
	if (pGameLobby)
	{
		if(pCmdArgs->GetArgCount() == 2)
		{
			if(!pGameLobby->m_server)
			{
				CryLog("Server only command");
				return;
			}

			const char *gameRules = pGameLobby->GetCurrentGameModeName();

			CryFixedStringT<32> levelName = CGameLobby::GetValidMapForGameRules(pCmdArgs->GetArg(1), gameRules, false);

			if (levelName.empty()==false)
			{
				pGameLobby->m_currentLevelName = levelName.c_str();

				if (g_pGame->GetIGameFramework()->StartedGameContext() || g_pGame->GetIGameFramework()->StartingGameContext())
				{
					gEnv->pConsole->ExecuteString("unload", false, true);
					pGameLobby->SetState(eLS_EndSession);
					pGameLobby->m_bServerUnloadRequired = false;

					pGameLobby->m_pendingLevelName = levelName.c_str();
				}
				else
				{
					ICVar *pCVar = gEnv->pConsole->GetCVar("sv_gamerules");
					if (pCVar)
					{
						pCVar->Set(pGameLobby->m_currentGameRules.c_str());
					}

					CryFixedStringT<128> command;
					command.Format("map %s s nb", levelName.c_str());
					gEnv->pConsole->ExecuteString(command.c_str(), false, true);
					pGameLobby->m_taskQueue.AddTask(CLobbyTaskQueue::eST_Update, true);
				}
			}
			else
			{
				//This is a console command, CryLogAlways usage is correct
				CryLogAlways("Failed to find level or level not supported on current gamerules");
			}
		}
		else
		{
			//This is a console command, CryLogAlways usage is correct
			CryLogAlways("Usage: gl_map <mapname>");
		}
	}
}

//-------------------------------------------------------------------------
void CGameLobby::ChangeMap( const char *pMapName )
{
	const char *gameRules = GetCurrentGameModeName();

	CryFixedStringT<32> levelName = CGameLobby::GetValidMapForGameRules(pMapName, gameRules, false);

	if (levelName.empty() == false)
	{
		m_currentLevelName = levelName.c_str();
		m_taskQueue.AddTask(CLobbyTaskQueue::eST_Update, true);
	}
}

//static-------------------------------------------------------------------------
void CGameLobby::CmdSetGameRules(IConsoleCmdArgs* pCmdArgs)
{
	CGameLobby *pGameLobby = g_pGame->GetGameLobby();
	if (pGameLobby)
	{
		if(pCmdArgs->GetArgCount() == 2)
		{
			if(!pGameLobby->m_server)
			{
				CryLog("Server only command");
				return;
			}

			CryFixedStringT<32> validGameRules = CGameLobby::GetValidGameRules(pCmdArgs->GetArg(1),false);

			if(validGameRules.empty()==false)
			{
				pGameLobby->m_currentGameRules = validGameRules.c_str();
				pGameLobby->m_taskQueue.AddTask(CLobbyTaskQueue::eST_Update, true);
				pGameLobby->GameRulesChanged(validGameRules.c_str());
			}
			else
			{
				//This is a console command, CryLogAlways usage is correct
				CryLogAlways("Failed to find rules");
			}
		}
		else
		{
			//This is a console command, CryLogAlways usage is correct
			CryLogAlways("Usage: gl_GameRules <substr of gamerules>");
		}
	}
}

//-------------------------------------------------------------------------
void CGameLobby::ChangeGameRules( const char *pGameRulesName )
{
	CryFixedStringT<32> validGameRules = CGameLobby::GetValidGameRules(pGameRulesName, false);

	if (validGameRules.empty()==false)
	{
		m_currentGameRules = validGameRules.c_str();
		m_taskQueue.AddTask(CLobbyTaskQueue::eST_Update, true);
		CryLog("Set GameRules '%s'", validGameRules.c_str());
		GameRulesChanged(validGameRules.c_str());
	}
}

//static-------------------------------------------------------------------------
void CGameLobby::CmdVote(IConsoleCmdArgs* pCmdArgs)
{
	CryLog("CGameLobby::CmdVote()");

	if (CGameLobby* gl=g_pGame->GetGameLobby())
	{
		if (gl->m_state == eLS_Lobby)
		{
			if (gl->m_votingEnabled)
			{
				if (gl->m_localVoteStatus == eLVS_notVoted)
				{
					bool  showUsage = true;

					if (pCmdArgs->GetArgCount() == 2)
					{
						if (const char* voteArg=pCmdArgs->GetArg(1))
						{
							if (!stricmp("left", voteArg))
							{
								gl->SetLocalVoteStatus(eLVS_votedLeft);
								VOTING_DBG_LOG("[tlh] set m_localVoteStatus [5] to eLVS_votedLeft");
							}
							else if (!stricmp("right", voteArg))
							{
								gl->SetLocalVoteStatus(eLVS_votedRight);
								VOTING_DBG_LOG("[tlh] set m_localVoteStatus [6] to eLVS_votedRight");
							}

							CryLog("gl_Vote: \"%s\" vote cast", voteArg);
						}
					}

					if (showUsage)
					{
						CryLog("Usage: gl_Vote <left/right>");
					}
				}
				else
				{
					CryLog("gl_Vote: can only vote once!");
				}
			}
			else
			{
				CryLog("gl_Vote: voting is not enabled for this lobby session!");
			}
		}
		else
		{
			CryLog("gl_Vote: lobby not in correct state for voting!");
		}
	}
}

//---------------------------------------
void CGameLobby::MatchmakingSessionRoomOwnerChangedCallback( UCryLobbyEventData eventData, void *userParam )
{
	ENSURE_ON_MAIN_THREAD;

	CGameLobby *lobby = static_cast<CGameLobby *>(userParam);
	SCryLobbyRoomOwnerChanged	*pRoomOwnerChangedData = eventData.pRoomOwnerChanged;

	if (lobby->m_currentSession == pRoomOwnerChangedData->m_session)
	{
		uint32 ip = pRoomOwnerChangedData->m_ip;
		uint16 port = pRoomOwnerChangedData->m_port;
		cry_sprintf(lobby->m_joinCommand, sizeof(lobby->m_joinCommand), "connect <session>%d,%d.%d.%d.%d:%d", pRoomOwnerChangedData->m_session, ((uint8*)&ip)[0], ((uint8*)&ip)[1], ((uint8*)&ip)[2], ((uint8*)&ip)[3], port);
		CryLog("CGameLobby::MatchmakingSessionRoomOwnerChangedCallback() setting join command to %s", lobby->m_joinCommand);
	}
	else
	{
		CryLog("CGameLobby::MatchmakingSessionRoomOwnerChangedCallback() on wrong session, ignoring");
	}
}


//---------------------------------------
void CGameLobby::MatchmakingSessionJoinUserCallback( UCryLobbyEventData eventData, void *userParam )
{
	ENSURE_ON_MAIN_THREAD;

	CGameLobby *lobby = static_cast<CGameLobby *>(userParam);
	if(eventData.pSessionUserData->session == lobby->m_currentSession)
	{
		lobby->InsertUser(&eventData.pSessionUserData->data);
	}
}

//---------------------------------------
void CGameLobby::MatchmakingSessionLeaveUserCallback( UCryLobbyEventData eventData, void *userParam )
{
	ENSURE_ON_MAIN_THREAD;

	CGameLobby *lobby = static_cast<CGameLobby *>(userParam);
	if(eventData.pSessionUserData->session == lobby->m_currentSession)
	{
		lobby->RemoveUser(&eventData.pSessionUserData->data);
	}
}

//---------------------------------------
void CGameLobby::MatchmakingSessionUpdateUserCallback( UCryLobbyEventData eventData, void *userParam )
{
	ENSURE_ON_MAIN_THREAD;

	CGameLobby *lobby = static_cast<CGameLobby *>(userParam);
	if(eventData.pSessionUserData->session == lobby->m_currentSession)
	{
		lobby->UpdateUser(&eventData.pSessionUserData->data);
	}
}

//---------------------------------------
void CGameLobby::MatchmakingLocalUserDataCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* arg)
{
	ENSURE_ON_MAIN_THREAD;

	CGameLobby *pGameLobby = static_cast<CGameLobby*>(arg);
	pGameLobby->NetworkCallbackReceived(taskID, error);
}

//---------------------------------------
bool CGameLobby::ComparePlayerGroups(SPlayerGroup *lhs, SPlayerGroup *rhs)
{
	if (lhs->m_numMembers > rhs->m_numMembers)
	{
		return false;
	}
	else if (lhs->m_numMembers < rhs->m_numMembers)
	{
		return true;
	}
	else if (lhs->m_totalPrevScore != rhs->m_totalPrevScore)
	{
		// If member count is the same, compare on previous score 
		return (lhs->m_totalPrevScore < rhs->m_totalPrevScore);
	}
	else
	{
		// If previous score is the same, compare on skill 
		return (lhs->m_totalSkill < rhs->m_totalSkill);
	}
}

//---------------------------------------
void CGameLobby::BalanceTeams()
{
	TEAM_BALANCING_LOG("CGameLobby::BalanceTeams beginning");

	bool bChanged = false;

	typedef std::vector<SPlayerGroup*> TGroupsVec;
	TGroupsVec remainingGroups;
	remainingGroups.reserve(MAX_PLAYER_GROUPS);

	TEAM_BALANCING_LOG("    init");

	int numTotalPlayers = 0;
	for (int groupIdx = 0; groupIdx < MAX_PLAYER_GROUPS; ++ groupIdx)
	{
		SPlayerGroup *pPlayerGroup = &m_playerGroups[groupIdx];
		if (pPlayerGroup->m_valid)
		{
			remainingGroups.push_back(pPlayerGroup);
			numTotalPlayers += pPlayerGroup->m_numMembers;
			// Calculate the total previous score
			uint32 totalPrevScore = 0;
			for (int i = 0; i<pPlayerGroup->m_numMembers; ++i)
			{
				totalPrevScore += GetPrevScore(&pPlayerGroup->m_members[i]);
			}
			pPlayerGroup->m_totalPrevScore = totalPrevScore;
			TEAM_BALANCING_LOG("        found group, type=%i, numMembers=%i, totalSkill=%u, totalPrevScore=%u", int(pPlayerGroup->m_type), pPlayerGroup->m_numMembers, pPlayerGroup->m_totalSkill, pPlayerGroup->m_totalPrevScore);
		}
	}

	// Calculate number of players allowed on a team
	int maxPlayersPerTeam = numTotalPlayers / 2;
	if ((numTotalPlayers % 2) == 1)
	{
		++ maxPlayersPerTeam;
	}
	TEAM_BALANCING_LOG("        total players=%i, maxPerTeam=%i", numTotalPlayers, maxPlayersPerTeam);

	std::sort(remainingGroups.begin(), remainingGroups.end(), ComparePlayerGroups);

	int teamMembers[2] = {0};
	uint32 teamScore[2] = {0};

	TEAM_BALANCING_LOG("    balancing");
	while (remainingGroups.size())
	{
		SPlayerGroup *pCurrentGroup = *remainingGroups.rbegin();
		TEAM_BALANCING_LOG("        group type=%i, members=%i, skill=%u, score=%u", int(pCurrentGroup->m_type), pCurrentGroup->m_numMembers, pCurrentGroup->m_totalSkill, pCurrentGroup->m_totalPrevScore);

		const int numMembersInGroup = pCurrentGroup->m_numMembers;
		uint8 teamIdxToUse = 0;
		// Use the team which has the lowest total score, unless this group won't fit entirely on that team
		if (teamScore[0] > teamScore[1])
		{
			teamIdxToUse = 1;
		}
		TEAM_BALANCING_LOG("        team %i has lower score", (teamIdxToUse + 1));
		if ((teamMembers[teamIdxToUse] + numMembersInGroup > maxPlayersPerTeam))
		{
			TEAM_BALANCING_LOG("        group does not fit within team %i, switching", (teamIdxToUse + 1));
			teamIdxToUse = 1 - teamIdxToUse;
		}
		const uint8 teamId = (teamIdxToUse + 1);

		if (teamMembers[teamIdxToUse] + numMembersInGroup <= maxPlayersPerTeam)
		{
			TEAM_BALANCING_LOG("            space is available, assigning team");
			teamMembers[teamIdxToUse] += numMembersInGroup;
			teamScore[teamIdxToUse] += pCurrentGroup->m_totalPrevScore;
			for (int playerIdx = 0; playerIdx < numMembersInGroup; ++ playerIdx)
			{
				SSessionNames::SSessionName *pMember = m_nameList.GetSessionName(pCurrentGroup->m_members[playerIdx], false);
				
				if (pMember != NULL && pMember->m_teamId != teamId)
				{
					pMember->m_teamId = teamId;
					bChanged = true;
				}
			}
		}
		else
		{
			// Need to split group :-(
			const int membersForFirstTeam = (maxPlayersPerTeam - teamMembers[teamIdxToUse]);
			const int membersForSecondTeam = (numMembersInGroup - membersForFirstTeam);
			const uint8 secondTeamId = (3 - teamId);
			TEAM_BALANCING_LOG("            not enough space available, splitting squad, %i members for first team, %i for second", membersForFirstTeam, membersForSecondTeam);

			for (int playerIdx = 0; playerIdx < numMembersInGroup; ++ playerIdx)
			{
				SSessionNames::SSessionName *pMember = m_nameList.GetSessionName(pCurrentGroup->m_members[playerIdx], false);

				if (pMember)
				{
					uint8 teamIdToUse;
					if (playerIdx < membersForFirstTeam)
					{
						teamIdToUse = teamId;
					}
					else
					{
						teamIdToUse = secondTeamId;
					}
					uint16 prevScore = GetPrevScore(&pMember->m_conId);
					teamScore[(teamIdToUse - 1)] += prevScore;
					if (pMember->m_teamId != teamIdToUse)
					{
						pMember->m_teamId = teamIdToUse;
						bChanged = true;
					}
				}
			}

			teamMembers[teamIdxToUse] += membersForFirstTeam;
			teamMembers[1 - teamIdxToUse] += membersForSecondTeam;
		}

		TEAM_BALANCING_LOG("        team 1 has %i members (total score=%i), team 2 has %i (total score=%i)", teamMembers[0], teamScore[0], teamMembers[1], teamScore[1]);
		remainingGroups.pop_back();
	}

	CRY_ASSERT((teamMembers[0] + teamMembers[1]) == numTotalPlayers);
	CRY_ASSERT(abs(teamMembers[0] - teamMembers[1]) <= 1);

	TEAM_BALANCING_LOG(" ");
#if DEBUG_TEAM_BALANCING
	SSessionNames::SSessionName *pResult = NULL;

	SSessionNames *pNames = &m_nameList;
	const int numPlayers = pNames->Size();
	CryFixedStringT<DISPLAY_NAME_LENGTH> playerName;
	for (uint8 teamId = 1; teamId < 3; ++ teamId)
	{
		TEAM_BALANCING_LOG("    Team %i", teamId);
		for (int i = 0; i < numPlayers; ++ i)
		{
			SSessionNames::SSessionName *pPlayer = &pNames->m_sessionNames[i];
			if (pPlayer->m_teamId == teamId)
			{
				pPlayer->GetDisplayName(playerName);
				TEAM_BALANCING_LOG("        %s   (score %i)", playerName.c_str(), GetPrevScore(&pPlayer->m_conId));
			}
		}
	}
#endif

	if (bChanged)
	{
		TEAM_BALANCING_LOG("    at least 1 team has changed, updating flash and clients");
		m_nameList.m_dirty = true;
		SendPacket(eGUPD_SetTeam);
	}
}

//---------------------------------------
void CGameLobby::GameRulesChanged( const char *pGameRules )
{
#ifdef GAME_IS_CRYSIS2
	CGameRulesModulesManager *pGameRulesModulesManager = CGameRulesModulesManager::GetInstance();

	m_hasValidGameRules = pGameRulesModulesManager->IsValidGameRules(pGameRules);
	if(m_hasValidGameRules)
	{
		bool isTeamGame = pGameRulesModulesManager->IsTeamGame(pGameRules);
		if (m_isTeamGame != isTeamGame)
		{
			m_isTeamGame = isTeamGame;
			if (m_isTeamGame)
			{
				if (m_nameList.Size())
				{
					m_needsTeamBalancing = true;
				}
			}
			else	//not team game
			{
				CryLog("    GameRulesChanged::ummuting all players Lobby: %p", this);
				MutePlayersOnTeam(0, false);
				MutePlayersOnTeam(1, false);
				MutePlayersOnTeam(2, false);

			}
		}

		ICVar *pCVar = gEnv->pConsole->GetCVar("sv_gamerules");
		if (pCVar)
		{
			pCVar->Set(pGameRules);
		}

		m_currentGameRules = pGameRules;
	}
#else

	if (gEnv->pGameFramework->GetIGameRulesSystem()->HaveGameRules(pGameRules))
	{
		ICVar *pCVar = gEnv->pConsole->GetCVar("sv_gamerules");
		if (pCVar)
		{
			pCVar->Set(pGameRules);
		}

		m_currentGameRules = pGameRules;
	}

#endif
	CryLog("[CG] CGameLobby::GameRulesChanged() newRules=%s, hasValidRules=%i, isTeamGame=%i Lobby: %p", pGameRules, m_hasValidGameRules?1:0, m_isTeamGame?1:0, this);
}


#ifdef GAME_IS_CRYSIS2
//------------------------------------------------------------------------
void CGameLobby::GetDogtagInfoByChannel(int channelId, CDogtag::STagInfo &dogtagInfo)
{
	CrySessionHandle hdl = m_currentSession;

	if(hdl != CrySessionInvalidHandle)
	{
		SCryMatchMakingConnectionUID temp = gEnv->pNetwork->GetLobby()->GetMatchMaking()->GetConnectionUIDFromGameSessionHandleAndChannelID(hdl, channelId);
		SSessionNames::SSessionName *pPlayer = m_nameList.GetSessionName(temp, true);
		if (pPlayer)
		{
			GetDogtagInfoBySessionName(pPlayer, dogtagInfo);
		}
		else
		{
			CryLog("CGameLobby::GetDogtagInfoByChannel() failed to find player for channelId %i Lobby: %p", channelId, this);

#ifndef _RELEASE
			if (gl_dummyUserlist)
			{
				sprintf(dogtagInfo.m_ownerName, "Channel%d", channelId);
				CDogtag::SStyle::EEnum style = (CDogtag::SStyle::EEnum)(cry_rand()%(int)CDogtag::SStyle::NUM);
				dogtagInfo.m_currentSkin = cry_rand()%10;
				dogtagInfo.m_currentSkinStyle = style;
			}
#endif
		}
	}
}

//---------------------------------------
void CGameLobby::GetDogtagInfoBySessionName(SSessionNames::SSessionName *pSessionName, CDogtag::STagInfo &dogtagInfo)
{
	if (pSessionName)
	{
		dogtagInfo.m_rank = pSessionName->m_userData[eLUD_Rank];
		uint8 styleAndSpecial = pSessionName->m_userData[eLUD_DogtagStyle];
		dogtagInfo.m_currentSkinStyle = (styleAndSpecial & 0x0F);
		dogtagInfo.m_special = styleAndSpecial >> 4;
		dogtagInfo.m_currentSkin = pSessionName->m_userData[eLUD_DogtagId];
		dogtagInfo.m_reincarnations = pSessionName->GetReincarnations();

		IPlayerProfileManager *pPlayerProfileManager = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager();
		int userIndex = pPlayerProfileManager ? pPlayerProfileManager->GetExclusiveControllerDeviceIndex() : 0;

		CryUserID localUser = gEnv->pNetwork->GetLobby()->GetLobbyService()->GetUserID(userIndex);
		if (localUser.IsValid())
		{
			if (localUser == pSessionName->m_userId)
			{
				dogtagInfo.m_localPlayer = true;
			}
		}

		CryFixedStringT<CLAN_TAG_LENGTH> clanTag;

		pSessionName->GetClanTagName(clanTag);

		cry_strncpy(dogtagInfo.m_ownerName, pSessionName->m_name, sizeof(dogtagInfo.m_ownerName));
		cry_strncpy(dogtagInfo.m_clanName, clanTag.c_str(), sizeof(dogtagInfo.m_clanName));
	}
}
#endif

void CGameLobby::GetPlayerNameFromChannelId(int channelId, CryFixedStringT<CRYLOBBY_USER_NAME_LENGTH> &name)
{
	SCryMatchMakingConnectionUID temp = gEnv->pNetwork->GetLobby()->GetMatchMaking()->GetConnectionUIDFromGameSessionHandleAndChannelID(m_currentSession, channelId);
	SSessionNames::SSessionName *pPlayer = m_nameList.GetSessionName(temp, true);
	
	if (pPlayer)
	{
		name = pPlayer->m_name;
	}
}

void CGameLobby::GetClanTagFromChannelId(int channelId, CryFixedStringT<CLAN_TAG_LENGTH> &name)
{
	SCryMatchMakingConnectionUID temp = gEnv->pNetwork->GetLobby()->GetMatchMaking()->GetConnectionUIDFromGameSessionHandleAndChannelID(m_currentSession, channelId);
	SSessionNames::SSessionName *pPlayer = m_nameList.GetSessionName(temp, true);
	if (pPlayer)
	{
		pPlayer->GetClanTagName(name);
	}
}

void CGameLobby::LocalUserDataUpdated()
{
	if (IsCurrentlyInSession())
	{
		m_taskQueue.AddTask(CLobbyTaskQueue::eST_SetLocalUserData, true);
	}
}

//---------------------------------------
CryUserID CGameLobby::GetLocalUserId( void )
{
	SSessionNames& snames = m_nameList;
	if( snames.Size() )
	{
		return snames.m_sessionNames[0].m_userId;
	}

	return NULL;
}


//---------------------------------------
void CGameLobby::GetLocalUserDisplayName(CryFixedStringT<DISPLAY_NAME_LENGTH> &displayName)
{
#ifdef GAME_IS_CRYSIS2
#if !defined(_RELEASE)
	if (g_pGame->GetUI() && g_pGame->GetUI()->GetCVars()->menu_namesAsWs)
	{
		displayName = HUD_CVARS_NAMES_AS_WS_WITH_CLAN;
		return;
	}
#endif
#endif

	SSessionNames& snames = m_nameList;
	if( snames.Size() )
	{
		SSessionNames::SSessionName& sessionName = snames.m_sessionNames[0];
		sessionName.GetDisplayName(displayName);
	}
}

//---------------------------------------
void CGameLobby::GetPlayerDisplayNameFromEntity(EntityId entityId, CryFixedStringT<DISPLAY_NAME_LENGTH> &displayName)
{
#ifdef GAME_IS_CRYSIS2
#if !defined(_RELEASE)
	if (g_pGame->(GetUI)() && g_pGame->GetUI()->GetCVars()->menu_namesAsWs)
	{
		displayName = HUD_CVARS_NAMES_AS_WS_WITH_CLAN;
		return;
	}
#endif
#endif

	IGameObject	*pGameObject = gEnv->pGame->GetIGameFramework()->GetGameObject(entityId);
	GetPlayerDisplayNameFromChannelId(pGameObject ? pGameObject->GetChannelId() : 0, displayName);
}

//---------------------------------------
void CGameLobby::GetPlayerDisplayNameFromChannelId(int channelId, CryFixedStringT<DISPLAY_NAME_LENGTH> &displayName)
{
#ifdef GAME_IS_CRYSIS2
#if !defined(_RELEASE)
	if (g_pGame->GetUI() && g_pGame->GetUI()->GetCVars()->menu_namesAsWs)
	{
		displayName = HUD_CVARS_NAMES_AS_WS_WITH_CLAN;
		return;
	}
#endif
#endif

	CrySessionHandle mySession = m_currentSession;

	ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
	if (pLobby != NULL && mySession != CrySessionInvalidHandle)
	{
		ICryMatchMaking *pMatchmaking = pLobby->GetMatchMaking();
		if (pMatchmaking != NULL)
		{
			SCryMatchMakingConnectionUID conID = pMatchmaking->GetConnectionUIDFromGameSessionHandleAndChannelID(mySession, channelId);

			SSessionNames::SSessionName *pSessionName = m_nameList.GetSessionName(conID, false);
			if (pSessionName)
			{
				pSessionName->GetDisplayName(displayName);
			}
		}
	}
}

//---------------------------------------
bool CGameLobby::CanShowGamercard(CryUserID userId)
{











	return false;
}

//---------------------------------------
void CGameLobby::ShowGamercardByUserId(CryUserID userId) 
{




















}

//---------------------------------------
int CGameLobby::GetTeamByChannelId( int channelId )
{
	int teamId = 0;

	CrySessionHandle mySession = m_currentSession;

	if ((g_pGameCVars->g_autoAssignTeams != 0) && (m_isTeamGame == true) && (mySession != CrySessionInvalidHandle))
	{
		// For now just assign teams alternately
		ICryMatchMaking* pMatchMaking = NULL;
		if (gEnv->pNetwork)
		{
			ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
			if (pLobby)
			{
				pMatchMaking = pLobby->GetMatchMaking();
				if (pMatchMaking)
				{
					SCryMatchMakingConnectionUID temp = pMatchMaking->GetConnectionUIDFromGameSessionHandleAndChannelID(mySession, channelId);
					int playerIndex = m_nameList.Find(temp);
					if (playerIndex != SSessionNames::k_unableToFind)
					{
						teamId = int(m_nameList.m_sessionNames[playerIndex].m_teamId);
					}
				}
			}
		}
	}

	return teamId;
}

//---------------------------------------
SCryMatchMakingConnectionUID CGameLobby::GetConnectionUIDFromChannelID(int channelId)
{
	SCryMatchMakingConnectionUID temp;

	CrySessionHandle mySession = m_currentSession;

	ICryMatchMaking* pMatchMaking = NULL;
	if (gEnv->pNetwork && mySession != CrySessionInvalidHandle)
	{
		ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
		if (pLobby)
		{
			pMatchMaking = pLobby->GetMatchMaking();
			if (pMatchMaking)
			{
				temp = pMatchMaking->GetConnectionUIDFromGameSessionHandleAndChannelID(mySession, channelId);
			}
		}
	}

	return temp;
}

//---------------------------------------
void CGameLobby::UpdatePrivatePasswordedGame()
{
	assert(gEnv->IsDedicated());

	bool privateGame = false;
	bool passworded = false;
	ICVar *pCVar = gEnv->pConsole->GetCVar("sv_password");
	if (pCVar)
	{
		const char* password = pCVar->GetString();
		if(strcmp(password, "") != 0)
		{
			privateGame = true;
			passworded = true;
		}
	}

	SetPasswordedGame(passworded);
	SetPrivateGame(privateGame);
}

//---------------------------------------
void CGameLobby::FindGameEnter()
{
	m_shouldFindGames = true;
#ifdef GAME_IS_CRYSIS2
	CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
#if USE_DEDICATED_LEVELROTATION
	if (gEnv->IsDedicated())
	{
		// Don't try to merge if we're using a custom rotation
		m_shouldFindGames = !pPlaylistManager->IsUsingCustomRotation();
	}
#endif
#endif

	SetMatchmakingGame(true);

	if (gEnv->IsDedicated())
	{
		UpdatePrivatePasswordedGame();
	}
	else
	{
		SetPrivateGame(false); // Can't matchmake private games.
	}

#ifdef GAME_IS_CRYSIS2
	ILevelRotation *pLevelRotation = pPlaylistManager->GetLevelRotation();
	CRY_ASSERT(pLevelRotation->GetLength() > 0);

	if (pLevelRotation->GetLength() > 0)
	{
		const char *nextLevel = pLevelRotation->GetNextLevel();
		const char *nextGameRules = pLevelRotation->GetNextGameRules();

		//Set params prematurely for the lobby so find game screen looks acceptable
		m_userData[eLDI_Map].m_int32 = GameLobbyData::ConvertMapToHash(nextLevel);
		m_userData[eLDI_Gamemode].m_int32 = GameLobbyData::ConvertGameRulesToHash(nextGameRules);

		UpdateRulesAndLevel(nextGameRules, nextLevel);

		//Technically we should let the new system decide if it cares about level rotations
		//look to move this out of here, and rename this function to "prepare screen for search" or something

		if( gEnv->IsDedicated() == false )
		{
			if( CMatchMakingHandler* pMMhandler = m_gameLobbyMgr->GetMatchMakingHandler() )
			{
				pMMhandler->OnEnterMatchMaking();
			}
		}
		else
		{
			CryLog("CGameLobby::FindGameEnter() callingFindGameCreateGame() Lobby: %p", this);
			FindGameCreateGame();
		}

	}
	else
#endif
	{
		CryLog("CGameLobby::FindGameEnter() ERROR: Current rotation has no levels in it");
	}
}




//---------------------------------------
bool CGameLobby::MergeToServer( CrySessionID sessionId )
{
	if( sessionId == CrySessionInvalidID )
	{
		return false;
	}
	return m_gameLobbyMgr->NewSessionRequest(this, sessionId);
}


//---------------------------------------
void CGameLobby::FindGameMoveSession(CrySessionID sessionId)
{
	//means networking has ok-ed the start of the merge and we should complete it
	//think we can leave this in the lobby
	CRY_ASSERT(sessionId != CrySessionInvalidID);
	m_nextSessionId = sessionId;

	const int numPlayers = m_nameList.Size();

	CryLog("CGameLobby::FindGameMoveSession() numPlayers=%i", numPlayers);

	SendPacket(eGUPD_LobbyMoveSession);

	if (gEnv->IsDedicated())
	{
		m_gameLobbyMgr->CancelMoveSession(this);
	}
	else
	{
		// Everyone else has to leave before us or we end up in host migration horribleness
		if (numPlayers > 1)
		{
			m_gameLobbyMgr->MoveUsers(this);
			m_allowRemoveUsers = false;	// not allowed to remove users, we need them to stay until we switch lobbies

			CryLog("  CGameLobby::FindGameMoveSession can't leave yet");
			// Start from 1 since we're always the first person in the list
			for (int i = 1; i < numPlayers; ++ i)
			{
				SSessionNames::SSessionName &player = m_nameList.m_sessionNames[i];
				player.m_bMustLeaveBeforeServer = true;
			}
			m_isLeaving = true;
			m_leaveGameTimeout = gl_leaveGameTimeout;

#if defined (TRACK_MATCHMAKING)
			//we're asking our clients to merge to a different session, log this
			if( CMatchmakingTelemetry* pMMTel = g_pGame->GetMatchMakingTelemetry() )
			{
				pMMTel->AddEvent( SMMServerRequestingMerge( m_currentSessionId, sessionId ) );
			}
#endif //defined (TRACK_MATCHMAKING)
		}
		else
		{
			CryLog("  CGameLobby::FindGameMoveSession leaving now");
			LeaveSession(true);		//end current session (when callback is finished should join nextSession)
		}
	}
}


//---------------------------------------
bool CGameLobby::BidirectionalMergingRequiresCancel(CrySessionID other)
{
	//-- if we are in a merging state, test the CrySessionID of the session attempting to merge into us
	CGameLobby* pNextLobby = m_gameLobbyMgr->GetNextGameLobby();

	CrySessionID sessionIdToCheck = CrySessionInvalidID;

	if (pNextLobby)
	{
		sessionIdToCheck = pNextLobby->m_currentSessionId;
		if (!sessionIdToCheck)
		{
			sessionIdToCheck = pNextLobby->m_pendingConnectSessionId;
		}
	}

	if (sessionIdToCheck && (NetworkUtils_CompareCrySessionId(sessionIdToCheck, other)))
	{
		CryLog("[LOBBYMERGE] Bidirectional Lobby merge detected!");

		//-- We have a bidirectional merge in progress. 
		//-- In this case the CURRENT session with the "lowest" ID must cancel its merge,
		//-- which means the higher sessionID players always join the lower sessionID session.

		if (*m_currentSessionId < *other)
		{
			CryLog("[LOBBYMERGE] Aborting merge at this end.");
			return true;
		}

		CryLog("[LOBBYMERGE] Continuing merge at this end.");
		return false;
	}

	CryLog("[LOBBYMERGE] Aborting merge, not sure what state we're in. (pNextLobby=%p, next->id=%p, other=%p)", pNextLobby, pNextLobby ? pNextLobby->m_currentSessionId.get() : NULL, other.get());
	return true;
}

//---------------------------------------
void CGameLobby::FindGameCancelMove()
{
	//can stay in lobby, may need to inform matchmaking handler
	CryLog("CGameLobby::FindGameCancelMove() lobby=%p", this);
	if (m_gameLobbyMgr->IsPrimarySession(this) && !m_gameLobbyMgr->IsMergingComplete())
	{
		m_findGameTimeout = GetFindGameTimeout();	//timeout to search again
		m_gameLobbyMgr->CancelMoveSession(this);

		m_allowRemoveUsers = true;

		// Canceled the move, need to start searching again
		CGameLobby::s_bShouldBeSearching = true;

		CGameBrowser *pGameBrowser = g_pGame->GetGameBrowser();
		if (pGameBrowser)
		{
			pGameBrowser->CancelSearching();
		}
	}
}

//---------------------------------------
float CGameLobby::GetFindGameTimeout()
{
	//not part of the lobbies anymore, but may be a useful utility
	float randf = (((float) (g_pGame->GetRandomNumber() % 1000))/1000.0f);

	uint32 numPlayers = m_nameList.Size();

	return gl_findGameTimeoutBase + (gl_findGameTimeoutPerPlayer * numPlayers) + (randf * gl_findGameTimeoutRandomRange);
}

//---------------------------------------
void CGameLobby::MakeReservations(SSessionNames* nameList, bool squadReservation)
{
	CRY_ASSERT(nameList->Size() > 0);
	CryLog("CGameLobby::MakeReservations() lobby:%p, nameListSize=%u, m_server=%s, squadReservation=%s", this, nameList->Size(), m_server ? "true" : "false", squadReservation ? "true" : "false");
	if (m_server)
	{
		SCryMatchMakingConnectionUID  reservationRequests[MAX_RESERVATIONS];
		int numReservationsToRequest = BuildReservationsRequestList(reservationRequests, ARRAY_COUNT(reservationRequests), nameList);

		const EReservationResult result = DoReservations(numReservationsToRequest, reservationRequests);
		if (squadReservation)
		{
			g_pGame->GetSquadManager()->ReservationsFinished(result);
		}
	}
	else
	{
		m_reservationList = nameList;
		m_squadReservation = squadReservation;

		SendPacket(eGUPD_ReservationRequest);
	}
}

//---------------------------------------
int CGameLobby::BuildReservationsRequestList(SCryMatchMakingConnectionUID reservationRequests[], const int maxRequests, const SSessionNames*  members)
{
	const int  numMembers = members->Size();
	CRY_ASSERT(numMembers > 0);

	int  numReservationsToRequest = (numMembers - 1);
	CRY_ASSERT(numReservationsToRequest <= maxRequests);
	numReservationsToRequest = MIN(numReservationsToRequest, maxRequests);

	CryLog("[tlh]   reservations needed:");
	for (int i=0; i<numReservationsToRequest; i++)
	{
		CRY_ASSERT((i + 1) < (int)members->m_sessionNames.size());
		reservationRequests[i] = members->m_sessionNames[i + 1].m_conId;  // NOTE assuming that the local user's connection id is always first in the list (hence [i + 1], as local user is not needed for reservations)

		CryLog("[tlh]     %02d: {%llu,%d}", (i + 1), reservationRequests[i].m_sid, reservationRequests[i].m_uid);
	}

#if SQUADMGR_DBG_ADD_FAKE_RESERVATION
	if ((numReservationsToRequest + 1) <= maxRequests)
	{
		SCryMatchMakingConnectionUID  fake = members->m_sessionNames[0].m_conId;
		fake.m_uid = 666;
		CryLog("[tlh]     ADDING FAKE RESERVATION! %02d: {%llu,%d}", (numReservationsToRequest + 1), fake.m_sid, fake.m_uid);
		reservationRequests[numReservationsToRequest] = fake;
		numReservationsToRequest++;
	}
#endif

	return numReservationsToRequest;
}

//---------------------------------------
EReservationResult CGameLobby::DoReservations(const int numReservationsRequested, const SCryMatchMakingConnectionUID requestedReservations[])
{
	EReservationResult  result = eRR_NoneNeeded;

	CGameLobby*  lobby = g_pGame->GetGameLobby();

	CRY_ASSERT(m_server);

	CRY_ASSERT(numReservationsRequested <= MAX_RESERVATIONS);
	CryLog("CGameLobby::DoReservations() numReservationsRequested=%d", numReservationsRequested);

	const float  timeNow = gEnv->pTimer->GetAsyncCurTime();

	CryLog("  counting (and refreshing) reservations:");
	int  reservedCount = 0;
	for (int i=0; i<ARRAY_COUNT(m_slotReservations); i++)
	{
		SSlotReservation*  res = &m_slotReservations[i];
		CryLog("  %02d: con = {%llu,%d}, stamp = %f", (i + 1), res->m_con.m_sid, res->m_con.m_uid, res->m_timeStamp);

		if (res->m_con != CryMatchMakingInvalidConnectionUID)
		{
			if ((timeNow - res->m_timeStamp) > gl_slotReservationTimeout)
			{
				CryLog("    slot has expired, so invalidating it");
				res->m_con = CryMatchMakingInvalidConnectionUID;
			}
			else
			{
				reservedCount++;
			}
		}
	}

	const int  numPrivate = lobby->GetNumPrivateSlots();
	const int  numPublic = lobby->GetNumPublicSlots();
	const int  numFilledExc = MAX(0, (lobby->GetSessionNames().Size() - 1));  // NOTE -1 because client in question will be in list already but we don't want them to be included in the calculations
	const int  numEmptyExc = ((numPrivate + numPublic) - numFilledExc);

	CryLog("  nums private = %d, public = %d, filled (exc. leader) = %d, empty (exc. leader) = %d, reserved = %d", numPrivate, numPublic, numFilledExc, numEmptyExc, reservedCount);

	if ((numReservationsRequested + 1) <= (numEmptyExc - reservedCount))  // NOTE the +1 is for the leader 
	{
		if (numReservationsRequested > 0)
		{
			CryLog("    got enough space left, processing requested reservations:");
			int  numDone = 0;
			for (int i=0; i<ARRAY_COUNT(m_slotReservations); i++)
			{
				SSlotReservation*  res = &m_slotReservations[i];
				CryLog("       (existing) %02d: con = {%llu,%d}, stamp = %f", (i + 1), res->m_con.m_sid, res->m_con.m_uid, res->m_timeStamp);

				if (res->m_con == CryMatchMakingInvalidConnectionUID)
				{
					const SCryMatchMakingConnectionUID*  req = &requestedReservations[numDone];
					CryLog("        (new) %02d: {%llu,%d}, setting at index %d", (numDone + 1), req->m_sid, req->m_uid, i);

					res->m_con = (*req);
					res->m_timeStamp = timeNow;
					numDone++;

					if (numDone >= numReservationsRequested)
					{
						break;
					}
				}
			}
			result = eRR_Success;
		}
	}
	else
	{
		result = eRR_Fail;
	}

	return result;
}

//---------------------------------------
void CGameLobby::FindGameCreateGame()
{
	//not sure if this remains responsibility of lobby
#ifndef _RELEASE
	if (gl_debugLobbyRejoin)
	{
		++ m_failedSearchCount;
		CryLog("CGameLobby::FindGameCreateGame() didn't find a game, counter = %i", m_failedSearchCount);
		FindGameEnter();
		return;
	}
#endif

	CRY_ASSERT_MESSAGE( CMatchMakingHandler::AllowedToCreateGame(), "[GameLobby] Trying to create a session when we're not allowed to!");

	m_taskQueue.AddTask(CLobbyTaskQueue::eST_Create, true);
}


//---------------------------------------
CGameLobby::SPlayerGroup *CGameLobby::FindGroupByConnectionUID( const SCryMatchMakingConnectionUID *pConId )
{
	for (int groupIdx = 0; groupIdx < MAX_PLAYER_GROUPS; ++ groupIdx)
	{
		SPlayerGroup *pPlayerGroup = &m_playerGroups[groupIdx];
		if (pPlayerGroup->m_valid)
		{
			const int numPlayers = pPlayerGroup->m_members.size();
			for (int playerIdx = 0; playerIdx < numPlayers; ++ playerIdx)
			{
				if (pPlayerGroup->m_members[playerIdx].m_uid == pConId->m_uid)
				{
					return pPlayerGroup;
				}
			}
		}
	}
	return NULL;
}

//---------------------------------------
CGameLobby::SPlayerGroup * CGameLobby::FindGroupByClan( const char *pClanName )
{
	CGameLobby::SPlayerGroup *pResult = NULL;

	for (int groupIdx = 0; groupIdx < MAX_PLAYER_GROUPS; ++ groupIdx)
	{
		SPlayerGroup *pPlayerGroup = &m_playerGroups[groupIdx];
		if (pPlayerGroup->m_valid && (pPlayerGroup->m_type == ePGT_Clan))
		{
			if (!stricmp(pPlayerGroup->m_clanTag.c_str(), pClanName))
			{
				pResult = pPlayerGroup;
				break;
			}
		}
	}

	return pResult;
}

//---------------------------------------
CGameLobby::SPlayerGroup *CGameLobby::FindEmptyGroup()
{
	SPlayerGroup *pResult = NULL;

	for (int groupIdx = 0; groupIdx < MAX_PLAYER_GROUPS; ++ groupIdx)
	{
		SPlayerGroup *pPlayerGroup = &m_playerGroups[groupIdx];
		if (!pPlayerGroup->m_valid)
		{
			pResult = pPlayerGroup;
			break;
		}
	}

	CRY_ASSERT_MESSAGE(pResult, "Should always be an empty group when needed");
#ifndef _RELEASE
	if (!pResult)
	{
		CryLog("CGameLobby::FindEmptyGroup() - failed to find an empty group Lobby: %p", this);
	}
#endif
	return pResult;
}

//---------------------------------------
uint8 CGameLobby::GetRank( const SCryMatchMakingConnectionUID *pConId )
{
	int playerIndex = m_nameList.Find(*pConId);
	if (playerIndex != SSessionNames::k_unableToFind)
	{
		return m_nameList.m_sessionNames[playerIndex].m_userData[eLUD_Rank];
	}
	CRY_ASSERT_MESSAGE(false, "[CG] Failed to find rank from SCryMatchMakingConnectionUID");
	return 0;
}

//---------------------------------------
uint16 CGameLobby::GetSkill( const SCryMatchMakingConnectionUID *pConId )
{
	int playerIndex = m_nameList.Find(*pConId);
	if (playerIndex != SSessionNames::k_unableToFind)
	{
		return m_nameList.m_sessionNames[playerIndex].GetSkillRank();
	}
	CRY_ASSERT_MESSAGE(false, "[CG] Failed to find skill rank from SCryMatchMakingConnectionUID");
	return 0;
}

//---------------------------------------
uint16 CGameLobby::GetPrevScore( const SCryMatchMakingConnectionUID *pConId )
{
	uint32 totalPrevScore = 0;
	uint32 prevNumPlayers = 0;
	TPlayerScoresList::iterator itPrevScore;
	for (itPrevScore = m_previousGameScores.begin(); itPrevScore != m_previousGameScores.end(); ++itPrevScore)
	{
		if (itPrevScore->m_fracTimeInGame > 0.75f)
		{
			// Only consider players who played for more than 75% of the game
			prevNumPlayers++;
			totalPrevScore += itPrevScore->m_score;
			if (itPrevScore->m_playerId == *pConId)
			{
				// Found the player score, return it
				return itPrevScore->m_score;
			}
		}
	}
	// If we reach here then the player either didn't play the previous game or not for long enough
	// Predict what score he is expected to get based on his skill level
	float averageScore = 100.f;	// If nobody played a previous game then just set an average score of 100
	if (prevNumPlayers > 0 && totalPrevScore > 0)
	{
		averageScore = (float)totalPrevScore / (float)prevNumPlayers;
	}
	uint16 skill = GetSkill(pConId);
	int32 averageSkill = CalculateAverageSkill();
	float frac = 0;
	if (averageSkill > 0)
	{
		frac = (float)skill / (float)averageSkill;
	}
	return (uint16)(averageScore * frac);
}

//---------------------------------------
bool CGameLobby::GetConnectionUIDFromUserID( const CryUserID userId, SCryMatchMakingConnectionUID &result )
{
	CRY_ASSERT(userId.IsValid());
	if (userId.IsValid())
	{
		SSessionNames::SSessionName *pPlayer = m_nameList.GetSessionNameByUserId(userId);
		if (pPlayer)
		{
			result = pPlayer->m_conId;
			return true;
		}
	}
	return false;
}

//---------------------------------------
bool CGameLobby::GetUserIDFromConnectionUID( const SCryMatchMakingConnectionUID conId, CryUserID &result )
{
	if (conId != CryMatchMakingInvalidConnectionUID)
	{
		SSessionNames::SSessionName *pPlayer = m_nameList.GetSessionName(conId, true);
		if (pPlayer)
		{
			result = pPlayer->m_userId;
			return true;
		}
	}
	return false;
}

//---------------------------------------
CryFixedStringT<CRYLOBBY_USER_GUID_STRING_LENGTH> CGameLobby::GetGUIDFromActorID(EntityId actorId)
{
	IActor* pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(actorId);
	if(pActor)
	{
		CryUserID userId = GetUserIDFromChannelID(pActor->GetChannelId());
		if(userId.IsValid())
		{
			return userId.get()->GetGUIDAsString();
		}
	}

	return "";
}

//---------------------------------------
CryUserID CGameLobby::GetUserIDFromChannelID(int channelId)
{
	CryUserID userId = CryUserInvalidID;

	if (gEnv->pNetwork && gEnv->pNetwork->GetLobby())
	{
		if (ICryMatchMaking* pMatchMaking = gEnv->pNetwork->GetLobby()->GetMatchMaking())
		{
			CrySessionHandle sessionHandle = m_currentSession;
			if (sessionHandle != CrySessionInvalidHandle)
			{
				SCryMatchMakingConnectionUID conId = pMatchMaking->GetConnectionUIDFromGameSessionHandleAndChannelID(sessionHandle, channelId);
				GetUserIDFromConnectionUID(conId, userId);
			}
		}
	}

	return userId;
}

//---------------------------------------
void CGameLobby::MutePlayerByChannelId( int channelId, bool mute, int reason )
{
	CryUserID playerId = GetUserIDFromChannelID(channelId);
	SSessionNames::SSessionName *pPlayer = m_nameList.GetSessionNameByUserId(playerId);
	if (pPlayer)
	{
		MutePlayerBySessionName(pPlayer, mute, reason);
	}
	else
	{
		CryLog("[CG] CGameLobby::MutePlayerByChannelId() failed to find player for channelId %i Lobby: %p", channelId, this);
	}
}

//---------------------------------------
void CGameLobby::MutePlayerBySessionName( SSessionNames::SSessionName *pUser, bool mute, int reason )
{
	if (pUser && pUser->m_userId.IsValid())
	{
		ICryVoice *pCryVoice = gEnv->pNetwork->GetLobby()->GetVoice();
		uint32 userIndex = GameLobby_GetCurrentUserIndex();
		if (pCryVoice)
		{
			if (reason & SSessionNames::SSessionName::MUTE_REASON_MANUAL)
			{
				pCryVoice->MuteExternally(userIndex, pUser->m_userId, mute);
				reason &= ~SSessionNames::SSessionName::MUTE_REASON_MANUAL;
			}

			bool isMuted = (pUser->m_muted != 0);
			if (mute)
			{
				pUser->m_muted |= reason;
			}
			else
			{
				pUser->m_muted &= ~reason;
			}
			bool shouldBeMuted = (pUser->m_muted != 0);
			//if (isMuted != shouldBeMuted)
			{
				CryLog("[CG] CGameLobby::MutePlayerBySessionName() Setting player '%s' to %s Lobby: %p", pUser->m_name, (mute ? "muted" : "un-muted"), this);
				pCryVoice->Mute(userIndex, pUser->m_userId, shouldBeMuted);
			}
		}
	}
}

//---------------------------------------
void CGameLobby::MutePlayersOnTeam( uint8 teamId, bool mute )
{
	const int numPlayers = m_nameList.Size();
	for (int i = 0; i < numPlayers; ++ i)
	{
		SSessionNames::SSessionName *pPlayer = &m_nameList.m_sessionNames[i];
		if (pPlayer->m_teamId == teamId)
		{
			MutePlayerBySessionName(pPlayer, mute, SSessionNames::SSessionName::MUTE_REASON_WRONG_TEAM);
		}
	}
}

//---------------------------------------
int CGameLobby::GetPlayerMuteReason(CryUserID userId)
{
	int result = 0;

	if (userId.IsValid())
	{
		SSessionNames::SSessionName *pPlayer = m_nameList.GetSessionNameByUserId(userId);
		if (pPlayer)
		{
			result = pPlayer->m_muted;

			ICryVoice *pCryVoice = gEnv->pNetwork->GetLobby()->GetVoice();
			uint32 userIndex = GameLobby_GetCurrentUserIndex();
			if (pCryVoice != NULL && pCryVoice->IsMutedExternally(userIndex, userId))
			{
				result |= SSessionNames::SSessionName::MUTE_REASON_MANUAL;
			}
		}
	}

	return result;
}

//---------------------------------------
ELobbyVOIPState CGameLobby::GetVoiceState(int channelId)
{
	ELobbyVOIPState voiceState = eLVS_off;

	CryUserID playerId = GetUserIDFromChannelID(channelId);
	if (playerId.IsValid())
	{
		uint32 userIndex = GameLobby_GetCurrentUserIndex();

		int muteReason = GetPlayerMuteReason(playerId);
		if (muteReason & SSessionNames::SSessionName::MUTE_REASON_WRONG_TEAM)
		{
			voiceState = eLVS_mutedWrongTeam;
		}
		else if (muteReason != 0)
		{
			voiceState = eLVS_muted;
		}
		else
		{
			ICryVoice *pCryVoice = gEnv->pNetwork->GetLobby()->GetVoice();
			if (pCryVoice != NULL && pCryVoice->IsMicrophoneConnected(playerId))
			{
				voiceState = eLVS_on;

				if (pCryVoice->IsSpeaking(userIndex, playerId))
				{
					voiceState = eLVS_speaking;
				}
			}
		}
	}

	return voiceState;
}

//---------------------------------------
void CGameLobby::CycleAutomaticMuting()
{
	int newType = m_autoVOIPMutingType + 1;
	if (newType == (int)eLAVT_end)
	{
		newType = (int)eLAVT_start;
	}

	SetAutomaticMutingState((ELobbyAutomaticVOIPType)newType);
}

void CGameLobby::SetAutomaticMutingStateForPlayer(SSessionNames::SSessionName *pPlayer, ELobbyAutomaticVOIPType newType)
{
	if (!pPlayer)
		return;

	switch (newType)
	{
	case eLAVT_off:
		{
			MutePlayerBySessionName(pPlayer, false, (SSessionNames::SSessionName::MUTE_PLAYER_AUTOMUTE_REASONS));
#if !defined(_RELEASE)
			if(gl_voip_debug)
			{
				CryLog("Trying to un-mute %s Lobby: %p", pPlayer->m_name, this);
			}
#endif
		}
		break;
	case eLAVT_allButParty:
		{
			if (g_pGame->GetSquadManager() && g_pGame->GetSquadManager()->IsSquadMateByUserId(pPlayer->m_userId) == false)
			{
				MutePlayerBySessionName(pPlayer, true, SSessionNames::SSessionName::MUTE_REASON_NOT_IN_SQUAD);
#if !defined(_RELEASE)
				if(gl_voip_debug)
				{
					CryLog("Trying to mute %s Lobby: %p", pPlayer->m_name, this);
				}
#endif
			}
			MutePlayerBySessionName(pPlayer, false, SSessionNames::SSessionName::MUTE_REASON_MUTE_ALL);
		}
		break;
	case eLAVT_all:
		{
			MutePlayerBySessionName(pPlayer, true, SSessionNames::SSessionName::MUTE_REASON_MUTE_ALL);
			MutePlayerBySessionName(pPlayer, false, SSessionNames::SSessionName::MUTE_REASON_NOT_IN_SQUAD);
#if !defined(_RELEASE)
			if(gl_voip_debug)
			{
				CryLog("Trying to mute %s Lobby: %p", pPlayer->m_name, this);
			}
#endif
		}
		break;
	default:
		CRY_ASSERT_MESSAGE(0, "Unknown automatic voice muting type");
		break;
	}

}

void CGameLobby::SetAutomaticMutingState(ELobbyAutomaticVOIPType newType)
{
#if !defined(_RELEASE)
	if(gl_voip_debug)
	{
		CryLog("SetAutomaticMutingState %d -> %d Lobby: %p", (int)m_autoVOIPMutingType, (int)newType, this);

		switch (newType)
		{
		case eLAVT_off:					CryLog("Automatic muting off Lobby: %p", this);												break;
		case eLAVT_allButParty:	CryLog("Automatic muting all but party Lobby: %p", this);							break;
		case eLAVT_all:					CryLog("Automatic muting all Lobby: %p", this);												break;
		default:								CryLog("Unknown automatic muting %d Lobby: %p", (int)newType, this);	break;
		};
	}
#endif
	
	m_autoVOIPMutingType = (ELobbyAutomaticVOIPType)newType;

	const int numPlayers = m_nameList.Size();

	for (int i = 0; i < numPlayers; ++ i)
	{
		SSessionNames::SSessionName* pPlayer = &m_nameList.m_sessionNames[i];

		if (pPlayer)
		{
			const CryUserID localUser = gEnv->pNetwork->GetLobby()->GetLobbyService()->GetUserID(GameLobby_GetCurrentUserIndex());
			const bool bIsLocalUser = localUser.IsValid() && (localUser == pPlayer->m_userId);
			if (!bIsLocalUser)
			{
				SetAutomaticMutingStateForPlayer(pPlayer, m_autoVOIPMutingType);
			}
		}
	}
}

//---------------------------------------
void CGameLobby::MatchmakingSessionStartCallback( CryLobbyTaskID taskID, ECryLobbyError error, void* pArg )
{
	ENSURE_ON_MAIN_THREAD;

	CryLog("[GameLobby] MatchmakingSessionStartCallback %s %d Lobby: %p", error == eCLE_Success ? "succeeded with error" : "failed with error", error, pArg);

	CGameLobby *pGameLobby = static_cast<CGameLobby*>(pArg);
	if (pGameLobby->NetworkCallbackReceived(taskID, error))
	{
		pGameLobby->m_bSessionStarted = true;

		// Tell the IGameSessionHandler
		g_pGame->GetIGameFramework()->GetIGameSessionHandler()->StartSession();

		pGameLobby->SetState(eLS_PreGame);

		CSquadManager *pSquadManager = g_pGame->GetSquadManager();
		if (pSquadManager)
		{
			pSquadManager->OnGameSessionStarted();
		}
	}
	else if (error != eCLE_TimeOut)
	{
		pGameLobby->SessionStartFailed(error);
	}
}

void CGameLobby::SessionEndCleanup()
{
	m_bSessionStarted = false;

	// Tell the IGameSessionHandler
	g_pGame->GetIGameFramework()->GetIGameSessionHandler()->EndSession();

	if (m_state != eLS_Leaving)
	{
		if(m_server)
		{
			SetState(eLS_GameEnded);
		}
		else
		{
			if(!IsQuitting())
			{
				//-- remove the loading screen.
#ifdef USE_C2_FRONTEND
				CFlashFrontEnd *pFFE = g_pGame->GetFlashMenu();
				if(pFFE != NULL && pFFE->IsFlashRenderingDuringLoad())
				{
					pFFE->StopFlashRenderingDuringLoading();
				}
#endif //#ifdef USE_C2_FRONTEND

				//-- enable action mapping again, loading screen will have disabled it
				gEnv->pGame->GetIGameFramework()->GetIActionMapManager()->Enable(true);

#ifdef USE_C2_FRONTEND
				CFlashFrontEnd *pFlashFrontEnd = g_pGame->GetFlashMenu();
				if (pFlashFrontEnd)
				{
					pFlashFrontEnd->Initialize(CFlashFrontEnd::eFlM_Menu, eFFES_game_lobby, eFFES_game_lobby);
				}
#endif //#ifdef USE_C2_FRONTEND

				gEnv->pConsole->ExecuteString("unload", false, true);
				SetState(eLS_PostGame);
				SendPacket(eGUPD_LobbyEndGameResponse);
			}
			else
			{
				CryLog("  delaying unload as we are quitting");
			}
		}
	}

	CSquadManager *pSquadManager = g_pGame->GetSquadManager();
	if (pSquadManager)
	{
		pSquadManager->OnGameSessionEnded();
	}
}

//---------------------------------------
void CGameLobby::MatchmakingSessionEndCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pArg)
{
	ENSURE_ON_MAIN_THREAD;

	CryLog("[GameLobby] MatchmakingSessionEndCallback %s %d Lobby: %p", error == eCLE_Success ? "succeeded with error" : "failed with error", error, pArg);

	g_pGame->ResetServerGameTokenSynch();

	CGameLobby *pGameLobby = static_cast<CGameLobby*>(pArg);
	if (pGameLobby->NetworkCallbackReceived(taskID, error))
	{
		pGameLobby->SessionEndCleanup();
	}
	else if (error != eCLE_TimeOut)
	{
		pGameLobby->SessionEndFailed(error);
	}
}

//---------------------------------------
void CGameLobby::SessionStartFailed(ECryLobbyError error)
{
	CryLog ("Session start failed (error=%d)", error);
	INDENT_LOG_DURING_SCOPE();

	LeaveSession(true);

#ifdef GAME_IS_CRYSIS2
	CWarningsManager *pWM = g_pGame->GetWarnings();
	if(pWM)
	{
		pWM->RemoveWarning("StartingSession");
	}

	g_pGame->AllowMultiplayerFrontEndAssets(true, true);
#endif
}

//---------------------------------------
void CGameLobby::SessionEndFailed(ECryLobbyError error)
{
	if(m_gameLobbyMgr->IsPrimarySession(this))
	{
#ifdef USE_C2_FRONTEND
		CFlashFrontEnd *pFlashFrontend = g_pGame->GetFlashMenu();
		CMPMenuHub *pMPMenu = pFlashFrontend ? pFlashFrontend->GetMPMenu() : NULL;

		if(pMPMenu)
		{
			pMPMenu->SetDisconnectError(CMPMenuHub::eDCE_SessionEndFailed, true);
		}
#endif //#ifdef USE_C2_FRONTEND
	}

	// Failed to do a session end - this is a disconnect case so we need to reset the flags (and do the same for the squad)
	m_bSessionStarted = false;
	CSquadManager *pSquadManager = g_pGame->GetSquadManager();
	if (pSquadManager)
	{
		pSquadManager->OnGameSessionEnded();
	}
}

//-------------------------------------------------------------------------
bool CGameLobby::OnInitiate( SHostMigrationInfo& hostMigrationInfo, uint32& state )
{
	if (hostMigrationInfo.m_session == m_currentSession)
	{
		if (m_gameLobbyMgr->IsNewSession(this))
		{
			ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
			if (pLobby)
			{
				ICryMatchMaking *pMatchmaking = pLobby->GetMatchMaking();
				if (pMatchmaking)
				{
					pMatchmaking->TerminateHostMigration(m_currentSession);
				}
			}
		}

		if (m_server && m_bMatchmakingSession)
		{
			CryLog("CGameLobby::OnInitiate(), aborting lobby merging and searches, lobby: %p", this);

			FindGameCancelMove();

			if (m_gameLobbyMgr->IsPrimarySession(this))
			{
				CGameBrowser *pGameBrowser = g_pGame->GetGameBrowser();
				if (pGameBrowser)
				{
					pGameBrowser->CancelSearching();
				}
			}
		}
#if defined (TRACK_MATCHMAKING)
		if( m_bMatchmakingSession )
		{
			if( CMatchmakingTelemetry* pMMTel = g_pGame->GetMatchMakingTelemetry() )
			{
				pMMTel->AddEvent( SMMMigrateHostLobbyEvent() );
			}
		}
#endif

	}
#ifndef _RELEASE
		m_migrationStarted = true;
#endif

	return true;
}

//---------------------------------------
bool CGameLobby::OnDemoteToClient(SHostMigrationInfo& hostMigrationInfo, uint32& state)
{
	if (hostMigrationInfo.m_session == m_currentSession)
	{
#if defined (TRACK_MATCHMAKING)
		if( m_server )
		{
			//we were the server and we have been made a client;
			if( CMatchmakingTelemetry* pMMTel = g_pGame->GetMatchMakingTelemetry() )
			{
				pMMTel->AddEvent( SMMDemotedToClientEvent() );
			}
		}
#endif

		m_server = false;
		m_connectedToDedicatedServer = false;
		CryLog("CGameLobby::OnDemoteToClient() isNewServer=false Lobby: %p", this);
	}

	return true;
}

//---------------------------------------
bool CGameLobby::OnPromoteToServer(SHostMigrationInfo& hostMigrationInfo, uint32& state)
{
	if (hostMigrationInfo.m_session == m_currentSession)
	{
		m_server = true;
		m_connectedToDedicatedServer = false;
		CryLog("CGameLobby::OnPromoteToServer() isNewServer=true Lobby: %p", this);

		// Mark all connections as fully connected
		SSessionNames &names = m_nameList;
		// Remove all placeholder entries, if we don't know about the player at this point, they aren't
		// going to manage the migration
		names.RemoveBlankEntries();
		const int numNames = names.Size();
		for (int i = 0; i < numNames; ++ i)
		{
			names.m_sessionNames[i].m_bFullyConnected = true;
		}

	#ifdef GAME_IS_CRYSIS2
		CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
		if (pPlaylistManager)
		{
			pPlaylistManager->OnPromoteToServer();
		}
#endif

		m_bHasReceivedVoteOptions = false;

#if defined (TRACK_MATCHMAKING)
		if( CMatchmakingTelemetry* pMMTel = g_pGame->GetMatchMakingTelemetry() )
		{
			pMMTel->AddEvent( SMMBecomeHostEvent() );
		}
#endif
	}

	return true;
}

//---------------------------------------
bool CGameLobby::OnFinalise( SHostMigrationInfo& hostMigrationInfo, uint32& state )
{
	if (hostMigrationInfo.m_session != m_currentSession)
	{
		return true;
	}

	const bool isNewServer = hostMigrationInfo.IsNewHost();
	CryLog("CGameLobby::OnFinalise() isNewServer=%s Lobby: %p", isNewServer ? "true" : "false", this);
	CryLog("  isPrimarySession=%s, numPlayers=%i", m_gameLobbyMgr->IsPrimarySession(this) ? "true" : "false", m_nameList.Size());

	ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
	if (pLobby)
	{
		ICryMatchMaking *pMatchmaking = pLobby->GetMatchMaking();
		if (pMatchmaking)
		{
			CrySessionID sessionId = pMatchmaking->SessionGetCrySessionIDFromCrySessionHandle(m_currentSession);
			SetCurrentId(sessionId, true, true);
			InformSquadManagerOfSessionId();
		}
	}

	if (isNewServer)
	{
		if (m_bMatchmakingSession)
		{
			CryLog("  this is a matchmaking session so setting m_shouldFindGames = TRUE");
			CGameLobby::s_bShouldBeSearching = true;
			m_shouldFindGames = true;
		}

		if (m_bSessionStarted)
		{
			SendPacket(eGUPD_LobbyGameHasStarted);
		}

		m_lastActiveStatus = GetActiveStatus(m_state);
		m_taskQueue.AddTask(CLobbyTaskQueue::eST_Migrate, false);
		m_taskQueue.AddTask(CLobbyTaskQueue::eST_Update, true);

#if !defined(_RELEASE)
		if (gl_debugForceLobbyMigrations)
		{
			gEnv->pConsole->GetCVar("net_hostHintingActiveConnectionsOverride")->Set(1);
			CryLog("  setting net_hostHintingActiveConnectionsOverride to 1");
			m_timeTillCallToEnsureBestHost = gl_debugForceLobbyMigrationsTimer;
		}
#endif
	}
#if !defined(_RELEASE)
	else
	{
		if (gl_debugForceLobbyMigrations)
		{
			int numConnections = (int) (gEnv->pTimer->GetAsyncTime().GetMilliSecondsAsInt64() % 127);
			numConnections = numConnections + 2;
			CryLog("  setting net_hostHintingActiveConnectionsOverride to %i", numConnections);
			gEnv->pConsole->GetCVar("net_hostHintingActiveConnectionsOverride")->Set(numConnections);
		}
	}
#endif

	if( m_bMatchmakingSession )
	{
		if( CMatchMakingHandler* pMatchmakingHandler = m_gameLobbyMgr->GetMatchMakingHandler() )
		{
			pMatchmakingHandler->OnHostMigrationFinished( true, isNewServer );
		}
	}

	if (m_state == eLS_Lobby)
	{
#ifdef USE_C2_FRONTEND
		CFlashFrontEnd *pFrontEnd = g_pGame->GetFlashMenu();
		if (pFrontEnd)
		{
			pFrontEnd->RefreshCurrentScreen();
		}
#endif //#ifdef USE_C2_FRONTEND
	}

#if defined(TRACK_MATCHMAKING)
	if( CMatchmakingTelemetry* pMMTel = g_pGame->GetMatchMakingTelemetry() )
	{
		ICryLobbyService* pLobbyService = pLobby ? pLobby->GetLobbyService( eCLS_Online ) : NULL;
		ICryMatchMaking* pMatchMaking = pLobbyService ? pLobbyService->GetMatchMaking(): NULL;

		SCryMatchMakingConnectionUID conUID = pMatchMaking->GetHostConnectionUID( hostMigrationInfo.m_session );

		CryFixedStringT<DISPLAY_NAME_LENGTH> displayName;

		SSessionNames::SSessionName *pSessionName = m_nameList.GetSessionName( conUID, false );
		if( pSessionName )
		{
			pSessionName->GetDisplayName( displayName );
		}

		pMMTel->AddEvent( SMMMigrateCompletedEvent( displayName, m_currentSessionId ) );
	}
#endif //defined(TRACK_MATCHMAKING)

	return true;
}

//---------------------------------------
bool CGameLobby::OnWarningReturn(THUDWarningId id, const char* returnValue)
{
#ifdef GAME_IS_CRYSIS2
	if (id == m_DLCServerStartWarningId)
	{
		if(stricmp(returnValue, "yes")==0)
		{
			// Start the server
			m_bSkipCountdown = true;
		}
	}
	else if(id == g_pGame->GetWarnings()->GetWarningId("ServerListPassword"))
	{
		if (returnValue && returnValue[0] && strcmp(returnValue, "*cancel*"))
		{
			ICVar *pCVar = gEnv->pConsole->GetCVar("sv_password");
			if (pCVar)
			{
				pCVar->Set(returnValue);
			}

			JoinServer(m_pendingConnectSessionId, NULL, CryMatchMakingInvalidConnectionUID, true);
		}
	}
#endif
	return true;
}

//---------------------------------------
void CGameLobby::OnWarningRemoved(THUDWarningId id)
{
}

//---------------------------------------
CGameLobby::SPlayerGroup::SPlayerGroup()
{
	Reset();
}

//---------------------------------------
void CGameLobby::SPlayerGroup::Reset()
{
	m_totalSkill = 0;
	m_totalPrevScore = 0;
	m_numMembers = 0;
	m_type = ePGT_Unknown;
	m_members.clear();
	m_valid = false;
}

//---------------------------------------
void CGameLobby::SPlayerGroup::InitClan( const char *pClanName )
{
	m_clanTag = pClanName;
	m_type = ePGT_Clan;
	m_valid = true;
}

//---------------------------------------
void CGameLobby::SPlayerGroup::InitIndividual( const SCryMatchMakingConnectionUID *pConId, CGameLobby *pGameLobby )
{
	m_type = ePGT_Individual;
	m_members.push_back(*pConId);
	++ m_numMembers;
	CRY_ASSERT(m_numMembers == 1);
	m_totalSkill = pGameLobby->GetSkill(pConId);
	m_valid = true;
}

//---------------------------------------
void CGameLobby::SPlayerGroup::InitSquad( const SCryMatchMakingConnectionUID *pLeader )
{
	m_pSquadLeader = *pLeader;
	m_type = ePGT_Squad;
	m_valid = true;
}

//---------------------------------------
void CGameLobby::SPlayerGroup::AddMember( const SCryMatchMakingConnectionUID *pConId, CGameLobby *pGameLobby )
{
	CRY_ASSERT((m_type == ePGT_Clan) || (m_type == ePGT_Squad));
	m_members.push_back(*pConId);
	++ m_numMembers;
	m_totalSkill += pGameLobby->GetSkill(pConId);
}

//---------------------------------------
void CGameLobby::SPlayerGroup::RemoveMember( const SCryMatchMakingConnectionUID *pConId, CGameLobby *pGameLobby )
{
	RemoveMember(pConId, pGameLobby->GetSkill(pConId));
}

//---------------------------------------
void CGameLobby::SPlayerGroup::RemoveMember( const SCryMatchMakingConnectionUID *pConId, int skill )
{
	const int numMembers = m_numMembers;
	for (int i = 0; i < numMembers; ++ i)
	{
		if (m_members[i] == *pConId)
		{
			m_members.removeAt(i);
			break;
		}
	}
	-- m_numMembers;
	CRY_ASSERT(m_members.size() == m_numMembers);
	if (m_numMembers > 0)
	{
		m_totalSkill -= skill;
	}
	else
	{
		Reset();
	}
}

//---------------------------------------
void CGameLobby::SwitchToPrimaryLobby()
{
	CRY_ASSERT(m_gameLobbyMgr->IsPrimarySession(this));
	g_pGame->GetSquadManager()->GameSessionIdChanged(CSquadManager::eGSC_LobbyMerged, m_currentSessionId);
}

//---------------------------------------
void CGameLobby::OnOptionsChanged()
{
	CRY_ASSERT(m_gameLobbyMgr->IsPrimarySession(this));
	if (m_server && (m_state != eLS_Leaving))
	{
#ifdef GAME_IS_CRYSIS2
		SendPacket(eGUPD_SetGameVariant);
#endif
		m_nameList.m_dirty = true;		// Options may have changed the names list, need to update it
	}
}

//---------------------------------------
void CGameLobby::SetLobbyService(ECryLobbyService lobbyService)
{
	if ((lobbyService != eCLS_Online) && (lobbyService != eCLS_LAN))
	{
		GameWarning("Unknown lobby service %d expecting eCLS_Online or eCLS_LAN", (int)(lobbyService));
		return;
	}

	const bool allowJoinMultipleSessions = gEnv->pNetwork->GetLobby()->GetLobbyServiceFlag( lobbyService, eCLSF_AllowMultipleSessions );
	g_pGame->GetSquadManager()->Enable( allowJoinMultipleSessions, true );
	gEnv->pNetwork->GetLobby()->SetLobbyService(lobbyService);
}

//---------------------------------------
#ifdef USE_C2_FRONTEND
void CGameLobby::UpdateVotingInfoFlashInfo()
{
	CryLog("CGameLobby::UpdateVotingInfoFlashInfo()");

#ifndef _RELEASE
	m_votingFlashInfo.tmpWatchInfoIsSet = false;
#endif

	if (m_votingEnabled)
	{
		m_votingFlashInfo.leftNumVotes = m_leftVoteChoice.m_numVotes;
		m_votingFlashInfo.rightNumVotes = m_rightVoteChoice.m_numVotes;

		m_votingFlashInfo.votingClosed = m_votingClosed;
		m_votingFlashInfo.votingDrawn = (m_leftVoteChoice.m_numVotes == m_rightVoteChoice.m_numVotes);
		m_votingFlashInfo.leftWins = m_leftWinsVotes;

		m_votingFlashInfo.localHasCandidates = (m_localVoteStatus != eLVS_awaitingCandidates);
		m_votingFlashInfo.localHasVoted = ((m_localVoteStatus == eLVS_votedLeft) || (m_localVoteStatus == eLVS_votedRight));
		m_votingFlashInfo.localVotedLeft = (m_localVoteStatus == eLVS_votedLeft);


		// Vote status message
		m_votingFlashInfo.votingStatusMessage.clear();
		if (m_votingFlashInfo.localHasCandidates)
		{
			if (m_votingFlashInfo.votingClosed)
			{
				const char* winningMapName = m_votingFlashInfo.leftWins ? m_votingCandidatesFlashInfo.leftLevelName.c_str() : m_votingCandidatesFlashInfo.rightLevelName.c_str();
				if (m_votingFlashInfo.votingDrawn)
				{
					m_votingFlashInfo.votingStatusMessage = CHUDUtils::LocalizeString("@ui_menu_gamelobby_vote_draw_winner", winningMapName);
				}
				else
				{
					m_votingFlashInfo.votingStatusMessage = CHUDUtils::LocalizeString("@ui_menu_gamelobby_vote_winner", winningMapName);
				}
			}
			else
			{
				if (m_votingFlashInfo.localHasVoted)
				{
					const char* votedMapName = m_votingFlashInfo.localVotedLeft ? m_votingCandidatesFlashInfo.leftLevelName.c_str() : m_votingCandidatesFlashInfo.rightLevelName.c_str();
					m_votingFlashInfo.votingStatusMessage = CHUDUtils::LocalizeString("@ui_menu_gamelobby_voted", votedMapName);
				}
				else
				{
					m_votingFlashInfo.votingStatusMessage = CHUDUtils::LocalizeString("@ui_menu_gamelobby_vote_now");
				}
			}
		}

		m_sessionUserDataDirty = true;

#ifndef _RELEASE
		m_votingFlashInfo.tmpWatchInfoIsSet = true;
#endif
	}
}

//---------------------------------------
void CGameLobby::UpdateVotingCandidatesFlashInfo()
{
	CryLog("CGameLobby::UpdateVotingCandidatesFlashInfo()");

#ifndef _RELEASE
	m_votingCandidatesFlashInfo.tmpWatchInfoIsSet = false;
#endif

	// Collect left choice info
	m_votingCandidatesFlashInfo.leftLevelMapPath = m_leftVoteChoice.m_levelName.c_str();
	m_votingCandidatesFlashInfo.leftRulesName.Format("@ui_rules_%s", m_leftVoteChoice.m_gameRules.c_str());
	GetMapImageName(m_votingCandidatesFlashInfo.leftLevelMapPath, &m_votingCandidatesFlashInfo.leftLevelImage);

	m_votingCandidatesFlashInfo.leftLevelName = PathUtil::GetFileName(m_votingCandidatesFlashInfo.leftLevelMapPath.c_str()).c_str();
	m_votingCandidatesFlashInfo.leftLevelName = g_pGame->GetMappedLevelName(m_votingCandidatesFlashInfo.leftLevelName.c_str());

	// Collect right choice info
	m_votingCandidatesFlashInfo.rightLevelMapPath = m_rightVoteChoice.m_levelName.c_str();
	m_votingCandidatesFlashInfo.rightRulesName.Format("@ui_rules_%s", m_rightVoteChoice.m_gameRules.c_str());
	GetMapImageName(m_votingCandidatesFlashInfo.rightLevelMapPath, &m_votingCandidatesFlashInfo.rightLevelImage);

	m_votingCandidatesFlashInfo.rightLevelName = PathUtil::GetFileName(m_votingCandidatesFlashInfo.rightLevelMapPath.c_str()).c_str();
	m_votingCandidatesFlashInfo.rightLevelName = g_pGame->GetMappedLevelName(m_votingCandidatesFlashInfo.rightLevelName.c_str());

	m_sessionUserDataDirty = true;

#ifndef _RELEASE
	m_votingCandidatesFlashInfo.tmpWatchInfoIsSet = true;
#endif
}
#endif //#ifdef USE_C2_FRONTEND

//---------------------------------------
void CGameLobby::SetLobbyTaskId( CryLobbyTaskID taskId )
{
	CryLog("CGameLobby::SetLobbyTaskId() this=%p, taskId=%u", this, taskId);
	m_currentTaskId = taskId;
}

//---------------------------------------
void CGameLobby::SetMatchmakingGame( bool bMatchmakingGame )
{
	CryLog("CGameLobby::SetMatchmakingGame() this=%p, bMatchmakingGame=%s", this, bMatchmakingGame ? "TRUE" : "FALSE");
	m_bMatchmakingSession = bMatchmakingGame;
}

//---------------------------------------
void CGameLobby::LeaveAfterSquadMembers()
{
	CryLog("CGameLobby::LeaveAfterSquadMembers()");

	eHostMigrationState hostMigrationState = eHMS_Unknown;
	ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
	if (pLobby)
	{
		ICryMatchMaking *pMatchmaking = pLobby->GetMatchMaking();
		if (pMatchmaking)
		{
			hostMigrationState = pMatchmaking->GetSessionHostMigrationState(m_currentSession);
		}
	}

	if (hostMigrationState != eHMS_Idle)
	{
		// Currently migrating, don't know if we should wait or not so just leave
		LeaveSession(true);
		return;
	}

	CSquadManager *pSquadManager = g_pGame->GetSquadManager();
	bool bFoundSquadMembers = false;
	if (pSquadManager)
	{
		SSessionNames &playerList = m_nameList;

		const int numPlayers = playerList.Size();
		// Start at 1 since we don't include ourselves
		for (int i = 1; i < numPlayers; ++ i)
		{
			SSessionNames::SSessionName &player = playerList.m_sessionNames[i];
			if (pSquadManager->IsSquadMateByUserId(player.m_userId))
			{
				player.m_bMustLeaveBeforeServer = true;
				bFoundSquadMembers = true;
			}
		}
	}
	if (bFoundSquadMembers)
	{
		CryLog("  found squad members in the lobby, start terminate host hinting task");
		m_taskQueue.AddTask(CLobbyTaskQueue::eST_TerminateHostHinting, false);
	}
	else
	{
		CryLog("  failed to find squad members in the lobby, leave the squad");
		LeaveSession(true);
	}
}

//---------------------------------------
void CGameLobby::InformSquadManagerOfSessionId()
{
	if(m_gameLobbyMgr->IsPrimarySession(this) && (!m_gameLobbyMgr->IsLobbyMerging()))
	{
		CSquadManager *pSquadManager = g_pGame->GetSquadManager();
		if (m_currentSessionId == CrySessionInvalidID)
		{
			pSquadManager->GameSessionIdChanged(CSquadManager::eGSC_LeftSession, m_currentSessionId);
		}
		else
		{
			if (m_bMigratedSession)
			{
				pSquadManager->GameSessionIdChanged(CSquadManager::eGSC_LobbyMigrated, m_currentSessionId);
			}
			else
			{
				pSquadManager->GameSessionIdChanged(CSquadManager::eGSC_JoinedNewSession, m_currentSessionId);
			}
		}
	}
}

//---------------------------------------
// [static]
THUDWarningId CGameLobby::ShowErrorDialog(const ECryLobbyError error, const char* pDialogName, const char* pDialogParam, IGameWarningsListener* pWarningsListener)
{
	CryLog("CGameLobby::ShowErrorDialog(), error=%d, pDialogName='%s', pDialogParam='%s' pWarningsListener=0x%p", error, pDialogName, pDialogParam, pWarningsListener);

#ifdef GAME_IS_CRYSIS2
	CRY_ASSERT_MESSAGE((!pDialogParam || pDialogName), "A custom dialog param should only be provided if a custom dialog name is also provided.");

	CryFixedStringT<32>  name;
	CryFixedStringT<32>  param;

	bool handled = false;
#ifdef USE_C2_FRONTEND
	CMPMenuHub* pMPMenuHub = CMPMenuHub::GetMPMenuHub();

	switch (error)
	{
	case eCLE_UserNotSignedIn:
		{
			if (pMPMenuHub)
			{
				pMPMenuHub->SetDisconnectError(CMPMenuHub::eDCE_F_UserNotSignedIn, true);
				handled = true;
			}
			name.Format("CantMultiplayerNoSignIn");
			break;
		}
	case eCLE_NoOnlineAccount:
		{
			if (pMPMenuHub)
			{
				pMPMenuHub->SetDisconnectError(CMPMenuHub::eDCE_F_OnlineUserNotSignedIn, true);
				handled = true;
			}
			name.Format("CantOnlineNotOnlineSignIn");
			break;
		}
	case eCLE_InternalError:
		{
			if (pDialogName)
			{
				name = pDialogName;

				if (pDialogParam)
				{
					param = pDialogParam;
				}
				else
				{
					param.Format("@ui_menu_lobby_internal_error");
				}
			}
			else
			{
				name = "LobbyInternalError";
			}

			break;
		}
#if USE_CRYLOBBY_GAMESPY
	case eCLE_CDKeyMalformed:
	case eCLE_CDKeyAuthFailed:	// fall through
		name = "SerialCannotVerify";
		break;
	case eCLE_CDKeyUnknown:
		name = "SerialInvalid";
		break;
	case eCLE_CDKeyDisabled:
		name = "SerialInvalidDisabled";
		break;
	case eCLE_CDKeyInUse:
		name = "SerialInvalidInUse";
		break;
	case eCLE_SessionFull:
		// Since we can't edit the string table, repurose another string entry.
		name = "LobbyError";
		param = "@ui_menu_disconnect_error_4";
		break;
#endif
	default:
		{
			bool  generateParam = false;

			if (pDialogName)
			{
				name = pDialogName;

				if (pDialogParam)
				{
					param = pDialogParam;
				}
				else
				{
					generateParam = true;
				}
			}
			else
			{
				name = "LobbyError";
				generateParam = true;
			}

			if (generateParam)
			{
				param.Format("@ui_menu_lobby_error_%d", (int)error);
			}

			break;
		}
	}
#endif //#ifdef USE_C2_FRONTEND

	if (handled)
		CryLog(" > handled warning (name='%s', param='%s'), no need to show now.", name.c_str(), param.c_str());
	else
		CryLog(" > trying to show warning (name='%s', param='%s')", name.c_str(), param.c_str());

	bool showError = !handled;

	if(!CGameLobby::IsSignInError(error))
	{
		CGameLobbyManager *pGameLobbyManager = g_pGame->GetGameLobbyManager();
		showError = !pGameLobbyManager || pGameLobbyManager->IsCableConnected(); // if cable is not connected, far worse has happened
	}

#ifndef _RELEASE
	// Ignore errors when doing autotesting
	ICVar* pAutoTest = gEnv->pConsole->GetCVar("autotest_enabled");
	if(pAutoTest != NULL && pAutoTest->GetIVal())
	{
		showError = false;
	}
#endif

	if(showError)
	{
#ifdef USE_C2_FRONTEND
		if (pMPMenuHub != NULL && !pWarningsListener)
		{
			CryLog("   > showing warning via MPMenuHub");
			pMPMenuHub->ShowErrorDialog(name.c_str(), param.c_str());
		}
		else
#endif //#ifdef USE_C2_FRONTEND
		{
			CryLog("   > showing warning directly though Warning Manager");
#ifdef GAME_IS_CRYSIS2
			g_pGame->GetWarnings()->AddWarning(name, param, pWarningsListener);
#endif
		}
	}

	return g_pGame->GetWarnings()->GetWarningId(name.c_str());
#else
return 0;
#endif
}

bool CGameLobby::IsSignInError(ECryLobbyError error)
{
	bool isSignInError = false;

	switch(error)
	{
		case eCLE_UserNotSignedIn:
		case eCLE_NoOnlineAccount:
		case eCLE_InsufficientPrivileges:
			isSignInError = true;
			break;

		default:
			break;
	}

	return isSignInError;
}

//----------------------------------------------------------------------
void CGameLobby::LeaveSession(bool clearPendingTasks)
{
  CryLog("CGameLobby::LeaveSession() lobby:%p cancelling %d", this, m_bCancelling);
  INDENT_LOG_DURING_SCOPE();

	if(m_bCancelling)
	{
	  // need to make sure that any tasks added to the list since entering
	  // the new session are removed, the only ones we want left are the task in progress
	  // and the session delete
	  CancelSessionInit();
		return;
	}

	ICVar *pCVar = gEnv->pConsole->GetCVar("sv_password");
	if (pCVar != NULL && ((pCVar->GetFlags() & VF_WASINCONFIG) == 0))
	{
		pCVar->Set("");
	}

	IGameFramework *pFramework = g_pGame->GetIGameFramework();
	if (pFramework->StartedGameContext() || pFramework->StartingGameContext())
	{
		DoBetweenRoundsUnload();

		gEnv->pConsole->ExecuteString("disconnectchannel", false, true);

		TerminateHostMigration();
	}

	if ((m_currentSession != CrySessionInvalidHandle) && (m_state != eLS_Leaving))
	{
		if (clearPendingTasks)
		{
			CancelAllLobbyTasks();
		}

#if ENABLE_CHAT_MESSAGES
		ClearChatMessages();

#ifdef USE_C2_FRONTEND
		CFlashFrontEnd *pFlashMenu = g_pGame->GetFlashMenu();
		if (pFlashMenu && pFlashMenu->IsMenuActive(CFlashFrontEnd::eFlM_Menu))
		{
			if (IFlashPlayer *pFlashPlayer = pFlashMenu->GetFlash())
			{
				EFlashFrontEndScreens screen = pFlashMenu->GetCurrentMenuScreen();
				if (screen == eFFES_game_lobby)
				{
					pFlashPlayer->Invoke0(FRONTEND_SUBSCREEN_PATH_SET("ClearChatMessages"));
				}
			}
		}
#endif //#ifdef USE_C2_FRONTEND
#endif

#ifdef USE_C2_FRONTEND
		CFlashFrontEnd *flashMenu = g_pGame->GetFlashMenu();
		CMPMenuHub *mpMenuHub = flashMenu ? flashMenu->GetMPMenu() : NULL;
		CUIScoreboard *scoreboard = mpMenuHub ? mpMenuHub->GetScoreboardUI() : NULL;
		if (scoreboard)
		{
			scoreboard->ClearLastMatchResults();
		}
#endif //#ifdef USE_C2_FRONTEND

		CryLog("  setting state eLS_Leaving");
		SetState(eLS_Leaving);
	}
	else if (m_state == eLS_Leaving)
	{
		CryLog("  already in eLS_Leaving state");
		m_taskQueue.ClearNonVitalTasks();
	}
	else if (m_state == eLS_None)
	{
		CryLog("  already in eLS_None state");
	}
	else if (m_state == eLS_Initializing)
	{
		CryLog("  currently initializing, cancel session init");
		CancelSessionInit();
	}
	else
	{
		CRY_ASSERT(m_state == eLS_FindGame);
		CryLog("  not in a session but we're not in eLS_Leaving or eLS_None states, state=%u", m_state);
		CancelAllLobbyTasks();
		SetState(eLS_None);
	}

#ifdef USE_C2_FRONTEND
	CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
	CMPMenuHub *mpMenuHub = menu ? menu->GetMPMenu() : NULL;
	if (mpMenuHub != NULL && mpMenuHub->IsLoadingDialogVisible())
	{
		mpMenuHub->HideLoadingDialog("JoinSession");
	}
#endif //#ifdef USE_C2_FRONTEND
}

//----------------------------------------------------------------------
void CGameLobby::StartFindGame()
{
	if (m_state != eLS_None)
	{
		LeaveSession(false);
	}
	m_taskQueue.AddTask(CLobbyTaskQueue::eST_FindGame, true);

	// new quick match, starting from a clean slate
	// so clear the bad servers list too
	ClearBadServers();

#ifdef GAME_IS_CRYSIS2
	CPersistantStats *pStats = g_pGame->GetPersistantStats();
	if (pStats)
	{
		pStats->OnEnterFindGame();
	}
#endif
}

//----------------------------------------------------------------------
void CGameLobby::TaskStartedCallback( CLobbyTaskQueue::ESessionTask task, void *pArg )
{
	ENSURE_ON_MAIN_THREAD;

	CryLog("CGameLobby::TaskStartedCallback() task=%u, lobby=%p", task, pArg);
	INDENT_LOG_DURING_SCOPE();

	CGameLobby *pGameLobby = static_cast<CGameLobby*>(pArg);
	CRY_ASSERT(pGameLobby);
	if (pGameLobby)
	{
		CRY_ASSERT(pGameLobby->m_currentTaskId == CryLobbyInvalidTaskID);

		ECryLobbyError result = eCLE_ServiceNotSupported;
		CryLobbyTaskID taskId = CryLobbyInvalidTaskID;
		bool bMatchMakingTaskStarted = false;
		bool bRestartTask = false;

		ICryLobby *pCryLobby = gEnv->pNetwork->GetLobby();
		if (pCryLobby)
		{
			ICryMatchMaking *pMatchMaking = pCryLobby->GetMatchMaking();
			if (pMatchMaking)
			{
				switch (task)
				{
				case CLobbyTaskQueue::eST_Create:
					{
						result = pGameLobby->DoCreateSession(pMatchMaking, taskId);
						bMatchMakingTaskStarted = true;
					}
					break;
				case CLobbyTaskQueue::eST_Migrate:
					{
						if(pGameLobby->m_currentSession != CrySessionInvalidHandle)
						{
							result = pGameLobby->DoMigrateSession(pMatchMaking, taskId);
							bMatchMakingTaskStarted = true;
						}
					}
					break;
				case CLobbyTaskQueue::eST_Join:
					{
						if (pGameLobby->m_pendingConnectSessionId != CrySessionInvalidID)
						{
							bool bAlreadyInSession = NetworkUtils_CompareCrySessionId(pGameLobby->m_currentSessionId, pGameLobby->m_pendingConnectSessionId);
							if (bAlreadyInSession == false)
							{
								result = pGameLobby->DoJoinServer(pMatchMaking, taskId);
								bMatchMakingTaskStarted = true;
							}
							else
							{
								if( pGameLobby->m_bMatchmakingSession )
								{
									if( CMatchMakingHandler* pMMHandler = pGameLobby->m_gameLobbyMgr->GetMatchMakingHandler() )
									{
										pMMHandler->GameLobbyJoinFinished( eCLE_AlreadyInSession );
									}
								}
								CryLog("  task not started, already in correct session");
							}
						}
						else
						{
							if( pGameLobby->m_bMatchmakingSession )
							{
								if( CMatchMakingHandler* pMMHandler = pGameLobby->m_gameLobbyMgr->GetMatchMakingHandler() )
								{
									pMMHandler->GameLobbyJoinFinished( eCLE_InvalidParam );
								}
							}
							CryLog("  task not started, invalid target sessionId");
						}
					}
					break;
				case CLobbyTaskQueue::eST_Delete:
					{
						if(pGameLobby->m_currentSession != CrySessionInvalidHandle)
						{
							result = pGameLobby->DoDeleteSession(pMatchMaking, taskId);
							bMatchMakingTaskStarted = true;
						}
						else
						{
							if (pGameLobby->m_bCancelling)
							{
								CryLog("CGameLobby::eST_Delete no session, resetting m_bCancelling");
								pGameLobby->m_bCancelling = false;
							}
						}
					}
					break;
				case CLobbyTaskQueue::eST_SetLocalUserData:
					{
						if(pGameLobby->m_currentSession != CrySessionInvalidHandle)
						{
							result = pGameLobby->DoUpdateLocalUserData(pMatchMaking, taskId);
							bMatchMakingTaskStarted = true;
						}
					}
					break;
				case CLobbyTaskQueue::eST_SessionStart:
					{
						if(pGameLobby->m_currentSession != CrySessionInvalidHandle)
						{
							if (g_pGame->GetIGameFramework()->IsGameStarted())
							{
								CryLog("  didn't start eST_SessionStart - unloading previous level");
								bMatchMakingTaskStarted = false;
								bRestartTask = true;

								pGameLobby->DoBetweenRoundsUnload();
							}
							else
							{
								result = pGameLobby->DoStartSession(pMatchMaking, taskId, bMatchMakingTaskStarted);
							}
						}
					}
					break;
				case CLobbyTaskQueue::eST_SessionEnd:
					{
						if(pGameLobby->m_currentSession != CrySessionInvalidHandle)
						{
							result = pGameLobby->DoEndSession(pMatchMaking, taskId);
							bMatchMakingTaskStarted = true;
						}
					}
					break;
				case CLobbyTaskQueue::eST_Query:
					{
						if(pGameLobby->m_currentSession != CrySessionInvalidHandle)
						{
							result = pGameLobby->DoQuerySession(pMatchMaking, taskId);
							bMatchMakingTaskStarted = true;
						}
					}
					break;
				case CLobbyTaskQueue::eST_Update:
					{
						if (pGameLobby->m_server && (pGameLobby->m_currentSession != CrySessionInvalidHandle))
						{
							eHostMigrationState hostMigrationState = eHMS_Unknown;
							ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
							if (pLobby)
							{
								ICryMatchMaking *pMatchmaking = pLobby->GetMatchMaking();
								if (pMatchmaking)
								{
									hostMigrationState = pMatchmaking->GetSessionHostMigrationState(pGameLobby->m_currentSession);
								}
							}

							if (hostMigrationState == eHMS_Idle)
							{
								result = pGameLobby->DoUpdateSession(pMatchMaking, taskId);
								bMatchMakingTaskStarted = true;
							}
							else if (hostMigrationState == eHMS_Finalise)
							{
								// We add an Update task during OnFinalise callback but we can't run it until the migration has
								// fully completed, delay the start until we're ready
								CryLog("  can't do eST_Update yet as we're mid-migration");
								bRestartTask = true;
							}
						}
					}
					break;
				case CLobbyTaskQueue::eST_EnsureBestHost:
					{
						if(pGameLobby->m_currentSession != CrySessionInvalidHandle)
						{
							if (pGameLobby->ShouldCheckForBestHost())
							{
								result = pGameLobby->DoEnsureBestHost(pMatchMaking, taskId);
								bMatchMakingTaskStarted = true;
							}
#if !defined(_RELEASE)
							else
							{
								if (gl_debugForceLobbyMigrations)
								{
									pGameLobby->m_timeTillCallToEnsureBestHost = gl_debugForceLobbyMigrationsTimer;
								}
							}
#endif
						}
					}
					break;
				case CLobbyTaskQueue::eST_FindGame:
					{
						pGameLobby->SetState(eLS_FindGame);
					}
					break;
				case CLobbyTaskQueue::eST_TerminateHostHinting:
					{
						if(pGameLobby->m_currentSession != CrySessionInvalidHandle)
						{
							result = pGameLobby->DoTerminateHostHintingForGroup(pMatchMaking, taskId, bMatchMakingTaskStarted);
						}
					}
					break;
				case CLobbyTaskQueue::eST_SessionSetLocalFlags:
					{
						if(pGameLobby->m_currentSession != CrySessionInvalidHandle)
						{
							result = pGameLobby->DoSessionSetLocalFlags(pMatchMaking, taskId);
							bMatchMakingTaskStarted = true;
						}
					}
					break;
				case CLobbyTaskQueue::eST_StartGameContext:
					{
						pGameLobby->StartGame();
					}
					break;
				case CLobbyTaskQueue::eST_SessionRequestDetailedInfo:
					{
						pGameLobby->DoSessionDetailedInfo(pMatchMaking, taskId);
					}
					break;
				case CLobbyTaskQueue::eST_Unload:
					{
						pGameLobby->DoUnload();
					}
					break;
				}
			}
		}

		if (bMatchMakingTaskStarted)
		{
			if (result == eCLE_Success)
			{
				pGameLobby->SetLobbyTaskId(taskId);
			}
			else if(result == eCLE_SuccessInvalidSession)
			{
				pGameLobby->m_taskQueue.TaskFinished();
			}
			else if (result == eCLE_TooManyTasks)
			{
				CryLog("  too many tasks, restarting next frame");
				pGameLobby->m_taskQueue.RestartTask();
			}
			else
			{
				ShowErrorDialog(result, NULL, NULL, NULL);
				pGameLobby->m_taskQueue.TaskFinished();
			}
		}
		else
		{
			if (bRestartTask)
			{
				pGameLobby->m_taskQueue.RestartTask();
			}
			else
			{
				// Either the task wasn't started (no longer valid given the lobby state) or we're not expecting a callback
				pGameLobby->m_taskQueue.TaskFinished();
			}
		}
	}
}

int CGameLobby::GetNumberOfExpectedClients() const
{
	const int numPlayers = m_nameList.Size();

	return numPlayers;
}

//-------------------------------------------------------------------------
bool CGameLobby::NetworkCallbackReceived( CryLobbyTaskID taskId, ECryLobbyError result )
{
	ENSURE_ON_MAIN_THREAD;

	if (result == eCLE_SuccessContinue)
	{
		// Network task has not finished yet but we need to process the results received so far
		return true;
	}
	CRY_ASSERT(m_currentTaskId == taskId);
	if (m_currentTaskId == taskId)
	{
		m_currentTaskId = CryLobbyInvalidTaskID;
	}
	else
	{
		CryLog("CGameLobby::NetworkCallbackReceived() received callback with an unexpected taskId=%d, expected=%d", taskId, m_currentTaskId);
	}

	if (result == eCLE_TimeOut)
	{
		if(!m_bCancelling || m_taskQueue.GetCurrentTask() == CLobbyTaskQueue::eST_Delete)
		{
			CryLog("CGameLobby::NetworkCallbackReceived() task timed out, restarting");
			m_taskQueue.RestartTask();
		}
		else
		{
			CryLog("CGameLobby::NetworkCallbackReceived() task timed out, finishing because of user initiated cancel");
			m_taskQueue.TaskFinished();
		}
	}
	else
	{
		if (result != eCLE_Success)
		{
			CryLog("CGameLobby::NetworkCallbackReceived() task unsuccessful, result=%u", result);
			if (!m_bMatchmakingSession)
			{
				if(!m_bCancelling)
				{
					if (!(m_bRetryIfPassworded && (result == eCLE_PasswordIncorrect)))
					{
						ShowErrorDialog(result, NULL, NULL, NULL);
					}
				}
			}
		}

		m_taskQueue.TaskFinished();
	}

	
	return (result == eCLE_Success);
}

//-------------------------------------------------------------------------
ECryLobbyError CGameLobby::DoStartSession( ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId, bool &bTaskStartedOut )
{
	CryLog("CGameLobby::DoStartSession() pLobby=%p", this);
	INDENT_LOG_DURING_SCOPE();

	CRY_ASSERT(m_taskQueue.GetCurrentTask() == CLobbyTaskQueue::eST_SessionStart);

	ECryLobbyError result = eCLE_Success;

	if (m_gameLobbyMgr->IsPrimarySession(this))
	{
		result = pMatchMaking->SessionStart(m_currentSession, &taskId, MatchmakingSessionStartCallback, this);

		if(result == eCLE_Success)
		{
#ifdef GAME_IS_CRYSIS2
			CWarningsManager *pWM = g_pGame->GetWarnings();
			if(pWM)
			{
				// can happen on timeout
				if(!pWM->IsWarningActive(pWM->GetWarningId("StartingSession")))
				{
					// SN - Temporarily removing this warning dialogue
					// as it never gets removed again. C3Prep branch has a fix
					// for this and it will shortly be integrated. 
					//pWM->AddWarning("StartingSession", NULL);
				}
			}

			g_pGame->AllowMultiplayerFrontEndAssets(false, true);
#endif
		}
		else if (result != eCLE_TooManyTasks)
		{
			CryLog("[GameLobby] Failed to start StartSession lobby task");
			SessionStartFailed(result);
		}

		bTaskStartedOut = true;
		m_numPlayerJoinResets  = 0;
	}
	else
	{
		CryLog("[GameLobby] Didn't start - waiting till we're the primary session");
		m_taskQueue.AddTask(CLobbyTaskQueue::eST_SessionStart, false);

		bTaskStartedOut = false;
	}

	CryLog("[GameLobby] DoStartSession returning %d (%s)", result, (result == eCLE_Success) ? "success" : "fail");

	return result;
}

//-------------------------------------------------------------------------
ECryLobbyError CGameLobby::DoEndSession( ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId )
{
	CryLog("CGameLobby::DoEndSession() pLobby=%p", this);
	CRY_ASSERT(m_taskQueue.GetCurrentTask() == CLobbyTaskQueue::eST_SessionEnd);

	ECryLobbyError result = pMatchMaking->SessionEnd(m_currentSession, &taskId, MatchmakingSessionEndCallback, this);

	if (result == eCLE_SuccessInvalidSession)
	{
		SessionEndCleanup();
	}
	else if ((result != eCLE_Success) && (result != eCLE_TooManyTasks))
	{
		CryLog("[GameLobby] Failed to start StartSession lobby task");
		SessionEndFailed(result);
	}
		
	return result;
}

//-------------------------------------------------------------------------
ECryLobbyError CGameLobby::DoEnsureBestHost( ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId )
{
	CryLog("CGameLobby::DoEnsureBestHost() pLobby=%p", this);
	CRY_ASSERT(m_taskQueue.GetCurrentTask() == CLobbyTaskQueue::eST_EnsureBestHost);

	ECryLobbyError result = pMatchMaking->SessionEnsureBestHost(m_currentSession, &taskId, MatchmakingEnsureBestHostCallback, this);

	return result;
}

//-------------------------------------------------------------------------
void CGameLobby::CancelSessionInit()
{
	m_taskQueue.ClearNotStartedTasks();	// always clear not started tasks

	if(m_taskQueue.HasTaskInProgress())
	{
		CLobbyTaskQueue::ESessionTask currentTask = m_taskQueue.GetCurrentTask();

		CryLog("CancelSessionInit currentTask %d", currentTask);

		if(currentTask != CLobbyTaskQueue::eST_Create && currentTask != CLobbyTaskQueue::eST_Join && currentTask != CLobbyTaskQueue::eST_Delete)
		{
			CancelLobbyTask(currentTask);
		}
	
		if(currentTask != CLobbyTaskQueue::eST_Delete)
		{	
			m_taskQueue.AddTask(CLobbyTaskQueue::eST_Delete, false);
		}

		m_bCancelling = true;
	}
	else
	{
		CryLog("CancelSessionInit currentSession %d", m_currentSession);

		if(m_currentSession != CrySessionInvalidHandle) // we have a session, get ready to delete it
		{
			m_taskQueue.AddTask(CLobbyTaskQueue::eST_Delete, false);
			m_bCancelling = true;
		}
		else // no session, go to null state
		{
			SetState(eLS_None);
			m_bCancelling = false;
		}
	}

	m_allowRemoveUsers = true;	// cancelling session, user removal authorised
}

//-------------------------------------------------------------------------
bool CGameLobby::IsCreatingOrJoiningSession()
{
	return (m_taskQueue.HasTaskInProgress()) && (m_taskQueue.GetCurrentTask() == CLobbyTaskQueue::eST_Create || m_taskQueue.GetCurrentTask() == CLobbyTaskQueue::eST_Join);
}

//-------------------------------------------------------------------------
ECryLobbyError CGameLobby::DoTerminateHostHintingForGroup( ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId, bool &bTaskStartedOut )
{
	CRY_ASSERT(m_taskQueue.GetCurrentTask() == CLobbyTaskQueue::eST_TerminateHostHinting);

	// Find the players we need to disable hinting for
	int numPlayersThatNeedToLeaveFirst = 0;
	SCryMatchMakingConnectionUID users[MAX_PLAYER_LIMIT];

	const int numRemainingPlayers = m_nameList.Size();
	for (int i = 1; i < numRemainingPlayers; ++ i)
	{
		SSessionNames::SSessionName &player = m_nameList.m_sessionNames[i];
		if (player.m_bMustLeaveBeforeServer)
		{
			users[numPlayersThatNeedToLeaveFirst] = player.m_conId;
			++ numPlayersThatNeedToLeaveFirst;
		}
	}

	if (numPlayersThatNeedToLeaveFirst)
	{
		ECryLobbyError result = pMatchMaking->SessionTerminateHostHintingForGroup(m_currentSession, &users[0], numPlayersThatNeedToLeaveFirst, &taskId, MatchmakingSessionTerminateHostHintingForGroupCallback, this);
		bTaskStartedOut = true;
		return result;
	}
	else
	{
		// No need to do the task, mark it as finished
		taskId = CryLobbyInvalidTaskID;
		bTaskStartedOut = false;
		return eCLE_Success;
	}
}

//-------------------------------------------------------------------------
void CGameLobby::MatchmakingSessionTerminateHostHintingForGroupCallback( CryLobbyTaskID taskID, ECryLobbyError error, void* pArg )
{
	ENSURE_ON_MAIN_THREAD;

	CryLog("CGameLobby::MatchmakingSessionTerminateHostHintingForGroupCallback()");

	CGameLobby *pGameLobby = static_cast<CGameLobby *>(pArg);
	if (pGameLobby->NetworkCallbackReceived(taskID, error))
	{
		pGameLobby->m_isLeaving = true;
		pGameLobby->m_leaveGameTimeout = gl_leaveGameTimeout;

		pGameLobby->CheckCanLeave();
	}
	else if (error != eCLE_TimeOut)
	{
		pGameLobby->LeaveSession(true);
	}
}

//-------------------------------------------------------------------------
void CGameLobby::MatchmakingSessionClosedCallback( UCryLobbyEventData eventData, void *userParam )
{
#ifdef USE_C2_FRONTEND
	MatchmakingSessionClosed(eventData, userParam, CMPMenuHub::eDCE_HostMigrationFailed);
#endif //#ifdef USE_C2_FRONTEND
}

//-------------------------------------------------------------------------
void CGameLobby::MatchmakingSessionKickedCallback( UCryLobbyEventData eventData, void *userParam )
{
#ifdef USE_C2_FRONTEND
	MatchmakingSessionClosed(eventData, userParam, CMPMenuHub::eDCE_Kicked);
#endif //#ifdef USE_C2_FRONTEND
}

//-------------------------------------------------------------------------
void CGameLobby::MatchmakingSessionClosed(UCryLobbyEventData eventData, void *userParam, int reason)
{
	ENSURE_ON_MAIN_THREAD;

	CGameLobby *pGameLobby = static_cast<CGameLobby*>(userParam);
	CRY_ASSERT(pGameLobby);
	if (pGameLobby)
	{
		SCryLobbySessionEventData *pEventData = eventData.pSessionEventData;

		if ((pGameLobby->m_currentSession == pEventData->session) && (pEventData->session != CrySessionInvalidHandle))
		{
			CryLog("CGameLobby::MatchmakingSessionClosed() received SessionClosed event, leaving session");
			pGameLobby->ConnectionFailed(reason);

#if INCLUDE_DEDICATED_LEADERBOARDS
#ifdef USE_C2_FRONTEND
			CFlashFrontEnd *pFFE = g_pGame->GetFlashMenu();
			if(pFFE)
			{
				pFFE->ClearDelaySessionLeave();

				CMPMenuHub *pMPMH = pFFE->GetMPMenu();
				if(pMPMH)
				{
					pMPMH->ClearDelaySessionLeave();
				}
			}
#endif //#ifdef USE_C2_FRONTEND
#endif
		}
	}
}

//-------------------------------------------------------------------------
void CGameLobby::MatchmakingForcedFromRoomCallback(UCryLobbyEventData eventData, void *pArg)
{
	ENSURE_ON_MAIN_THREAD;

	CGameLobby *pGameLobby = static_cast<CGameLobby*>(pArg);
	if (pGameLobby)
	{
		SCryLobbyForcedFromRoomData *pEventData = eventData.pForcedFromRoomData;

		CryLog("CGameLobby::ForcedFromRoomCallback session %d reason %d", (int)pEventData->m_session, pEventData->m_why);

		if (pEventData->m_session != CrySessionInvalidHandle && pGameLobby->m_currentSession == pEventData->m_session)
		{
			CryLog("[game] received eCLSE_ForcedFromRoom event with reason %d, leaving session", pEventData->m_why);

			// if in game, then this will effectively tell the user that the connection to
			// their game has been lost, if in the lobby, then it will try and find a new session
#ifdef USE_C2_FRONTEND
			pGameLobby->ConnectionFailed(CMPMenuHub::eDCE_HostMigrationFailed);
#endif //#ifdef USE_C2_FRONTEND
		}
	}
}

//-------------------------------------------------------------------------
void CGameLobby::RequestDetailedServerInfo(CrySessionID sessionId, EDetailedSessionInfoResponseFlags flags)
{
	memset(&m_detailedServerInfo, 0, sizeof(m_detailedServerInfo));
	m_detailedServerInfo.m_sessionId = sessionId;
	m_detailedServerInfo.m_taskID = CryLobbyInvalidTaskID;
	m_detailedServerInfo.m_flags = flags;

	m_taskQueue.AddTask(CLobbyTaskQueue::eST_SessionRequestDetailedInfo, true);
}

ECryLobbyError CGameLobby::DoSessionDetailedInfo( ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId )
{
	CryLog("CGameLobby::DoSessionDetailedInfo");

	CRY_ASSERT(m_taskQueue.GetCurrentTask() == CLobbyTaskQueue::eST_SessionRequestDetailedInfo);

	ECryLobbyError detailedInfoResult = eCLE_ServiceNotSupported;
	CCryLobbyPacket packet;
	const uint32 bufferSize = CryLobbyPacketHeaderSize + CryLobbyPacketUINT8Size;

	if (packet.CreateWriteBuffer(bufferSize))
	{
		packet.StartWrite(eGUPD_DetailedServerInfoRequest, true);
		//-- eDSIRF_IncludePlayers means include player names in the response.
		//-- eDSIRF_IncludeCustomFields means include custom fields in response (but only if server is custom mode).
		//-- clear the flag to not return players.
		packet.WriteUINT8(m_detailedServerInfo.m_flags);	

		detailedInfoResult = pMatchMaking->SessionSendRequestInfo(&packet, m_detailedServerInfo.m_sessionId, &taskId, CGameLobby::MatchmakingSessionDetailedInfoResponseCallback, this);
		if(detailedInfoResult != eCLE_Success)
		{
			taskId = CryLobbyInvalidTaskID;
		}
		else
		{
			m_detailedServerInfo.m_taskID = taskId;
		}
	}
	else
	{
		detailedInfoResult = eCLE_OutOfMemory;
	}

	CryLog("DoSessionInfo %s with error %d", detailedInfoResult == eCLE_Success ? "succeeded" : "failed", detailedInfoResult);

	return detailedInfoResult;
}

void CGameLobby::MatchmakingSessionDetailedInfoRequestCallback(UCryLobbyEventData eventData, void *pArg)
{
	ENSURE_ON_MAIN_THREAD;

	//-- Server receives eGUPD_DetailedServerInfoRequest packet and generates eCLSE_SessionRequestInfo event
	//-- which calls this function.
	//-- This function creates a eGUPD_DetailedServerInfoResponse response packet and punts the results back to the client.

	CGameLobby *pGameLobby = static_cast<CGameLobby*>(pArg);
	if (pGameLobby)
	{
		SCryLobbySessionRequestInfo* pEventData = eventData.pSessionRequestInfo;

		CryLog("CGameLobby::MatchmakingSessionDetailedInfoRequestCallback session %d", (int)pEventData->session);

		if (pEventData->session != CrySessionInvalidHandle && pGameLobby->m_currentSession == pEventData->session)
		{
			CCryLobbyPacket* pRequest = pEventData->pPacket;
			if (pRequest)
			{
				pRequest->StartRead();
				uint8 flags = pRequest->ReadUINT8();

				uint8 playerCount = (uint8)MIN(DETAILED_SESSION_MAX_PLAYERS, pGameLobby->m_nameList.Size());

				uint32 numCustoms = 0;
#ifdef GAME_IS_CRYSIS2
				CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
				if (pPlaylistManager)
				{
					CRY_ASSERT(pPlaylistManager->GetGameModeOptionCount() <= DETAILED_SESSION_MAX_CUSTOMS);
					numCustoms = MIN(DETAILED_SESSION_MAX_CUSTOMS, pPlaylistManager->GetGameModeOptionCount());
				}
#endif

				CCryLobbyPacket packet;
				uint32 bufferSize = CryLobbyPacketHeaderSize + CryLobbyPacketUINT8Size;
				bufferSize += CryLobbyPacketUINT8Size * DETAILED_SESSION_INFO_MOTD_SIZE;
				bufferSize += CryLobbyPacketUINT8Size * DETAILED_SESSION_INFO_URL_SIZE;
				if (flags & eDSIRF_IncludePlayers)
				{
					bufferSize += CryLobbyPacketUINT8Size;	//-- count
					bufferSize += CryLobbyPacketUINT8Size * playerCount * CRYLOBBY_USER_NAME_LENGTH; //-- names
				}
				if (flags & eDSIRF_IncludeCustomFields) 
				{
					bufferSize += CryLobbyPacketUINT8Size;  //-- count
					bufferSize += CryLobbyPacketUINT16Size * numCustoms; //-- custom fields
				}

				if (packet.CreateWriteBuffer(bufferSize))
				{
					char motd[DETAILED_SESSION_INFO_MOTD_SIZE] = {0};
					char url[DETAILED_SESSION_INFO_URL_SIZE] = {0};

					strncpy(motd, g_pGameCVars->g_messageOfTheDay->GetString(), DETAILED_SESSION_INFO_MOTD_SIZE);
					strncpy(url, g_pGameCVars->g_serverImageUrl->GetString(), DETAILED_SESSION_INFO_URL_SIZE);

					motd[DETAILED_SESSION_INFO_MOTD_SIZE-1] = 0;
					url[DETAILED_SESSION_INFO_URL_SIZE-1] = 0;

					packet.StartWrite(eGUPD_DetailedServerInfoResponse, true);
					packet.WriteUINT8(flags);
					packet.WriteString(motd, DETAILED_SESSION_INFO_MOTD_SIZE);
					packet.WriteString(url, DETAILED_SESSION_INFO_URL_SIZE);

					if (flags & eDSIRF_IncludePlayers)
					{
						CryLog("  Including %d Players...", playerCount);
						packet.WriteUINT8(playerCount);
						for (uint32 i = 0; i < playerCount; i++)
						{
							packet.WriteString(pGameLobby->m_nameList.m_sessionNames[i].m_name, CRYLOBBY_USER_NAME_LENGTH);
						}
					}

					if (flags & eDSIRF_IncludeCustomFields)
					{
#ifdef GAME_IS_CRYSIS2
						if (pPlaylistManager)
						{
							CryLog("  Including %d Custom Fields...", numCustoms);
							packet.WriteUINT8((uint8)numCustoms);
							for (uint32 j = 0; j < numCustoms; j++)
							{
								uint16 value = pPlaylistManager->PackCustomVariantOption(j);
								packet.WriteUINT16(value);
							}
						}
						else
#endif
						{
							packet.WriteUINT8(0);
						}
					}

					if (ICryMatchMaking* pMatchMaking = gEnv->pNetwork->GetLobby()->GetMatchMaking())
					{
						pMatchMaking->SessionSendRequestInfoResponse(&packet, pEventData->requester);
					}

					packet.FreeWriteBuffer();
				}
			}
		}
	}
}

#if USE_CRYLOBBY_GAMESPY
//-------------------------------------------------------------------------
// Checks for server image display, starts download if out of date or not found
void CGameLobby::CheckGetServerImage()
{
	SDetailedServerInfo* pDetails = &m_detailedServerInfo;

	// Needs a valid server id & url
	if (pDetails->m_url && pDetails->m_url[0]!=0)
	{
#ifdef USE_C2_FRONTEND
		CMPMenuHub *pMPMenu = CMPMenuHub::GetMPMenuHub();
		CServerImageCache *pServerImageCache = pMPMenu ? pMPMenu->GetServerImageCache() : NULL;
		if (pServerImageCache)
		{
			CryHashStringId urlHash = pDetails->m_url;
			pServerImageCache->GetServerImageFromCache(urlHash, pDetails->m_url, true, NULL); // Downloads image if not found
		}
#endif //#ifdef USE_C2_FRONTEND
	}
}
#endif

//-------------------------------------------------------------------------
// Process a detailed server info packet
void CGameLobby::MatchmakingSessionDetailedInfoResponseCallback(CryLobbyTaskID taskID, ECryLobbyError error, CCryLobbyPacket* pPacket, void* pArg)
{
	ENSURE_ON_MAIN_THREAD;

	//-- Called on the client when eGUPD_DetailedServerInfoResponse packet is received.
	//-- Should put the response details into a cache somewhere for Flash to query

	CryLog("MatchmakingSessionDetailedInfoResponseCallback error %d packet %p", error, pPacket);

	CGameLobby *pGameLobby = static_cast<CGameLobby*>(pArg);
	if (error == eCLE_Success)
	{
		if (pPacket)
		{
			CryLog("MatchmakingSessionDetailedInfoResponseCallback with valid packet!");

			if (pGameLobby)
			{
				SDetailedServerInfo* pDetails = &pGameLobby->m_detailedServerInfo;

				if(pDetails->m_taskID == taskID)
				{
					CryLog("processing detailed info response callback");

					pPacket->StartRead();
					uint8 flags = pPacket->ReadUINT8();
					pPacket->ReadString(pDetails->m_motd, DETAILED_SESSION_INFO_MOTD_SIZE);
					pDetails->m_motd[DETAILED_SESSION_INFO_MOTD_SIZE - 1] = 0;
					pPacket->ReadString(pDetails->m_url, DETAILED_SESSION_INFO_URL_SIZE);
					pDetails->m_url[DETAILED_SESSION_INFO_URL_SIZE - 1] = 0;

					CryLog("  MOTD: %s", pDetails->m_motd);
					CryLog("  URL: %s", pDetails->m_url);

					int nCustoms = 0;

					if (flags & eDSIRF_IncludePlayers)
					{
						uint32 nCount = (uint32)pPacket->ReadUINT8();
						nCount = MIN(nCount, DETAILED_SESSION_MAX_PLAYERS);
						CryLog("  PlayerCount %d:", nCount);

						pDetails->m_namesCount = nCount;
						for (uint32 i = 0; i < nCount; i++)
						{
							pPacket->ReadString(pDetails->m_names[i], CRYLOBBY_USER_NAME_LENGTH);
							pDetails->m_names[i][CRYLOBBY_USER_NAME_LENGTH - 1] = 0;	

							CryLog("    Name %d: %s", i, pDetails->m_names[i]);
						}
					}
					if (flags & eDSIRF_IncludeCustomFields)
					{
						nCustoms = (uint32)pPacket->ReadUINT8();
						nCustoms = MIN(nCustoms, DETAILED_SESSION_MAX_CUSTOMS);
						CryLog("  CustomCount %d:", nCustoms);

						for (uint32 i = 0; i < (uint32)nCustoms; i++)
						{
							pDetails->m_customs[i] = pPacket->ReadUINT16();

							CryLog("    Custom %d: %d", i, pDetails->m_customs[i]);
						}

#ifdef GAME_IS_CRYSIS2
						CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
						if(pPlaylistManager)
						{
							pPlaylistManager->ReadDetailedServerInfo(pDetails->m_customs, nCustoms);
						}
#endif
					}

#ifdef USE_C2_FRONTEND
					CFlashFrontEnd *pFlashMenu = g_pGame->GetFlashMenu();
					if (pFlashMenu != NULL && pFlashMenu->GetCurrentMenuScreen()==eFFES_lobby_browser)
					{
						CMPMenuHub *pMPMenu = CMPMenuHub::GetMPMenuHub();
						CUIServerList *pServerList = pMPMenu ? pMPMenu->GetServerListUI() : NULL;
						if (pServerList)
						{
							pServerList->UpdateExtendedInfoPopUp(pFlashMenu->GetFlash(), pDetails, nCustoms);
						}
					}
#endif //#ifdef USE_C2_FRONTEND

				}
				else
				{
					CryLog("ignoring detailed info response callback as taskIds do not match, assuming we no longer care about it");
				}
			}
		}
	}
	else
	{
		CryLog("Error on receiving SessionInfo response. (%d)", error);

		if (pGameLobby)
		{
			SDetailedServerInfo* pDetails = &pGameLobby->m_detailedServerInfo;

			if(pDetails->m_taskID == taskID)
			{
#ifdef USE_C2_FRONTEND
				CFlashFrontEnd *pFlashMenu = g_pGame->GetFlashMenu();
				if (pFlashMenu != NULL && pFlashMenu->GetCurrentMenuScreen()==eFFES_lobby_browser)
				{
					CMPMenuHub *pMPMenu = CMPMenuHub::GetMPMenuHub();
					CUIServerList *pServerList = pMPMenu ? pMPMenu->GetServerListUI() : NULL;
					if (pServerList)
					{
						pServerList->FailedGettingExtendedDetails(pFlashMenu->GetFlash(), pDetails->m_sessionId);
					}
				}
#endif //#ifdef USE_C2_FRONTEND
			}
		}
	}
}

//-------------------------------------------------------------------------
bool CGameLobby::OnTerminate( SHostMigrationInfo& hostMigrationInfo, uint32& state )
{
	if (hostMigrationInfo.m_session == m_currentSession)
	{
		CryLog("CGameLobby::OnTerminate() host migration failed, leaving session");
#ifdef USE_C2_FRONTEND
		ConnectionFailed(CMPMenuHub::eDCE_HostMigrationFailed);
#endif //#ifdef USE_C2_FRONTEND
	}

	if( m_bMatchmakingSession )
	{
		if( CMatchMakingHandler* pMatchmakingHandler = m_gameLobbyMgr->GetMatchMakingHandler() )
		{
			pMatchmakingHandler->OnHostMigrationFinished( false, false );
		}
	}

	return true;
}

//-------------------------------------------------------------------------
bool CGameLobby::OnReset(SHostMigrationInfo& hostMigrationInfo, uint32& state)
{
	return true;
}

//-------------------------------------------------------------------------
void CGameLobby::ConnectionFailed(int reason)
{
	const ELobbyState currentState = m_state;

	if (currentState == eLS_Leaving)
	{
		CryLog("CGameLobby::ConnectionFailed() already in leaving state, ignoring");
		return;
	}

	CryLog("CGameLobby::ConnectionFailed()");

	if(!m_bMatchmakingSession && !IsQuitting())
	{
#ifdef USE_C2_FRONTEND
		CMPMenuHub* pMPMenu = CMPMenuHub::GetMPMenuHub();
		if(pMPMenu)
		{
			pMPMenu->SetDisconnectError((CMPMenuHub::EDisconnectError)reason, true);
		}
#endif //#ifdef USE_C2_FRONTEND
	}

	LeaveSession(true);

}

//-------------------------------------------------------------------------
#ifdef USE_C2_FRONTEND
void CGameLobby::ResetFlashInfos()
{
	m_votingFlashInfo.Reset();
	m_votingCandidatesFlashInfo.Reset();
}
#endif //#ifdef USE_C2_FRONTEND

//-------------------------------------------------------------------------
ECryLobbyError CGameLobby::DoSessionSetLocalFlags( ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId )
{
	CRY_ASSERT(m_taskQueue.GetCurrentTask() == CLobbyTaskQueue::eST_SessionSetLocalFlags);

	ECryLobbyError result = pMatchMaking->SessionSetLocalFlags(m_currentSession, CRYSESSION_LOCAL_FLAG_HOST_MIGRATION_CAN_BE_HOST, &taskId, MatchmakingSessionSetLocalFlagsCallback, this);
	return result;
}

//-------------------------------------------------------------------------
void CGameLobby::MatchmakingSessionSetLocalFlagsCallback( CryLobbyTaskID taskID, ECryLobbyError error, CrySessionHandle h, uint32 flags, void* pArg )
{
	ENSURE_ON_MAIN_THREAD;

	CGameLobby *pGameLobby = static_cast<CGameLobby*>(pArg);

	CryLog("CGameLobby::MatchmakingSessionSetLocalFlagsCallback() error=%u, pGameLobby=%p", error, pGameLobby);
	
	pGameLobby->NetworkCallbackReceived(taskID, error);
}

//-------------------------------------------------------------------------
void CGameLobby::OnHaveLocalPlayer()
{
	CryLog("CGameLobby::OnHaveLocalPlayer() m_bNeedToSetAsElegibleForHostMigration=%s", m_bNeedToSetAsElegibleForHostMigration ? "true" : "false");
	if (m_bNeedToSetAsElegibleForHostMigration)
	{
		m_taskQueue.AddTask(CLobbyTaskQueue::eST_SessionSetLocalFlags, false);
		m_bNeedToSetAsElegibleForHostMigration = false;
	}
}

//-------------------------------------------------------------------------
void CGameLobby::CmdAdvancePlaylist( IConsoleCmdArgs *pArgs )
{
	CGameLobby *pGameLobby = g_pGame->GetGameLobby();
	if (pGameLobby)
	{
		pGameLobby->DebugAdvancePlaylist();
	}
}

//-------------------------------------------------------------------------
void CGameLobby::DebugAdvancePlaylist()
{
	if ((m_currentSession != CrySessionInvalidHandle) && (m_bMatchmakingSession))
	{
		if (m_server)
		{
			CryLog("CGameLobby::DebugAdvancePlaylist()");
			UpdateLevelRotation();
			SvResetVotingForNextElection();
			m_bPlaylistHasBeenAdvancedThroughConsole = true;		// This is nasty but we can't pass arguments into the SendPacket function :-(
			CryLog("[tlh] calling SendPacket(eGUPD_SyncPlaylistRotation) [2]");
			SendPacket(eGUPD_SyncPlaylistRotation);
			m_bPlaylistHasBeenAdvancedThroughConsole = false;
		}
		else
		{
			SendPacket(eGUPD_RequestAdvancePlaylist);
		}
	}
}

//-------------------------------------------------------------------------
void CGameLobby::SetLocalVoteStatus( ELobbyVoteStatus state )
{
	if (m_localVoteStatus != state)
	{
		m_localVoteStatus = state;

		// Distinguish between networked vote status and local vote status so we don't send multiple updates
		// without actually changing anything ("waiting for candidates" to "not voted" for instance)
		ELobbyNetworkedVoteStatus networkedVoteState = eLNVS_NotVoted;
		if (state == eLVS_votedLeft)
		{
			networkedVoteState = eLNVS_VotedLeft;
		}
		else if (state == eLVS_votedRight)
		{
			networkedVoteState = eLNVS_VotedRight;
		}

		if (m_networkedVoteStatus != networkedVoteState)
		{
			m_networkedVoteStatus = networkedVoteState;
			if (m_currentSession != CrySessionInvalidHandle)
			{
				if (!m_bCancelling)
				{
					m_taskQueue.AddTask(CLobbyTaskQueue::eST_SetLocalUserData, true);
				}
			}
		}
	}
}

//-------------------------------------------------------------------------
ELobbyNetworkedVoteStatus CGameLobby::GetVotingStateForPlayer(SSessionNames::SSessionName *pSessionName) const
{
	ELobbyNetworkedVoteStatus result = eLNVS_NotVoted;
	if (pSessionName)
	{
		uint8 reincarnationAndVote = pSessionName->m_userData[eLUD_ReincarnationsAndVoteChoice];
		uint8 voteStatus = (reincarnationAndVote & 0xF);

		result = (ELobbyNetworkedVoteStatus) voteStatus;
	}
	return result;
}

//-------------------------------------------------------------------------
bool CGameLobby::CalculateVotes()
{
	if (m_votingEnabled)
	{
		if (!m_votingClosed)
		{
			int numVotesForLeft = 0;
			int numVotesForRight = 0;

			const unsigned int numPlayers = m_nameList.Size();
			for (unsigned int i = 0; i < numPlayers; ++ i)
			{
				SSessionNames::SSessionName *pPlayer = &m_nameList.m_sessionNames[i];
				const ELobbyNetworkedVoteStatus votingStatus = GetVotingStateForPlayer(pPlayer);
				if (votingStatus == eLNVS_VotedLeft)
				{
					++ numVotesForLeft;
				}
				else if (votingStatus == eLNVS_VotedRight)
				{
					++ numVotesForRight;
				}
			}

			if ((numVotesForLeft > m_highestLeadingVotesSoFar) && (numVotesForLeft > numVotesForRight))
			{
				m_highestLeadingVotesSoFar = numVotesForLeft;
				m_leftHadHighestLeadingVotes = true;
			}
			else if ((numVotesForRight > m_highestLeadingVotesSoFar) && (numVotesForRight > numVotesForLeft))
			{
				m_highestLeadingVotesSoFar = numVotesForRight;
				m_leftHadHighestLeadingVotes = false;
			}

			if ((m_leftVoteChoice.m_numVotes != numVotesForLeft) || (m_rightVoteChoice.m_numVotes != numVotesForRight))
			{
				m_leftVoteChoice.m_numVotes = numVotesForLeft;
				m_rightVoteChoice.m_numVotes = numVotesForRight;
				return true;
			}
		}
	}
	return false;
}

//-------------------------------------------------------------------------
void CGameLobby::CheckForVotingChanges(bool bUpdateFlash)
{
	if (CalculateVotes() && bUpdateFlash)
	{
		if (m_state == eLS_Lobby)
		{
#ifdef USE_C2_FRONTEND
			UpdateVotingInfoFlashInfo();
#endif //#ifdef USE_C2_FRONTEND
		}
	}
}

//-------------------------------------------------------------------------
void CGameLobby::ResetLocalVotingData()
{
	SetLocalVoteStatus(eLVS_awaitingCandidates);		// Reset our vote so that our initial user data update will not have a vote in it
	m_leftVoteChoice.Reset();
	m_rightVoteChoice.Reset();
	m_highestLeadingVotesSoFar = 0;
	m_leftHadHighestLeadingVotes = false;
#ifdef USE_C2_FRONTEND
	UpdateVotingInfoFlashInfo();
#endif //#ifdef USE_C2_FRONTEND
}

//-------------------------------------------------------------------------
void CGameLobby::ClearBadServers()
{
	CryLog("[GameLobby] %p ClearBadServers", this);

	m_badServers.clear();
}

//static------------------------------------------------------------------------
void CGameLobby::ReadOnlineDataCallback(CryLobbyTaskID taskID, ECryLobbyError error, SCryLobbyUserData* pData, uint32 numData, void* pArg)
{
	ENSURE_ON_MAIN_THREAD;

#if INCLUDE_DEDICATED_LEADERBOARDS
	CryLog("[GameLobby] ReadOnlineCallback %s with error %d", (error == eCLE_Success || error == eCLE_ReadDataNotWritten) ? "succeeded" : "failed", error);

	CGameLobby *pGameLobby = (CGameLobby*)pArg;
	SOnlineAttributeTask *pTask = pGameLobby->FindOnlineAttributeTask(taskID);
	SSessionNames::SSessionName *pSessionName = pTask ? pGameLobby->m_nameList.GetSessionNameByUserId(pTask->m_user) : NULL;
	IPlayerProfileManager *pPlayerProfileManager = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager();

	CRY_ASSERT(pGameLobby);
	CRY_ASSERT(gEnv->IsDedicated());	// only the dedicated server should be getting here
	CRY_ASSERT(pTask);

	bool bAllowUserToJoinGame = true;

	if(pSessionName)
	{
		CRY_ASSERT(pSessionName->m_onlineStatus == eOAS_Uninitialised);

		bool validData = (error == eCLE_Success);
		
		int profileVersionIdx = 0;
		int statsVersionIdx = 0;

		if(validData)
		{
			if(pPlayerProfileManager)
			{
				profileVersionIdx = pPlayerProfileManager->GetOnlineAttributeIndexByName("OnlineAttributes/version");
				statsVersionIdx = pPlayerProfileManager->GetOnlineAttributeIndexByName("MP/PersistantStats/Version");

				if((profileVersionIdx >= 0) && (pData[profileVersionIdx].m_int32 != pPlayerProfileManager->GetOnlineAttributesVersion()))
				{
					CryLog("  player profile version mismatch");
					validData = false;
				}
				else
				{
					if(!pPlayerProfileManager->ValidChecksums(pData, numData))
					{
						CryLog("  online attributes not valid, wrong checksums");
						validData = false;
					}
					else
					{
						if((statsVersionIdx >= 0) && (pData[statsVersionIdx].m_int32 != CPersistantStats::GetInstance()->GetOnlineAttributesVersion()))
						{
							CryLog("  persistant stats version mismatch");
							validData = false;
						}
					}
				}

				if(!validData)
				{
					error = eCLE_ReadDataCorrupt;
				}
			}
		}

		if(validData)
		{
			CryLog("  valid online data found setting...");

			pSessionName->SetOnlineData(pData, numData);
			pSessionName->m_onlineStatus = eOAS_Initialised;
			pGameLobby->SendPacket(eGUPD_StartOfGameUserStats, pSessionName->m_conId);
		}
		else if(error == eCLE_ReadDataNotWritten || error == eCLE_ReadDataCorrupt)
		{
			CryLog("[GameLobby] no or invalid user data, set to default");

			if(pPlayerProfileManager)
			{
				SCryLobbyUserData *pUserData = pSessionName->m_onlineData;
				uint32 count = pPlayerProfileManager->GetOnlineAttributes(pUserData, MAX_ONLINE_STATS_SIZE); // this will give us our data format, need a better way of doing this really
			
				CRY_ASSERT(count == pPlayerProfileManager->GetOnlineAttributeCount());

				// 0 the data, for sanities sake
				for(uint32 i = 0; i < count; ++i)
				{
					switch(pUserData[i].m_type)
					{
						case eCLUDT_Int8:
							pUserData[i].m_int8 = 0;
							break;
						case eCLUDT_Int16:
							pUserData[i].m_int16 = 0;
							break;
						case eCLUDT_Int32:
							pUserData[i].m_int32 = 0;
							break;
						case eCLUDT_Float32:
							pUserData[i].m_f32 = 0.f;
							break;
						default:
							CRY_ASSERT_MESSAGE(0, string().Format("Unknown data type in online attribute data", pUserData[i].m_type));
							break;
					}
				}

				// need to init version numbers or we are just sending 0's
				if(profileVersionIdx >= 0)
				{
					CRY_ASSERT(pUserData[profileVersionIdx].m_type == eCLUDT_Int32);
					pUserData[profileVersionIdx].m_int32 = pPlayerProfileManager->GetOnlineAttributesVersion();
				}

				if(statsVersionIdx >= 0)
				{
					CRY_ASSERT(pUserData[statsVersionIdx].m_type == eCLUDT_Int32);
					pUserData[statsVersionIdx].m_int32 = CPersistantStats::GetInstance()->GetOnlineAttributesVersion();
				}
			
				pSessionName->m_onlineDataCount = count;
				pSessionName->m_onlineStatus = eOAS_Initialised;

				// need to apply checksums
				pPlayerProfileManager->ApplyChecksums(pSessionName->m_onlineData, pSessionName->m_onlineDataCount);
			
				pGameLobby->SendPacket(eGUPD_StartOfGameUserStats, pSessionName->m_conId);
			}
		}
		else
		{
			// if not timeout, then we'll try and requeue for later, if it is timeout
			// then we just need to stop the user from joining, the task will be restarted for us
			if(error != eCLE_TimeOut)
			{
				pSessionName->m_numGetOnlineDataRetries++;

				CryLog("[GameLobby] ReadOnlineAttributes has returned a fatal error (%u) for user %s numAttempts %u, adding user back into queue and refusing entry to game", error, pSessionName->m_name, pSessionName->m_numGetOnlineDataRetries);

				if((gl_maxGetOnlineDataRetries <= 0) || (pSessionName->m_numGetOnlineDataRetries < gl_maxGetOnlineDataRetries))
				{
					pGameLobby->AddOnlineAttributeTask(pSessionName->m_userId, eOATT_ReadUserData);
				}
				else
				{
					pGameLobby->SendPacket(eGUPD_FailedToReadOnlineData, pSessionName->m_conId);
				}
			}

			bAllowUserToJoinGame = false;
		}
	}
	else
	{
		CryLog("[GameLobby] Failed to find session name for user, not setting online attributes, forcing error to eCLE_InternalError to clear out task");
		error = eCLE_InternalError;
	}

	if(pGameLobby->OnlineAttributeTaskFinished(taskID, error))
	{
		CryLog("[GameLobby] send join game/lobby to user");
		
		if ((pGameLobby->m_state == eLS_Game) && pSessionName && bAllowUserToJoinGame)
		{
			pGameLobby->SendPacket(eGUPD_LobbyGameHasStarted, pSessionName->m_conId);
		}
	}
#endif
}

//static------------------------------------------------------------------------
void CGameLobby::WriteOnlineDataCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pArg)
{
	ENSURE_ON_MAIN_THREAD;

#if INCLUDE_DEDICATED_LEADERBOARDS
	CryLog("[GameLobby] WriteOnlineDataCallback %s with error %d", (error == eCLE_Success) ? "succeeded" : "failed", error);

	CGameLobby *pGameLobby = (CGameLobby*)pArg;

	// if succeeded or error is something other than timeout, then
	// remove the data from the list, if timeout, then potentally
	// we can try again
	if(error == eCLE_Success || error != eCLE_TimeOut)
	{
		int numDatasToRemove = MIN(MAX_WRITEUSERDATA_USERS, pGameLobby->m_writeUserData.size());
		for(int i = 0; i < numDatasToRemove; i++)
		{
			SWriteUserData *pData = &pGameLobby->m_writeUserData[0];
			SWriteUserData dataCopy;
			bool requeue = pData->m_requeue;

			if(requeue)
			{
				CryLog("  copying data as needs requeue");

				dataCopy.m_userID = pData->m_userID;
				dataCopy.SetData(pData->m_data, pData->m_numData);
				dataCopy.m_writing = false;
				dataCopy.m_requeue = false;
			}

			pGameLobby->m_writeUserData.removeAt(0);

			if(requeue)
			{
				CryLog("  requeuing this data as updated during write");
				pGameLobby->m_writeUserData.push_back(dataCopy);
			}
		}
	}
	pGameLobby->m_WriteUserDataTaskID = CryLobbyInvalidTaskID;
#endif
}

void CGameLobby::ClearWriteLeaderboardData()
{
#if INCLUDE_DEDICATED_LEADERBOARDS
	// N.B. If CPersistantStats::WriteToDedicatedLeaderBoards() ever writes more than 1 user's data
	// at a time, then this needs to reverse iterate over the data to minimise the amount of copying
	CryLog("[ARC] write leaderboard data vector has %i elements - cleaning", m_writeLeaderboardData.size());
	uint32 index = 0;
	while (index < m_writeLeaderboardData.size())
	{
		if (!(m_writeLeaderboardData[index].m_dirty))
		{
			m_writeLeaderboardData.removeAt(index);
		}
		else
		{
			++index;
		}
	}
	CryLog("[ARC] write leaderboard data vector now has %i elements", m_writeLeaderboardData.size());
#endif
}

#if INCLUDE_DEDICATED_LEADERBOARDS
void CGameLobby::WritePendingLeaderboardData()
{
	// we use the server id here so that we don't loose the task
	// if any clients disconnect, the server will do all clients data
	// in bulk
	CryUserID serverID = gEnv->pNetwork->GetLobby()->GetLobbyService()->GetUserID(0);
	AddOnlineAttributeTask(serverID, eOATT_WriteToLeaderboards);
}
#endif

bool CGameLobby::AllowOnlineAttributeTasks()
{
#if INCLUDE_DEDICATED_LEADERBOARDS
	return IsUsingDedicatedLeaderboards() && (gEnv->IsDedicated()) && (IsServer());
#else
	return false;
#endif
}

void CGameLobby::AddOnlineAttributeTask(CryUserID id, EOnlineAttributeTaskType type)
{
#if INCLUDE_DEDICATED_LEADERBOARDS
	if(!AllowOnlineAttributeTasks())
	{
		return;
	}

	m_onlineAttributeTasks.push_back(SOnlineAttributeTask(id, CryLobbyInvalidTaskID, type, eOATS_NotStarted));	
#endif
}

int CGameLobby::FindWriteUserDataByUserID(CryUserID id)
{
	int result = -1;

#if INCLUDE_DEDICATED_LEADERBOARDS
	int size = m_writeUserData.size();
	
	for(int i = 0; i < size; i++)
	{
		if(m_writeUserData[i].m_userID == id)
		{
			result = i;
			break;
		}
	}
#endif

	return result;
}

void CGameLobby::TickWriteUserData(float dt)
{
#if INCLUDE_DEDICATED_LEADERBOARDS
	int numUserDatas = m_writeUserData.size();

	float wudTime = m_writeUserDataTimer + dt;
	m_writeUserDataTimer = wudTime;

	if(m_WriteUserDataTaskID == CryLobbyInvalidTaskID && numUserDatas > 0 && wudTime >= g_pGameCVars->g_writeUserDataInterval)
	{
		const uint32 numUsers = MAX_WRITEUSERDATA_USERS;

		uint32 userCount = 0;
		uint32 numData[numUsers];
		CryUserID userIDs[numUsers];
		SCryLobbyUserData onlineData[numUsers][MAX_ONLINE_STATS_SIZE];
		SCryLobbyUserData *pOnlineDataStart[numUsers];

		for(int x = 0; x < numUsers; ++x)
		{
			pOnlineDataStart[x] = &onlineData[x][0];
		}

		uint32 limit = MIN(MAX_WRITEUSERDATA_USERS, m_writeUserData.size());
		uint32 i = 0;
		for(i = 0; i < limit; ++i)
		{
			SWriteUserData *userData = &m_writeUserData[i];

			CRY_ASSERT(sizeof(SCryLobbyUserData) * userData->m_numData <= sizeof(userData->m_data));
			CRY_ASSERT(userData->m_numData <= MAX_ONLINE_STATS_SIZE);
			// assert online data count is same as player profile manager

			userIDs[userCount] = userData->m_userID;
			numData[userCount] = userData->m_numData;
			memcpy(&onlineData[userCount][0], userData->m_data, min(sizeof(SCryLobbyUserData) * MAX_ONLINE_STATS_SIZE, sizeof(SCryLobbyUserData) * userData->m_numData));
			++userCount;
		}

		ICryStats* pStats = gEnv->pNetwork->GetLobby()->GetStats();
		ECryLobbyError error = pStats ? pStats->StatsWriteUserData(userIDs, &pOnlineDataStart[0], numData, userCount, &m_WriteUserDataTaskID, CGameLobby::WriteOnlineDataCallback, this) : eCLE_ServiceNotSupported;
		if(error != eCLE_Success)	// if too many task, can maybe try again in a bit
		{
			CryLog("TickWriteUserData failed with error %d", error);
			
			if(error != eCLE_TooManyTasks)
			{
				// removing the failed users, really, the only way this can happen
				// is because we're out of memory or data format mismatch
				for(i = 0; i < limit; i++)
				{
					m_writeUserData.removeAt(0);	
				}

				m_WriteUserDataTaskID = CryLobbyInvalidTaskID;	// damn you gamespy
			}
		}
		else
		{
			// success, mark as in progress
			for(i = 0; i < limit; i++)
			{
				m_writeUserData[i].m_writing = true;	
			}
		}

		m_writeUserDataTimer = 0.f;
	}
#endif
}

void CGameLobby::TickOnlineAttributeTasks()
{
	// the static analyser does not like this on 360 profile, it should
	// never be called on the consoles anyway, so #if'ing here
#if INCLUDE_DEDICATED_LEADERBOARDS
	if(!AllowOnlineAttributeTasks())
	{
		return;
	}

	int numTasks = m_onlineAttributeTasks.size();

	if(numTasks > 0 && !IsOnlineAttributeTaskInProgress())	// process tasks in FIFO order
	{
		SOnlineAttributeTask *pTask = &m_onlineAttributeTasks[0];
		ECryLobbyError error = eCLE_Success;	// optimistic?
		CryLobbyTaskID taskID = CryLobbyInvalidTaskID;
		bool taskStarted = false;

		CRY_ASSERT(pTask->m_status == eOATS_NotStarted);

		switch(pTask->m_type)
		{
			case eOATT_ReadUserData:
				{
					int limit = m_writeUserData.size();
					for(int i = 0; i < limit; i++)
					{
						if (m_writeUserData[i].m_writing)
						{
							if (pTask->m_user == m_writeUserData[i].m_userID)
							{
								// Can't read yet
								CryLog("can't read because we're still writing");
								return;
							}
						}
					}
					ICryStats* stats = gEnv->pNetwork->GetLobby()->GetStats();
					if(stats)
					{
						error = stats->StatsReadUserData(0, pTask->m_user, &taskID, CGameLobby::ReadOnlineDataCallback, this);
						if(error == eCLE_Success)
						{
							taskStarted = true;
						}
					}
					break;
				}

			case eOATT_WriteToLeaderboards:
				{
#if LEADERBOARD_PLATFORM
					CPersistantStats* pPersistantStats = CPersistantStats::GetInstance();
					if ((m_writeLeaderboardData.size() > 0) && (pPersistantStats->IsWritingLeaderboardData() == false))
					{
						pPersistantStats->WriteToDedicatedLeaderBoards(taskID, error);
						if(taskID != CryLobbyInvalidTaskID)
						{
							taskStarted = true;
						}
					}
#endif
					break;
				}

			default:
				{
					CRY_ASSERT_MESSAGE(0, string().Format("[GameLobby] unknown online task type %d", m_onlineAttributeTasks[0].m_type));
					break;
				}
		}

		if(error == eCLE_Success)
		{
			if(taskStarted)
			{
				CryLog("[GameLobby] online attribute task %d started with error %d", pTask->m_type, error);	

				pTask->m_status = eOATS_InProgress;
				pTask->m_task = taskID;
			}
			else
			{
				// it is possible for these two tasks to 'fail' due to there being no users to write
				// data for, this attempts to cope with that here
				if(pTask->m_type != eOATT_WriteToLeaderboards)
				{
					CryLog("Invalid task %d has succeeded but not marked its task as started", pTask->m_type);
				}
			}
		}
		else if(error != eCLE_TooManyTasks)	// retry if too many, otherwise fail and remove
		{
			CryLog("[GameLobby] could not start online attribute task %d", pTask->m_type);

			m_onlineAttributeTasks.removeAt(0);
		}
	}
#endif
}

SOnlineAttributeTask* CGameLobby::FindOnlineAttributeTask(CryLobbyTaskID taskID)
{
#if INCLUDE_DEDICATED_LEADERBOARDS
	if(!AllowOnlineAttributeTasks())
	{
		return NULL;
	}

	SOnlineAttributeTask *pTask = NULL;
	int numTasks = m_onlineAttributeTasks.size();
	
	for(int i=0; i < numTasks; i++)
	{
		if(m_onlineAttributeTasks[i].m_task == taskID)
		{
			pTask = &m_onlineAttributeTasks[i];
			break;
		}
	}

	return pTask;
#else
	return NULL;
#endif
}

void CGameLobby::RemoveOnlineAttributeTask(CryUserID userID, bool allowStartedTaskRemoval)
{
#if INCLUDE_DEDICATED_LEADERBOARDS
	if(!AllowOnlineAttributeTasks())
	{
		return;
	}

	int numTasks = m_onlineAttributeTasks.size();
	int index = -1;
	
	for(int i=0; i < numTasks; i++)
	{
		if(m_onlineAttributeTasks[i].m_user == userID && (m_onlineAttributeTasks[i].m_status != eOATS_InProgress || allowStartedTaskRemoval))
		{
			index = i;
			break;
		}
	}

	if(index >= 0)
	{
		m_onlineAttributeTasks.removeAt(index);
	}
#endif
}

void CGameLobby::RemoveOnlineAttributeTask(CryLobbyTaskID taskID, bool allowStartedTaskRemoval)
{
#if INCLUDE_DEDICATED_LEADERBOARDS
	if(!AllowOnlineAttributeTasks())
	{
		return;
	}

	int numTasks = m_onlineAttributeTasks.size();
	int index = -1;
	
	for(int i=0; i < numTasks; i++)
	{
		if(m_onlineAttributeTasks[i].m_task == taskID && (m_onlineAttributeTasks[i].m_status != eOATS_InProgress || allowStartedTaskRemoval))
		{
			index = i;
			break;
		}
	}

	if(index >= 0)
	{
		m_onlineAttributeTasks.removeAt(index);
	}
#endif
}

bool CGameLobby::IsOnlineAttributeTaskInProgress()
{
#if INCLUDE_DEDICATED_LEADERBOARDS
	if(!AllowOnlineAttributeTasks())
	{
		return false;
	}

	int numTasks = m_onlineAttributeTasks.size();
	
	for(int i=0; i < numTasks; i++)
	{
		if(m_onlineAttributeTasks[i].m_status == eOATS_InProgress)
		{
			return true;
		}
	}
#endif

	return false;
}

bool CGameLobby::OnlineAttributeTaskFinished(CryLobbyTaskID taskID, ECryLobbyError error)
{
#if INCLUDE_DEDICATED_LEADERBOARDS
	SOnlineAttributeTask *pTask = FindOnlineAttributeTask(taskID);
	bool result = false;
	CRY_ASSERT(pTask);

	CryLog("[GameLobby] OnlineAttributeTask finished taskID %d error %d pTask %p", pTask ? pTask->m_task : -1, error, pTask);

	if(pTask)
	{
		if(error == eCLE_TimeOut)
		{
			CryLog("[GameLobby] Restarting online attribute task due to time out");
			pTask->m_status = eOATS_NotStarted;
		}
		else 
		{
			RemoveOnlineAttributeTask(taskID, true);
			result = true;
		}
	}

	return result;
#else
	return true;
#endif
}

//-------------------------------------------------------------------------
uint16 CGameLobby::GetSkillRanking( int channelId )
{
	uint16 skillRanking = 1500;		// Default

	if (gEnv->pNetwork && gEnv->pNetwork->GetLobby())
	{
		if (ICryMatchMaking* pMatchMaking = gEnv->pNetwork->GetLobby()->GetMatchMaking())
		{
			const CrySessionHandle sessionHandle = m_currentSession;
			const SCryMatchMakingConnectionUID conId = pMatchMaking->GetConnectionUIDFromGameSessionHandleAndChannelID(sessionHandle, channelId);

			int playerIndex = m_nameList.Find(conId);
			if (playerIndex != SSessionNames::k_unableToFind)
			{
				SSessionNames::SSessionName &player = m_nameList.m_sessionNames[playerIndex];
				skillRanking = player.GetSkillRank();
			}
		}
	}

	return skillRanking;
}

//-------------------------------------------------------------------------
CGameLobby::EActiveStatus CGameLobby::GetActiveStatus(const ELobbyState currentState) const
{
	EActiveStatus activeStatus = eAS_Lobby;
	if (currentState == eLS_Game)
	{
		activeStatus = eAS_StartingGame;
#ifdef GAME_IS_CRYSIS2
		CGameRules *pGameRules = g_pGame->GetGameRules();
		if (pGameRules)
		{
			IGameRulesStateModule *pStateModule = pGameRules->GetStateModule();
			if (pStateModule != NULL && (pStateModule->GetGameState() == IGameRulesStateModule::EGRS_InGame))
			{
				activeStatus = eAS_Game;
			}
			if (pGameRules->IsTimeLimited())
			{
				const float remainingGameTime = pGameRules->GetRemainingGameTime();
				if (remainingGameTime < gl_timeTillEndOfGameForNoMatchMaking)
				{
					activeStatus = eAS_EndGame;
				}
			}
		}
#endif
	}
	else if (m_startTimerCountdown)
	{
		if (m_startTimer < gl_timeBeforeStartOfGameForNoMatchMaking)
		{
			activeStatus = eAS_StartingGame;
		}
	}

	return activeStatus;
}

//-------------------------------------------------------------------------
int32 CGameLobby::CalculateAverageSkill()
{
	// Note: This won't be an exact average since we're returning an int, however the value
	// is going to be used as an int in the session details and we're not too worried about
	// being absolutely correct on it (it's used as an approximate measure for matchmaking)

	int32 totalSkill = 0;

	const unsigned int numPlayers = m_nameList.Size();
	if (numPlayers > 1)
	{
		for (unsigned int i = 0; i < numPlayers; ++ i)
		{
			uint16 skillRank = m_nameList.m_sessionNames[i].GetSkillRank();
			totalSkill += (int32) skillRank;
		}
		totalSkill /= numPlayers;
	}
	else
	{
#ifdef GAME_IS_CRYSIS2
		totalSkill = CPlayerProgression::GetInstance()->GetData(EPP_SkillRank);
#endif
	}

	return totalSkill;
}

//-------------------------------------------------------------------------
void CGameLobby::CheckForSkillChange()
{
	if (m_server && (m_state != eLS_Leaving))
	{
		uint32 newAverage = CalculateAverageSkill();
		if (newAverage != m_lastUpdatedAverageSkill)
		{
			m_timeTillUpdateSession = gl_skillChangeUpdateDelayTime;
		}
	}
}

//-------------------------------------------------------------------------
void CGameLobby::GameOver()
{
#if INCLUDE_DEDICATED_LEADERBOARDS
	CryLog("[GameLobby] GameOver isServer %d isDedicated %d", gEnv->bServer, gEnv->IsDedicated());
	
	if(gEnv->bServer)
	{
		if(gEnv->IsDedicated())
		{
			SSessionNames *pSessionNames = &m_nameList;
			uint32 count = pSessionNames->Size();

			for(uint32 i = 0; i < count; ++i)
			{
				SSessionNames::SSessionName *pSessionName = &pSessionNames->m_sessionNames[i];
				if(pSessionName->m_onlineStatus == eOAS_Initialised)
				{
					CryLog("  waiting for stats from user %s", pSessionName->m_name);

					pSessionName->m_onlineStatus = eOAS_WaitingForUpdate; 
					pSessionName->m_recvOnlineDataCount = 0;	// waiting for new user data to appear
				}
			}
		}
	}
	else
	{
		// can't stop the client sending, but server will ignore
		if(!IsQuitting())
		{
			CryLog("  sending end of game user data");
			SendPacket(eGUPD_EndOfGameUserStats, CryMatchMakingInvalidConnectionUID);
		}
		else
		{
			CryLog("  not sending end of game user data");
		}
	}
#endif
}

//-------------------------------------------------------------------------
void CGameLobby::SetSessionNamesWithStatusTo(EOnlineAttributeStatus withStatus, EOnlineAttributeStatus toStatus)
{
#if INCLUDE_DEDICATED_LEADERBOARDS
	SSessionNames *pSessionNames = &m_nameList;
	uint32 count = pSessionNames->Size();

	for(uint32 i = 0; i < count; ++i)
	{
		SSessionNames::SSessionName *pSessionName = &pSessionNames->m_sessionNames[i];
		if(pSessionName->m_onlineStatus == withStatus)
		{
			pSessionName->m_onlineStatus = toStatus; // back to default
		}
	}
#endif
}

//-------------------------------------------------------------------------
bool CGameLobby::AnySessionNamesWithStatus(EOnlineAttributeStatus status)
{
#if INCLUDE_DEDICATED_LEADERBOARDS
	bool hasStatus = false;

	SSessionNames *pSessionNames = &m_nameList;
	uint32 count = pSessionNames->Size();

	for(uint32 i = 0; i < count; ++i)
	{
		if(pSessionNames->m_sessionNames[i].m_onlineStatus == status)
		{
			hasStatus = true;
			break;
		}
	}

	return hasStatus;
#else
	return false;
#endif
}

//-------------------------------------------------------------------------
bool CGameLobby::IsUsingDedicatedLeaderboards()
{
	ICryStats *pStats = gEnv->pNetwork->GetLobby()->GetStats();
	return (pStats != NULL) && (pStats->GetLeaderboardType() == eCLLT_Dedicated);
}

//-------------------------------------------------------------------------
int32 CGameLobby::GetCurrentLanguageId()
{
	int32 languageId = 0;
	ILocalizationManager* pLocalizationManager = gEnv->pSystem->GetLocalizationManager();
	if (pLocalizationManager)
	{
		CryHashStringId hash(	pLocalizationManager->GetLanguage() );
		languageId = (int32) hash.id;
	}
	return languageId;
}

//-------------------------------------------------------------------------
void CGameLobby::CreateSessionFromSettings(const char *pGameRules, const char *pLevelName)
{
#ifdef GAME_IS_CRYSIS2
	CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
	if (pPlaylistManager)
	{
		int defaultPlaylistId = pPlaylistManager->GetDefaultVariant();
		pPlaylistManager->ChooseVariant(defaultPlaylistId);
	}
#endif

	SetMatchmakingGame(false);
	m_currentGameRules = pGameRules;
	m_currentLevelName = pLevelName;

	if (m_state != eLS_None)
	{
		LeaveSession(true);
	}

	m_taskQueue.AddTask(CLobbyTaskQueue::eST_Create, false);

	// Show create session dialog whether we can start the create now or not
#ifdef USE_C2_FRONTEND
	CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
	CMPMenuHub *mpMenuHub = menu ? menu->GetMPMenu() : NULL;
	if (mpMenuHub)
	{
		mpMenuHub->ShowLoadingDialog("CreateSession");
	}
#endif //#ifdef USE_C2_FRONTEND
}

//-------------------------------------------------------------------------
void CGameLobby::UpdateRulesAndMapFromVoting()
{
	if (m_leftWinsVotes)
	{
		UpdateRulesAndLevel(m_leftVoteChoice.m_gameRules.c_str(), m_leftVoteChoice.m_levelName.c_str());
	}
	else
	{
		UpdateRulesAndLevel(m_rightVoteChoice.m_gameRules.c_str(), m_rightVoteChoice.m_levelName.c_str());
	}
}

//-------------------------------------------------------------------------
void CGameLobby::UpdateRulesAndLevel(const char *pGameRules, const char *pLevelName)
{
	m_currentGameRules = pGameRules;
	m_currentLevelName = pLevelName;
}

//-------------------------------------------------------------------------
void CGameLobby::UpdateVoteChoices()
{
#ifdef GAME_IS_CRYSIS2
	if (ILevelRotation* pLevelRotation=g_pGame->GetPlaylistManager()->GetLevelRotation())
	{
		const int curNext = pLevelRotation->GetNext();

		m_leftVoteChoice.m_levelName = pLevelRotation->GetNextLevel();
		m_leftVoteChoice.m_gameRules = pLevelRotation->GetNextGameRules();

		if (!pLevelRotation->Advance())
		{
			pLevelRotation->First();
		}

		m_rightVoteChoice.m_levelName = pLevelRotation->GetNextLevel();
		m_rightVoteChoice.m_gameRules = pLevelRotation->GetNextGameRules();

		while (pLevelRotation->GetNext() != curNext)
		{
			if (!pLevelRotation->Advance())
			{
				pLevelRotation->First();
			}
		}
	}
#endif
}

//-------------------------------------------------------------------------
void CGameLobby::MoveUsers(CGameLobby *pFromLobby)
{
	CryLog("[GameLobby] MoveUsers pFromLobby %p pToLobby %p", pFromLobby, this);
	CRY_ASSERT_MESSAGE(pFromLobby->IsServer(), "Only the server should be moving users from one lobby to another");
	CRY_ASSERT_MESSAGE(pFromLobby != this, "Lobby we are trying to move users into is the one we are already in");
	CRY_ASSERT_MESSAGE(m_gameLobbyMgr->IsPrimarySession(pFromLobby), "Trying to move users but we're not the primary session");

	const SSessionNames &fromSession = pFromLobby->GetSessionNames();
	SSessionNames *toSession = &m_nameList;
	unsigned int userCount = fromSession.Size();

	// temporarily copy my details as well for the time being
	for(unsigned int i = 1; i < userCount; i++)
	{
		const SSessionNames::SSessionName *pSessionName = &fromSession.m_sessionNames[i];
		if (pSessionName->m_name[0] != 0)
		{
			if (toSession->FindByUserId(pSessionName->m_userId) == SSessionNames::k_unableToFind)
			{
				toSession->Insert(pSessionName->m_userId, CryMatchMakingInvalidConnectionUID, pSessionName->m_name, pSessionName->m_userData, pSessionName->m_isDedicated); // we pass invalid connection id so as not to confuse the new lobby
			}
		}

		CryLog("[GameLobby] Moving user %s", pSessionName->m_name);
	}

	CRY_ASSERT_MESSAGE(toSession->Size() <= MAX_PLAYER_LIMIT, string().Format("Too many players added to session names. Count %d Max %d", toSession->Size(), MAX_PLAYER_LIMIT).c_str());
}

//-------------------------------------------------------------------------
void CGameLobby::OnStartPlaylistCommandIssued()
{
	CryLog("CGameLobby::OnStartPlaylistCommandIssued()");
	SetMatchmakingGame(true);
	
#ifdef GAME_IS_CRYSIS2
	if (m_state != eLS_None)
	{
		if (m_server)
		{
			CryLog("  currently server, switching playlist");

			if (g_pGame->GetIGameFramework()->StartedGameContext())
			{
				// Already in a started session, need to end it so we can change playlist
				gEnv->pConsole->ExecuteString("unload", false, false);
				SetState(eLS_EndSession);
			}

			uint32 playListSeed = (uint32)(gEnv->pTimer->GetFrameStartTime(ITimer::ETIMER_UI).GetValue());
			m_playListSeed = playListSeed;
			CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
			ILevelRotation*  pLevelRotation = pPlaylistManager ? pPlaylistManager->GetLevelRotation() : NULL;
			if (pLevelRotation != NULL && pLevelRotation->IsRandom())
			{
				pLevelRotation->Shuffle(playListSeed);
			}

			if (m_votingEnabled)
			{
				SvResetVotingForNextElection();
			}
			SendPacket(eGUPD_SyncPlaylistContents);
			SendPacket(eGUPD_SyncPlaylistRotation);

			if (pLevelRotation)
			{
				const int  len = pLevelRotation->GetLength();
				if (len > 0)
				{
					m_currentLevelName = pLevelRotation->GetNextLevel();

					// game modes can have aliases, so we get the correct name here
					IGameRulesSystem *pGameRulesSystem = g_pGame->GetIGameFramework()->GetIGameRulesSystem();
					const char *pGameRulesName = pGameRulesSystem->GetGameRulesName(pLevelRotation->GetNextGameRules());

					if (stricmp(m_currentGameRules.c_str(), pGameRulesName))
					{
						GameRulesChanged(pGameRulesName);
					}

					pPlaylistManager->SetModeOptions();
				}
			}

			m_taskQueue.AddTask(CLobbyTaskQueue::eST_Update, true);
		}
		else
		{
			CryLog("  not server, leaving and starting a new game");
			StartFindGame();
		}
	}
	else
	{
		CryLog("  not in a game currently, starting FindGame");
		StartFindGame();
	}
#endif
}

//-------------------------------------------------------------------------
eHostMigrationState CGameLobby::GetMatchMakingHostMigrationState()
{
	eHostMigrationState hostMigrationState = eHMS_Unknown;

	if (m_currentSession != CrySessionInvalidHandle)
	{
		ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
		if (pLobby)
		{
			ICryMatchMaking *pMatchmaking = pLobby->GetMatchMaking();
			if (pMatchmaking)
			{
				hostMigrationState = pMatchmaking->GetSessionHostMigrationState(m_currentSession);
			}
		}
	}

	return hostMigrationState;
}

//-------------------------------------------------------------------------
void CGameLobby::TerminateHostMigration()
{
	CryLog("CGameLobby::TerminateHostMigration()");

	if (m_currentSession != CrySessionInvalidHandle)
	{
		ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
		if (pLobby)
		{
			ICryMatchMaking *pMatchmaking = pLobby->GetMatchMaking();
			if (pMatchmaking)
			{
				pMatchmaking->TerminateHostMigration(m_currentSession);
			}
		}
	}
}

const char* CGameLobby::GetSessionName()
{
 	return m_currentSessionName.c_str();
}

SDetailedServerInfo* CGameLobby::GetDetailedServerInfo()
{

	return &m_detailedServerInfo; 
}

void CGameLobby::CancelDetailedServerInfoRequest()
{
	CryLog("CancelDetailedServerInfoRequest");

	if(m_taskQueue.HasTaskInProgress() && m_taskQueue.GetCurrentTask() == CLobbyTaskQueue::eST_SessionRequestDetailedInfo)
	{
		CryLog("invalidating in progress detailed session info request");
		m_detailedServerInfo.m_taskID = CryLobbyInvalidTaskID;
	}

	// cancel any versions of this task we have in the queue
	m_taskQueue.CancelTask(CLobbyTaskQueue::eST_SessionRequestDetailedInfo);
}

bool CGameLobby::AllowCustomiseEquipment()
{
	bool allow = true;
#ifdef USE_C2_FRONTEND
	CFlashFrontEnd *pFFE = g_pGame->GetFlashMenu();
	EFlashFrontEndScreens screen = pFFE ? pFFE->GetCurrentMenuScreen() : eFFES_unknown;
	
	EActiveStatus activeStatus = GetActiveStatus(m_state);

	if(screen == eFFES_game_lobby)
	{
		CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
		const SPlaylist* myPlaylist = pPlaylistManager ? pPlaylistManager->GetCurrentPlaylist() : NULL;

		if(myPlaylist)
		{
			// assault playlist
			if(!strcmp(myPlaylist->rotExtInfo.uniqueName, "ASSAULT"))
			{
				allow = false;
			}
		}

		if(allow)
		{
			if (m_votingEnabled)
			{
				// this, as far as I'm aware, is really only a problem on pc at the moment where people could setup
				// their own custom playlists, which might include assault and another gamemode, in that instance
				// we'd like to keep the  customise equipment option available if unlocked
				if(!strcmp(m_leftVoteChoice.m_gameRules.c_str(), "Assault") && !strcmp(m_rightVoteChoice.m_gameRules.c_str(), "Assault"))
				{
					allow = false;
				}
			}
			else
			{
				const char *currentGameRules = GetCurrentGameModeName();
				if(!strcmp(currentGameRules, "Assault"))
				{
					allow = false;
				}
			}
		}
	}
#endif //#ifdef USE_C2_FRONTEND

	return allow;
}

void CGameLobby::RefreshCustomiseEquipment()
{
	CryLog("CGameLobby::RefreshCustomiseEquipment");

	// if it's not a matchmaking session, then we have all the details we need,
	// we need to refresh the screen so as to update the customise loadout button
	// as it may need disabling, matchmaing sessions will refresh the screen when they 
	// receive the playlist, not we also do not want to do this if we're not the game lobby screen
#ifdef USE_C2_FRONTEND
	CFlashFrontEnd *pFFE = g_pGame->GetFlashMenu();
	EFlashFrontEndScreens screen = pFFE ? pFFE->GetCurrentMenuScreen() : eFFES_unknown;

	if(screen == eFFES_game_lobby)
	{
		pFFE->RefreshCurrentScreen();
	}
#endif //#ifdef USE_C2_FRONTEND
}

//-------------------------------------------------------------------------
void CGameLobby::OnVariantChanged()
{
	if (m_server && (m_currentSession != CrySessionInvalidHandle))
	{
		m_taskQueue.AddTask(CLobbyTaskQueue::eST_Update, true);
	}
}

//-------------------------------------------------------------------------
void CGameLobby::UpdatePreviousGameScores()
{
	m_previousGameScores.clear();
#ifdef GAME_IS_CRYSIS2
	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules)
	{
		IGameRulesPlayerStatsModule *pPlayerStatsModule = pGameRules->GetPlayerStatsModule();
		if (pPlayerStatsModule)
		{
			const float fGameStartTime = pGameRules->GetGameStartTime();
			const float fGameLength = (pGameRules->GetCurrentGameTime() * 1000.f);		// Spawn time is in milliseconds, length is in seconds
			const int numStats = pPlayerStatsModule->GetNumPlayerStats();
			for (int i=0; i<numStats; i++)
			{
				const SGameRulesPlayerStat* pPlayerStats = pPlayerStatsModule->GetNthPlayerStats(i);
				float fFracTimeInGame = 0.f;
				CPlayer *pPlayer = static_cast<CPlayer *>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pPlayerStats->playerId));
				if (pPlayer)
				{
					const float fTimeSpawned = pPlayer->GetTimeFirstSpawned();
					const float fTimeInGame = (fGameLength + fGameStartTime) - fTimeSpawned;
					if (fGameLength > 0.f)
					{
						fFracTimeInGame = (fTimeInGame / fGameLength);
						fFracTimeInGame = CLAMP(fFracTimeInGame, 0.f, 1.f);
					}

					SPlayerScores playerScores;
					playerScores.m_playerId = GetConnectionUIDFromChannelID(pPlayer->GetChannelId());
					playerScores.m_score = pPlayerStats->points;
					playerScores.m_fracTimeInGame = fFracTimeInGame;
					m_previousGameScores.push_back(playerScores);
				}
			}
			m_needsTeamBalancing = true;
		}
	}
#endif
}

//-------------------------------------------------------------------------
void CGameLobby::RequestLeaveFromMenu()
{
	SetState(eLS_Leaving);

	m_taskQueue.AddTask(CLobbyTaskQueue::eST_Unload, false);

	m_isMidGameLeaving = true;

	CSquadManager *pSquadManager = g_pGame->GetSquadManager();
	if (pSquadManager)
	{
		pSquadManager->LeftGameSessionInProgress();
	}
}

//-------------------------------------------------------------------------
void CGameLobby::DoUnload()
{
	CRY_ASSERT(m_taskQueue.GetCurrentTask() == CLobbyTaskQueue::eST_Unload);

	SetState(eLS_None);

	IGameFramework *pFramework = g_pGame->GetIGameFramework();
	if (pFramework->StartedGameContext() || pFramework->StartingGameContext())
	{
		gEnv->pGame->GetIGameFramework()->ExecuteCommandNextFrame("disconnect");
	}

#ifdef USE_C2_FRONTEND
	TFlashFrontEndPtr pFlashFrontEnd = g_pGame->GetFlashFrontEnd();
	if (pFlashFrontEnd)
	{
		CMPMenuHub *pMPMenuHub = pFlashFrontEnd->GetMPMenu();
		if (pMPMenuHub)
		{
			CUIScoreboard *pScoreboard = pMPMenuHub->GetScoreboardUI();
			if (pScoreboard)
			{
				pScoreboard->ClearLastMatchResults();
			}
		}
	}
#endif //#ifdef USE_C2_FRONTEND

	m_isMidGameLeaving = false;
}

//-------------------------------------------------------------------------
void CGameLobby::DoBetweenRoundsUnload()
{
	CryLog("CGameLobby::DoBetweenRoundsUnload()");
	gEnv->pConsole->ExecuteString("unload", false, true);
#ifdef USE_C2_FRONTEND
	CFlashFrontEnd *pFlashFrontEnd = g_pGame->GetFlashMenu();
	if (pFlashFrontEnd)
	{
		pFlashFrontEnd->OnAboutToUnloadLevel();
	}
#endif //#ifdef USE_C2_FRONTEND

#ifdef GAME_IS_CRYSIS2
	CRecordingSystem *pRecordingSystem = g_pGame->GetRecordingSystem();
	if (pRecordingSystem)
	{
		pRecordingSystem->StopHighlightReel();
	}
#endif
}

//-------------------------------------------------------------------------
// Be friendly with uber files
#undef GAME_LOBBY_DO_ENSURE_BEST_HOST
#undef GAME_LOBBY_DO_LOBBY_MERGING
#undef GAME_LOBBY_IGNORE_BAD_SERVERS_LIST
