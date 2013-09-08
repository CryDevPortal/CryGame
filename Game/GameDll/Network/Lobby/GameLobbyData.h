/******************************************************************************
** GameLobbyData.h
******************************************************************************/

#ifndef __GAMELOBBYDATA_H__
#define __GAMELOBBYDATA_H__

#include "AutoEnum.h"

//------------
// LOBBY DATA
//------------






































#if defined( PS3 ) && defined(GAME_IS_CRYSIS2)

// PS3 Note : There are only 8 uint32 attribute fields on PSN, everything else must be packed into 1 of 2 binary attribute fields which
//						are 256 bytes each. If filtering is to be required make sure items you want to filter by are in the 8 uint32 fields.
//						To put it another way, the first 8 eCSUDT_Int32 values registered will be allocated to the 8 searchable uint32 fields,
//						all other data will be placed into binary data and not be able to be filtered!


// Ids
enum ELOBBYIDS
{
	LID_MATCHDATA_GAMEMODE = 0,
	LID_MATCHDATA_MAP,
	LID_MATCHDATA_ACTIVE,
	LID_MATCHDATA_VERSION,
	LID_MATCHDATA_REQUIRED_DLCS,
	LID_MATCHDATA_PLAYLIST,
	LID_MATCHDATA_VARIANT,
	LID_MATCHDATA_SKILL,
	LID_MATCHDATA_LANGUAGE,
	LID_MATCHDATA_COUNTRY,
};

#define REQUIRED_SESSIONS_QUERY	0
#define FIND_GAME_SESSION_QUERY 0
#define FIND_GAME_SESSION_QUERY_WC 0
#define REQUIRED_SESSIONS_SEARCH_PARAM	0

// Rich presence string
#define RICHPRESENCE_ID					0

// Types of rich presence we can have
#define RICHPRESENCE_GAMEPLAY		0        
#define RICHPRESENCE_LOBBY			1
#define RICHPRESENCE_FRONTEND		2
#define RICHPRESENCE_SINGLEPLAYER	3
#define RICHPRESENCE_IDLE			4
#define RICHPRESENCE_SHOWROOM		5

// Rich presence string params
#define RICHPRESENCE_GAMEMODES	0
#define RICHPRESENCE_MAPS				1

// These string ids need to match those found in Scripts/Network/RichPresence.xml

// Rich presence strings, game mode
#define RICHPRESENCE_GAMEMODES_INSTANTACTION			0   
#define RICHPRESENCE_GAMEMODES_TEAMINSTANTACTION	1
#define RICHPRESENCE_GAMEMODES_ASSAULT						2          
#define RICHPRESENCE_GAMEMODES_CAPTURETHEFLAG			3
#define RICHPRESENCE_GAMEMODES_EXTRACTION					4
#define RICHPRESENCE_GAMEMODES_CRASHSITE					5
#define RICHPRESENCE_GAMEMODES_ALLORNOTHING				6
#define	RICHPRESENCE_GAMEMODES_BOMBTHEBASE				7
#define RICHPRESENCE_GAMEMODES_POWERSTRUGGLE			8

// Rich presence strings for maps
#define RICHPRESENCE_MAPS_ROOFTOPGARDENS			0
#define RICHPRESENCE_MAPS_PIER17              1
#define RICHPRESENCE_MAPS_WALLSTREET          2
#define RICHPRESENCE_MAPS_LIBERTYISLAND       3
#define RICHPRESENCE_MAPS_HARLEMGORGE         4
#define RICHPRESENCE_MAPS_COLLIDEDBUILDINGS   5
#define RICHPRESENCE_MAPS_ALIENCRASHTRAIL     6
#define RICHPRESENCE_MAPS_ALIENVESSEL        7
#define RICHPRESENCE_MAPS_BATTERYPARK         8
#define RICHPRESENCE_MAPS_BRYANTPARK          9
#define RICHPRESENCE_MAPS_CHURCH              10
#define RICHPRESENCE_MAPS_CONEYISLAND         11
#define RICHPRESENCE_MAPS_DOWNTOWN            12
#define RICHPRESENCE_MAPS_LIGHTHOUSE          13
#define RICHPRESENCE_MAPS_PARADE              14
#define RICHPRESENCE_MAPS_ROOSEVELT           15
#define RICHPRESENCE_MAPS_CITYHALL            16
#define RICHPRESENCE_MAPS_ALIENVESSELSMALL    17
#define RICHPRESENCE_MAPS_PIERSMALL           18
#define RICHPRESENCE_MAPS_LIBERTYISLAND_MIL   19
#define RICHPRESENCE_MAPS_TERMINAL            20
#define RICHPRESENCE_MAPS_DLC_1_MAP_1         21
#define RICHPRESENCE_MAPS_DLC_1_MAP_2         22
#define RICHPRESENCE_MAPS_DLC_1_MAP_3         23
#define RICHPRESENCE_MAPS_DLC_2_MAP_1         24
#define RICHPRESENCE_MAPS_DLC_2_MAP_2         25
#define RICHPRESENCE_MAPS_DLC_2_MAP_3         26
#define RICHPRESENCE_MAPS_DLC_3_MAP_1         27
#define RICHPRESENCE_MAPS_DLC_3_MAP_2         28
#define RICHPRESENCE_MAPS_DLC_3_MAP_3         29
#define RICHPRESENCE_MAPS_LIBERTYISLAND_STATUE 30

