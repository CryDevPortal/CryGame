////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2011.
// -------------------------------------------------------------------------
//  File name:   UIHUD3D.cpp
//  Version:     v1.00
//  Created:     22/11/2011 by Paul Reindell.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "UIHUD3D.h"

#include <IEntitySystem.h>
#include <IMaterial.h>

#include "UIManager.h"
#include "Game.h"
#include "GameCVars.h"
#include "Actor.h"

#include <IGameFramework.h>
#include <IGameRulesSystem.h>

#define HUD3D_PREFAB_LIB   "Prefabs/HUD.xml"
#define HUD3D_PREFAB_NAME  "HUD.HUD_3D"

////////////////////////////////////////////////////////////////////////////
CUIHUD3D::CUIHUD3D()
: m_pHUDRootEntity(NULL)
, m_fHudDist(1.1f)
, m_fHudZOffset(-0.1f)
, m_HUDRootEntityId(0)
{
}

////////////////////////////////////////////////////////////////////////////
void CUIHUD3D::InitEventSystem()
{
	assert(gEnv->pSystem);
	if (gEnv->pSystem)
		gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener( this );

#if !defined(PS3) && !defined(XENON)
	gEnv->pFlashUI->RegisterModule(this, "CUIHUD3D");

	m_Offsets.push_back( SHudOffset(1.2f,  1.4f, -0.2f) );
	m_Offsets.push_back( SHudOffset(1.33f, 1.3f, -0.1f) );
	m_Offsets.push_back( SHudOffset(1.6f,  1.1f, -0.1f) );
	m_Offsets.push_back( SHudOffset(1.77f, 1.0f,  0.0f) );
#endif

	ICVar* pShowHudVar = gEnv->pConsole->GetCVar("g_showHud");
	if (pShowHudVar)
		pShowHudVar->SetOnChangeCallback( &OnVisCVarChange );
}

////////////////////////////////////////////////////////////////////////////
void CUIHUD3D::UnloadEventSystem()
{
	if (gEnv->pSystem)
		gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener( this );

#if !defined(PS3) && !defined(XENON)
	gEnv->pFlashUI->UnregisterModule(this);
#endif
}

////////////////////////////////////////////////////////////////////////////
void CUIHUD3D::OnSystemEvent( ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam )
{
	switch ( event )
	{
	case ESYSTEM_EVENT_LEVEL_GAMEPLAY_START:
		SpawnHudEntities();
		break;
	case ESYSTEM_EVENT_LEVEL_UNLOAD:
		m_HUDRootEntityId = 0;
		m_pHUDRootEntity = NULL;
		m_HUDEnties.clear();
		break;
	}
}

////////////////////////////////////////////////////////////////////////////
void CUIHUD3D::UpdateView(const SViewParams &viewParams)
{
	CActor* pLocalPlayer = (CActor*)g_pGame->GetIGameFramework()->GetClientActor();

	if (gEnv->IsEditor() && !gEnv->IsEditing() && !m_pHUDRootEntity)
		SpawnHudEntities();

	if (m_pHUDRootEntity && pLocalPlayer)
	{
		Vec3 position = viewParams.position;
		const Vec3 viewDir = viewParams.rotation.GetColumn1();
		Vec3 vOffset(0,0,0);

		if (!pLocalPlayer->IsDead() && pLocalPlayer->GetLinkedVehicle() == NULL)
		{
			const Vec3 eyePos = pLocalPlayer->GetLocalEyePos2();
			const Vec3 cameraXAxis = viewParams.rotation.GetColumn0();
			const float difZ = (eyePos.z - 1.68f) * 0.4f;
			const float difX = (eyePos.x - 0.12f) * 0.15f;

			vOffset += cameraXAxis * difX;
			vOffset += Vec3(0, 0, difZ);
		}

		const float distance = g_pGameCVars->g_hud3D_cameraOverride ? g_pGameCVars->g_hud3d_cameraDistance : m_fHudDist;
		const float offset = g_pGameCVars->g_hud3D_cameraOverride ? g_pGameCVars->g_hud3d_cameraOffsetZ  : m_fHudZOffset;
		vOffset += Vec3(0, 0, offset);

		position += vOffset;
		position += viewDir * distance;

		static const Quat rot90Deg = Quat::CreateRotationXYZ( Ang3(gf_PI * 0.5f, 0, 0) );
		const Quat rotation = viewParams.rotation * rot90Deg; // rotate 90 degrees around X-Axis

		m_pHUDRootEntity->SetPos(position);
		m_pHUDRootEntity->SetRotation(rotation);
	}
}

