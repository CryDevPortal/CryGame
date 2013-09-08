////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   ParticleParams.h
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef _PARTICLEPARAMS_H_
#define _PARTICLEPARAMS_H_ 1

#include <FinalizingSpline.h>
#include <Cry_Color.h>
#include <CryString.h>
#include <CryName.h>

#include <CryCustomTypes.h>
#include <Cry_Math.h>

BASIC_TYPE_INFO(CCryName)

enum EParticleSpawnType
{
	ParticleSpawn_Direct,
	ParticleSpawn_ParentStart,
	ParticleSpawn_ParentCollide,
	ParticleSpawn_ParentDeath,
};

enum EParticleBlendType
{
  ParticleBlendType_AlphaBased			= OS_ALPHA_BLEND,
  ParticleBlendType_Additive				= OS_ADD_BLEND,
	ParticleBlendType_Multiplicative	= OS_MULTIPLY_BLEND,
  ParticleBlendType_Opaque					= 0,
};

enum EParticleFacing
{
	ParticleFacing_Camera,
	ParticleFacing_Free,
	ParticleFacing_Velocity,
	ParticleFacing_Water,
	ParticleFacing_Terrain,
	ParticleFacing_Decal,
};

enum EParticlePhysicsType
{
	ParticlePhysics_None,
	ParticlePhysics_SimpleCollision,
	ParticlePhysics_SimplePhysics,
	ParticlePhysics_RigidBody
};

enum EParticleForceType
{
	ParticleForce_None,
	ParticleForce_Wind,
	ParticleForce_Gravity,
	ParticleForce_Target,
};

enum EAnimationCycle
{
	AnimationCycle_Once,
	AnimationCycle_Loop,
	AnimationCycle_Mirror
};

enum EParticleHalfResFlag
{
	ParticleHalfResFlag_NotAllowed,
	ParticleHalfResFlag_Allowed,
	ParticleHalfResFlag_Forced,
};

enum ETextureMapping
{
	TextureMapping_PerParticle,
	TextureMapping_PerStream,
};

enum EMoveRelative
{
	MoveRelative_No,
	MoveRelative_Yes,
	MoveRelative_YesWithTail,
};

enum ETrinary
{
	Trinary_Both,
	Trinary_If_True,
	Trinary_If_False
};

inline bool TrinaryMatch(ETrinary t1, ETrinary t2)
{
	return (t1 | t2) != 3;
}

inline bool TrinaryMatch(ETrinary t, bool b)
{
	return t != (Trinary_If_True + (int)b);
}

enum EConfigSpecBrief
{
	ConfigSpec_Low = 0,
	ConfigSpec_Medium,
	ConfigSpec_High,
	ConfigSpec_VeryHigh,
};

// Sound related
enum ESoundControlTime
{
	SoundControlTime_EmitterLifeTime,
	SoundControlTime_EmitterExtendedLifeTime,
	SoundControlTime_EmitterPulsePeriod
};

// Pseudo-random number generation, from a key.
class CChaosKey
{
public:
	// Initialize with an int.
	explicit inline CChaosKey(uint32 uSeed)
		: m_Key(uSeed) {}

	explicit inline CChaosKey(float fSeed)
		: m_Key((uint32)(fSeed * float(0xFFFFFFFF))) {}

	CChaosKey Jumble(CChaosKey key2) const
	{
		return CChaosKey( Jumble(m_Key ^ key2.m_Key) );
	}
	CChaosKey Jumble(void const* ptr) const
	{
		return CChaosKey( Jumble(m_Key ^ (uint32)((UINT_PTR)ptr & 0xFFFFFFFF)) );
	}

	// Scale input range.
	inline float operator *(float fRange) const
	{
		return (float)m_Key / float(0xFFFFFFFF) * fRange;
	}
	inline uint32 operator *(uint32 nRange) const
	{
		return m_Key % nRange;
	}