#elif defined(USE_CRYLOBBY_GAMESPY) && defined(GAME_IS_CRYSIS2)

// ELOBBYIDS that map to predefined GameSpy keys should be defined to be equal
// to one of the eCGSK_*Key constants between eCGSK_FirstGameSpyUsedKey and
// eCGSK_LastGameSpyUsedKey. Other ELOBBYIDS should have values in the range
// eCGSK_FirstGameReservedKey to eCGSK_LastGameReservedKey.

// Ids
enum ELOBBYIDS
{
	LID_MATCHDATA_GAMEMODE = eCGSK_FirstGameReservedKey,
	LID_MATCHDATA_MAP = eCGSK_FirstGameReservedKey + 1,
	LID_MATCHDATA_ACTIVE = eCGSK_FirstGameReservedKey + 2,
	LID_MATCHDATA_VERSION = eCGSK_FirstGameReservedKey + 3,
	LID_MATCHDATA_REQUIRED_DLCS = eCGSK_FirstGameReservedKey + 4,
	LID_MATCHDATA_PLAYLIST = eCGSK_FirstGameReservedKey + 5,
	LID_MATCHDATA_VARIANT = eCGSK_GameVariantKey,		// Maps to predefined GameSpy key
	LID_MATCHDATA_SKILL = eCGSK_SkillKey,		// Maps to predefined GameSpy key
	LID_MATCHDATA_LANGUAGE = eCGSK_FirstGameReservedKey + 6,
	LID_MATCHDATA_OFFICIAL = eCGSK_FirstGameReservedKey + 7,
	LID_MATCHDATA_REGION = eCGSK_RegionKey,		// Maps to predefined GameSpy key
	LID_MATCHDATA_FAVOURITE_ID = eCGSK_HostProfileKey		// Maps to optional CryNetwork key
};

// Double NUL terminated list of names required for each ELOBBYIDS between
// eCGSK_FirstGameReservedKey and eCGSK_LastGameReservedKey (one or two names)
// or between eCGSK_FirstCryNetworkOptionalKey and
// eCGSK_LastCryNetworkOptionalKey (one name).
#define GAMESPY_KEYNAME_MATCHDATA_GAMEMODE	"gt\0gametype\0"
#define GAMESPY_KEYNAME_MATCHDATA_MAP	"mn\0mapname\0"
#define GAMESPY_KEYNAME_MATCHDATA_ACTIVE	"a\0"
#define GAMESPY_KEYNAME_MATCHDATA_VERSION	"gv\0"
#define GAMESPY_KEYNAME_MATCHDATA_REQUIRED_DLCS		"d\0"
#define GAMESPY_KEYNAME_MATCHDATA_PLAYLIST	"p\0"
#define GAMESPY_KEYNAME_MATCHDATA_LANGUAGE "l\0"
#define GAMESPY_KEYNAME_MATCHDATA_OFFICIAL "o\0"
#define GAMESPY_KEYNAME_MATCHDATA_FAVOURITE_ID "fi\0"


#define REQUIRED_SESSIONS_QUERY	0
#define FIND_GAME_SESSION_QUERY 0
#define REQUIRED_SESSIONS_SEARCH_PARAM	0

// Rich presence string
#define RICHPRESENCE_ID					0

