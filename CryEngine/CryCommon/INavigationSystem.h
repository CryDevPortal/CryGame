#ifndef __INavigationSystem_h__
#define __INavigationSystem_h__

#pragma once


enum ENavigationIDTag
{
	MeshIDTag = 0,
	AgentTypeIDTag,
	VolumeIDTag,
};

template <ENavigationIDTag T>
struct TNavigationID
{
	explicit TNavigationID(uint32 id = 0) : id(id) {}

	TNavigationID& operator=(const TNavigationID& other) { id = other.id; return *this; }

	ILINE operator uint32() const { return id; }
	ILINE bool operator==(const TNavigationID& other) const { return id == other.id; }
	ILINE bool operator!=(const TNavigationID& other) const { return id != other.id; }
	ILINE bool operator<(const TNavigationID& other) const { return id < other.id; }

	AUTO_STRUCT_INFO

private:
	uint32 id;
};


typedef TNavigationID<MeshIDTag> NavigationMeshID;
typedef TNavigationID<AgentTypeIDTag> NavigationAgentTypeID;
typedef TNavigationID<VolumeIDTag> NavigationVolumeID;
typedef Functor3<NavigationAgentTypeID, NavigationMeshID, uint32> NavigationMeshChangeCallback;

struct INavigationSystem
{
	virtual ~INavigationSystem() {}

	enum ENavigationEvent
	{
		MeshReloaded = 0,
		MeshReloadedAfterExporting,
		NavigationCleared,
	};

	struct INavigationSystemListener
	{
		virtual ~INavigationSystemListener(){}
		virtual void OnNavigationEvent(const ENavigationEvent event) = 0;
	};

	enum WorkingState
	{
		Idle = 0,
		Working,
	};


	typedef struct CreateAgentTypeParams
	{
		CreateAgentTypeParams(const Vec3& _voxelSize = Vec3(0.1f), uint16 _radiusVoxelCount = 4,
			uint16 _climbableVoxelCount = 4, uint16 _heightVoxelCount = 18,
			uint16 _maxWaterDepthVoxelCount = 6)
			: voxelSize(_voxelSize)
			, radiusVoxelCount(_radiusVoxelCount)
			, climbableVoxelCount(_climbableVoxelCount)
			, heightVoxelCount(_heightVoxelCount)
			, maxWaterDepthVoxelCount()
		{
		}

		Vec3 voxelSize;

		uint16 radiusVoxelCount;
		uint16 climbableVoxelCount;
		uint16 heightVoxelCount;
		uint16 maxWaterDepthVoxelCount;
	};

	typedef struct CreateMeshParams
	{
		CreateMeshParams(const Vec3& _origin = Vec3(ZERO), const Vec3i& _tileSize = Vec3i(8), const uint32 _tileCount = 1024)
			: origin(_origin)
			, tileSize(_tileSize)
			, tileCount(_tileCount)
		{
		}

		Vec3 origin;
		Vec3i tileSize;
		uint32 tileCount;
	};

	virtual NavigationAgentTypeID CreateAgentType(const char* name, const CreateAgentTypeParams& params) = 0;
	virtual NavigationAgentTypeID GetAgentTypeID(const char* name) const = 0;
	virtual NavigationAgentTypeID GetAgentTypeID(size_t index) const = 0;
	virtual const char* GetAgentTypeName(NavigationAgentTypeID agentTypeID) const = 0;
	virtual size_t GetAgentTypeCount() const = 0;

	virtual NavigationMeshID CreateMesh(const char* name, NavigationAgentTypeID agentTypeID, const CreateMeshParams& params) = 0;
	virtual NavigationMeshID CreateMesh(const char* name, NavigationAgentTypeID agentTypeID, const CreateMeshParams& params, NavigationMeshID requestedID) = 0;
	virtual void DestroyMesh(NavigationMeshID meshID) = 0;

	virtual void AddMeshChangeCallback(NavigationAgentTypeID agentTypeID, const NavigationMeshChangeCallback& callback) = 0;
	virtual void RemoveMeshChangeCallback(NavigationAgentTypeID agentTypeID, const NavigationMeshChangeCallback& callback) = 0;

	virtual void SetMeshBoundaryVolume(NavigationMeshID meshID, NavigationVolumeID volumeID) = 0;

	virtual NavigationVolumeID CreateVolume(Vec3* vertices,	size_t vertexCount, float height) = 0;
	virtual NavigationVolumeID CreateVolume(Vec3* vertices,	size_t vertexCount, float height, NavigationVolumeID requestedID) = 0;
	virtual void DestroyVolume(NavigationVolumeID volumeID) = 0;
	virtual void SetVolume(NavigationVolumeID volumeID,	Vec3* vertices, size_t vertexCount, float height) = 0;
	virtual bool ValidateVolume(NavigationVolumeID volumeID) = 0;
	virtual NavigationVolumeID GetVolumeID(NavigationMeshID meshID) = 0;

	virtual void SetExclusionVolume(const NavigationAgentTypeID* agentTypeIDs, size_t agentTypeIDCount,
		NavigationVolumeID volumeID) = 0;

	virtual NavigationMeshID GetMeshID(const char* name, NavigationAgentTypeID agentTypeID) const = 0;
	virtual const char* GetMeshName(NavigationMeshID meshID) const = 0;
	virtual void SetMeshName(NavigationMeshID meshID, const char* name) = 0;

	virtual WorkingState GetState() const = 0;
	virtual WorkingState Update(bool blocking = false) = 0;
	virtual void PauseNavigationUpdate() = 0;
	virtual void RestartNavigationUpdate() = 0;

	virtual size_t QueueMeshUpdate(NavigationMeshID meshID, const AABB& aabb) = 0;

	virtual void Clear() = 0;
	/* 
	ClearAndNotify it's used when the listeners need to be notified about
	the performed clear operation.
	*/
	virtual void ClearAndNotify() = 0;
	virtual bool ReloadConfig() = 0;
	virtual void DebugDraw() = 0;

	virtual void WorldChanged(const AABB& aabb) = 0;

	virtual void SetDebugDisplayAgentType(NavigationAgentTypeID agentTypeID) = 0;
	virtual NavigationAgentTypeID GetDebugDisplayAgentType() const = 0;

	virtual bool ReadFromFile(const char* fileName, bool bAfterExporting) = 0;
	virtual bool SaveToFile(const char* fileName) const = 0;

	virtual void RegisterListener(INavigationSystemListener* pListener, const char* name = NULL) = 0;
	virtual void UnRegisterListener(INavigationSystemListener* pListener) = 0;

	virtual void RegisterArea(const char* shapeName) = 0;
	virtual void UnRegisterArea(const char* shapeName) = 0;
	virtual NavigationVolumeID GetAreaId(const char* shapeName) const = 0;
	virtual void SetAreaId(const char* shapeName, NavigationVolumeID id) = 0;
	virtual void UpdateAreaNameForId(const NavigationVolumeID id, const char* newShapeName) = 0;

	virtual void StartWorldMonitoring() = 0;
	virtual void StopWorldMonitoring() = 0;

	virtual bool IsInUse() const = 0;

	virtual void CalculateAccessibility() = 0;
};



#endif // __INavigationSystem_h__
