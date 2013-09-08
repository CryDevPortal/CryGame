/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2010.
-------------------------------------------------------------------------
Description:
	- Contains various shared network util functions
-------------------------------------------------------------------------
History:
	- 19/07/2010 : Created by Colin Gulliver

*************************************************************************/

#include "StdAfx.h"
#include "GameNetworkUtils.h"
#include "Lobby/SessionNames.h"
#include <INetwork.h>

ECryLobbyError NetworkUtils_SendToAll( CCryLobbyPacket* pPacket, CrySessionHandle h, SSessionNames &clients, bool bCheckConnectionState )
{
	ECryLobbyError result = eCLE_Success;

	ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
	if (pLobby)
	{
		ICryMatchMaking *pMatchMaking = pLobby->GetMatchMaking();
		if (pMatchMaking)
		{
			const unsigned int numClients = clients.m_sessionNames.size();
			// Start from 1 since we don't want to send to ourselves (unless we're a dedicated server)
			const int startIndex = gEnv->IsDedicated() ? 0 : 1;
			for (unsigned int i = startIndex; (i < numClients) && (result == eCLE_Success); ++ i)
			{
				SSessionNames::SSessionName &client = clients.m_sessionNames[i];

				if (!bCheckConnectionState || client.m_bFullyConnected)
				{
					result = pMatchMaking->SendToClient(pPacket, h, client.m_conId);
				}
			}
		}
	}

	return result;
}

//---------------------------------------
bool NetworkUtils_CompareCrySessionId(const CrySessionID &lhs, const CrySessionID &rhs)
{
	if (!lhs && !rhs)
	{
		return true;
	}
	if ((lhs && !rhs) || (!lhs && rhs))
	{
		return false;
	}
	return (*lhs == *rhs);
}
