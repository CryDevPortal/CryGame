#include "StdAfx.h"
#include "GameObjects/GameObject.h"
#include "Item.h"
#include "Weapon.h"
#include "FlowWeaponCustomizationNodes.h"

//--------------------------------------------------------------------------------------------
CFlashUIInventoryNode::~CFlashUIInventoryNode()
{
}

void CFlashUIInventoryNode::GetConfiguration( SFlowNodeConfig &config )
{
	static const SInputPortConfig in_config[] = {
		InputPortConfig_Void( "Call", "Calls the function" ),
		InputPortConfig_Null()
	};

	static const SOutputPortConfig out_config[] = {
		OutputPortConfig_Void( "OnCall", "Triggered if this node starts the action" ),
		OutputPortConfig<string>( "Args", "Comma separated argument string" ),
		OutputPortConfig_Null()
	};


	config.nFlags |= EFLN_TARGET_ENTITY;
	config.pInputPorts = in_config;
	config.pOutputPorts = out_config;
	config.sDescription = "Outputs the players inventory as a string for UI processing";
	config.SetCategory(EFLN_APPROVED);
}


void CFlashUIInventoryNode::ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
{
	if(event == eFE_Activate && IsPortActive(pActInfo, 0))
	{
		IActor* pActor = GetInputActor( pActInfo );
		if(pActor)
		{
			IInventory* pInventory = pActor->GetInventory();
			if(pInventory)
			{
				string weapons = "";
				bool first = true;
				int inv_cap = gEnv->pConsole->GetCVar("i_inventory_capacity")->GetIVal();
				for (int i = 0; i < inv_cap; i++)
				{
					const char* weaponName = pInventory->GetItemString(i);

					if(strcmp(weaponName, "") != 0)
					{
						bool selectable = false;

						//Get the weapon and check if it is a selectable item
						IEntityClassRegistry *pRegistry = gEnv->pEntitySystem->GetClassRegistry();
						EntityId item = pInventory->GetItemByClass(pRegistry->FindClass(weaponName));
						IEntity* pEntity = gEnv->pEntitySystem->GetEntity(item);

						if(pEntity)
						{
							CGameObject * pGameObject = (CGameObject*)pEntity->GetProxy(ENTITY_PROXY_USER);
							CItem* pItem = (CItem*)pGameObject->QueryExtension(pGameObject->GetEntity()->GetClass()->GetName());
							if(pItem)
							{
								selectable = pItem->CanSelect();
							}
						}
						if(selectable)
						{
							if(!first)
								weapons.append(",");
							first = false;
							weapons.append(weaponName);
						}
					}

				}

				ActivateOutput(pActInfo, eO_OnCall, true);
				ActivateOutput(pActInfo, eO_Args, weapons);
			}
		}
	}
}


CFlashUIInventoryNode::CFlashUIInventoryNode( SActivationInfo * pActInfo )
{

}

//--------------------------------------------------------------------------------------------
CFlashUIGetEquippedAccessoriesNode::~CFlashUIGetEquippedAccessoriesNode()
{
}

void CFlashUIGetEquippedAccessoriesNode::GetConfiguration( SFlowNodeConfig &config )
{
	static const SInputPortConfig in_config[] = {
		InputPortConfig_Void( "Call", "Calls the function" ),
		InputPortConfig<string>( "Weapon", "Weapon to get the equipment for" ),
		InputPortConfig_Null()
	};

	static const SOutputPortConfig out_config[] = {
		OutputPortConfig_Void( "OnCall", "Triggered if this node starts the action" ),
		OutputPortConfig<string>( "Args", "Comma separated argument string" ),
		OutputPortConfig_Null()
	};


	config.nFlags |= EFLN_TARGET_ENTITY;
	config.pInputPorts = in_config;
	config.pOutputPorts = out_config;
	config.sDescription = "Outputs all equipped accessories in a comma separated string";
	config.SetCategory(EFLN_APPROVED);
}

