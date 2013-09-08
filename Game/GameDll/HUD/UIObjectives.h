////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2011.
// -------------------------------------------------------------------------
//  File name:   UIObjectives.h
//  Version:     v1.00
//  Created:     20/1/2011 by Paul Reindell.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __UIObjectives_H__
#define __UIObjectives_H__

#include "IUIGameEventSystem.h"
#include <IFlashUI.h>

class CGameRules;

class CUIObjectives	
	: public IUIGameEventSystem
{
public:
	CUIObjectives();

	// IUIGameEventSystem
	UIEVENTSYSTEM( "UIObjectives" );
	virtual void InitEventSystem();
	virtual void UnloadEventSystem();

	// these functions are called by CGameRules ( mission objective related functions )
	void MissionObjectiveAdded( const string& objectiveID, int state );
	void MissionObjectiveRemoved( const string& objectiveID );
	void MissionObjectivesReset();
	void MissionObjectiveStateChanged( const string& objectiveID, int state );

private:
	enum EUIObjectiveEvent
	{
		eUIOE_ObjectiveAdded,
		eUIOE_ObjectiveRemoved,
		eUIOE_ObjectivesReset,
		eUIOE_ObjectiveStateChanged,
	};

	SUIEventReceiverDispatcher<CUIObjectives> m_eventDispatcher;
	SUIEventSenderDispatcher<EUIObjectiveEvent> m_eventSender;
	IUIEventSystem* m_pUIEvents;
	IUIEventSystem* m_pUIFunctions;

	// mission objectives info
	struct SMissionObjectiveInfo
	{
		string Name;
		string Desc;
	};
	typedef std::map< string, SMissionObjectiveInfo > TObjectiveMap;
	TObjectiveMap m_ObjectiveMap;

private:
	void UpdateObjectiveInfo();
	SMissionObjectiveInfo* GetMissionObjectiveInfo( const string& objectiveID, bool bLogError = true );
	CGameRules* GetGameRules();

	void OnRequestMissionObjectives();
};

#endif