////////////////////////////////////////////////////////////////////////////
void CUIHUD3D::Reload()
{
	if (gEnv->IsEditor())
	{
		RemoveHudEntities();
	}
}

////////////////////////////////////////////////////////////////////////////
void CUIHUD3D::Update(float fDeltaTime)
{
	static int width = -1;
	static int height = -1;
	int currWidth = 1;
	int currHeight = 1;

	if (gEnv->IsEditor())
	{
		int x, y;
		gEnv->pRenderer->GetViewport( &x, &y, &currWidth, &currHeight );
	}
	else
	{
		static ICVar* pCVarWidth = gEnv->pConsole->GetCVar("r_Width");
		static ICVar* pCVarHeight = gEnv->pConsole->GetCVar("r_Height");
		currWidth = pCVarWidth ? pCVarWidth->GetIVal() : 1;
		currHeight = pCVarHeight ? pCVarHeight->GetIVal() : 1;
	}

	if (currWidth != width || currHeight != height)
	{
		width = currWidth;
		height = currHeight;
		float aspect = (float) width / (float) height;
		THudOffset::const_iterator foundit = m_Offsets.end();
		for (THudOffset::const_iterator it = m_Offsets.begin(); it != m_Offsets.end(); ++it)
		{
			if (foundit == m_Offsets.end() || fabs(it->Aspect - aspect) < fabs(foundit->Aspect - aspect))
				foundit = it;
		}
		if (foundit != m_Offsets.end())
		{
			m_fHudDist = foundit->HudDist;
			m_fHudZOffset = foundit->HudZOffset;
		}
	}
}

