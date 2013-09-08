#include "StdAfx.h"

#include "GameBrowser.h"
#include "GameLobbyData.h"

#include "ICryLobby.h"
#include "ICryMatchMaking.h"
#include "IGameFramework.h"
#include "IPlayerProfiles.h"

#include "StringUtils.h"

#if IMPLEMENT_PC_BLADES
#include "Network/Lobby/GameServerLists.h"
#endif

#include "Game.h"

#ifdef GAME_IS_CRYSIS2
#include "PersistantStats.h"
#include "PlaylistManager.h"
#include "HUD/HUDUtils.h"
#include "FrontEnd/FlashFrontEnd.h"
#include "FrontEnd/Multiplayer/MPMenuHub.h"
#include "FrontEnd/Multiplayer/UIServerList.h"
#endif

#include "Endian.h"

#include "GameCVars.h"
#include "Utility/StringUtils.h"
//#include "GameCodeCoverage/GameCodeCoverageTracker.h"
#include "Network/Squad/SquadManager.h"

#include <IPlayerProfiles.h>
#include <IPlatformOS.h>

#if USE_CRYLOBBY_GAMESPY_VOIP
#include "CryGameSpyVoiceCodec.h"
#endif // USE_CRYLOBBY_GAMESPY_VOIP

#if defined(USE_SESSION_SEARCH_SIMULATOR)
#include "SessionSearchSimulator.h"
#endif










static char s_profileName[CRYLOBBY_USER_NAME_LENGTH] = "";

#if USE_CRYLOBBY_GAMESPY
#define START_SEARCHING_FOR_SERVERS_NUM_DATA	5
#else
#define START_SEARCHING_FOR_SERVERS_NUM_DATA	3
#endif

#define MAX_NUM_PER_FRIEND_ID_SEARCH 15	// Number of favourite Id's to search for at once.
#define MIN_TIME_BETWEEN_SEARCHES 1.f		// Time in secs before performing another search query. See m_delayedSearchType. Excludes favouriteId searches.

//-------------------------------------------------------------------------
CGameBrowser::CGameBrowser( void )
{
#if defined(USE_SESSION_SEARCH_SIMULATOR)
	m_pSessionSearchSimulator = NULL;
#endif //defined(USE_SESSION_SEARCH_SIMULATOR)

	m_NatType = eNT_Unknown;

	m_searchingTask = CryLobbyInvalidTaskID;

	m_hasEnteredLoginDetails = false;
	m_loginGUIState = eCLLGS_NotFinished; 

	m_bFavouriteIdSearch = false;

	m_delayedSearchType = eDST_None;

	m_lastSearchTime = 0.f;

#if IMPLEMENT_PC_BLADES
	m_currentFavouriteIdSearchType = CGameServerLists::eGSL_Favourite;
	m_currentSearchFavouriteIdIndex = 0;
	m_numSearchFavouriteIds = 0;
	memset(m_searchFavouriteIds, INVALID_SESSION_FAVOURITE_ID, sizeof(m_searchFavouriteIds));
#endif

	Init(); // init and trigger a request to register data - needs to be done early.
}

//-------------------------------------------------------------------------
void CGameBrowser::Init( void )
{
	gEnv->pNetwork->GetLobby()->RegisterEventInterest(eCLSE_NatType, CGameBrowser::GetNatTypeCallback, this);

#if defined(USE_SESSION_SEARCH_SIMULATOR)
	m_pSessionSearchSimulator = new CSessionSearchSimulator();
#endif //defined(USE_SESSION_SEARCH_SIMULATOR)
}

//--------------------------------------------------------------------------
CGameBrowser::~CGameBrowser()
{
#if defined(USE_SESSION_SEARCH_SIMULATOR)
	SAFE_DELETE( m_pSessionSearchSimulator );
#endif //defined(USE_SESSION_SEARCH_SIMULATOR)
}

//-------------------------------------------------------------------------
void CGameBrowser::StartSearchingForServers(CryMatchmakingSessionSearchCallback cb)
{
	ICryLobby *lobby = gEnv->pNetwork->GetLobby();
	if (lobby != NULL && lobby->GetLobbyService())
	{
#ifdef GAME_IS_CRYSIS2
		CCCPOINT (GameLobby_StartSearchingForServers);
#endif

		if (CanStartSearch())
		{
			CryLog("[UI] Delayed Searching for sessions");
			m_delayedSearchType = eDST_Full;

#ifdef USE_C2_FRONTEND
			if (CMPMenuHub *pMPMenu = CMPMenuHub::GetMPMenuHub())
			{
				pMPMenu->StartSearching();
			}
#endif //#ifdef USE_C2_FRONTEND

			return;
		}

		SCrySessionSearchParam param;
		SCrySessionSearchData data[START_SEARCHING_FOR_SERVERS_NUM_DATA];

		param.m_type = REQUIRED_SESSIONS_QUERY;
		param.m_data = data;



		param.m_numFreeSlots = 0; 

		param.m_maxNumReturn = g_pGameCVars->g_maxGameBrowserResults;
		param.m_ranked = false;

		int curData = 0;









		CRY_ASSERT_MESSAGE( curData < START_SEARCHING_FOR_SERVERS_NUM_DATA, "Session search data buffer overrun" );
		data[curData].m_operator = eCSSO_Equal;
		data[curData].m_data.m_id = LID_MATCHDATA_VERSION;
		data[curData].m_data.m_type = eCLUDT_Int32;
		data[curData].m_data.m_int32 = GameLobbyData::GetVersion();
		curData++;

#ifdef GAME_IS_CRYSIS2
		if (!g_pGameCVars->g_ignoreDLCRequirements)
		{
			// Note: GetSquadCommonDLCs is actually a bit field, so it should really be doing a bitwise & to determine
			// if the client can join the server. However this is not supported so the less than equal operator
			// is used instead. This may return some false positives but never any false negatives, the false
			// positives will be filtered out when the results are retreived.
			CRY_ASSERT_MESSAGE( curData < START_SEARCHING_FOR_SERVERS_NUM_DATA, "Session search data buffer overrun" );
			data[curData].m_operator = eCSSO_LessThanEqual;
			data[curData].m_data.m_id = LID_MATCHDATA_REQUIRED_DLCS;
			data[curData].m_data.m_type = eCLUDT_Int32;
			data[curData].m_data.m_int32 = g_pGame->GetDLCManager()->GetSquadCommonDLCs();
			curData++;
		}
#endif

#if USE_CRYLOBBY_GAMESPY
		uint32	region = eSR_All;	// Game side support for region filtering needs to change this.

		if ( region != eSR_All )
		{
			CRY_ASSERT_MESSAGE( curData < START_SEARCHING_FOR_SERVERS_NUM_DATA, "Session search data buffer overrun" );
			data[curData].m_operator = eCSSO_BitwiseAndNotEqualZero;
			data[curData].m_data.m_id = LID_MATCHDATA_REGION;
			data[curData].m_data.m_type = eCLUDT_Int32;
			data[curData].m_data.m_int32 = region;
			curData++;
		}

		int32		favouriteID = 0;	// Game side support for favourite servers needs to change this.

		if ( favouriteID )
		{
			CRY_ASSERT_MESSAGE( curData < START_SEARCHING_FOR_SERVERS_NUM_DATA, "Session search data buffer overrun" );
			data[curData].m_operator = eCSSO_Equal;
			data[curData].m_data.m_id = LID_MATCHDATA_FAVOURITE_ID;
			data[curData].m_data.m_type = eCLUDT_Int32;
			data[curData].m_data.m_int32 = favouriteID;
			curData++;
		}

#endif

		param.m_numData = curData;

		CRY_ASSERT_MESSAGE(m_searchingTask==CryLobbyInvalidTaskID,"CGameBrowser Trying to search for sessions when you think you are already searching.");

		ECryLobbyError error = StartSearchingForServers(&param, cb, this, false);


		CRY_ASSERT_MESSAGE(error==eCLE_Success,"CGameBrowser searching for sessions failed.");

#ifdef USE_C2_FRONTEND
		CMPMenuHub *pMPMenu = CMPMenuHub::GetMPMenuHub();

		if (error == eCLE_Success)
		{
			if (pMPMenu)
			{
				pMPMenu->StartSearching();
			}

			CryLogAlways("CCGameBrowser::StartSearchingForServers %d", m_searchingTask);
		}
		else
		{
			if (pMPMenu)
			{
				pMPMenu->SearchComplete();
			}

			m_searchingTask = CryLobbyInvalidTaskID;
		}
#endif //#ifdef USE_C2_FRONTEND
	}
	else
	{
		CRY_ASSERT_MESSAGE(0,"CGameBrowser Cannot search for servers : no lobby service available.");
	}
}

