#ifndef _CRE_BREAKABLE_GLASS_
#define _CRE_BREAKABLE_GLASS_

#pragma once

// Debugging (This should *always* be committed disabled...)
#if !defined(RELEASE) && !defined(IS_CRYDEV)
//#define GLASS_DEBUG_MODE
#endif

// Includes
#include "CREBreakableGlassHelpers.h"

// Forward decls
struct IRenderAuxGeom;
struct SAuxGeomRenderFlags;
struct SPipTangents;
class CCryNameR;

template <class T, uint N>
class FixedPodArray;

template <class T, uint32 GridSize, uint32 BucketSize>
class CSpatialHashGrid;

// Typedefs
typedef CSpatialHashGrid<uint8, GLASSCFG_HASH_GRID_SIZE, GLASSCFG_HASH_GRID_BUCKET_SIZE>	TGlassHashGrid;
typedef CryFixedArray<uint8, GLASSCFG_HASH_GRID_BUCKET_SIZE>															TGlassHashBucket;

typedef FixedPodArray<int, GLASSCFG_MAX_NUM_RADIAL_CRACKS>						TRadialCrackArray;
typedef CryFixedArray<SGlassFragment, GLASSCFG_FRAGMENT_ARRAY_SIZE>		TFragArray;
typedef	CryFixedArray<uint8, GLASSCFG_FRAGMENT_ARRAY_SIZE>						TFragIndexArray;
typedef	FixedPodArray<uint8, GLASSCFG_FRAGMENT_ARRAY_SIZE>						TFragIndexPodArray;
typedef	FixedPodArray<uint8, GLASSCFG_MAX_NUM_RADIAL_CRACKS>					TSubFragArray;
typedef CryFixedArray<SGlassImpactParams, GLASSCFG_MAX_NUM_IMPACTS>		TImpactArray;

//==================================================================================================
// Name: SBreakableGlassREParams
// Desc: Breakable glass sim render element params
// Author: Chris Bunner
//==================================================================================================
struct SBreakableGlassREParams
{
	SBreakableGlassREParams()
	: centre(Vec3Constants<f32>::fVec3_Zero)
	{
		matrix.SetIdentity();
	}

	Vec3			centre;
	Matrix34	matrix;
};//------------------------------------------------------------------------------------------------

//==================================================================================================
// Name: CREBreakableGlass
// Desc: Breakable glass sim render element
// Author: Chris Bunner
//==================================================================================================
class CREBreakableGlass : public CRendElementBase
{
public:
	CREBreakableGlass();
	virtual ~CREBreakableGlass();

	// CRendElementBase interface
	virtual void	mfPrepare(bool bCheckOverflow);
	virtual bool	mfDraw(CShader* ef, SShaderPass* sfm);
	virtual float	mfDistanceToCameraSquared(Matrix34& matInst);

	// CREBreakableGlass interface
	virtual bool	InitialiseRenderElement(const SBreakableGlassInitParams& params);
	virtual void	ReleaseRenderElement();

	virtual void	Update(SBreakableGlassUpdateParams& params);
	virtual void	GetGlassState(SBreakableGlassState& state);

	virtual void	ApplyImpactToGlass(const SGlassImpactParams& params);
	virtual void	ApplyExplosionToGlass(const SGlassImpactParams& params);

	virtual void	SetCVars(const SBreakableGlassCVars* pCVars);

#ifdef GLASS_DEBUG_MODE
#ifndef NULL_RENDERER
	virtual void	DrawFragmentDebug(const uint fragIndex, const Matrix34& matrix, const uint8 buffId, const float alpha);
#else
	virtual void	DrawFragmentDebug(const uint fragIndex, const Matrix34& matrix, const uint8 buffId, const float alpha) {}
#endif
#endif

	ILINE SBreakableGlassREParams* GetParams()
	{
		return &m_params;
	}

private:
	// Internal constants
	static CCryNameR		s_ImpactDecalParamName;

	enum EGlassSurfaceSide
	{
		EGlassSurfaceSide_Center,
		EGlassSurfaceSide_Front,
		EGlassSurfaceSide_Back
	};

	// Core
	void	ResetTempCrackData();
	void	RT_UpdateBuffers(const int subFragIndex = -1);

	// Random
	void	SeedRand(const uint seed);
	float	GetRandF();
	float GetRandF(const uint index);

