/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Game side definitions for lobby user packets

-------------------------------------------------------------------------
History:
- 12:03:2010 : Created By Ben Parbury

*************************************************************************/

#include "ICryLobby.h"
#include "CryLobbyPacket.h"

#pragma once

enum GameUserPacketDefinitions
{
	eGUPD_LobbyStartCountdownTimer = CRYLOBBY_USER_PACKET_START,
	eGUPD_LobbyGameHasStarted,
	eGUPD_LobbyEndGame,
	eGUPD_LobbyEndGameResponse,
	eGUPD_LobbyUpdatedSessionInfo,
	eGUPD_LobbyMoveSession,
	eGUPD_SquadJoin,
	eGUPD_SquadJoinGame,
	eGUPD_SquadNotInGame,
	eGUPD_SetTeam,
	eGUPD_SendChatMessage,								// Clients request to send a message to other players
	eGUPD_ChatMessage,										// Server sent message to all appropriate other players.
	eGUPD_ReservationRequest,							// Sent by squad leader client after joined game to identify self as leader and to tell the game server to reserve slots for its members
	eGUPD_ReservationClientIdentify,			// Sent by clients after joined game to identify self to game server so it can check if client passes reserve checks (if any)
	eGUPD_ReservationsMade,								// Sent to a squad leader by a server when requested reservations have been successfully made upon receipt of a eGUPD_ReservationRequest packet 
	eGUPD_ReservationFailedSessionFull,		// Can be sent to clients when there's no room for them in a game session. Generally causes them to "kick" themselves by deleting their own session
	eGUPD_SyncPlaylistContents,						// Sync entire playlist
	eGUPD_SetGameVariant,
	eGUPD_SyncPlaylistRotation,
	eGUPD_SquadLeaveGame,									// Squad: Tell all members in the squad to leave (game host will leave last)
	eGUPD_SquadNotSupported,							// Squads not suported in current gamemode
	eGUPD_UpdateCountdownTimer,
	eGUPD_RequestAdvancePlaylist,
	eGUPD_SyncExtendedServerDetails,
#if INCLUDE_DEDICATED_LEADERBOARDS
	eGUPD_StartOfGameUserStats,						// User has joined a ranked server, send them there stats
	eGUPD_EndOfGameUserStats,							// User has played to end of round, save their stats
	eGUPD_UpdateUserStats,
	eGUPD_UpdateUserStatsReceived,				// response from server to client to say it has received all the stats
	eGUPD_FailedToReadOnlineData,
#endif
	eGUPD_DetailedServerInfoRequest,
	eGUPD_DetailedServerInfoResponse,
	eGUPD_SquadDifferentVersion,					// Response to SquadJoin packet sent when the client is on a different patch
	eGUPD_SquadKick,
	eGUPD_UnloadPreviousLevel,						// Tell clients that we're about to start and they should clean up the previous game

	eGUPD_End
};