////////////////////////////////////////////////////////////////////////////
void CUIHUD3D::SpawnHudEntities()
{
	RemoveHudEntities();

	if (gEnv->IsEditor() && gEnv->IsEditing())
		return;

	const char* hudprefab = NULL;
	IGameRules* pGameRules = gEnv->pGame->GetIGameFramework()->GetIGameRulesSystem()->GetCurrentGameRules();
	if (pGameRules)
	{
		IScriptTable* pTable = pGameRules->GetEntity()->GetScriptTable();
		if (pTable)
		{
			if (!pTable->GetValue("hud_prefab", hudprefab))
				hudprefab = NULL;
		}
	}
	hudprefab = hudprefab ? hudprefab : HUD3D_PREFAB_LIB;

	XmlNodeRef node = gEnv->pSystem->LoadXmlFromFile(hudprefab);
	if (node)
	{
		// get the prefab with the name defined in HUD3D_PREFAB_NAME
		XmlNodeRef prefab = NULL;
		for (int i = 0; i < node->getChildCount(); ++i)
		{
			const char* name = node->getChild(i)->getAttr("Name");
			if (name && strcmp(name, HUD3D_PREFAB_NAME) == 0)
			{
				prefab = node->getChild(i);
				prefab = prefab ? prefab->findChild("Objects") : XmlNodeRef();
				break;
			}
		}

		if (prefab)
		{
			// get the PIVOT entity and collect childs
			XmlNodeRef pivotNode = NULL;
			std::vector<XmlNodeRef> childs;
			const int count = prefab->getChildCount();
			childs.reserve(count-1);

			for (int i = 0; i < count; ++i)
			{
				const char* name = prefab->getChild(i)->getAttr("Name");
				if (strcmp("PIVOT", name) == 0)
				{
					assert(pivotNode == NULL);
					pivotNode = prefab->getChild(i);
				}
				else
				{
					childs.push_back(prefab->getChild(i));
				}
			}
			
			if (pivotNode)
			{
				// spawn pivot entity
				IEntityClass* pEntClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass( pivotNode->getAttr("EntityClass") );
				if (pEntClass)
				{
					SEntitySpawnParams params;
					params.nFlags = ENTITY_FLAG_CLIENT_ONLY;
					params.pClass = pEntClass;
					m_pHUDRootEntity = gEnv->pEntitySystem->SpawnEntity(params);
				}

				if (!m_pHUDRootEntity) return;

				m_HUDRootEntityId = m_pHUDRootEntity->GetId();

				// spawn the childs and link to the pivot enity
				for (std::vector<XmlNodeRef>::iterator it = childs.begin(); it != childs.end(); ++it)
				{
					XmlNodeRef child = *it;
					pEntClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass( child->getAttr("EntityClass") );
					if (pEntClass)
					{
						const char* material = child->getAttr("Material");
						Vec3 pos;
						Vec3 scale;
						Quat rot;
						child->getAttr("Pos", pos);
						child->getAttr("Rotate", rot);
						child->getAttr("Scale", scale);

						SEntitySpawnParams params;
						params.nFlags = ENTITY_FLAG_CLIENT_ONLY;
						params.pClass = pEntClass;
						params.vPosition = pos;
						params.qRotation = rot;
						params.vScale = scale;
						IEntity* pEntity = gEnv->pEntitySystem->SpawnEntity(params);
						if (pEntity)
						{
							IScriptTable* pScriptTable = pEntity->GetScriptTable();
							if (pScriptTable)
							{
								SmartScriptTable probs;
								pScriptTable->GetValue("Properties", probs);

								XmlNodeRef properties = child->findChild("Properties");
								if (probs && properties)
								{
									for (int k = 0; k < properties->getNumAttributes(); ++k)
									{
										const char* sKey;
										const char* sVal;
										properties->getAttributeByIndex(k, &sKey, &sVal);
										probs->SetValue(sKey, sVal);
									}
								}
								Script::CallMethod(pScriptTable,"OnPropertyChange");
							}

							if (material)
							{
								IMaterial* pMat = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(material);
								if (pMat)
									pEntity->SetMaterial(pMat);
							}
							m_pHUDRootEntity->AttachChild(pEntity);
							m_HUDEnties.push_back( pEntity->GetId() );
						}
					}
				}
			}
		}
	}

	OnVisCVarChange( NULL );
}

////////////////////////////////////////////////////////////////////////////
void CUIHUD3D::RemoveHudEntities()
{
	if (m_HUDRootEntityId)
		gEnv->pEntitySystem->RemoveEntity(m_HUDRootEntityId, true);
	for (THUDEntityList::const_iterator it = m_HUDEnties.begin(); it != m_HUDEnties.end(); ++it)
		gEnv->pEntitySystem->RemoveEntity(*it, true);

	m_pHUDRootEntity = NULL;
	m_HUDRootEntityId = 0;
	m_HUDEnties.clear();
}

////////////////////////////////////////////////////////////////////////////
void CUIHUD3D::SetVisible( bool visible )
{
	IEntity* pHUDRootEntity = gEnv->pEntitySystem->GetEntity(m_HUDRootEntityId);
	if (pHUDRootEntity)
		pHUDRootEntity->Invisible(!visible);
	for (THUDEntityList::const_iterator it = m_HUDEnties.begin(); it != m_HUDEnties.end(); ++it)
	{
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(*it);
		if (pEntity)
			pEntity->Invisible(!visible);
	}
}

////////////////////////////////////////////////////////////////////////////
bool CUIHUD3D::IsVisible() const
{
	return m_pHUDRootEntity ? !m_pHUDRootEntity->IsInvisible() : false;
}

////////////////////////////////////////////////////////////////////////////
void CUIHUD3D::OnVisCVarChange( ICVar * )
{
	bool bVisible = g_pGameCVars->g_showHud > 0;

	CUIHUD3D* pHud3D = UIEvents::Get<CUIHUD3D>();
	if (pHud3D)
		pHud3D->SetVisible( bVisible );

	if (gEnv->pFlashUI)
		gEnv->pFlashUI->SetHudElementsVisible(bVisible);
}

////////////////////////////////////////////////////////////////////////////
REGISTER_UI_EVENTSYSTEM( CUIHUD3D );
