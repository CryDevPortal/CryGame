/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2011.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Class for interface between C++ and Lua for MatchMaking
-------------------------------------------------------------------------
History:
- 01:08:2011 : Created By Andrew Blackwell

*************************************************************************/

#include "StdAfx.h"
//////////////////////////////////////////////////////////////////////////
// This Include
#include "Game.h"
#include "MatchMakingHandler.h"

#include "GameLobby.h"
#include "GameLobbyManager.h"
#include "MatchMakingTelemetry.h"
#include "MatchMakingEvents.h"
#include "GameBrowser.h"
#include "ScriptBind_MatchMaking.h"

#include "GameCVars.h"


int CMatchMakingHandler::s_currentMMSearchID = 0;

//------------------------------------------------------------------------
//Constructor
CMatchMakingHandler::CMatchMakingHandler()
:	m_pScript( gEnv->pScriptSystem, true ),
	m_sessionIdIndex( 0 )
{
	//m_sessionIdMap.reserve( 20 );
}


//------------------------------------------------------------------------
//Destructor
CMatchMakingHandler::~CMatchMakingHandler()
{

}

//------------------------------------------------------------------------
bool CMatchMakingHandler::LoadScript()
{
	bool retval = false;
	if( gEnv->pScriptSystem )
	{
		// Load matchmaking script.
		retval = gEnv->pScriptSystem->ExecuteFile( "scripts/Matchmaking/matchmaking.lua", true, true );

		if( retval )
		{
			retval = gEnv->pScriptSystem->GetGlobalValue( "MatchMaking", m_pScript );
		}
		else
			CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Unable to load Scripts/Matchmaking/MatchMaking.lua!");
	}

	return retval;
}

//------------------------------------------------------------------------
void CMatchMakingHandler::OnInit( CGameLobbyManager* pLobbyManager )
{
	g_pGame->GetMatchMakingScriptBind()->AttachTo( this, pLobbyManager );

	m_waitingTask = eMMHT_None;
	m_bIsMerging = false;
	m_nSessionParams = 0;
	m_waitingTaskSuccess = false;

	HSCRIPTFUNCTION scriptFunction;
	if( m_pScript->GetValue( "OnInit", scriptFunction ) )  
	{
		Script::Call( gEnv->pScriptSystem, scriptFunction, m_pScript );
		gEnv->pScriptSystem->ReleaseFunc( scriptFunction );
	}
}

//------------------------------------------------------------------------
void CMatchMakingHandler::OnEnterMatchMaking()
{
	HSCRIPTFUNCTION scriptFunction;
	if( m_pScript->GetValue( "OnEnterMatchMaking", scriptFunction ) )  
	{
		Script::Call( gEnv->pScriptSystem, scriptFunction, m_pScript );
		gEnv->pScriptSystem->ReleaseFunc( scriptFunction );
	}

	m_startTime = gEnv->pTimer->GetFrameStartTime().GetSeconds();
}

//------------------------------------------------------------------------
void CMatchMakingHandler::OnLeaveMatchMaking()
{
	HSCRIPTFUNCTION scriptFunction;
	if( m_pScript->GetValue( "OnLeaveMatchMaking", scriptFunction ) )  
	{
		Script::Call( gEnv->pScriptSystem, scriptFunction, m_pScript );
		gEnv->pScriptSystem->ReleaseFunc( scriptFunction );
	}
}

