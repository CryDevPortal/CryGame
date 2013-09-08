/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a flow nodes for vehicles in Game02

-------------------------------------------------------------------------
History:
- 12:12:2005: Created by Mathieu Pinard

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "CryAction.h"
#include "IVehicleSystem.h"
#include "IFlowSystem.h"
#include "Nodes/G2FlowBaseNode.h"
#include "VehicleActionEntityAttachment.h"
#include "GameCVars.h"
#include "IItemSystem.h"
#include "WeaponSystem.h"
#include "Projectile.h"

class CVehicleActionEntityAttachment;

//------------------------------------------------------------------------
class CFlowVehicleEntityAttachment
: public CFlowBaseNode<eNCT_Instanced>
{
public:

	CFlowVehicleEntityAttachment(SActivationInfo* pActivationInfo);
	~CFlowVehicleEntityAttachment() {}

	// CFlowBaseNode
	virtual IFlowNodePtr Clone(SActivationInfo* pActivationInfo);
	virtual void GetConfiguration(SFlowNodeConfig& nodeConfig);
	virtual void ProcessEvent(EFlowEvent flowEvent, SActivationInfo* pActivationInfo);
	virtual void Serialize(SActivationInfo* pActivationInfo, TSerialize ser);

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}
	// ~CFlowBaseNode

protected:

	IVehicle* GetVehicle();
	CVehicleActionEntityAttachment* GetVehicleAction();

	IFlowGraph *m_pGraph;
	TFlowNodeId m_nodeID;

	EntityId m_vehicleId;

	enum EInputs
	{
		IN_DROPATTACHMENTTRIGGER,
	};

	enum EOutputs
	{
		OUT_ENTITYID,
		OUT_ISATTACHED,
	};
};

//------------------------------------------------------------------------
CFlowVehicleEntityAttachment::CFlowVehicleEntityAttachment(SActivationInfo* pActivationInfo)
{
	m_nodeID = pActivationInfo->myID;
	m_pGraph = pActivationInfo->pGraph;

	if (IEntity* pEntity = pActivationInfo->pEntity)
	{
		IVehicleSystem* pVehicleSystem = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem();
		assert(pVehicleSystem);

		if (pVehicleSystem->GetVehicle(pEntity->GetId()))
			m_vehicleId = pEntity->GetId();
	}
	else
		m_vehicleId = 0;
}

//------------------------------------------------------------------------
IFlowNodePtr CFlowVehicleEntityAttachment::Clone(SActivationInfo* pActivationInfo)
{
	return new CFlowVehicleEntityAttachment(pActivationInfo);
}

void CFlowVehicleEntityAttachment::Serialize(SActivationInfo* pActivationInfo, TSerialize ser)
{
	// MATHIEU: FIXME!
}

//------------------------------------------------------------------------
void CFlowVehicleEntityAttachment::GetConfiguration(SFlowNodeConfig& nodeConfig)
{
	static const SInputPortConfig pInConfig[] = 
	{
		InputPortConfig_Void("DropAttachmentTrigger", _HELP("Trigger to drop the attachment")),
		InputPortConfig_Null()
	};

	static const SOutputPortConfig pOutConfig[] = 
	{
		OutputPortConfig<int>("EntityId", _HELP("Entity Id of the attachment")),
		OutputPortConfig<bool>("IsAttached", _HELP("If the attachment is still attached")),
		OutputPortConfig_Null()
	};

	nodeConfig.sDescription = _HELP("Handle the entity attachment used as vehicle action");
	nodeConfig.nFlags |= EFLN_TARGET_ENTITY;
	nodeConfig.pInputPorts = pInConfig;
	nodeConfig.pOutputPorts = pOutConfig;
	nodeConfig.SetCategory(EFLN_APPROVED);
}

