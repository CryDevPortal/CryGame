#ifndef ___GAME_LOBBY_H___
#define ___GAME_LOBBY_H___

#include "INetwork.h"
#include "IGameFramework.h"
#include "ICryLobby.h"
#include "ICryMatchMaking.h"
#include "GameLobbyData.h"
#include "GameUserPackets.h"
#include "AutoLockData.h"
#include "SessionNames.h"
#include <CryFixedArray.h>
//#include "Dogtag.h"
//#include "FrontEnd/MenuData.h"
#include "Network/LobbyTaskQueue.h"
//#include "Audio/AudioSignalPlayer.h"

struct SGameStartParams;
struct IFlashPlayer;
class CGameLobbyManager;
class CCryLobbyPacket;

//12 players - minus yourself
#define MAX_RESERVATIONS		(MAX_PLAYER_LIMIT - 1)

#define MAX_CHATMESSAGE_LENGTH 128
#define NUM_CHATMESSAGES_STORED 32
#define CHAT_MESSAGE_POSTFIX "> "  // this gets used for the lobby chat and the ingame chat

#define DETAILED_SESSION_INFO_MOTD_SIZE (256)		// allow 256 bytes for detailed session info Message Of The Day
#define DETAILED_SESSION_INFO_URL_SIZE	(256)		// allow 256 bytes for detailed session info data URL
#define DETAILED_SESSION_MAX_PLAYERS		(MAX_PLAYER_LIMIT) // number of players
#define DETAILED_SESSION_MAX_CUSTOMS		(32)		// technically, should match however many items are in the CPlayerlistManager::m_custom vector, but I'm going to force it due to packet size limitations.


#if INCLUDE_DEDICATED_LEADERBOARDS
	// at the moment, this envisioned to be 1 read or write per player
	// + 4 server tasks at most
	#define MAX_ONLINE_ATTRIBUTE_TASKS (MAX_PLAYER_LIMIT + 4)
#endif

#if (!defined(XENON) && !defined(PS3))
	#define ENABLE_CHAT_MESSAGES 1
#else
	#define ENABLE_CHAT_MESSAGES 0
#endif

#define MAX_USER_DATAS 32

#if !defined(_RELEASE)
#define ENSURE_ON_MAIN_THREAD \
	if (CGameLobby::s_mainThreadHandle != CryGetCurrentThreadId()) \
{ \
	CryLogAlways("*** FIX ME - NOT ON MAIN THREAD ***"); \
	/* *((char*)NULL) = 0; */ \
}
#else
#define ENSURE_ON_MAIN_THREAD
#endif

#if ! defined (DEDICATED_SERVER)
#ifdef GAME_IS_CRYSIS2
#define TRACK_MATCHMAKING
#endif
#endif

//typedef void (*GameLobbyJoinCallback)(ECryLobbyError error, void* arg);

struct SLobbyGameStartParams
{
	unsigned flags;
	uint16 port;
	int maxPlayers;

	CryFixedStringT<32> hostname;
	CryFixedStringT<32> connectionString;
	CryFixedStringT<64> levelName;
	CryFixedStringT<32> gameModeName;
	CryFixedStringT<32> demoRecorderFilename;
	CryFixedStringT<32> demoPlaybackFilename;

	SLobbyGameStartParams()
	{
		flags = 0;
		port = 0;
		maxPlayers = MAX_PLAYER_LIMIT;
	}
};

enum EOnlineAttributeTaskType
{
	eOATT_Invalid = 0,
	eOATT_ReadUserData,
	eOATT_WriteUserData,
	eOATT_WriteToLeaderboards,
};

enum EOnlineAttributeTaskStatus
{
	eOATS_NotStarted = 0,
	eOATS_InProgress,
	eOATS_Success,
	eOATS_Failed,
};

enum EDetailedSessionInfoResponseFlags
{
	eDSIRF_Basic = 0,
	eDSIRF_IncludePlayers = BIT(0),
	eDSIRF_IncludeCustomFields = BIT(1),

	eDSIRF_All = eDSIRF_Basic | eDSIRF_IncludePlayers | eDSIRF_IncludeCustomFields,
};

struct SOnlineAttributeTask
{
	CryUserID	m_user;
	CryLobbyTaskID m_task;
	EOnlineAttributeTaskStatus m_status;
	EOnlineAttributeTaskType m_type;

	SOnlineAttributeTask()
	{
		m_user = CryUserInvalidID;
		m_task = CryLobbyInvalidTaskID;
		m_status = eOATS_NotStarted;
		m_type = eOATT_Invalid;
	}

	SOnlineAttributeTask(CryUserID userID, CryLobbyTaskID taskID, EOnlineAttributeTaskType taskType, EOnlineAttributeTaskStatus taskStatus)
	{
		m_user = userID;
		m_task = taskID;
		m_type = taskType;
		m_status = taskStatus;
	}
};

#if INCLUDE_DEDICATED_LEADERBOARDS
struct SWriteUserData
{
	CryUserID						m_userID;
	SCryLobbyUserData		m_data[MAX_ONLINE_STATS_SIZE];
	int									m_numData;
	bool								m_writing;
	bool								m_requeue;

	SWriteUserData()
	{
		m_userID = CryUserInvalidID;
		memset(m_data, 0, MAX_ONLINE_STATS_SIZE);
		m_numData = 0;
		m_writing = false;
		m_requeue = false;
	}

	SWriteUserData(CryUserID userID, SCryLobbyUserData *pData, int numData)
	{
		m_userID = userID;
		SetData(pData, numData);
		m_writing = false;
		m_requeue = false;
	}