//------------------------------------------------------------------------
void CMatchMakingHandler::Search( int freeSlots, int maxResults, SCrySessionSearchData* searchParameters, int numSearchParameters )
{
	//might still want equivalents of these
	//m_findGameTimeout = GetFindGameTimeout();
	//m_findGameResults.clear();

	SCrySessionSearchParam param;

	param.m_type = FIND_GAME_SESSION_QUERY;

	param.m_data = searchParameters;
	param.m_numFreeSlots = freeSlots;
	CRY_ASSERT(param.m_numFreeSlots > 0);
	param.m_maxNumReturn = maxResults;
	param.m_ranked = false;

	int curData = 0;











	CRY_ASSERT_MESSAGE( numSearchParameters < FIND_GAMES_SEARCH_NUM_DATA, "Session search data buffer overrun" );
	searchParameters[ numSearchParameters ].m_operator = eCSSO_Equal;
	searchParameters[ numSearchParameters ].m_data.m_id = LID_MATCHDATA_VERSION;
	searchParameters[ numSearchParameters ].m_data.m_type = eCLUDT_Int32;
	searchParameters[ numSearchParameters ].m_data.m_int32 = GameLobbyData::GetVersion();
	numSearchParameters++;


// TODO: Need to decide how to handle regions in searches on all platforms (currently different IDs/types used)
/*
#if USE_CRYLOBBY_GAMESPY

	uint32	region = eSR_All;	// Game side support for region filtering needs to change this.

	if ( region != eSR_All )
	{
		CRY_ASSERT_MESSAGE( curData < FIND_GAMES_SEARCH_NUM_DATA, "Session search data buffer overrun" );
		data[curData].m_operator = eCSSO_BitwiseAndNotEqualZero;
		data[curData].m_data.m_id = LID_MATCHDATA_REGION;
		data[curData].m_data.m_type = eCLUDT_Int32;
		data[curData].m_data.m_int32 = region;
		curData++;
	}

	int32		favouriteID = 0;	// Game side support for favourite servers needs to change this.

	if ( favouriteID )
	{
		CRY_ASSERT_MESSAGE( curData < FIND_GAMES_SEARCH_NUM_DATA, "Session search data buffer overrun" );
		data[curData].m_operator = eCSSO_Equal;
		data[curData].m_data.m_id = LID_MATCHDATA_FAVOURITE_ID;
		data[curData].m_data.m_type = eCLUDT_Int32;
		data[curData].m_data.m_int32 = favouriteID;
		curData++;
	}
#endif

	int userRegion = g_pGame->GetUserRegion();
	if (userRegion)
	{
		const CTimeValue &now = gEnv->pTimer->GetFrameStartTime();
		const float timeSinceStarted = (now -	m_timeSearchStarted).GetSeconds();
		if (timeSinceStarted < gl_findGameExpandSearchTime)
		{
			CryLog("CGameLobby::FindGamesSearch() using local region");
			CRY_ASSERT_MESSAGE( curData < FIND_GAMES_SEARCH_NUM_DATA, "Session search data buffer overrun" );
			data[curData].m_operator = eCSSO_Equal;
			data[curData].m_data.m_id = LID_MATCHDATA_COUNTRY;
			data[curData].m_data.m_type = eCLUDT_Int32;
			data[curData].m_data.m_int32 = g_pGame->GetUserRegion();
			curData++;

			// Need to use a different query since we've got more filters
			param.m_type = FIND_GAME_SESSION_QUERY_WC;
		}
		else
		{
			CryLog("CGameLobby::FindGamesSearch() using all regions");
		}
	}
	else
	{
		CryLog("CGameLobby::FindGamesSearch() using all regions - no user region detected");
	}
*/
	param.m_numData = numSearchParameters;

	++s_currentMMSearchID;
#if defined(TRACK_MATCHMAKING)
	if( CMatchmakingTelemetry* pMMTel = g_pGame->GetMatchMakingTelemetry() )
	{
		pMMTel->AddEvent( SMMStartSearchEvent( param, s_currentMMSearchID ) );
	}
#endif

	ECryLobbyError result = g_pGame->GetGameBrowser()->StartSearchingForServers(&param, CMatchMakingHandler::SearchCallback, this, false);
	if (result == eCLE_Success)
	{
		CryLog("MatchMakingHandler::Search() search successfully started, ");//setting s_bShouldBeSearching to FALSE to prevent another one starting");
	}
	else
	{
		CryLog("MatchMakingHandler::Search() search failed to start (error=%i)", result);// setting s_bShouldBeSearching to TRUE so we start another one when the timeout occurs", result);
	}
	
}

//------------------------------------------------------------------------
void CMatchMakingHandler::CancelSearch()
{
	if( CGameBrowser *pGameBrowser = g_pGame->GetGameBrowser() )
	{
		pGameBrowser->CancelSearching();
	}
}