//------------------------------------------------------------------------
void CFlowVehicleEntityAttachment::ProcessEvent(EFlowEvent flowEvent, SActivationInfo* pActivationInfo)
{
	if (flowEvent == eFE_SetEntityId)
	{
		if (IEntity* pEntity = pActivationInfo->pEntity)
		{
			IVehicleSystem* pVehicleSystem = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem();
			assert(pVehicleSystem);

			if (pEntity->GetId() != m_vehicleId)
				m_vehicleId = 0;

			if (IVehicle* pVehicle = pVehicleSystem->GetVehicle(pEntity->GetId()))
				m_vehicleId = pEntity->GetId();
		}
		else
		{
			m_vehicleId = 0;
		}
	}
	else if (flowEvent == eFE_Activate)
	{
		if (CVehicleActionEntityAttachment* pAction = GetVehicleAction())
		{
			if (IsPortActive(pActivationInfo, IN_DROPATTACHMENTTRIGGER))
			{
				pAction->DetachEntity();

				SFlowAddress addr(m_nodeID, OUT_ISATTACHED, true);
				m_pGraph->ActivatePort(addr, pAction->IsEntityAttached());
			}

			SFlowAddress addr(m_nodeID, OUT_ENTITYID, true);
			m_pGraph->ActivatePort(addr, pAction->GetAttachmentId());
		}
	}
}

//------------------------------------------------------------------------
IVehicle* CFlowVehicleEntityAttachment::GetVehicle()
{
	if (!m_vehicleId)
		return NULL;

	IVehicleSystem* pVehicleSystem = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem();
	assert(pVehicleSystem);

	return pVehicleSystem->GetVehicle(m_vehicleId);
}

//------------------------------------------------------------------------
CVehicleActionEntityAttachment* CFlowVehicleEntityAttachment::GetVehicleAction()
{
	if (!m_vehicleId)
		return NULL;

	IVehicleSystem* pVehicleSystem = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem();
	assert(pVehicleSystem);

	if (IVehicle* pVehicle = pVehicleSystem->GetVehicle(m_vehicleId))
	{
		for (int i = 1; i < pVehicle->GetActionCount(); i++)
		{
			IVehicleAction* pAction = pVehicle->GetAction(i);
			assert(pAction);

			if (CVehicleActionEntityAttachment* pAttachment = 
				CAST_VEHICLEOBJECT(CVehicleActionEntityAttachment, pAction))
			{
				return pAttachment;
			}
		}
	}

	return NULL;
}

//------------------------------------------------------------------------
class CFlowVehicleSetAltitudeLimit
	: public CFlowBaseNode<eNCT_Instanced>
{
public:

	CFlowVehicleSetAltitudeLimit(SActivationInfo* pActivationInfo) {};
	~CFlowVehicleSetAltitudeLimit() {}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

	virtual IFlowNodePtr Clone( SActivationInfo *pActInfo )
	{
			return new CFlowVehicleSetAltitudeLimit(pActInfo);
	}

	enum Inputs
	{
		EIP_SetLimit,
		EIP_Limit
	};

	virtual void GetConfiguration(SFlowNodeConfig& nodeConfig)
	{
		static const SInputPortConfig pInConfig[] = 
		{
			InputPortConfig_Void  ("SetLimit", _HELP("Trigger to set limit")),
			InputPortConfig<float>("Limit", _HELP("Altitude limit in meters")),
			InputPortConfig_Null()
		};

		nodeConfig.sDescription = _HELP("Set Vehicle's Maximum Altitude");
		nodeConfig.pInputPorts = pInConfig;
		nodeConfig.pOutputPorts = 0;
		nodeConfig.SetCategory(EFLN_ADVANCED);
	}

	virtual void ProcessEvent(EFlowEvent flowEvent, SActivationInfo* pActivationInfo)
	{
		if (flowEvent == eFE_Activate && IsPortActive(pActivationInfo, EIP_SetLimit))
		{
			const float fVal = GetPortFloat(pActivationInfo, EIP_Limit);
			CryFixedStringT<128> buf;
			buf.FormatFast("%g", fVal);
			g_pGameCVars->pAltitudeLimitCVar->ForceSet(buf.c_str());
		}
	}
};

