#include DEVIRTUALIZE_HEADER_FIX(CREParticle.h)

#ifndef __CREPARTICLE_H__
#define __CREPARTICLE_H__

#include "CryThread.h"
#include <IJobManager.h>

typedef SVF_P3F_C4B_T4B_N3F2		SVertexParticle;

// forward declarations
class CREParticle;
struct CRenderListData;


struct SRenderVertices
{
	FixedDynArray<SVertexParticle>	aVertices;
	FixedDynArray<uint16>						aIndices;
	int															nBaseVertexIndex;
	float														fMaxPixels;
	float														fPixels;

	SRenderVertices()
		{ memset(this, 0, sizeof(*this)); }

	inline void CopyVertices( Array<SVertexParticle>& aSrc )
	{
		int nVerts = min(aSrc.size(), aVertices.available());
		cryMemcpy(aVertices.grow_raw(nVerts), &aSrc[0], sizeof(SVertexParticle) * nVerts);
		aSrc.erase_front(nVerts);
	}

	ILINE static void DuplicateVertices( Array<SVertexParticle> aDest, const SVertexParticle& Src )
	{
		static const uint nStride = sizeof(SVertexParticle) / sizeof(uint);
		CryPrefetch(aDest.end());
		const uint* ps = reinterpret_cast<const uint*>(&Src);
		uint* pd = reinterpret_cast<uint*>(aDest.begin());
		for (uint n = aDest.size(); n > 0; n--)
		{
			for (uint i = 0; i < nStride; i++)
				*pd++ = ps[i];
		}
	}

	ILINE static void SetQuadVertices(SVertexParticle aV[4])
	{
		aV[1].st.x = 255;
		aV[2].st.y = 255;
		aV[3].st.x = 255;
		aV[3].st.y = 255;
	}

	inline void ExpandQuadVertices()
	{
		SVertexParticle& vert = aVertices.back();
		DuplicateVertices(aVertices.append_raw(3), vert);
		SetQuadVertices(&vert);
	}

	inline void ExpandQuadVertices(const SVertexParticle& RESTRICT_REFERENCE vert)
	{
		DuplicateVertices(aVertices.append_raw(4), vert);
		SetQuadVertices(aVertices.end()-4);
	}

	inline void SetOctVertices(SVertexParticle aV[8])
	{
		aV[0].st.x = 75;
		aV[1].st.x = 180;
		aV[2].st.x = 255;
		aV[2].st.y = 75;
		aV[3].st.x = 255;
		aV[3].st.y = 180;
		aV[4].st.x = 180;
		aV[4].st.y = 255;
		aV[5].st.x = 75;
		aV[5].st.y = 255;
		aV[6].st.y = 180;
		aV[7].st.y = 75;
	}

	inline void ExpandOctVertices()
	{
		SVertexParticle& vert = aVertices.back();
		DuplicateVertices(aVertices.append_raw(7), vert);
		SetOctVertices(&vert);
	}

	inline void ExpandOctVertices(const SVertexParticle& RESTRICT_REFERENCE vert)
	{
		DuplicateVertices(aVertices.append_raw(8), vert);
		SetOctVertices(aVertices.end()-8);
	}

	inline void SetQuadIndices(int nVertAdvance = 4)
	{
		uint16* pIndices = aIndices.grow(6);

		pIndices[0] = 0 + nBaseVertexIndex;
		pIndices[1] = 1 + nBaseVertexIndex;
		pIndices[2] = 2 + nBaseVertexIndex;

		pIndices[3] = 3 + nBaseVertexIndex;
		pIndices[4] = 2 + nBaseVertexIndex;
		pIndices[5] = 1 + nBaseVertexIndex;

		nBaseVertexIndex += nVertAdvance;
	}

	inline void SetQuadsIndices()
	{
		assert((aVertices.size() & 3) == 0);
		int nQuads = aVertices.size() >> 2;
		assert(aIndices.available() >= nQuads*6);
		while (nQuads-- > 0)
			SetQuadIndices();
	}

	inline void SetOctIndices()
	{
		uint16* pIndices = aIndices.grow(18);

		pIndices[0] = 0 + nBaseVertexIndex;
		pIndices[1] = 1 + nBaseVertexIndex;
		pIndices[2] = 2 + nBaseVertexIndex;
		pIndices[3] = 0 + nBaseVertexIndex;
		pIndices[4] = 2 + nBaseVertexIndex;
		pIndices[5] = 4 + nBaseVertexIndex;
		pIndices[6] = 2 + nBaseVertexIndex;
		pIndices[7] = 3 + nBaseVertexIndex;
		pIndices[8] = 4 + nBaseVertexIndex;
		pIndices[9] = 0 + nBaseVertexIndex;
		pIndices[10] = 4 + nBaseVertexIndex;
		pIndices[11] = 6 + nBaseVertexIndex;
		pIndices[12] = 4 + nBaseVertexIndex;
		pIndices[13] = 5 + nBaseVertexIndex;
		pIndices[14] = 6 + nBaseVertexIndex;
		pIndices[15] = 6 + nBaseVertexIndex;
		pIndices[16] = 7 + nBaseVertexIndex;
		pIndices[17] = 0 + nBaseVertexIndex;

		nBaseVertexIndex += 8;
	}

