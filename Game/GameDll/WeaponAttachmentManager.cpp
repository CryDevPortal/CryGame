/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: C++ WeaponAttachmentManager Implementation

-------------------------------------------------------------------------
History:
- 30:5:2007   16:55 : Created by Benito Gangoso Rodriguez

*************************************************************************/

#include "StdAfx.h"
#include "WeaponAttachmentManager.h"
#include "Actor.h"
#include "Item.h"
#include "IValidator.h"

#define WEAPON_ATTACHMENTS_FILE "Scripts/Entities/Items/XML/WeaponAttachments.xml"

CWeaponAttachmentManager::CWeaponAttachmentManager(CActor* _pOwner):
m_pOwner(_pOwner),
m_itemToBack(0),
m_itemToHand(0)
{
	m_boneAttachmentMap.clear();
	m_attachedWeaponList.clear();
}

CWeaponAttachmentManager::~CWeaponAttachmentManager()
{
	m_boneAttachmentMap.clear();
	m_attachedWeaponList.clear();
}

bool CWeaponAttachmentManager::Init()
{
	if(gEnv->bMultiplayer || m_pOwner->GetActorSpecies()!=eGCT_HUMAN)
		return false;

	m_pItemSystem = g_pGame->GetIGameFramework()->GetIItemSystem();

	CreatePlayerBoneAttachments();

	return true;
}

void CWeaponAttachmentManager::RemoveAttachments()
{
	if (ICharacterInstance* pCharInstance = m_pOwner->GetEntity()->GetCharacter(0))
	{
		IAttachmentManager* pAttachmentManager = pCharInstance->GetIAttachmentManager(); 

		if(pAttachmentManager == NULL)
			return;

		TBoneAttachmentMap::iterator fi;
		for (fi=m_boneAttachmentMap.begin();fi!=m_boneAttachmentMap.end();++fi)
		{
			pAttachmentManager->RemoveAttachmentByName(fi->first);
		}
	}
}

//======================================================================
void CWeaponAttachmentManager::CreatePlayerBoneAttachments()
{
	if (ICharacterInstance* pCharInstance = m_pOwner->GetEntity()->GetCharacter(0))
	{
		IAttachmentManager* pAttachmentManager = pCharInstance->GetIAttachmentManager(); 

		if(pAttachmentManager == NULL)
			return;

		const XmlNodeRef rootNode = gEnv->pSystem->LoadXmlFromFile( WEAPON_ATTACHMENTS_FILE );

		if (!rootNode || strcmpi(rootNode->getTag(), "WeaponAttachments"))
		{
			CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Could not load Weapon Attachments data. Invalid XML file '%s'! ", WEAPON_ATTACHMENTS_FILE);
			return;
		}

		IAttachment *pAttachment = NULL;
		const int childCount = rootNode->getChildCount();

		for (int i = 0; i < childCount; ++i)
		{
			XmlNodeRef weaponAttachmentNode = rootNode->getChild(i);

			if(weaponAttachmentNode == (IXmlNode*)NULL)
				continue;

			const char* attachmentName = "";
			weaponAttachmentNode->getAttr("name", &attachmentName);

			if(!strcmp(attachmentName, ""))
			{
				CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Empty Weapon Attachment name in file: %s! Skipping Weapon Attachment.", WEAPON_ATTACHMENTS_FILE);
				continue;
			}

			pAttachment = pAttachmentManager->GetInterfaceByName(attachmentName);

			if(pAttachment)
			{
				continue;
			}

			//Attachment doesn't exist, create it
			const int weaponAttachmentCount = weaponAttachmentNode->getChildCount();
			const char* boneName = "";
			Vec3 attachmentOffset(ZERO);
			Ang3 attachmentRotation(ZERO);

			for (int a = 0; a < weaponAttachmentCount; ++a)
			{
				const XmlNodeRef childNode = weaponAttachmentNode->getChild(a);

				if(childNode == (IXmlNode*)NULL)
					continue;

				if(!strcmp(childNode->getTag(), "Bone"))
				{
					childNode->getAttr("name", &boneName);
				}
				else if(!strcmp(childNode->getTag(), "Offset"))
				{
					childNode->getAttr("x", attachmentOffset.x); 
					childNode->getAttr("y", attachmentOffset.y);
					childNode->getAttr("z", attachmentOffset.z);
				}
				else if(!strcmp(childNode->getTag(), "Rotation"))
				{
					float value = 0.0f;
					childNode->getAttr("x", value); 
					attachmentRotation.x = DEG2RAD(value);

					childNode->getAttr("y", value);
					attachmentRotation.y = DEG2RAD(value);

					childNode->getAttr("z", value);
					attachmentRotation.z = DEG2RAD(value);
				}
			}

			const char* attachmentType = "";
			weaponAttachmentNode->getAttr("type", &attachmentType);

			if(!strcmp(attachmentType, ""))
			{
				CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "No Weapon Attachment type assigned! Skipping Weapon Attachment: %s", attachmentName);
				continue;
			}

			const bool isBoneAttachment = !strcmp(attachmentType, "Bone");
			const bool isFaceAttachment = !strcmp(attachmentType, "Face");

			//Bone attachment needs the bone name
			if (!strcmp(boneName, "") && isBoneAttachment)
			{
				CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Bone Attachment with no bone name assigned! Skipping Weapon Attachment: %s", attachmentName);
				continue;
			}

			if(isBoneAttachment)
			{
				pAttachment = pAttachmentManager->CreateAttachment(attachmentName, CA_BONE, boneName);
			}
			else if(isFaceAttachment)
			{
				pAttachment = pAttachmentManager->CreateAttachment(attachmentName, CA_FACE, 0);
			}
			
			if(pAttachment)
			{
				if(isBoneAttachment)
					m_boneAttachmentMap.insert(TBoneAttachmentMap::value_type(attachmentName,0));

				if(pAttachment && !attachmentOffset.IsZero())
				{
					QuatT attachmentQuat(IDENTITY);
					attachmentQuat.SetRotationXYZ(attachmentRotation, attachmentOffset);

					pAttachment->SetAttAbsoluteDefault( attachmentQuat );
					pAttachment->ProjectAttachment();
				}
			}
		}
	}
}

