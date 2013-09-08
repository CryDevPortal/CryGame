//////////////////////////////////////////////////////////////////////
//
//	Crytek Common Source code
//	
//	File: Cry_Vector3.h
//	Description: Common vector class
//
//	History:
//	-Feb 27,2003: Created by Ivo Herzeg
//
//////////////////////////////////////////////////////////////////////

#ifndef XENON_VECTOR_H
#define XENON_VECTOR_H

#if _MSC_VER > 1000
# pragma once
#endif





//we need a conversion type to access the members in a XMVECTOR, union is defined inside Cry_PS3_Math.h
#if !defined(PS3)
	#define XMVECTOR_CONV XMVECTOR




#endif





#include "Endian.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// class Vec3_tpl
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


template <> struct __passinreg Vec3_tpl<float>
{
	typedef float F;
	typedef F value_type;
	union
	{
		struct
		{
			F x,y,z;
		};
		struct
		{
			XMFLOAT3 m;
		};
	};

#ifdef _DEBUG
	ILINE Vec3_tpl() 
	{
		if (sizeof(F)==4)
		{
			uint32* p=alias_cast<uint32*>(&x);		p[0]=F32NAN;	p[1]=F32NAN; p[2]=F32NAN;
		}
		if (sizeof(F)==8)
		{
			uint64* p=alias_cast<uint64*>(&x);		p[0]=F64NAN;	p[1]=F64NAN; p[2]=F64NAN;
		}
	}
#else
	ILINE Vec3_tpl()	{};
#endif

	/*!
	* template specailization to initialize a vector 
	* 
	* Example:
	*  Vec3 v0=Vec3(ZERO);
	*  Vec3 v1=Vec3(MIN);
	*  Vec3 v2=Vec3(MAX);
	*/
	Vec3_tpl(type_zero) : x(0),y(0),z(0) {}
	inline Vec3_tpl(type_min) { x=y=z=-3.3E38f; }
	inline Vec3_tpl(type_max) { x=y=z=3.3E38f; }

	/*!
	* constrctors and bracket-operator to initialize a vector 
	* 
	* Example:
	*  Vec3 v0=Vec3(1,2,3);
	*  Vec3 v1(1,2,3);
	*  v2.Set(1,2,3);
	*/
	ILINE Vec3_tpl( F vx, F vy, F vz ) : x(vx),y(vy),z(vz){ assert(this->IsValid()); }
	ILINE void operator () ( F vx, F vy, F vz ) { x=vx; y=vy; z=vz; assert(this->IsValid()); }
	template<class F>
		explicit ILINE Vec3_tpl(const Ang3_tpl<F>& v) : x((F)v.x), y((F)v.y), z((F)v.z) { assert(this->IsValid()); }
	ILINE Vec3_tpl<F>& Set(const F xval,const F yval, const F zval) { x=xval; y=yval; z=zval; assert(this->IsValid()); return *this; }

	explicit Vec3_tpl( F f ) : x(f),y(f),z(f) { assert(this->IsValid()); }

	/*!
	* the copy/casting/assignement constructor 
	* 
	* Example:
	*  Vec3 v0=v1;
	*/
#if !defined(__SPU__) // Note: This copy constructor changes crycg inlining behaviour so that errors in animation occur
  ILINE Vec3_tpl( const Vec3_tpl<F>& v ) : x(v.x), y(v.y), z(v.z)  { /*assert(this->IsValid());*/ }
#endif

#ifndef __SPU__
	template <class T> ILINE  Vec3_tpl( const Vec3_tpl<T>& v ) : x((F)v.x), y((F)v.y), z((F)v.z) { assert(this->IsValid()); }
	ILINE Vec3_tpl &operator=(const Vec3_tpl<F> &src) { x=src.x; y=src.y; z=src.z; return *this; }
















#endif
	ILINE Vec3_tpl(const Vec2_tpl<F>& v) : x((F)v.x), y((F)v.y), z(0) { assert(this->IsValid()); }


	/*!
	* overloaded arithmetic operator  
	* 
	* Example:
	*  Vec3 v0=v1*4;
	*/
	ILINE Vec3_tpl<F> operator * (F k) const
	{
		return Vec3_tpl<F>(x*k, y*k, z*k);
  //  XMVECTOR X = XMVec4::LoadVec3(&m);
		//Vec3_tpl<F> res;
		//XMVec4::StoreVec3(&res.m, XMVectorScale(X,k));
		//return res;
	}

	ILINE friend Vec3_tpl<F> operator * (f32 f, const Vec3_tpl &vec)
	{
		return Vec3_tpl<F>(vec.x*f, vec.y*f, vec.z*f);
		//XMVECTOR X = XMVec4::LoadVec3(&vec.m);
		//Vec3_tpl<F> res;
		//XMVec4::StoreVec3(&res.m, XMVectorScale(X,f));
		//return res;
	}

	ILINE Vec3_tpl<F> operator / (F k) const
	{
		return Vec3_tpl<F>(x/k, y/k, z/k);
		//XMVECTOR X = XMVec4::LoadVec3(&m);
		//Vec3_tpl<F> res;
		//XMVec4::StoreVec3(&res.m, XMVectorScale(X, 1.0f / k));
		//return res;
	}

	ILINE Vec3_tpl<F>& operator *= (F k)
	{
		//XMVECTOR X = XMVec4::LoadVec3(&m);
		//XMVec4::StoreVec3(&m, XMVectorScale(X,k));
		x *=k; y *=k; z*=k;
		return *this;
	}
	ILINE Vec3_tpl<F>& operator /= (F k)
	{
		//XMVECTOR X = XMVec4::LoadVec3(&m);
		//XMVec4::StoreVec3(&m, XMVectorScale(X, 1.0f / k));
		x /=k; y /=k; z/=k;
		return *this;
	}

	ILINE Vec3_tpl<F> operator - ( void ) const
	{
		return Vec3_tpl(-x, -y, -z);
	}

	ILINE Vec3_tpl<F>& Flip()
	{
		XMVECTOR X = XMVec4::LoadVec3(&m);
		XMVec4::StoreVec3(&m, XMVectorNegate(X));
		return *this;
	}

	/*!
	* bracked-operator   
	* 
	* Example:
	*  uint32 t=v[0];
	*/
	ILINE F &operator [] (int32 index)		  { assert(index>=0 && index<=2); return ((F*)this)[index]; }
	ILINE F operator [] (int32 index) const { assert(index>=0 && index<=2); return ((F*)this)[index]; }


	/*!
	* functions and operators to compare vector   
	* 
	* Example:
	*  if (v0==v1) dosomething;
	*/
	ILINE bool operator==(const Vec3_tpl<F> &vec)
	{
		XMVECTOR X = XMVec4::LoadVec3(&m);
		XMVECTOR Y = XMVec4::LoadVec3(&vec.m);
		return static_cast<bool>(XMVector3Equal(X, Y));
	}
	ILINE bool operator!=(const Vec3_tpl<F> &vec) { return !(*this == vec); }

	ILINE friend bool operator ==(const Vec3_tpl<F> &v0, const Vec3_tpl<F> &v1)
	{
		XMVECTOR X = XMVec4::LoadVec3(&v0.m);
		XMVECTOR Y = XMVec4::LoadVec3(&v1.m);
		return static_cast<bool>(XMVector3Equal(X, Y));
	}
	ILINE friend bool operator !=(const Vec3_tpl<F> &v0, const Vec3_tpl<F> &v1)	{	return !(v0==v1);	}

	ILINE bool IsZero(F e=(F)0.0) const
	{
		XMVECTOR X = XMVec4::LoadVec3(&m);
		return static_cast<bool>(XMVector3NearEqual(X, XMVectorZero(), XMVectorReplicate(e)));
	}

	ILINE bool IsZeroFast(F e=(F)0.0003) const
	{
		return (fabs_tpl(x) + fabs_tpl(y) +fabs_tpl(z))<e;
	}

	ILINE bool IsEquivalent(const Vec3_tpl<F>& v1, F epsilon=VEC_EPSILON) const 
	{
		assert(v1.IsValid()); 
		assert(this->IsValid()); 
		XMVECTOR X = XMVec4::LoadVec3(&m);
		XMVECTOR Y = XMVec4::LoadVec3(&v1.m);
		return static_cast<bool>(XMVector3NearEqual(X, Y, XMVectorReplicate(epsilon)));
	}

	ILINE bool IsUnit(F epsilon=VEC_EPSILON) const 
	{
		return (fabs_tpl(1 - GetLengthSquared()) <= epsilon);
	}

	ILINE bool IsValid() const
	{
		if (!NumberValid(x)) return false;
		if (!NumberValid(y)) return false;
		if (!NumberValid(z)) return false;
		return true;
	}

	//! force vector length by normalizing it
	ILINE void SetLength(F fLen)
	{ 
		XMVECTOR X = XMVec4::LoadVec3(&m);
		XMVECTOR fLenMe = XMVector3LengthSq(X);
		if (XMVector4Less(fLenMe, XMVectorReplicate(0.00001f*0.00001f)))
			return;
		//FIXME: do we need high precision here? If yes we have to use XMVectorReciprocalSqrt (slower)
		XMVECTOR_CONV c = XMVectorReciprocalSqrt(fLenMe);
		fLen *= c.v[0];
		XMVec4::StoreVec3(&m, XMVectorScale(X,fLen));
	}

	ILINE void ClampLength(F maxLength)
	{
		F sqrLength = GetLengthSquared();
		if (sqrLength > (maxLength * maxLength))
		{
			F scale = maxLength * isqrt_tpl(sqrLength);
			x *= scale; y *= scale; z *= scale;
		}
	}

	//! calculate the length of the vector
	ILINE F	GetLength() const
	{
		XMVECTOR X = XMVec4::LoadVec3(&m);
		XMVECTOR_CONV c = XMVector3Length(X);
		return c.v[0];
	}

	ILINE F	GetLengthFloat() const
	{
		return sqrt_fast_tpl(x*x+y*y+z*z);
	}

	ILINE F	GetLengthFast() const
	{
		XMVECTOR X = XMVec4::LoadVec3(&m);
		XMVECTOR_CONV c = XMVector3LengthEst(X);
		return c.v[0];
	}		

	//! calculate the squared length of the vector
	ILINE F GetLengthSquared() const
	{
		XMVECTOR X = XMVec4::LoadVec3(&m);
		XMVECTOR_CONV c = XMVector3LengthSq(X);
		return c.v[0];
	}

	//! calculate the squared length of the vector using floats to avoid LHS
	ILINE F GetLengthSquaredFloat() const
	{
		return x*x+y*y+z*z;
	}

	//! calculate the length of the vector ignoring the z component
	ILINE F	GetLength2D() const
	{
		XMVECTOR X = XMVec4::LoadVec2(&x);
		XMVECTOR_CONV c = XMVector2Length(X);
		return c.v[0];
	}		

	//! calculate the squared length of the vector ignoring the z component
	ILINE F	GetLengthSquared2D() const
	{
		XMVECTOR X = XMVec4::LoadVec2(&x);
		XMVECTOR_CONV c = XMVector2LengthSq(X);
		return c.v[0];
	}		

	ILINE F GetDistance(const Vec3_tpl<F> vec1) const
	{
		XMVECTOR X = XMVec4::LoadVec3(&m);
		XMVECTOR Y = XMVec4::LoadVec3(&vec1.m);
		XMVECTOR_CONV c = XMVector3Length(X-Y);
		return c.v[0];
	}
	ILINE F GetSquaredDistance ( const Vec3_tpl<F> &v) const
	{		
		XMVECTOR X = XMVec4::LoadVec3(&m);
		XMVECTOR Y = XMVec4::LoadVec3(&v.m);
		XMVECTOR_CONV c = XMVector3LengthSq(X-Y);
		return c.v[0];
	}
	ILINE F GetSquaredDistance2D ( const Vec3_tpl<F> &v) const
	{	
		return  (x-v.x)*(x-v.x) + (y-v.y)*(y-v.y);

// 		XMVECTOR X = XMVec4::LoadVec2(&x);
// 		XMVECTOR Y = XMVec4::LoadVec2(&v.x);
// 		XMVECTOR_CONV c = XMVector2LengthSq(X-Y);
// 		return c.v[0];
	}

	//! normalize the vector
	// this version is "safe", zero vectors remain zero
	ILINE void Normalize() 
	{ 
		assert(this->IsValid()); 
		XMVECTOR X = XMVec4::LoadVec3(&m);
		X = XMVector3Normalize(X);
		XMVec4::StoreVec3(&m, X);
	}

	//! may be faster and less accurate
	ILINE void NormalizeFast() 
	{
		assert(this->IsValid()); 
		XMVECTOR X = XMVec4::LoadVec3(&m);
		X = XMVector3NormalizeEst(X);
		XMVec4::StoreVec3(&m, X);
	}

	//! normalize the vector
	// check for null vector - set to the passed in vector (which should be normalised!) if it is null vector
	// returns the original length of the vector
	ILINE F NormalizeSafe(const struct Vec3_tpl<F>& safe = Vec3Constants<F>::fVec3_Zero) 
	{ 
		assert(this->IsValid()); 
		XMVECTOR X = XMVec4::LoadVec3(&m);
		XMVECTOR fLen2 = XMVector3LengthSq(X);
		XMVECTOR_CONV c = fLen2;
		if (c.v[0]>0.0f)
		{
			XMVECTOR fInvLen = XMVectorReciprocalSqrt(fLen2);
			XMVec4::StoreVec3(&m, XMVectorMultiply(X,fInvLen));
			c = fInvLen;
			return F(1) / c.v[0];
		}
		else
		{
			*this = safe;
			return F(0);
		}
	}

	//! return a normalized vector using floats to avoid LHS penalties
	ILINE Vec3_tpl GetNormalizedFloat() const 
	{ 
		F fInvLen = isqrt_safe_tpl( x*x+y*y+z*z );
		return *this * fInvLen;
	}

	//! return a normalized vector
	ILINE Vec3_tpl GetNormalized() const 
	{ 
		XMVECTOR X = XMVec4::LoadVec3(&m);
		XMVECTOR Norm = XMVector3Normalize(X);
		Vec3_tpl <F> v;
		XMVec4::StoreVec3(&v.m, Norm);
		return v;
	}

	//! return a safely normalized vector - returns safe vector (should be normalised) if original is zero length
	ILINE Vec3_tpl GetNormalizedSafe(const struct Vec3_tpl<F>& safe = Vec3Constants<F>::fVec3_OneX) const 
	{ 
		XMVECTOR X = XMVec4::LoadVec3(&m);
		XMVECTOR fLen2 = XMVector3LengthSq(X);
		XMVECTOR_CONV c = fLen2;
		if (c.v[0]>0.0f)
		{
			XMVECTOR fInvLen = XMVectorReciprocalSqrt(fLen2);
			Vec3_tpl <F> v;
			XMVec4::StoreVec3(&v.m, XMVectorMultiply(X,fInvLen));
			return v;
		}
		else
		{
			return safe;
		}
	}

	//! return a safely normalized vector - returns safe vector (should be normalised) if original is zero length
	ILINE Vec3_tpl GetNormalizedSafeFloat(const struct Vec3_tpl<F>& safe = Vec3Constants<F>::fVec3_OneX) const 
	{
		const float fSizeSq = Dot(*this);
		const float fNegSizeSq = -fSizeSq;
		const float fDivisorSq	= (F)__fsel(fNegSizeSq, 1.0f, fSizeSq);

		const float fInvDivisor		= isqrt_tpl(fDivisorSq);

		return Vec3_tpl((F)__fsel(fNegSizeSq, x * fInvDivisor, safe.x),
										(F)__fsel(fNegSizeSq, y * fInvDivisor, safe.y),
										(F)__fsel(fNegSizeSq, z * fInvDivisor, safe.z));
	}

	// permutate coordinates so that z goes to new_z slot
	ILINE Vec3_tpl GetPermutated(int new_z) const { return Vec3_tpl(*(&x+inc_mod3[new_z]), *(&x+dec_mod3[new_z]), *(&x+new_z)); }

	// returns volume of a box with this vector as diagonal 
	ILINE F GetVolume() const { return x*y*z; }

	// returns a vector that consists of absolute values of this one's coordinates
	ILINE Vec3_tpl<F> abs() const
	{
		XMVECTOR X = XMVec4::LoadVec3(&x);
		XMVECTOR AbsV = XMVectorAbs(X);
		Vec3_tpl<F> v;
		XMVec4::StoreVec3(&v.x, AbsV);
		return v;
	}

	//! check for min bounds
	ILINE void CheckMin(const Vec3_tpl<F> &other)
	{ 
		x = min(other.x,x);
		y = min(other.y,y);
		z = min(other.z,z);
	}			
	//! check for max bounds
	ILINE void CheckMax(const Vec3_tpl<F> &other)
	{
		x = max(other.x,x);
		y = max(other.y,y);
		z = max(other.z,z);
	}


	/*!
	* sets a vector orthogonal to the input vector 
	* 
	* Example:
	*  Vec3 v;
	*  v.SetOrthogonal( i );
	*/
	ILINE void SetOrthogonal( const Vec3_tpl<F>& v )
	{
		//#ifndef XENON_INTRINSICS
		int i = isneg(square((F)0.9)*v.GetLengthSquared()-v.x*v.x);
		(*this)[i]=0; (*this)[incm3(i)]= v[decm3(i)];	(*this)[decm3(i)]=-v[incm3(i)];
		/*#else
		XMVECTOR X = XMLoadVector3(&v.x);
		XMVECTOR Orth = XMVector3Orthogonal(X);
		XMStoreVector3(&x, Orth);
		#endif*/
	}
	// returns a vector orthogonal to this one
	ILINE Vec3_tpl GetOrthogonal() const
	{
		//#ifndef XENON_INTRINSICS
		int i = isneg(square((F)0.9)*GetLengthSquared()-x*x);
		Vec3_tpl<F> res;
		res[i]=0; res[incm3(i)]=(*this)[decm3(i)]; res[decm3(i)]=-(*this)[incm3(i)];
		return res;
		/*#else
		XMVECTOR X = XMLoadVector3(&x);
		XMVECTOR Orth = XMVector3Orthogonal(X);
		Vec3_tpl<F> res;
		XMStoreVector3(&res.x, Orth);
		return res;
		#endif*/
	}

	/*!
	* Project a point/vector on a (virtual) plane 
	* Consider we have a plane going through the origin. 
	* Because d=0 we need just the normal. The vector n is assumed to be a unit-vector.
	* 
	* Example:
	*  Vec3 result=Vec3::CreateProjection( incident, normal );
	*/
	ILINE void SetProjection( const Vec3_tpl& i, const Vec3_tpl& n ) { *this = i-n*(n|i); }
	ILINE static Vec3_tpl<F> CreateProjection( const Vec3_tpl& i, const Vec3_tpl& n ) { return i-n*(n|i); }

	/*!
	* Calculate a reflection vector. Vec3 n is assumed to be a unit-vector.
	* 
	* Example:
	*  Vec3 result=Vec3::CreateReflection( incident, normal );
	*/
	ILINE void SetReflection( const Vec3_tpl& i, const Vec3_tpl &n )
	{
		//#ifndef XENON_INTRINSICS
		*this=(n*(i|n)*2)-i;
		/*#else
		XMVECTOR I = XMLoadVector3(&i.x);
		XMVECTOR N = XMLoadVector3(&n.x);
		XMVECTOR ReflV = XMVector3Reflect(I, N);
		XMStoreVector3(&x, ReflV);
		#endif*/
	}
	ILINE static Vec3_tpl<F> CreateReflection( const Vec3_tpl& i, const Vec3_tpl &n )
	{
		//#ifndef XENON_INTRINSICS
		return (n*(i|n)*2)-i;
		/*#else
		XMVECTOR I = XMLoadVector3(&i.x);
		XMVECTOR N = XMLoadVector3(&n.x);
		XMVECTOR ReflV = XMVector3Reflect(I, N);
		Vec3_tpl<F> res;
		XMStoreVector3(&res.x, ReflV);
		return res;
		#endif*/
	}

	/*!
	* Linear-Interpolation between Vec3 (lerp)
	* 
	* Example:
	*  Vec3 r=Vec3::CreateLerp( p, q, 0.345f );
	*/
	ILINE void SetLerp( const Vec3_tpl<F> &p, const Vec3_tpl<F> &q, F t )
	{
		XMVECTOR P = XMLoadVector3(&p.x);
		XMVECTOR Q = XMLoadVector3(&q.x);
		XMVECTOR LerpV = XMVectorLerp(P, Q, t);
		XMVec4::StoreVec3(&x, LerpV);
	}
	ILINE static Vec3_tpl<F> CreateLerp( const Vec3_tpl<F> &p, const Vec3_tpl<F> &q, F t )
	{
		XMVECTOR P = XMLoadVector3(&p.x);
		XMVECTOR Q = XMLoadVector3(&q.x);
		XMVECTOR LerpV = XMVectorLerp(P, Q, t);
		Vec3_tpl<F> res;
		XMVec4::StoreVec3(&res.x, LerpV);
		return res;
	}


	/*!
	* Spherical-Interpolation between 3d-vectors (geometrical slerp)
	* both vectors are assumed to be normalized.  
	*
	* Example:
	*  Vec3 r=Vec3::CreateSlerp( p, q, 0.5674f );
	*/
	ILINE void SetSlerp( const Vec3_tpl<F>& p, const Vec3_tpl<F>& q, F t ) {
		assert(p.IsUnit(0.005f));
		assert(q.IsUnit(0.005f));
		// calculate cosine using the "inner product" between two vectors: p*q=cos(radiant)
		F cosine = clamp_tpl((p|q),-1.0f,1.0f);
		//we explore the special cases where the both vectors are very close together, 
		//in which case we approximate using the more economical LERP and avoid "divisions by zero" since sin(Angle) = 0  as   Angle=0
		if(cosine>=(F)0.99) {
			SetLerp(p,q,t); //perform LERP:
			Normalize();
		}	else {
			//perform SLERP: because of the LERP-check above, a "division by zero" is not possible
			F rad				= acos_tpl(cosine);
			F scale_0   = sin_tpl((1-t)*rad);
			F scale_1   = sin_tpl(t*rad);
			*this=(p*scale_0 + q*scale_1) / sin_tpl(rad);
			Normalize();
		}
	}
	ILINE static Vec3_tpl<F> CreateSlerp( const Vec3_tpl<F>& p, const Vec3_tpl<F>& q, F t ) {
		Vec3_tpl<F> v;	v.SetSlerp(p,q,t); return v;	
	}

	/*!
	* set random normalized vector (=random position on unit sphere) 
	* 
	* Example:
	*  Vec3 v;
	*  v.SetRandomDirection(); 
	*/
	void SetRandomDirection( void )
	{ 
		int nMax = 5; // To prevent hanging with bad randoms.
		F Length2;
		do {
			x = 1.0f - 2.0f*cry_frand();
			y = 1.0f - 2.0f*cry_frand();
			z = 1.0f - 2.0f*cry_frand();
			Length2 = len2();
			nMax--;
		} while((Length2>1.0f || Length2<0.0001f) && nMax > 0);
		F InvScale = isqrt_tpl(Length2);				// dividion by 0 is prevented
		x *= InvScale; y *= InvScale; z *= InvScale;
	}	


	/*!
	* rotate a vector using angle&axis 
	* 
	* Example:
	*  Vec3 r=v.GetRotated(axis,angle);
	*/
	ILINE Vec3_tpl GetRotated(const Vec3_tpl &axis, F angle) const
	{ 
		return GetRotated(axis,cos_tpl(angle),sin_tpl(angle)); 
	}
	ILINE Vec3_tpl GetRotated(const Vec3_tpl &axis, F cosa,F sina) const {
		Vec3_tpl zax = axis*(*this|axis); 
		Vec3_tpl xax = *this-zax; 
		Vec3_tpl yax = axis%xax;
		return xax*cosa + yax*sina + zax;
	}

	/*!
	* rotate a vector around a center using angle&axis 
	* 
	* Example:
	*  Vec3 r=v.GetRotated(axis,angle);
	*/
	ILINE Vec3_tpl GetRotated(const Vec3_tpl &center,const Vec3_tpl &axis, F angle) const { 
		return center+(*this-center).GetRotated(axis,angle); 
	}
	ILINE Vec3_tpl GetRotated(const Vec3_tpl &center,const Vec3_tpl &axis, F cosa,F sina) const { 
		return center+(*this-center).GetRotated(axis,cosa,sina); 
	}

	/*!
	* component wise multiplication of two vectors
	*/
	ILINE Vec3_tpl CompMul( const Vec3_tpl& rhs ) const { 
		return( Vec3_tpl( x * rhs.x, y * rhs.y, z * rhs.z ) ); 
	}

	//DEPRICATED ILINE friend F GetDistance(const Vec3_tpl<F> &vec1, const Vec3_tpl<F> &vec2) { 
	//return  sqrt_tpl((vec2.x-vec1.x)*(vec2.x-vec1.x)+(vec2.y-vec1.y)*(vec2.y-vec1.y)+(vec2.z-vec1.z)*(vec2.z-vec1.z)); 
	//}	
	//DEPRICATED ILINE friend F	GetSquaredDistance(const Vec3_tpl<F> &vec1, const Vec3_tpl<F> &vec2)	{		
	//return (vec2.x-vec1.x)*(vec2.x-vec1.x)+(vec2.y-vec1.y)*(vec2.y-vec1.y)+(vec2.z-vec1.z)*(vec2.z-vec1.z);
	//}
	//three methods for a "dot-product" operation
	ILINE F Dot (const Vec3_tpl<F> &vec2)	const
	{
    return vec2.x*x + vec2.y*y + vec2.z*z;
//    XMVECTOR X = XMVec4::LoadVec3(&x);
//		XMVECTOR Y = XMVec4::LoadVec3(&vec2.x);
//		return XMVector3Dot(X, Y).v[0];
	}
	//two methods for a "cross-product" operation
	ILINE Vec3_tpl<F> Cross (const Vec3_tpl<F> &vec2) const
	{
		//#ifndef XENON_INTRINSICS
		return Vec3_tpl<F>( y*vec2.z  -  z*vec2.y,     z*vec2.x -    x*vec2.z,   x*vec2.y  -  y*vec2.x);
		/*#else
		XMVECTOR X = XMLoadVector3(&x);
		XMVECTOR Y = XMLoadVector3(&vec2.x);
		XMVECTOR CrossV = XMVector3Cross(X, Y);
		Vec3_tpl<F> res;
		XMStoreVector3(&res.x, CrossV);
		return res;
		#endif*/
	}	

	//f32* fptr=vec;
	DEPRICATED operator F* ()					{ return (F*)this; }
	template <class T>	explicit DEPRICATED Vec3_tpl(const T *src) { x=src[0]; y=src[1]; z=src[2]; }

	ILINE Vec3_tpl& zero() { x=y=z=0; return *this; }

	//! Legacy aliases.
	ILINE F len() const
	{
		XMVECTOR X = XMVec4::LoadVec3(&x);
		XMVECTOR_CONV c =XMVector3Length(X);
		return c.v[0];
	}
	ILINE F len2() const
	{
		XMVECTOR X = XMVec4::LoadVec3(&x);
		XMVECTOR_CONV c =XMVector3LengthSq(X);
		return c.v[0];
	}

	ILINE Vec3_tpl& normalize()
	{ 
		Normalize();
		return *this; 
	}
	ILINE Vec3_tpl normalized() const
	{ 
		return GetNormalized();
	}

	// function overrides which should be used for uncached XMox memory!!!
	//vector subtraction
	template<class F1>
	ILINE Vec3_tpl<F1> sub(const Vec3_tpl<F1> &v) const
	{
		return Vec3_tpl<F1>(x-v.x, y-v.y, z-v.z);
	}

	//vector dot product
	template<class F1>
	ILINE F1 dot(const Vec3_tpl<F1> &v) const
	{
		return (F1)(x*v.x+y*v.y+z*v.z); 
	}
	//vector scale
	template<class F1>
	ILINE Vec3_tpl<F1> scale(const F1 k) const
	{
		return Vec3_tpl<F>(x*k,y*k,z*k);
	}

	//vector cross product
	template<class F1>
	ILINE Vec3_tpl<F1> cross(const Vec3_tpl<F1> &v) const
	{
		return Vec3_tpl<F1>(y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x); 
	}

	AUTO_STRUCT_INFO
	//static const CTypeInfo& TypeInfo();
};