//------------------------------------------------------------------------
class CFlowPlayerVehicleGetSpeed
	: public CFlowBaseNode<eNCT_Instanced>
{
public:

	CFlowPlayerVehicleGetSpeed(SActivationInfo* pActivationInfo) {};
	~CFlowPlayerVehicleGetSpeed() {}

	enum InputPorts
	{
		eI_Activate,
		eI_Paused,
	};

	enum OutputPorts
	{
		eO_Speed = 0,
	};

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

	virtual IFlowNodePtr Clone(SActivationInfo *pActInfo)
	{
		return new CFlowPlayerVehicleGetSpeed(pActInfo);
	}

	virtual void GetConfiguration(SFlowNodeConfig& nodeConfig)
	{
		static const SInputPortConfig pInConfig[] = 
		{
			InputPortConfig_Void  ( "Get", _HELP("Get current value") ),
			InputPortConfig<bool> ( "Paused", true, _HELP("Pause output") ),
			InputPortConfig_Null()
		};
			
		static const SOutputPortConfig pOutConfig[] = 
		{
			OutputPortConfig<float>( "Speed", "vehicle speed" ),
			OutputPortConfig_Null()
		};

		nodeConfig.sDescription = _HELP( "Get speed of the players vehicle" );
		nodeConfig.pInputPorts = pInConfig;
		nodeConfig.pOutputPorts = pOutConfig;
		nodeConfig.SetCategory(EFLN_ADVANCED);
	}

	f32 GetVehicleSpeed()
	{
		IActor* pActor = gEnv->pGameFramework->GetClientActor();
		SVehicleStatus state;
		if(pActor)
		{
			IVehicle* pVehicle = pActor->GetLinkedVehicle();
			if(pVehicle)
			{
				state = pVehicle->GetStatus();
				return state.vel.len();
			}
		}
		return 0.f;
	}

	virtual void ProcessEvent(EFlowEvent flowEvent, SActivationInfo* pActInfo)
	{
		switch (flowEvent)
		{	
			case eFE_Initialize:
			{
				pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, !(GetPortBool(pActInfo, eI_Paused)));
				break;
			}
		
			case eFE_Activate:
			{
				if(IsPortActive(pActInfo, eI_Activate))
					ActivateOutput(pActInfo, eO_Speed, GetVehicleSpeed());

				pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, !(GetPortBool(pActInfo, eI_Paused)));
				break;
			}

			case eFE_Update:
			{
				if(!(GetPortBool(pActInfo, eI_Paused)) && GetVehicleSpeed()>0.f)
					ActivateOutput(pActInfo, eO_Speed, GetVehicleSpeed());
				break;
			}
		}
	}
};

//------------------------------------------------------------------------
class CFlowNode_CameraThirdPersonAdjustment : public CFlowBaseNode<eNCT_Singleton>
{
public:
	CFlowNode_CameraThirdPersonAdjustment( SActivationInfo * pActInfo ) 
	{
	};

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

	enum EInputPorts
	{
		EIP_Enabled,
		EIP_Disabled,
		EIP_CamHeight,
	};

	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_AnyType( "Enabled",	"Enable Third Person Camera Adjustment" ),
			InputPortConfig_AnyType( "Disabled","Disable Third Person Camera Adjustment" ),
			InputPortConfig<float>( "Adjustment",	"Camera height adjustment" ),
			InputPortConfig_Null()
		};
		config.pInputPorts = in_config;
		config.SetCategory(EFLN_DEBUG);
	}
	virtual void ProcessEvent( EFlowEvent event,SActivationInfo *pActInfo )
	{
		switch (event)
		{	
			case eFE_Activate:
			{
				if (IsPortActive(pActInfo, EIP_Enabled) || IsPortActive(pActInfo, EIP_Disabled))
				{
					IGameFramework * pGF = gEnv->pGameFramework;

					if(!pGF || !pGF->GetClientActor()->GetLinkedVehicle())
						return;

					EntityId playerVehicle = pGF->GetClientActor()->GetLinkedVehicle()->GetEntityId();
					
					if(IsPortActive(pActInfo,EIP_Enabled))
					{
						pGF->GetIVehicleSystem()->GetVehicle(playerVehicle)->SetCameraAdjustment(GetPortFloat(pActInfo,EIP_CamHeight));
					}
					else if(IsPortActive(pActInfo, EIP_Disabled))
					{
						pGF->GetIVehicleSystem()->GetVehicle(playerVehicle)->SetCameraAdjustment(0);
					}
				}
			}
			break;
		}
	}
};