	uint32 GetKey() const
	{
		return m_Key;
	}

	// Jumble with a range variable to produce a random value.
	template<class T>
	inline T Random(T const* pRange) const
	{
		return Jumble(CChaosKey(uint32(pRange))) * *pRange;
	}

private:
	uint32	m_Key;

	static inline uint32 Jumble(uint32 key)
	{
		key += ~rot_left(key, 15);
		key ^= rot_right(key, 10);
		key += rot_left(key, 3);
		key ^= rot_right(key, 6);
		key += ~rot_left(key, 11);
		key ^= rot_right(key, 16);
		return key;
	}

	static inline uint32 rot_left(uint32 u, int n)
	{
		return (u<<n) | (u>>(32-n));
	}
	static inline uint32 rot_right(uint32 u, int n)
	{
		return (u>>n) | (u<<(32-n));
	}
};

//////////////////////////////////////////////////////////////////////////
//
// Float storage
//

#if !defined(PARTICLE_COMPRESSED_FLOATS)

	typedef TRangedType<float>					SFloat;
	typedef TRangedType<float,0>				UFloat;
	typedef TRangedType<float,0,1>			UnitFloat;
	typedef TRangedType<float,0,2>			Unit2Float;
	typedef TRangedType<float,-180,180>	SAngle;
	typedef TRangedType<float,0,180>		UHalfAngle;
	typedef TRangedType<float,0,360>		UFullAngle;

#else

	typedef SFloat16										SFloat;
	typedef UFloat16										UFloat;
	typedef UnitFloat8									UnitFloat;
	typedef TFixed<uint8,2>							Unit2Float8;
	typedef TFixed<int16,180>						SAngle;
	typedef TFixed<uint16,180>					UHalfAngle;
	typedef TFixed<uint16,360>					UFullAngle;

#endif

typedef TSmall<bool> TSmallBool;

typedef TFixed<uint8,1,240>		UnitFloat8e;

template<class T>
struct TStorageTraits
{
	typedef float				TValue;
	typedef UnitFloat8e	TMod;
	typedef UnitFloat		TRandom;
};

template<>
struct TStorageTraits<SFloat>
{
	typedef float				TValue;
	typedef UnitFloat8e	TMod;
	typedef Unit2Float	TRandom;
};

template<>
struct TStorageTraits<SAngle>
{
	typedef float				TValue;
	typedef UnitFloat8e	TMod;
	typedef Unit2Float	TRandom;
};

//////////////////////////////////////////////////////////////////////////
//
// Vec3 TypeInfo
//

// Must override Vec3 constructor to avoid polluting params with NANs
template<class T>
struct Vec3_Zero: Vec3_tpl<T>
{
	Vec3_Zero() : Vec3_tpl<T>(ZERO) {}
	Vec3_Zero(const Vec3& v) : Vec3_tpl<T>(v) {}
};

template<class T>
Vec3 BiRandom( const Vec3_Zero<T>& v )
{
	return BiRandom( Vec3(v) );
}

typedef Vec3_Zero<SFloat> Vec3S;
typedef Vec3_Zero<UFloat> Vec3U;

//////////////////////////////////////////////////////////////////////////
//
// Color specialisations.

template<class T>
struct Color3: Vec3_tpl<T>
{
	Color3( T v = T(0) )
		: Vec3_tpl<T>(v) {}

	Color3( T r, T g, T b )
		: Vec3_tpl<T>(r, g, b) {}

	template<class T2>
	Color3( Vec3_tpl<T2> const& c )
		: Vec3_tpl<T>(c) {}

	operator ColorF() const
		{ return ColorF(this->x, this->y, this->z, 1.f); }

	// Implement color multiplication.
	Color3& operator *=(Color3 const& c)
		{ this->x *= c.x; this->y *= c.y; this->z *= c.z; return *this; }
};