typedef Vec3_tpl<f32>		Vec3;			//we will use only this throughout the project
#ifndef REAL_IS_FLOAT
typedef Vec3_tpl<real>	Vec3r;		//for systems with float precision higher then 64bit
#else
typedef Vec3_tpl<f32>	Vec3r;		//for systems with float precision higher then 64bit
#endif
typedef Vec3_tpl<f64>	  Vec3_f64; //for double-precision
typedef Vec3_tpl<int>	  Vec3i;		//for integers
template<> inline Vec3_tpl<f64>::Vec3_tpl(type_min) { x=y=z=-1.7E308; }
template<> inline Vec3_tpl<f64>::Vec3_tpl(type_max) { x=y=z=1.7E308; }

//////////////////////////////////////////////////////////////////////////
// Random vector functions.

inline Vec3 Random(const Vec3& v)
{
	return Vec3( Random(v.x), Random(v.y), Random(v.z) );
}
inline Vec3 Random(const Vec3& a, const Vec3& b)
{
	return Vec3( Random(a.x,b.x), Random(a.y,b.y), Random(a.z,b.z) );
}

// Random point in sphere.
inline Vec3 SphereRandom(float fRadius)
{
	Vec3 v;
	do {
		v( BiRandom(fRadius), BiRandom(fRadius), BiRandom(fRadius) );
	} while (v.GetLengthSquared() > fRadius*fRadius);
	return v;
}

