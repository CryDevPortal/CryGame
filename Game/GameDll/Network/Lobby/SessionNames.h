/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Struct for keeping track of names in sessions
-------------------------------------------------------------------------
History:
- 15:03:2010 : Created By Ben Parbury

*************************************************************************/
#include "Utility/StringUtils.h"
#include <CryFixedArray.h>

#include <IPlayerProfiles.h>

#include "ICryLobby.h"
#include "ICryMatchMaking.h"

#pragma once

#define CLAN_TAG_LENGTH		(5)
#define DISPLAY_NAME_LENGTH (CRYLOBBY_USER_NAME_LENGTH + CLAN_TAG_LENGTH + 1)	// +1 for space
#define MAX_ONLINE_STATS_SIZE (1500)
#define MAX_SESSION_NAMES (MAX_PLAYER_LIMIT)

enum ELocalUserData
{
	eLUD_Rank = 0,
	eLUD_DogtagStyle = 1,	//first 4 bits are style, last 4 bits are special style
	eLUD_DogtagId = 2,
	eLUD_SquadId1 = 3,
	eLUD_SquadId2 = 4,
	eLUD_SkillRank1 = 5,
	eLUD_SkillRank2 = 6,
	eLUD_ClanTag1 = 7,
	eLUD_ClanTag2 = 8,
	eLUD_ClanTag3 = 9,
	eLUD_ClanTag4 = 10,
	eLUD_LoadedDLCs = 11,
	eLUD_ReincarnationsAndVoteChoice = 12,	// first 4 bits Reincarnation count, last 4 bits vote choice
};

enum EOnlineAttributeStatus
{
	eOAS_Uninitialised = 0,					// data has not yet been retrieved
	eOAS_Initialised,								// data has been rerieved
	eOAS_WaitingForUpdate,					// waiting for update from the client, used at end of game
	eOAS_Updating,									// for when a user is leaving
	eOAS_UpdateComplete,						// the data is complete
};

struct SSessionNames
{
	struct SSessionName
	{
		static const int MUTE_REASON_WRONG_TEAM			= (1 << 0);
		static const int MUTE_REASON_MANUAL					= (1 << 1);
		static const int MUTE_REASON_NOT_IN_SQUAD		= (1 << 2);
		static const int MUTE_REASON_MUTE_ALL				= (1 << 3);

		static const int MUTE_PLAYER_AUTOMUTE_REASONS = (MUTE_REASON_NOT_IN_SQUAD | MUTE_REASON_MUTE_ALL); // Reasons controlled by the automute

		SSessionName(CryUserID userId, const SCryMatchMakingConnectionUID &conId, const char* name, const uint8* userData, bool isDedicated);
		void Set(CryUserID userId, const SCryMatchMakingConnectionUID &conId, const char* name, const uint8* userData, bool isDedicated);		
		void SetUserData(const uint8 *userData);
		void GetDisplayName(CryFixedStringT<DISPLAY_NAME_LENGTH> &displayName) const;
		void GetClanTagName(CryFixedStringT<CLAN_TAG_LENGTH> &name) const;
		uint8 GetReincarnations() const;
		void SetOnlineData(SCryLobbyUserData* pData, uint32 dataCount);
		uint16 GetSkillRank() const;

		//---------------------------------------
		CryUserID m_userId;
#if INCLUDE_DEDICATED_LEADERBOARDS
		SCryLobbyUserData m_onlineData[MAX_ONLINE_STATS_SIZE];	// this needs to use the player profile define
		uint32 m_onlineDataCount; // number of items in m_onlineData
		uint32 m_recvOnlineDataCount;
		EOnlineAttributeStatus m_onlineStatus;
		uint8 m_numGetOnlineDataRetries;
#endif
		float m_timeWithoutConnection;
		SCryMatchMakingConnectionUID m_conId;
		char m_name[CRYLOBBY_USER_NAME_LENGTH];
		uint8	m_userData[CRYLOBBY_USER_DATA_SIZE_IN_BYTES];
		uint8 m_teamId;
		uint8 m_muted;
		bool m_bMustLeaveBeforeServer;
		bool m_isDedicated;
		bool m_bFullyConnected;		// Server only flag used by the GameLobby to determine if players have finished identifying themselves
	};

	const static int k_unableToFind = -1;
	CryFixedArray<SSessionName, MAX_SESSION_NAMES> m_sessionNames;
	bool m_dirty;		// So the UI knows it needs to be updated

	SSessionNames();
	unsigned int Size() const;
	void Clear();
	int Find(const SCryMatchMakingConnectionUID &conId) const;
	int FindByUserId(const CryUserID &userId) const;
	int FindIgnoringSID(const SCryMatchMakingConnectionUID &conId) const;
	SSessionNames::SSessionName* GetSessionName(const SCryMatchMakingConnectionUID &conId, bool ignoreSID);
	SSessionNames::SSessionName* GetSessionNameByUserId(const CryUserID &userId);
	const SSessionNames::SSessionName* GetSessionNameByUserId(const CryUserID &userId) const;
	int Insert(CryUserID userId, const SCryMatchMakingConnectionUID &conId, const char* name, const uint8* userData, bool isDedicated);
	void Remove(const SCryMatchMakingConnectionUID &conId);
	void Update(CryUserID userId, const SCryMatchMakingConnectionUID &conId, const char* name, const uint8* userData, bool isDedicated, bool findByUserId);
	void RemoveBlankEntries();
	void Tick(float dt);
	bool RemoveEntryWithInvalidConnection();
};