//------------------------------------------------------------------------
class CFlowPlayerVehicleToggleBoost : public CFlowBaseNode<eNCT_Instanced>
{
protected:
	EntityId m_vehicleId;
public:
	CFlowPlayerVehicleToggleBoost( SActivationInfo * pActInfo ) 
	{
		m_vehicleId = 0;
	};

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

	enum EInputPorts
	{
		EIP_Enabled,
		EIP_Disabled,
	};

	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_AnyType( "Enabled",	"Enable boosting on this vehicle" ),
			InputPortConfig_AnyType( "Disabled","Disable boosting on this vehicle" ),
			InputPortConfig_Null()
		};

		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_config;
		config.SetCategory(EFLN_APPROVED);
	}

	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return new CFlowPlayerVehicleToggleBoost(pActInfo);
	}


	virtual void ProcessEvent( EFlowEvent event,SActivationInfo *pActInfo )
	{
		switch (event)
		{	
		case eFE_SetEntityId:
			{
				if (IEntity* pEntity = pActInfo->pEntity)
				{
					IVehicleSystem* pVehicleSystem = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem();
					assert(pVehicleSystem);

					if (IVehicle* pVehicle = pVehicleSystem->GetVehicle(pEntity->GetId()))
						m_vehicleId = pEntity->GetId();
				}
				else
				{
					m_vehicleId = 0;
				}
			}
			break;
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, EIP_Enabled) || IsPortActive(pActInfo, EIP_Disabled))
				{
					IVehicleSystem* pVehicleSystem = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem();
					assert(pVehicleSystem);

					IVehicle* pVehicle = pVehicleSystem->GetVehicle(m_vehicleId);
					if(pVehicle)
					{
						IVehicleMovement* pVehicleMovement = pVehicle->GetMovement();
						if(pVehicleMovement)
						{
							if(IsPortActive(pActInfo,EIP_Enabled))
							{
								pVehicleMovement->AllowBoosting(true);
							}
							else if(IsPortActive(pActInfo, EIP_Disabled))
							{
								pVehicleMovement->AllowBoosting(false);
							}
						}
					}
				}
			}
			break;
		}
	}
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_VehicleDebugDraw : public CFlowBaseNode<eNCT_Instanced>
{

public:

	enum EInputs 
	{
		IN_SHOW,
		IN_SIZE,
		IN_PARTS,
	};

	enum EOutputs 
	{

	};

	CFlowNode_VehicleDebugDraw( SActivationInfo * pActInfo )
	{

	};

	~CFlowNode_VehicleDebugDraw()
	{

	}
	
	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return new CFlowNode_VehicleDebugDraw(pActInfo);
	}
	

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SInputPortConfig in_config[] = 
		{
			InputPortConfig_Void   ( "Trigger", _HELP("show debug informations on screen") ),
			InputPortConfig<float> ( "Size", 1.5f, _HELP("font size")),
			InputPortConfig<string>( "vehicleParts_Parts", _HELP("select vehicle parts"), 0, _UICONFIG("ref_entity=entityId") ),
			InputPortConfig_Null()
		};

		static const SOutputPortConfig out_config[] = 
		{
			OutputPortConfig_Null()
		};

		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.SetCategory(EFLN_DEBUG);
	}

	string currentParam;
	string currentSetting;

	float column1;
	float column2;
	int loops;
	
	virtual void ProcessEvent( EFlowEvent event,SActivationInfo *pActInfo )
	{
		IVehicleSystem * pVehicleSystem = NULL;
		IVehicle * pVehicle = NULL;

		switch(event)
		{
			case eFE_Initialize:
			{
				pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
				break;
			}
		
			case eFE_Activate:
			{
				if (!pActInfo->pEntity)
					return;

				pVehicleSystem = gEnv->pGameFramework->GetIVehicleSystem();
				pVehicle = pVehicleSystem->GetVehicle(pActInfo->pEntity->GetId());

				if (!pVehicleSystem || !pVehicle)
					return;

				string givenString = GetPortString(pActInfo, IN_PARTS);
				currentParam = givenString.substr(0,givenString.find_first_of(":"));
				currentSetting = givenString.substr(givenString.find_first_of(":")+1,(givenString.length()-givenString.find_first_of(":")));

				column1 = 10.f;
				column2 = 100.f;

				if (IsPortActive(pActInfo,IN_SHOW))
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);

				break;
			}
		
			case eFE_Update:
			{
				IRenderer * pRenderer = gEnv->pRenderer;

				pVehicleSystem = gEnv->pGameFramework->GetIVehicleSystem();
				pVehicle = pVehicleSystem->GetVehicle(pActInfo->pEntity->GetId());
				
				if(!pVehicleSystem || !pActInfo->pEntity || !pVehicle)
					return;

				pRenderer->Draw2dLabel(column1,10,GetPortFloat(pActInfo,IN_SIZE)+2.f,Col_Cyan,false,pActInfo->pEntity->GetName());

				if(currentParam=="Seats")
				{
					loops = 0;

					for(int i=0;i<pVehicle->GetSeatCount();i++)
					{
						IVehicleSeat * currentSeat;

						if(currentSetting=="All")
						{
							currentSeat = pVehicle->GetSeatById(i+1);
						}
						else
						{
							currentSeat = pVehicle->GetSeatById(pVehicle->GetSeatId(currentSetting));
							i = pVehicle->GetSeatCount()-1;
						}

						loops += 1;
						
						// column 1
						string pMessage = ("%s:", currentSeat->GetSeatName());

						if (column2<pMessage.size()*8*GetPortFloat(pActInfo, IN_SIZE))
							column2=pMessage.size()*8*GetPortFloat(pActInfo, IN_SIZE);
						
						pRenderer->Draw2dLabel(column1,(15*(float(loops+1))*GetPortFloat(pActInfo,IN_SIZE)),GetPortFloat(pActInfo,IN_SIZE),Col_Cyan,false,pMessage);
						
						// column 2
						if(currentSeat->GetPassenger(true))
						{
							pMessage = ("- %s", gEnv->pEntitySystem->GetEntity(currentSeat->GetPassenger(true))->GetName());
							pRenderer->Draw2dLabel(column2,(15*(float(loops+1))*GetPortFloat(pActInfo,IN_SIZE)),GetPortFloat(pActInfo,IN_SIZE),Col_Cyan,false,pMessage);
						}
					}
				}
				
				else if(currentParam=="Wheels")
				{
					pRenderer->Draw2dLabel(column1,50.f,GetPortFloat(pActInfo,IN_SIZE)+1.f,Col_Red,false,"!");
				}

				else if(currentParam=="Weapons")
				{
					loops = 0;

					for(int i=0;i<pVehicle->GetWeaponCount();i++)
					{
						IItemSystem * pItemSystem = gEnv->pGameFramework->GetIItemSystem();
						IWeapon * currentWeapon;
						EntityId currentEntityId;
						IItem * pItem;

						if(currentSetting=="All")
						{
							currentEntityId = pVehicle->GetWeaponId(i+1);
						}
						else
						{
							currentEntityId = gEnv->pEntitySystem->FindEntityByName(currentSetting)->GetId();
							i = pVehicle->GetWeaponCount()-1;
						}

						if(!pItemSystem->GetItem(currentEntityId))
							return;
						
						pItem = pItemSystem->GetItem(currentEntityId);
						currentWeapon = pItem->GetIWeapon();

						loops += 1;

						// column 1
						string pMessageName = string().Format("%s", gEnv->pEntitySystem->GetEntity(currentEntityId)->GetName());
						pRenderer->Draw2dLabel(column1,(15*(float(loops+1))*GetPortFloat(pActInfo,IN_SIZE)),GetPortFloat(pActInfo,IN_SIZE),Col_Cyan,false,pMessageName);

						if (column2<pMessageName.size()*8*GetPortFloat(pActInfo, IN_SIZE))
							column2=pMessageName.size()*8*GetPortFloat(pActInfo, IN_SIZE);

						// column 2
						string pMessageValue = string().Format("seat: %s firemode: %i", pVehicle->GetWeaponParentSeat(currentEntityId)->GetSeatName(), currentWeapon->GetCurrentFireMode()).c_str();
						pRenderer->Draw2dLabel(column2,(15*(float(loops+1))*GetPortFloat(pActInfo,IN_SIZE)),GetPortFloat(pActInfo,IN_SIZE),Col_Cyan,false,pMessageValue);
					}
				}

				else if(currentParam=="Components")
				{
					loops = 0;

					for(int i=0;i<pVehicle->GetComponentCount();i++)
					{
						IVehicleComponent * currentComponent;

						if(currentSetting=="All")
						{
							currentComponent = pVehicle->GetComponent(i);
						}
						else
						{
							currentComponent = pVehicle->GetComponent(currentSetting);
							i = pVehicle->GetComponentCount()-1;
						}

						loops += 1;

						ColorF labelColor;
						labelColor = ColorF(currentComponent->GetDamageRatio(),(1.f-currentComponent->GetDamageRatio()),0.f);
						
						// column 1
						string pMessageName = string().Format("%s", currentComponent->GetComponentName()).c_str();
						pRenderer->Draw2dLabel(column1,(15*(float(loops+1))*GetPortFloat(pActInfo,IN_SIZE)),GetPortFloat(pActInfo,IN_SIZE),labelColor,false,pMessageName);

						if (column2<pMessageName.size()*8*GetPortFloat(pActInfo, IN_SIZE))
							column2=pMessageName.size()*8*GetPortFloat(pActInfo, IN_SIZE);

						// column 2
						string pMessageValue = string().Format("%5.2f (%3.2f)", currentComponent->GetDamageRatio()*currentComponent->GetMaxDamage(), currentComponent->GetDamageRatio()).c_str();
						pRenderer->Draw2dLabel(column2,(15*(float(loops+1))*GetPortFloat(pActInfo,IN_SIZE)),GetPortFloat(pActInfo,IN_SIZE),labelColor,false,pMessageValue);
					}
				}

				else
				{
					pRenderer->Draw2dLabel(column1,50.f,GetPortFloat(pActInfo,IN_SIZE)+1.f,Col_Red,false,"no component selected!");
				}
				break;
			}
		}
	};
};