	void SetData(SCryLobbyUserData *pData, int numData)
	{
		m_numData = min(MAX_ONLINE_STATS_SIZE, numData);
		memcpy(m_data, pData, sizeof(SCryLobbyUserData) * m_numData);
	}
};

struct SWriteLeaderboardData
{
	CryUserID					m_userID;
	SCryLobbyUserData	m_data[MAX_ONLINE_STATS_SIZE];
	int								m_numData;
	bool							m_dirty;

	SWriteLeaderboardData()
		: m_userID(CryUserInvalidID), m_numData(0), m_dirty(false)
	{
	}

	SWriteLeaderboardData(CryUserID userID, SCryLobbyUserData *pData, int numData)
		: m_userID(userID), m_numData(min(MAX_ONLINE_STATS_SIZE, numData)), m_dirty(true)
	{
		memcpy(m_data, pData, sizeof(SCryLobbyUserData) * m_numData);
	}

	void Replace(SCryLobbyUserData* pData, int numData)
	{
		m_numData = min(MAX_ONLINE_STATS_SIZE, numData);
		memcpy(m_data, pData, sizeof(SCryLobbyUserData) * m_numData);
		m_dirty = true;
	}
};
#endif

struct SDetailedServerInfo
{
	//-- Request
	CrySessionID												m_sessionId;
	CryLobbyTaskID											m_taskID;
	EDetailedSessionInfoResponseFlags		m_flags;

	//-- Response
	char																m_motd[DETAILED_SESSION_INFO_MOTD_SIZE];
	char																m_url[DETAILED_SESSION_INFO_URL_SIZE];
	char																m_names[MAX_PLAYER_LIMIT][CRYLOBBY_USER_NAME_LENGTH];
	uint16															m_customs[DETAILED_SESSION_MAX_CUSTOMS];
	uint16															m_namesCount;
};

struct SPlayerScores
{
	SCryMatchMakingConnectionUID				m_playerId;
	int																	m_score;
	float																m_fracTimeInGame;
};


enum ELobbyState
{
	eLS_None,
	eLS_Initializing,	//join game for create game callbacks
	eLS_FindGame,
	eLS_JoinSession,
	eLS_Lobby,
	eLS_PreGame,
	eLS_PostGame,
	eLS_Game,
	eLS_EndSession,
	eLS_GameEnded, //server only
	eLS_Leaving,
};


enum ELobbyVOIPState
{
	eLVS_off = 1,			// No voice available
	eLVS_on,					// Have voice and is not muted
	eLVS_muted,				// Has been muted - manually or by filter
	eLVS_mutedWrongTeam,	// Has been muted because they're on the other team
	eLVS_speaking,		// Voice is not muted and is speaking
};

enum ELobbyAutomaticVOIPType		/* Type of automatic voice muting */
{
	eLAVT_start = 0,
	eLAVT_off = 0,					// No automatic muting
	eLAVT_allButParty,			// Mute all but your squad
	eLAVT_all,							// Mute all
	eLAVT_end,
};

enum ELobbyVoteStatus
{
	eLVS_notVoted						= -1,
	eLVS_awaitingCandidates	=  0,
	eLVS_votedLeft					=  1,
	eLVS_votedRight					=  2,
};

enum ELobbyNetworkedVoteStatus
{
	eLNVS_NotVoted,
	eLNVS_VotedLeft,
	eLNVS_VotedRight,
};

enum EReservationResult
{
	eRR_Fail = 0,
	eRR_Success,
	eRR_NoneNeeded,
};

enum ELobbyEntryType
{
	eLET_Lobby = 0,
	eLET_Squad,
	eLET_Matchmaking,
};

