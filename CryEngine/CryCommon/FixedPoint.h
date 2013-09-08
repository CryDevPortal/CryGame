#ifndef __FIXED_POINT_H
#define __FIXED_POINT_H

#pragma once


#include <BitFiddling.h>

#ifdef fabsf
#undef fabsf
#endif
#ifdef fmodf
#undef fmodf
#endif
#ifdef sqrtf
#undef sqrtf
#endif
#ifdef expf
#undef expf
#endif
#ifdef logf
#undef logf
#endif
#ifdef powf
#undef powf
#endif
#ifdef fmodf
#undef fmodf
#endif


/*
	This file implements a configurable fixed point number type.
	If base type is unsigned, fixed point number will be unsigned too and any signed base operations will be not work properly.
	Only 8..32bit basic types are supported.

	TODO: -When overflow_type is 64bit, shift operations are implemented in software through in the CRT through
		_allshl, _allshr, _alldiv, _allmul - extra call is generated and no optimizations applied.
	-Implement basic trigonometry routines: (arc)sin, (arc)cos, (arc)tan
*/

/*

	Here's a Visual Studio Debugger Visualizer - to be put under the [Visualizer] section in your autoexp.dat

	;------------------------------------------------------------------------------
	; fixed_t
	;------------------------------------------------------------------------------
	fixed_t<*,*> {
		preview (
			#([$e.v / (float)fixed_t<$T1,$T2>::fractional_bitcount, f])
		)
	}

*/
namespace FixedPoint
{
	template<typename Ty>
	struct overflow_type
	{
	};

	template<>
	struct overflow_type<char>
	{
		typedef short type;
	};

	template<>
	struct overflow_type<unsigned char>
	{
		typedef unsigned short type;
	};

	template<>
	struct overflow_type<short>
	{
		typedef int type;
	};

	template<>
	struct overflow_type<unsigned short>
	{
		typedef unsigned int type;
	};

	template<>
	struct overflow_type<int>
	{
		typedef int64 type;
	};

	template<>
	struct overflow_type<unsigned int>
	{
		typedef uint64 type;
	};
}

template<bool IsTrue>
struct Selector{};

template<>
struct Selector<true>{};
template<>
struct Selector<false>{};

template<typename BaseType, size_t IntegerBitCount>
struct fixed_t
{	

	typedef BaseType value_type;

	enum { value_size = sizeof(value_type), };
	enum { bit_size = sizeof(value_type) * 8, };
	enum { is_signed = std::numeric_limits<value_type>::is_signed ? 1 : 0, };
	enum { integer_bitcount = IntegerBitCount, };
	enum { fractional_bitcount = bit_size - integer_bitcount - is_signed, };
	enum { integer_scale = 1 << fractional_bitcount, };


	inline fixed_t()
	{
	}

	inline fixed_t(const fixed_t& other)
		: v(other.v)
	{
	}
	
	template<typename BaseTypeO, size_t IntegerBitCountO>
	void ConstructorHelper( const fixed_t<BaseTypeO, IntegerBitCountO>& other, const Selector<true>&)
	{
		v = other.get();
		v <<= static_cast<unsigned>(fractional_bitcount) - static_cast<unsigned>(fixed_t<BaseTypeO, IntegerBitCountO>::fractional_bitcount);
	}

	template<typename BaseTypeO, size_t IntegerBitCountO>
	void ConstructorHelper( const fixed_t<BaseTypeO, IntegerBitCountO>& other, const Selector<false>&)
	{
		BaseTypeO r = other.get();
		r >>= static_cast<unsigned>(fixed_t<BaseTypeO, IntegerBitCountO>::fractional_bitcount - fractional_bitcount);
		v = r;
	}


	template<typename BaseTypeO, size_t IntegerBitCountO>
	inline explicit fixed_t(const fixed_t<BaseTypeO, IntegerBitCountO>& other)
	{ 
		ConstructorHelper(other, Selector<static_cast<unsigned>(fractional_bitcount) >= static_cast<unsigned>(fixed_t<BaseTypeO, IntegerBitCountO>::fractional_bitcount)>() );		
	}

	inline fixed_t(const int& value)
		: v(value << fractional_bitcount)
	{
	}

	inline fixed_t(const unsigned int& value)
		: v(value << fractional_bitcount)
	{
	}

	inline explicit fixed_t(const float& value)
		: v((value_type)(value * integer_scale + (value >= 0.0f ? 0.5f : -0.5f)))
	{
	}

	inline explicit fixed_t(const double& value)
		: v((value_type)(value * integer_scale + (value >= 0.0 ? 0.5 : -0.5)))
	{
	}

	inline explicit fixed_t(const bool& value)
		: v((value_type)(value * integer_scale))
	{
	}