//------------------------------------------------------------------------
void CMatchMakingHandler::SearchCallback( CryLobbyTaskID taskID, ECryLobbyError error, SCrySessionSearchResult* session, void* arg )
{
	CMatchMakingHandler* pThis = (CMatchMakingHandler*)(arg);
	
	if( session )
	{
		pThis->OnSearchResult( session );
	}

	if( (error != eCLE_SuccessContinue) && (error != eCLE_SuccessUnreachable) )
	{
		//search done, inform the Lua next update
		pThis->AddWaitingTask( eMMHT_EndSearch, true );
	}
}

//------------------------------------------------------------------------
void CMatchMakingHandler::OnSearchResult( SCrySessionSearchResult* pSession )
{
	//session will expire at the end of the callback
	//so copy it into a results structure (we need lots of details to pass to the matchmaking part
	//store it in a map (indexed via sessionID?)
	//pass the index to the lua

	CGameLobby* pLobby = g_pGame->GetGameLobby();

	const CGameLobby::EActiveStatus activeStatus = (CGameLobby::EActiveStatus) GameLobbyData::GetSearchResultsData( pSession, LID_MATCHDATA_ACTIVE );
	bool bIsBadServer = pLobby->IsBadServer( pSession->m_id );
	const int skillRank = pLobby->CalculateAverageSkill();
	const int sessionSkillRank = GameLobbyData::GetSearchResultsData( pSession, LID_MATCHDATA_SKILL );
	const int sessionLanguageId = GameLobbyData::GetSearchResultsData( pSession, LID_MATCHDATA_LANGUAGE );

	float sessionScore = LegacyC2MatchMakingScore( pSession, pLobby, false );

	int32 region = 0;
#if USE_CRYLOBBY_GAMESPY
	region = GameLobbyData::GetSearchResultsData( pSession, LID_MATCHDATA_REGION );
#endif //USE_CRYLOBBY_GAMESPY

#if defined(TRACK_MATCHMAKING)
	if( CMatchmakingTelemetry* pMMTel = g_pGame->GetMatchMakingTelemetry() )
	{
		pMMTel->AddEvent( SMMFoundSessionEvent( pSession, activeStatus, skillRank - sessionSkillRank, region, sessionLanguageId, bIsBadServer, sessionScore ) );
	}
#endif //defined(TRACK_MATCHMAKING)

	HSCRIPTFUNCTION scriptFunction;

	if( m_pScript->GetValue( "OnSearchResult", scriptFunction ) )  
	{
		//Make a table to hold the search result
		SmartScriptTable result( gEnv->pScriptSystem );

		SessionDetails newSession;
		newSession.m_id = pSession->m_id;
		cry_strncpy( newSession.m_name, pSession->m_data.m_name, sizeof( newSession.m_name ) );

		//capture the session ID in a map
		std::pair< TSessionIdMap::iterator, bool > insertResult = m_sessionIdMap.insert( std::make_pair( m_sessionIdIndex++, newSession ) );

		if( insertResult.second )
		{
			result->SetValue( "SessionId", insertResult.first->first );
			result->SetValue( "SearchId", s_currentMMSearchID );

			float timeSinceStarted = gEnv->pTimer->GetFrameStartTime().GetSeconds() - m_startTime;
			result->SetValue( "TimeFound", timeSinceStarted );
				
			//make a sub table to hold all the known session parameters	
			SmartScriptTable parameters( gEnv->pScriptSystem );
			result->SetValue( "Parameters", parameters );

			parameters->SetValue( "ActiveStatus", activeStatus );
			parameters->SetValue( "ServerAvSkill", sessionSkillRank );
			parameters->SetValue( "Region", region );
			parameters->SetValue( "Language", sessionLanguageId );
			parameters->SetValue( "BadServer", bIsBadServer );
			parameters->SetValue( "Ping", pSession->m_ping );
			parameters->SetValue( "FilledSlots", pSession->m_numFilledSlots );

			Script::Call( gEnv->pScriptSystem, scriptFunction, m_pScript, result );
			gEnv->pScriptSystem->ReleaseFunc( scriptFunction );
		}
	}
}


//------------------------------------------------------------------------
CrySessionID CMatchMakingHandler::GetSessionId( int sessionIndex )
{
	TSessionIdMap::const_iterator it = m_sessionIdMap.find( sessionIndex );
	
	if( it == m_sessionIdMap.end() )
	{
		return CrySessionInvalidID;
	}
	else
	{ 
		return it->second.m_id;
	}
}