//------------------------------------------------------------------------
class CFlowPlayerVehicleID
	: public CFlowBaseNode<eNCT_Instanced>
{
public:

	CFlowPlayerVehicleID(SActivationInfo* pActivationInfo) {};
	~CFlowPlayerVehicleID() {}

	enum InputPorts
	{
		eI_Get = 0,
	};

	enum OutputPorts
	{
		eO_Id = 0,
	};

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

	virtual IFlowNodePtr Clone( SActivationInfo *pActInfo )
	{
		return new CFlowPlayerVehicleID(pActInfo);
	}

	virtual void GetConfiguration(SFlowNodeConfig& nodeConfig)
	{
		static const SInputPortConfig pInConfig[] = 
		{
			InputPortConfig_Void  ("Get", _HELP("Get id")),
			InputPortConfig_Null()
		};

		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<EntityId>( "Id", "vehicle id" ),
			OutputPortConfig_Null()
		};

		nodeConfig.sDescription = _HELP("Get id of the players vehicle");
		nodeConfig.pInputPorts = pInConfig;
		nodeConfig.pOutputPorts = out_config;
		nodeConfig.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent(EFlowEvent flowEvent, SActivationInfo* pActivationInfo)
	{
		if (flowEvent == eFE_Activate)
		{
			IActor* pActor = gEnv->pGameFramework->GetClientActor();
			EntityId vehicleId;
			if(pActor)
			{
				IVehicle* pVehicle = pActor->GetLinkedVehicle();
				if(pVehicle)
				{
					vehicleId = pVehicle->GetEntityId();
				}
			}

			ActivateOutput( pActivationInfo, eO_Id, vehicleId );
		}
	}
};