//-------------------------------------------------------------------------
ECryLobbyError CGameBrowser::StartSearchingForServers(SCrySessionSearchParam* param, CryMatchmakingSessionSearchCallback cb, void* cbArg, const bool bFavouriteIdSearch)
{
	m_bFavouriteIdSearch = bFavouriteIdSearch;
	m_delayedSearchType = eDST_None;
	m_lastSearchTime = gEnv->pTimer->GetCurrTime(); 

#if defined(USE_SESSION_SEARCH_SIMULATOR)	
	if( m_pSessionSearchSimulator && gEnv->pConsole->GetCVar( "gl_searchSimulatorEnabled" )->GetIVal() )
	{
		const char* filepath = gEnv->pConsole->GetCVar( "gl_searchSimulatorFilepath" )->GetString();
		if( filepath != NULL && strcmpi( filepath, m_pSessionSearchSimulator->GetCurrentFilepath() ) != 0  )
		{
			m_pSessionSearchSimulator->OpenSessionListXML( filepath );
		}

		m_pSessionSearchSimulator->OutputSessionListBlock( m_searchingTask, cb, cbArg );
		return eCLE_Success;
	}
	else
#endif //defined(USE_SESSION_SEARCH_SIMULATOR)
	{
		ECryLobbyError error = eCLE_ServiceNotSupported;
		ICryLobby *lobby = gEnv->pNetwork->GetLobby();
		IPlayerProfileManager *pPlayerProfileManager = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager();
		uint32 userIndex = pPlayerProfileManager ? pPlayerProfileManager->GetExclusiveControllerDeviceIndex() : 0;
		if(lobby)
		{
			CryLobbyTaskID previousTask = m_searchingTask;
			error = lobby->GetLobbyService()->GetMatchMaking()->SessionSearch(userIndex, param, &m_searchingTask, cb, cbArg);
			CryLog("CGameBrowser::StartSearchingForServers previousTask=%u, newTask=%u", previousTask, m_searchingTask);
		}
		return error;
	}
}

//-------------------------------------------------------------------------
void CGameBrowser::CancelSearching(bool feedback /*= true*/)
{
	CryLogAlways("CGameBrowser::CancelSearching");

	if (m_searchingTask != CryLobbyInvalidTaskID)
	{
		CryLog("  canceling search task %u", m_searchingTask);
		ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
		pLobby->GetMatchMaking()->CancelTask(m_searchingTask);
		// Calling FinishedSearch will clear m_searchingTask
	}

	m_delayedSearchType = eDST_None;

	FinishedSearch(feedback, true);
}

//-------------------------------------------------------------------------
void CGameBrowser::FinishedSearch(bool feedback, bool finishedSearch)
{
	CryLog("CGameBrowser::FinishedSearch()");
	m_searchingTask = CryLobbyInvalidTaskID;

#if IMPLEMENT_PC_BLADES
	if (m_bFavouriteIdSearch)
	{
		bool bSearchOver = true;

		if (!finishedSearch && (m_currentSearchFavouriteIdIndex < m_numSearchFavouriteIds))
		{
			if (DoFavouriteIdSearch())
			{
				feedback = false;
				bSearchOver = false;
			}
		}

		if (bSearchOver)	
		{
			for (uint32 i = 0; i < m_numSearchFavouriteIds; ++i)
			{
				if (m_searchFavouriteIds[i] != INVALID_SESSION_FAVOURITE_ID)
				{
					g_pGame->GetGameServerLists()->ServerNotFound(m_currentFavouriteIdSearchType, m_searchFavouriteIds[i]);
				}

				m_searchFavouriteIds[i] = INVALID_SESSION_FAVOURITE_ID;
			}
		}
	}
#endif
		
	if(feedback)
	{
#ifdef USE_C2_FRONTEND
		if (CMPMenuHub *pMPMenu = CMPMenuHub::GetMPMenuHub())
		{
			pMPMenu->SearchComplete();
		}
#endif //#ifdef USE_C2_FRONTEND
	}
}

bool CGameBrowser::CanStartSearch()
{
	const float currTime = gEnv->pTimer->GetCurrTime();
	return ((m_lastSearchTime + MIN_TIME_BETWEEN_SEARCHES) >= currTime);
}

//---------------------------------------
void CGameBrowser::Update(const float dt)
{
	if (m_delayedSearchType != eDST_None)
	{
		if (m_searchingTask==CryLobbyInvalidTaskID && !CanStartSearch())
		{
			CryLog("[UI] Activate delayed search %d", m_delayedSearchType);
			if (m_delayedSearchType == eDST_Full)
			{
				StartSearchingForServers();
			}
			else if (m_delayedSearchType == eDST_FavouriteId)
			{
				if (!DoFavouriteIdSearch())
				{
#ifdef USE_C2_FRONTEND
					if (CMPMenuHub *pMPMenu = CMPMenuHub::GetMPMenuHub())
					{
						pMPMenu->SearchComplete();
					}
#endif //#ifdef USE_C2_FRONTEND
				}
			}

			m_delayedSearchType = eDST_None;
		}
	}
}

//------------
// CALLBACKS
//------------

// This is a new callback since adding PSN support, Gamespy will also likely fire this callback.
//Basically whenever this callback is fired, you are required to fill in the requestedParams that are asked for.
//At present specifications are that the callback can fire multiple times.
//
// 'PCom'		void*		ptr to static sceNpCommunitcationId					(not copied - DO NOT PLACE ON STACK!)
// 'PPas'		void*		ptr to static sceNpCommunicationPassphrase	(not copied - DO NOT PLACE ON STACK!)
// 'XSvc'		uint32	Service Id for XLSP servers
// 'XPor'		uint16	Port for XLSP server to communicate with Telemetry
// 'LUnm'		void*		ptr to user name for local user - used by LAN (due to lack of guid) (is copied internally - DO NOT PLACE ON STACK)
//

#if USE_CRYLOBBY_GAMESPY

