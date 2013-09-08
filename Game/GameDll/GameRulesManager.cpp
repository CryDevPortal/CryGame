/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2012.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 

	-------------------------------------------------------------------------
	History:
	- 06:07:2012  : Created by Michiel Meesters

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesManager.h"
#include "GameRules.h"

#define GAMERULES_DEFINITIONS_XML_PATH		"Scripts/GameRules/GameModes.xml"

//------------------------------------------------------------------------
CGameRulesManager* CGameRulesManager::s_pInstance = NULL;

//------------------------------------------------------------------------
CGameRulesManager* CGameRulesManager::GetInstance( bool create /*= true*/ )
{
	if (create && !s_pInstance)
	{
		s_pInstance = new CGameRulesManager();
	}
	return s_pInstance;
}

//------------------------------------------------------------------------
CGameRulesManager::CGameRulesManager()
{
	assert(!s_pInstance);
	s_pInstance = this;
}

//------------------------------------------------------------------------
CGameRulesManager::~CGameRulesManager()
{
	assert(s_pInstance == this);
	s_pInstance = NULL;
}

//------------------------------------------------------------------------
void CGameRulesManager::Init()
{
	XmlNodeRef root = gEnv->pSystem->LoadXmlFromFile( GAMERULES_DEFINITIONS_XML_PATH );
	if (root)
	{
		if (!stricmp(root->getTag(), "Modes"))
		{
			IGameRulesSystem *pGameRulesSystem = g_pGame->GetIGameFramework()->GetIGameRulesSystem();

			int numModes = root->getChildCount();

			for (int i = 0; i < numModes; ++ i)
			{
				XmlNodeRef modeXml = root->getChild(i);

				if (!stricmp(modeXml->getTag(), "GameMode"))
				{
					const char *modeName;

					if (modeXml->getAttr("name", &modeName))
					{
						pGameRulesSystem->RegisterGameRules(modeName, "GameRules");

						SGameRulesData gameRulesData;

						int numModeChildren = modeXml->getChildCount();
						for (int j = 0; j < numModeChildren; ++ j)
						{
							XmlNodeRef modeChildXml = modeXml->getChild(j);

							const char *nodeTag = modeChildXml->getTag();
							if (!stricmp(nodeTag, "Alias"))
							{
								const char *alias;
								if (modeChildXml->getAttr("name", &alias))
								{
									pGameRulesSystem->AddGameRulesAlias(modeName, alias);
								}
							}
							else if (!stricmp(nodeTag, "LevelLocation"))
							{
								const char *path;
								if (modeChildXml->getAttr("path", &path))
								{
									pGameRulesSystem->AddGameRulesLevelLocation(modeName, path);
								}
							}
							else if (!stricmp(nodeTag, "Rules"))
							{
								const char *path;
								if (modeChildXml->getAttr("path", &path))
								{
									gameRulesData.m_rulesXMLPath = path;
								}
							}
							else if( !stricmp(nodeTag, "DefaultHudState"))
							{
								const char *name;
								if (modeChildXml->getAttr("name", &name))
								{
									gameRulesData.m_defaultHud = name;
								}
							}
						}

						// Check if we're a team game
						if (!gameRulesData.m_rulesXMLPath.empty())
						{
							XmlNodeRef rulesXml = gEnv->pSystem->LoadXmlFromFile( gameRulesData.m_rulesXMLPath.c_str() );
							if (rulesXml)
							{
								XmlNodeRef teamXml = rulesXml->findChild("Teams");
								gameRulesData.m_bIsTeamGame = (teamXml != (IXmlNode*)NULL);
							}
						}
										// Insert gamerule specific data
						m_rulesData.insert(TDataMap::value_type(modeName, gameRulesData));
					}
					else
					{
						CryLogAlways("CGameRulesModulesManager::Init(), invalid 'GameMode' node, requires 'name' attribute");
					}
				}
				else
				{
					CryLogAlways("CGameRulesModulesManager::Init(), invalid xml, expected 'GameMode' node, got '%s'", modeXml->getTag());
				}
			}
		}
		else
		{
			CryLogAlways("CGameRulesModulesManager::Init(), invalid xml, expected 'Modes' node, got '%s'", root->getTag());
		}
	}
}

//------------------------------------------------------------------------
const char * CGameRulesManager::GetXmlPath( const char *gameRulesName ) const
{
	TDataMap::const_iterator it = m_rulesData.find(gameRulesName);
	if (it == m_rulesData.end())
	{
		return NULL;
	}
	return it->second.m_rulesXMLPath.c_str();
}

//------------------------------------------------------------------------
const char * CGameRulesManager::GetDefaultHud( const char *gameRulesName ) const
{
	TDataMap::const_iterator it = m_rulesData.find(gameRulesName);
	if (it == m_rulesData.end())
	{
		return NULL;
	}
	return it->second.m_defaultHud.c_str();
}

//------------------------------------------------------------------------
int CGameRulesManager::GetRulesCount()
{
	return m_rulesData.size();
}

//------------------------------------------------------------------------
const char* CGameRulesManager::GetRules(int index)
{
	assert (index >= 0 && index < (int)m_rulesData.size());
	TDataMap::const_iterator iter = m_rulesData.begin();
	std::advance(iter, index);
	return iter->first.c_str();
}

//------------------------------------------------------------------------
bool CGameRulesManager::IsTeamGame( const char *gameRulesName) const
{
	TDataMap::const_iterator it = m_rulesData.find(gameRulesName);
	if (it == m_rulesData.end())
	{
		return false;
	}
	return it->second.m_bIsTeamGame;
}

bool CGameRulesManager::IsValidGameRules(const char *gameRulesName) const
{
	TDataMap::const_iterator it = m_rulesData.find(gameRulesName);
	if (it == m_rulesData.end())
	{
		return false;
	}
	return true;
}