class CGameLobby : public IHostMigrationEventListener,
                   public IGameWarningsListener
{
public:
	const static int SESSION_NAME_LENGTH = 32;

	enum EActiveStatus
	{
		eAS_Lobby,
		eAS_Game,
		eAS_EndGame,
		eAS_StartingGame,
	};

protected:

	struct SVotingChoiceInfo
	{
		void Reset()
		{
			m_levelName = "";
			m_gameRules = "";
			m_numVotes = 0;
		}

		CryFixedStringT<64> m_levelName;
		CryFixedStringT<32> m_gameRules;
		uint8 m_numVotes;
	};

	SCryLobbyUserData m_userData[eLDI_Num];
	SCrySessionData m_sessionData;

	CryLobbyTaskID m_currentTaskId;

	ELobbyState m_state;
	ELobbyState m_requestedState;

	CrySessionHandle m_currentSession;

	CrySessionID m_pendingConnectSessionId;
	CryFixedStringT<SESSION_NAME_LENGTH> m_pendingConnectSessionName;
	SCryMatchMakingConnectionUID m_pendingReservationId;

	CryFixedStringT<SESSION_NAME_LENGTH> m_currentSessionName;

	SDetailedServerInfo m_detailedServerInfo;

	SSessionNames m_nameList;

	uint32	m_sessionFavouriteKeyId;	// Session's associated user account as an id
	int m_endGameResponses;

	uint32 m_playListSeed;

	float m_leaveGameTimeout;

	SVotingChoiceInfo m_leftVoteChoice;
	SVotingChoiceInfo m_rightVoteChoice;

	uint8 m_highestLeadingVotesSoFar;
	
	bool m_leftHadHighestLeadingVotes;
	bool m_votingEnabled;
	bool m_votingClosed;
	bool m_leftWinsVotes;
	bool m_server;
	bool m_connectedToDedicatedServer;
	bool m_sessionUserDataDirty;
	bool m_needsTeamBalancing;
	bool m_squadDirty;
	bool m_isLeaving;
	bool m_stateHasChanged;

public:
	
#if INCLUDE_DEDICATED_LEADERBOARDS
	typedef CryFixedArray<SWriteLeaderboardData, (MAX_PLAYER_LIMIT * 4)> TWriteLeaderboardData;
#endif

	CGameLobby(CGameLobbyManager* pMgr);
	virtual ~CGameLobby();
	void Update( float dt );

	bool CheckDLCRequirements();

	void CancelSessionInit();
	bool IsCreatingOrJoiningSession();

	bool JoinServer( CrySessionID sessionId, const char *sessionName, const SCryMatchMakingConnectionUID &reservationId, bool bRetryIfPassworded );
	void CreateSessionFromSettings( const char *pGameRules, const char *pLevelName );

	bool MergeToServer( CrySessionID sessionId );
	void FindGameCreateGame();

	void OnNewGameStartParams( const SGameStartParams* pGameStartParams );
	bool ShouldCallMapCommand( const char * pLevelName, const char *pGameRules );
	void OnMapCommandIssued();
	void OnStartPlaylistCommandIssued();
	
	inline ELobbyState GetState() { return m_state; }

	void SvFinishedGame(const float dt);

#if ENABLE_CHAT_MESSAGES
	static void CmdChatMessage(IConsoleCmdArgs* pCmdArgs);
	static void CmdChatMessageTeam(IConsoleCmdArgs* pCmdArgs);
	void SendChatMessage(bool team, const char* message);
#endif

	static void CmdStartGame(IConsoleCmdArgs* pCmdArgs);
	static void CmdSetMap(IConsoleCmdArgs* pCmdArgs);
	static void CmdSetGameRules(IConsoleCmdArgs* pCmdArgs);
	static void CmdVote(IConsoleCmdArgs* pCmdArgs);

	static const char* GetValidGameRules(const char* gameRules, bool returnBackup=false);
	static CryFixedStringT<32> GetValidMapForGameRules(const char* inLevel, const char* gameRules, bool returnBackup=false);

	static void SetLocalUserData(uint8 * localUserData);
	int GetNumberOfExpectedClients() const;

	void SetChoosingGamemode(const bool bChoosingGamemode) { m_bChoosingGamemode = bChoosingGamemode; }
	void ResetLevelOverride() { m_uiOverrideLevel.clear(); }

	// UI Related
#ifdef USE_C2_FRONTEND
	bool IsGameLobbyScreen(EFlashFrontEndScreens screen);
	const char* GetMapImageName(const char* levelFileName, CryFixedStringT<128>* pOutLevelImageName);

	void UpdateStatusMessage(IFlashPlayer *pFlashPlayer);

	void SendChatMessagesToFlash(IFlashPlayer *pFlashPlayer);
	void SendUserListToFlash(IFlashPlayer *pFlashPlayer);
	void SendSessionDetailsToFlash(IFlashPlayer *pFlashPlayer, const char* levelOverride=0);
	void UpdateMatchmakingDetails(IFlashPlayer *pFlashPlayer, const char* status);
	void UpdateVotingInfoFlashInfo();
	void UpdateVotingCandidatesFlashInfo();
	void ResetFlashInfos();
#endif //#ifdef USE_C2_FRONTEND

	bool CanShowGamercard(CryUserID userId);
	void ShowGamercardByUserId(CryUserID userId);
#ifdef GAME_IS_CRYSIS2
	void GetDogtagInfoByChannel(int channelId, CDogtag::STagInfo &dogtagInfo);
	void GetDogtagInfoBySessionName(SSessionNames::SSessionName *pSessionName, CDogtag::STagInfo &dogtagInfo);
#endif

	void GetPlayerNameFromChannelId(int channelId, CryFixedStringT<CRYLOBBY_USER_NAME_LENGTH> &name);
	void GetClanTagFromChannelId(int channelId, CryFixedStringT<CLAN_TAG_LENGTH> &name);
	void LocalUserDataUpdated();

	CryUserID GetLocalUserId();
	void GetLocalUserDisplayName(CryFixedStringT<DISPLAY_NAME_LENGTH> &displayName);
	void GetPlayerDisplayNameFromEntity(EntityId entityId, CryFixedStringT<DISPLAY_NAME_LENGTH> &displayName);
	void GetPlayerDisplayNameFromChannelId(int channelId, CryFixedStringT<DISPLAY_NAME_LENGTH> &displayName);

	ELobbyAutomaticVOIPType GetVOIPAutoMutingType() { return m_autoVOIPMutingType; }

	CryFixedStringT<CRYLOBBY_USER_GUID_STRING_LENGTH> GetGUIDFromActorID(EntityId actorId);
	CryUserID GetUserIDFromChannelID(int channelId);
	int GetTeamByChannelId(int channelId);
	SCryMatchMakingConnectionUID GetConnectionUIDFromChannelID(int channelId);

	inline void OnSquadChanged() { m_squadDirty = true; }

	void MutePlayerByChannelId(int channelId, bool mute, int reason);
	int GetPlayerMuteReason(CryUserID userId);
	ELobbyVOIPState GetVoiceState(int channelId);
	void CycleAutomaticMuting();
	void SetAutomaticMutingState(ELobbyAutomaticVOIPType newType);

	void SessionStartFailed(ECryLobbyError error);
	void SessionEndFailed(ECryLobbyError error);

	void FindGameMoveSession(CrySessionID sessionId);
	void FindGameCancelMove();

	void MakeReservations(SSessionNames* nameList, bool squadReservation);
	void SwitchToPrimaryLobby();

	void SetPrivateGame(bool enable);
	inline void SetPasswordedGame(bool enable) { m_passwordGame = enable; }
	void SetMatchmakingGame(bool bMatchmakingGame);

	inline bool IsServer() { return m_server; }
	inline bool IsPrivateGame() const { return m_privateGame; }
	inline bool IsPasswordedGame() const { return  m_passwordGame; }
	inline bool IsOnlineGame() { return (gEnv->pNetwork->GetLobby()->GetLobbyServiceType() == eCLS_Online); }
	inline bool IsMatchmakingGame() { return m_bMatchmakingSession; }
	CryUserID GetHostUserId();
	
	inline bool IsCurrentlyInSession() { return m_currentSession != CrySessionInvalidHandle; }

	inline const char* GetCurrentLevelName() { return m_currentLevelName.c_str(); }
	const char* GetCurrentGameModeName(const char* unknownStr="Unknown");

	const char* GetLoadingLevelName() { return m_loadingLevelName.c_str(); }
	const char* GetLoadingGameModeName() { return m_loadingGameRules.c_str(); }

	const SSessionNames &GetSessionNames() const 
	{ 
		return m_nameList; 
	}

#if INCLUDE_DEDICATED_LEADERBOARDS
	TWriteLeaderboardData &GetWriteLeaderboardData() { return m_writeLeaderboardData; }
	void WritePendingLeaderboardData(void);
#endif

	void LeaveAfterSquadMembers();

	void OnOptionsChanged();

	// IHostMigrationEventListener
	virtual bool OnInitiate(SHostMigrationInfo& hostMigrationInfo, uint32& state);
	virtual bool OnDisconnectClient(SHostMigrationInfo& hostMigrationInfo, uint32& state) { return true; }
	virtual bool OnDemoteToClient(SHostMigrationInfo& hostMigrationInfo, uint32& state);
	virtual bool OnPromoteToServer(SHostMigrationInfo& hostMigrationInfo, uint32& state);
	virtual bool OnReconnectClient(SHostMigrationInfo& hostMigrationInfo, uint32& state) { return true; }
	virtual bool OnFinalise(SHostMigrationInfo& hostMigrationInfo, uint32& state);
	virtual bool OnTerminate(SHostMigrationInfo& hostMigrationInfo, uint32& state);
	virtual bool OnReset(SHostMigrationInfo& hostMigrationInfo, uint32& state);
	// ~IHostMigrationEventListener

	// IGameWarningsListener
	virtual bool OnWarningReturn(THUDWarningId id, const char* returnValue);
	virtual void OnWarningRemoved(THUDWarningId id);
	// ~IGameWarningsListener

	static void SetLobbyService(ECryLobbyService lobbyService);
	static THUDWarningId ShowErrorDialog(const ECryLobbyError error, const char* pDialogName, const char* pDialogParam, IGameWarningsListener* pWarningsListener);
	static bool IsSignInError(ECryLobbyError error);

	void StartFindGame();
	void LeaveSession(bool clearPendingTasks);

	void OnHaveLocalPlayer();

	uint16 GetSkillRanking(int channelId);
	
	void DebugAdvancePlaylist();

	void GameOver();
	void SetSessionNamesWithStatusTo(EOnlineAttributeStatus withStatus, EOnlineAttributeStatus toStatus);
	bool AnySessionNamesWithStatus(EOnlineAttributeStatus status);
	void RemoveOnlineAttributeTask(CryLobbyTaskID taskID, bool allowStartedTaskRemoval = false);
	bool OnlineAttributeTaskFinished(CryLobbyTaskID taskID, ECryLobbyError error);
	void ClearWriteLeaderboardData();
	bool IsUsingDedicatedLeaderboards();
	void SetQuitting(bool yesNo) { m_bQuitting = yesNo; }
	bool OnLeaveOrQuit();

	void MoveUsers(CGameLobby *pFromLobby);

	eHostMigrationState GetMatchMakingHostMigrationState();
	void TerminateHostMigration();

	uint32 GetSessionFavouriteKeyId() const { return m_sessionFavouriteKeyId; }
	const char* GetSessionName();

	//-- Ask for extended server info
	void RequestDetailedServerInfo(CrySessionID sessionId, EDetailedSessionInfoResponseFlags flags);

	SDetailedServerInfo* GetDetailedServerInfo();
	void CancelDetailedServerInfoRequest();
	
	void UpdateDebugString();

	bool AllowCustomiseEquipment();
	void RefreshCustomiseEquipment();

	void OnVariantChanged();

	bool CheckRankRestrictions();
	void UpdatePreviousGameScores();

	inline CrySessionHandle GetCurrentSessionHandle() { return m_currentSession; }

	void RequestLeaveFromMenu();
	bool IsMidGameLeaving() const { return m_isMidGameLeaving; }

	void ChangeMap(const char *pMapName);
	void ChangeGameRules(const char *pGameRulesName);

	bool IsGameStarting() const { return m_startTimerCountdown; }

	bool IsBadServer(CrySessionID sessionId);
	EActiveStatus GetActiveStatus(const ELobbyState currentState) const;
	int32 CalculateAverageSkill();
	int32 GetCurrentLanguageId();

private:

	struct SFindGameResults
	{
		SFindGameResults(const char* name, CrySessionID sessionId, float score, bool bIsBadServer)
		{
			memcpy(&m_name, name, MAX_SESSION_NAME_LENGTH);
			m_sessionId = sessionId;
			m_score = score;
			m_isBadServer = (bIsBadServer ? 1 : 0);
		};

		char m_name[MAX_SESSION_NAME_LENGTH];
		CrySessionID m_sessionId;
		float m_score;
		uint8 m_isBadServer;
	};

	struct SFlashLobbyPlayerInfo
	{
		SFlashLobbyPlayerInfo()
		{
			m_conId=0;
			m_rank=0;
			m_reincarnations = 0;
			m_voiceState=0;
			m_teamId=0;

			m_entryType=eLET_Lobby;
			m_onLocalTeam=false;
			m_isLocal=false;
			m_isSquadMember=false;
			m_isSquadLeader=false;
		}

		CryFixedStringT<DISPLAY_NAME_LENGTH> m_nameString;
		uint32 m_conId;
		uint8 m_rank;
		uint8 m_reincarnations;
		uint8 m_voiceState;

		int m_teamId;
		ELobbyEntryType m_entryType;
		bool m_onLocalTeam;
		bool m_isLocal;
		bool m_isSquadMember;
		bool m_isSquadLeader;
	};

	struct SSlotReservation
	{
		SCryMatchMakingConnectionUID  m_con;
		float  m_timeStamp;
	};

	struct SVotingFlashInfo
	{
		CryFixedStringT<64> votingStatusMessage;

		int  leftNumVotes;
		int  rightNumVotes;
		bool  votingClosed;
		bool  votingDrawn;
		bool  leftWins;
		bool  localHasCandidates;
		bool  localHasVoted;
		bool  localVotedLeft;
#ifndef _RELEASE
		bool  tmpWatchInfoIsSet;
#endif
		void Reset()
		{
			votingStatusMessage.clear();

			leftNumVotes = 0;
			rightNumVotes = 0;
			votingClosed = false;
			votingDrawn = false;
			leftWins = false;
			localHasCandidates = false;
			localHasVoted = false;
			localVotedLeft = false;
#ifndef _RELEASE
			tmpWatchInfoIsSet = false;
#endif
		}
	};
	
	struct SVotingCandidatesFlashInfo
	{
		CryFixedStringT<64>  leftLevelMapPath;
		CryFixedStringT<64>  leftLevelName;
		CryFixedStringT<32>  leftRulesName;
		CryFixedStringT<128>  leftLevelImage;
		CryFixedStringT<64>  rightLevelMapPath;
		CryFixedStringT<64>  rightLevelName;
		CryFixedStringT<32>  rightRulesName;
		CryFixedStringT<128>  rightLevelImage;

#ifndef _RELEASE
		bool  tmpWatchInfoIsSet;
#endif
		void Reset()
		{
			leftLevelMapPath.clear();
			leftLevelName.clear();
			leftRulesName.clear();
			leftLevelImage.clear();
			rightLevelMapPath.clear();
			rightLevelName.clear();
			rightRulesName.clear();
			rightLevelImage.clear();
#ifndef _RELEASE
			tmpWatchInfoIsSet = false;
#endif
		}
	};

	struct SChatMessage
	{
		SChatMessage()
		{
			Clear();
		}

		void Set(SCryMatchMakingConnectionUID inConId, int inTeamId, const char* inMessage)
		{
			if (inMessage)
			{
				if (strlen(inMessage) > MAX_CHATMESSAGE_LENGTH-1)
				{
					message.assign(inMessage, MAX_CHATMESSAGE_LENGTH-1);
				}
				else
				{
					message = inMessage;
				}
			}
			conId = inConId;
			teamId = inTeamId;
		}

		void Clear()
		{
			message.clear();
			conId = CryMatchMakingInvalidConnectionUID;
			teamId = 0;
		}

		CryFixedStringT<MAX_CHATMESSAGE_LENGTH> message;
		SCryMatchMakingConnectionUID conId;
		int teamId;
	};

	typedef CryFixedStringT<MAX_CHATMESSAGE_LENGTH> TChatMessageDisplayString;
	struct SFlashChatMessage
	{
		SFlashChatMessage() { Clear(); }

		void Clear()
		{
			m_name.clear();
			m_message.clear();
			m_local = false;
		}

		void Set(const char* name, const char* message, bool isLocal)
		{
			Clear();

			m_name = name;
			m_message = message;
			m_local = isLocal;
		}

#ifdef USE_C2_FRONTEND
		void SendChatMessageToFlash(IFlashPlayer *pFlashPlayer, bool isInit);
#endif //#ifdef USE_C2_FRONTEND

		CryFixedStringT<DISPLAY_NAME_LENGTH> m_name;
		CryFixedStringT<MAX_CHATMESSAGE_LENGTH> m_message;
		bool m_local;
	};


	CLobbyTaskQueue m_taskQueue;

#if ENABLE_CHAT_MESSAGES
	SFlashChatMessage					m_chatMessagesArray[NUM_CHATMESSAGES_STORED];
	int												m_chatMessagesIndex;
#endif

	SChatMessage m_chatMessageStore;

	const static int k_maxBadServers = 4;
	typedef CryFixedArray<CrySessionID, k_maxBadServers> TBadServersArray;
	TBadServersArray m_badServers;
	int m_badServersHead;

	int m_findGameNumRetries;
	static int s_currentMMSearchID;

	const static int k_maxFoundGames = 20;
	typedef CryFixedArray<SFindGameResults, k_maxFoundGames> TFindGames;
	TFindGames m_findGameResults;

	char m_joinCommand[128];
	THUDWarningId m_DLCServerStartWarningId;

	SLobbyGameStartParams *m_gameStartParams;
	float m_startTimer;
	float m_findGameTimeout;
	float m_lastUserListUpdateTime;
	float m_timeTillCallToEnsureBestHost;
	float m_startTimerLength;
	float m_timeTillUpdateSession;

	EActiveStatus m_lastActiveStatus;
	int32 m_lastUpdatedAverageSkill;
	int m_numPlayerJoinResets;

	volatile static bool s_bShouldBeSearching;

	bool m_hasReceivedSessionQueryCallback;
	bool m_hasReceivedStartCountdownPacket;
	bool m_hasReceivedPlaylistSync;
	bool m_hasReceivedMapCommand;
	bool m_gameHadStartedWhenPlaylistRotationSynced;
	bool m_startTimerCountdown;
	bool m_initialStartTimerCountdown;
	bool m_isTeamGame;
	bool m_hasValidGameRules;
	bool m_shouldFindGames;
	bool m_privateGame;
	bool m_passwordGame;
	bool m_bMatchmakingSession;
	bool m_bWaitingForGameToFinish;
	bool m_bMigratedSession;
	bool m_bSessionStarted;
	bool m_bCancelling;
	bool m_bQuitting;
	bool m_bNeedToSetAsElegibleForHostMigration;
	bool m_bPlaylistHasBeenAdvancedThroughConsole;
	bool m_bSkipCountdown;			// Set by gl_startGame command
	bool m_allowRemoveUsers;
	bool m_bServerUnloadRequired;
	bool m_bChoosingGamemode;
	bool m_bHasUserList;
	bool m_bHasReceivedVoteOptions;
	bool m_isMidGameLeaving;
	bool m_bRetryIfPassworded;

	CrySessionID m_currentSessionId;
	CrySessionID m_nextSessionId;
	CGameLobbyManager* m_gameLobbyMgr;

	SSessionNames* m_reservationList;
	bool m_squadReservation;
	SSlotReservation m_slotReservations[MAX_RESERVATIONS];

	ELobbyAutomaticVOIPType m_autoVOIPMutingType;

	ELobbyVoteStatus m_localVoteStatus;
	ELobbyNetworkedVoteStatus m_networkedVoteStatus;

	CryFixedStringT<128> m_uiOverrideLevel;

	SVotingFlashInfo  m_votingFlashInfo;
	SVotingCandidatesFlashInfo  m_votingCandidatesFlashInfo;

#ifdef GAME_IS_CRYSIS2
	CAudioSignalPlayer m_lobbyCountdown;
#endif

#if INCLUDE_DEDICATED_LEADERBOARDS
	CryLobbyTaskID m_WriteUserDataTaskID;

	TWriteLeaderboardData m_writeLeaderboardData;

	typedef CryFixedArray<SWriteUserData, MAX_USER_DATAS> TWriteUserData;
	TWriteUserData m_writeUserData;

	typedef CryFixedArray<SOnlineAttributeTask, MAX_ONLINE_ATTRIBUTE_TASKS> TOnlineAttributeTask;
	TOnlineAttributeTask m_onlineAttributeTasks;

	float m_writeUserDataTimer;
#endif

	void EnterState(ELobbyState prevState, ELobbyState newState);
	void UpdateState();

	void StartGame();
	void UpdateLevelRotation();

	void ClearChatMessages();
	void RecievedChatMessage(const SChatMessage* message);
	void SendChatMessageToClients();

	void SvInitialiseRotationAdvanceAtFirstLevel();
	void SvResetVotingForNextElection();
	void SvVoteForLevel(const bool voteLeft);
	void SvCloseVotingAndDecideWinner();
	void AdvanceLevelRotationAccordingToVotingResults();

	const char* GetMapDescription(const char* levelFileName, CryFixedStringT<64>* pOutLevelDescription);
	
	void SetCurrentSession(CrySessionHandle h);
	void SetCurrentId(CrySessionID id, bool isCreator, bool isMigratedSession);

	void SendPacket(CCryLobbyPacket *pPacket, GameUserPacketDefinitions packetType, SCryMatchMakingConnectionUID connectionUID = CryMatchMakingInvalidConnectionUID);
	void SendPacket(GameUserPacketDefinitions packetType, SCryMatchMakingConnectionUID connectionUID = CryMatchMakingInvalidConnectionUID);
	void ReadPacket(SCryLobbyUserPacketData** ppPacketData);

	void FindGameEnter();

	float GetFindGameTimeout();

	void JoinServerFailed(ECryLobbyError error, CrySessionID serverSessionId);
	void InformSquadManagerOfSessionId();

	static int BuildReservationsRequestList(SCryMatchMakingConnectionUID reservationRequests[], const int maxRequests, const SSessionNames*  members);
	EReservationResult DoReservations(const int numReservationsRequested, const SCryMatchMakingConnectionUID requestedReservations[]);

	void InsertUser(SCryUserInfoResult* user);
	void UpdateUser(SCryUserInfoResult* user);
	void RemoveUser(SCryUserInfoResult* user);
	void BalanceTeams();
	void GameRulesChanged(const char *pGameRules);

	static bool SortPlayersByTeam(const SFlashLobbyPlayerInfo &elem1, const SFlashLobbyPlayerInfo &elem2);
	uint32 GetSessionCreateFlags() const;

#if USE_CRYLOBBY_GAMESPY
	void CheckGetServerImage();
#endif

	void SetLobbyTaskId(CryLobbyTaskID taskId);


	void RecordReceiptOfSessionQueryCallback() { ENSURE_ON_MAIN_THREAD; m_hasReceivedSessionQueryCallback = true; }
	void SetupSessionData();
	void ClearBadServers();

	bool AllowOnlineAttributeTasks();
	void AddOnlineAttributeTask(CryUserID id, EOnlineAttributeTaskType type);
	void TickOnlineAttributeTasks();
	SOnlineAttributeTask* FindOnlineAttributeTask(CryLobbyTaskID taskID);
	void RemoveOnlineAttributeTask(CryUserID userID, bool allowStartedTaskRemoval = false);
	void ClearOnlineAttributeTask(SOnlineAttributeTask *pTask);
	bool IsOnlineAttributeTaskInProgress();

	void UpdatePrivatePasswordedGame();

	bool BidirectionalMergingRequiresCancel(CrySessionID other);

	// user data management
	int FindWriteUserDataByUserID(CryUserID id);
	void TickWriteUserData(float dt);

	// Callbacks
	static void MatchmakingSessionCreateCallback(CryLobbyTaskID taskID, ECryLobbyError error, CrySessionHandle h, void* arg);
	static void MatchmakingSessionMigrateCallback(CryLobbyTaskID taskID, ECryLobbyError error, CrySessionHandle h, void* arg);
	static void MatchmakingSessionJoinCallback(CryLobbyTaskID taskID, ECryLobbyError error, CrySessionHandle h, uint32 ip, uint16 port, void* arg);
	static void MatchmakingSessionDeleteCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* arg);
	static void MatchmakingSessionQueryCallback(CryLobbyTaskID taskID, ECryLobbyError error, SCrySessionSearchResult* session, void* arg);
	static void MatchmakingSessionUpdateCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* arg);
	static void MatchmakingSessionUserPacketCallback(UCryLobbyEventData eventData, void *userParam);
	static void MatchmakingSessionRoomOwnerChangedCallback(UCryLobbyEventData eventData, void *userParam);
	static void MatchmakingSessionJoinUserCallback(UCryLobbyEventData eventData, void *userParam);
	static void MatchmakingSessionLeaveUserCallback(UCryLobbyEventData eventData, void *userParam);
	static void MatchmakingSessionUpdateUserCallback(UCryLobbyEventData eventData, void *userParam);
	static void MatchmakingSessionClosedCallback(UCryLobbyEventData eventData, void *userParam);
	static void MatchmakingSessionKickedCallback(UCryLobbyEventData eventData, void *userParam);
	static void MatchmakingLocalUserDataCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* arg);
	static void MatchmakingEnsureBestHostCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* arg);
	static void MatchmakingSessionStartCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pArg);
	static void MatchmakingSessionEndCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pArg);
	static void MatchmakingSessionTerminateHostHintingForGroupCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pArg);
	static void MatchmakingSessionSetLocalFlagsCallback(CryLobbyTaskID taskID, ECryLobbyError error, CrySessionHandle h, uint32 flags, void* pArg);
	static void MatchmakingForcedFromRoomCallback(UCryLobbyEventData eventData, void *pArg);
	static void MatchmakingSessionDetailedInfoRequestCallback(UCryLobbyEventData eventData, void *pArg);
	static void MatchmakingSessionDetailedInfoResponseCallback(CryLobbyTaskID taskID, ECryLobbyError error, CCryLobbyPacket* pPacket, void* pArg);
	
	static void WriteOnlineAttributeData(CGameLobby *pLobby, CCryLobbyPacket *pPacket, GameUserPacketDefinitions packetType, SCryLobbyUserData *pData, int32 numData, SCryMatchMakingConnectionUID connnectionUID);
	static void ReadOnlineAttributeData(CCryLobbyPacket *pPacket, SSessionNames::SSessionName *pSessionName, IPlayerProfileManager *pPlayerProfileManager);

	static void ReadOnlineDataCallback(CryLobbyTaskID taskID, ECryLobbyError error, SCryLobbyUserData* pData, uint32 numData, void* pArg);
	static void WriteOnlineDataCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pArg);

	static void TaskStartedCallback(CLobbyTaskQueue::ESessionTask task, void *pArg);

	static void SendChatMessageAllCheckProfanityCallback( CryLobbyTaskID taskID, ECryLobbyError error, const char* pString, bool isProfanity, void* pArg );
	static void SendChatMessageTeamCheckProfanityCallback( CryLobbyTaskID taskID, ECryLobbyError error, const char* pString, bool isProfanity, void* pArg );
	void SendChatMessageCheckProfanityCallback( const bool team, CryLobbyTaskID taskID, ECryLobbyError error, const char* pString, bool isProfanity, void* pArg );

	static void MatchmakingSessionClosed(UCryLobbyEventData eventData, void *userParam, int reason);	// reason is of type CMPMenuHub::EDisconnectError