#define GAMESPY_GAMEVERSION	( 1 )			// Needs to live in a .dll that gets updated when the game gets patched.
#define GAMESPY_GAMEVERSIONSTRING	( "CryGameSDK PC 1.00" )			// Needs to live in a .dll that gets updated when the game gets patched.
#define GAMESPY_DISTRIBUTIONID	( 0 )
#define GAMESPY_PRODUCTID	( 13429 )
#define GAMESPY_GAMEID		( 3300 )
#define GAMESPY_NAMESPACEID	( 95 )
#define GAMESPY_PARTNERID	( 95 )
#define GAMESPY_REQUIREDNICK ( "MyCrysis" )
#define GAMESPY_P2P_LEADERBOARD_FORMAT	( "LDB%d" )
#define GAMESPY_DEDICATED_LEADERBOARD_FORMAT	( "DEDICATED%d" )
#define GAMESPY_P2P_STATS_TABLE	( "STATS" )
#define GAMESPY_DEDICATED_STATS_TABLE	( "DEDICATEDSTATS" )
#if defined( DEDICATED_SERVER )
#define GAMESPY_DEDI_VERSION	( 1 )
#endif

#endif // USE_CRYLOBBY_GAMESPY




































































































#if defined(PS3) || USE_CRYLOBBY_GAMESPY

/* static */
const char* CGameBrowser::GetGameModeStringFromId(int32 id)
{
	char *strGameMode = NULL;
	switch(id)
	{
#ifdef GAME_IS_CRYSIS2
	case RICHPRESENCE_GAMEMODES_INSTANTACTION:
		strGameMode = "@ui_rules_InstantAction";
		break;

	case RICHPRESENCE_GAMEMODES_TEAMINSTANTACTION:
		strGameMode = "@ui_rules_TeamInstantAction";
		break;

	case RICHPRESENCE_GAMEMODES_ASSAULT:
		strGameMode = "@ui_rules_Assault";
		break;

	case RICHPRESENCE_GAMEMODES_CAPTURETHEFLAG:
		strGameMode = "@ui_rules_CaptureTheFlag";
		break;

	case RICHPRESENCE_GAMEMODES_EXTRACTION:
		strGameMode = "@ui_rules_Extraction";
		break;

	case RICHPRESENCE_GAMEMODES_CRASHSITE:
		strGameMode = "@ui_rules_CrashSite";
		break;

	case RICHPRESENCE_GAMEMODES_ALLORNOTHING:
		strGameMode = "@ui_rules_AllOrNothing";
		break;

	case RICHPRESENCE_GAMEMODES_BOMBTHEBASE:
		strGameMode = "@ui_rules_BombTheBase";
		break;

	case RICHPRESENCE_GAMEMODES_POWERSTRUGGLE:
		strGameMode = "@ui_rules_PowerStruggleLite";
		break;

	default:
		CRY_ASSERT_MESSAGE(false, "Failed to find game rules rich presence string");
		break;	
#else
	case 0: //fallthrough to prevent warning
	default:
		CRY_ASSERT_MESSAGE(false, "Failed to find game rules rich presence string");
		break;	
	}
#endif
	return strGameMode;
}

/* static */
const char* CGameBrowser::GetMapStringFromId(int32 id)
{
	char *strMap = NULL;
	switch(id)
	{
#ifdef GAME_IS_CRYSIS2
	case RICHPRESENCE_MAPS_ROOFTOPGARDENS:
		strMap = "@mp_map_rooftop_gardens";
		break;

	case RICHPRESENCE_MAPS_PIER17:
		strMap = "@mp_map_pier";
		break;

	case RICHPRESENCE_MAPS_WALLSTREET:
		strMap = "@mp_map_wallstreet";
		break;

	case RICHPRESENCE_MAPS_LIBERTYISLAND:
		strMap = "@mp_map_liberty";
		break;

	case RICHPRESENCE_MAPS_HARLEMGORGE:
		strMap = "@mp_map_harlem_gorge";
		break;

	case RICHPRESENCE_MAPS_COLLIDEDBUILDINGS:
		strMap = "@mp_map_collided_buildings";
		break;

	case RICHPRESENCE_MAPS_ALIENCRASHTRAIL:
		strMap = "@mp_map_alien_crash_trail";
		break;

	case RICHPRESENCE_MAPS_ALIENVESSEL:
		strMap = "@mp_map_alien_vessel";
		break;

	case RICHPRESENCE_MAPS_BATTERYPARK:
		strMap = "@mp_map_battery_park";
		break;

	case RICHPRESENCE_MAPS_BRYANTPARK:
		strMap = "@mp_map_bryant_park";
		break;

	case RICHPRESENCE_MAPS_CHURCH:
		strMap = "@mp_map_church";
		break;

	case RICHPRESENCE_MAPS_CONEYISLAND:
		strMap = "@mp_map_coney_island";
		break;

	case RICHPRESENCE_MAPS_DOWNTOWN:
		strMap = "@mp_map_downtown";
		break;

	case RICHPRESENCE_MAPS_LIGHTHOUSE:
		strMap = "@mp_map_lighthouse";
		break;

	case RICHPRESENCE_MAPS_PARADE:
		strMap = "@mp_map_parade";
		break;

	case RICHPRESENCE_MAPS_ROOSEVELT:
		strMap = "@mp_map_roosevelt";
		break;

	case RICHPRESENCE_MAPS_CITYHALL:
		strMap = "@mp_map_city_hall";
		break;

	case RICHPRESENCE_MAPS_ALIENVESSELSMALL:
		strMap = "@mp_map_alien_vessel_small";
		break;

	case RICHPRESENCE_MAPS_PIERSMALL:
		strMap = "@mp_map_pier_small";
		break;

	case RICHPRESENCE_MAPS_LIBERTYISLAND_MIL:
		strMap = "@mp_map_liberty_mil";
		break;

	case RICHPRESENCE_MAPS_TERMINAL:
		strMap = "@mp_map_terminal";
		break;

	case RICHPRESENCE_MAPS_DLC_1_MAP_1:
		strMap = "@mp_dlc1_map1";
		break;

	case RICHPRESENCE_MAPS_DLC_1_MAP_2:
		strMap = "@mp_dlc1_map2";
		break;

	case RICHPRESENCE_MAPS_DLC_1_MAP_3:
		strMap = "@mp_dlc1_map3";
		break;

	case RICHPRESENCE_MAPS_DLC_2_MAP_1:
		strMap = "@mp_dlc2_map1";
		break;

	case RICHPRESENCE_MAPS_DLC_2_MAP_2:
		strMap = "@mp_dlc2_map2";
		break;

	case RICHPRESENCE_MAPS_DLC_2_MAP_3:
		strMap = "@mp_dlc2_map3";
		break;

	case RICHPRESENCE_MAPS_DLC_3_MAP_1:
		strMap = "@mp_dlc3_map1";
		break;

	case RICHPRESENCE_MAPS_DLC_3_MAP_2:
		strMap = "@mp_dlc3_map2";
		break;

	case RICHPRESENCE_MAPS_DLC_3_MAP_3:
		strMap = "@mp_dlc3_map3";
		break;

	case RICHPRESENCE_MAPS_LIBERTYISLAND_STATUE:
		strMap = "@mp_map_liberty_statue";
		break;

	default:
		CRY_ASSERT_MESSAGE(false, "Failed to find map rich presence string");
		break;	
#else
	case 0: // deliberate fallthrough to prevent compile warning
	default:
		CRY_ASSERT_MESSAGE(false, "Failed to find map rich presence string");
		break;	
	}
#endif
	return strMap;
}