void CFlashUIGetEquippedAccessoriesNode::ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
{
	if(event == eFE_Activate && IsPortActive(pActInfo, 0))
	{
		string accessories = "";
		IActor* pActor = GetInputActor( pActInfo );

		if(pActor)
		{
			IInventory* pInventory = pActor->GetInventory();
			if(pInventory)
			{

				//Get the item ID via the Input string
				const string weapon_name = GetPortString(pActInfo, eI_Weapon);
				IEntityClassRegistry *pRegistery = gEnv->pEntitySystem->GetClassRegistry();
				EntityId item = pInventory->GetItemByClass(pRegistery->FindClass(weapon_name));

				//Fetch the actual object via the ID
				CGameObject * pGameObject = (CGameObject*)gEnv->pEntitySystem->GetEntity(item)->GetProxy(ENTITY_PROXY_USER);
				const char* ext = pGameObject->GetEntity()->GetClass()->GetName();
				IItem* pWeapon = (IItem*)pGameObject->QueryExtension(pGameObject->GetEntity()->GetClass()->GetName());

				//If the weapon exists, return all equipped attachments in a comma seperated string
				if(pWeapon)
				{
					//All equipped accessories for this weapon weapons
					accessories = static_cast<CItem*>(pWeapon)->GetAttachedAccessoriesString();
				}
			}
		}

		//return, if 'accesories' is empty, it has no attachments, or something was invalid
		ActivateOutput(pActInfo, eO_OnCall, true);
		ActivateOutput(pActInfo, eO_Args, accessories);
	}
}

//--------------------------------------------------------------------------------------------
CFlashUIGetCompatibleAccessoriesNode ::~CFlashUIGetCompatibleAccessoriesNode ()
{
}

void CFlashUIGetCompatibleAccessoriesNode ::GetConfiguration( SFlowNodeConfig &config )
{
	static const SInputPortConfig in_config[] = {
		InputPortConfig_Void( "Call", "Calls the function" ),
		InputPortConfig<string>( "Weapon", "Weapon to get the equipment for" ),
		InputPortConfig_Null()
	};

	static const SOutputPortConfig out_config[] = {
		OutputPortConfig_Void( "OnCall", "Triggered if this node starts the action" ),
		OutputPortConfig<string>( "Args", "Comma separated argument string" ),
		OutputPortConfig_Null()
	};


	config.nFlags |= EFLN_TARGET_ENTITY;
	config.pInputPorts = in_config;
	config.pOutputPorts = out_config;
	config.sDescription = "Outputs the all compatible accessories for a weapon in a comma seperated string";
	config.SetCategory(EFLN_APPROVED);
}

void CFlashUIGetCompatibleAccessoriesNode ::ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
{
	if(event == eFE_Activate && IsPortActive(pActInfo, 0))
	{
		string accessories = "";
		IActor* pActor = GetInputActor( pActInfo );

		if(pActor)
		{
			IInventory* pInventory = pActor->GetInventory();
			if(pInventory)
			{
				//Get the item ID via the Input string
				const string weapon_name = GetPortString(pActInfo, eI_Weapon);
				IEntityClassRegistry *pRegistery = gEnv->pEntitySystem->GetClassRegistry();
				EntityId item = pInventory->GetItemByClass(pRegistery->FindClass(weapon_name));

				//Fetch the actual weapon via the ID
				IEntity* pEntity = gEnv->pEntitySystem->GetEntity(item);
				if(pEntity)
				{

					CGameObject * pGameObject = (CGameObject*)pEntity->GetProxy(ENTITY_PROXY_USER);
					const char* ext = pGameObject->GetEntity()->GetClass()->GetName();
					CWeapon* pWeapon = (CWeapon*)pGameObject->QueryExtension(pGameObject->GetEntity()->GetClass()->GetName());

					//If the weapon exists, ask for all compatible accessories
					if(pWeapon)
					{
						//All compatible accessories for this weapon
						const DynArray<string>* pCompatibleAccessoriesVec = pWeapon->GetCompatibleAccessories();

						DynArray<string>::const_iterator it;
						for (it = pCompatibleAccessoriesVec->begin(); it != pCompatibleAccessoriesVec->end(); it++)
						{
							accessories.append((*it));
							if(it != pCompatibleAccessoriesVec->begin())
								accessories.append(",");
						}
					}
				}
			}
		}

		//return, if 'accessories' is empty, it has no compatible attachments, or the weapon/inventory was invalid
		ActivateOutput(pActInfo, eO_OnCall, true);
		ActivateOutput(pActInfo, eO_Args, accessories);
	}
}

