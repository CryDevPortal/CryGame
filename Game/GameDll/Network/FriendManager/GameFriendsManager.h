#ifndef __CGAMEFRIENDSMANAGER_H__
#define __CGAMEFRIENDSMANAGER_H__

//---------------------------------------------------------------------------
#include "Network/GameNetworkDefines.h"
#if IMPLEMENT_PC_BLADES

//---------------------------------------------------------------------------

#include "Network/FriendManager/GameFriendData.h"

#include <IGameFramework.h>
#include <ICryFriends.h>
#include <ICryFriendsManagement.h>

//---------------------------------------------------------------------------

#define GFM_FRIENDSMENU_MAXFRIENDS 124
#define GFM_NUMBER_OF_FRIENDS_IN_RESULT 1
#define GFM_QUOTE(a) #a

//---------------------------------------------------------------------------

class CFriendsListDisplayManager;
class CFriendsOverviewWidget;

//---------------------------------------------------------------------------

// A class to manage and store invites from the game side
class CGameFriendsManager
	: public IGameFrameworkListener
	, public IGameWarningsListener
{

public :

	struct SPotentialFriendData
	{
		SPotentialFriendData()
			: m_name("")
			, m_cryUserId(CryUserInvalidID)
			, m_internalId(__s_internal_id_count++)
		{ /*-*/	}

		static uint32 __s_internal_id_count;

		string     m_name;
		CryUserID  m_cryUserId;
		uint32     m_internalId;
	};

	//typedef CryFixedArray<SGameFriend, GFM_FRIENDSMENU_MAXFRIENDS> TFriendsList;  need erase
	typedef std::vector<SGameFriend>     TFriendsList;
	typedef std::vector<SGameFriendEventCache> TFriendsListEventCache;
	typedef std::vector<SPotentialFriendData> TPotentialFriendsArray;

	typedef CryFixedStringT<CRYLOBBY_USER_NAME_LENGTH> TFriendSearchString;


public :

	// ctor/dtor
	CGameFriendsManager();
	virtual ~CGameFriendsManager();
	//~ctor/dtor

public :

	// Action Triggers
	void UpdateFriendsList();
	void UpdateFriendsRichPresence(const float dtime);
	bool StartFriendSearch( const char* searchStr );
	void SendFriendRequest( /*const*/ CryUserID& userId, const char* name );
	void AcceptFriendRequest( const TGameFriendId userId );
	void DeclineFriendRequest( const TGameFriendId userId );
	void SendMatchInvite( const TGameFriendId userId );
	void AcceptMatchInvite( const TGameFriendId userId );
	void DeclineMatchInvite( const TGameFriendId userId );
	void RequestRemoveFriend( const TGameFriendId userId );
	void RequestUserData( const TGameFriendId userId );
	void JoinUsersGame( const TGameFriendId userId );
	// ~Action Trigger

	const bool IsSignedIn() const { return m_signedIn; }
	const bool GetNewInvite(TLobbyUserNameString &outName);
	void       ScheduleFriendsListUpdate();
	void       ScheduleDisplayListUpdate();

	// friend-info
	const bool                    HaveFriends() const { return (m_friends.size() > 0); }
	const bool                    HaveFriendRequests() const { return (m_friendRequests.size() > 0); }
	SGameFriend*          GetFriendDataByCryUser( const CryUserID& userId );
	SGameFriend*          GetFriendDataByInternalId( const TGameFriendId userId );
	SGameFriend*          GetAnyFriendByInternalId( const TGameFriendId userId );
	const TFriendsList&           GetFriendsInfo() const { return m_friends; }
	const TFriendsList&           GetFriendRequestsInfo() const { return m_friendRequests; }
	SGameFriend*          GetFriendRequestCacheByInternalId( const TGameFriendId userId );
	//~friend-info

	// current user of interest
	void                          SetCurrentUserOfInterest( const TGameFriendId userId, const int row=-1 ) { m_currentUserOfInterest = userId; m_currentUserOfInterestsRow = row; }
	const TGameFriendId           GetCurrentUserOfInterest() const { return m_currentUserOfInterest; }
	const int                     GetCurrentUserOfInterestsRow() const { return m_currentUserOfInterestsRow; }
	// current user of interest

	// searching
	const TFriendSearchString&    GetFriendSearchString( ) const { return m_searchString; }
	const TPotentialFriendsArray& GetFriendSearchResults( ) const { return m_potentialFriends; }
	TPotentialFriendsArray&       GetFriendSearchResults( ){ return m_potentialFriends; }
	const bool                    IsSearchTaskActive() const { return m_searchQueryInProgress; }
	void													CancelSearchTask();
	//~searching

	// display
	void                          UpdateDisplayList();
#ifdef USE_C2_FRONTEND
	CFriendsListDisplayManager*   GetFriendListDisplayManager(){ return m_pDisplayList; }
#endif //#ifdef USE_C2_FRONTEND
	//~display

private :

	SGameFriend*                   AddUser( const SFriendInfo& inFriendInfo );
	void                                   AddUsers( const SFriendInfo* pFriendInfo, const int size );
	void                                   RemoveFriend( const CryUserID& userId );
	//const char*                            GetRichPresenceTitle( const CryUserID& userId );
	//const char*                            GetRichPresenceText( const CryUserID& userId );

	TFriendsList::iterator                 GetFriendDataIteratorByCryUser( const CryUserID& userId );
	TFriendsListEventCache::iterator       GetFriendCacheIteratorByCryUser( const CryUserID& userId );
	SGameFriendEventCache*              GetFriendCacheByCryUser( const CryUserID& userId );
	SGameFriendEventCache*              GetCreateFriendCacheByCryUser( const CryUserID& userId, bool* pOut_WasCreated=NULL );
	//SGameFriend*                           GetFriendCacheInternalId( const TGameFriendId userId );

	TFriendsList::iterator                 GetFriendRequestCacheIteratorByCryUser( const CryUserID& userId );
	SGameFriend*                   GetFriendRequestCacheByCryUser( const CryUserID& userId );
	TFriendsList::iterator                 GetFriendRequestCacheIteratorByInternalId( const TGameFriendId userId );
	SGameFriend*                   GetCreateFriendRequestCacheByCryUser( const CryUserID& userId, const char* name );

	void                                   UpdateUsersRow(const TGameFriendId internalId);

	// Callbacks
	static void GetFriendsListCallback(CryLobbyTaskID taskID, ECryLobbyError error, SFriendInfo* pFriendInfo, uint32 numFriends, void* pArg);
	static void SendFriendRequestCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pCbArg);
	static void AcceptFriendRequestCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pCbArg);
	static void DeclineFriendRequestCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pCbArg);
	static void SendMatchInviteCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pCbArg);
	static void AcceptMatchInviteCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pCbArg);
	// void DeclineMatchInviteCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pCbArg); -- No ICryFriendsManagement interface
	static void RemoveFriendCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pCbArg);
	static void FriendRequestEventGetNameCallback(CryLobbyTaskID taskID, ECryLobbyError error, SFriendInfo* pInfo, uint32 numUserIDs, void* pCbArg);
	static void FriendRequestRichPresCallback(CryLobbyTaskID taskID, ECryLobbyError error, SFriendStatusInfo* pInfo, uint32 numUserIDs, void* pCbArg);
	static void FriendsSearchCallback(CryLobbyTaskID taskID, ECryLobbyError error, SFriendInfo* pInfo, uint32 numUserIDs, void* pCbArg);
	static void ReadOnlineDataCallback(CryLobbyTaskID taskID, ECryLobbyError error, SCryLobbyUserData* pData, uint32 numData, void* pArg);
	//tmplt void Callback(CryLobbyTaskID taskID, ECryLobbyError error, void* pCbArg);
	//~Callbacks


	// Event listeners
	static void ListenerFriendRequest(UCryLobbyEventData eventData, void *userParam);
	static void ListenerFriendMessage(UCryLobbyEventData eventData, void *userParam);
	static void ListenerFriendAuthorised(UCryLobbyEventData eventData, void *userParam);
	static void ListenerFriendRevoked(UCryLobbyEventData eventData, void *userParam);
	static void ListenerMatchInvite(UCryLobbyEventData eventData, void *userParam);
	static void ListenerFriendOnlineStatus(UCryLobbyEventData eventData, void *userParam);
	static void ListenerOnlineState(UCryLobbyEventData eventData, void *pUserParam);
	//~Event listeners

	// IGameFrameworkListener
	virtual void OnPostUpdate(float fDeltaTime);
	virtual void OnSaveGame(ISaveGame* pSaveGame) {}
	virtual void OnLoadGame(ILoadGame* pLoadGame) {}
	virtual void OnLevelEnd(const char* nextLevel) {}
	virtual void OnActionEvent(const SActionEvent& event) {}
	//~IGameFrameworkListener
	
	// IGameWarningsListener
	virtual bool OnWarningReturn(THUDWarningId id, const char* returnValue);
	//virtual void OnWarningRemoved(THUDWarningId id) {}
	//~IGameWarningsListener

	// Utils
	static uint32 GetCurrentUserIndex();