/* static */
void CGameBrowser::LocalisePresenceString(CryFixedStringT<MAX_PRESENCE_STRING_SIZE> &out, const char* stringId)
{
#ifdef GAME_IS_CRYSIS2
#if USE_CRYLOBBY_GAMESPY
	out = stringId;
#else
	out = CHUDUtils::LocalizeString( stringId );
#endif
#else
	out = stringId;
#endif
}

/*static*/
void CGameBrowser::UnpackRecievedInGamePresenceString(CryFixedStringT<MAX_PRESENCE_STRING_SIZE> &out, const CryFixedStringT<MAX_PRESENCE_STRING_SIZE>& inString)
{
#if USE_CRYLOBBY_GAMESPY

	const int firstIntStart = inString.find(':');
	const int lastIntStart = inString.rfind(':');

	CryFixedStringT<MAX_PRESENCE_STRING_SIZE> stringId( inString.substr(0, firstIntStart) );
	CryFixedStringT<MAX_PRESENCE_STRING_SIZE> sGameModeId(inString.substr(firstIntStart+1, lastIntStart));
	CryFixedStringT<MAX_PRESENCE_STRING_SIZE> sMapId(inString.substr(lastIntStart+1, inString.length()));

	const uint32 gameModeId = atoi(sGameModeId.c_str());
	const uint32 mapId = atoi(sMapId.c_str());

	const char* gamemodeStringId = GetGameModeStringFromId(gameModeId);
	const char* mapIDString = GetMapStringFromId(mapId);
	ILocalizationManager* pLocMgr = gEnv->pSystem->GetLocalizationManager();
	wstring translated;
	pLocMgr->LocalizeString( mapIDString, translated );
	const bool haveMapString = (translated.length()) && (translated[0] != '@');
	if(haveMapString)
	{
		out = inString;//CHUDUtils::LocalizeString(stringId.c_str(), gamemodeStringId, mapIDString);
	}
	else
	{
		out = inString;//CHUDUtils::LocalizeString("@mp_rp_gameplay_unknownmap", gamemodeStringId);
	}

#else
	out = inString;
#endif
}

#ifdef GAME_IS_CRYSIS2
/*static*/
void CGameBrowser::LocaliseInGamePresenceString(CryFixedStringT<MAX_PRESENCE_STRING_SIZE> &out, const char* stringId, const int32 gameModeId, const int32 mapId)
{
#if USE_CRYLOBBY_GAMESPY
	out.Format("%s:%d:%d", stringId, gameModeId, mapId);
#else
	out = CHUDUtils::LocalizeString("@mp_rp_gameplay", GetGameModeStringFromId(gameModeId), GetMapStringFromId(mapId));
#endif
}
#endif

/* static */
bool CGameBrowser::CreatePresenceString(CryFixedStringT<MAX_PRESENCE_STRING_SIZE> &out, SCryLobbyUserData *pData, uint32 numData)
{
	bool result = true;

	if(numData > 0)
	{
		CRY_ASSERT_MESSAGE(pData[CGame::eRPT_String].m_id == RICHPRESENCE_ID, "");

		// pData[0] indicates the type of rich presence we setting, i.e. frontend, lobby, in-game
		// additional pData's are parameters that can be passed into the rich presence string, only used for gameplay at the moment
		switch(pData[CGame::eRPT_String].m_int32)
		{
		case RICHPRESENCE_FRONTEND:
			LocalisePresenceString(out, "@mp_rp_frontend");
			break;

		case RICHPRESENCE_LOBBY:
			LocalisePresenceString(out, "@mp_rp_lobby");
			break;

		case RICHPRESENCE_GAMEPLAY:
#ifdef GAME_IS_CRYSIS2
			if(numData == 3)
			{
				const int gameModeId = pData[CGame::eRPT_Param1].m_int32;
				const int mapId = pData[CGame::eRPT_Param2].m_int32;
				LocaliseInGamePresenceString( out, "@mp_rp_gameplay", gameModeId, mapId );
			}
#if !defined(_RELEASE)
			else
			{
				CRY_ASSERT_MESSAGE(numData == 3, "Invalid data passed for gameplay rich presence state");
				result = false;
			}
#endif
#endif
			break;

		case RICHPRESENCE_SINGLEPLAYER:
			LocalisePresenceString(out, "@mp_rp_singleplayer");
			break;

		case RICHPRESENCE_IDLE:
			LocalisePresenceString(out, "@mp_rp_idle");
			break;

		default:
			CRY_ASSERT_MESSAGE(false, "[RichPresence] unknown rich presence type given");
			result = false;
			break;
		}
	}
	else
	{
		CryLog("[RichPresence] Failed to set richpresence because numData was 0 or there was no hud");
		result = false;
	}

	return result;
}

#endif