	// Glass impact state/reaction
	void	HandleAdditionalImpact(const SGlassImpactParams& params);
	void	ShatterGlass();
	void	PlayShatterEffect(const uint8 fragIndex);

	float	CalculateBreakThreshold();
	bool	CheckForGlassBreak(const float breakThreshold, float& excessEnergy);
	void	CalculateImpactEnergies(const float totalEnergy, float& radialEnergy, float& circularEnergy);

	// Crack pattern generation
	uint	GenerateCracksFromImpact(const uint8 parentFragIndex, const SGlassImpactParams& impact);
	float	PropagateRadialCrack(const uint startIndex, const uint currIndex, const uint localIndex, const float angle, const float energyPerStep);
	uint	GenerateRadialCracks(const float totalEnergy, const uint impactDepth, const float fragArea);
	float	GetImpactRadius(const SGlassImpactParams& impact);

	// Fragment impacts
	bool	ApplyImpactToFragments(SGlassImpactParams& impact);
	bool	FindImpactFragment(const SGlassImpactParams& impact, uint8& fragIndex);

	SGlassFragment* AddFragment();
	void						RemoveFragment(const uint8 fragIndex);
	void						RebuildChangedFragment(const uint8 fragIndex);

	void	GenerateSubFragments(const uint8 parentFragIndex, const SGlassImpactParams& impact);
	void	GenerateSubFragmentCracks(const uint8 parentFragIndex, const SGlassImpactParams& impact, TRadialCrackArray& fragIntersectPts);
	bool	CreateFragmentFromOutline(const Vec2* pOutline, const int outlineSize, const uint8 parentFragIndex, const bool forceLoose = false);

	bool	IsFragmentLoose(const SGlassFragment& frag);
	void	RemoveFragmentConnections(const uint8 fragIndex, PodArray<uint8>& connections);

	void	ConnectSubFragments(const uint8* pSubFrags, const uint8 numSubFrags);
	void	ReplaceFragmentConnections(const uint8 parentFragIndex, const uint8* pSubFrags, const uint8 numSubFrags);
	void	ConnectFragmentPair(const uint8 fragAIndex, const uint8 fragBIndex);

	void	FindLooseFragments(TGlassPhysFragmentArray* pPhysFrags, TGlassPhysFragmentInitArray* pPhysFragsInitData);
	void	TraverseStableConnection(const uint8 fragIndex, TFragIndexPodArray& stableFrags);
	void	PrePhysicalizeLooseFragment(TGlassPhysFragmentArray* pPhysFrags, TGlassPhysFragmentInitArray* pPhysFragsInitData, const uint8 fragIndex, const bool dampenVel);

	// Fragment hashing
	void	SetHashGridSize(const Vec2& size);
	void	HashFragment(const uint8 fragIndex, const bool remove = false);

	void	HashFragmentTriangle(const Vec2& a, const Vec2& b, const Vec2& c,
		const uint8& triFrag, const bool removeTri = false);

	void	HashFragmentTriangleSpan(const struct STriEdge& a, const struct STriEdge& b,
		const uint8& triFrag, const bool removeTri = false);

	// Fragment management
	void DeactivateFragment(const uint8 index, const uint32 bit);

	// Geometry generation
	void	GenerateLoosePieces(const int numRadialCracks);
	void	GenerateStablePieces(const int numRadialCracks);
	float	ClosestImpactDistance(const Vec2& pt);

	void	RebuildAllFragmentGeom();
	void	RehashAllFragmentData();
	void	ProcessFragmentGeomData();
	void	BuildFragmentTriangleData(const uint8 fragIndex, uint& pVertOffs, uint& pIndOffs, const EGlassSurfaceSide side = EGlassSurfaceSide_Center, const int subFragID = -1);
	void	BuildFragmentOutlineData(const uint8 fragIndex, uint& pVertOffs, uint& pIndOffs, const int subFragID = -1);

	void	GenerateDefaultPlaneGeom();
	bool	GenerateGeomFromFragment(const Vec2* pFragPts, const uint numPts);
	void	GenerateVertFromPoint(const Vec3& pt, const Vec2& uvOffset, SVF_P3F_C4B_T2F& vert);
	void	GenerateTriangleTangent(const Vec3& triPt0, const Vec3& triPt1, const Vec3& triPt2, SPipTangents& tangent);
	
