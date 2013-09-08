//////////////////////////////////////////////////////////////////////
//
//	Crytek Common Source code
//
//	File:Cry_Math.h
//	Description: Misc mathematical functions
//
//	History:
//	-Jan 31,2001: Created by Marco Corbetta
//	-Jan 04,2003: SSE and 3DNow optimizations by Andrey Honich
//
//////////////////////////////////////////////////////////////////////

#ifndef CRY_XENON_OPTIMIZE
#define CRY_XENON_OPTIMIZE

#pragma once


inline void multMatrixf_VMX128(float *product, const float *m1, const float *m2)
{
	XMVECTOR X, Y, Z, W, TV;
	XMMATRIX *M1, *M2;

	M1 = (XMMATRIX *)m1;
	M2 = (XMMATRIX *)m2;

	X = XMVectorSplatX(M1->r[0]);
	Y = XMVectorSplatY(M1->r[0]);
	Z = XMVectorSplatZ(M1->r[0]);
	W = XMVectorSplatW(M1->r[0]);

	TV = XMVectorMultiply(X, M2->r[0]);
	TV = XMVectorMultiplyAdd(Y, M2->r[1], TV);
	TV = XMVectorMultiplyAdd(Z, M2->r[2], TV);
	XMStoreVector4A(&product[0], XMVectorMultiplyAdd(W, M2->r[3], TV));

	X = XMVectorSplatX(M1->r[1]);
	Y = XMVectorSplatY(M1->r[1]);
	Z = XMVectorSplatZ(M1->r[1]);
	W = XMVectorSplatW(M1->r[1]);

	TV = XMVectorMultiply(X, M2->r[0]);
	TV = XMVectorMultiplyAdd(Y, M2->r[1], TV);
	TV = XMVectorMultiplyAdd(Z, M2->r[2], TV);
	XMStoreVector4A(&product[4], XMVectorMultiplyAdd(W, M2->r[3], TV));

	X = XMVectorSplatX(M1->r[2]);
	Y = XMVectorSplatY(M1->r[2]);
	Z = XMVectorSplatZ(M1->r[2]);
	W = XMVectorSplatW(M1->r[2]);

	TV = XMVectorMultiply(X, M2->r[0]);
	TV = XMVectorMultiplyAdd(Y, M2->r[1], TV);
	TV = XMVectorMultiplyAdd(Z, M2->r[2], TV);
	XMStoreVector4A(&product[8], XMVectorMultiplyAdd(W, M2->r[3], TV));

	X = XMVectorSplatX(M1->r[3]);
	Y = XMVectorSplatY(M1->r[3]);
	Z = XMVectorSplatZ(M1->r[3]);
	W = XMVectorSplatW(M1->r[3]);

	TV = XMVectorMultiply(X, M2->r[0]);
	TV = XMVectorMultiplyAdd(Y, M2->r[1], TV);
	TV = XMVectorMultiplyAdd(Z, M2->r[2], TV);
	XMStoreVector4A(&product[12], XMVectorMultiplyAdd(W, M2->r[3], TV));
}
inline void transposeMatrixf_VMX128(float *product, const float *m)
{
	XMMATRIX *M;
	M = (XMMATRIX *)m;

	XMMATRIX P;

	P.r[0] = XMVectorMergeXY(M->r[0], M->r[2]); // m00m20m01m21
	P.r[1] = XMVectorMergeXY(M->r[1], M->r[3]); // m10m30m11m31
	P.r[2] = XMVectorMergeZW(M->r[0], M->r[2]); // m02m22m03m23
	P.r[3] = XMVectorMergeZW(M->r[1], M->r[3]); // m12m32m13m33

	XMStoreVector4A(&product[0], XMVectorMergeXY(P.r[0], P.r[1])); // m00m10m20m30
	XMStoreVector4A(&product[4], XMVectorMergeZW(P.r[0], P.r[1])); // m01m11m21m31
	XMStoreVector4A(&product[8], XMVectorMergeXY(P.r[2], P.r[3])); // m02m12m22m32
	XMStoreVector4A(&product[12], XMVectorMergeZW(P.r[2], P.r[3])); // m03m13m23m33
}
inline void invertMatrixf_VMX128(float *product, const float *m)
{
	XMVECTOR Determinant;
	*(XMMATRIX *)product = XMMatrixInverse(&Determinant, *(XMMATRIX *)m);
}































































#endif