template<class T>
Color3<T> operator *(Color3<T> const& c, Color3<T> const& d)
	{ return Color3<T>(c.x * d.x, c.y * d.y, c.z * d.z); }

typedef Color3<float> Color3F;
typedef Color3< TFixed<uint8,1> > Color3B;

class RandomColor: public UnitFloat8
{
public:
	inline RandomColor(float f = 0.f, bool bRandomHue = false)
		: UnitFloat8(f), m_bRandomHue(bRandomHue)
	{}

	Color3F GetRandom() const
	{
		if (m_bRandomHue)
		{
			ColorB clr(cry_rand32());
			float fScale = float(*this) / 255.f;
			return Color3F(clr.r * fScale, clr.g * fScale, clr.b * fScale);
		}
		else
		{
			return Color3F(Random(float(*this)));
		}
	}

	AUTO_STRUCT_INFO

protected:
	TSmallBool	m_bRandomHue;
};

inline Color3F Random(RandomColor const& rc)
{
	return rc.GetRandom();
}

template<>
struct TStorageTraits<Color3B>
{
	typedef Color3F							TValue;
	typedef Color3<UnitFloat8e>	TMod;
	typedef RandomColor					TRandom;
};

template<>
struct TStorageTraits<Color3F>
{
	typedef Color3F							TValue;
	typedef Color3<UnitFloat8e>	TMod;
	typedef RandomColor					TRandom;
};

///////////////////////////////////////////////////////////////////////
//
// Spline implementation class.
//

template<>
inline const CTypeInfo& TypeInfo(ISplineInterpolator**)
{
	static CTypeInfo Info("ISplineInterpolator*", sizeof(void*), alignof(void*));
	return Info;
}

template<class S>
class TCurve: public spline::OptSpline< typename TStorageTraits<S>::TValue >
{
	typedef TCurve<S> TThis;
	typedef typename TStorageTraits<S>::TValue T;
	typedef typename TStorageTraits<S>::TMod TMod;
	typedef spline::OptSpline<T> super_type;

	using_type(super_type, source_spline)
	using_type(super_type, key_type)
	using super_type::num_keys;
	using super_type::key;
	using super_type::empty;

public:
	// Implement serialization for particles.
	string ToString( FToString flags = 0 ) const;
	bool FromString( cstr str, FFromString flags = 0 );

	// Spline interface.
	ILINE T GetValue(float fTime) const
	{
		if (empty())
			return T(1.f);
		T val;
		interpolate(fTime, val);
		return val;
	}
	T GetMinValue() const
	{
		if (empty())
			return T(1.f);
		T val;
		min_value(val);
		return val;
	}
	bool IsIdentity() const
	{
		return GetMinValue() == T(1.f);
	}

	struct CCustomInfo;
	CUSTOM_STRUCT_INFO(CCustomInfo)
};

//
// Composite parameter types, incorporating base value, randomness, and lifetime curves.
//

template<class S>
struct TVarParam: S
{
	typedef typename TStorageTraits<S>::TValue T;
	using_type(TStorageTraits<S>, TRandom)

	// Construction.
	TVarParam()
		: S()
	{
	}

	TVarParam(const T& tBase)
		: S(tBase)
	{
	}

	void Set(const T& tBase)
	{ 
		Base() = tBase;
	}
	void Set(const T& tBase, const TRandom& tRan)
	{ 
		Base() = tBase;
		m_Random = tRan;
	}

	//
	// Value extraction.
	//
	S& Base()
	{
		return static_cast<S&>(*this);
	}
	const S& Base() const
	{
		return static_cast<const S&>(*this);
	}

	ILINE bool operator !() const
	{
		return Base() == S(0);
	}

	ILINE T GetMaxValue() const
	{
		return Base();
	}
	ILINE T GetMinValue() const
	{
		return Base() - Base() * float(m_Random);
	}

	ILINE TRandom GetRandomRange() const
	{
		return m_Random;
	}