//------------------------------------------------------------------------
void CMatchMakingHandler::Join( int sessionIndex )
{
	bool success = false;

	if( CGameLobby* pLobby = g_pGame->GetGameLobby() )
	{
		TSessionIdMap::const_iterator itSession = m_sessionIdMap.find( sessionIndex );

		if( itSession != m_sessionIdMap.end() )
		{
#if defined (TRACK_MATCHMAKING)
			if( CGameLobbyManager* pLobbyMan = g_pGame->GetGameLobbyManager() )
			{
				if( CMatchmakingTelemetry* pMMTel = g_pGame->GetMatchMakingTelemetry() )
				{
					pMMTel->AddEvent( SMMChosenSessionEvent( itSession->second.m_name, itSession->second.m_id, "Lua Matchmaking", false, s_currentMMSearchID, pLobbyMan->IsPrimarySession( pLobby ) ) );
				}
			}
#endif

			m_bIsMerging = false;

			success = pLobby->JoinServer( itSession->second.m_id, itSession->second.m_name, CryMatchMakingInvalidConnectionUID, false); 
		}
		else
		{
			MMLog( "Unable to find session ID in session ID map", true );
		}
	}

	if( success == false )
	{
		AddWaitingTask( eMMHT_EndJoin, false );
	}

}

//------------------------------------------------------------------------
void CMatchMakingHandler::Merge( int sessionIndex )
{
	if( CGameLobby* pLobby = g_pGame->GetGameLobby() )
	{
		TSessionIdMap::const_iterator itSession = m_sessionIdMap.find( sessionIndex );

		if( itSession != m_sessionIdMap.end() )
		{
#if defined (TRACK_MATCHMAKING)
			if( CGameLobbyManager* pLobbyMan = g_pGame->GetGameLobbyManager() )
			{
				if( CMatchmakingTelemetry* pMMTel = g_pGame->GetMatchMakingTelemetry() )
				{
					pMMTel->AddEvent( SMMChosenSessionEvent( itSession->second.m_name, itSession->second.m_id, "Lua Matchmaking", false, s_currentMMSearchID, pLobbyMan->IsPrimarySession( pLobby ) ) );
				}
			}
#endif

			m_bIsMerging = true;

			bool success = pLobby->MergeToServer( itSession->second.m_id );

			if( success == false )
			{
				AddWaitingTask( eMMHT_EndMerge, false );
			}
		}
	}
}

//------------------------------------------------------------------------
void CMatchMakingHandler::GameLobbyJoinFinished( ECryLobbyError error )
{
	bool success = (error == eCLE_Success);

	if( m_bIsMerging )
	{
		AddWaitingTask( eMMHT_EndMerge, success );
	}
	else
	{
		AddWaitingTask( eMMHT_EndJoin, success );
	}

}

//------------------------------------------------------------------------
void CMatchMakingHandler::Update()
{
	if( m_waitingTask != eMMHT_None )
	{
		//should be replaced with a pop
		EMatchMakingHandlerTask temp = m_waitingTask;
		m_waitingTask = eMMHT_None;

		switch( temp )
		{
		case eMMHT_EndSearch:
			{
				HSCRIPTFUNCTION scriptFunction;
				if( m_pScript->GetValue( "OnSearchFinished", scriptFunction ) )  
				{
					Script::Call( gEnv->pScriptSystem, scriptFunction, m_pScript );
					gEnv->pScriptSystem->ReleaseFunc( scriptFunction );
				}
				break;
			}
		case eMMHT_EndJoin:
			{
				HSCRIPTFUNCTION scriptFunction;
				CryLog( "MMHandler: OnJoinedFinished" );
				if( m_pScript->GetValue( "OnJoinFinished", scriptFunction ) )  
				{
					Script::Call( gEnv->pScriptSystem, scriptFunction, m_pScript, m_waitingTaskSuccess );
					gEnv->pScriptSystem->ReleaseFunc( scriptFunction );
				}
				break;
			}
		case eMMHT_EndMerge:
			{
				HSCRIPTFUNCTION scriptFunction;
				if( m_pScript->GetValue( "OnMergeFinished", scriptFunction ) )  
				{
					Script::Call( gEnv->pScriptSystem, scriptFunction, m_pScript, m_waitingTaskSuccess );
					gEnv->pScriptSystem->ReleaseFunc( scriptFunction );
				}
				break;
			}
		default:
			CRY_ASSERT_MESSAGE( false, "MMHandler: Invalid task ID in waiting tasks" );
		}
	
	}
}