// Types of rich presence we can have
#define RICHPRESENCE_GAMEPLAY		0        
#define RICHPRESENCE_LOBBY			1
#define RICHPRESENCE_FRONTEND		2
#define RICHPRESENCE_SINGLEPLAYER	3
#define RICHPRESENCE_IDLE			4
#define RICHPRESENCE_SHOWROOM		5

// Rich presence string params
#define RICHPRESENCE_GAMEMODES	0
#define RICHPRESENCE_MAPS				1

// These string ids need to match those found in Scripts/Network/RichPresence.xml

// Rich presence strings, game mode
#define RICHPRESENCE_GAMEMODES_INSTANTACTION			0   
#define RICHPRESENCE_GAMEMODES_TEAMINSTANTACTION	1
#define RICHPRESENCE_GAMEMODES_ASSAULT						2          
#define RICHPRESENCE_GAMEMODES_CAPTURETHEFLAG			3
#define RICHPRESENCE_GAMEMODES_EXTRACTION					4
#define RICHPRESENCE_GAMEMODES_CRASHSITE					5
#define RICHPRESENCE_GAMEMODES_ALLORNOTHING				6
#define	RICHPRESENCE_GAMEMODES_BOMBTHEBASE				7
#define RICHPRESENCE_GAMEMODES_POWERSTRUGGLE			8

// Rich presence strings for maps
#define RICHPRESENCE_MAPS_ROOFTOPGARDENS			0
#define RICHPRESENCE_MAPS_PIER17              1
#define RICHPRESENCE_MAPS_WALLSTREET          2
#define RICHPRESENCE_MAPS_LIBERTYISLAND       3
#define RICHPRESENCE_MAPS_HARLEMGORGE         4
#define RICHPRESENCE_MAPS_COLLIDEDBUILDINGS   5
#define RICHPRESENCE_MAPS_ALIENCRASHTRAIL     6
#define RICHPRESENCE_MAPS_ALIENVESSEL        7
#define RICHPRESENCE_MAPS_BATTERYPARK         8
#define RICHPRESENCE_MAPS_BRYANTPARK          9
#define RICHPRESENCE_MAPS_CHURCH              10
#define RICHPRESENCE_MAPS_CONEYISLAND         11
#define RICHPRESENCE_MAPS_DOWNTOWN            12
#define RICHPRESENCE_MAPS_LIGHTHOUSE          13
#define RICHPRESENCE_MAPS_PARADE              14
#define RICHPRESENCE_MAPS_ROOSEVELT           15
#define RICHPRESENCE_MAPS_CITYHALL            16
#define RICHPRESENCE_MAPS_ALIENVESSELSMALL    17
#define RICHPRESENCE_MAPS_PIERSMALL           18
#define RICHPRESENCE_MAPS_LIBERTYISLAND_MIL   19
#define RICHPRESENCE_MAPS_TERMINAL            20
#define RICHPRESENCE_MAPS_DLC_1_MAP_1         21
#define RICHPRESENCE_MAPS_DLC_1_MAP_2         22
#define RICHPRESENCE_MAPS_DLC_1_MAP_3         23
#define RICHPRESENCE_MAPS_DLC_2_MAP_1         24
#define RICHPRESENCE_MAPS_DLC_2_MAP_2         25
#define RICHPRESENCE_MAPS_DLC_2_MAP_3         26
#define RICHPRESENCE_MAPS_DLC_3_MAP_1         27
#define RICHPRESENCE_MAPS_DLC_3_MAP_2         28
#define RICHPRESENCE_MAPS_DLC_3_MAP_3         29
#define RICHPRESENCE_MAPS_LIBERTYISLAND_STATUE 30

#else // default