void CGameBrowser::ConfigurationCallback(ECryLobbyService service, SConfigurationParams *requestedParams, uint32 paramCount)
{
	uint32 a;
	for (a=0;a<paramCount;a++)
	{
		switch (requestedParams[a].m_fourCCID)
		{
		case CLCC_LAN_USER_NAME:
			{
				IPlayerProfileManager *pPlayerProfileManager = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager();
				uint32 userIndex = pPlayerProfileManager ? pPlayerProfileManager->GetExclusiveControllerDeviceIndex() : 0;

				IPlatformOS *pPlatformOS = gEnv->pSystem->GetPlatformOS();
				IPlatformOS::TUserName tUserName = "";
				if(pPlatformOS)
				{
					pPlatformOS->UserGetName(userIndex, tUserName);
				}
			
				// this will null terminate for us if necessary	
				cry_strncpy(s_profileName, tUserName.c_str(), CRYLOBBY_USER_NAME_LENGTH);

				requestedParams[a].m_pData = s_profileName;
			}
			break;

#if defined(PS3) || USE_CRYLOBBY_GAMESPY
		case CLCC_CRYLOBBY_PRESENCE_CONVERTER:
			{
				SCryLobbyPresenceConverter* pConverter = (SCryLobbyPresenceConverter*)requestedParams[a].m_pData;
				if (pConverter)
				{
					//-- Use the pConverter->m_numData data items in pConverter->m_pData to create a string in pConverter->m_pStringBuffer
					//-- Use the initial value of pConverter->sizeOfStringBuffer as a maximum string length allowed, but
					//-- update pConverter->sizeOfStringBuffer to the correct length when the string is filled in.
					//-- Set pConverter->sizeOfStringBuffer = 0 to invalidate bad data so it isn't sent to PSN.
					CryFixedStringT<MAX_PRESENCE_STRING_SIZE> strPresence;
					if(CreatePresenceString(strPresence, pConverter->m_pData, pConverter->m_numData))
					{
						CryLog("[RichPresence] Succeeded %s", strPresence.c_str());
						sprintf((char*)pConverter->m_pStringBuffer, "%s", strPresence.c_str());
						pConverter->m_sizeOfStringBuffer = strlen((char*)pConverter->m_pStringBuffer);









					}
					else
					{
						CryLog("[RichPresence] Failed to create rich presence string");
						pConverter->m_sizeOfStringBuffer = 0;
					}
				}
			}
			break;
#endif

#if USE_CRYLOBBY_GAMESPY
			// The following GameSpy data are always available.

		case CLCC_GAMESPY_TITLE:
			requestedParams[ a ].m_pData = ObfuscateGameSpyTitle();
			break;
		case CLCC_GAMESPY_SECRETKEY:
			requestedParams[ a ].m_pData = ObfuscateGameSpySecretKey();
			break;
		case CLCC_GAMESPY_GAMEVERSION:
			requestedParams[ a ].m_32 = GameLobbyData::GetVersion();
			break;
		case CLCC_GAMESPY_GAMEVERSIONSTRING:
			requestedParams[ a ].m_pData = ( void* )GAMESPY_GAMEVERSIONSTRING;
			break;
		case CLCC_GAMESPY_DISTRIBUTIONID:
			requestedParams[ a ].m_32 = GAMESPY_DISTRIBUTIONID;
			break;
		case CLCC_GAMESPY_PRODUCTID:
			requestedParams[ a ].m_32 = GAMESPY_PRODUCTID;
			break;
		case CLCC_GAMESPY_GAMEID:
			requestedParams[ a ].m_32 = GAMESPY_GAMEID;
			break;
		case CLCC_GAMESPY_NAMESPACEID:
			requestedParams[ a ].m_32 = GAMESPY_NAMESPACEID;
			break;
		case CLCC_GAMESPY_PARTNERID:
			requestedParams[ a ].m_32 = GAMESPY_PARTNERID;
			break;
		case CLCC_GAMESPY_REQUIREDNICK:
			requestedParams[ a ].m_pData = GAMESPY_REQUIREDNICK;
			break;
		case CLCC_GAMESPY_D2GCATALOGREGION:
			if ( g_pGameCVars )
			{
				requestedParams[a].m_pData = ( void* )g_pGameCVars->g_gamespy_catalog_region;
			}
			else
			{
				requestedParams[a].m_pData = NULL;
			}
			break;
		case CLCC_GAMESPY_D2GCATALOGVERSION:
			if ( g_pGameCVars )
			{
				requestedParams[a].m_32 = g_pGameCVars->g_gamespy_catalog_version->GetIVal();
			}
			else
			{
				requestedParams[a].m_32 = 0;
			}
			break;
		case CLCC_GAMESPY_D2GCATALOGTOKEN:
			if ( g_pGameCVars )
			{
				requestedParams[a].m_pData = ( void* )g_pGameCVars->g_gamespy_catalog_token;
			}
			else
			{
				requestedParams[a].m_pData = 0;
			}
			break;

			// CLCC_CRYLOBBY_LOGINGUISTATE is common to all online services for
			// which login is not handled by the OS. It will be requested if any
			// other requested data may require a login GUI to be displayed.

		case CLCC_CRYLOBBY_LOGINGUISTATE:
			//CRY_TODO( 30, 4, 2010, "Display a real GUI, don't use these hard coded values" );
			if ( g_pGameCVars )
			{
				ECryLobbyLoginGUIState requestedGUIState = eCLLGS_ExistingAccount;//eCLLGS_NotFinished;
				
				if(gEnv->IsDedicated() || g_pGameCVars->g_gamespy_loginUI==0)
				{
					requestedGUIState = eCLLGS_ExistingAccount;
				}
				else if(g_pGame->GetGameBrowser())
				{
				//	requestedGUIState = g_pGame->GetGameBrowser()->GetLoginGUIState();
				}

				requestedParams[ a ].m_32 = requestedGUIState;
			}
			else
			{
				requestedParams[ a ].m_32 = eCLLGS_Cancelled;
			}
			break;

		case CLCC_CRYLOBBY_LOGINGUICOUNT:
			if ( g_pGameCVars )
			{
				requestedParams[ a ].m_32 = 0;//g_pGameCVars->g_gamespy_loginCount;
			}
			else
			{
				requestedParams[ a ].m_32 = 0;
			}
			break;

			// The following GameSpy data may require a login GUI to be displayed.

		case CLCC_GAMESPY_EMAIL:
			if ( g_pGameCVars )
			{
				string email = "crysis2.";
				email.append(g_pGameCVars->g_gamespy_accountnumber->GetString());
				email.append(".paulm@crytek.com");
				requestedParams[a].m_pData = (void*)email.c_str();//(void*)g_pGameCVars->g_gamespy_email->GetString();
			}
			else
			{
				requestedParams[a].m_pData = NULL;
			}
			break;
		case CLCC_GAMESPY_UNIQUENICK:
			if ( g_pGameCVars )
			{
				string nick = "crysis2_";
				nick.append(g_pGameCVars->g_gamespy_accountnumber->GetString());
				nick.append("_paulm");
				requestedParams[a].m_pData = (void*)nick.c_str();//(void*)g_pGameCVars->g_gamespy_unique_nick->GetString();
			}
			else
			{
				requestedParams[a].m_pData = NULL;
			}
			break;
		case CLCC_GAMESPY_PASSWORD:
			if ( g_pGameCVars )
			{
				requestedParams[a].m_pData = "upple?9!";//(void*)g_pGameCVars->g_gamespy_password->GetString();
			}
			else
			{
				requestedParams[a].m_pData = NULL;
			}
			break;
		case CLCC_GAMESPY_CDKEY:
			if ( g_pGameCVars )
			{
				requestedParams[a].m_pData = (void*)g_pGameCVars->g_gamespy_cdkey->GetString();
			}
			else
			{
				requestedParams[a].m_pData = NULL;
			}
			break;

		case CLCC_GAMESPY_KEYNAME:

			// Session user data IDs map to GameSpy keys, which must be named.
			// CLCC_GAMESPY_KEYNAME will be requested for each session user data ID
			// greater than or equal to NUM_RESERVED_KEYS.
			//
			// IN:	requestedParams[ a ].m_8 holds a session user data ID.
			// OUT:	requestedParams[ a ].m_pData holds the GameSpy key name.
			//
			// NOTE: m_8 and m_pData are members of a union, so setting m_pData
			// will overwrite m_8.

			switch ( requestedParams[ a ].m_8 )
			{
			case LID_MATCHDATA_GAMEMODE:

				requestedParams[ a ].m_pData = GAMESPY_KEYNAME_MATCHDATA_GAMEMODE;
				break;

			case LID_MATCHDATA_MAP:

				requestedParams[ a ].m_pData = GAMESPY_KEYNAME_MATCHDATA_MAP;
				break;

			case LID_MATCHDATA_ACTIVE:

				requestedParams[ a ].m_pData = GAMESPY_KEYNAME_MATCHDATA_ACTIVE;
				break;

			case LID_MATCHDATA_VERSION:

				requestedParams[ a ].m_pData = GAMESPY_KEYNAME_MATCHDATA_VERSION;
				break;

			case LID_MATCHDATA_REQUIRED_DLCS:

				requestedParams[ a ].m_pData = GAMESPY_KEYNAME_MATCHDATA_REQUIRED_DLCS;
				break;

			case LID_MATCHDATA_PLAYLIST:

				requestedParams[ a ].m_pData = GAMESPY_KEYNAME_MATCHDATA_PLAYLIST;
				break;

			case LID_MATCHDATA_LANGUAGE:

				requestedParams[ a ].m_pData = GAMESPY_KEYNAME_MATCHDATA_LANGUAGE;
				break;

			case LID_MATCHDATA_OFFICIAL:

				requestedParams[ a ].m_pData = GAMESPY_KEYNAME_MATCHDATA_OFFICIAL;
				break;

			case LID_MATCHDATA_FAVOURITE_ID:

				requestedParams[ a ].m_pData = GAMESPY_KEYNAME_MATCHDATA_FAVOURITE_ID;
				break;

			default:

				CRY_ASSERT_MESSAGE( 0, "Session user data ID has no GameSpy key name" );
				requestedParams[ a ].m_pData = NULL;
				break;
			}

			break;

		case CLCC_GAMESPY_KEYSTRINGVALUE:

			{
				SCryLobbyUserDataStringParam*	pParam = static_cast< SCryLobbyUserDataStringParam* >( requestedParams[ a ].m_pData );

				switch ( pParam->id )
				{
				case LID_MATCHDATA_MAP:
					requestedParams[ a ].m_pData = const_cast< void* >( static_cast< const void* >( GameLobbyData::GetMapFromHash( pParam->value ) ) );
					break;
				case LID_MATCHDATA_GAMEMODE:
					requestedParams[ a ].m_pData = const_cast< void* >( static_cast< const void* >( GameLobbyData::GetGameRulesFromHash( pParam->value ) ) );
					break;
				default:
					requestedParams[ a ].m_pData = NULL;
					break;
				}
			}

			break;

		case CLCC_GAMESPY_P2PLEADERBOARDFMT:

			requestedParams[a].m_pData = ( void* )GAMESPY_P2P_LEADERBOARD_FORMAT;
			break;

		case CLCC_GAMESPY_DEDICATEDLEADERBOARDFMT:

			requestedParams[a].m_pData = ( void* )GAMESPY_DEDICATED_LEADERBOARD_FORMAT;
			break;

		case CLCC_GAMESPY_P2PSTATSTABLE:

			requestedParams[a].m_pData = ( void* )GAMESPY_P2P_STATS_TABLE;
			break;

		case CLCC_GAMESPY_DEDICATEDSTATSTABLE:

			requestedParams[a].m_pData = ( void* )GAMESPY_DEDICATED_STATS_TABLE;
			break;

		case CLCC_GAMESPY_TITLECDKEYSERVER:

			requestedParams[a].m_32 = 1;
			break;

#if defined( DEDICATED_SERVER )
		case CLCC_GAMESPY_DEDIVERSION:

			requestedParams[a].m_32 = GAMESPY_DEDI_VERSION;
			break;
#endif

#if USE_CRYLOBBY_GAMESPY_VOIP
		case CLCC_GAMESPY_VOICE_CODEC:
			requestedParams[a].m_pData = (void*)CCryGameSpyVoiceCodec::Initialise();
			break;
#endif // USE_CRYLOBBY_GAMESPY_VOIP
#endif // USE_CRYLOBBY_GAMESPY































































































































		case CLCC_MATCHMAKING_SESSION_PASSWORD_MAX_LENGTH:
			requestedParams[a].m_8 = MATCHMAKING_SESSION_PASSWORD_MAX_LENGTH;
			break;

		default:
			CRY_ASSERT_MESSAGE(0,"Unknown Configuration Parameter Requested!");
			break;
		}
	}
}