private:
	typedef CryFixedStringT<6> TSmallString;
	typedef CryFixedArray<SCryMatchMakingConnectionUID, MAX_PLAYER_LIMIT> TPlayerList;
	typedef CryFixedArray<SPlayerScores, MAX_PLAYER_LIMIT> TPlayerScoresList;

	enum EPlayerGroupType
	{
		ePGT_Unknown,
		ePGT_Squad,
		ePGT_Clan,
		ePGT_Individual,
	};

	struct SPlayerGroup
	{
		SPlayerGroup();

		void Reset();

		void InitClan(const char *pClanName);
		void InitIndividual(const SCryMatchMakingConnectionUID *pConId, CGameLobby *pGameLobby);
		void InitSquad(const SCryMatchMakingConnectionUID *pLeader);

		void AddMember(const SCryMatchMakingConnectionUID *pConId, CGameLobby *pGameLobby);
		void RemoveMember(const SCryMatchMakingConnectionUID *pConId, CGameLobby *pGameLobby);
		void RemoveMember(const SCryMatchMakingConnectionUID *pConId, int skill);

		TSmallString m_clanTag;
		SCryMatchMakingConnectionUID m_pSquadLeader;

		uint32 m_totalSkill;
		uint32 m_totalPrevScore;
		int m_numMembers;

		EPlayerGroupType m_type;

		TPlayerList m_members;

		bool m_valid;
	};

	SPlayerGroup *FindGroupByConnectionUID(const SCryMatchMakingConnectionUID *pConId);
	SPlayerGroup *FindGroupByClan(const char *pClanName);
	SPlayerGroup *FindEmptyGroup();
	uint8 GetRank(const SCryMatchMakingConnectionUID *pConId);
	uint16 GetSkill(const SCryMatchMakingConnectionUID *pConId);
	uint16 GetPrevScore(const SCryMatchMakingConnectionUID *pConId);
	bool GetConnectionUIDFromUserID(const CryUserID userId, SCryMatchMakingConnectionUID &result);
	bool GetUserIDFromConnectionUID(const SCryMatchMakingConnectionUID conId, CryUserID &result);
	void UpdatePlayerGroup(SSessionNames::SSessionName *pPlayer, uint16 previousSkill);

	void MutePlayersOnTeam(uint8 teamId, bool mute);
	void MutePlayerBySessionName(SSessionNames::SSessionName *pUser, bool mute, int reason);
	void SetAutomaticMutingStateForPlayer(SSessionNames::SSessionName *pPlayer, ELobbyAutomaticVOIPType newType);

	void OnGameStarting();

	static bool ComparePlayerGroups(SPlayerGroup *lhs, SPlayerGroup *rhs);

	bool ShouldCheckForBestHost();

	void CheckCanLeave();

	void SetState(ELobbyState state);
	void FinishDelete(ECryLobbyError result);

	bool NetworkCallbackReceived( CryLobbyTaskID taskId, ECryLobbyError result );

	void CancelLobbyTask(CLobbyTaskQueue::ESessionTask taskType);
	void CancelAllLobbyTasks();

	void SessionEndCleanup();
	ECryLobbyError DoCreateSession(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId);
	ECryLobbyError DoMigrateSession(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId);
	ECryLobbyError DoJoinServer(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId);
	ECryLobbyError DoDeleteSession(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId);
	ECryLobbyError DoUpdateLocalUserData(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId);
	ECryLobbyError DoStartSession(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId, bool &bTaskStartedOut);
	ECryLobbyError DoEndSession(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId);
	ECryLobbyError DoQuerySession(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId);
	ECryLobbyError DoUpdateSession(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId);
	ECryLobbyError DoEnsureBestHost(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId);
	ECryLobbyError DoTerminateHostHintingForGroup(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId, bool &bTaskStartedOut);
	ECryLobbyError DoSessionSetLocalFlags(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId);
	ECryLobbyError DoSessionDetailedInfo(ICryMatchMaking *pMatchMaking, CryLobbyTaskID &taskId);
	void DoUnload();

	void ConnectionFailed(int reason);	// reason is of type CMPMenuHub::EDisconnectError