	// Drawing
	void	UpdateImpactShaderConstants();
	void	SetImpactShaderConstants(CShader* pShader);

#ifdef GLASS_DEBUG_MODE
	// Debug geometry
	void	GenerateImpactGeom(const SGlassImpactParams& impact);
	void	TransformPointList(PodArray<Vec3>& ptList, const bool inverse = false);

	// Debug drawing
	void	DebugDraw(const bool wireframe, const bool data);
	void	DrawLooseGeom(IRenderAuxGeom* const pRenderer, SAuxGeomRenderFlags& flags);
	void	DrawOutlineGeom(IRenderAuxGeom* const pRenderer, SAuxGeomRenderFlags& flags);
	void	DrawFragmentDebug(IRenderAuxGeom* const pRenderer, SAuxGeomRenderFlags& flags);
	void	DrawFragmentConnectionDebug(IRenderAuxGeom* const pRenderer, SAuxGeomRenderFlags& flags);
	void	DrawDebugData(IRenderAuxGeom* const pRenderer, SAuxGeomRenderFlags& flags);
#endif

	// Geometry buffer
	struct SGlassGeom
	{
		void Clear()
		{
			m_verts.Clear();
			m_inds.Clear();
			m_tans.Clear();
		}

		void Free()
		{
			m_verts.Free();
			m_inds.Free();
			m_tans.Free();
		}

		PodArray<SVF_P3F_C4B_T2F>		m_verts;
		PodArray<uint16>						m_inds;
		PodArray<SPipTangents>			m_tans;
	};

	// Physicalized fragment
	struct SGlassPhysFragId
	{
		uint32	m_geomBufferId;
		uint8		m_fragId;
	};

	// Persistent data
	TFragArray										m_frags;
	TImpactArray									m_impactParams;
	TFragIndexArray								m_freeFragIndices;

	SBreakableGlassInitParams			m_glassParams;
	SBreakableGlassREParams				m_params;
	SBreakableGlassDecalConstant	m_decalPSConsts[GLASSCFG_MAX_NUM_IMPACT_DECALS];

	// Temp crack generation data
	Vec2											m_pointArray[GLASSCFG_MAX_NUM_CRACK_POINTS];
	PodArray<Vec2*>						m_radialCrackTrees[GLASSCFG_MAX_NUM_RADIAL_CRACKS];
	
	// Temp fragment mesh data
	SGlassGeom								m_fragFullGeom[GLASSCFG_MAX_NUM_PHYS_FRAGMENTS];
	SGlassPhysFragId					m_fragGeomBufferIds[GLASSCFG_MAX_NUM_PHYS_FRAGMENTS];
	volatile bool							m_fragGeomRebuilt[GLASSCFG_MAX_NUM_PHYS_FRAGMENTS];

	// Temp stable mesh data
	SGlassGeom								m_planeGeom;
	SGlassGeom								m_crackGeom;

#ifdef GLASS_DEBUG_MODE
	// Debug drawing
	PodArray<Vec3>											m_impactLineList;
#endif

	static const SBreakableGlassCVars*	s_pCVars;

	// Persistent data
	Vec2											m_hashCellSize;
	Vec2											m_hashCellInvSize;
	Vec2											m_hashCellHalfSize;
	Vec2											m_invUVRange;
	
	TGlassHashGrid*						m_pHashGrid;
	float											m_invMaxGlassSize;
	float											m_impactEnergy;
	uint32										m_geomBufferId;

	// Fragment state bit arrays
	uint32										m_fragsActive;
	uint32										m_fragsLoose;
	uint32										m_fragsFree;
	uint32										m_lastFragBit;

	// Shader constant state
	uint8											m_numDecalImpacts;
	uint8											m_numDecalPSConsts;

	// Temp counts
	uint8											m_pointArrayCount;
	uint8											m_lastPhysFrag;
	uint8											m_totalNumFrags;
	uint8											m_lastFragIndex;
	uint8											m_numLooseFrags;

	// Glass state
	bool											m_shattered;
	bool											m_geomDirty;
	volatile bool							m_geomRebuilt; // Accessed by MT and RT
	volatile bool							m_geomBufferLost; // Accessed by MT and RT
	volatile uint8						m_dirtyGeomBufferCount; // Accessed by MT and RT
};//------------------------------------------------------------------------------------------------

#endif // _CRE_BREAKABLE_GLASS_
