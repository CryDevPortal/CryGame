/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006-2012.
-------------------------------------------------------------------------
Description: Access to the IK Target System
-------------------------------------------------------------------------
History:
- January 9th, 2012: Created by Michelle Martin
*************************************************************************/
#include DEVIRTUALIZE_HEADER_FIX(IIKTargetSystem.h)

#ifndef __I_IKTARGETSYSTEM_H__
#define __I_IKTARGETSYSTEM_H__




// ====================================================

enum EIK_States
{
	IK_BLENDING_IN,
	IK_FULLY_BLENDED_IN,
	IK_BLENDING_OUT,
	IK_FULLY_FADED_OUT
};

// ====================================================

//! IK Parameters that should be set by the calling code (making the call to IKTargetSystem)
struct SIKTargetParams
{
	//! Entity ID of the character that shall execute the IK
	//! It needs to have the necessary interfaces attached, and own the assigned IKHandle
	EntityId	characterEntityID;

	//! if set to true, the IK Solver will also adjust the end effector (for example the hand) orientation
	bool			alignEndEffector;

	//! name of the IK Handle as it is set up in the character's IK File (or chrparams)
	//! this IK setup will be used to reach the target
	string		ikHandleName;
	uint64		ikHandleHash;

	//! OPTIONAL: if this is != 0 then involvement of this Spine is desired - this limb _MUST_ have the solver SPNE assigned to it
	//! This spine can be used by multiple IK Targets and a good solution will be found for a most natural look
	//! relevant for humanoid, two-armed characters mostly
	uint64		ikSpineHash;

	//! OPTIONAL: if the spine is involved (ikSpineHash != 0) then it might become necessary to fix the other arm.
	//! If a limb for the "other" arm is specified here, then it will be fixed, even though the spine might be moved.
	//! relevant mostly for humanoid, two-armed characters so that there is no intersection or the animation isn't broken
	uint64		ikFixedLimb;

	//! time in seconds to fade in 100% into the IK
	//! values <= 0.0f will result in instant IK activation, no blend-in
	float			blendInTimeSecs;

	//! Local offset which is applied to the targets transformation -> the end result is the target location for the IK.
	//! This can be used to correct the target position for the end-effector, for example to make a hand palm
	//! fit properly to a surface
	QuatT			localOffset;

	//! false by default - if true, then the offset will be applied as a global one, not local (special cases)
	//! The rotation will be the final world rotation, and the translation will merely be added to the target's position
	bool			offsetIsGlobal;

	// optional animation data

	//! Animation Layer (for hand/finger animation for example)
	int		limbAnimLayer;

	//! Animation Name (for hand/finger animation for example)
	CCryName limbAnimName;


	SIKTargetParams(EntityId characterID, const char* cIKHandleName, bool bAlignEndEffector = true, float bBlendInTimeSecs = 0.3f, QuatT vLocalOffset = QuatT(IDENTITY), bool globalOffset = false, const char* spineHandle = NULL, const char* fixedLimbHandle = NULL)
	{
		characterEntityID = characterID;
		ikHandleName = cIKHandleName;
		ikHandleHash = *((uint64*)cIKHandleName);
		alignEndEffector = bAlignEndEffector;
		blendInTimeSecs = bBlendInTimeSecs;
		localOffset = vLocalOffset;
		offsetIsGlobal = globalOffset;
		ikSpineHash = (spineHandle) ? *((uint64*)spineHandle) : 0;
		ikFixedLimb = (fixedLimbHandle) ? *((uint64*)fixedLimbHandle) : 0;
		limbAnimLayer = 0;
		limbAnimName = "";
	}

	SIKTargetParams(EntityId characterID, uint64 IKHandleHash, bool bAlignEndEffector = true, float bBlendInTimeSecs = 0.3f, QuatT vLocalOffset = QuatT(IDENTITY), bool globalOffset = false, uint64 spineHandle = 0, uint64 fixedLimbHandle = 0)
	{
		characterEntityID = characterID;
		ikHandleName = (const char*)&IKHandleHash;
		ikHandleHash = IKHandleHash;
		alignEndEffector = bAlignEndEffector;
		blendInTimeSecs = bBlendInTimeSecs;
		localOffset = vLocalOffset;
		offsetIsGlobal = globalOffset;
		ikSpineHash = spineHandle;
		ikFixedLimb = fixedLimbHandle;
		limbAnimLayer = 0;
		limbAnimName = "";
	}