	inline void SetOctsIndices()
	{
		assert((aVertices.size() & 7) == 0);
		int nOcts = aVertices.size() >> 3;
		assert(aIndices.available() >= nOcts*18);

		while (nOcts-- > 0)
			SetOctIndices();
	}

	inline void SetPolyIndices( int nVerts )
	{
		nVerts >>= 1;
		while (--nVerts > 0)
			SetQuadIndices(2);

		// Final quad.
		nBaseVertexIndex += 2;
	}

	void SetPoliesIndices( Array<SVertexParticle>& aSrcVerts, Array<uint16>& aSrcVertCounts )
	{
		int nAvailVerts = aVertices.available();
		int nVerts = 0;
		int nPolygon = 0;
		for (; nPolygon < aSrcVertCounts.size(); nPolygon++)
		{
			int nPolyVerts = aSrcVertCounts[nPolygon];
			if (nVerts + nPolyVerts > nAvailVerts)
				break;
			nVerts += nPolyVerts;
			SetPolyIndices(nPolyVerts);
		}
		aSrcVertCounts.erase_front(nPolygon);

		cryMemcpy(aVertices.grow_raw(nVerts), aSrcVerts.begin(), sizeof(SVertexParticle) * nVerts);
		aSrcVerts.erase_front(nVerts);
	}
};

struct IAllocRender: SRenderVertices
{
	// Render existing SVertices, alloc new ones.
	virtual void Alloc( int nAllocVerts, int nAllocInds = 0 ) = 0; 
	virtual CREParticle* RenderElement() const { return 0; }
	virtual ~IAllocRender(){}
};

UNIQUE_IFACE struct IParticleVertexCreator
{
	// Create the vertices for the particle emitter.
	virtual void ComputeVertices( const CCamera& cam, IAllocRender& alloc ) = 0;
	virtual float GetDistSquared( const Vec3& vPos ) const = 0;
	virtual float GetApproxParticleArea () const = 0;

	// Reference counting.
	virtual void AddRef() = 0;
	virtual void Release() = 0;
	virtual ~IParticleVertexCreator() {}
};

class CREParticle : public CRendElementBase
{
public:
	CREParticle( IParticleVertexCreator* pVC, const CCamera& cam );

	void Reset( IParticleVertexCreator* pVC, const CCamera& cam );

	// Custom copy constructor required to avoid m_Lock copy.
	CREParticle( const CREParticle& in )
	: m_pVertexCreator(in.m_pVertexCreator)
	, m_pCamera(in.m_pCamera)
	, m_fPixels(0.f)
	{
	}

	virtual ~CREParticle();
	virtual void Release(bool bForce);

	virtual void GetMemoryUsage(ICrySizer *pSizer) const 
	{
		//pSizer->AddObject(this, sizeof(*this)); // allocated in own allocator
	}
	// CRendElement implementation.
	static CREParticle* Create( IParticleVertexCreator* pVC, const CCamera& cam, int nThreadList );

	virtual CRendElementBase* mfCopyConstruct()
	{
		return new CREParticle(*this);
	}
  virtual int Size()
	{
		return sizeof(*this);
	}

	virtual void mfPrepare(bool bCheckOverflow);
	virtual float mfDistanceToCameraSquared( Matrix34& matInst );

	virtual bool mfPreDraw( SShaderPass *sl );
	virtual bool mfDraw( CShader *ef, SShaderPass *sl );

	// Additional methods.
	void StoreVertices( bool bWait );
	void TransferVertices() const;

	void SetVertices( Array<SVertexParticle> aVerts, Array<uint16> aVertCounts, float fPixels )
	{
		m_aVerts = aVerts;
		m_aVertCounts = aVertCounts;
		m_fPixels = fPixels;
	}

	float GetPixelCount() const
	{
		return m_fPixels;
	}
	JobManager::SJobState* GetSPUState()
	{
		return &m_SPUState;
	}

	void ResetVertexCreator()
	{
		m_pVertexCreator = NULL;
	}

private:
	IParticleVertexCreator*							m_pVertexCreator;		// Particle object which computes vertices.
	CCamera const*											m_pCamera;
	Array<SVertexParticle>							m_aVerts;						// Computed particle vertices.
	Array<uint16>												m_aVertCounts;			// Verts in each particle (multi-seg particles only).
	float																m_fPixels;					// Total pixels rendered.
	JobManager::SJobState								m_SPUState;
};

#endif  // __CREPARTICLE_H__