//======================================================================
void CWeaponAttachmentManager::DoHandToBackSwitch()
{
	CItem* pItem = static_cast<CItem*>(m_pItemSystem->GetItem(m_itemToBack));
	if(pItem)
	{
		pItem->AttachToHand(false);
		pItem->AttachToBack(true);
	}
	m_itemToBack = 0;
}

//=====================================================================
void CWeaponAttachmentManager::DoBackToHandSwitch()
{
	CItem* pItem = static_cast<CItem*>(m_pItemSystem->GetItem(m_itemToHand));
	if(pItem)
	{
		pItem->AttachToBack(false);
		pItem->AttachToHand(true);
	}
	m_itemToHand = 0;
}

//=======================================================================
void CWeaponAttachmentManager::SetWeaponAttachment(bool attach, const char* attachmentName, EntityId weaponId)
{
	TBoneAttachmentMap::iterator it = m_boneAttachmentMap.find(CONST_TEMPITEM_STRING(attachmentName));
	if(it!=m_boneAttachmentMap.end())
	{
		if(attach)
		{
			it->second = weaponId;
			stl::push_back_unique(m_attachedWeaponList,weaponId);
		}
		else
		{
			it->second = 0;
			m_attachedWeaponList.remove(weaponId);
		}
	}
}

//=========================================================================
bool CWeaponAttachmentManager::IsAttachmentFree(const char* attachmentName)
{
	TBoneAttachmentMap::iterator it = m_boneAttachmentMap.find(CONST_TEMPITEM_STRING(attachmentName));
	if(it!=m_boneAttachmentMap.end())
	{
		if(it->second==0)
			return true;
		else
			return false;
	}

	return false;
}

//========================================================================
void CWeaponAttachmentManager::HideAllAttachments(bool hide)
{
	TAttachedWeaponsList::const_iterator it = m_attachedWeaponList.begin();
	while(it!=m_attachedWeaponList.end())
	{
		CItem *pItem = static_cast<CItem*>(m_pItemSystem->GetItem(*it));
		if(pItem)
			pItem->Hide(hide);
		it++;
	}
}