//-------------------------------------------------------------------------
// Returns the NAT type when set-up.
void CGameBrowser::GetNatTypeCallback(UCryLobbyEventData eventData, void *arg)
{
	SCryLobbyNatTypeData *natTypeData = eventData.pNatTypeData;
	if (natTypeData)
	{
		CGameBrowser* pGameBrowser = (CGameBrowser*) arg;
		pGameBrowser->SetNatType(natTypeData->m_curState);

		const char* natString = pGameBrowser->GetNatTypeString();
		CryLog( "natString=%s", natString);

#if !defined(_RELEASE)
		if(g_pGameCVars)
		{
			g_pGameCVars->net_nat_type->ForceSet(natString);
		}
#endif
		if(g_pGame)
		{
#ifdef USE_C2_FRONTEND
			if (CFlashFrontEnd *flashMenu = g_pGame->GetFlashMenu())
			{
				if (flashMenu->GetMPMenu())
					flashMenu->GetMPMenu()->UpdateNatType(natString);
			}
#endif //#ifdef USE_C2_FRONTEND
		}
	}
}

//-------------------------------------------------------------------------
const ENatType CGameBrowser::GetNatType() const
{
	return m_NatType;
}

//-------------------------------------------------------------------------
const char * CGameBrowser::GetNatTypeString() const
{
	switch(m_NatType)
	{
	case eNT_Open:
		return "@ui_mp_nattype_open";
	case eNT_Moderate:
		return "@ui_mp_nattype_moderate";
	case eNT_Strict:
		return "@ui_mp_nattype_strict";
	default:
		return "@ui_mp_nattype_unknown";
	};
	return "";
}

//-------------------------------------------------------------------------
// Register the data in CryLobby.
void CGameBrowser::InitialiseCallback(ECryLobbyService service, ECryLobbyError error, void* arg)
{
	assert( error == eCLE_Success );

	if (error == eCLE_Success)
	{
		SCryLobbyUserData userData[eLDI_Num];
		
		userData[eLDI_Gamemode].m_id = LID_MATCHDATA_GAMEMODE;
		userData[eLDI_Gamemode].m_type = eCLUDT_Int32;
		userData[eLDI_Gamemode].m_int32 = 0;

		userData[eLDI_Version].m_id = LID_MATCHDATA_VERSION;
		userData[eLDI_Version].m_type = eCLUDT_Int32;
		userData[eLDI_Version].m_int32 = 0;

		userData[eLDI_Playlist].m_id = LID_MATCHDATA_PLAYLIST;
		userData[eLDI_Playlist].m_type = eCLUDT_Int32;
		userData[eLDI_Playlist].m_int32 = 0;

		userData[eLDI_Variant].m_id = LID_MATCHDATA_VARIANT;
		userData[eLDI_Variant].m_type = eCLUDT_Int32;
		userData[eLDI_Variant].m_int32 = 0;

		userData[eLDI_RequiredDLCs].m_id = LID_MATCHDATA_REQUIRED_DLCS;
		userData[eLDI_RequiredDLCs].m_type = eCLUDT_Int32;
		userData[eLDI_RequiredDLCs].m_int32 = 0;

#if USE_CRYLOBBY_GAMESPY
		userData[eLDI_Official].m_id = LID_MATCHDATA_OFFICIAL;
		userData[eLDI_Official].m_type = eCLUDT_Int32;
		userData[eLDI_Official].m_int32 = 0;

		userData[eLDI_Region].m_id = LID_MATCHDATA_REGION;
		userData[eLDI_Region].m_type = eCLUDT_Int32;
		userData[eLDI_Region].m_int32 = 0;

		userData[eLDI_FavouriteID].m_id = LID_MATCHDATA_FAVOURITE_ID;
		userData[eLDI_FavouriteID].m_type = eCLUDT_Int32;
		userData[eLDI_FavouriteID].m_int32 = 0;
#endif

		userData[eLDI_Language].m_id = LID_MATCHDATA_LANGUAGE;
		userData[eLDI_Language].m_type = eCLUDT_Int32;
		userData[eLDI_Language].m_int32 = 0;

		userData[eLDI_Map].m_id = LID_MATCHDATA_MAP;
		userData[eLDI_Map].m_type = eCLUDT_Int32;
		userData[eLDI_Map].m_int32 = 0;

		userData[eLDI_Skill].m_id = LID_MATCHDATA_SKILL;
		userData[eLDI_Skill].m_type = eCLUDT_Int32;
		userData[eLDI_Skill].m_int32 = 0;

		userData[eLDI_Active].m_id = LID_MATCHDATA_ACTIVE;
		userData[eLDI_Active].m_type = eCLUDT_Int32;
		userData[eLDI_Active].m_int32 = 0;

		// we only want to register the user data with the service here, not set the service itself, we
		// already do that during the Init call
		ICryLobbyService* pCryLobbyService = gEnv->pNetwork->GetLobby()->GetLobbyService(service);
		assert(pCryLobbyService);
		if (pCryLobbyService)
		{
			ICryMatchMaking* pCryMatchMaking = pCryLobbyService->GetMatchMaking();
			assert(pCryMatchMaking);
			if (pCryMatchMaking)
			{
				pCryMatchMaking->SessionRegisterUserData(userData, eLDI_Num, 0, NULL, NULL);
			}
		}

#if INCLUDE_DEDICATED_LEADERBOARDS
		if (g_pGameCVars->g_useDedicatedLeaderboards)
		{
			ICryStats *pStats = gEnv->pNetwork->GetLobby()->GetLobbyService(service)->GetStats();
			if(pStats)
			{
				pStats->SetLeaderboardType(eCLLT_Dedicated);
			}
		}
#endif
	}
}

