#include "StdAfx.h"

#include "Game.h"
#include "GameFriendsManager.h"

#if IMPLEMENT_PC_BLADES

#include "ICryFriends.h"
#include "ICryFriendsManagement.h"
#include "IPlayerProfiles.h"

#include "Network/Lobby/GameLobby.h"
#include "Network/Lobby/GameBrowser.h"
#include "Network/Squad/SquadManager.h"
#ifdef GAME_IS_CRYSIS2
#include "FrontEnd/FlashFrontEnd.h"
#include "FrontEnd/Multiplayer/FriendsMenus.h"
#include "FrontEnd/Multiplayer/FriendsOverviewWidget.h"
#include "FrontEnd/Multiplayer/Friends/FriendsListDisplayManager.h"
#include "FrontEnd/Multiplayer/MPMenuHub.h"
#include "FrontEnd/WarningsManager.h"
#include "HUD/HUDUtils.h"

#include "PlayerProgression.h" // ranks etc.

// Cvars...
#include "HUD/HUD.h"
#include "HUD/HUDCVars.h"
#include "UI/UIManager.h"
#endif

#include "Network/GameNetworkDefines.h"

#if GFM_ENABLE_EXTRA_DEBUG
#include "Utility/CryWatch.h"
#ifdef GAME_IS_CRYSIS2
#include "Utility/CryDebugLog.h"
#endif
#endif // GFM_ENABLE_EXTRA_DEBUG

//.............................................................................................................

void SafeWarning(const char* warningId)
{
#ifdef GAME_IS_CRYSIS2
	CWarningsManager* pWarnings = g_pGame->GetWarnings();
	CGameFriendsManager* pGFm = g_pGame->GetGameFriendsManager();
	if(pWarnings && pGFm)
	{
		pWarnings->AddWarning(warningId, pGFm);
	}
#endif
}

//.............................................................................................................

#define gfmassert( cond ) assert( cond );

#if GFM_ENABLE_EXTRA_DEBUG
#ifdef GAME_IS_CRYSIS2
#include "HUD/HUD.h"
#include "HUD/HUDCVars.h"
#endif
#define GFMDbgLog( ... )                                       \
{                                                              \
	if(g_pGame)                                                  \
	{                                                            \
		if(CUIManager* pUI = g_pGame->GetUI())                         \
		{                                                          \
			if( pUI->GetCVars()->menu_friends_menus_debug )         \
			{                                                        \
				string tmp;                                            \
				tmp.Format(__VA_ARGS__);                               \
				CryLogAlways( "[GFM] : %s", tmp.c_str() );             \
			}                                                        \
		}                                                          \
	}                                                            \
}
#define GFMDbgLogAlways(...)                                   \
{                                                              \
	string tmp;                                                  \
	tmp.Format(__VA_ARGS__);                                     \
	CryLogAlways( "[GFM] : %s", tmp.c_str() );                   \
}
#define GFMDbgLogCond( show, ...) if(show){GFMDbgLog(__VA_ARGS__); }
#define GFMDbgLogAlwaysCond( show, ...) if(show){GFMDbgLogAlways(__VA_ARGS__); }
#else // GFM_ENABLE_EXTRA_DEBUG
#define GFMDbgLog(...)
#define GFMDbgLogAlways(...)
#define GFMDbgLogCond( show, ...)
#define GFMDbgLogAlwaysCond( show, ...)
#endif // GFM_ENABLE_EXTRA_DEBUG

//.............................................................................................................

/*static*/ TGameFriendId SGameFriendEventCache::__s_internal_ids = 0;
/*static*/ uint32 CGameFriendsManager::SPotentialFriendData::__s_internal_id_count = 0;

//.............................................................................................................

const bool CompareCryUserId( const CryUserID& c1, const CryUserID& c2 )
{
	return c1.IsValid() && ( c1 == c2 );
}

//.............................................................................................................

CGameFriendsManager::CGameFriendsManager()
: m_searchTaskID(CryLobbyInvalidTaskID)
, m_richPresenceTaskId(CryLobbyInvalidTaskID)
, m_currentUserOfInterest(-1)
, m_currentUserOfInterestsRow(-1)
, m_localUserId(NULL)
, m_userBeingAdded(NULL)
, m_richPresenceTimeOut(0.0f)
, m_queryInProgress(false)
, m_signedIn(false)
, m_scheduleAFriendListUpdate(false)
, m_scheduleADisplayListUpdate(false)
, m_haveFriendsList(false)
, m_searchQueryInProgress(false)
{
	m_friends.reserve( GFM_FRIENDSMENU_MAXFRIENDS );
	m_friendEventCache.reserve( GFM_FRIENDSMENU_MAXFRIENDS );
	m_friendRequests.reserve( GFM_FRIENDSMENU_MAXFRIENDS );
	m_potentialFriends.reserve( GFM_NUMBER_OF_FRIENDS_IN_RESULT );

#ifdef USE_C2_FRONTEND
	m_pDisplayList = new CFriendsListDisplayManager;
#endif //#ifdef USE_C2_FRONTEND

	assert( g_pGame );
	if( g_pGame )
	{
		g_pGame->GetIGameFramework()->RegisterListener(this, "FriendsMenu", eFLPriority_Game);

		if( ICryLobby* pLobby = gEnv->pNetwork->GetLobby() )
		{
			pLobby->RegisterEventInterest(eCLSE_FriendRequest, ListenerFriendRequest, (void*)this);
			pLobby->RegisterEventInterest(eCLSE_FriendMessage, ListenerFriendMessage, (void*)this);
			pLobby->RegisterEventInterest(eCLSE_FriendAuthorised, ListenerFriendAuthorised, (void*)this);
			pLobby->RegisterEventInterest(eCLSE_FriendRevoked, ListenerFriendRevoked, (void*)this);
			pLobby->RegisterEventInterest(eCLSE_RecievedInvite, ListenerMatchInvite, (void*)this);
			pLobby->RegisterEventInterest(eCLSE_FriendOnlineState, ListenerFriendOnlineStatus, (void*)this);
			pLobby->RegisterEventInterest(eCLSE_OnlineState, ListenerOnlineState, (void*)this);
		}
	}

#if GFM_ENABLE_EXTRA_DEBUG
	REGISTER_COMMAND("gfm_useDummyData", CGameFriendsManager::CMDPopulateWithDummyData, VF_CHEAT, "Enable/disable dummy data for the friends menus." );
#endif
}

CGameFriendsManager::~CGameFriendsManager()
{
#ifdef USE_C2_FRONTEND
	SAFE_DELETE(m_pDisplayList);
#endif //#ifdef USE_C2_FRONTEND

	if( g_pGame )
	{
		g_pGame->GetIGameFramework()->UnregisterListener(this);

		if( ICryLobby* pLobby = gEnv->pNetwork->GetLobby() )
		{
			pLobby->UnregisterEventInterest(eCLSE_FriendRequest, ListenerFriendRequest, (void*)this);
			pLobby->UnregisterEventInterest(eCLSE_FriendMessage, ListenerFriendMessage, (void*)this);
			pLobby->UnregisterEventInterest(eCLSE_FriendAuthorised, ListenerFriendAuthorised, (void*)this);
			pLobby->UnregisterEventInterest(eCLSE_FriendRevoked, ListenerFriendRevoked, (void*)this);
			pLobby->UnregisterEventInterest(eCLSE_RecievedInvite, ListenerMatchInvite, (void*)this);
			pLobby->UnregisterEventInterest(eCLSE_FriendOnlineState, ListenerFriendOnlineStatus, (void*)this);
			pLobby->UnregisterEventInterest(eCLSE_OnlineState, ListenerOnlineState, (void*)this);
		}
	}
}

CGameFriendsManager::TFriendsList::iterator CGameFriendsManager::GetFriendDataIteratorByCryUser( const CryUserID& userId )
{
	TFriendsList::iterator it = m_friends.begin();
	const TFriendsList::const_iterator end = m_friends.end();
	for(; it!=end; it++)
	{
		SGameFriend* myFriend = &(*it);
		if( CompareCryUserId( myFriend->m_userId, userId ) )
		{
			return it;
		}
	}
	return m_friends.end();
}

SGameFriend* CGameFriendsManager::GetFriendDataByCryUser( const CryUserID& userId )
{
	TFriendsList::iterator it = GetFriendDataIteratorByCryUser( userId );
	if( it != m_friends.end() )
	{
		SGameFriend* myFriend = &(*it);
		return myFriend;
	}
	return NULL;
}

SGameFriend* CGameFriendsManager::GetFriendDataByInternalId( const TGameFriendId userId )
{
	TFriendsList::iterator it = m_friends.begin();
	const TFriendsList::const_iterator end = m_friends.end();
	for(; it!=end; it++)
	{
		SGameFriend* myFriend = &(*it);
		if( myFriend->m_internalId == userId )
		{
			return myFriend;
		}
	}
	return NULL;
}

CGameFriendsManager::TFriendsListEventCache::iterator CGameFriendsManager::GetFriendCacheIteratorByCryUser( const CryUserID& userId )
{
	// Search for the friend in the existing data cache and return them if they exist.
	TFriendsListEventCache::iterator it = m_friendEventCache.begin();
	const TFriendsListEventCache::const_iterator end = m_friendEventCache.end();
	for(; it!=end; it++)
	{
		SGameFriendEventCache* myFriend = &(*it);
		if( CompareCryUserId( myFriend->m_userId, userId ) )
		{
			return it;
		}
	}
	return m_friendEventCache.end();
}

SGameFriendEventCache* CGameFriendsManager::GetFriendCacheByCryUser( const CryUserID& userId )
{
	// Search for the friend in the existing data cache and return them if they exist.
	TFriendsListEventCache::iterator it = GetFriendCacheIteratorByCryUser( userId );
	if( it != m_friendEventCache.end()  )
	{
		SGameFriendEventCache* myFriend = &(*it);
		return myFriend;
	}

	return NULL;
}