#ifndef _RELEASE
	static void CmdTestTeamBalancing(IConsoleCmdArgs *pArgs);
	static void CmdTestMuteTeam(IConsoleCmdArgs *pArgs);
	static void CmdCallEnsureBestHost(IConsoleCmdArgs *pArgs);
	static void CmdFillReservationSlots(IConsoleCmdArgs *pArgs);

	float m_timeTillAutoLeaveLobby;
	int m_failedSearchCount;
	bool m_migrationStarted;
#endif

	static void CmdAdvancePlaylist(IConsoleCmdArgs *pArgs);
	static void CmdDumpValidMaps(IConsoleCmdArgs *pArgs);

	void SetLocalVoteStatus(ELobbyVoteStatus state);
	ELobbyNetworkedVoteStatus GetVotingStateForPlayer(SSessionNames::SSessionName *pSessionName) const;
	void CheckForVotingChanges(bool bUpdateFlash);
	bool CalculateVotes();
	void ResetLocalVotingData();

	void CheckForSkillChange();

	void UpdateRulesAndMapFromVoting();
	void UpdateRulesAndLevel(const char *pGameRules, const char *pLevelName);
	void UpdateVoteChoices();

	bool IsQuitting() { return m_bQuitting; }
	void DoBetweenRoundsUnload();

	int GetNumPublicSlots() { return m_sessionData.m_numPublicSlots; }
	int GetNumPrivateSlots() { return m_sessionData.m_numPrivateSlots; }

	static const int MAX_PLAYER_GROUPS = MAX_SESSION_NAMES;
	SPlayerGroup m_playerGroups[MAX_PLAYER_GROUPS];
	TPlayerScoresList m_previousGameScores;

	CryFixedStringT<64> m_currentLevelName;
	CryFixedStringT<32> m_currentGameRules;

	CryFixedStringT<64> m_loadingLevelName;
	CryFixedStringT<32> m_loadingGameRules;

	CryFixedStringT<64> m_pendingLevelName;

	CryLobbyTaskID m_profanityTask;

#if !defined(_RELEASE)
	public:
	static unsigned int s_mainThreadHandle;
#endif
};

#endif // ___GAME_LOBBY_H___