#ifdef USE_C2_FRONTEND
	const bool IsOnValidFriendsScreen(CFlashFrontEnd *pFlashMenu, CFriendsOverviewWidget* pFriendsWidget) const;
	void RefreshCurrentScreen(CFlashFrontEnd *pFlashMenu);
	void UpdateUsersRow(const TGameFriendId internalId, CFlashFrontEnd* pFlashMenu);
#endif //#ifdef USE_C2_FRONTEND

#if GFM_ENABLE_EXTRA_DEBUG
	void PopulateWithDummyData();
	void ClearDummyData();
	static void CMDPopulateWithDummyData(IConsoleCmdArgs *pArgs);
#endif // GFM_ENABLE_EXTRA_DEBUG

private :

#ifdef USE_C2_FRONTEND
	CFriendsListDisplayManager*  m_pDisplayList; // should probably go somewhere else
#endif //#ifdef USE_C2_FRONTEND

	TFriendsList           m_friends;
	TFriendsListEventCache m_friendEventCache;
	TFriendsList           m_friendRequests;
	TPotentialFriendsArray m_potentialFriends;
	TFriendSearchString    m_searchString;
	CryLobbyTaskID         m_searchTaskID; // Only 1 search at a time. Required to clear task if early exiting search
	CryLobbyTaskID         m_richPresenceTaskId;
	TGameFriendId          m_currentUserOfInterest;
	int                    m_currentUserOfInterestsRow;
	CryUserID              m_localUserId;
	CryUserID              m_userBeingAdded;
	float                  m_richPresenceTimeOut;
	bool                   m_queryInProgress;
	bool                   m_signedIn;
	bool                   m_scheduleAFriendListUpdate;
	bool                   m_scheduleADisplayListUpdate;
	bool                   m_haveFriendsList;
	bool                   m_searchQueryInProgress;
};

//---------------------------------------------------------------------------

#endif // IMPLEMENT_PC_BLADES

//---------------------------------------------------------------------------
#endif //__CGAMEFRIENDSMANAGER_H__
