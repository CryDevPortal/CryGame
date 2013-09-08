#ifndef ___GAME_SERVER_LISTS_H___
#define ___GAME_SERVER_LISTS_H___

#include "Network/GameNetworkDefines.h"

#ifdef GAME_IS_CRYSIS2
#include "FrontEnd/Multiplayer/UIServerList.h"
#endif

#if IMPLEMENT_PC_BLADES

#include <ICryLobby.h>
#include <IPlayerProfiles.h>

class CGameServerLists : public IPlayerProfileListener
{
public:
	CGameServerLists();
	virtual ~CGameServerLists();

	enum EGameServerLists
	{
		eGSL_Recent,
		eGSL_Favourite,
		eGSL_Size
	};

	const bool Add(const EGameServerLists list, const char* name, const uint32 favouriteId, bool bFromProfile);
	const bool Remove(const EGameServerLists list, const uint32 favouriteId);

#ifdef USE_C2_FRONTEND
	void ServerFound( const CUIServerList::SServerInfo &serverInfo, const EGameServerLists list, const uint32 favouriteId );
#endif //#ifdef USE_C2_FRONTEND
	void ServerNotFound( const EGameServerLists list, const uint32 favouriteId );

	const bool InList(const EGameServerLists list, const uint32 favouriteId) const;
	const int GetTotal(const EGameServerLists list) const;

	void PopulateMenu(const EGameServerLists list) const;

	void SaveChanges();

	//IPlayerProfileListener
	void SaveToProfile(IPlayerProfile* pProfile, bool online, unsigned int reason);
	void LoadFromProfile(IPlayerProfile* pProfile, bool online, unsigned int reason);
	//~IPlayerProfileListener













	static const int k_maxServersStoredInList = 50;

protected:
	struct SServerInfo
	{
		SServerInfo(const char* name, uint32 favouriteId);
		bool operator == (const SServerInfo & other) const;

		string m_name;
		uint32 m_favouriteId;
	};

	struct SListRules
	{
		SListRules();

		void Reset();
		void PreApply(std::list<SServerInfo>* pList, const SServerInfo &pNewInfo);

		uint m_limit;
		bool m_unique;
	};

	void Reset();

	std::list<SServerInfo> m_list[eGSL_Size];
	SListRules m_rule[eGSL_Size];

	bool m_bHasChanges;
};

#endif //IMPLEMENT_PC_BLADES

#endif //___GAME_SERVER_LISTS_H___