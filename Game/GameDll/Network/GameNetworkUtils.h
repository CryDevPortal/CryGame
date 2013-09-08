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

struct SSessionNames;

ECryLobbyError NetworkUtils_SendToAll(CCryLobbyPacket* pPacket, CrySessionHandle h, SSessionNames &clients, bool bCheckConnectionState);

bool NetworkUtils_CompareCrySessionId(const CrySessionID &lhs, const CrySessionID &rhs);