	ILINE T GetVarMod() const
	{
		T val = T(1.f);
		if (m_Random)
		{
			T ran(Random(m_Random));
			ran *= val;
			val -= ran;
		}
		return val;
	}

	ILINE T GetVarValue() const
	{
		return GetVarMod() * T(Base());
	}

	ILINE T GetVarValue(CChaosKey key) const
	{
		T val = Base();
		if (!!m_Random)
			val -= key.Jumble(this) * m_Random * val;
		return val;
	}

	ILINE T GetVarValue(float fRandom) const
	{
		T val = Base();
		val *= (1.f - fRandom * m_Random);
		return val;
	}

	ILINE T GetValueFromMod( T val ) const
	{
		return T(Base()) * val;
	}

	AUTO_STRUCT_INFO

protected:

	TRandom			m_Random;			// Random variation, multiplicative.
};

///////////////////////////////////////////////////////////////////////

template<class S>
struct TVarEParam: TVarParam<S>
{
	typedef typename TStorageTraits<S>::TValue	T;

	// Construction.
	ILINE TVarEParam()
	{
	}

	ILINE TVarEParam(const T& tBase)
		: TVarParam<S>(tBase)
	{
	}

	//
	// Value extraction.
	//

	T GetMinValue() const
	{
		T val = TVarParam<S>::GetMinValue();
		val *= m_EmitterStrength.GetMinValue();
		return val;
	}

	bool IsConstant() const
	{
		return m_EmitterStrength.IsIdentity();
	}

	ILINE T GetVarMod(float fEStrength) const
	{
		T val = T(1.f);
		if (m_Random)
			val -= Random(m_Random);
		val *= m_EmitterStrength.GetValue(fEStrength);
		return val;
	}

	ILINE T GetVarValue(float fEStrength) const
	{
		return GetVarMod(fEStrength) * T(Base());
	}

	ILINE T GetVarValue(float fEStrength, CChaosKey keyRan) const
	{
		T val = Base();
		if (!!m_Random)
			val -= keyRan.Jumble(this) * m_Random * val;
		val *= m_EmitterStrength.GetValue(fEStrength);
		return val;
	}

	ILINE T GetVarValue(float fEStrength, float fRandom) const
	{
		T val = Base();
		val *= (1.f - fRandom * m_Random);
		val *= m_EmitterStrength.GetValue(fEStrength);
		return val;
	}

	AUTO_STRUCT_INFO

protected:
	TCurve<S>		m_EmitterStrength;

	// Dependent name nonsense.
	using TVarParam<S>::Base;
	using TVarParam<S>::m_Random;
};

///////////////////////////////////////////////////////////////////////
template<class S>
struct TVarEPParam: TVarEParam<S>
{
	typedef typename TStorageTraits<S>::TValue	T;

	// Construction.
	TVarEPParam()
	{}

	TVarEPParam(const T& tBase)
		: TVarEParam<S>(tBase)
	{}

	//
	// Value extraction.
	//

	T GetMinValue() const
	{
		T val = TVarEParam<S>::GetMinValue();
		val *= m_ParticleLife.GetMinValue();
		return val;
	}

	ILINE bool IsConstant() const
	{
		return m_ParticleLife.IsIdentity();
	}

	ILINE T GetValueFromBase( T val, float fParticleAge ) const
	{
		return val * m_ParticleLife.GetValue(fParticleAge);
	}

	ILINE T GetValueFromMod( T val, float fParticleAge ) const
	{
		return T(Base()) * val * m_ParticleLife.GetValue(fParticleAge);
	}

	using TVarParam<S>::GetValueFromMod;

	AUTO_STRUCT_INFO

protected:

	TCurve<S>	m_ParticleLife;

	// Dependent name nonsense.
	using TVarEParam<S>::Base;
};


///////////////////////////////////////////////////////////////////////
// Special surface type enum