	inline fixed_t operator-() const
	{
		fixed_t r;
		r.v = -v;

		return r;
	}

	inline fixed_t& operator+=(const fixed_t& other)
	{
		v += other.v;
		return *this;
	}

	inline fixed_t& operator-=(const fixed_t& other)
	{
		v -= other.v;
		return *this;
	}

	inline fixed_t& operator*=(const fixed_t& other)
	{
		v = (typename FixedPoint::overflow_type<value_type>::type(v) * other.v) >> fractional_bitcount;
		return *this;
	}

	inline fixed_t& operator/=(const fixed_t& other)
	{
		v = (typename FixedPoint::overflow_type<value_type>::type(v) << fractional_bitcount) / other.v;
		return *this;
	}

	inline fixed_t operator+(const fixed_t& other) const
	{
		fixed_t r(*this);
		r += other;
		return r;
	}

	inline fixed_t operator-(const fixed_t& other) const
	{
		fixed_t r(*this);
		r -= other;
		return r;
	}

	inline fixed_t operator*(const fixed_t& other) const
	{
		fixed_t r(*this);
		r *= other;
		return r;
	}

	inline fixed_t operator/(const fixed_t& other) const
	{
		fixed_t r(*this);
		r /= other;
		return r;
	}

	inline bool operator<(fixed_t other) const
	{
		return v < other.v;
	}

	inline bool operator<=(fixed_t other) const
	{
		return v <= other.v;
	}

	inline bool operator>(fixed_t other) const
	{
		return v > other.v;
	}

	inline bool operator>=(fixed_t other) const
	{
		return v >= other.v;
	}

	inline bool operator==(fixed_t other) const
	{
		return v == other.v;
	}

	inline bool operator!=(fixed_t other) const
	{
		return v != other.v;
	}

	inline fixed_t& operator++()
	{
		v += integer_scale;
		return *this;
	}

	inline fixed_t operator++(int)
	{
		fixed_t tmp(*this);
		operator++();
		return tmp;
	}

	inline fixed_t& operator--()
	{
		v -= integer_scale;
		return *this;
	}

	inline fixed_t operator--(int)
	{
		fixed_t tmp(*this);
		operator--();
		return tmp;
	}

	inline fixed_t& operator=(const fixed_t& other)
	{
		v = other.v;
		return *this;
	}

	inline fixed_t& operator=(const float& other)
	{
		fixed_t(other).swap(*this);
		return *this;
	}

	inline fixed_t& operator=(const double& other)
	{
		fixed_t(other).swap(*this);
		return *this;
	}

	template<typename Ty>
	inline fixed_t& operator=(const Ty& other)
	{
		fixed_t(other).swap(*this);
		return *this;
	}


	inline float as_float() const
	{
		return v / (float)integer_scale;
	}

	inline double as_double() const
	{
		return v / (double)integer_scale;
	}

	inline int as_int() const
	{
		return v >> fractional_bitcount;
	}

	inline unsigned int as_uint() const
	{
		return v >> fractional_bitcount;
	}

	inline value_type get() const
	{
		return v;
	}

	inline void set(value_type value)
	{
		v = value;
	}

	inline void swap(fixed_t& other)
	{
		std::swap(v, other.v);
	}

	inline static fixed_t max()
	{
		fixed_t v;
		v.set(std::numeric_limits<value_type>::max());
		return v;
	}

	inline static fixed_t min()
	{
		fixed_t v;
		v.set(std::numeric_limits<value_type>::min());
		return v;
	}

	inline static fixed_t fraction(int num, int denom)
	{
		fixed_t v;
		v.set((typename FixedPoint::overflow_type<value_type>::type(num) << (fractional_bitcount + fractional_bitcount))
			/ (typename FixedPoint::overflow_type<value_type>::type(denom) << fractional_bitcount));
		return v;
	}

	AUTO_STRUCT_INFO

protected:
	value_type v;
};


template<typename BaseType, size_t IntegerBitCount>
inline fixed_t<BaseType, IntegerBitCount> floor(const fixed_t<BaseType, IntegerBitCount>& x)
{
	fixed_t<BaseType, IntegerBitCount> r;
	r.set(x.get() & ~(fixed_t<BaseType, IntegerBitCount>::fractional_bitcount - 1));

	return r;
}

template<typename BaseType, size_t IntegerBitCount>
inline fixed_t<BaseType, IntegerBitCount> ceil(const fixed_t<BaseType, IntegerBitCount>& x)
{
	fixed_t<BaseType, IntegerBitCount> r;
	r.set(x.get() & ~(fixed_t<BaseType, IntegerBitCount>::fractional_bitcount - 1));
	if (x.get() & (fixed_t<BaseType, IntegerBitCount>::fractional_bitcount - 1))
		r += fixed_t<BaseType, IntegerBitCount>(1);

	return r;
}