//-------------------------------------------------------------------------
// Process a search result.
void CGameBrowser::MatchmakingSessionSearchCallback(CryLobbyTaskID taskID, ECryLobbyError error, SCrySessionSearchResult* session, void* arg)
{
#ifdef USE_C2_FRONTEND
	CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
	CMPMenuHub *mpMenuHub = menu ? menu->GetMPMenu() : NULL;
	CGameBrowser* pGameBrowser = (CGameBrowser*) arg;

	if (error == eCLE_SuccessContinue || error == eCLE_Success)
	{
		if(session && mpMenuHub != NULL && GameLobbyData::IsCompatibleVersion(GameLobbyData::GetSearchResultsData(session, LID_MATCHDATA_VERSION)))
		{
			CUIServerList::SServerInfo si;
			si.m_hostName     = session->m_data.m_name;

			CRY_TODO(20, 5, 2010, "In the case where too many servers have been filtered out, start a new search query and merge the results with the existing ones");
			int requiredDLCs = GameLobbyData::GetSearchResultsData(session, LID_MATCHDATA_REQUIRED_DLCS);
			CryLog("Found server (%s), num data (%d): with DLC version %d", si.m_hostName.c_str(), session->m_data.m_numData, requiredDLCs);
			if (g_pGameCVars->g_ignoreDLCRequirements || CDLCManager::MeetsDLCRequirements(requiredDLCs, g_pGame->GetDLCManager()->GetSquadCommonDLCs()) || pGameBrowser->m_bFavouriteIdSearch)
			{
				si.m_numPlayers   = session->m_numFilledSlots;
				si.m_maxPlayers   = session->m_data.m_numPublicSlots;
				si.m_gameTypeName = GameLobbyData::GetGameRulesFromHash(GameLobbyData::GetSearchResultsData(session, LID_MATCHDATA_GAMEMODE));
				si.m_gameTypeDisplayName = si.m_gameTypeName.c_str();
				si.m_mapName = GameLobbyData::GetMapFromHash(GameLobbyData::GetSearchResultsData(session, LID_MATCHDATA_MAP));
				si.m_mapDisplayName = PathUtil::GetFileName(si.m_mapName.c_str()).c_str();
				si.m_friends = (session->m_numFriends>0);
				si.m_reqPassword = session->m_flags&eCSSRF_RequirePassword;

				uint32 variantId = GameLobbyData::GetSearchResultsData(session, LID_MATCHDATA_VARIANT);
				CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
				if (pPlaylistManager)
				{
					const SGameVariant* pVariant = pPlaylistManager->GetVariant(variantId);
					if (pVariant)
					{
						si.m_gameVariantName = pVariant->m_name.c_str();
						si.m_gameVariantDisplayName = CHUDUtils::LocalizeString(pVariant->m_localName.c_str());
					}
				}

				// Get more readable map name and game type
				if (ILocalizationManager* pLocMgr = gEnv->pSystem->GetLocalizationManager())
				{
					si.m_mapDisplayName = CHUDUtils::LocalizeString(g_pGame->GetMappedLevelName(si.m_mapDisplayName.c_str()));

					CryFixedStringT<64> tmpString;
					tmpString.Format("ui_rules_%s", si.m_gameTypeName.c_str());
					SLocalizedInfoGame outInfo;
					if (pLocMgr->GetLocalizedInfoByKey(tmpString.c_str(), outInfo))
					{
						wstring wcharstr = outInfo.swTranslatedText;
						CryStringUtils::WStrToUTF8(wcharstr, si.m_gameTypeDisplayName);
					}
				}

#if USE_CRYLOBBY_GAMESPY
				int numData = session->m_data.m_numData;
				for (int i=0; i<numData; ++i)
				{
					SCryLobbyUserData& pUserData = session->m_data.m_data[i];
					if (pUserData.m_id == LID_MATCHDATA_FAVOURITE_ID)
					{
						CRY_ASSERT(pUserData.m_type == eCLUDT_Int32);
						if (pUserData.m_type == eCLUDT_Int32)
						{
							si.m_sessionFavouriteKeyId = pUserData.m_int32;
						}
					}
					else if ((pUserData.m_id == LID_MATCHDATA_REGION))
					{
						CRY_ASSERT(pUserData.m_type == eCLUDT_Int32);
						if (pUserData.m_type == eCLUDT_Int32)
						{
							si.m_region = pUserData.m_int32;
						}
					}
					else if ((pUserData.m_id == LID_MATCHDATA_OFFICIAL))
					{
						CRY_ASSERT(pUserData.m_type == eCLUDT_Int32);
						if (pUserData.m_type == eCLUDT_Int32)
						{
							si.m_official = (pUserData.m_int32!=0);
						}
					}
				}
#endif

				// FIXME :
				//   Make server id unique in some other way....
				si.m_serverId     = (int)session->m_id.get(); // for lack of unique ids deref the pointer location of the server. This is the id that gets sent to flash...
				si.m_sessionId    = session->m_id;
				si.m_ping         = session->m_ping;

				if (pGameBrowser->m_bFavouriteIdSearch)
				{
#if IMPLEMENT_PC_BLADES
					if (si.m_sessionFavouriteKeyId != INVALID_SESSION_FAVOURITE_ID)
					{
						for (uint32 i = 0; i < pGameBrowser->m_currentSearchFavouriteIdIndex; ++i)
						{
							if (si.m_sessionFavouriteKeyId == pGameBrowser->m_searchFavouriteIds[i])
							{
								g_pGame->GetGameServerLists()->ServerFound(si, pGameBrowser->m_currentFavouriteIdSearchType, si.m_sessionFavouriteKeyId);

								// Invalidate any current search favourite ids, as it has been found
								pGameBrowser->m_searchFavouriteIds[i] = INVALID_SESSION_FAVOURITE_ID;
								break;
							}
						}
					}
#endif
				}
				else
				{
					mpMenuHub->AddServer(si);
				}
			}
		}
	}
	else if (error != eCLE_SuccessUnreachable)
	{
		CryLogAlways("CGameBrowser search for sessions error %d", (int)error);
		CGameLobby::ShowErrorDialog(error, NULL, NULL, NULL);
	}

	if ((error != eCLE_SuccessContinue) && (error != eCLE_SuccessUnreachable))
	{
		CryLogAlways("CCGameBrowser::MatchmakingSessionSearchCallback DONE");

		pGameBrowser->FinishedSearch(true, !pGameBrowser->m_bFavouriteIdSearch); // FavouriteId might start another search after this one has finished
	}
#endif //#ifdef USE_C2_FRONTEND
}