struct CSurfaceTypeIndex
{
	uint16	nIndex;

	STRUCT_INFO
};

///////////////////////////////////////////////////////////////////////
struct STextureTiling
{
	uint16	nTilesX, nTilesY;	// $<Min=1> $<Max=256> Number of tiles texture is split into 
	uint16	nFirstTile;				// First (or only) tile to use.
	uint16	nVariantCount;		// $<Min=1> Number of randomly selectable tiles; 0 or 1 if no variation.
	uint16	nAnimFramesCount;	// $<Min=1> Number of tiles (frames) of animation; 0 or 1 if no animation.
	TSmall<EAnimationCycle> eAnimCycle;
	TSmallBool bAnimBlend;		// Blend textures between frames.
	UFloat	fAnimFramerate;		// $<SoftMax=60> Tex framerate; 0 = 1 cycle / particle life.

	STextureTiling()
	{
		nFirstTile = 0;
		fAnimFramerate = 0.f;
		eAnimCycle = AnimationCycle_Once;
		bAnimBlend = 0;
		nTilesX = nTilesY = nVariantCount = nAnimFramesCount = 1;
	}

	int GetTileCount() const
	{
		return nTilesX * nTilesY - nFirstTile;
	}

	int GetFrameCount() const
	{
		return nVariantCount * nAnimFramesCount;
	}

	void Correct()
	{
		nTilesX = max(nTilesX, (uint16)1);
		nTilesY = max(nTilesY, (uint16)1);
		nFirstTile = min((uint16)nFirstTile, (uint16)(nTilesX*nTilesY-1));
		nAnimFramesCount = max(min((uint16)nAnimFramesCount, (uint16)GetTileCount()), (uint16)1);
		nVariantCount = max(min((uint16)nVariantCount, (uint16)(GetTileCount() / nAnimFramesCount) ), (uint16)1);
	}

	AUTO_STRUCT_INFO
};

///////////////////////////////////////////////////////////////////////
//! Particle system parameters
struct ParticleParams
{
	// <Group=Emitter>
	TSmallBool bEnabled;									// Set false to disable this effect.
	TSmallBool bContinuous;								// Emit particles gradually until nCount reached
	TSmall<EParticleSpawnType> eSpawnIndirection;
																				// Spawn from particles in parent emitter.
	TVarEParam<UFloat> fCount;						// Number of particles in system at once.

	// Timing
	TVarParam<UFloat> fSpawnDelay;				// Delay the actual spawn time by this value	
	TVarParam<UFloat> fEmitterLifeTime;		// Min lifetime of the emitter, 0 if infinite. Always emits at least nCount particles.
	TVarParam<SFloat> fPulsePeriod;				// Time between auto-restarts of finite systems. 0 if just once, or continuous.
	TVarEParam<UFloat> fParticleLifeTime;	// Lifetime of particle
	TSmallBool bRemainWhileVisible;				// Particles will only die when not rendered (by any viewport).

	// <Group=Location>
	TSmall<EGeomType> eAttachType;				// Where to emit from attached geometry.
	TSmall<EGeomForm> eAttachForm;				// How to emit from attached geometry.
	Vec3S vPositionOffset;								// Spawn Position offset from the effect spawn position
	Vec3U vRandomOffset;									// Random offset of the particle relative position to the spawn position
	UnitFloat fOffsetRoundness;						// Fraction of corners to round: 0 = box, 1 = ellipsoid.
	UnitFloat fOffsetInnerScale;						// Fraction of inner emit volume to avoid.

	// <Group=Angles>
	TVarEParam<UHalfAngle> fFocusAngle;		// Angle to vary focus from default (Y axis), for variation.
	TVarEParam<SFloat> fFocusAzimuth;			// $<SoftMax=360> Angle to rotate focus about default, for variation. 0 = Z axis.
	TSmallBool bFocusGravityDir;					// Uses negative gravity dir, rather than emitter Y, as focus dir.
	TSmallBool bFocusRotatesEmitter;			// Focus rotation is equivalent to emitter rotation; else affects just emission direction.
	TSmallBool bEmitOffsetDir;						// Default emission direction parallel to emission offset from origin.
	TVarEParam<UHalfAngle> fEmitAngle;		// Angle from focus dir (emitter Y), in degrees. RandomVar determines min angle.
	