//--------------------------------------------------------------------------------------------
CFlashUICheckAccessoryState ::~CFlashUICheckAccessoryState ()
{
}

void CFlashUICheckAccessoryState ::GetConfiguration( SFlowNodeConfig &config )
{
	static const SInputPortConfig in_config[] = {
		InputPortConfig_Void( "Call", "Calls the function" ),
		InputPortConfig<string>( "Accessory", "Accessory we are checking" ),
		InputPortConfig<string>( "Weapon", "Weapon we are checking" ),
		InputPortConfig_Null()
	};

	static const SOutputPortConfig out_config[] = {
		OutputPortConfig<bool>( "Equipped", "Accessory is equipped" ),
		OutputPortConfig<bool>( "InInventory", "Accessory is in inventory, not equipped" ),
		OutputPortConfig<bool>( "DontHave", "Entity does not possess accessory" ),
		OutputPortConfig_Null()
	};


	config.nFlags |= EFLN_TARGET_ENTITY;
	config.pInputPorts = in_config;
	config.pOutputPorts = out_config;
	config.sDescription = "Outputs the state of a certain accessory of a weapon";
	config.SetCategory(EFLN_APPROVED);
}

void CFlashUICheckAccessoryState ::ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
{
	if(event == eFE_Activate && IsPortActive(pActInfo, 0))
	{
		IActor* pActor = GetInputActor( pActInfo );
		bool is_equipped = false;
		bool is_inInventory = false;

		if(pActor)
		{
			IInventory* pInventory = pActor->GetInventory();

			if(pInventory)
			{
				IEntityClassRegistry *pRegistry = gEnv->pEntitySystem->GetClassRegistry();

				//Find the accessory's class in the registry
				const string accessory_name = GetPortString(pActInfo, eI_Accessory);				
				IEntityClass* pClass = pRegistry->FindClass(accessory_name);

				//Check if its in inventory
				if(pInventory->HasAccessory(pClass) != 0)
				{
					is_inInventory = true;
				}	

				//if it is, check if its equipped as well
				if(is_inInventory)
				{
					//Get the weapon ID via the Input string
					const char* weapon_name = GetPortString(pActInfo, eI_Weapon).c_str();
					EntityId item = pInventory->GetItemByClass(pRegistry->FindClass(weapon_name));

					//Fetch the actual weapon via the ID
					IEntity* pEntity = gEnv->pEntitySystem->GetEntity(item);
					if(pEntity)
					{

						CGameObject * pGameObject = (CGameObject*)pEntity->GetProxy(ENTITY_PROXY_USER);
						const char* ext = pGameObject->GetEntity()->GetClass()->GetName();
						CWeapon* pWeapon = (CWeapon*)pGameObject->QueryExtension(pGameObject->GetEntity()->GetClass()->GetName());
						bool selectable = pWeapon->CanSelect();
						if(pWeapon)
						{
							if(pWeapon->GetAccessory(pClass->GetName()) != 0)
							{
								is_equipped = true;
							}					
						}
					}
				}
			}
		}

		if(!is_inInventory)
			ActivateOutput(pActInfo, eO_DontHave, true);
		else if(is_equipped)
			ActivateOutput(pActInfo, eO_Equipped, true);
		else
			ActivateOutput(pActInfo, eO_InInventory, true);

	}
}

//--------------------------------------------------------------------------------------------
REGISTER_FLOW_NODE( "UI:WeaponCustomization:CheckAccessoryState", CFlashUICheckAccessoryState  );
REGISTER_FLOW_NODE( "UI:WeaponCustomization:CompatibleAccessories", CFlashUIGetCompatibleAccessoriesNode  );
REGISTER_FLOW_NODE( "UI:WeaponCustomization:GetEquippedAccessories", CFlashUIGetEquippedAccessoriesNode );
REGISTER_FLOW_NODE( "UI:WeaponCustomization:GetInventoryForUI", CFlashUIInventoryNode );