//------------------------------------------------------------------------
void CMatchMakingHandler::OnHostMigrationFinished( bool success, bool isNewHost )
{
	HSCRIPTFUNCTION scriptFunction;
	if( m_pScript->GetValue( "OnHostMigrationFinished", scriptFunction ) )  
	{
		Script::Call( gEnv->pScriptSystem, scriptFunction, m_pScript, success, isNewHost );
		gEnv->pScriptSystem->ReleaseFunc( scriptFunction );
	}
}

//------------------------------------------------------------------------
float SafeGetCvarHelper( const char* cvarName, bool allowNegative=true )
{
	if( ICVar* pCvar = gEnv->pConsole->GetCVar( cvarName ) )
	{
		float val = pCvar->GetFVal();

		if( !allowNegative && val < 0.f )
		{
			return 1.0f;
		}
		else
		{
			return val;
		}
	}
	else
	{
		return 1.0f;
	}
}

//------------------------------------------------------------------------
float CMatchMakingHandler::LegacyC2MatchMakingScore( SCrySessionSearchResult* session, CGameLobby *lobby, bool includeRand )
{
	//Creates sub metrics (between 0-1 (1 being best))

	const CGameLobby::EActiveStatus activeStatus = (CGameLobby::EActiveStatus) GameLobbyData::GetSearchResultsData( session, LID_MATCHDATA_ACTIVE );

	const float pingScale = SafeGetCvarHelper( "gl_findGamePingScale", false );
	const float idealPlayerCount = SafeGetCvarHelper( "gl_findGameIdealPlayerCount", false);

	CryLog( "MMLua: Cvar pingscale = %.3f, idealPlayerCount = %.3f", pingScale, idealPlayerCount );

	float pingSubMetric = 1.0f - clamp((session->m_ping / pingScale), 0.0f, 1.0f);					//300ms or above gives you a 0 rating
	float playerSubMetric = clamp((float)session->m_numFilledSlots / idealPlayerCount, 0.0f, 1.0f);					//more players the better
	float lobbySubMetric = (activeStatus != CGameLobby::eAS_Lobby) ? 0.0f : 1.f;		// Prefer games that haven't started yet

	float skillSubMetric = 0.f;
	const int skillRank = lobby->CalculateAverageSkill();
	const int sessionSkillRank = GameLobbyData::GetSearchResultsData(session, LID_MATCHDATA_SKILL);
	if (skillRank)
	{										
		float diff = (float) abs(skillRank - sessionSkillRank);
		float fracDiff = diff / (float) skillRank;
		skillSubMetric = 1.f - MIN(fracDiff, 1.f);
		skillSubMetric = (skillSubMetric * skillSubMetric);
	}

	int32 languageId = lobby->GetCurrentLanguageId();

	float languageSubMetric = 0.f;
	if (languageId == GameLobbyData::GetSearchResultsData(session, LID_MATCHDATA_LANGUAGE))
	{
		languageSubMetric = 1.f;
	}

	float randomSubMetric = ((float) (g_pGame->GetRandomNumber() & 0xffff)) / ((float) 0xffff);

	pingSubMetric *= SafeGetCvarHelper( "gl_findGamePingMultiplyer" );
	playerSubMetric *= SafeGetCvarHelper( "gl_findGamePlayerMultiplyer" );
	lobbySubMetric *= SafeGetCvarHelper( "gl_findGameLobbyMultiplyer" );
	skillSubMetric *= SafeGetCvarHelper( "gl_findGameSkillMultiplyer" );
	languageSubMetric *= SafeGetCvarHelper( "gl_findGameLanguageMultiplyer" );
	randomSubMetric *= SafeGetCvarHelper( "gl_findGameRandomMultiplyer" );

	CryLog("MMLua: Found %s metrics: (ping:%.2f, player:%.2f, lobby:%.2f, skill:%.2f, language:%.2f, random:%.2f)", session->m_data.m_name, pingSubMetric, playerSubMetric, lobbySubMetric, skillSubMetric, languageSubMetric, randomSubMetric);

	float score = pingSubMetric + playerSubMetric + lobbySubMetric + skillSubMetric + languageSubMetric;
	
	if( includeRand )
	{
		score += randomSubMetric;
	}

	CryLog("MMLua: Final Score %.2f", score );

	return score;
}