	// <Group=Appearance>
	TSmall<EParticleFacing> eFacing;			// The basic particle shape type.
	TSmallBool bOrientToVelocity;					// Rotate particle X axis in velocity direction.

	TSmall<EParticleBlendType> eBlendType;// Blend rendering type.
	CCryName sTexture;										// Texture for particle.
	STextureTiling TextureTiling;					// Animation etc.	
	CCryName sMaterial;										// Material (overrides texture).
	CCryName sGeometry;										// Geometry for 3D particles.
	TVarEPParam<UFloat> fAlpha;						// Alpha value (opacity, or multiplier for additive).
	UnitFloat fAlphaTest;									// Alpha test reference value (AlphaTest from material overrides this).
	TVarEPParam<Color3B> cColor;					// Color modulation.

	TSmallBool bOctagonalShape;						// Use octogonal shape for particles instead of quad.
	TSmallBool bGeometryInPieces;					// Spawn geometry pieces separately.
	TSmallBool bSoftParticle;							// Enable soft particle processing in the shader.

	// <Group=Lighting>
	UFloat fDiffuseLighting;							// $<SoftMax=1> Multiplier for particle dynamic lighting.
	UnitFloat8 fDiffuseBacklighting;			// Fraction of diffuse lighting applied in all directions.
	UFloat fEmissiveLighting;							// $<SoftMax=1> Multiplier for particle emissive lighting.
	SFloat fEmissiveHDRDynamic;						// $<SoftMin=-2> $<SoftMax=2> Power to apply to engine HDR multiplier for emissive lighting in HDR.

	TSmallBool bReceiveShadows;						// Shadows will cast on these particles.
	TSmallBool bCastShadows;							// Particles will cast shadows (currently only geom particles).
	TSmallBool bNotAffectedByFog;
	TSmallBool bGlobalIllumination;				// Enable global illumination in the shader.
	TSmallBool bDiffuseCubemap;						// Use nearest deferred cubemap for diffuse lighting
	TFixed<uint8,MAX_HEATSCALE> fHeatScale;	// Multiplier to thermal vision.

	struct SLightSource
	{
		TVarEPParam<UFloat> fRadius;				// $<SoftMax=10>
		TVarEPParam<UFloat> fIntensity;			// $<SoftMax=10>
		SFloat fHDRDynamic;									// $<SoftMin=-2> $<SoftMax=2>
		AUTO_STRUCT_INFO
	} LightSource;

	// <Group=Sound>
	CCryName sSound;											// Name of the sound file
	TVarEParam<UFloat> fSoundFXParam;			// Custom real-time sound modulation parameter.
	TSmall<ESoundControlTime> eSoundControlTime;		// The sound control time type

	// <Group=Size>
	TVarEPParam<UFloat> fSize;						// $<SoftMax=10> Particle size.
	struct SStretch: TVarEPParam<UFloat>
	{
		SFloat fOffsetRatio;								// $<SoftMin=-1> $<SoftMax=1> Move particle center this fraction in direction of stretch.
		AUTO_STRUCT_INFO
	} fStretch;														// $<SoftMax=1> Stretch particle into moving direction
	struct STailLength: TVarEPParam<UFloat>
	{
		uint8 nTailSteps;										// $<SoftMax=16> Number of tail segments
		AUTO_STRUCT_INFO
	} fTailLength;												// $<SoftMax=10> Length of tail in seconds
	UFloat fMinPixels;										// $<SoftMax=10> Augment true size with this many pixels.

