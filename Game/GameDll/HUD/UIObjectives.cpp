////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2011.
// -------------------------------------------------------------------------
//  File name:   UIObjectives.cpp
//  Version:     v1.00
//  Created:     20/1/2011 by Paul Reindell.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "UIObjectives.h"
#include "GameRules.h"
#include "Actor.h"

////////////////////////////////////////////////////////////////////////////
CUIObjectives::CUIObjectives()
	: m_pUIEvents(NULL)
	, m_pUIFunctions(NULL)
{
}

////////////////////////////////////////////////////////////////////////////
void CUIObjectives::InitEventSystem()
{
	if ( gEnv->pFlashUI )
	{
		// events that be fired to the UI
		m_pUIEvents = gEnv->pFlashUI->CreateEventSystem( "UIObjectives", IUIEventSystem::eEST_SYSTEM_TO_UI );
		m_eventSender.Init(m_pUIEvents);

		{
			SUIEventDesc evtDesc("ObjectiveAdded", "Mission objective added");
			evtDesc.AddParam<SUIParameterDesc::eUIPT_Int>("MissionID", "ID of the mission" );
			evtDesc.AddParam<SUIParameterDesc::eUIPT_String>("Name", "Name of the mission");
			evtDesc.AddParam<SUIParameterDesc::eUIPT_String>("Desc", "Description of the mission");
			evtDesc.AddParam<SUIParameterDesc::eUIPT_String>("State", "State of the objective");
			m_eventSender.RegisterEvent<eUIOE_ObjectiveAdded>(evtDesc);
		}

		{
			SUIEventDesc evtDesc( "ObjectiveRemoved", "Mission objective removed" );
			evtDesc.AddParam<SUIParameterDesc::eUIPT_Int>("MissionID", "ID of the mission" );
			m_eventSender.RegisterEvent<eUIOE_ObjectiveRemoved>(evtDesc);
		}

		{
			SUIEventDesc evtDesc( "ObjectivesReset", "All mission objectives reset" );
			m_eventSender.RegisterEvent<eUIOE_ObjectivesReset>(evtDesc);
		}

		{
			SUIEventDesc evtDesc( "ObjectiveStateChanged", "Objective status changed" );
			evtDesc.AddParam<SUIParameterDesc::eUIPT_Int>("MissionID", "ID of the mission" );
			evtDesc.AddParam<SUIParameterDesc::eUIPT_String>("State", "State of the objective");
			m_eventSender.RegisterEvent<eUIOE_ObjectiveStateChanged>(evtDesc);
		}

		// event system to receive events from UI
		m_pUIFunctions = gEnv->pFlashUI->CreateEventSystem( "UIObjectives", IUIEventSystem::eEST_UI_TO_SYSTEM );
		m_eventDispatcher.Init(m_pUIFunctions, this, "CUIObjectives");

		{
			SUIEventDesc evtDesc( "RequestObjectives", "Request all mission objectives (force to call ObjectiveAdded for each objective)" );
			m_eventDispatcher.RegisterEvent( evtDesc, &CUIObjectives::OnRequestMissionObjectives );
		}
	}
	UpdateObjectiveInfo();
}

////////////////////////////////////////////////////////////////////////////
void CUIObjectives::UnloadEventSystem()
{
}

////////////////////////////////////////////////////////////////////////////
// functions that generate events for the UI 
////////////////////////////////////////////////////////////////////////////
void CUIObjectives::MissionObjectiveAdded( const string& objectiveID, int state )
{
	if ( gEnv->IsEditor() )
	{
		UpdateObjectiveInfo();
	}
	SMissionObjectiveInfo* pInfo = GetMissionObjectiveInfo( objectiveID );
	if ( pInfo )
	{
		m_eventSender.SendEvent<eUIOE_ObjectiveAdded>(objectiveID, pInfo->Name, pInfo->Desc, state);
	}
}

//--------------------------------------------------------------------------------------------
void CUIObjectives::MissionObjectiveRemoved( const string& objectiveID )
{
	SMissionObjectiveInfo* pInfo = GetMissionObjectiveInfo( objectiveID );
	if ( pInfo )
	{
		m_eventSender.SendEvent<eUIOE_ObjectiveRemoved>(objectiveID);
	}
}