// Return a normalized copy of the vector flattened to the XY plane.
static inline Vec3 Vec3FlattenXY(const Vec3 & vIn)
{
	return Vec3(vIn.x,vIn.y,0.0f).GetNormalizedSafe(Vec3Constants<float>::fVec3_OneY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// class Vec4_tpl
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////













	template <typename F> struct __passinreg Vec4_tpl_base
	{
		F x,y,z,w;
	};



#ifndef PS3
template<>  struct __passinreg __declspec(align(16)) Vec4_tpl_base<float> //: public XMFLOAT4
{
	union {
		struct {
			float x,y,z,w;
		};
		XMVECTOR v;
	};
};
#endif

template <typename F> struct __passinreg Vec4_tpl : public Vec4_tpl_base<F>
{
	//	F x,y,z,w;

#ifdef _DEBUG
	ILINE Vec4_tpl() 
	{
		if (sizeof(F)==4)
		{
			uint32* p=alias_cast<uint32*>(&this->x);		p[0]=F32NAN;	p[1]=F32NAN; p[2]=F32NAN; p[3]=F32NAN;
		}
		if (sizeof(F)==8)
		{
			uint64* p=alias_cast<uint64*>(&this->x);		p[0]=F64NAN;	p[1]=F64NAN; p[2]=F64NAN; p[3]=F64NAN;
		}

	}
#else
	ILINE Vec4_tpl()	{};
#endif

  template<typename F2>
  ILINE Vec4_tpl<F>& operator = (const Vec4_tpl<F2> &v1) 
  {
    (*this).x=F(v1.x); (*this).y=F(v1.y); (*this).z=F(v1.z); (*this).w=F(v1.w);
    return (*this);
  }

	ILINE Vec4_tpl( F vx, F vy, F vz, F vw ) { this->x=vx; this->y=vy; this->z=vz; this->w=vw; };
	ILINE Vec4_tpl( const Vec3_tpl<F> &v, F vw ) {  this->x=v.x; this->y=v.y; this->z=v.z; this->w=vw; };

	ILINE void operator () ( F vx, F vy, F vz, F vw ) { this->x=vx; this->y=vy; this->z=vz; this->w=vw; };
	ILINE void operator () ( const Vec3_tpl<F> &v, F vw ) {  this->x=v.x; this->y=v.y; this->z=v.z; vw=vw; };

	ILINE F &operator [] (int index)		  { assert(index>=0 && index<=3); return ((F*)this)[index]; }
	ILINE F operator [] (int index) const { assert(index>=0 && index<=3); return ((F*)this)[index]; }
	template <class T> ILINE  Vec4_tpl( const Vec4_tpl<T>& v )  {  x=F(v.x); y=F(v.y); z=F(v.z); w=F(v.w); assert(this->IsValid()); }

	ILINE F Dot (const Vec4_tpl<F> &vec2);
	ILINE F GetLength();
	ILINE void Normalize() 
	{
		assert(this->IsValid()); 
		*this *= isqrt_safe_tpl(Dot(*this));
	}
	ILINE void NormalizeFast() 
	{
		assert(this->IsValid()); 
		*this *= isqrt_safe_tpl(Dot(*this));
	}























	ILINE bool IsValid() const
	{
		if (!NumberValid(this->x)) return false;
		if (!NumberValid(this->y)) return false;
		if (!NumberValid(this->z)) return false;
		if (!NumberValid(this->w)) return false;
		return true;
	}

  /*!
  * functions and operators to compare vector   
  * 
  * Example:
  *  if (v0==v1) dosomething;
  */
  ILINE bool operator==(const Vec4_tpl<F> &vec)
  {
    XMVECTOR X = XMVec4::LoadVec4(&x);
    XMVECTOR Y = XMVec4::LoadVec4(&vec.x);
    return static_cast<bool>(XMVector4Equal(X, Y));
  }
  ILINE bool operator!=(const Vec4_tpl<F> &vec) { return !(*this == vec); }

  ILINE friend bool operator ==(const Vec4_tpl<F> &v0, const Vec4_tpl<F> &v1)
  {
    XMVECTOR X = XMVec4::LoadVec4(&v0.x);
    XMVECTOR Y = XMVec4::LoadVec4(&v1.x);
    return static_cast<bool>(XMVector4Equal(X, Y));
  }
  ILINE friend bool operator !=(const Vec4_tpl<F> &v0, const Vec4_tpl<F> &v1)	{	return !(v0==v1);	}

	ILINE void SetLerp( const Vec4_tpl<F> &p, const Vec4_tpl<F> &q, F t ) {	*this = p*(1.0f-t) + q*t;}
	ILINE static Vec4_tpl<F> CreateLerp( const Vec4_tpl<F> &p, const Vec4_tpl<F> &q, F t ) {	return p*(1.0f-t) + q*t;}

	AUTO_STRUCT_INFO
} 



;

typedef Vec4_tpl<real>	Vec4r;		//for systems with float precision higher then 64bit


template<class F>
ILINE Vec4_tpl<F> operator / (const Vec4_tpl<F>& v, F k)
{ 
	k=(F)1.0/k; 
	return Vec4_tpl<F>(v.x*k,v.y*k,v.z*k,v.w*k); 
}

//template<>
ILINE Vec4_tpl<float> operator / (const Vec4_tpl<float>& v, __vecreg float k)
{ 
	Vec4_tpl<float> t;
  XMVECTOR xm = XMVec4::LoadVec4(&v);
	xm = XMVectorScale(xm, 1.0f / k);
  XMVec4::StoreVec4(&t, xm);
	return t;
}

template<class F>
ILINE Vec4_tpl<F> operator * (const Vec4_tpl<F>& v, F k)
{ 
	return Vec4_tpl<F>(v.x*k,v.y*k,v.z*k,v.w*k); 
}

//template<>
ILINE Vec4_tpl<float> operator * (const Vec4_tpl<float>& v, __vecreg float k)
{ 
	Vec4_tpl<float> t;
  XMVECTOR xm = XMVec4::LoadVec4(&v);
	xm = XMVectorScale(xm, k);
  XMVec4::StoreVec4(&t, xm);
	return t;
}

template<class F>
ILINE F Vec4_tpl<F>::Dot (const Vec4_tpl<F> &vec2)
{ 
	return this->x*vec2.x + this->y*vec2.y + this->z*vec2.z + this->w*vec2.w; 
}

template<>
ILINE float Vec4_tpl<float>::Dot (const Vec4_tpl<float> &vec2)
{ 
	Vec4_tpl<float> t(*this);
  XMVECTOR xm0 = XMVec4::LoadVec4(&x);
  XMVECTOR xm1 = XMVec4::LoadVec4(&vec2);
	XMVECTOR_CONV c = XMVector4Dot(xm0, xm1);
	return c.x;
}

template<class F>
ILINE F Vec4_tpl<F>::GetLength()
{ 
	return sqrt_tpl(Dot(*this)); 
}

template<>
ILINE float Vec4_tpl<float>::GetLength()
{ 
	XMVECTOR_CONV c = XMVector4Length(XMVec4::LoadVec4(this));
	return c.x; 
}

template<class F>
ILINE Vec4_tpl<F>& operator *= (Vec4_tpl<F>& v,  F k) 
{ 
	v.x*=k;
	v.y*=k;
	v.z*=k;
	v.w*=k; 
	return v; 
}

ILINE Vec4_tpl<float>& operator *= (Vec4_tpl<float>& v, __vecreg float k) 
{ 
	v = v * k;
	return v; 
}

template<class F>
ILINE Vec4_tpl<F>& operator /= (Vec4_tpl<F>& v, F k) 
{ 
	k=(F)1.0/k; 
	v.x*=k;		v.y*=k;		v.z*=k;		v.w*=k; 
	return v; 
}










//vector self-addition
template<class F1,class F2>
ILINE Vec4_tpl<F1>& operator += (Vec4_tpl<F1> &v0, const Vec4_tpl<F2> &v1) 
{
	v0.x+=v1.x; v0.y+=v1.y; v0.z+=v1.z; v0.w+=v1.w;
	return v0;
}

//vector addition
template<class F1,class F2>
ILINE Vec4_tpl<F1> operator + (const Vec4_tpl<F1> &v0, const Vec4_tpl<F2> &v1) 
{




	return Vec4_tpl<F1>(v0.x+v1.x, v0.y+v1.y, v0.z+v1.z, v0.w+v1.w);

}









/*
ILINE Vec4_tpl<float> operator + (const Vec4_tpl<float> &v0, const Vec4_tpl<float> &v1) 
{
	Vec4_tpl<float> v;
	v = v0 + v1;
	return v;
}
*/
//vector subtraction
template<class F1,class F2>
ILINE Vec4_tpl<F1> operator - (const Vec4_tpl<F1> &v0, const Vec4_tpl<F2> &v1) 
{




	return Vec4_tpl<F1>(v0.x-v1.x, v0.y-v1.y, v0.z-v1.z, v0.w-v1.w);

}

//vector subtraction
/*ILINE Vec4_tpl<float> operator - (const Vec4_tpl<float> &v0, const Vec4_tpl<float> &v1) 
{
	Vec4_tpl<float> v;
	v = v0 - v1;
	return v;
}
*/
//vector multiplication
template<class F1,class F2>
ILINE Vec4_tpl<F1> operator * (const Vec4_tpl<F1> &v0, const Vec4_tpl<F2> &v1) 
{




	return Vec4_tpl<F1>(v0.x*v1.x, v0.y*v1.y, v0.z*v1.z, v0.w*v1.w);

}

//vector multiplication
//template<>
//ILINE Vec4_tpl<float> operator * (const Vec4_tpl<float> &v0, const Vec4_tpl<float> &v1) 
//{
//	Vec4_tpl<float> v;
//	v = v0 * v1;
//	return v;
//}

//vector division
template<class F1,class F2>
ILINE Vec4_tpl<F1> operator / (const Vec4_tpl<F1> &v0, const Vec4_tpl<F2> &v1) 
{




	return Vec4_tpl<F1>(v0.x/v1.x, v0.y/v1.y, v0.z/v1.z, v0.w/v1.w);

}

//template<>
//ILINE Vec4_tpl<float> operator / (const Vec4_tpl<float> &v0, const Vec4_tpl<float> &v1) 
//{
//	Vec4_tpl<float> v;
//	v = v0 / v1;
//	return v;
//}


typedef Vec4_tpl<f32>		Vec4;			//we will use only this throughout the project
typedef DEFINE_ALIGNED_DATA(Vec4, Vec4A, 16); // typedef __declspec(align(16)) Vec4_tpl<f32>		Vec4A;			//we will use only this throughout the project
typedef Vec4_tpl<real>	Vec4r;		//for systems with float precision higher then 64bit


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// struct Ang3_tpl
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template <typename F> struct __passinreg  Ang3_tpl
{
	F x,y,z;

#ifdef _DEBUG
	ILINE Ang3_tpl() 
	{
		if (sizeof(F)==4)
		{
			uint32* p=alias_cast<uint32*>(&x);		p[0]=F32NAN;	p[1]=F32NAN; p[2]=F32NAN;
		}
		if (sizeof(F)==8)
		{
			uint64* p=alias_cast<uint64*>(&x);		p[0]=F64NAN;	p[1]=F64NAN; p[2]=F64NAN;
		}
	}
#else
	ILINE Ang3_tpl()	{};
#endif


	Ang3_tpl(type_zero) { x=y=z=0; }

	void operator () ( F vx, F vy,F vz ) { x=vx; y=vy; z=vz; };
	ILINE Ang3_tpl<F>( F vx, F vy, F vz )	{	x=vx; y=vy; z=vz;	}  

	explicit ILINE Ang3_tpl(const Vec3_tpl<F>& v) : x((F)v.x), y((F)v.y), z((F)v.z) { assert(this->IsValid()); }

	ILINE bool operator==(const Ang3_tpl<F> &vec) { return x == vec.x && y == vec.y && z == vec.z; }
	ILINE bool operator!=(const Ang3_tpl<F> &vec) { return !(*this == vec); }

	ILINE Ang3_tpl<F> operator * (F k) const { return Ang3_tpl<F>(x*k,y*k,z*k); }
	ILINE Ang3_tpl<F> operator / (F k) const { k=(F)1.0/k; return Ang3_tpl<F>(x*k,y*k,z*k); }


	ILINE Ang3_tpl<F>& operator *= (F k) { x*=k;y*=k;z*=k; return *this; }
	//explicit ILINE Ang3_tpl<F>& operator = (const Vec3_tpl<F>& v)  { x=v.x; y=v.y; z=v.z; return *this; 	}

	ILINE Ang3_tpl<F> operator - ( void ) const { return Ang3_tpl<F>(-x,-y,-z); }

	ILINE friend bool operator ==(const Ang3_tpl<F> &v0, const Ang3_tpl<F> &v1)	{
		return ((v0.x==v1.x) && (v0.y==v1.y) && (v0.z==v1.z));
	}
	ILINE void Set(F xval,F yval,F zval) { x=xval; y=yval; z=zval; }

	ILINE bool IsEquivalent( const Ang3_tpl<F> v1, F epsilon=VEC_EPSILON) const {
		return  ((fabs_tpl(x-v1.x) <= epsilon) &&	(fabs_tpl(y-v1.y) <= epsilon)&&	(fabs_tpl(z-v1.z) <= epsilon));	
	}
	ILINE bool IsInRangePI() const {
		F pi=(F)(gf_PI+0.001); //we need this to fix fp-precision problem 
		return  (  (x>-pi)&&(x<pi) && (y>-pi)&&(y<pi) && (z>-pi)&&(z<pi) );	
	}
	//! normalize angle to -pi and +pi range 
	ILINE void RangePI() {
		if (x< (F)gf_PI) x+=(F)gf_PI2;
		if (x> (F)gf_PI) x-=(F)gf_PI2;
		if (y< (F)gf_PI) y+=(F)gf_PI2;
		if (y> (F)gf_PI) y-=(F)gf_PI2;
		if (z< (F)gf_PI) z+=(F)gf_PI2;
		if (z> (F)gf_PI) z-=(F)gf_PI2;
	}


	//Convert unit quaternion to angle (xyz).
	template<class F1> explicit ILINE Ang3_tpl( const Quat_tpl<F1>& q )
	{
		assert(q.IsValid());
		y = (F)asin_tpl(max((F)-1.0,min((F)1.0,-(q.v.x*q.v.z-q.w*q.v.y)*2)));
		if (fabs_tpl(fabs_tpl(y)-(F)(g_PI*0.5))<(F)0.01)	
		{
			x = (F)0;
			z = (F)atan2_tpl(-2*(q.v.x*q.v.y-q.w*q.v.z),1-(q.v.x*q.v.x+q.v.z*q.v.z)*2);
		} else {
			x = (F)atan2_tpl((q.v.y*q.v.z+q.w*q.v.x)*2, 1-(q.v.x*q.v.x+q.v.y*q.v.y)*2);
			z = (F)atan2_tpl((q.v.x*q.v.y+q.w*q.v.z)*2, 1-(q.v.z*q.v.z+q.v.y*q.v.y)*2);
		}
	}

	//Convert matrix33 to angle (xyz).
	template<class F1> explicit ILINE Ang3_tpl( const Matrix33_tpl<F1>& m )
	{
		assert( m.IsOrthonormalRH(0.001f) );
		y = (F)asin_tpl(max((F)-1.0,min((F)1.0,-m.m20)));
		if (fabs_tpl(fabs_tpl(y)-(F)(g_PI*0.5))<(F)0.01)	
		{
			x = (F)0;
			z = (F)atan2_tpl(-m.m01,m.m11);
		} else {
			x = (F)atan2_tpl(m.m21, m.m22);
			z = (F)atan2_tpl(m.m10, m.m00);
		}
	}

	//Convert matrix34 to angle (xyz).
	template<class F1, class B> explicit ILINE Ang3_tpl( const Matrix34_tpl<F1, B>& m )
	{
		assert( m.IsOrthonormalRH(0.001f) );
		y = (F)asin_tpl(max((F)-1.0,min((F)1.0,-m.m20)));
		if (fabs_tpl(fabs_tpl(y)-(F)(g_PI*0.5))<(F)0.01)	
		{
			x = (F)0;
			z = (F)atan2_tpl(-m.m01,m.m11);
		} else {
			x = (F)atan2_tpl(m.m21, m.m22);
			z = (F)atan2_tpl(m.m10, m.m00);
		}
	}

	//Convert matrix34 to angle (xyz).
	template<class F1, class B> explicit ILINE Ang3_tpl( const Matrix44_tpl<F1, B>& m )
	{
		assert( Matrix33(m).IsOrthonormalRH(0.001f) );
		y = (F)asin_tpl(max((F)-1.0,min((F)1.0,-m.m20)));
		if (fabs_tpl(fabs_tpl(y)-(F)(g_PI*0.5))<(F)0.01)	
		{
			x = (F)0;
			z = (F)atan2_tpl(-m.m01,m.m11);
		} else {
			x = (F)atan2_tpl(m.m21, m.m22);
			z = (F)atan2_tpl(m.m10, m.m00);
		}
	}

	template<typename F1>	ILINE static F CreateRadZ( const Vec2_tpl<F1>& v0, const Vec2_tpl<F1>& v1 )
	{
		F cz	= v0.x*v1.y-v0.y*v1.x; 
		F c		=	v0.x*v1.x+v0.y*v1.y;
		return F( atan2(cz,c) );
	}

	template<typename F1>	ILINE static F CreateRadZ( const Vec3_tpl<F1>& v0, const Vec3_tpl<F1>& v1 )
	{
		F cx	= v0.y*v1.z; 
		F cz	= v0.x*v1.y-v0.y*v1.x; 
		F s		=	sgn(cz)*sqrt(cx*cx+cz*cz);
		F c		=	v0.x*v1.x+v0.y*v1.y;
		return F( atan2(s,c) );
	}

	template<typename F1>	ILINE static Ang3_tpl<F> GetAnglesXYZ( const Quat_tpl<F1>& q ) {	return Ang3_tpl<F>(q); }
	template<typename F1>	ILINE void SetAnglesXYZ( const Quat_tpl<F1>& q )	{	*this=Ang3_tpl<F>(q);	}

	template<typename F1>	ILINE static Ang3_tpl<F> GetAnglesXYZ( const Matrix33_tpl<F1>& m ) {	return Ang3_tpl<F>(m); }
	template<typename F1>	ILINE void SetAnglesXYZ( const Matrix33_tpl<F1>& m )	{	*this=Ang3_tpl<F>(m);	}

	template<typename F1, class B>	ILINE static Ang3_tpl<F> GetAnglesXYZ( const Matrix34_tpl<F1, B>& m ) {	return Ang3_tpl<F>(m); }
	template<typename F1, class B>	ILINE void SetAnglesXYZ( const Matrix34_tpl<F1, B>& m )	{	*this=Ang3_tpl<F>(m);	}

	//---------------------------------------------------------------
	ILINE F &operator [] (int index)		  { assert(index>=0 && index<=2); return ((F*)this)[index]; }
	ILINE F operator [] (int index) const { assert(index>=0 && index<=2); return ((F*)this)[index]; }


	ILINE bool IsValid() const
	{
		if (!NumberValid(x)) return false;
		if (!NumberValid(y)) return false;
		if (!NumberValid(z)) return false;
		return true;
	}

	AUTO_STRUCT_INFO
};

typedef Ang3_tpl<f32>		Ang3;
typedef Ang3_tpl<real>	Ang3r;
typedef Ang3_tpl<f64>		Ang3_f64;

//---------------------------------------

//vector addition
template<class F1,class F2>
ILINE Ang3_tpl<F1> operator + (const Ang3_tpl<F1> &v0, const Ang3_tpl<F2> &v1) {
	return Ang3_tpl<F1>(v0.x+v1.x, v0.y+v1.y, v0.z+v1.z);
}
//vector subtraction
template<class F1,class F2>
ILINE Ang3_tpl<F1> operator - (const Ang3_tpl<F1> &v0, const Ang3_tpl<F2> &v1) {
	return Ang3_tpl<F1>(v0.x-v1.x, v0.y-v1.y, v0.z-v1.z);
}

//---------------------------------------

//vector self-addition
template<class F1,class F2>
ILINE Ang3_tpl<F1>& operator += (Ang3_tpl<F1> &v0, const Ang3_tpl<F2> &v1) {
	v0.x+=v1.x; v0.y+=v1.y; v0.z+=v1.z; return v0;
}
//vector self-subtraction
template<class F1,class F2>
ILINE Ang3_tpl<F1>& operator -= (Ang3_tpl<F1> &v0, const Ang3_tpl<F2> &v1) {
	v0.x-=v1.x; v0.y-=v1.y; v0.z-=v1.z; return v0;
}


//! normalize the val to 0-360 range 
//
//ILINE f32 Snap_s360( f32 val ) {
//if( val < 0.0f )
//val =f32( 360.0f + cry_fmod(val,360.0f));
//else
//if(val >= 360.0f)
//val =f32(cry_fmod(val,360.0f));
//return val;
//}
//
////! normalize the val to -180, 180 range 
//ILINE f32 Snap_s180( f32 val ) {
//if( val > -180.0f && val < 180.0f)
//return val;
//val = Snap_s360( val );
//if( val>180.0f )
//return -(360.0f - val);
//return val;
//}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// struct CAngleAxis
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
template <typename F> struct AngleAxis_tpl {

	//! storage for the Angle&Axis coordinates.
	F angle; Vec3_tpl<F> axis;

	// default quaternion constructor
	AngleAxis_tpl( void ) { };
	AngleAxis_tpl( F a, F ax, F ay, F az ) {  angle=a; axis.x=ax; axis.y=ay; axis.z=az; }
	AngleAxis_tpl( F a, Vec3_tpl<F> &n ) { angle=a; axis=n; }
	void operator () ( F a, const Vec3_tpl<F> &n ) {  angle=a; axis=n; }
	AngleAxis_tpl( const AngleAxis_tpl<F>& aa ); //CAngleAxis aa=angleaxis
	const Vec3_tpl<F> operator * ( const Vec3_tpl<F>& v ) const;

	AngleAxis_tpl( const Quat_tpl<F>& q)
	{
		angle = acos_tpl(q.w)*2;
		axis	= q.v;
		axis.Normalize();
		F s = sin_tpl(angle * 0.5);
		if (s == 0)
		{
			angle = 0;
			axis.x = 0;
			axis.y = 0;
			axis.z = 1;
		}
	}

};

typedef AngleAxis_tpl<f32> AngleAxis;
typedef AngleAxis_tpl<f64> AngleAxis_f64;

template<typename F> 
ILINE const Vec3_tpl<F> AngleAxis_tpl<F>::operator * ( const Vec3_tpl<F> &v ) const {
	Vec3_tpl<F> origin 	= axis*(axis|v);
	return 	origin +  (v-origin)*cos_tpl(angle)  +  (axis % v)*sin_tpl(angle);
}

















//////////////////////////////////////////////////////////////////////
template<typename F> struct __passinreg   Plane_tpl
{

	//plane-equation: n.x*x + n.y*y + n.z*z + d > 0 is in front of the plane 
	Vec3_tpl<F>	n;	//!< normal
	F	d;						//!< distance

	//----------------------------------------------------------------	 

#ifdef _DEBUG
	ILINE Plane_tpl() 
	{
		if (sizeof(F)==4)
		{
			uint32* p=alias_cast<uint32*>(&n.x);		p[0]=F32NAN;	p[1]=F32NAN; p[2]=F32NAN; p[3]=F32NAN;
		}
		if (sizeof(F)==8)
		{
			uint64* p=alias_cast<uint64*>(&n.x);		p[0]=F64NAN;	p[1]=F64NAN; p[2]=F64NAN; p[3]=F64NAN;
		}
	}
#else
	ILINE Plane_tpl()	{};
#endif


	ILINE Plane_tpl( const Plane_tpl<F> &p ) {	n=p.n; d=p.d; }
	ILINE Plane_tpl( const Vec3_tpl<F> &normal, const F &distance ) {  n=normal; d=distance; }

	//! set normal and dist for this plane and  then calculate plane type
	ILINE void Set(const Vec3_tpl<F> &vNormal,const F fDist)	{	
		n = vNormal; 
		d = fDist;
	}

	ILINE void SetPlane( const Vec3_tpl<F> &normal, const Vec3_tpl<F> &point ) { 
		n=normal; 
		d=-(point | normal); 
	}
	ILINE static Plane_tpl<F> CreatePlane(  const Vec3_tpl<F> &normal, const Vec3_tpl<F> &point ) {  
		return Plane_tpl<F>( normal,-(point|normal)  );
	}

	ILINE Plane_tpl<F> operator - ( void ) const { return Plane_tpl<F>(-n,-d); }

	/*!
	* Constructs the plane by tree given Vec3s (=triangle) with a right-hand (anti-clockwise) winding
	*
	* Example 1:
	*  Vec3 v0(1,2,3),v1(4,5,6),v2(6,5,6);
	*  Plane_tpl<F>  plane;
	*  plane.SetPlane(v0,v1,v2);
	*
	* Example 2:
	*  Vec3 v0(1,2,3),v1(4,5,6),v2(6,5,6);
	*  Plane_tpl<F>  plane=Plane_tpl<F>::CreatePlane(v0,v1,v2);
	*/
	ILINE void SetPlane( const Vec3_tpl<F> &v0, const Vec3_tpl<F> &v1, const Vec3_tpl<F> &v2 ) {  
		n = ((v1-v0)%(v2-v0)).GetNormalized();	//vector cross-product
		d	=	-(n | v0);				//calculate d-value
	}
	ILINE static Plane_tpl<F> CreatePlane( const Vec3_tpl<F> &v0, const Vec3_tpl<F> &v1, const Vec3_tpl<F> &v2 ) {  
		Plane_tpl<F> p;	p.SetPlane(v0,v1,v2);	return p;
	}

	/*!
	* Computes signed distance from point to plane.
	* This is the standart plane-equation: d=Ax*By*Cz+D.
	* The normal-vector is assumed to be normalized.
	* 
	* Example:
	*  Vec3 v(1,2,3);
	*  Plane_tpl<F>  plane=CalculatePlane(v0,v1,v2);
	*  f32 distance = plane|v;
	*/
	ILINE F operator | ( const Vec3_tpl<F> &point ) const { return (n | point) + d; }
	ILINE F	DistFromPlane(const Vec3_tpl<F> &vPoint) const	{	return (n*vPoint+d); }

	ILINE Plane_tpl<F> operator - ( const Plane_tpl<F> &p) const { return Plane_tpl<F>( n-p.n, d-p.d); }
	ILINE Plane_tpl<F> operator + ( const Plane_tpl<F> &p) const { return Plane_tpl<F>(n+p.n,d+p.d); }
	ILINE void operator -= (const Plane_tpl<F> &p) { d-=p.d; n-=p.n; }
	ILINE Plane_tpl<F> operator * ( F s ) const {	return Plane_tpl<F>(n*s,d*s);	}
	ILINE Plane_tpl<F> operator / ( F s ) const {	return Plane_tpl<F>(n/s,d/s); }

	//! check for equality between two planes
	ILINE  friend	bool operator ==(const Plane_tpl<F> &p1, const Plane_tpl<F> &p2) {
		if (fabsf(p1.n.x-p2.n.x)>0.0001f) return (false);
		if (fabsf(p1.n.y-p2.n.y)>0.0001f) return (false);
		if (fabsf(p1.n.z-p2.n.z)>0.0001f) return (false);
		if (fabsf(p1.d-p2.d)<0.01f) return(true);
		return (false);
	}

	Vec3_tpl<F> MirrorVector(const Vec3_tpl<F>& i)   {	return n*(2* (n|i))-i;	}
	Vec3_tpl<F> MirrorPosition(const Vec3_tpl<F>& i) {  return n*(2* ((n|i)-d))-i;	}

	AUTO_STRUCT_INFO
};

typedef Plane_tpl<f32>	Plane;
typedef Plane_tpl<real>	Planer;
typedef Plane_tpl<f64>	Plane_f64;

//-----------------------------------------------------------------
// define the constants

template <typename T> const Vec3_tpl<T> Vec3Constants<T>::fVec3_Zero(0, 0, 0);
template <typename T> const Vec3_tpl<T> Vec3Constants<T>::fVec3_OneX(1, 0, 0);
template <typename T> const Vec3_tpl<T> Vec3Constants<T>::fVec3_OneY(0, 1, 0);
template <typename T> const Vec3_tpl<T> Vec3Constants<T>::fVec3_OneZ(0, 0, 1);
template <typename T> const Vec3_tpl<T> Vec3Constants<T>::fVec3_One(1, 1, 1);

#endif //vector