	// Connection
	struct SConnection: TSmallBool
	{
		TSmallBool bConnectToOrigin;							// Newest particle connected to emitter origin.
		TSmall<ETextureMapping> eTextureMapping;	// Basis of texture coord mapping (particle or stream).
		UFloat fTextureFrequency;									// Number of mirrored texture wraps in line.

		SConnection() : 
			fTextureFrequency(1.f)
		{}
		AUTO_STRUCT_INFO
	} Connection;

	// <Group=Movement>
	TVarEParam<SFloat> fSpeed;						// Initial speed of a particle.
	SFloat fInheritVelocity;							// $<SoftMin=0> $<SoftMax=1> Fraction of emitter's velocity to inherit.
	TVarEPParam<UFloat> fAirResistance;		// $<SoftMax=10> Air drag value, in inverse seconds.
	UFloat fRotationalDragScale;					// $<SoftMax=1> Multipler to AirResistance, for rotational motion.
	TVarEPParam<SFloat> fGravityScale;		// $<SoftMin=-1> $<SoftMax=1> Multiplier for world gravity.
	Vec3S vAcceleration;									// Specific world-space acceleration vector.
	TVarEPParam<UFloat> fTurbulence3DSpeed;// $<SoftMax=10> 3D random turbulence force
	TVarEPParam<UFloat> fTurbulenceSize;	// $<SoftMax=10> Radius of turbulence
	TVarEPParam<SFloat> fTurbulenceSpeed;	// $<SoftMin=-360> $<SoftMax=360> Speed of rotation

	EMoveRelative eMoveRelEmitter;				// Particle motion is in emitter space.
	TSmallBool bBindEmitterToCamera;			// Emitter is camera-relative.
	TSmallBool bSpaceLoop;								// Lock particles in box around emitter position; if BindEmitterToCamera, use CameraMaxDistance, else PositionOffset & RandomOffset

	struct STargetAttraction							// Customise movement toward ParticleTargets.
	{
		TSmallBool bIgnore;
		TSmallBool bExtendSpeed;						// Extend particle speed as necessary to reach target in normal lifetime.
		TSmallBool bShrink;									// Shrink particle as it approaches target.
		TSmallBool bOrbit;
		TVarEPParam<SFloat> fOrbitDistance;	// If > 0, orbits at specified distance rather than disappearing.

		AUTO_STRUCT_INFO
	} TargetAttraction;

	// <Group=Rotation>
	Vec3_Zero<SAngle> vInitAngles;				// Initial rotation in symmetric angles (degrees).
	Vec3_Zero<UFullAngle> vRandomAngles;	// Bidirectional random angle variation.
	Vec3S vRotationRate;									// $<SoftMin=-360> $<SoftMax=360> Rotation speed (degree/sec).
	Vec3U vRandomRotationRate;						// $<SoftMax=360> Random variation.

	// <Group=Collision>
	TSmall<EParticlePhysicsType> ePhysicsType;
	TSmallBool bCollideTerrain;						// Collides with terrain (if Physics <> none)
	TSmallBool bCollideStaticObjects;			// Collides with static physics objects (if Physics <> none)
	TSmallBool bCollideDynamicObjects;		// Collides with dynamic physics objects (if Physics <> none)
	CSurfaceTypeIndex sSurfaceType;				// Surface type for physicalised particles.
	SFloat fBounciness;										// $<SoftMin=0> $<SoftMax=1> Elasticity: 0 = no bounce, 1 = full bounce, <0 = die.
	UFloat fDynamicFriction;							// $<SoftMax=10> Sliding drag value, in inverse seconds.
	UFloat fThickness;										// $<SoftMax=1> Lying thickness ratio - for physicalized particles only
	UFloat fDensity;											// $<SoftMax=2000> Mass density for physicslized particles.
	uint8 nMaxCollisionEvents;						// Max # collision events per particle (0 = no limit).

