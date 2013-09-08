/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Script Binding for GameRules

-------------------------------------------------------------------------
History:
- 23:2:2006   18:30 : Created by Marcio Martins

*************************************************************************/
#ifndef __SCRIPTBIND_GAMERULES_H__
#define __SCRIPTBIND_GAMERULES_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include <IScriptSystem.h>
#include <ScriptHelpers.h>


class CGameRules;
class CActor;
struct IGameFramework;
struct ISystem;


class CScriptBind_GameRules :
	public CScriptableBase
{
public:
	CScriptBind_GameRules(ISystem *pSystem, IGameFramework *pGameFramework);
	virtual ~CScriptBind_GameRules();

	void AttachTo(CGameRules *pGameRules);

	// <title IsServer>
	// Syntax: GameRules.IsServer()
	// Description:
	//		Checks if this entity is a server.
	int IsServer(IFunctionHandler *pH);
	// <title IsClient>
	// Syntax: GameRules.IsClient()
	// Description:
	//		Checks if this entity is a client.
	int IsClient(IFunctionHandler *pH);
	// <title CanCheat>
	// Syntax: GameRules.CanCheat()
	// Description:
	//		Checks if in this game entity is possible to cheat.
	int CanCheat(IFunctionHandler *pH);

	// <title SpawnPlayer>
	// Syntax: GameRules.SpawnPlayer( int channelId, const char *name, const char *className, Vec3 pos, Vec3 angles )
	// Arguments:
	//		channelId	- Channel identifier.
	//		name		- Player name.
	//		className	- Name of the player class.
	//		pos			- Player position.
	//		angles		- Player angle.
	// Description:
	//		Spawns a player.
	int SpawnPlayer(IFunctionHandler *pH, int channelId, const char *name, const char *className, Vec3 pos, Vec3 angles);
	// <title ChangePlayerClass>
	// Syntax: GameRules.ChangePlayerClass( int channelId, const char *className )
	// Arguments:
	//		channelId - Channel identifier.
	//		className - Name of the new class for the player.
	// Description:
	//		Changes the player class.
	int ChangePlayerClass(IFunctionHandler *pH, int channelId, const char *className);
	// <title RevivePlayer>
	// Syntax: GameRules.RevivePlayer( ScriptHandle playerId, Vec3 pos, Vec3 angles, int teamId, bool clearInventory )
	// Arguments:
	//		playerId		- Player identifier.
	//		pos				- Player position.
	//		angles			- Player angle.
	//		teamId			- Team identifier.
	//		clearInventory	- True to clean the inventory, false otherwise. 
	// Description:
	//		Revives the player.
	int RevivePlayer(IFunctionHandler *pH, ScriptHandle playerId, Vec3 pos, Vec3 angles, int teamId, bool clearInventory);
	// <title RevivePlayerInVehicle>
	// Syntax: GameRules.RevivePlayerInVehicle( ScriptHandle playerId, ScriptHandle vehicleId, int seatId, int teamId, bool clearInventory )
	// Arguments:
	//		playerId		- Player identifier.
	//		vehicleId		- Vehicle identifier.
	//		seatId			- Seat identifier.
	//		teamId			- Team identifier.
	//		clearInventory	- True to clean the inventory, false otherwise.
	// Description:
	//		Revives a player inside a vehicle.
	int RevivePlayerInVehicle(IFunctionHandler *pH, ScriptHandle playerId, ScriptHandle vehicleId, int seatId, int teamId, bool clearInventory);
	// <title RenamePlayer>
	// Syntax: GameRules.RenamePlayer( ScriptHandle playerId, const char *name )
	// Arguments:
	//		playerId - Player identifier.
	//		name	 - New name for the player.
	// Description:
	//		Renames the player.
	int RenamePlayer(IFunctionHandler *pH, ScriptHandle playerId, const char *name);
	// <title KillPlayer>
	// Syntax: GameRules.KillPlayer( ScriptHandle playerId, bool dropItem, bool ragdoll,
	//		ScriptHandle shooterId, ScriptHandle weaponId, float damage, int material, int hit_type, Vec3 impulse)
	// Arguments:
	//		playerId	- Player identifier.
	//		dropItem	- True to drop the item, false otherwise.
	//		ragdoll		- True to ragdollize, false otherwise.
	//		shooterId	- Shooter identifier.
	//		weaponId	- Weapon identifier.
	//		damage		- Damage amount.
	//		material	- Material identifier.
	//		hit_type	- Type of the hit.
	//		impulse		- Impulse vector due to the hit.
	// Description:
	//		Kills the player.
	int KillPlayer(IFunctionHandler *pH, ScriptHandle playerId, bool dropItem, bool ragdoll,
			ScriptHandle shooterId, ScriptHandle weaponId, float damage, int hitJoint, int hit_type, Vec3 impulse, ScriptHandle projectileId);
	// <title MovePlayer>
	// Syntax: GameRules.MovePlayer( ScriptHandle playerId, Vec3 pos, Vec3 angles )
	// Arguments:
	//		playerId - Player identifier.
	//		pos		 - Position to be reached.
	//		angles	 - .
	// Description:
	//		Moves the player.
	int MovePlayer(IFunctionHandler *pH, ScriptHandle playerId, Vec3 pos, Vec3 angles);
	// <title GetPlayerByChannelId>
	// Syntax: GameRules.GetPlayerByChannelId( int channelId )
	// Arguments:
	//		channelId - Channel identifier.
	// Description: 
	//		Gets the player from the channel id.
	int GetPlayerByChannelId(IFunctionHandler *pH, int channelId);
	// <title GetChannelId>
	// Syntax: GameRules.GetChannelId( playerId )
	// Arguments:
	//		playerId - Player identifier.
	// Description:
	//		Gets the channel id where the specified player is.
	int GetChannelId(IFunctionHandler *pH, ScriptHandle playerId);
	// <title GetPlayerCount>
	// Syntax: GameRules.GetPlayerCount()
	// Description:
	//		Gets the number of the players.
	int GetPlayerCount(IFunctionHandler *pH);
	// <title GetSpectatorCount>
	// Syntax: GameRules.GetSpectatorCount()
	// Description:
	//		Gets the number of the spectators.
	int GetSpectatorCount(IFunctionHandler *pH);
	// <title GetPlayers>
	// Syntax: GameRules.GetPlayers()
	// Description:
	//		Gets the player.
	int GetPlayers(IFunctionHandler *pH);
	// <title IsPlayerInGame>
	// Syntax: GameRules.IsPlayerInGame(playerId)
	// Arguments:
	//		playerId - Player identifier.
	// Description:
	//		Checks if the specified player is in game.
	int IsPlayerInGame(IFunctionHandler *pH, ScriptHandle playerId);
	// <title IsProjectile>
	// Syntax: GameRules.IsProjectile(entityId)
	// Arguments:
	//		entityId - Entity identifier.
	// Description:
	//		Checks if the specified entity is a projectile.
	int IsProjectile(IFunctionHandler *pH, ScriptHandle entityId);
	// <title IsSameTeam>
	// Syntax: GameRules.IsSameTeam(entityId0,entityId1)
	// Arguments:
	//		entityId0 - First entity identifier.
	//		entityId1 - Second entity identifier.
	// Description:
	//		Checks if the two entity are in the same team.
	int IsSameTeam(IFunctionHandler *pH, ScriptHandle entityId0, ScriptHandle entityId1);
	// <title IsNeutral>
	// Syntax: GameRules.IsNeutral(entityId)
	// Arguments:
	//		entityId - Entity identifier.
	// Description:
	//		Checks if the entity is neutral.
	int IsNeutral(IFunctionHandler *pH, ScriptHandle entityId);

	// <title AddSpawnLocation>
	// Syntax: GameRules.AddSpawnLocation(entityId)
	// Arguments:
	//		entityId - Entity identifier.
	// Description:
	//		Adds the spawn location for the specified entity.
	int AddSpawnLocation(IFunctionHandler *pH, ScriptHandle entityId);
	// <title RemoveSpawnLocation>
	// Syntax: GameRules.RemoveSpawnLocation(id)
	// Arguments:
	//		id - Identifier for the script.
	// Description:
	//		Removes the span location.
	int RemoveSpawnLocation(IFunctionHandler *pH, ScriptHandle id);
	// <title GetSpawnLocationCount>
	// Syntax: GameRules.GetSpawnLocationCount()
	// Description:
	//		Gets the number of the spawn location.
	int GetSpawnLocationCount(IFunctionHandler *pH);
	// <title GetSpawnLocationByIdx>
	// Syntax: GameRules.GetSpawnLocationByIdx( int idx )
	// Arguments:
	//		idx - Spawn location identifier.
	// Description:
	//		Gets the spawn location from its identifier.
	int GetSpawnLocationByIdx(IFunctionHandler *pH, int idx);
	// <title GetSpawnLocation>
	// Syntax: GameRules.GetSpawnLocation( playerId, bool ignoreTeam, bool includeNeutral )
	// Arguments:
	//		playerId		- Player identifier.
	//		ignoreTeam		- True to ignore team, false otherwise.
	//		includeNeutral	- True to include neutral entity, false otherwise.
	int GetSpawnLocation(IFunctionHandler *pH, ScriptHandle playerId, bool ignoreTeam, bool includeNeutral);
	// <title GetSpawnLocations>
	// Syntax: GameRules.GetSpawnLocations()
	// Description:
	//		Gets the spawn locations.
	int GetSpawnLocations(IFunctionHandler *pH);
	// <title GetFirstSpawnLocation>
	// Syntax: GameRules.GetFirstSpawnLocation( int teamId )
	// Arguments:
	//		teamId - Team identifier.
	// Description:
	//		Gets the first spawn location for the team.
	int GetFirstSpawnLocation(IFunctionHandler *pH, int teamId);

	// <title AddSpawnGroup>
	// Syntax: GameRules.AddSpawnGroup(groupId)
	// Arguments:
	//		groupId - Group identifier.
	// Description:
	//		Adds a spawn group.
	int AddSpawnGroup(IFunctionHandler *pH, ScriptHandle groupId);
	// <title AddSpawnLocationToSpawnGroup>
	// Syntax: GameRules.AddSpawnLocationToSpawnGroup(groupId,location)
	// Arguments:
	//		groupId		- Group identifier.
	//		location	- Location.
	// Description:
	//		Add a spawn location to spawn a group.
	int AddSpawnLocationToSpawnGroup(IFunctionHandler *pH, ScriptHandle groupId, ScriptHandle location);
	// <title RemoveSpawnLocationFromSpawnGroup>
	// Syntax: GameRules.RemoveSpawnLocationFromSpawnGroup(groupId,location)
	// Arguments:
	//		groupId  - Group identifier.
	//		location - Location.
	// Description:
	//		Removes a spawn location from spawn group.
	int RemoveSpawnLocationFromSpawnGroup(IFunctionHandler *pH, ScriptHandle groupId, ScriptHandle location);
	// <title RemoveSpawnGroup>
	// Syntax: GameRules.RemoveSpawnGroup(groupId)
	// Arguments:
	//		groupId - Group identifier.
	// Description:
	//		Removes spawn group.
	int RemoveSpawnGroup(IFunctionHandler *pH, ScriptHandle groupId);
	// <title GetSpawnLocationGroup>
	// Syntax: GameRules.GetSpawnLocationGroup(spawnId)
	// Arguments:
	//		spawnId - Spawn identifier.
	// Description:
	//		Gets spawn location group.
	int GetSpawnLocationGroup(IFunctionHandler *pH, ScriptHandle spawnId);
	// <title GetSpawnGroups>
	// Syntax: GameRules.GetSpawnGroups()
	// Description:
	//		Gets spawn groups.
	int GetSpawnGroups(IFunctionHandler *pH);
	// <title IsSpawnGroup>
	// Syntax: GameRules.IsSpawnGroup(entityId)
	// Arguments:
	//		entityId - Entity identifier.
	// Description:
	//		Checks if the entity is a spawn group.
	int IsSpawnGroup(IFunctionHandler *pH, ScriptHandle entityId);

	// <title GetTeamDefaultSpawnGroup>
	// Syntax: GameRules.GetTeamDefaultSpawnGroup( int teamId )
	// Arguments:
	//		teamId - Team identifier.
	// Description:
	//		Gets team default spawn group.
	int GetTeamDefaultSpawnGroup(IFunctionHandler *pH, int teamId);
	// <title SetTeamDefaultSpawnGroup>
	// Syntax: GameRules.SetTeamDefaultSpawnGroup( int teamId, groupId)
	// Arguments:
	//		teamId  - Team identifier.
	//		groupId - Group identifier.
	// Description:
	//		Sets the default spawn group for the team.
	int SetTeamDefaultSpawnGroup(IFunctionHandler *pH, int teamId, ScriptHandle groupId);
	// <title SetPlayerSpawnGroup>
	// Syntax: GameRules.SetPlayerSpawnGroup(ScriptHandle playerId, ScriptHandle groupId)
	// Arguments:
	//		playerId - Player identifier.
	//		groupId  - Group identifier.
	// Description:
	//		Sets the player spawn group.
	int SetPlayerSpawnGroup(IFunctionHandler *pH, ScriptHandle playerId, ScriptHandle groupId);

	// <title AddSpectatorLocation>
	// Syntax: GameRules.AddSpectatorLocation( ScriptHandle location )
	// Arguments:
	//		location - Location.
	// Description:
	//		Adds a spectator location.
	int AddSpectatorLocation(IFunctionHandler *pH, ScriptHandle location);
	// <title RemoveSpectatorLocation>
	// Syntax: GameRules.RemoveSpectatorLocation( ScriptHandle id )
	// Arguments:
	//		id - Spectator identifier.
	// Description:
	//		Removes a spectator location.
	int RemoveSpectatorLocation(IFunctionHandler *pH, ScriptHandle id);
	// <title GetSpectatorLocationCount>
	// Syntax: GameRules.GetSpectatorLocationCount(  )
	// Description:
	//		Gets the number of the spectator locations.
	int GetSpectatorLocationCount(IFunctionHandler *pH);
	// <title GetSpectatorLocation>
	// Syntax: GameRules.GetSpectatorLocation( int idx )
	// Arguments:
	//		idx - Spectator location identifier.
	// Description:
	//		Gets the spectator location from its identifier.
	int GetSpectatorLocation(IFunctionHandler *pH, int idx);
	// <title GetSpectatorLocations>
	// Syntax: GameRules.GetSpectatorLocations(  )
	// Description:
	//		Gets the spectator locations.
	int GetSpectatorLocations(IFunctionHandler *pH);
	// <title GetRandomSpectatorLocation>
	// Syntax: GameRules.GetRandomSpectatorLocation(  )
	// Description:
	//		Gets a random spectator location.
	int GetRandomSpectatorLocation(IFunctionHandler *pH);
	// <title GetInterestingSpectatorLocation>
	// Syntax: GameRules.GetInterestingSpectatorLocation(  )
	// Description:
	//		Gets an interesting spectator location.
	int GetInterestingSpectatorLocation(IFunctionHandler *pH);
	// <title GetNextSpectatorTarget>
	// Syntax: GameRules.GetNextSpectatorTarget( ScriptHandle playerId, int change )
	// Description:
	//		For 3rd person follow-cam mode. Gets the next spectator target.
	int GetNextSpectatorTarget(IFunctionHandler *pH, ScriptHandle playerId, int change);
	// <title ChangeSpectatorMode>
	// Syntax: GameRules.ChangeSpectatorMode( ScriptHandle playerId, int mode, ScriptHandle targetId )
	// Arguments:
	//		playerId - Player identifier.
	//		mode	 - New spectator mode.
	//		targetId - Target identifier.
	// Description:
	//		Changes the spectator mode.
	int ChangeSpectatorMode(IFunctionHandler* pH, ScriptHandle playerId, int mode, ScriptHandle targetId);
	// <title CanChangeSpectatorMode>
	// Syntax: GameRules.CanChangeSpectatorMode( ScriptHandle playerId )
	// Arguments:
	//		playerId - Player identifier.
	// Description:
	//		Check if it's possible to change the spectator mode.
	int CanChangeSpectatorMode(IFunctionHandler* pH, ScriptHandle playerId);
	
	// <title AddMinimapEntity>
	// Syntax: GameRules.AddMinimapEntity( ScriptHandle entityId, int type, float lifetime )
	// Arguments:
	//		entityId - Entity identifier.
	//		type	 - .
	//		lifetime - .
	// Description:
	//		Adds a mipmap entity.
	int AddMinimapEntity(IFunctionHandler *pH, ScriptHandle entityId, int type, float lifetime);
	// <title RemoveMinimapEntity>
	// Syntax: GameRules.RemoveMinimapEntity( ScriptHandle entityId )
	// Arguments:
	//		entityId - Mipmap entity player.
	// Description:
	//		Removes a mipmap entity.
	int RemoveMinimapEntity(IFunctionHandler *pH, ScriptHandle entityId);
	// <title ResetMinimap>
	// Syntax: GameRules.ResetMinimap(  )
	int ResetMinimap(IFunctionHandler *pH);

	// <title GetPing>
	// Syntax: GameRules.GetPing( int channelId )
	// Arguments:
	//		channelId - Channel identifier.
	// Description:
	//		Gets the ping to a channel.
	int GetPing(IFunctionHandler *pH, int channelId);

	// <title ResetEntities>
	// Syntax: GameRules.ResetEntities(  )
	int ResetEntities(IFunctionHandler *pH);
	// <title ServerExplosion>
	// Syntax: GameRules.ServerExplosion( ScriptHandle shooterId, ScriptHandle weaponId, float dmg,
	//		Vec3 pos, Vec3 dir, float radius, float angle, float pressure, float holesize )
	int ServerExplosion(IFunctionHandler *pH, ScriptHandle shooterId, ScriptHandle weaponId, float dmg,
		Vec3 pos, Vec3 dir, float radius, float angle, float pressure, float holesize);
	// <title ServerHit>
	// Syntax: GameRules.ServerHit( ScriptHandle targetId, ScriptHandle shooterId, ScriptHandle weaponId, float dmg, float radius, int materialId, int partId, int typeId )
	int ServerHit(IFunctionHandler *pH, ScriptHandle targetId, ScriptHandle shooterId, ScriptHandle weaponId, float dmg, float radius, int materialId, int partId, int typeId);

	// <title CreateTeam>
	// Syntax: GameRules.CreateTeam( const char *name )
	// Arguments:
	//		name - Team name.
	// Description:
	//		Creates a team.
	int CreateTeam(IFunctionHandler *pH, const char *name);
	// <title RemoveTeam>
	// Syntax: GameRules.RemoveTeam( int teamId )
	// Arguments:
	//		teamId - Team identifier.
	// Description:
	//		Removes the specified team.
	int RemoveTeam(IFunctionHandler *pH, int teamId);
	// <title GetTeamName>
	// Syntax: GameRules.GetTeamName( int teamId )
	// Arguments:
	//		teamId - Team identifier.
	// Description:
	//		Gets the name of the specified team.
	int GetTeamName(IFunctionHandler *pH, int teamId);
	// <title GetTeamId>
	// Syntax: GameRules.GetTeamId( const char *teamName )
	// Arguments:
	//		teamName - Team name.
	// Description:
	//		Gets the team identifier from the team name.
	int GetTeamId(IFunctionHandler *pH, const char *teamName);
	// <title GetTeamCount>
	// Syntax: GameRules.GetTeamCount(  )
	// Description:
	//		Gets the team number.
	int GetTeamCount(IFunctionHandler *pH);
	// <title GetTeamPlayerCount>
	// Syntax: GameRules.GetTeamPlayerCount( int teamId )
	// Arguments:
	//		teamId - Team identifier.
	// Description:
	//		Gets the number of players in the specified team.
	int GetTeamPlayerCount(IFunctionHandler *pH, int teamId);
	// <title GetTeamChannelCount>
	// Syntax: GameRules.GetTeamChannelCount( int teamId )
	// Arguments:
	//		teamId - Team identifier.
	// Description:
	//		Gets the team channel count.
	int GetTeamChannelCount(IFunctionHandler *pH, int teamId);
	// <title GetTeamPlayers>
	// Syntax: GameRules.GetTeamPlayers( int teamId )
	// Arguments:
	//		teamId - Team identifier.
	// Description:
	//		Gets the players in the specified team.
	int GetTeamPlayers(IFunctionHandler *pH, int teamId);

	// <title SetTeam>
	// Syntax: GameRules.SetTeam( int teamId, ScriptHandle playerId )
	// Arguments:
	//		teamId		- Team identifier.
	//		playerId	- Player identifier.
	// Description:
	//		Adds a player to a team.
	int SetTeam(IFunctionHandler *pH, int teamId, ScriptHandle playerId);
	// <title GetTeam>
	// Syntax: GameRules.GetTeam( ScriptHandle playerId )
	// Arguments:
	//		playerId - Player identifier.
	// Description:
	//		Gets the team of the specified player.
	int GetTeam(IFunctionHandler *pH, ScriptHandle playerId);
	// <title GetChannelTeam>
	// Syntax: GameRules.GetChannelTeam( int channelId )
	// Arguments:
	//		channelId - Channel identifier.
	// Description:
	//		Gets the team in the specified channel.
	int GetChannelTeam(IFunctionHandler *pH, int channelId);

	// <title AddObjective>
	// Syntax: GameRules.AddObjective( int teamId, const char *objective, int status, ScriptHandle entityId )
	// Arguments:
	//		teamId		- Team identifier.
	//		objective	- Objective name.
	//		status		- Status.
	//		entityId	- Entity identifier.
	// Description:
	//		Adds an objective for the specified team with the specified status.
	int AddObjective(IFunctionHandler *pH, int teamId, const char *objective, int status, ScriptHandle entityId);
	// <title SetObjectiveStatus>
	// Syntax: GameRules.SetObjectiveStatus( int teamId, const char *objective, int status )
	// Arguments:
	//		teamId		- Team identifier.
	//		objective	- Objective name.
	//		status		- Status.
	// Description:
	//		Sets the status of an objective.
	int SetObjectiveStatus(IFunctionHandler *pH, int teamId, const char *objective, int status);
	// <title SetObjectiveEntity>
	// Syntax: GameRules.SetObjectiveEntity( int teamId, const char *objective, ScriptHandle entityId )
	// Arguments:
	//		teamId		- Team identifier.
	//		objective	- Objective name.
	//		entityId	- Entity identifier.
	// Description:
	//		Sets the objective entity.
	int SetObjectiveEntity(IFunctionHandler *pH, int teamId, const char *objective, ScriptHandle entityId);
	// <title RemoveObjective>
	// Syntax: GameRules.RemoveObjective( int teamId, const char *objective )
	// Arguments:
	//		teamId		- Team identifier.
	//		objective	- Objective name.
	// Description:
	//		Removes an objective.
	int RemoveObjective(IFunctionHandler *pH, int teamId, const char *objective);
	// <title ResetObjectives>
	// Syntax: GameRules.ResetObjectives(  )
	// Description:
	//		Resets all the objectives.
	int ResetObjectives(IFunctionHandler *pH);

	// <title TextMessage>
	// Syntax: GameRules.TextMessage(  )
	// Arguments:
	//		type - Message type.
	//		msg	 - Message string.
	// Description:
	//		Displays a text message type with the specified text.
	int TextMessage(IFunctionHandler *pH, int type, const char *msg);
	// <title SendTextMessage>
	// Syntax: GameRules.SendTextMessage( int type, const char *msg )
	// Arguments:
	//		type - Message type.
	//		msg	 - Message string.
	// Description:
	//		Sends a text message type with the specified text.
	int SendTextMessage(IFunctionHandler *pH, int type, const char *msg);
	// <title SendChatMessage>
	// Syntax: GameRules.SendChatMessage( int type, ScriptHandle sourceId, ScriptHandle targetId, const char *msg )
	// Arguments:
	//		type		- Message type.
	//		sourceId	- Source identifier.
	//		targetId	- Target identifier.
	//		msg			- Message string.
	// Description:
	//		Sends a text message from the source to the target with the specified type and text.
	int SendChatMessage(IFunctionHandler *pH, int type, ScriptHandle sourceId, ScriptHandle targetId, const char *msg);
	// <title ForbiddenAreaWarning>
	// Syntax: GameRules.ForbiddenAreaWarning( bool active, int timer, ScriptHandle targetId )
	// Arguments:
	//		active		- .
	//		timer		- .
	//		targetId	- .
	// Description:
	//		Warnings for a forbidden area.
	int ForbiddenAreaWarning(IFunctionHandler *pH, bool active, int timer, ScriptHandle targetId);

	// <title ResetGameTime>
	// Syntax: GameRules.ResetGameTime()
	// Description:
	//		Resets the game time.
	int ResetGameTime(IFunctionHandler *pH);
	// <title GetRemainingGameTime>
	// Syntax: GameRules.GetRemainingGameTime()
	// Description:
	//		Gets the remaining game time.
	int GetRemainingGameTime(IFunctionHandler *pH);
	// <title IsTimeLimited>
	// Syntax: GameRules.IsTimeLimited()
	// Description:
	//		Checks if the game time is limited.
	int IsTimeLimited(IFunctionHandler *pH);

	// <title ResetRoundTime>
	// Syntax: GameRules.ResetRoundTime()
	// Description:
	//		Resets the round time.
	int ResetRoundTime(IFunctionHandler *pH);
	// <title GetRemainingRoundTime>
	// Syntax: GameRules.GetRemainingRoundTime()
	// Description:
	//		Gets the remaining round time.
	int GetRemainingRoundTime(IFunctionHandler *pH);
	// <title IsRoundTimeLimited>
	// Syntax: GameRules.IsRoundTimeLimited()
	// Description:
	//		Checks if the round time is limited.
	int IsRoundTimeLimited(IFunctionHandler *pH);

	// <title ResetPreRoundTime>
	// Syntax: GameRules.ResetPreRoundTime()
	// Description:
	//		Resets the pre-round time.
	int ResetPreRoundTime(IFunctionHandler *pH);
	// <title GetRemainingPreRoundTime>
	// Syntax: GameRules.GetRemainingPreRoundTime()
	// Description:
	//		Gets the remaining pre-round time.
	int GetRemainingPreRoundTime(IFunctionHandler *pH);

	// <title ResetReviveCycleTime>
	// Syntax: GameRules.ResetReviveCycleTime()
	// Description:
	//		Resets the revive cycle time.
	int ResetReviveCycleTime(IFunctionHandler *pH);
	// <title GetRemainingReviveCycleTime>
	// Syntax: GameRules.GetRemainingReviveCycleTime()
	// Description:
	//		Gets the remaining cycle time.
	int GetRemainingReviveCycleTime(IFunctionHandler *pH);

	// <title ResetGameStartTimer>
	// Syntax: GameRules.ResetGameStartTimer()
	// Description:
	//		Resets game start timer.
	int ResetGameStartTimer(IFunctionHandler *pH, float time);
	// <title GetRemainingStartTimer>
	// Syntax: GameRules.GetRemainingStartTimer(  )
	// Description:
	//		Gets the remaining start timer.
	int GetRemainingStartTimer(IFunctionHandler *pH);

	// <title EndGame>
	// Syntax: GameRules.EndGame()
	// Description:
	//		Ends the game.
	int	EndGame(IFunctionHandler *pH);
	// <title NextLevel>
	// Syntax: GameRules.NextLevel()
	// Description:
	//		Loads the next level.
	int NextLevel(IFunctionHandler *pH);

	// <title RegisterHitMaterial>
	// Syntax: GameRules.RegisterHitMaterial( const char *materialName )
	// Arguments:
	//		materialName - Name of the material.
	// Description:
	//		Registers the specified hit material.
	int RegisterHitMaterial(IFunctionHandler *pH, const char *materialName);
	// <title GetHitMaterialId>
	// Syntax: GameRules.GetHitMaterialId( const char *materialName )
	// Arguments:
	//		materialName - Name of the material.
	// Description:
	//		Gets the hit material identifier from its name.
	int GetHitMaterialId(IFunctionHandler *pH, const char *materialName);
	// <title GetHitMaterialName>
	// Syntax: GameRules.GetHitMaterialName( int materialId )
	// Arguments:
	//		materialId - Material identifier.
	// Description:
	//		Gets the hit material name from its identifier.
	int GetHitMaterialName(IFunctionHandler *pH, int materialId);
	// <title ResetHitMaterials>
	// Syntax: GameRules.ResetHitMaterials()
	// Description:
	//		Resets the hit materials.
	int ResetHitMaterials(IFunctionHandler *pH);

	// <title RegisterHitType>
	// Syntax: GameRules.RegisterHitType( const char *type )
	// Arguments:
	//		type - Hit type.
	// Description:
	//		Registers a type for the hits.
	int RegisterHitType(IFunctionHandler *pH, const char *type);
	// <title GetHitTypeId>
	// Syntax: GameRules.GetHitTypeId( const char *type )
	// Arguments:
	//		type - Hit type name.
	// Description:
	//		Gets a hit type identifier from the name.
	int GetHitTypeId(IFunctionHandler *pH, const char *type);
	// <title GetHitType>
	// Syntax: GameRules.GetHitType( int id )
	//		id	- Identifier.
	// Arguments:
	//		Gets the hit type from identifier.
	int GetHitType(IFunctionHandler *pH, int id);
	// <title ResetHitTypes>
	// Syntax: GameRules.ResetHitTypes()
	// Description:
	//		Resets the hit types.
	int ResetHitTypes(IFunctionHandler *pH);

	// <title ForceScoreboard>
	// Syntax: GameRules.ForceScoreboard( bool force )
	// Arguments:
	//		force - True to force scoreboard, false otherwise.
	// Description:
	//		Forces the display of the scoreboard on the HUD.
	int ForceScoreboard(IFunctionHandler *pH, bool force);
	// <title FreezeInput>
	// Syntax: GameRules.FreezeInput( bool freeze )
	// Arguments:
	//		freeze - True to freeze input, false otherwise.
	// Description:
	//		Freezes the input.
	int FreezeInput(IFunctionHandler *pH, bool freeze);

	// <title ScheduleEntityRespawn>
	// Syntax: GameRules.ScheduleEntityRespawn( ScriptHandle entityId, bool unique, float timer )
	// Arguments:
	//		entityId - Entity identifier.
	//		unique	 - True to have a unique respawn, false otherwise.
	//		timer	 - Float value for the respawning time.
	// Description:
	//		Schedules a respawning of the specified entity.
	int ScheduleEntityRespawn(IFunctionHandler *pH, ScriptHandle entityId, bool unique, float timer);
	// <title AbortEntityRespawn>
	// Syntax: GameRules.AbortEntityRespawn( ScriptHandle entityId, bool destroyData )
	// Arguments:
	//		entityId - Entity identifier.
	//		destroyData - True to destroy the data, false otherwise.
	// Description:
	//		Aborts a respawning for the specified entity.
	int AbortEntityRespawn(IFunctionHandler *pH, ScriptHandle entityId, bool destroyData);

	// <title ScheduleEntityRemoval>
	// Syntax: GameRules.ScheduleEntityRemoval( ScriptHandle entityId, float timer, bool visibility )
	// Arguments:
	//		entityId	- Entity identifier.
	//		timer		- Float value for the time of the entity removal.
	//		visibility	- Removal visibility. 
	// Description:
	//		Schedules the removal of the specified entity.
	int ScheduleEntityRemoval(IFunctionHandler *pH, ScriptHandle entityId, float timer, bool visibility);
	// <title AbortEntityRemoval>
	// Syntax: GameRules.AbortEntityRemoval( ScriptHandle entityId )
	// Arguments:
	//		entityId - Entity identifier.
	// Description:
	//		Aborts the entity removal.
	int AbortEntityRemoval(IFunctionHandler *pH, ScriptHandle entityId);

	// <title SetSynchedGlobalValue>
	// Syntax: GameRules.SetSynchedGlobalValue( int key , value)
	// Arguments:
	//		key		- Key for the sync global value.
	//		value	- Value we want to set. The function recognize the type of the value.
	// Description:
	//		Sets a sync global value.
	int SetSynchedGlobalValue(IFunctionHandler *pH, int key);
	// <title GetSynchedGlobalValue>
	// Syntax: GameRules.GetSynchedGlobalValue( int key )
	// Arguments:
	//		key - Key for the sync global value.
	// Description:
	//		Gets the sync global value from its key.
	int GetSynchedGlobalValue(IFunctionHandler *pH, int key);

	// <title SetSynchedEntityValue>
	// Syntax: GameRules.SetSynchedEntityValue( ScriptHandle entityId, int key )
	// Arguments:
	//		entityId - Entity identifier.
	//		key		 - Key for the sync entity value.
	// Description:
	//		Sets a sync entity value.
	int SetSynchedEntityValue(IFunctionHandler *pH, ScriptHandle entityId, int key);
	// <title GetSynchedEntityValue>
	// Syntax: GameRules.GetSynchedEntityValue( ScriptHandle entityId, int key )
	// Arguments:
	//		entityId - Entity identifier.
	//		key		 - Key for the sync entity value.
	// Description:
	//		Gets the specified sync entity value.
	int GetSynchedEntityValue(IFunctionHandler *pH, ScriptHandle entityId, int key);

	// <title ResetSynchedStorage>
	// Syntax: GameRules.ResetSynchedStorage()
	// Description:
	//		Resets the sync storage.
	int ResetSynchedStorage(IFunctionHandler *pH);
	// <title ForceSynchedStorageSynch>
	// Syntax: GameRules.ForceSynchedStorageSynch( int channelId )
	// Arguments:
	//		channelId - Channel identifier.
	// Description:
	//		Forces sync storage syncing.
	int ForceSynchedStorageSynch(IFunctionHandler *pH, int channelId);

	// <title IsDemoMode>
	// Syntax: GameRules.IsDemoMode()
	// Description:
	//		Checks if the game is running in demo mode.
	int IsDemoMode(IFunctionHandler *pH);


	// functions which query the console variables relevant to Crysis gamemodes.

	// <title GetTimeLimit>
	// Syntax: GameRules.GetTimeLimit()
	// Description:
	//		Gets the time limit.
	int GetTimeLimit(IFunctionHandler *pH);
	// <title GetRoundTime>
	// Syntax: GameRules.GetRoundTime()
	// Description:
	//		Gets the round time.
	int GetRoundTime(IFunctionHandler *pH);
	// <title GetPreRoundTime>
	// Syntax: GameRules.GetPreRoundTime()
	// Description:
	//		Gets the pre-round time.
	int GetPreRoundTime(IFunctionHandler *pH);
	// <title GetRoundLimit>
	// Syntax: GameRules.GetRoundLimit()
	// Description:
	//		Gets the round time limit.
	int GetRoundLimit(IFunctionHandler *pH);
	// <title GetFragLimit>
	// Syntax: GameRules.GetFragLimit()
	// Description:
	//		Gets the frag limit.
	int GetFragLimit(IFunctionHandler *pH);
	// <title GetFragLead>
	// Syntax: GameRules.GetFragLead()
	// Description:
	//		Gets the frag lead.
	int GetFragLead(IFunctionHandler *pH);
	// <title GetFriendlyFireRatio>
	// Syntax: GameRules.GetFriendlyFireRatio()
	// Description:
	//		Gets the friendly fire ratio.
	int GetFriendlyFireRatio(IFunctionHandler *pH);
	// <title GetReviveTime>
	// Syntax: GameRules.GetReviveTime()
	// Description:
	//		Gets the revive time.
	int GetReviveTime(IFunctionHandler *pH);
	// <title GetMinPlayerLimit>
	// Syntax: GameRules.GetMinPlayerLimit()
	// Description:
	//		Gets the minimum player limit.
	int GetMinPlayerLimit(IFunctionHandler *pH);
	// <title GetMinTeamLimit>
	// Syntax: GameRules.GetMinTeamLimit()
	// Description:
	//		Gets the minimum team limit.
	int GetMinTeamLimit(IFunctionHandler *pH);
	// <title GetTeamLock>
	// Syntax: GameRules.GetTeamLock()
	// Description:
	//		Gets the team lock.
	int GetTeamLock(IFunctionHandler *pH);
	
	// <title IsFrozen>
	// Syntax: GameRules.IsFrozen( ScriptHandle entityId )
	// Arguments:
	//		entityId - Entity identifier.
	// Description:
	//		Checks if the entity is frozen.
	int IsFrozen(IFunctionHandler *pH, ScriptHandle entityId);
	// <title FreezeEntity>
	// Syntax: GameRules.FreezeEntity( ScriptHandle entityId, bool freeze, bool vapor, bool force )
	// Arguments:
	//		entityId - Entity identifier.
	//		freeze	 - True if the entity is froozen, false otherwise.
	//		vapor	 - True to spawn vapor after freezing, false otherwise.
	//		force	 - True to force freezing even for the entity that implements "GetFrozen Amount",
	//				   false otherwise. The default value is false.
	int FreezeEntity(IFunctionHandler *pH, ScriptHandle entityId, bool freeze, bool vapor);
	// <title ShatterEntity>
	// Syntax: GameRules.ShatterEntity( ScriptHandle entityId, Vec3 pos, Vec3 impulse )
	// Arguments:
	//		entityId - Entity identifier.
	//		pos		 - Position vector.
	//		impulse	 - Impulse vector for the shattering.
	// Description:
	//		Shatters an entity.
	int ShatterEntity(IFunctionHandler *pH, ScriptHandle entityId, Vec3 pos, Vec3 impulse);

	// <title DebugCollisionDamage>
	// Syntax: GameRules.DebugCollisionDamage()
	// Description:
	//		Debugs collision damage.
	int DebugCollisionDamage(IFunctionHandler *pH);
	// <title DebugHits>
	// Syntax: GameRules.DebugHits()
	// Description:
	//		Debugs hits.
	int DebugHits(IFunctionHandler *pH);

	// <title SendHitIndicator>
	// Syntax: GameRules.SendHitIndicator( ScriptHandle shooterId )
	// Arguments:
	//		shooterId - Shooter identifier.
	// Description:
	//		Sends a hit indicator.
	int SendHitIndicator(IFunctionHandler* pH, ScriptHandle shooterId);
	// <title SendDamageIndicator>
	// Syntax: GameRules.SendDamageIndicator( ScriptHandle targetId, ScriptHandle shooterId, ScriptHandle weaponId )
	// Arguments:
	//		targetId	- Target identifier.
	//		shooterId	- Shooter identifier.
	//		weaponId	- Weapon identifier.
	// Description:
	//		Send a damage indicator from the shooter to the target.
	int SendDamageIndicator(IFunctionHandler* pH, ScriptHandle targetId, ScriptHandle shooterId, ScriptHandle weaponId);

	// <title IsInvulnerable>
	// Syntax: GameRules.IsInvulnerable( ScriptHandle playerId )
	// Arguments:
	//		playerId - Player identifier.
	// Description:
	//		Checks if the player is invulnerable.
	int IsInvulnerable(IFunctionHandler* pH, ScriptHandle playerId);
	// <title SetInvulnerability>
	// Syntax: GameRules.SetInvulnerability( ScriptHandle playerId, bool invulnerable )
	// Arguments:
	//		playerId		- Player identifier.
	//		invulnerable	- True to set invulnerability to the player, false to unset.
	// Description:
	//		Sets/unsets invulnerability for the specified player.
	int SetInvulnerability(IFunctionHandler* pH, ScriptHandle playerId, bool invulnerable);


	// For the power struggle tutorial.
	// <title GameOver>
	// Syntax: GameRules.GameOver( int localWinner )
	// Arguments:
	//		localWinner - Local winner ID.
	// Description:
	//		Ends the game with a local winner.
	int GameOver(IFunctionHandler* pH, int localWinner);
	// <title EnteredGame>
	// Syntax: GameRules.EnteredGame()
	// Description:
	//		Get the game rules and enter the game.
	int EnteredGame(IFunctionHandler* pH);
	// <title EndGameNear>
	// Syntax: GameRules.EndGameNear( ScriptHandle entityId )
	// Arguments:
	//		entityId - Entity identifier.
	int EndGameNear(IFunctionHandler* pH, ScriptHandle entityId);

	// <title SPNotifyPlayerKill>
	// Syntax: GameRules.SPNotifyPlayerKill( ScriptHandle targetId, ScriptHandle weaponId, bool bHeadShot )
	// Arguments:
	//		targetId	- Target identifier.
	//		weaponId	- Weapon identifier.
	//		bHeadShot	- True if the shot was to the head of the target, false otherwise.
	// Description:
	//		Notifies that the player kills somebody.
	int SPNotifyPlayerKill(IFunctionHandler* pH, ScriptHandle targetId, ScriptHandle weaponId, bool bHeadShot);


	// EMP Grenade

	// <title ProcessEMPEffect>
	// Syntax: GameRules.ProcessEMPEffect( ScriptHandle targetId, float timeScale )
	// Arguments:
	//		targetId  - Target identifier.
	//		timeScale - Time scale.
	// Description:
	//		Processes the EMP (Electro Magnetic Pulse) effect.
	int ProcessEMPEffect(IFunctionHandler* pH, ScriptHandle targetId, float timeScale);
	// <title PerformDeadHit>
	// Syntax: GameRules.PerformDeadHit()
	// Description:
	//		Performs a death hit.
	int PerformDeadHit(IFunctionHandler* pH);

private:
	void RegisterGlobals();
	void RegisterMethods();

	CGameRules *GetGameRules(IFunctionHandler *pH);
	CActor *GetActor(EntityId id);

	SmartScriptTable	m_players;
	SmartScriptTable	m_teamplayers;
	SmartScriptTable	m_spawnlocations;
	SmartScriptTable	m_spectatorlocations;
	SmartScriptTable	m_spawngroups;

	ISystem						*m_pSystem;
	IScriptSystem			*m_pSS;
	IGameFramework		*m_pGameFW;
};


#endif //__SCRIPTBIND_GAMERULES_H__