// Ids
enum ELOBBYIDS
{
	LID_MATCHDATA_GAMEMODE = eCGSK_FirstGameReservedKey,
	LID_MATCHDATA_MAP = eCGSK_FirstGameReservedKey + 1,
	LID_MATCHDATA_ACTIVE = eCGSK_FirstGameReservedKey + 2,
	LID_MATCHDATA_VERSION = eCGSK_FirstGameReservedKey + 3,
	LID_MATCHDATA_REQUIRED_DLCS = eCGSK_FirstGameReservedKey + 4,
	LID_MATCHDATA_PLAYLIST = eCGSK_FirstGameReservedKey + 5,
	LID_MATCHDATA_VARIANT = eCGSK_GameVariantKey,		// Maps to predefined GameSpy key
	LID_MATCHDATA_SKILL = eCGSK_SkillKey,		// Maps to predefined GameSpy key
	LID_MATCHDATA_LANGUAGE = eCGSK_FirstGameReservedKey + 6,
	LID_MATCHDATA_OFFICIAL = eCGSK_FirstGameReservedKey + 7,
	LID_MATCHDATA_REGION = eCGSK_RegionKey,		// Maps to predefined GameSpy key
	LID_MATCHDATA_FAVOURITE_ID = eCGSK_HostProfileKey,		// Maps to optional CryNetwork key
	LID_MATCHDATA_SPECTATORS_ALLOWED = eCGSK_HostProfileKey + 8,
	LID_MATCHDATA_SPECTATORS_CURRENT = eCGSK_HostProfileKey + 9,
};

// Names for GameSpy keys.
#define GAMESPY_KEYNAME_MATCHDATA_GAMEMODE	"gt"
#define GAMESPY_KEYNAME_MATCHDATA_MAP	"mn"
#define GAMESPY_KEYNAME_MATCHDATA_ACTIVE	"a"
#define GAMESPY_KEYNAME_MATCHDATA_VERSION	"gv"
#define GAMESPY_KEYNAME_MATCHDATA_REQUIRED_DLCS		"d"
#define GAMESPY_KEYNAME_MATCHDATA_PLAYLIST	"p"
#define GAMESPY_KEYNAME_MATCHDATA_VARIANT	"v"
#define GAMESPY_KEYNAME_MATCHDATA_SKILL	"s"
#define GAMESPY_KEYNAME_MATCHDATA_LANGUAGE "l"
#define GAMESPY_KEYNAME_MATCHDATA_OFFICIAL "o"
#define GAMESPY_KEYNAME_MATCHDATA_REGION "rg"
#define GAMESPY_KEYNAME_MATCHDATA_FAVOURITE_ID "fi"

#define REQUIRED_SESSIONS_QUERY	0
#define FIND_GAME_SESSION_QUERY 0
#define REQUIRED_SESSIONS_SEARCH_PARAM	0

// Rich presence string
#define RICHPRESENCE_ID					0

// Types of rich presence we can have
#define RICHPRESENCE_GAMEPLAY		0        
#define RICHPRESENCE_LOBBY			1
#define RICHPRESENCE_FRONTEND		2
#define RICHPRESENCE_SINGLEPLAYER	3
#define RICHPRESENCE_IDLE			4
#define RICHPRESENCE_SHOWROOM		5

// Rich presence string params
#define RICHPRESENCE_GAMEMODES	0
#define RICHPRESENCE_MAPS				1

#endif

#define INVALID_SESSION_FAVOURITE_ID 0	// For eLDI_FavouriteID/LID_MATCHDATA_FAVOURITE_ID

enum ELobbyDataIndex
{
	eLDI_Gamemode = 0,
	eLDI_Version = 1,
	eLDI_Playlist = 2,
	eLDI_Variant = 3,
	eLDI_RequiredDLCs = 4,
#if USE_CRYLOBBY_GAMESPY
	eLDI_Official = 5,
	eLDI_Region = 6,
	eLDI_FavouriteID = 7,
#endif
	eLDI_Language,
	eLDI_Map,
	eLDI_Skill,
	eLDI_Active,

	eLDI_Num
};

namespace GameLobbyData
{
	extern char const * const g_sUnknownMapName;

	const uint32 ConvertGameRulesToHash(const char* gameRules);
	const char* GetGameRulesFromHash(uint32 hash, const char* unknownStr="Unknown");

	const uint32 ConvertMapToHash(const char* mapName);
	const char* GetMapFromHash(uint32 hash, const char *pUnknownStr = g_sUnknownMapName);

	const uint32 GetVersion();
	const bool IsCompatibleVersion(uint32 version);

	const uint32 GetPlaylistId();
	const uint32 GetVariantId();
	const uint32 GetGameDataPatchCRC();

#if USE_CRYLOBBY_GAMESPY
	const int GetOfficialServer();
#endif

	const int32 GetSearchResultsData(SCrySessionSearchResult* session, CryLobbyUserDataID id);
};

#endif // __GAMELOBBYDATA_H__