//-------------------------------------------------------------------------
// static
void CGameBrowser::InitLobbyServiceType()
{
	// Setup the default lobby service. 
	ECryLobbyService defaultLobbyService = eCLS_LAN;

#if LEADERBOARD_PLATFORM
	if (gEnv->IsDedicated())
	{
		if ( g_pGameCVars && (g_pGameCVars->g_useOnlineLobbyService))
		{
			defaultLobbyService = eCLS_Online;
		}
	}
	else
	{
		defaultLobbyService = eCLS_Online;									// We support leaderboards so must use online by default to ensure stats are posted correctly
	}
#endif

#ifndef _RELEASE
	if (g_pGameCVars && g_pGameCVars->net_initLobbyServiceToLan)
	{
		defaultLobbyService = eCLS_LAN;
	}
#endif

	CGameLobby::SetLobbyService(defaultLobbyService);
}

#if IMPLEMENT_PC_BLADES
void CGameBrowser::StartFavouriteIdSearch( const CGameServerLists::EGameServerLists serverList, uint32 *pFavouriteIds, uint32 numFavouriteIds )
{
	CRY_ASSERT(numFavouriteIds <= CGameServerLists::k_maxServersStoredInList);
	numFavouriteIds = MIN(numFavouriteIds, CGameServerLists::k_maxServersStoredInList);
	if (numFavouriteIds <= CGameServerLists::k_maxServersStoredInList)
	{
		memset(m_searchFavouriteIds, INVALID_SESSION_FAVOURITE_ID, sizeof(m_searchFavouriteIds));

		for (uint32 i=0; i<numFavouriteIds; ++i)
		{
			m_searchFavouriteIds[i] = pFavouriteIds[i];
		}

		m_currentSearchFavouriteIdIndex = 0;
		m_numSearchFavouriteIds = numFavouriteIds;

		if (CanStartSearch())
		{
			CryLog("[UI] Delayed Searching for favId sessions");
			m_delayedSearchType = eDST_FavouriteId;

#ifdef USE_C2_FRONTEND
			if (CMPMenuHub *pMPMenu = CMPMenuHub::GetMPMenuHub())
			{
				pMPMenu->StartSearching();
			}
#endif //#ifdef USE_C2_FRONTEND
		}
		else
		{
			if (DoFavouriteIdSearch())
			{
				m_currentFavouriteIdSearchType = serverList;

#ifdef USE_C2_FRONTEND
				if (CMPMenuHub *pMPMenu = CMPMenuHub::GetMPMenuHub())
				{
					pMPMenu->StartSearching();
				}
#endif //#ifdef USE_C2_FRONTEND
			}
		}
	}
}
#endif

//-------------------------------------------------------------------------
bool CGameBrowser::DoFavouriteIdSearch()
{
	CryLog("[UI] DoFavouriteIdSearch");

	bool bResult = false;

#if USE_CRYLOBBY_GAMESPY
	const int k_maxNumData = START_SEARCHING_FOR_SERVERS_NUM_DATA + MAX_NUM_PER_FRIEND_ID_SEARCH;
	SCrySessionSearchParam param;
	SCrySessionSearchData data[k_maxNumData];

	param.m_type = REQUIRED_SESSIONS_QUERY;
	param.m_data = data;
	param.m_numFreeSlots = 0;
	param.m_maxNumReturn = MAX_NUM_PER_FRIEND_ID_SEARCH;
	param.m_ranked = false;

	int curData = 0;

	data[curData].m_operator = eCSSO_Equal;
	data[curData].m_data.m_id = LID_MATCHDATA_VERSION;
	data[curData].m_data.m_type = eCLUDT_Int32;
	data[curData].m_data.m_int32 = GameLobbyData::GetVersion();
	curData++;

	uint32 numAdded = 0;
#if IMPLEMENT_PC_BLADES
	for (; m_currentSearchFavouriteIdIndex<m_numSearchFavouriteIds && numAdded<MAX_NUM_PER_FRIEND_ID_SEARCH; ++m_currentSearchFavouriteIdIndex)
	{
		if (m_searchFavouriteIds[m_currentSearchFavouriteIdIndex] != INVALID_SESSION_FAVOURITE_ID)
		{
			CRY_ASSERT_MESSAGE( curData < k_maxNumData, "Session search data buffer overrun" );
			data[curData].m_operator = eCSSO_Equal;
			data[curData].m_data.m_id = LID_MATCHDATA_FAVOURITE_ID;
			data[curData].m_data.m_type = eCLUDT_Int32;
			data[curData].m_data.m_int32 = m_searchFavouriteIds[m_currentSearchFavouriteIdIndex];
			curData++;

			++numAdded;

			CryLog("[UI] Do favourite ID search for %d, on index %d of %d", m_searchFavouriteIds[m_currentSearchFavouriteIdIndex], m_currentSearchFavouriteIdIndex, m_numSearchFavouriteIds);
		}
	}
#endif

	param.m_numData = curData;

	if (numAdded > 0)
	{
		ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
		if (pLobby)
		{
			if (pLobby->GetLobbyService())
			{
				CRY_ASSERT_MESSAGE(m_searchingTask==CryLobbyInvalidTaskID,"CGameBrowser Trying to search for sessions when you think you are already searching.");

				ECryLobbyError result = StartSearchingForServers(&param, CGameBrowser::MatchmakingSessionSearchCallback, this, true);

				CryLog("CCGameBrowser::DoFavouriteIdSearch result=%u, taskId=%u", result, m_searchingTask);
				if (result == eCLE_Success)
				{
					bResult = true;
				}
				else
				{
					m_searchingTask = CryLobbyInvalidTaskID;
				}
			}
		}
	}
#endif

	return bResult;
}

#if USE_CRYLOBBY_GAMESPY

void* CGameBrowser::ObfuscateGameSpyTitle()
{
	//#define GAMESPY_TITLE		( "capricorn" )
	static char buffer[10] = {0};
	strcpy(buffer, "ca");
	strcat(buffer, "p");
	strcat(buffer, "ric");
	strcat(buffer, "or");
	strcat(buffer, "n");
	return (void*)buffer;
}

void* CGameBrowser::ObfuscateGameSpySecretKey()
{
	//#define GAMESPY_SECRETKEY	( "8TTq4M" )
	static char buffer[7] = {0};
	strcpy(buffer, "8T");
	strcat(buffer, "T");
	strcat(buffer, "q");
	strcat(buffer, "4");
	strcat(buffer, "M");
	return (void*)buffer;
}

#endif // USE_CRYLOBBY_GAMESPY