// This function returns one of :
//      * the data for a friend if they are known,
//      * the data cache for someone who should be a friend but the friend list hasn't updated yet.
//      * a new data cache pointer to store the data in.
//      * should only be called by event handlers.
SGameFriendEventCache* CGameFriendsManager::GetCreateFriendCacheByCryUser( const CryUserID& userId, bool* pOut_WasCreated/*=NULL*/ )
{
	SGameFriendEventCache* myFriend = NULL;
	if(pOut_WasCreated)
	{
		*pOut_WasCreated = false;
	}

	// Get friend info from the friends list if they exist there.
	myFriend = static_cast<SGameFriendEventCache*>( GetFriendDataByCryUser( userId ) );
	if( myFriend )
	{
		return myFriend;
	}

	// Get friend info from the friends request list if they exist there.
	myFriend = static_cast<SGameFriendEventCache*>( GetFriendRequestCacheByCryUser( userId ) );
	if( myFriend )
	{
		return myFriend;
	}

	// Search for the friend in the existing data cache and return them if they exist.
	myFriend = GetFriendCacheByCryUser( userId );
	if( myFriend )
	{
		return myFriend;
	}

	// we have no known friend with this Id nor do we have a friend-data-cache, create one:
	if(pOut_WasCreated)
	{
		*pOut_WasCreated = true;
	}
	SGameFriendEventCache newFriend;
	newFriend.m_userId = userId;
	TFriendsListEventCache::iterator newFriendIt = m_friendEventCache.insert( m_friendEventCache.end(), newFriend );

	return &(*newFriendIt); // should be pointer to a SGameFriendEventCache object
}

CGameFriendsManager::TFriendsList::iterator CGameFriendsManager::GetFriendRequestCacheIteratorByCryUser( const CryUserID& userId )
{
	// Search for the friend in the existing invites data cache and return them if they exist.
	TFriendsList::iterator it = m_friendRequests.begin();
	const TFriendsList::const_iterator end = m_friendRequests.end();
	for(; it!=end; it++)
	{
		SGameFriend* myFriend = &(*it);
		if( CompareCryUserId( myFriend->m_userId, userId ) )
		{
			return it;
		}
	}
	return m_friendRequests.end();
}

SGameFriend* CGameFriendsManager::GetFriendRequestCacheByCryUser( const CryUserID& userId )
{
	// Search for the friend in the existing data cache and return them if they exist.
	TFriendsList::iterator it = GetFriendRequestCacheIteratorByCryUser( userId );
	if( it != m_friendRequests.end() )
	{
		SGameFriend* myFriend = &(*it);
		return myFriend;
	}

	return NULL;
}

CGameFriendsManager::TFriendsList::iterator CGameFriendsManager::GetFriendRequestCacheIteratorByInternalId( const TGameFriendId userId )
{
	TFriendsList::iterator it = m_friendRequests.begin();
	const TFriendsList::const_iterator end = m_friendRequests.end();
	for(; it!=end; it++)
	{
		SGameFriend* myFriend = &(*it);
		if( myFriend->m_internalId == userId )
		{
			return it;
		}
	}
	return m_friendRequests.end();
}

SGameFriend* CGameFriendsManager::GetFriendRequestCacheByInternalId( const TGameFriendId userId )
{
	TFriendsList::iterator it = GetFriendRequestCacheIteratorByInternalId( userId );
	if( it != m_friendRequests.end() )
	{
		SGameFriend* myFriend = &(*it);
		return myFriend;
	}
	return NULL;
}

// This function returns one of :
//      * NULL if they are already a friend.
//      * a new friend-invite-cache populated with what ever friend event data we may have received (deletes the friend event cache entry in such instances). 
SGameFriend* CGameFriendsManager::GetCreateFriendRequestCacheByCryUser( const CryUserID& userId, const char* name )
{	
	GFMDbgLog( "GetCreateFriendRequestCacheByCryUser:" );
	INDENT_LOG_DURING_SCOPE();

	// Search for the friend in the existing data cache and return NULL if they DO exist i.e. the calling code should do nothing with this data.
	if( SGameFriend* existingFriendRequestInfo = GetFriendDataByCryUser( userId )  )
	{			
		GFMDbgLog( "Recieved a Friend Invite from someone who is _already_ our friend! Updating name." );
#if GFM_ENABLE_EXTRA_DEBUG
		if( strcmp(existingFriendRequestInfo->m_name, name))
		{
			//names not the same
			GFMDbgLog( "      Updating Friends name from '%s' to '%s'", existingFriendRequestInfo->m_name.c_str(), (name?name:"<INVALID>") );
		}
		else
		{
			GFMDbgLog( "      Friends name is the same, not updating '%s' to '%s'", existingFriendRequestInfo->m_name.c_str(), (name?name:"<INVALID>") );
		}
#endif // GFM_ENABLE_EXTRA_DEBUG

		// We already have this friend don't add them as a new one...
		existingFriendRequestInfo->m_name = name; // update name
		UpdateUsersRow(existingFriendRequestInfo->m_internalId);
		return NULL;
	}

	// check we don't already have requests from this user.
	TFriendsList::iterator newFriendIt = GetFriendRequestCacheIteratorByCryUser(userId);
	if( newFriendIt != m_friendRequests.end() )
	{
		GFMDbgLog( "Recieved a Friend Invite from someone who is _already_ in our friend request information! Updating name." );
		SGameFriend* existingFriendRequestInfo = &(*newFriendIt);
#if GFM_ENABLE_EXTRA_DEBUG
		if( strcmp(existingFriendRequestInfo->m_name.c_str(), name))
		{
			//names not the same
			GFMDbgLog( "      Updating Friends name from '%s' to '%s'", existingFriendRequestInfo->m_name.c_str(), (name?name:"<INVALID>") );
		}
		else
		{
			GFMDbgLog( "      Friends name is the same, not updating '%s' to '%s'", existingFriendRequestInfo->m_name.c_str(), (name?name:"<INVALID>") );
		}
#endif // GFM_ENABLE_EXTRA_DEBUG
		existingFriendRequestInfo->m_name = name;
		UpdateUsersRow(existingFriendRequestInfo->m_internalId);
		return existingFriendRequestInfo;
	}

	// a new friend (possible with associated event data, add 'em to the pending list)
	GFMDbgLog( "Recieved a Friend Invite from someone we know noting about." );
	SGameFriend newFriend;

	// Check to see if we have associated event data with this friend.
	TFriendsListEventCache::iterator eventIt = GetFriendCacheIteratorByCryUser( userId );
	if( eventIt != m_friendEventCache.end() )
	{
		const SGameFriendEventCache& eventData = *eventIt;
		newFriend = eventData; // copy event data into friend info.
		m_friendEventCache.erase(eventIt);
	}

#if GFM_ENABLE_EXTRA_DEBUG
	GFMDbgLog( "      Adding a new friend : '%s'", (name?name:"<INVALID>") );	
#endif // GFM_ENABLE_EXTRA_DEBUG

	// force reset on certain data
	newFriend.m_userId = userId;
	newFriend.m_name = name;
	newFriend.m_havename = false;

	// Insert into pending data list.
	newFriendIt = m_friendRequests.insert( m_friendRequests.end(), newFriend );
	SGameFriend& retNewFriendData = *newFriendIt;

	ScheduleDisplayListUpdate();
	return &retNewFriendData;
}

SGameFriend* CGameFriendsManager::GetAnyFriendByInternalId( const TGameFriendId userId )
{
	SGameFriend* pFriend = GetFriendDataByInternalId(userId);
	if( !pFriend )
	{
		pFriend = GetFriendRequestCacheByInternalId(userId);
	}
	return pFriend;
}

// should be scheduled by a call to ScheduleFriendsListUpdate();
void CGameFriendsManager::UpdateDisplayList()
{
#ifdef USE_C2_FRONTEND
	m_pDisplayList->FlushList();
	m_pDisplayList->AppendToList(m_friends);
	m_pDisplayList->AppendToList(m_friendRequests);
	m_pDisplayList->RepositionScrollingIfNeeded();
	if( CFlashFrontEnd *pFlashMenu = g_pGame->GetFlashMenu() )
	{
		if( IsOnValidFriendsScreen(pFlashMenu, pFlashMenu->GetFriendInfo()) )
		{
			pFlashMenu->CurrentScreenCommand( "menu_fm_update_friendslist", NULL );
		}
	}
#endif //#ifdef USE_C2_FRONTEND
}

void CGameFriendsManager::UpdateUsersRow(const TGameFriendId internalId)
{
#ifdef USE_C2_FRONTEND
	CFlashFrontEnd *pFlashMenu = g_pGame->GetFlashMenu();
	if(pFlashMenu)
	{
		const EFlashFrontEndScreens screen = pFlashMenu->GetCurrentMenuScreen();
		const bool validScreen = (screen==eFFES_friends_menus);
		if(validScreen)
		{
			const int rowId = GetFriendListDisplayManager()->GetRowForInternalUserId(internalId);
			const bool userIsDisplayed = (rowId>=0);
			SetCurrentUserOfInterest(internalId, rowId);
			if( userIsDisplayed )
			{
				pFlashMenu->CurrentScreenCommand( "mp_update_row", NULL );
			}
		}
	}
#endif //#ifdef USE_C2_FRONTEND
}

SGameFriend* CGameFriendsManager::AddUser( const SFriendInfo& inFriendInfo )
{
	SGameFriend friendInfo;
	
	// We may have some cached event data for this user...
	TFriendsListEventCache::iterator friendDataCacheIt = GetFriendCacheIteratorByCryUser( inFriendInfo.userID );
	const bool haveEventCache = (friendDataCacheIt != m_friendEventCache.end());
	if( haveEventCache )
	{
		SGameFriendEventCache& cachedEventData = (*friendDataCacheIt);
		friendInfo = cachedEventData; // copy data from cache into new.
	}

	// Copy in the new data.
	friendInfo.m_name = inFriendInfo.name;
	friendInfo.m_userId = inFriendInfo.userID; // will be the same if we have an event cache

	// put the new data into the vector of mates, note this will increment the internal-Id.
	m_friends.push_back(friendInfo);
	assert( m_friends.size() < GFM_FRIENDSMENU_MAXFRIENDS );

	// Get the pointer to that new friend info.
	SGameFriend* pNewFriend = &(m_friends.back());

	// If we had an event cache, force the internal id to persist (as later code may have 
	// pending actions for that userId) 
	// and remove the event-cache object from the vector of cached data.
	if( haveEventCache )
	{
		SGameFriendEventCache& cachedEventData = (*friendDataCacheIt);
		pNewFriend->m_internalId = cachedEventData.m_internalId; // force the internal id to persist.
		m_friendEventCache.erase( friendDataCacheIt ); // we no longer care about caching the data, flush that bad bouy. ding. dong.
	}

	return pNewFriend;
}