//--------------------------------------------------------------------------------------------
void CUIObjectives::MissionObjectivesReset()
{
	m_eventSender.SendEvent<eUIOE_ObjectivesReset>();
}

//--------------------------------------------------------------------------------------------
void CUIObjectives::MissionObjectiveStateChanged( const string& objectiveID, int state )
{
	SMissionObjectiveInfo* pInfo = GetMissionObjectiveInfo( objectiveID );
	if ( pInfo )
	{
		m_eventSender.SendEvent<eUIOE_ObjectiveStateChanged>(objectiveID, state);
	}
}

////////////////////////////////////////////////////////////////////////////
// events that are fired by the UI
////////////////////////////////////////////////////////////////////////////
void CUIObjectives::OnRequestMissionObjectives()
{
	CGameRules* pGameRules = GetGameRules();
	if ( pGameRules && g_pGame->GetIGameFramework()->GetClientActor() )
	{
		CActor* pActor = (CActor*)pGameRules->GetActorByChannelId( g_pGame->GetIGameFramework()->GetClientActor()->GetChannelId() );
		if ( pActor )
		{
			std::map< string, int > tmpList;
			int teamID = pGameRules->GetTeam( pActor->GetEntityId() );
			for ( TObjectiveMap::iterator it = m_ObjectiveMap.begin(); it != m_ObjectiveMap.end(); ++it )
			{
				CGameRules::TObjective* pObjective = pGameRules->GetObjective( teamID, it->first.c_str() );
				if ( pObjective )
					tmpList[ it->first ] = pObjective->status;
			}
			for ( std::map< string, int >::iterator it = tmpList.begin(); it != tmpList.end(); ++it )
				MissionObjectiveAdded( it->first, it->second );
		}
	}

}

////////////////////////////////////////////////////////////////////////////
// private functions
////////////////////////////////////////////////////////////////////////////
void CUIObjectives::UpdateObjectiveInfo()
{
	m_ObjectiveMap.clear();

	string path = "Libs/UI/Objectives_new.xml";
	XmlNodeRef missionObjectives = GetISystem()->LoadXmlFromFile( path.c_str() );
	if (missionObjectives == 0)
	{
		gEnv->pLog->LogError("Error while loading MissionObjective file '%s'", path.c_str() );
		return;
	}

	for(int tag = 0; tag < missionObjectives->getChildCount(); ++tag)
	{
		XmlNodeRef mission = missionObjectives->getChild(tag);
		const char* attrib;
		const char* objective;
		const char* text;
		for(int obj = 0; obj < mission->getChildCount(); ++obj)
		{
			XmlNodeRef objectiveNode = mission->getChild(obj);
			string id(mission->getTag());
			id += ".";
			id += objectiveNode->getTag();
			if(objectiveNode->getAttributeByIndex(0, &attrib, &objective) && objectiveNode->getAttributeByIndex(1, &attrib, &text))
			{
				m_ObjectiveMap[ id ].Name = objective;
				m_ObjectiveMap[ id ].Desc = text;
			}
			else
			{
				gEnv->pLog->LogError("Error while loading MissionObjective file '%s'", path.c_str() );
				return;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////
CUIObjectives::SMissionObjectiveInfo* CUIObjectives::GetMissionObjectiveInfo( const string& objectiveID, bool bLogError )
{
	TObjectiveMap::iterator it = m_ObjectiveMap.find( objectiveID );
	if ( it != m_ObjectiveMap.end() )
	{
		return &it->second;
	}
	if ( bLogError )
		gEnv->pLog->LogError( "[UIObjectives] Mission Objective \"%s\" is not defined!", objectiveID.c_str() );
	return NULL;
}

////////////////////////////////////////////////////////////////////////////
CGameRules* CUIObjectives::GetGameRules()
{
	return static_cast<CGameRules *>( g_pGame->GetIGameFramework()->GetIGameRulesSystem()->GetCurrentGameRules() );
}

////////////////////////////////////////////////////////////////////////////
REGISTER_UI_EVENTSYSTEM( CUIObjectives );