#ifndef __CGAMEFRIENDDATA_H__
#define __CGAMEFRIENDDATA_H__

//---------------------------------------------------------------------------

#include "Network/GameNetworkDefines.h"
#if IMPLEMENT_PC_BLADES

//---------------------------------------------------------------------------

typedef CryFixedStringT<LOBBY_MESSAGE_SIZE> TGameFriendMessageString;

typedef int32 TGameFriendId;
const TGameFriendId INVALID_FRIEND_ID = -1;

typedef CryFixedStringT<CRYLOBBY_USER_NAME_LENGTH>  TLobbyUserNameString;

//---------------------------------------------------------------------------

struct SGameFriendEventCache
{
	SGameFriendEventCache()
		: m_messageFromUser("")
		, m_richPres()
		, m_userId(0)
		, m_internalId(__s_internal_ids++)
		, m_rankId(0)
		, m_reincarnations(0)
		, m_isOnline(false)
		, m_isInvitingToMatch(false)
		, m_isInvitingToMatchConsumed(false)
		, m_hasBeenInvitedToMatch(false)
		, m_taggedForBatchMatchInvite(false)
		, m_friendRequestPending(false)
		, m_wantsToBeYourFriend(false)
		, m_friendDeletePending(false)
	{ /*-*/	}

	virtual SGameFriendEventCache& operator=( const SGameFriendEventCache& _in )
	{
		m_messageFromUser = _in.m_messageFromUser;
		m_richPres = _in.m_richPres;
		m_userId = _in.m_userId;
		m_internalId = _in.m_internalId;
		m_rankId = _in.m_rankId;
		m_reincarnations = _in.m_reincarnations;
		m_isOnline = _in.m_isOnline;
		m_isInvitingToMatch = _in.m_isInvitingToMatch;
		m_isInvitingToMatchConsumed = _in.m_isInvitingToMatchConsumed;
		m_hasBeenInvitedToMatch = _in.m_hasBeenInvitedToMatch;
		m_taggedForBatchMatchInvite = _in.m_taggedForBatchMatchInvite;
		m_friendRequestPending = _in.m_friendRequestPending;
		m_wantsToBeYourFriend = _in.m_wantsToBeYourFriend;
		m_friendDeletePending = _in.m_friendDeletePending;
		return *this;
	}

private :

	static TGameFriendId __s_internal_ids;

public :

	TGameFriendMessageString m_messageFromUser;           // just one, atm.
	TGameFriendMessageString m_sessionConnection;         
	TGameFriendMessageString m_richPres;                  
	CryUserID      m_userId;
	TGameFriendId  m_internalId;
	int            m_rankId;
	int            m_reincarnations;
	bool           m_isOnline;                  // is the remote player online.
	bool           m_isInvitingToMatch;         // the remote player has sent the local player an invite to a game
	bool           m_isInvitingToMatchConsumed; // the remote player has sent the local player an invite to a game and the local player has been informed about this new invite
	bool           m_hasBeenInvitedToMatch;     // the remote user has been sent and an invite
	bool           m_taggedForBatchMatchInvite; // checkbox is marked for batch invites
	bool           m_friendRequestPending;      // has sent you a friends request.
	bool           m_wantsToBeYourFriend;       // has sent you a friends request.
	bool           m_friendDeletePending;       // you are removing them from the friends list.
};

//---------------------------------------------------------------------------

// We only have a valid one of these for a CryUserId when they are
// populated in the Friends list call back.
struct SGameFriend : SGameFriendEventCache
{
	SGameFriend()
		: m_name("")
		, m_havename(false)
#		if GFM_ENABLE_EXTRA_DEBUG
		, m_isDebug(false)
#		endif //GFM_ENABLE_EXTRA_DEBUG
		, SGameFriendEventCache()
	{ /*-*/	}

	virtual SGameFriend& operator=( const SGameFriendEventCache& _in )
	{
		SGameFriendEventCache::operator=( _in );
		m_name = "";
#   if GFM_ENABLE_EXTRA_DEBUG
		m_isDebug = false;
#   endif //GFM_ENABLE_EXTRA_DEBUG
		return *this;
	}

	static bool CompareFriendsAlphaCaseInsensitive(const SGameFriend &a, const SGameFriend &b)
	{
		TLobbyUserNameString s1 = a.m_name;
		TLobbyUserNameString s2 = b.m_name;

		const int len = min(s1.length(), s2.length());

		s1.MakeLower();
		s2.MakeLower();

		const char* const ps1 = s1.c_str();
		const char* const ps2 = s2.c_str();

		for(int i=0; i<len; i++)
		{
			char a = ps1[i];
			char b = ps2[i];
			if(a!=b)
			{
				return (a<b);
			}				
		}
		return true;
	}


public :
	TLobbyUserNameString m_name;
	bool                 m_havename;
#if GFM_ENABLE_EXTRA_DEBUG
	bool                 m_isDebug;
#endif //GFM_ENABLE_EXTRA_DEBUG
};

//---------------------------------------------------------------------------

#endif // IMPLEMENT_PC_BLADES

//---------------------------------------------------------------------------

#endif //__CGAMEFRIENDDATA_H__