void CGameFriendsManager::AddUsers( const SFriendInfo* pFriendInfo, const int numFriends )
{
	GFMDbgLog( "CGameFriendsManager::AddUsers()" );
	INDENT_LOG_DURING_SCOPE();
	typedef CryFixedArray<CryUserID, GFM_FRIENDSMENU_MAXFRIENDS> TObsoleteFriends;
	TObsoleteFriends currentFriends;

	bool listHasChanged = false;

	// Iterate over the new friends and add them if we don't have them
	// only store one copy if we do have them (internal IDs persist).
	for(int i=0; i<numFriends; i++)
	{
		const SFriendInfo& inFriendInfo = pFriendInfo[i];

		GFMDbgLog( "| Adding User '%s' (%p)", inFriendInfo.name, inFriendInfo.userID.get() );

		SGameFriend* pFI= GetFriendDataByCryUser( inFriendInfo.userID );
		if( !pFI )
		{
			GFMDbgLog( "   Unknown! '%s' (%p)", inFriendInfo.name, inFriendInfo.userID.get() );
			// add em if the don't exist already.
			pFI = AddUser( inFriendInfo );
			assert( pFI );
			listHasChanged = true;
		}
		else
		{
			if( stricmp( pFI->m_name.c_str(), inFriendInfo.name )) // NOT the same, list has changed!
			{
				GFMDbgLog( "   Already known as '%s' changing to '%s' (%03d | %p)", pFI->m_name.c_str(), inFriendInfo.name, pFI->m_internalId, inFriendInfo.userID.get() );
				// the name may have changed, update it.
				pFI->m_name = inFriendInfo.name;
				listHasChanged = true;
			}
		}

		currentFriends.push_back( pFI->m_userId );
	}

	// So now remove friends that have been remove from our list
	TObsoleteFriends friendsToRemove;

	CGameFriendsManager::TFriendsList::iterator itF = m_friends.begin();
	const CGameFriendsManager::TFriendsList::const_iterator endF = m_friends.end();
	for(; itF!=endF; itF++)
	{
		bool found = false;

		SGameFriend& existingFriendInfo = (*itF);
		const CryUserID myFriendId = existingFriendInfo.m_userId;

		// don't remove if we're adding them as a friend.
		if( existingFriendInfo.m_friendRequestPending )
		{
			GFMDbgLog( "Not removing '%s' because we have a friend request pending", existingFriendInfo.m_name.c_str() );
			continue;
		}

#if GFM_ENABLE_EXTRA_DEBUG
		// don't remove if they're debug.
		if( existingFriendInfo.m_isDebug )
		{
			GFMDbgLog( "Not removing '%s' because they're a debug user", existingFriendInfo.m_name.c_str() );
			continue;
		}
#endif // GFM_ENABLE_EXTRA_DEBUG

		TObsoleteFriends::const_iterator itId = currentFriends.begin();
		const TObsoleteFriends::const_iterator endId = currentFriends.end();
		for(; itId!=endId; itId++)
		{
			const CryUserID currentFriendId = (*itId);
			if( CompareCryUserId( currentFriendId, myFriendId ) )
			{
				found = true;
				GFMDbgLog( "Not removing '%s' because they were found in the current friends list", existingFriendInfo.m_name.c_str() );
				break;
			}
		}

		if( !found )
		{
			friendsToRemove.push_back(myFriendId);
			GFMDbgLog( "Scehduling '%s' to be removed because they were not found in the current friends list", existingFriendInfo.m_name.c_str() );
			listHasChanged = true;
		}

		CGameFriendsManager::TFriendsList::iterator itR;
		for (itR = m_friendRequests.begin(); itR != m_friendRequests.end(); itR++)
		{
			const SGameFriend& friendRequestInfo = (*itR);

			if (CompareCryUserId(friendRequestInfo.m_userId, myFriendId))
			{
				GFMDbgLog( "Removing '%s' from friends request list as they are now in the current friends list", friendRequestInfo.m_name.c_str() );
				m_friendRequests.erase(itR);
				listHasChanged = true;
				break;
			}
		}

		if(existingFriendInfo.m_friendDeletePending)
		{
			existingFriendInfo.m_friendDeletePending= false; // Update friends list callbacks are considered authoritative
			listHasChanged = true;
		}

		if(existingFriendInfo.m_friendRequestPending)
		{
			existingFriendInfo.m_friendRequestPending = false; // is your friend
			listHasChanged = true;
		}

		if(existingFriendInfo.m_wantsToBeYourFriend)
		{
			existingFriendInfo.m_wantsToBeYourFriend = false; // is your friend
			listHasChanged = true;
		}
	}

	TObsoleteFriends::const_iterator itId = friendsToRemove.begin();
	const TObsoleteFriends::const_iterator endId = friendsToRemove.end();
	for(; itId!=endId; itId++)
	{
		const CryUserID& exFriendId = (*itId);
		RemoveFriend(exFriendId);
	}

	m_haveFriendsList = true;

	if( listHasChanged )
	{
		ScheduleDisplayListUpdate();
	}
}

void CGameFriendsManager::UpdateFriendsRichPresence(const float dtime)
{
	const float timeout = m_richPresenceTimeOut-dtime;
	const bool timedout = (timeout<0.0f);
	m_richPresenceTimeOut = timedout ? 3.0f : timeout;

	int numFriends = m_friends.size(); // may change as we don't count dummy 

	if( timedout && (m_richPresenceTaskId==CryLobbyInvalidTaskID) && (numFriends>0) )
	{
		CryUserID* newFriendUserIds = new CryUserID[numFriends];

		CGameFriendsManager::TFriendsList::iterator itF = m_friends.begin();
		const CGameFriendsManager::TFriendsList::const_iterator endF = m_friends.end();
		for(int i=0; itF!=endF; itF++, i++)
		{
			const SGameFriend& existingFriendInfo = (*itF);

			if(existingFriendInfo.m_friendDeletePending)
			{
				numFriends--;
				i--;
				continue;
			}

#if GFM_ENABLE_EXTRA_DEBUG
			if(existingFriendInfo.m_isDebug)
			{
				numFriends--;
				i--;
				continue;
			}
#endif //GFM_ENABLE_EXTRA_DEBUG

			newFriendUserIds[i] = existingFriendInfo.m_userId;
		}

		// Trigger a task to get this new user's rich presence.
		ICryFriendsManagement* pCryFriendsMgmt = gEnv->pNetwork->GetLobby()->GetFriendsManagement();
		uint32 userIndex = GetCurrentUserIndex();
		if(pCryFriendsMgmt)
		{
			//using the array of user id given above : CryUserID newFriendUserIds[numFriends];
			ECryLobbyError error = pCryFriendsMgmt->FriendsManagementGetStatus( userIndex, newFriendUserIds, numFriends, &m_richPresenceTaskId, FriendRequestRichPresCallback, this);
		}
		SAFE_DELETE_ARRAY(newFriendUserIds);
	}
}

void CGameFriendsManager::RemoveFriend( const CryUserID& userId )
{
	CGameFriendsManager::TFriendsList::iterator it = GetFriendDataIteratorByCryUser( userId );
	if( it != m_friends.end() )
	{
#if GFM_ENABLE_EXTRA_DEBUG
		const SGameFriend& removingFriend = *it;
		GFMDbgLog( "Removing '%s' from real friends", removingFriend.m_name.c_str() );
#endif // GFM_ENABLE_EXTRA_DEBUG
		m_friends.erase( it );
		ScheduleDisplayListUpdate();
		return;
	}
}

void CGameFriendsManager::UpdateFriendsList( void )
{
	// setup the friends callback
	if( m_queryInProgress )
	{
		return;
	}

	ICryFriends* pCryFriends = gEnv->pNetwork->GetLobby()->GetFriends();
	uint32 userIndex = GetCurrentUserIndex();

	if(pCryFriends)
	{
		GFMDbgLog( "CGameFriendsManager::UpdateFriendsList() : Attempting a FriendsGetFriendsList() call." );
		INDENT_LOG_DURING_SCOPE();
		ECryLobbyError error = pCryFriends->FriendsGetFriendsList( userIndex, 0, GFM_FRIENDSMENU_MAXFRIENDS, NULL, GetFriendsListCallback, this );

		switch( error )
		{
		case eCLE_Success :
			m_signedIn = true;
			m_queryInProgress = true;
			m_scheduleAFriendListUpdate = false;
			break;

		case eCLE_UserNotSignedIn :
		case eCLE_ConnectionFailed :
		case eCLE_InvalidUser :
		case eCLE_UnhandledNickError :
			GameWarning("CGameFriendsManager::UpdateFriendsList: Failed error, not signed in:%d", (int)error);
			m_signedIn = false;
			break;

		default :
			GameWarning("CGameFriendsManager::UpdateFriendsList: Failed error:%d", (int)error);
			break;
		}
	}
}