//------------------------------------------------------------------------
bool CMatchMakingHandler::AllowedToCreateGame()
{



#ifdef GAME_IS_CRYSIS2
	return (gEnv->IsDedicated()) || (g_pGameCVars->g_EnableDevMenuOptions != 0);	// only allow create game on PC if dev options are enabled
#else
	return true;
#endif

}

bool CMatchMakingHandler::AddWaitingTask( EMatchMakingHandlerTask taskID, bool taskSuccess )
{
	bool ok = ( m_waitingTask == eMMHT_None );
	CRY_ASSERT( ok );
	if( ok )
	{
		m_waitingTask = taskID;
		m_waitingTaskSuccess = taskSuccess;
	}

	return ok;
}

void CMatchMakingHandler::MMLog( const char* message, bool isError )
{
	if( isError )
	{
		CryLog( "MMHandlerError: %s", message );
	}
	else
	{
		CryLog( "MMHandlerLog: %s", message );
	}

#ifdef GAME_IS_CRYSIS2
	if( CMatchmakingTelemetry* pMMTel = g_pGame->GetMatchMakingTelemetry() )
	{
		pMMTel->AddEvent( SMMGenericLogEvent( message, isError ) );
	}
#endif

}

void CMatchMakingHandler::ClearSessionParameters()
{
	memset(&m_sessionParams, 0, sizeof(m_sessionParams));
	m_nSessionParams = 0;
}

void CMatchMakingHandler::NewSessionParameter( ELOBBYIDS paramID, ScriptAnyValue valueVal )
{
	if( m_nSessionParams < eLDI_Num )
	{
		m_sessionParams[ m_nSessionParams ].m_id = paramID;

		switch( valueVal.type )
		{
		case ANY_TNUMBER:
			m_sessionParams[ m_nSessionParams ].m_type = eCLUDT_Int32;
			m_sessionParams[ m_nSessionParams ].m_int32 = (int32)valueVal.number;
			break;

		case ANY_TBOOLEAN:
			m_sessionParams[ m_nSessionParams ].m_type = eCLUDT_Int32;
			m_sessionParams[ m_nSessionParams ].m_int32 = (int32)valueVal.b;
			break;

		case ANY_THANDLE:
			m_sessionParams[ m_nSessionParams ].m_type = eCLUDT_Int32;
			m_sessionParams[ m_nSessionParams ].m_int32 = (int32)valueVal.ptr;
			break;

		default:
			MMLog( "MMLua: Unsupported type in session data", true );
		}

		CryLog( "MMLua: Create Session Parameter, id %d, value %d", m_sessionParams[ m_nSessionParams ].m_id, m_sessionParams[ m_nSessionParams ].m_int32 );
		m_nSessionParams++;
	}
	else
	{
		MMLog( "MMLua: Too many session search parameters set from Lua", true );
	}
}

void CMatchMakingHandler::AdjustCreateSessionData( SCrySessionData* pData, uint32 maxDataItems )
{
	//for every parameter in our data
	for( uint32 iParam = 0; iParam < m_nSessionParams; iParam++ )
	{
		int paramID = m_sessionParams[ iParam ].m_id;
		bool found = false;
		//check if it is in the source data
		for( uint32 iData = 0; iData < pData->m_numData; iData++ )
		{
			if( pData->m_data[ iData ].m_id == paramID )
			{
				//if it is, change it
				CRY_ASSERT( pData->m_data[ iData ].m_type == m_sessionParams[ iParam ].m_type );
				pData->m_data[ iData ].m_int32 = m_sessionParams[ iParam ].m_int32;
				found = true;
				break;
			}
		}

		if( ! found )
		{
			//if not, check we have space for another parameter
			if( pData->m_numData < maxDataItems )
			{
				//and add it to the source data
				pData->m_data[ pData->m_numData ] = m_sessionParams[ iParam ];
				pData->m_numData++;
			}		
		}		
	}
}