	// <Group=Visibility>
	UFloat fCameraMaxDistance;						// $<SoftMax=100> Max distance from camera to render particles.
	UFloat fCameraMinDistance;						// $<SoftMax=100> Min distance from camera to render particles.
	UFloat fViewDistanceAdjust;						// $<SoftMax=1> Multiplier to automatic distance fade-out.
	int8 nDrawLast;												// $<SoftMin=-16> $<SoftMax=16> Adjust draw order within effect group.
	TSmallBool bDrawNear;									// Render particle in near space (weapon)
	TSmall<ETrinary> tVisibleIndoors;			// Whether visible indoors/outdoors/both.
	TSmall<ETrinary> tVisibleUnderwater;	// Whether visible under/above water / both.

	// <Group=Advanced>
	TSmall<EParticleForceType> eForceGeneration;	// Generate physical forces if set.
	UFloat fFillRateCost;									// $<SoftMax=10> Adjustment to max screen fill allowed per emitter.
#if PARTICLE_MOTION_BLUR
	UFloat fMotionBlurScale;							// $<SoftMax=2> Multiplier to motion blur.
	UFloat fMotionBlurStretchScale;				// $<SoftMax=10> Multiplier for motion blur sprite stretching based on particle movement.
	UFloat fMotionBlurCamStretchScale;		// $<SoftMax=10> Multiplier for motion blur sprite stretching based on camera movement.
#endif
	TSmallBool bDrawOnTop;								// Render particle on top of everything (no depth test)
	TSmallBool bNoOffset;									// Disable centering of static objects
	TSmallBool bAllowMerging;							// Do not merge drawcalls for particle emitters of this type
	TSmall<EParticleHalfResFlag> eAllowHalfRes;	// Do not use half resolution rendering
	TSmallBool bStreamable;								// True if the texture/geometry allowed to be streamed.

	// <Group=Configuration>
	TSmall<EConfigSpecBrief> eConfigMin;
	TSmall<EConfigSpecBrief> eConfigMax;
	TSmall<ETrinary> tDX11;

	ParticleParams() 
	{
		memset(this, 0, sizeof(*this));
		new(this) ParticleParams(true);
	}

	// Derived properties
	int GetMaxVertexCount(int nDefaultVerts = 1) const
	{
		if (fTailLength.nTailSteps > 0)
			return (fTailLength.nTailSteps+3) * 2;
		else if (Connection)
			return 2;
		else
			return nDefaultVerts;
	}
	int GetMaxIndexCount(int nDefaultVerts = 1) const
	{
		if (fTailLength.nTailSteps > 0)
			return (fTailLength.nTailSteps+2) * 6;
		else if (nDefaultVerts == 1)
			return 0;
		else
			return (nDefaultVerts-2) * 3;
	}
	bool HasVariableVertexCount() const
	{
		return fTailLength.nTailSteps || Connection;
	}

	AUTO_STRUCT_INFO

protected:

	ParticleParams(bool) :
		bEnabled(true),
		fSize(1.f),
		cColor(1.f),
		fAlpha(1.f),
		eBlendType(ParticleBlendType_AlphaBased),
		fDiffuseLighting(1.f),
		bReceiveShadows(true),
		bGlobalIllumination(true),
		bStreamable(true),
		bOctagonalShape(true),
		fViewDistanceAdjust(1.f),
		fFillRateCost(1.f),
		fRotationalDragScale(1.f),
		fDensity(1000.f),
		fThickness(1.f),
#if PARTICLE_MOTION_BLUR
		fMotionBlurScale(1.f),
		fMotionBlurStretchScale(1.f),
		fMotionBlurCamStretchScale(1.f),
#endif
		fSoundFXParam(1.f),
		bAllowMerging(true),
		eConfigMax(ConfigSpec_VeryHigh),
		eSoundControlTime(SoundControlTime_EmitterLifeTime)
	{}
};

#endif