void CGameFriendsManager::OnPostUpdate(float fDeltaTime)
{
#ifdef USE_C2_FRONTEND
	CFlashFrontEnd *pFlashMenu = g_pGame->GetFlashMenu();
	CFriendsOverviewWidget* pFriendsWidget = pFlashMenu ? pFlashMenu->GetFriendInfo() : NULL;

	// periodically poll for friends and rich presence.
	if(IsOnValidFriendsScreen(pFlashMenu, pFriendsWidget))
	{
		UpdateFriendsRichPresence(fDeltaTime);

		if( m_scheduleAFriendListUpdate )
		{
			UpdateFriendsList();
		}
	}

	if(m_scheduleADisplayListUpdate)
	{
		UpdateDisplayList();
		m_scheduleADisplayListUpdate = false;
	}

	// Update the friends widget.
	if( pFriendsWidget )
	{
		int numberOfFriendsOnline = 0;
		int numberOfInvites = 0;

		TFriendsList::const_iterator it = m_friends.begin();
		const TFriendsList::const_iterator end = m_friends.end();
		for(; it!=end; it++)
		{
			const SGameFriend& mateInfo = *it;
			if( mateInfo.m_isOnline )
			{
				numberOfFriendsOnline++;
			}

			if( mateInfo.m_isInvitingToMatch )
			{
				numberOfInvites++;
			}

			if( mateInfo.m_wantsToBeYourFriend )
			{
				numberOfInvites++;
			}
		}

		TFriendsList::const_iterator itReq = m_friendRequests.begin();
		const TFriendsList::const_iterator endReq = m_friendRequests.end();
		for(; itReq!=endReq; itReq++)
		{
			const SGameFriend& mateInfo = *itReq;

			if( mateInfo.m_isInvitingToMatch )
			{
				numberOfInvites++;
			}

			if( mateInfo.m_wantsToBeYourFriend )
			{
				numberOfInvites++;
			}
		}

		pFriendsWidget->SetData(numberOfFriendsOnline, numberOfInvites);
	}
		

#if GFM_ENABLE_EXTRA_DEBUG
	if(g_pGame->GetHUD()->GetCVars()->menu_friends_menus_debug>1)
	{
		EFlashFrontEndScreens currentScreen = pFlashMenu->GetCurrentMenuScreen();
		CryWatch( "Current screen id = %d", currentScreen );
		/*
		switch( currentScreen )
		{
		case eFFES_main :
		if( !g_pGame->IsGameActive() )
		{
		return;
		}
		// ok.
		break;
		case eFFES_play_online :
		case eFFES_game_lobby :
		case eFFES_leaderboards :
		//ok.
		break;
		default :
		// not ok.
		return;
		}*/

		CGameLobby* pLobby = g_pGame->GetGameLobby();
		CryUserID localUserId = pLobby->GetLocalUserId();
		/*
		CryFixedStringT<DISPLAY_NAME_LENGTH> localPlayerName("");
		pLobby->GetLocalUserDisplayName( localPlayerName );
		*/

		IPlayerProfileManager *pPlayerProfileManager = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager();
		const int userIndex = pPlayerProfileManager ? pPlayerProfileManager->GetExclusiveControllerDeviceIndex() : 0;
		IPlatformOS::TUserName tUserName;
		if (gEnv->pSystem->GetPlatformOS()->UserGetOnlineName(userIndex, tUserName) == false)
		{
			gEnv->pSystem->GetPlatformOS()->UserGetName(userIndex, tUserName);
		}

		CryWatch( "Looking at GS info for %s (%p)", tUserName.c_str(), localUserId.get() );

		if( m_friends.size() || m_friendEventCache.size() || m_friendRequests.size() )
		{
			CryWatch( "I have friends, yay!" );

			CryWatch( " | Friend List : " );
			TFriendsList::const_iterator it = m_friends.begin();
			const TFriendsList::const_iterator end = m_friends.end();
			for(; it!=end; it++)
			{
				const SGameFriend& fi = *it;
				CryWatch( "  %s (%03d | %p) : %s%s%s%s", fi.m_name.c_str(), fi.m_internalId, fi.m_userId.get(), ( fi.m_isOnline?"online":"offline"), (fi.m_isInvitingToMatch?" : inviting" : ""), (fi.m_wantsToBeYourFriend?" : WantsToBeFriend":""), (fi.m_friendDeletePending?" : Deleting":"") );
			}

			CryWatch( " | Event Data For Friends not in List : " );
			TFriendsListEventCache::const_iterator it_cache = m_friendEventCache.begin();
			const TFriendsListEventCache::const_iterator end_cache = m_friendEventCache.end();
			for(; it_cache!=end_cache; it_cache++)
			{
				const SGameFriendEventCache& fi = *it_cache;
				CryWatch( "  Unknown (%03d | %p) : %s '%s'%s%s", fi.m_internalId, fi.m_userId.get(), ( fi.m_isOnline?"online":"offline"), fi.m_richPres.c_str(), (fi.m_isInvitingToMatch?" : inviting" : ""), (fi.m_wantsToBeYourFriend?" : WantsToBeFriend":"") );
			}

			CryWatch( " | Friends requests : " );
			it = m_friendRequests.begin();
			const TFriendsList::const_iterator end_friendRequests = m_friendRequests.end();
			for(; it!=end_friendRequests; it++)
			{
				const SGameFriend& fi = *it;
				assert( fi.m_friendRequestPending );
				CryWatch( "  %s (%03d | %p) : %s%s%s", fi.m_name.c_str(), fi.m_internalId, fi.m_userId.get(), ( fi.m_isOnline?"online":"offline"), (fi.m_isInvitingToMatch?" : inviting" : ""), (fi.m_wantsToBeYourFriend?" : WantsToBeFriend":"") );
			}
		}
		else
		{
			CryWatch( "I have no friends :'(");
		}
	}
#endif //GFM_ENABLE_EXTRA_DEBUG
#endif //#ifdef USE_C2_FRONTEND
}

bool CGameFriendsManager::OnWarningReturn(THUDWarningId id, const char* returnValue)
{
#ifdef USE_C2_FRONTEND
	if ( ( id == g_pGame->GetWarnings()->GetWarningId("TooManyFriendTasks") ) ||
			 ( id == g_pGame->GetWarnings()->GetWarningId("InvalidFriendRequest") ) ||
			 ( id == g_pGame->GetWarnings()->GetWarningId("FriendsInternalError") ) )
	{
		g_pGame->GetFlashMenu()->ScreenBack();
	}
#endif //#ifdef USE_C2_FRONTEND
	return true;
}

bool CGameFriendsManager::StartFriendSearch( const char* searchStr )
{
	assert(!m_searchQueryInProgress);

	ICryLobby* pLobby = gEnv->pNetwork->GetLobby();
	assert( pLobby );
	if( pLobby )
	{
		ICryFriendsManagement* pFriendsManager = pLobby->GetFriendsManagement();
		assert(pFriendsManager);
		if( pFriendsManager )
		{
			uint32 localUserIdx = GetCurrentUserIndex();
			SFriendManagementSearchInfo si;
			cry_strncpy( si.name, searchStr, CRYLOBBY_USER_NAME_LENGTH );
			m_searchString = searchStr;
			ECryLobbyError error = pFriendsManager->FriendsManagementFindUser(localUserIdx, &si, GFM_NUMBER_OF_FRIENDS_IN_RESULT, &m_searchTaskID, FriendsSearchCallback, NULL );
			if( error == eCLE_Success )
			{
				m_potentialFriends.clear();
				m_searchQueryInProgress = true;
				return true;
			}
		}
	}	
	return false;
}