class CFlowLaunchProjectileNode: public CFlowBaseNode<eNCT_Instanced>
{
public:
	enum EInputs
	{
		IN_LAUNCH,
		IN_PROJ,
		IN_POS,
		IN_DIR,
		IN_VEL
	};
	enum EOutputs
	{
		OUT_OK
	};

	CFlowLaunchProjectileNode( SActivationInfo * pActInfo )
	{
	}

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void("Launch", _HELP("Launch projectile")),
			InputPortConfig<string>("Projectile", _HELP("Launch projectile"), 0, _UICONFIG("enum_global:ammos") ),
			InputPortConfig<Vec3>("Pos", _HELP("position of launch")),
			InputPortConfig<Vec3>("Dir", _HELP("direction for projectile")),
			InputPortConfig<Vec3>("Vel", _HELP("velocity of projectile")),
			InputPortConfig_Null()
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig_Void( "launched", _HELP("outputs after launch") ),
			OutputPortConfig_Null()
		};    
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Launches a projectile");
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Activate:
			if (IsPortActive(pActInfo, IN_LAUNCH))
			{
				string classname = GetPortString(pActInfo, IN_PROJ);
				IEntityClass *pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(classname);
				if(!pClass)
				{
					CryLogAlways("Flownode LaunchProjectile: class not found");
					return;
				}
				CProjectile* proj = g_pGame->GetWeaponSystem()->SpawnAmmo(pClass, true);
				
				if(!proj)
				{
					CryLogAlways("Flownode LaunchProjectile: class found, no projectile");
					return;
				}
				
				Vec3  pos = GetPortVec3(pActInfo, IN_POS);
				Vec3  dir = GetPortVec3(pActInfo, IN_DIR);
				Vec3  vel = GetPortVec3(pActInfo, IN_VEL);
				proj->Launch(pos, dir, vel);
				ActivateOutput(pActInfo, OUT_OK, true);
			}
		}
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

	virtual IFlowNodePtr Clone( SActivationInfo *pActInfo )
	{
		return new CFlowLaunchProjectileNode(pActInfo);
	}

};

REGISTER_FLOW_NODE("Vehicle:EntityAttachment", CFlowVehicleEntityAttachment);
REGISTER_FLOW_NODE("Game:SetVehicleAltitudeLimit", CFlowVehicleSetAltitudeLimit);
REGISTER_FLOW_NODE("Game:GetPlayerVehicleSpeed", CFlowPlayerVehicleGetSpeed);
REGISTER_FLOW_NODE("Game:LocalPlayerVehicleID", CFlowPlayerVehicleID);
REGISTER_FLOW_NODE("Vehicle:ToggleBoost", CFlowPlayerVehicleToggleBoost);

REGISTER_FLOW_NODE( "Vehicle:ThirdPersonCameraAdjustment", CFlowNode_CameraThirdPersonAdjustment );
REGISTER_FLOW_NODE( "Vehicle:DebugDraw", CFlowNode_VehicleDebugDraw );

REGISTER_FLOW_NODE( "Game:LaunchProjectile", CFlowLaunchProjectileNode);