	SIKTargetParams()
	{
		characterEntityID = 0;
		alignEndEffector = true;
		ikHandleName = "";
		ikHandleHash = 0;
		blendInTimeSecs = 0.0f;
		localOffset = QuatT(IDENTITY);
		offsetIsGlobal = false;
		ikSpineHash = 0;
		ikFixedLimb = 0;
		limbAnimLayer = 0;
		limbAnimName = "";
	}

};


// ====================================================


/// IK Target System interface
UNIQUE_IFACE struct IIKTargetSystem
{
	virtual	~IIKTargetSystem() {}

	//! stops and removes all IK for all entities immediately (no further updates)
	virtual void StopAllIK() = 0;

	//! stops and removes all IK for this character immediately (no further updates)
	virtual void StopAllIKOnCharacter( EntityId entityID ) = 0;

	//! Fades out the IK for this character on the specified limb. 
	//! The IK is not necessarily immediately turned off, but the target is blended out instead.
	//! If either the character or the limb cannot be found, the function will do nothing and return.
	virtual void StopIKonCharacterLimb( EntityId entityID, const char* ikLimbName, float fadeOutTime = 0.3f ) = 0;
	virtual void StopIKonCharacterLimb( EntityId entityID, uint64 ikLimbHash, float fadeOutTime = 0.3f ) = 0;

	//! Sets a new offset for the specified IK Target
	virtual void SetOffsetOnTarget( EntityId entityID, uint64 ikLimbHash, QuatT newOffset ) = 0;

	//! Adds an IK Target for an entity as a target
	//! returns 0 if ok, error code otherwise, see ::EIK_ErrorCodes
	virtual int AddIKTarget_EntityPos(const SIKTargetParams& ikParams, EntityId targetEntity) = 0;

	//! Adds an IK Target for a helper dummy as a target (created and exported in a 3D software)
	//! returns 0 if ok, error code otherwise, see ::EIK_ErrorCodes
	virtual int AddIKTarget_AttachedHelperPos(const SIKTargetParams& ikParams, EntityId targetEntity, const char* helperName) = 0;

	//! Adds an IK Target for an attachment as a target
	//! returns 0 if ok, error code otherwise, see ::EIK_ErrorCodes
	virtual int AddIKTarget_Attachment(const SIKTargetParams& ikParams, EntityId targetEntity, const char* attachmentName) = 0;

	//! Adds an IK Target for a character's bone as a target
	//! returns 0 if ok, error code otherwise, see ::EIK_ErrorCodes
	virtual int AddIKTarget_Bone(const SIKTargetParams& ikParams, EntityId targetEntity, const char* boneName) = 0;
	virtual int AddIKTarget_Bone(const SIKTargetParams& ikParams, EntityId targetEntity, uint boneId) = 0;

	//! Returns true if an IK Target exists for this limb for this character.
	//! False in all other cases.
	//! Also returns true if IK Target exists but the state is fully blended out.
	virtual bool ExistsIKTargetForCharacterLimb( EntityId entityID, uint64 ikLimbHash ) = 0;

	//! Returns true if the IK Target for this character and limb exists and is 100% blended in.
	//! If no IK Target was found or it is not blended in completely, function will return false.
	virtual bool IsIKTargetForCharacterLimbFullyBlendedIn( EntityId entityID, uint64 ikLimbHash ) = 0;

	enum EIK_ErrorCodes
	{
		IKErr_Ok = 0,
		IKErr_No_Valid_EntityID = 1,
		IKErr_No_Valid_CharacterInterface,
		IKErr_No_Valid_TargetEntityID,
		IKErr_No_Valid_AttachedHelperName,
		IKErr_No_Valid_Bone,
		IKErr_No_Valid_Target_Skeleton,
		IKErr_IKHandle_Does_Not_Exist,

		// if you add more codes here, please update the WriteErrorMessage() function in the cpp file as well

		IKErr_MaxDocumented
	};  

};



#endif //__I_IKTARGETSYSTEM_H__