void CGameFriendsManager::SendFriendRequest( /*const*/ CryUserID& userId, const char* name )
{
	GFMDbgLog( "SendFriendRequest" );
	INDENT_LOG_DURING_SCOPE();

	if( userId.IsValid() && name )
	{
		ICryLobby* pLobby = gEnv->pNetwork->GetLobby();
		assert( pLobby );
		if( pLobby )
		{
			ICryFriendsManagement* pFriendsManager = pLobby->GetFriendsManagement();
			assert(pFriendsManager);
			if( pFriendsManager )
			{
				uint32 localUserIdx = GetCurrentUserIndex();
				GFMDbgLog( "Sending friend request (ICryFriendsManagement), requesting %s (%p) to be my friend", name, userId.get() );
				INDENT_LOG_DURING_SCOPE();
				const ECryLobbyError error = pFriendsManager->FriendsManagementSendFriendRequest(localUserIdx, &userId, 1, NULL, SendFriendRequestCallback, (void*)this);
				
				gfmassert(error == eCLE_Success); // temp debug assert;
				if( error == eCLE_Success )
				{
					if( !GetFriendDataByCryUser(userId) )
					{
						// Creates one if we don't have one.
						SGameFriend* pMateCache = GetCreateFriendRequestCacheByCryUser( userId, name );
						assert(pMateCache);
						if(pMateCache)
						{
							pMateCache->m_friendRequestPending = true;
							m_userBeingAdded = userId;

							ICryFriendsManagement* pCryFriendsMgmt = gEnv->pNetwork->GetLobby()->GetFriendsManagement();
							if(pCryFriendsMgmt)
							{
								uint32 userIndex = GetCurrentUserIndex();
								GFMDbgLog( "Starting a search for '%s's _real_ name.", name );
								ECryLobbyError error = pCryFriendsMgmt->FriendsManagementGetName( userIndex, &userId, 1, NULL, FriendRequestEventGetNameCallback, this );

								switch(error)
								{
								case eCLE_Success:
									//...
									GFMDbgLog( "Request sent for friend's real name, have '%s'", name );
									break;
								default :
									GFMDbgLog( "Request failed for friend's real name, have '%s'", name );
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	else
	{
		GFMDbgLog( "Invalid Data!" );
	}
}

void CGameFriendsManager::AcceptFriendRequest( const TGameFriendId userId )
{
	GFMDbgLog( "AcceptFriendRequest" );
	INDENT_LOG_DURING_SCOPE();

	TFriendsList::iterator mateSourceDataIt = GetFriendRequestCacheIteratorByInternalId( userId );
	assert(mateSourceDataIt!=m_friendRequests.end());
	if(mateSourceDataIt!=m_friendRequests.end())
	{
		const SGameFriend& newFriendSourceData = *mateSourceDataIt;
		assert(m_friends.size()<GFM_FRIENDSMENU_MAXFRIENDS);
		m_friends.push_back( newFriendSourceData ); // copy cached friend on to actual-friend array.
		m_friendRequests.erase(mateSourceDataIt);

		SGameFriend* pMate = GetFriendDataByInternalId( userId );
		assert(pMate && "[GFM] this should _never_ be NULL!"); 

		if( pMate )
		{
			m_currentUserOfInterest = pMate->m_internalId;

			ICryLobby* pLobby = gEnv->pNetwork->GetLobby();
			assert( pLobby );
			if( pLobby )
			{
				ICryFriendsManagement* pFriendsManager = pLobby->GetFriendsManagement();
				assert(pFriendsManager);
				if( pFriendsManager )
				{
					uint32 localUserIdx = GetCurrentUserIndex();
					GFMDbgLog( "Accepting Friends Request (ICryFriendsManagement), new friend should be %s", pMate->m_name.c_str() );
					INDENT_LOG_DURING_SCOPE();
					CryUserID gsUserId = pMate->m_userId;
					const ECryLobbyError error = pFriendsManager->FriendsManagementAcceptFriendRequest(localUserIdx, &(gsUserId), 1, NULL, AcceptFriendRequestCallback, (void*)this);

					gfmassert(error == eCLE_Success); // temp debug assert;
					if( error == eCLE_Success )
					{
						// OK, wait for call-backs and events.
						// friend should now appear in the list.
						ScheduleFriendsListUpdate();
					}
				}
			}
		}
	}
}

void CGameFriendsManager::DeclineFriendRequest( const TGameFriendId userId )
{
	GFMDbgLog( "DeclineFriendRequest" );
	INDENT_LOG_DURING_SCOPE();

	SGameFriend* pMate = GetFriendRequestCacheByInternalId( userId );

	if( pMate )
	{
		ICryLobby* pLobby = gEnv->pNetwork->GetLobby();
		assert( pLobby );
		if( pLobby )
		{
			ICryFriendsManagement* pFriendsManager = pLobby->GetFriendsManagement();
			assert(pFriendsManager);
			if( pFriendsManager )
			{
				uint32 localUserIdx = GetCurrentUserIndex();
				GFMDbgLog( "CGameFriendsManager: Rejecting Friends Request (ICryFriendsManagement) from %s", pMate->m_name.c_str() );
				INDENT_LOG_DURING_SCOPE();
				const ECryLobbyError error = pFriendsManager->FriendsManagementRejectFriendRequest(localUserIdx, &(pMate->m_userId), 1, NULL, DeclineFriendRequestCallback, (void*)this);

				gfmassert(error == eCLE_Success); // temp debug assert;
				if( error == eCLE_Success )
				{
					pMate->m_isInvitingToMatch = false;
					pMate->m_isInvitingToMatchConsumed=false;
					TFriendsList::iterator it = GetFriendRequestCacheIteratorByCryUser(pMate->m_userId);
					if(it!=m_friendRequests.end())
					{
						m_friendRequests.erase(it);
						ScheduleDisplayListUpdate();
					}
				}
			}
		}
	}
}

void CGameFriendsManager::SendMatchInvite( const TGameFriendId userId )
{
	GFMDbgLog( "SendMatchInvite" );
	INDENT_LOG_DURING_SCOPE();

	SGameFriend* pMate = GetFriendDataByInternalId( userId );

	if( pMate )
	{
		ICryLobby* pLobby = gEnv->pNetwork->GetLobby();
		assert( pLobby );
		if( pLobby )
		{
			ICryFriends* pFriends = pLobby->GetFriends();
			assert(pFriends);
			if( pFriends )
			{
				uint32 localUserIdx = GetCurrentUserIndex();
				GFMDbgLog( "CGameFriendsManager: Sending Match Invite (ICryFriends) to %s", pMate->m_name.c_str() );
				INDENT_LOG_DURING_SCOPE();
				CrySessionHandle sessionHandle = g_pGame->GetSquadManager()->GetSquadSessionHandle();
				const ECryLobbyError error = pFriends->FriendsSendGameInvite(localUserIdx, sessionHandle, &(pMate->m_userId), 1, NULL, SendMatchInviteCallback, (void*)this);

				gfmassert(error == eCLE_Success); // temp debug assert;
				if( error == eCLE_Success )
				{
					GFMDbgLog( "CGameFriendsManager: Match Invite sent ok, sucess" );
					pMate->m_hasBeenInvitedToMatch = true;
					UpdateUsersRow(pMate->m_internalId);
				}
			}
		}
	}
}

void CGameFriendsManager::AcceptMatchInvite( const TGameFriendId userId )
{
	GFMDbgLog( "AcceptMatchInvite" );
	INDENT_LOG_DURING_SCOPE();

	SGameFriend* pMate = GetFriendDataByInternalId( userId );
	assert(pMate);

	if( pMate )
	{
		ICryLobby* pLobby = gEnv->pNetwork->GetLobby();
		assert( pLobby );
		if( pLobby )
		{
			ICryFriendsManagement* pFriendsManager = pLobby->GetFriendsManagement();
			assert(pFriendsManager);
			if( pFriendsManager )
			{
				uint32 localUserIdx = GetCurrentUserIndex();
				GFMDbgLog( "CGameFriendsManager: Accepting Match Invite (ICryFriendsManagement) from %s", pMate->m_name.c_str() );
				INDENT_LOG_DURING_SCOPE();
				const ECryLobbyError error = pFriendsManager->FriendsManagementAcceptInvite(localUserIdx, &(pMate->m_userId), NULL, AcceptMatchInviteCallback, (void*)this);

				gfmassert(error == eCLE_Success); // temp debug assert;
				if( error == eCLE_Success )
				{
					GFMDbgLog( "CGameFriendsManager: Accepting Match Invite, success" );
					// OK, wait for call-back
					// SHOULDN't this be done within the callback then?
					if( SGameFriend* pMate = GetFriendDataByInternalId( userId ) )
					{
						pMate->m_isInvitingToMatch = false;
						pMate->m_isInvitingToMatchConsumed=false;
						pMate->m_hasBeenInvitedToMatch = false;
						pMate->m_taggedForBatchMatchInvite = false;
						ScheduleFriendsListUpdate();
					}					
				}
				else
				{
					GFMDbgLog( "CGameFriendsManager: Accepting Match Invite, FAILED!" );
				}
			}
		}
	}
}

void CGameFriendsManager::DeclineMatchInvite( const TGameFriendId userId )
{
	GFMDbgLog( "CGameFriendsManager: Declining match invite." );
	INDENT_LOG_DURING_SCOPE();
	// No ICryFriendsManagement interface
	SGameFriend* pMate = GetFriendDataByInternalId(userId);
	assert(pMate);
	if( pMate )
	{
		pMate->m_isInvitingToMatch = false;
		pMate->m_isInvitingToMatchConsumed=false;
		ScheduleFriendsListUpdate();
	}
}

void CGameFriendsManager::RequestRemoveFriend( const TGameFriendId userId )
{
	GFMDbgLog( "RequestRemoveFriend" );
	INDENT_LOG_DURING_SCOPE();

	SGameFriend* pMate = GetFriendDataByInternalId( userId );
	assert(pMate);

	if( pMate )
	{
		ICryLobby* pLobby = gEnv->pNetwork->GetLobby();
		assert( pLobby );
		if( pLobby )
		{
			ICryFriendsManagement* pFriendsManager = pLobby->GetFriendsManagement();
			assert(pFriendsManager);
			if( pFriendsManager )
			{
				uint32 localUserIdx = GetCurrentUserIndex();
				GFMDbgLog( "Removing Friend %s (%03d | %p)", pMate->m_name.c_str(), pMate->m_internalId, pMate->m_userId.get() );
				INDENT_LOG_DURING_SCOPE();
				const ECryLobbyError error = pFriendsManager->FriendsManagementRevokeFriendStatus(localUserIdx, &(pMate->m_userId), 1, NULL, RemoveFriendCallback, (void*)this);

				gfmassert(error == eCLE_Success); // temp debug assert;
				if( error == eCLE_Success )
				{
					// OK, wait for call-back
					pMate->m_friendDeletePending = true;
					ScheduleFriendsListUpdate();
				}
			}
		}
	}
}

void CGameFriendsManager::RequestUserData( const TGameFriendId userId )
{
	GFMDbgLog( "RequestUserData (ranks)" );
	INDENT_LOG_DURING_SCOPE();

	assert(0);
	/*
	SGameFriend* pMate = GetFriendDataByInternalId( userId );
	assert(pMate);

	if( pMate )
	{

	ICryStats* stats = gEnv->pNetwork->GetLobby()->GetStats();
	if(stats)
	{
		CryLobbyTaskID taskID = CryLobbyInvalidTaskID;
		error = stats->StatsReadUserData(0, pMate->m_userId, &taskID, CGameLobby::ReadOnlineDataCallback, this);
		if(error == eCLE_Success)
		{
			taskStarted = true;
		}
	}
	break;
	*/
}

void CGameFriendsManager::JoinUsersGame( const TGameFriendId userId )
{
	GFMDbgLog( "JoinUsersGame" );
	INDENT_LOG_DURING_SCOPE();

	SGameFriend* pMate = GetAnyFriendByInternalId(userId);
	assert(pMate);
	if(pMate)
	{
		if(!(pMate->m_sessionConnection.empty()))
		{
			ICryLobby*				pLobby = gEnv->pNetwork->GetLobby();

			if ( pLobby )
			{
				ICryLobbyService*	pLobbyService = pLobby->GetLobbyService();

				if ( pLobbyService )
				{
					ICryMatchMaking*	pMatchMaking = pLobbyService->GetMatchMaking();

					if ( pMatchMaking )
					{
						CrySessionID			sessionID;
						ECryLobbyError		error = pMatchMaking->GetSessionIDFromSessionURL( pMate->m_sessionConnection.c_str(), &sessionID );

						if ( sessionID )
						{
#ifdef USE_C2_FRONTEND
							CFlashFrontEnd*		pFlashFrontEnd = g_pGame->GetFlashMenu();

							if ( pFlashFrontEnd )
							{
								CMPMenuHub*				pMPMenuHub = pFlashFrontEnd->GetMPMenu();

								if ( pMPMenuHub )
								{
									CGameLobby*				pGameLobby = g_pGame->GetGameLobby();

									if ( pGameLobby )
									{
										GFMDbgLog( "JoinUsersGame, session join sent to join user '%s'(%d) at \"%s\"", pMate->m_name.c_str(), userId, pMate->m_sessionConnection.c_str() );
										pMPMenuHub->OnGameSessionAction();
										pGameLobby->JoinServer( sessionID, NULL, CryMatchMakingInvalidConnectionUID, true );
									}
								}
							}
#endif //#ifdef USE_C2_FRONTEND
						}
					}
				}
			}
		}
		else
		{
#ifdef GAME_IS_CRYSIS2
			CWarningsManager *pWM = g_pGame->GetWarnings();
			if(pWM)
			{
				pWM->AddWarning("SessionNotAvailable", NULL);
			}
#endif
			GFMDbgLog( "JoinUsersGame, FAILED, user '%s'(%d) doesn't have a session string!", pMate->m_name.c_str(), userId );
		}
	}
	else
	{
		GFMDbgLog( "JoinUsersGame, FAILED, user %d notfound", userId );
	}
}

//.............................................................................................................

/*static*/ uint32 CGameFriendsManager::GetCurrentUserIndex()
{
	IPlayerProfileManager *pPlayerProfileManager = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager();
	return pPlayerProfileManager ? pPlayerProfileManager->GetExclusiveControllerDeviceIndex() : 0;
}

//.............................................................................................................
/// Callbacks

/*static*/ void CGameFriendsManager::GetFriendsListCallback(CryLobbyTaskID taskID, ECryLobbyError error, SFriendInfo* pFriendInfo, uint32 numFriends, void* pArg)
{
	GFMDbgLog( "CGameFriendsManager::GetFriendsListCallback()" );
	INDENT_LOG_DURING_SCOPE();

	if( CGameFriendsManager* pFriendsMenu = g_pGame ? g_pGame->GetGameFriendsManager() : NULL )
	{
		pFriendsMenu->m_queryInProgress = false;
		assert(pArg);
		if( error == eCLE_Success )
		{
			GFMDbgLog( "CGameFriendsManager::GetFriendsListCallback() success" );
			pFriendsMenu->AddUsers( pFriendInfo, numFriends );
			return; // ok
		}
	}
	GFMDbgLog( "CGameFriendsManager::GetFriendsListCallback() FAIED!" );
}

/*static*/ void CGameFriendsManager::SendFriendRequestCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pCbArg)
{
	GFMDbgLog( "CGameFriendsManager::SendFriendRequestCallback" );

	gfmassert(error == eCLE_Success);
	assert(pCbArg);
	if( CGameFriendsManager* pGfm = (CGameFriendsManager*)pCbArg )
	{
#if GFM_ENABLE_EXTRA_DEBUG
		SGameFriend* pMate = pGfm->GetFriendRequestCacheByCryUser(pGfm->m_userBeingAdded);
		assert(pMate);
#endif //GFM_ENABLE_EXTRA_DEBUG
		pGfm->m_userBeingAdded = 0;
	}
}

/*static*/ void CGameFriendsManager::AcceptFriendRequestCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pCbArg)
{
	GFMDbgLog( "CGameFriendsManager::AcceptFriendRequestCallback" );
	INDENT_LOG_DURING_SCOPE();

	gfmassert(error == eCLE_Success);
	assert(pCbArg);
	if( CGameFriendsManager* pGfm = (CGameFriendsManager*)pCbArg )
	{
		//...
	}
}

/*static*/ void CGameFriendsManager::DeclineFriendRequestCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pCbArg)
{
	GFMDbgLog( "CGameFriendsManager::DeclineFriendRequestCallback" );
	INDENT_LOG_DURING_SCOPE();

	gfmassert(error == eCLE_Success);
	assert(pCbArg);
	if( CGameFriendsManager* pGfm = (CGameFriendsManager*)pCbArg )
	{
		//...
	}

}

/*static*/ void CGameFriendsManager::SendMatchInviteCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pCbArg)
{
	GFMDbgLog( "CGameFriendsManager::SendMatchInviteCallback" );
	INDENT_LOG_DURING_SCOPE();

	gfmassert(error == eCLE_Success);
	switch(error)
	{
	case eCLE_Success:
		//...
		GFMDbgLog( "CGameFriendsManager::SendMatchInviteCallback(): Match Invite sent ok, sucess" );
		break;
	default :
		GFMDbgLog( "CGameFriendsManager::SendMatchInviteCallback() : FAILED!" );
		SafeWarning("FriendsInternalError");
		break;
	}
}

/*static*/ void CGameFriendsManager::AcceptMatchInviteCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pCbArg)
{
	GFMDbgLog( "CGameFriendsManager::AcceptMatchInviteCallback" );

	gfmassert(error == eCLE_Success);
	switch(error)
	{
	case eCLE_Success:
		//...
		GFMDbgLog( "CGameFriendsManager::AcceptMatchInviteCallback(): Match Invite sent ok, sucess" );
		break;
	default :
		GFMDbgLog( "CGameFriendsManager::AcceptMatchInviteCallback() : FAILED!" );
		SafeWarning("FriendsInternalError");
		break;
	}
}

// /*static*/ void DeclineMatchInviteCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pCbArg); -- No ICryFriendsManagement interface
//{
//
//}

/*static*/ void CGameFriendsManager::RemoveFriendCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pCbArg)
{
	GFMDbgLog( "CGameFriendsManager::RemoveFriendCallback" );

	gfmassert(error == eCLE_Success);
	switch(error)
	{
	case eCLE_Success:
		{
			GFMDbgLog( "CGameFriendsManager::RemoveFriendCallback(): success" );
			CGameFriendsManager* pGfm = static_cast<CGameFriendsManager*>(pCbArg);
			if(pGfm)
			{
				pGfm->ScheduleFriendsListUpdate();
			}
		}
		break;
	default :
		GFMDbgLog( "CGameFriendsManager::RemoveFriendCallback() : FAILED!" );
		SafeWarning("FriendsInternalError");
		break;
	}
}


/*static*/ void CGameFriendsManager::FriendRequestEventGetNameCallback(CryLobbyTaskID taskID, ECryLobbyError error, SFriendInfo* pInfo, uint32 numUserIDs, void* pCbArg)
{
	GFMDbgLog( "FriendRequestEventGetNameCallback" );
	INDENT_LOG_DURING_SCOPE();
	switch( error )
	{
	case eCLE_TooManyTasks: 
		{
			SafeWarning("FriendsInternalError");
		}
		break;
	case eCLE_InvalidRequest:
		{
		}
		break;
	case eCLE_Success :
		{
			GFMDbgLog( "FriendRequestEventGetNameCallback: Success" );
			if( CGameFriendsManager* pGfm = static_cast<CGameFriendsManager*>(pCbArg) )
			{
				for(int i=0; i<numUserIDs; i++ )
				{
					SFriendInfo& friendInfo = pInfo[i];
					CryUserID userId = friendInfo.userID;
					SGameFriend* pMateRequestCache = pGfm->GetCreateFriendRequestCacheByCryUser( userId, friendInfo.name );
					if(pMateRequestCache)
					{
						pGfm->ScheduleFriendsListUpdate();
					}
				}
			}
		}
		break;
	default :
		{
			GameWarning( "!Unhandled error type on search" );
		}
		break;
	}
}

/*static*/ void CGameFriendsManager::FriendRequestRichPresCallback(CryLobbyTaskID taskID, ECryLobbyError error, SFriendStatusInfo* pInfo, uint32 numUserIDs, void* pCbArg)
{
	CGameFriendsManager* pGfm = static_cast<CGameFriendsManager*>(pCbArg);

	assert(pGfm);
	if(pGfm)
	{
		pGfm->m_richPresenceTaskId = CryLobbyInvalidTaskID;
	}

	switch( error )
	{
	case eCLE_TooManyTasks: 
		{
			SafeWarning("FriendsInternalError");
		}
		break;
	case eCLE_InvalidRequest:
		{
			SafeWarning("InvalidFriendRequest");
		}
		break;
	case eCLE_Success :
		{
			assert(pInfo);
			assert(numUserIDs>0);

			GFMDbgLog( "Recieved rich pres information : " );
			SGameFriendEventCache* pMate = NULL;
			if(pGfm)
			{
				for(int i=0; i<numUserIDs; i++)
				{
					const SFriendStatusInfo& info = pInfo[i];
					pMate = pGfm->GetCreateFriendCacheByCryUser(info.userID);
					CRY_ASSERT_MESSAGE( pMate, "Failed to find game friend or game friend cache data for a CryUserId on FriendRequestRichPresCallback()" );
					if(pMate)
					{
						CryFixedStringT<MAX_PRESENCE_STRING_SIZE> localisedRp;
						if( info.statusString==strstr(info.statusString, "@mp_rp_gameplay") )
						{
#if USE_CRYLOBBY_GAMESPY
							CryFixedStringT<MAX_PRESENCE_STRING_SIZE> toUnPack(info.statusString);
							CGameBrowser::UnpackRecievedInGamePresenceString(localisedRp,toUnPack);
#endif
						}
						else if( 0==stricmp("offline", info.statusString) )
						{
#ifdef GAME_IS_CRYSIS2
							localisedRp = CHUDUtils::LocalizeString("@menu_rp_offline");
#endif
						}
						else
						{
#ifdef GAME_IS_CRYSIS2
							localisedRp = CHUDUtils::LocalizeString(info.statusString);
#endif
						}

						if(0!=pMate->m_richPres.compare(localisedRp.c_str()))
						{
							pMate->m_richPres = localisedRp;
							pGfm->UpdateUsersRow(pMate->m_internalId);
						}

						pMate->m_sessionConnection.clear();
						if ( strlen( info.locationString ) )
						{
							pMate->m_sessionConnection = info.locationString;
						}

#if GFM_ENABLE_EXTRA_DEBUG
						const char* name = "CACHED USER!";
						SGameFriend* pExistingMate = pGfm->GetFriendDataByCryUser(info.userID);
						if(pExistingMate)
						{
							name = pExistingMate->m_name.c_str();
						}
						GFMDbgLog( "    RichPres for '%s'(%p) : Loc'%s', Status'%s'(%d)", name, info.userID.get(), info.locationString, info.statusString, info.status );
#endif //GFM_ENABLE_EXTRA_DEBUG
					}
				}
			}
		}
		break;
	default :
		{
			GameWarning( "!Unhandled error type on search" );
		}
		break;
	}
}

/*static*/ void CGameFriendsManager::FriendsSearchCallback(CryLobbyTaskID taskID, ECryLobbyError error, SFriendInfo* pInfo, uint32 numUserIDs, void* pCbArg)
{
	CGameFriendsManager* pGfm = g_pGame->GetGameFriendsManager();

	bool searchSuccessful = false;
	bool setNoNewFriendsFound = false;

	if(pGfm)
	{
		pGfm->m_searchQueryInProgress = false; // it is now ok to search again
		pGfm->m_searchTaskID = CryLobbyInvalidTaskID; 

		if( error == eCLE_Success )
		{
			if( numUserIDs == 0 )
			{
#ifdef USE_C2_FRONTEND
				if( CFlashFrontEnd *pFlashMenu = g_pGame->GetFlashMenu() )
				{
					searchSuccessful = false;
				}
#endif //#ifdef USE_C2_FRONTEND
			}
			else
			{
				assert(pInfo);
				for(int i=0; i<numUserIDs; i++)
				{
					const SFriendInfo& inPotentialMate = pInfo[i];
					SPotentialFriendData potentialMate;

					potentialMate.m_name = inPotentialMate.name;
					potentialMate.m_cryUserId = inPotentialMate.userID;

					const size_t size = pGfm->m_potentialFriends.size();
					const SGameFriend* mate = pGfm->GetFriendDataByCryUser( inPotentialMate.userID );
					const bool alreadyExists = mate ? true : false;
					if( !alreadyExists && size < GFM_NUMBER_OF_FRIENDS_IN_RESULT )
					{
						pGfm->m_potentialFriends.push_back( potentialMate );
					}
				}

				if( (pGfm->m_potentialFriends.size()==0) &&  (numUserIDs>0) )
				{
					setNoNewFriendsFound = true;
					searchSuccessful = false;
				}
				else
				{
					searchSuccessful = true;
				}
			}
		}
	}
	else
	{
		assert(!"Unhandled networking error");
	}

	// Update the menus.
#ifdef USE_C2_FRONTEND
	if( CFlashFrontEnd *pFlashMenu = g_pGame->GetFlashMenu() )
	{
		if( searchSuccessful )
		{
			pFlashMenu->CurrentScreenCommand( "menu_friends_have_searchresults", NULL );
		}
		else
		{
			if(setNoNewFriendsFound)
			{
				pFlashMenu->CurrentScreenCommand( "menu_friends_no_new_friends_found", NULL );
			}
			else
			{
				pFlashMenu->CurrentScreenCommand( "menu_friends_search_fail", NULL );
			}
		}
	}
#endif //#ifdef USE_C2_FRONTEND
}

/*static*/ void CGameFriendsManager::ReadOnlineDataCallback(CryLobbyTaskID taskID, ECryLobbyError error, SCryLobbyUserData* pData, uint32 numData, void* pArg)
{
	CryLog("[GameLobby] ReadOnlineCallback %s with error %d", (error == eCLE_Success || error == eCLE_ReadDataNotWritten) ? "succeeded" : "failed", error);

	assert(0);
	/*
	CGameFriendsManager *pGfm = (CGameFriendsManager*)pArg;
	IPlayerProfileManager *pPlayerProfileManager = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager();

	CRY_ASSERT(pGfm);
	CRY_ASSERT(pPlayerProfileManager);	// only the dedicated server should be getting here

	if(pGfm)
	{
		bool validData = (error == eCLE_Success);

		int profileVersionIdx = 0;
		int statsVersionIdx = 0;

		if(validData)
		{
			if(pPlayerProfileManager)
			{
				profileVersionIdx = pPlayerProfileManager->GetOnlineAttributeIndexByName("OnlineAttributes/version");
				statsVersionIdx = pPlayerProfileManager->GetOnlineAttributeIndexByName("MP/PersistantStats/Version");

				if(!pPlayerProfileManager->ValidChecksums(pData, numData))
				{
					CryLog("  online attributes not valid, wrong checksums");
					validData = false;
				}

				if((profileVersionIdx >= 0) && (pData[profileVersionIdx].m_int32 != pPlayerProfileManager->GetOnlineAttributesVersion()))
				{
					CryLog("  player profile version mismatch");
					validData = false;
				}

				if((statsVersionIdx >= 0) && (pData[statsVersionIdx].m_int32 != CPersistantStats::GetInstance()->GetOnlineAttributesVersion()))
				{
					CryLog("  persistant stats version mismatch");
					validData = false;
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
			pGameLobby->SendPacket(eGUPD_StartOfGameUserStats, &data, pSessionName->m_conId);
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

				pGameLobby->SendPacket(eGUPD_StartOfGameUserStats, &data, pSessionName->m_conId);
			}
		}
	}
	else
	{
		CryLog("[GameLobby] Failed to find session name for user, not setting online attributes");
	}

	if(pGameLobby->OnlineAttributeTaskFinished(taskID, error))
	{
		CryLog("[GameLobby] send join game/lobby to user");

		// at least let the user continue for the time being
		if(data.Get()->m_state == eLS_Game)
		{
			pGameLobby->SendPacket(eGUPD_LobbyGameHasStarted, &data, pSessionName->m_conId);
		}
	}
	*/
}

//tmplt /*static*/ void Callback(CryLobbyTaskID taskID, ECryLobbyError error, void* pCbArg)
// {
// 	gfmassert(error == eCLE_Success);
// 	if( CGameFriendsManager* pGfm = (CGameFriendsManager*)pCbArg )
// 	{
// 		//...
// 	}
// 
// }


//.............................................................................................................
/// Event listeners
/*static*/ void CGameFriendsManager::ListenerFriendRequest(UCryLobbyEventData eventData, void *userParam)
{
	GFMDbgLog( "ListenerFriendRequest" );

	assert(userParam);
	if( CGameFriendsManager* pGfm = static_cast<CGameFriendsManager*>(userParam) )
	{
		CryUserID userId = eventData.pFriendIDData->m_user;

		// Creates one if we don't have one.
		bool doesntAlreadyExist = true;
		SGameFriendEventCache* pMateCache = pGfm->GetCreateFriendCacheByCryUser( userId, &doesntAlreadyExist );

		GFMDbgLogAlwaysCond( !pMateCache,  "!  Got a friend request for a brother who is already your friend, according to CGameFriendsManager.");
		GFMDbgLogAlwaysCond( !pGfm->m_haveFriendsList,  "!  Got a friend request before friends list recieved.");

		assert(!pMateCache->m_friendDeletePending);
		assert(!pMateCache->m_friendRequestPending);
		if(doesntAlreadyExist && pMateCache && !pMateCache->m_friendDeletePending && !pMateCache->m_friendRequestPending)
		{
			pMateCache->m_wantsToBeYourFriend = true;

			ICryFriendsManagement* pCryFriendsMgmt = gEnv->pNetwork->GetLobby()->GetFriendsManagement();
			uint32 userIndex = GetCurrentUserIndex();

			if(pCryFriendsMgmt)
			{
				ECryLobbyError error = pCryFriendsMgmt->FriendsManagementGetName( userIndex, &userId, 1, NULL, FriendRequestEventGetNameCallback, pGfm );
				if( error == eCLE_Success )
				{
					pGfm->ScheduleDisplayListUpdate();
				}
			}
		}
	}
}

/*static*/ void CGameFriendsManager::ListenerFriendMessage(UCryLobbyEventData eventData, void *userParam)
{
	GFMDbgLog( "ListenerFriendMessage" );

	assert(userParam);
	if( CGameFriendsManager* pGfm = static_cast<CGameFriendsManager*>(userParam) )
	{
		CryUserID userId = eventData.pFriendMesssageData->m_user;

		// Creates one if we don't have one.
		SGameFriendEventCache* pMate = pGfm->GetCreateFriendCacheByCryUser( userId );

		assert( pMate );
		GFMDbgLogAlwaysCond( (pGfm->m_haveFriendsList && !pGfm->GetFriendDataByCryUser(userId)),  "!  Got a message from brother who is not your friend, you won't see this! message is '%s'", eventData.pFriendMesssageData->m_message);
		GFMDbgLogAlwaysCond( !pGfm->m_haveFriendsList,  "!  Got a friend request before friends list recieved.");

		pMate->m_messageFromUser = eventData.pFriendMesssageData->m_message;
		pGfm->UpdateUsersRow(pMate->m_internalId);
	}
}

/*static*/ void CGameFriendsManager::ListenerFriendAuthorised(UCryLobbyEventData eventData, void *userParam)
{
	GFMDbgLog( "ListenerFriendAuthorised" );

	assert(userParam);
	if( CGameFriendsManager* pGfm = static_cast<CGameFriendsManager*>(userParam) )
	{
		CryUserID userId = eventData.pFriendIDData->m_user;

		// Creates one if we don't have one.
		SGameFriendEventCache* pMate = pGfm->GetCreateFriendCacheByCryUser( userId );

		assert( pMate );
		if(pMate)
		{
			if( pGfm->m_haveFriendsList )
			{
				GFMDbgLogAlways( "Got a friend-authorisation" );
				TFriendsList::iterator user = pGfm->GetFriendRequestCacheIteratorByInternalId(pMate->m_internalId);
				if(user!=pGfm->m_friendRequests.end())
				{
					pGfm->m_friendRequests.erase(user);
				}
				pGfm->ScheduleFriendsListUpdate();
			}
			else
			{
				GFMDbgLogAlways( "Have a friend authorised but don't have a friends list, expecting this user to be in the friends list when it arrives!" );
			}
		}
		else
		{
			GFMDbgLogAlways( "Have a friend authorised but couldn't find any friend data!" );
		}
	}
}

/*static*/ void CGameFriendsManager::ListenerFriendRevoked(UCryLobbyEventData eventData, void *userParam)
{
	GFMDbgLog( "ListenerFriendRevoked" );

	assert(userParam);
	if( CGameFriendsManager* pGfm = static_cast<CGameFriendsManager*>(userParam) )
	{
		CryUserID userId = eventData.pFriendIDData->m_user;

		// Creates one if we don't have one.
		SGameFriendEventCache* pMate = pGfm->GetCreateFriendCacheByCryUser( userId );

		assert( pMate );
		if( pGfm->m_haveFriendsList )
		{
			GFMDbgLogAlways( "Got a friend-revoke" );
			pGfm->RemoveFriend( userId );
			pGfm->ScheduleFriendsListUpdate();
		}
		else
		{
			GFMDbgLogAlways( "Got a friend-revoke but don't have a friends list, expecting this user to NOT be in the friends list when it arrives!" );
		}
	}
}

/*static*/ void CGameFriendsManager::ListenerMatchInvite(UCryLobbyEventData eventData, void *userParam)
{
	GFMDbgLog( "ListenerMatchInvite" );

	assert(userParam);
	if( CGameFriendsManager* pGfm = static_cast<CGameFriendsManager*>(userParam) )
	{
		GFMDbgLogAlways( "Got a match-invite" );

		CryUserID userId = eventData.pFriendMesssageData->m_user;

		// Creates one if we don't have one.
		SGameFriendEventCache* pMate = pGfm->GetCreateFriendCacheByCryUser( userId );

		assert( pMate );

		GFMDbgLogAlwaysCond( !pGfm->m_haveFriendsList, "Got a match invite when we don't have the friends list yet!" );
		pMate->m_isInvitingToMatch = true;
		pMate->m_isInvitingToMatchConsumed=false;

		pGfm->UpdateUsersRow(pMate->m_internalId);
	}
}

/*static*/ void CGameFriendsManager::ListenerFriendOnlineStatus(UCryLobbyEventData eventData, void *userParam)
{
	GFMDbgLog( "ListenerFriendOnlineStatus" );
	assert(userParam);
	if( CGameFriendsManager* pGfm = static_cast<CGameFriendsManager*>(userParam) )
	{
		CryUserID userId = eventData.pFriendOnlineStateData->m_user;
		EOnlineState onlineState = eventData.pFriendOnlineStateData->m_curState;

		// Creates one if we don't have one.
		SGameFriendEventCache* pMate = pGfm->GetCreateFriendCacheByCryUser( userId );

		assert( pMate );
		GFMDbgLogAlwaysCond( !(pGfm->m_haveFriendsList && pGfm->GetFriendDataByCryUser(userId)),  "!  Got a signed in message for a brother who isn't in your friend list.");
		if(!(pGfm->m_haveFriendsList && pGfm->GetFriendDataByCryUser(userId)))
		{
			pGfm->ScheduleFriendsListUpdate(); // the friends list needs to be updated.
		}

		switch( onlineState )
		{
		case eOS_SignedOut:
			pMate->m_isOnline = false;
			break;
		case eOS_SignedIn:
			pMate->m_isOnline = true;
			break;
		case eOS_SigningIn:
			// don't care.
			break;
		case eOS_Unknown: // fall through intentional
		default :
			GameWarning("CGameFriendsManager: Unknown user online status");
			break;
		}
		pGfm->UpdateUsersRow(pMate->m_internalId);
	}
}

/*static*/ void CGameFriendsManager::ListenerOnlineState( UCryLobbyEventData eventData, void* pUserParam )
{
	CGameFriendsManager*	pThis = static_cast< CGameFriendsManager* >( pUserParam );

	if ( eventData.pOnlineStateData->m_curState == eOS_SignedOut )
	{
		pThis->m_friends.clear();
		pThis->m_friendEventCache.clear();
		pThis->m_friendRequests.clear();
		pThis->m_potentialFriends.clear();
	}
}

const bool CGameFriendsManager::GetNewInvite(TLobbyUserNameString &outName)
{
	bool newInviteFound=false;

	TFriendsList::iterator it = m_friends.begin();
	const TFriendsList::const_iterator end = m_friends.end();
	for(; it!=end; it++)
	{
		SGameFriend& fi = *it;
		if (fi.m_isInvitingToMatch && !fi.m_isInvitingToMatchConsumed)
		{
			newInviteFound=true;
			fi.m_isInvitingToMatchConsumed=true;
			outName = fi.m_name; // Message as well?
			break;
		}
	}

	return newInviteFound;
}

void CGameFriendsManager::ScheduleFriendsListUpdate( )
{
	m_scheduleAFriendListUpdate=true;
}

void CGameFriendsManager::ScheduleDisplayListUpdate( )
{
	m_scheduleADisplayListUpdate=true;
}

#ifdef USE_C2_FRONTEND
const bool CGameFriendsManager::IsOnValidFriendsScreen(CFlashFrontEnd *pFlashMenu, CFriendsOverviewWidget* pFriendsWidget) const
{
	const EFlashFrontEndScreens screen = pFlashMenu->GetCurrentMenuScreen();
	switch(screen)
	{
	case eFFES_friends_menus:
		return true;
	}

	return pFriendsWidget ? pFriendsWidget->ShouldBeSeen() : false;
}

void CGameFriendsManager::RefreshCurrentScreen(CFlashFrontEnd *pFlashMenu)
{
	if(pFlashMenu && IsOnValidFriendsScreen(pFlashMenu, pFlashMenu->GetFriendInfo()))
	{
		pFlashMenu->RefreshCurrentScreen();
	}
}
#endif #ifdef USE_C2_FRONTEND


void CGameFriendsManager::CancelSearchTask()
{
	if (m_searchTaskID != CryLobbyInvalidTaskID)
	{
		ICryLobby* pLobby = gEnv->pNetwork->GetLobby();
		assert( pLobby );
		if( pLobby )
		{
			ICryFriendsManagement* pFriendsManager = pLobby->GetFriendsManagement();
			assert(pFriendsManager);
			if( pFriendsManager )
			{
				pFriendsManager->CancelTask(m_searchTaskID);
			}
		}
	}

	m_searchTaskID = CryLobbyInvalidTaskID;
	m_searchQueryInProgress = false;
}

#if GFM_ENABLE_EXTRA_DEBUG

void CGameFriendsManager::PopulateWithDummyData()
{
	const int existingNumberOfFriends = m_friends.size();
	const int numberOfNewFriendsToAdd = GFM_FRIENDSMENU_MAXFRIENDS - existingNumberOfFriends;

	string name;
	for(int i=0; i<numberOfNewFriendsToAdd; ++i)
	{
		SGameFriend friendInfo;

		if(i%4==0)
		{
			friendInfo.m_name.Format("WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW");
		}
		else
		{
			friendInfo.m_name.Format("DummyPlayerGamespyFriend%00000d", i);
		}

		const int whichState = i%8;
		switch( whichState )
		{
		case 0:
			// do nothing.
			break;
		case 1:
			friendInfo.m_isOnline = true;
			break;
		case 2:
			friendInfo.m_isInvitingToMatch = true;
			friendInfo.m_isOnline = true;
			break;
		case 3:
			friendInfo.m_hasBeenInvitedToMatch = true;
			friendInfo.m_isOnline = true;
			break;
		case 4:
			{
				static bool online = false;
				online = !online;
				friendInfo.m_friendRequestPending = online;
			}
			break;
		case 5:
			{
				static bool online = false;
				online = !online;
				friendInfo.m_wantsToBeYourFriend = online;
			}
			break;
		case 6:
			{
				static bool online = false;
				online = !online;
				friendInfo.m_friendDeletePending = online;
			}
			break;
		case 7:
			//randomise
			friendInfo.m_isOnline = (rand()%2==0);
			friendInfo.m_isInvitingToMatch = (rand()%2==0);
			friendInfo.m_hasBeenInvitedToMatch = (rand()%2==0);
			friendInfo.m_friendRequestPending = (rand()%2==0);
			friendInfo.m_wantsToBeYourFriend = (rand()%2==0);
			friendInfo.m_friendDeletePending = (rand()%2==0);
			break;
		}

		if( friendInfo.m_isOnline )
		{
			switch((rand()%3))
			{
			case 0: friendInfo.m_richPres = "a dummy player is online"; break;
			case 1: friendInfo.m_richPres = "a dummy player is pretending to be in a lobby"; break;
			case 2: friendInfo.m_richPres = "Not playing TIA on Pier17"; break;
			}
		}
		else
		{
			friendInfo.m_richPres = "a dummy player is offline";
		}

		CPlayerProgression* pProgression = CPlayerProgression::GetInstance();
		if(pProgression)
		{
			friendInfo.m_rankId = rand()%pProgression->GetMaxRanks();
			friendInfo.m_rankId = rand()%pProgression->GetMaxReincarnations();
		}

		// Copy in the new data.
		friendInfo.m_userId = 0;
		friendInfo.m_isDebug = true;

		// put the new data into the vector of mates, note this will increment the internal-Id.
		m_friends.push_back(friendInfo);
		assert( m_friends.size() <= GFM_FRIENDSMENU_MAXFRIENDS );
	}
#ifdef USE_C2_FRONTEND
	RefreshCurrentScreen(g_pGame->GetFlashMenu());
#endif //#ifdef USE_C2_FRONTEND
}

void CGameFriendsManager::ClearDummyData()
{
	const TFriendsList::const_iterator end = m_friends.end();

	bool foundDebugUser = false;
	do
	{
		foundDebugUser = false;
		TFriendsList::iterator it = m_friends.begin();
		const TFriendsList::const_iterator end = m_friends.end();
		for(; it!=end; it++)
		{
			SGameFriend* myFriend = &(*it);
			if(myFriend->m_isDebug)
			{
				foundDebugUser = true;
				m_friends.erase(it);
				break;
			}
		}
	}
	while(foundDebugUser);
#ifdef USE_C2_FRONTEND
	RefreshCurrentScreen(g_pGame->GetFlashMenu());
#endif //#ifdef USE_C2_FRONTEND
}

/*static*/ void CGameFriendsManager::CMDPopulateWithDummyData(IConsoleCmdArgs *pArgs)
{
	const int numArgs = pArgs->GetArgCount();
	bool on = true;
	if(numArgs >= 2)
	{
		on = (0!=atoi(pArgs->GetArg(1)));
	}

	if( on )
	{
		if(CGameFriendsManager* pGfm = g_pGame->GetGameFriendsManager() )
		{
			pGfm->PopulateWithDummyData();
		}
	}
	else
	{
		if(CGameFriendsManager* pGfm = g_pGame->GetGameFriendsManager() )
		{
			pGfm->ClearDummyData();
		}
	}
}

#endif // GFM_ENABLE_EXTRA_DEBUG

// play nice with selotaped compiling
#undef GFMDbgLog
#undef GFMDbgLogAlways
#undef GFMDbgLogCond
#undef GFMDbgLogAlwaysCond

#endif IMPLEMENT_PC_BLADES
