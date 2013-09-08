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

//////////////////////////////////////////////////////////////////////////
//Header Guard
#ifndef __MATCHMAKINGHANDLER_H__
#define __MATCHMAKINGHANDLER_H__

//////////////////////////////////////////////////////////////////////////
// Important Includes
//For eLDI_Max
#include "GameLobbyData.h"


#if USE_CRYLOBBY_GAMESPY
#define FIND_GAMES_SEARCH_NUM_DATA	14
#else
#define FIND_GAMES_SEARCH_NUM_DATA	12
#endif

class CMatchMakingHandler
{
public:
	CMatchMakingHandler();
	~CMatchMakingHandler();

	bool	LoadScript();
	IScriptTable*	GetScriptTable() { return m_pScript; }


	//////////////////////////////////////////////////////////////////////////
	// These functions call directly to Lua
	void	OnInit( CGameLobbyManager* pLobbyManager );
	void	OnEnterMatchMaking();
	void	OnLeaveMatchMaking();

	void	Update();

	void	OnHostMigrationFinished( bool sucess, bool isNewHost );
	void	OnSearchResult( SCrySessionSearchResult* pSession );

	//////////////////////////////////////////////////////////////////////////
	// These functions are useful things for matchmaking
	static float	LegacyC2MatchMakingScore( SCrySessionSearchResult* session, CGameLobby *lobby, bool includeRand );
	static bool		AllowedToCreateGame();
	bool					IsJoiningSession();
	CrySessionID	GetSessionId( int sessionIndex );
	void					MMLog( const char* message, bool isError );

	//////////////////////////////////////////////////////////////////////////
	// These functions handle requests which have come from matchmaking (and usually require a response at a later time)
	void Search( int freeSlotsRequired, int maxResults, SCrySessionSearchData* searchParameters, int numSearchParameters );
	void Join( int sessionIndex );
	void Merge( int sessionIndex );
	void CancelSearch();

	void ClearSessionParameters();
	void NewSessionParameter( ELOBBYIDS paramID, ScriptAnyValue valueVal );

	//////////////////////////////////////////////////////////////////////////
	// These functions are callbacks and callback-likes
	static void SearchCallback( CryLobbyTaskID taskID, ECryLobbyError error, SCrySessionSearchResult* session, void* arg );
	
	void GameLobbyJoinFinished( ECryLobbyError error );
	void AdjustCreateSessionData( SCrySessionData* pData, uint32 maxDataItems );
			
private:

	//private data types etc.
	enum EMatchMakingHandlerTask
	{
		eMMHT_None,
		eMMHT_EndSearch,
		eMMHT_EndJoin,
		eMMHT_EndMerge,
	};

	struct SessionDetails
	{
		char									m_name[MAX_SESSION_NAME_LENGTH];
		CrySessionID					m_id;
	};

	typedef std::map< int, SessionDetails >	TSessionIdMap;

	//private functions
	bool AddWaitingTask( EMatchMakingHandlerTask taskID, bool taskSuccess );

	//Static members
	static int s_currentMMSearchID;

	//Data members
	SCryLobbyUserData	m_sessionParams[ eLDI_Num ];
	TSessionIdMap			m_sessionIdMap;
	SmartScriptTable	m_pScript;
	
	uint32						m_nSessionParams;
	int								m_sessionIdIndex;
	float							m_startTime;
	bool							m_bIsMerging;

	//TODO: Make these into a queue so if we receive two tasks needing delay on the same frame we don't get screwed
	EMatchMakingHandlerTask m_waitingTask;
	bool										m_waitingTaskSuccess;

};

#endif	//__MATCHMAKINGHANDLER_H__

