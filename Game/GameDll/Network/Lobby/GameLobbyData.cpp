
#include "StdAfx.h"
#include "GameLobbyData.h"

#include "Utility/CryHash.h"

#include "Game.h"
//#include "GameRulesModules/IGameRulesModulesManager.h"
//#include "GameRulesModules/GameRulesModulesManager.h"
#include <ILevelSystem.h>
#include <NameCRCHelper.h>
#include "GameCVars.h"
//#include "PlaylistManager.h"
//#include "DataPatcher.h"
#include "IGameRulesManager.h"
#include "GameRulesManager.h"

namespace GameLobbyData
{
	char const * const g_sUnknownMapName = "Unknown";

	const uint32 ConvertGameRulesToHash(const char* gameRules)
	{
		if (gameRules && (strlen(gameRules) < 32))
		{
			char lowerRulesName[32];
			NameCRCHelper::MakeLowercase(lowerRulesName, gameRules);
			return HashString(lowerRulesName);
		}
		else
		{
			return 0;
		}
	}

	const char* GetGameRulesFromHash(uint32 hash, const char* unknownStr/*="Unknown"*/)
	{
		IGameRulesManager *pGameRulesManager = CGameRulesManager::GetInstance();
		const int rulesCount = pGameRulesManager->GetRulesCount();
		for(int i = 0; i < rulesCount; i++)
		{
			const char* name = pGameRulesManager->GetRules(i);
			if(ConvertGameRulesToHash(name) == hash)
			{
				return name;
			}
		}

		return unknownStr;
	}

	const uint32 ConvertMapToHash(const char* mapName)
	{
		if (mapName && (strlen(mapName) < 128))
		{
			char lowerMapName[128];
			NameCRCHelper::MakeLowercase(lowerMapName, mapName);
			return HashString(lowerMapName);
		}
		else
		{
			return 0;
		}
	}

	const char* GetMapFromHash(uint32 hash, const char *pUnknownStr)
	{
		ILevelSystem* pLevelSystem = g_pGame->GetIGameFramework()->GetILevelSystem();
		const int levelCount = pLevelSystem->GetLevelCount();
		for(int i = 0; i < levelCount; i++)
		{
			const char* name = pLevelSystem->GetLevelInfo(i)->GetName();
			if(ConvertMapToHash(name) == hash)
			{
				return name;
			}
		}
		for(int i = 0; i < levelCount; i++)
		{
			const char* name = pLevelSystem->GetLevelInfo(i)->GetName();
			const char* pTrimmedLevelName = strrchr(name, '/');
			if(pTrimmedLevelName && ConvertMapToHash(pTrimmedLevelName+1) == hash)
			{
				return name;
			}
		}
		return pUnknownStr;
	}

	const uint32 GetVersion()
	{
		// matchmaking version defaults to build id, i've chose the bit shifts here to ensure we don't unnecessary truncate the version and matchmake against the wrong builds
		const uint32 version = (g_pGameCVars->g_MatchmakingVersion & 8191) + (g_pGameCVars->g_MatchmakingBlock * 8192);
		return version^GetGameDataPatchCRC();
	}

	const bool IsCompatibleVersion(uint32 version)
	{
		return version == GetVersion();
	}

	const uint32 GetPlaylistId()
	{
#ifdef GAME_IS_CRYSIS2
		CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
		if (pPlaylistManager)
		{
			const SPlaylist* pPlaylist = pPlaylistManager->GetCurrentPlaylist();
			if(pPlaylist)
			{
				return pPlaylist->id;
			}
		}
#endif

		return 0;
	}

	const uint32 GetVariantId()
	{
#ifdef GAME_IS_CRYSIS2
		CPlaylistManager *pPlaylistManager = g_pGame->GetPlaylistManager();
		if (pPlaylistManager)
		{
			return pPlaylistManager->GetActiveVariantIndex();
		}
#endif

		return 0;
	}

	const uint32 GetGameDataPatchCRC()
	{
		uint32		result=0;

#ifdef GAME_IS_CRYSIS2
		if (CDataPatcher *pDP=g_pGame->GetDataPatcher())
		{
			result=pDP->IsPatchingEnabled()?pDP->GetDataPatchHash():0;
		}
#endif

		return result;
	}

#if USE_CRYLOBBY_GAMESPY
	const int GetOfficialServer()
	{
		return g_pGameCVars->g_officialServer;
	}
#endif

	const int32 GetSearchResultsData(SCrySessionSearchResult* session, CryLobbyUserDataID id)
	{

		for (uint32 i = 0; i < session->m_data.m_numData; i++)
		{
			if (session->m_data.m_data[i].m_id == id)
			{
				return session->m_data.m_data[i].m_int32;
			}
		}

		CRY_ASSERT(0);

		return 0;
	}
}