// disable fabsf for unsigned base types to avoid sign warning
template<typename BaseType, size_t IntegerBitCount>
inline fixed_t<BaseType, IntegerBitCount> fabsf(const fixed_t<BaseType, IntegerBitCount>& x)
{
	typedef fixed_t<BaseType, IntegerBitCount> type;

	const BaseType mask = x.get() >> (type::bit_size - 1);

	type r;
	r.set((x.get() + mask) ^ mask);

	return r;
}

template<typename BaseType, size_t IntegerBitCount>
inline fixed_t<BaseType, IntegerBitCount> fmodf(const fixed_t<BaseType, IntegerBitCount>& x,
																								const fixed_t<BaseType, IntegerBitCount>& y)
{
	fixed_t<BaseType, IntegerBitCount> r;
	r.set(x.get() % y.get());
	return r;
}

template<typename BaseType, size_t IntegerBitCount>
inline fixed_t<BaseType, IntegerBitCount> sqrtf(const fixed_t<BaseType, IntegerBitCount>& x)
{
	typedef fixed_t<BaseType, IntegerBitCount> type;
	typedef typename FixedPoint::overflow_type<BaseType>::type overflow_type;

	overflow_type root = 0;
	overflow_type remHi = 0;
	overflow_type remLo = overflow_type(x.get()) << (type::fractional_bitcount & 1);
	overflow_type count = (type::bit_size >> 1) + (type::fractional_bitcount >> 1) - ((type::fractional_bitcount + 1) & 1);

	do {
		remHi = (remHi << 2) | (remLo >> (type::bit_size - 2));
		remLo <<= 2;
		remLo &= (overflow_type(1) << type::bit_size) - 1;
		root <<= 1;
		root &= (overflow_type(1) << type::bit_size) - 1;
		overflow_type div = (root << 1) + 1;
		if (remHi >= div)
		{
			remHi -= div;
			remHi &= (overflow_type(1) << type::bit_size) - 1;
			root += 1;
		}
	} while (count--);

	type r;
	r.set(static_cast<typename type::value_type>(root >> (type::fractional_bitcount & 1)));

	return r;
}

template<typename BaseType, size_t IntegerBitCount>
inline fixed_t<BaseType, IntegerBitCount> isqrtf(const fixed_t<BaseType, IntegerBitCount>& x)
{
	if (x > fixed_t<BaseType, IntegerBitCount>(0))
	{
		fixed_t<BaseType, IntegerBitCount> r(1);
		r /= sqrtf(x);

		return r;
	}

	return fixed_t<BaseType, IntegerBitCount>(0);
}


//http://en.wikipedia.org/wiki/Taylor_series
template<typename BaseType, size_t IntegerBitCount>
inline fixed_t<BaseType, IntegerBitCount> expf(const fixed_t<BaseType, IntegerBitCount>& x)
{
	const fixed_t<BaseType, IntegerBitCount> one(1);
	fixed_t<BaseType, IntegerBitCount> w(one);
	fixed_t<BaseType, IntegerBitCount> y(one);
	fixed_t<BaseType, IntegerBitCount> n(one);

	for( ; w != 0; ++n)
		y += (w *= x / n);

	return y;
}

template<typename BaseType, size_t IntegerBitCount>
inline fixed_t<BaseType, IntegerBitCount> logf(const fixed_t<BaseType, IntegerBitCount>& x)
{
	fixed_t<BaseType, IntegerBitCount> b((x - 1) / (x + 1));
	fixed_t<BaseType, IntegerBitCount> w;
	fixed_t<BaseType, IntegerBitCount> y(b);
	fixed_t<BaseType, IntegerBitCount> z(b);
	fixed_t<BaseType, IntegerBitCount> n(3);

	b *= b;

	for ( ; w != 0; n += 2)
	{
		z *= b;
		w = z / n;
		y += w;
	}

	return y + y;
}

template<typename BaseType, size_t IntegerBitCount>
inline fixed_t<BaseType, IntegerBitCount> powf(const fixed_t<BaseType, IntegerBitCount>& x,
																							 const fixed_t<BaseType, IntegerBitCount>& y)
{
	const fixed_t<BaseType, IntegerBitCount> zero(0);

	if (x < zero)
	{
		if (fmodf(y, fixed_t<BaseType, IntegerBitCount>(2)) != zero)
			return -expf((y * logf(-x)));
		else
			return expf((y * logf(-x)));
	}
	else
		return expf(y * logf(x));
}

#endif	// #ifndef __FIXED_POINT_H
