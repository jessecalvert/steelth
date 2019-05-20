/*@H
* File: steelth_simd.h
* Author: Jesse Calvert
* Created: June 15, 2018, 15:06
* Last modified: April 10, 2019, 17:31
*/

#pragma once

union f32_4x
{
	__m128 P;
	f32 E[4];
	u32 U32[4];
};

union v3_4x
{
	f32_4x E[3];

	struct
	{
		union
		{
			f32_4x x;
			f32_4x r;
		};
		union
		{
			f32_4x y;
			f32_4x g;
		};
		union
		{
			f32_4x z;
			f32_4x b;
		};
	};
};

union v4_4x
{
	f32_4x E[4];

	struct
	{
		union
		{
			f32_4x x;
			f32_4x r;
		};
		union
		{
			f32_4x y;
			f32_4x g;
		};
		union
		{
			f32_4x z;
			f32_4x b;
		};
		union
		{
			f32_4x w;
			f32_4x a;
		};
	};
};

//
// NOTE: f32_4x
//

#define M(a, i) ((f32 *)&(a))[i]
#define Mi(a, i) ((u32 *)&(a))[i]
#define MMSetWithExpr(Expr) F32_4x(Expr, Expr, Expr, Expr)

inline f32_4x
F32_4x(f32 EAll)
{
	f32_4x Result;
	Result.P = _mm_set1_ps(EAll);
	return Result;
}

inline f32_4x
F32_4x(f32 E0, f32 E1, f32 E2, f32 E3)
{
	f32_4x Result;
	Result.P = _mm_setr_ps(E0, E1, E2, E3);
	return Result;
}

inline f32_4x
U32_4x(u32 EAll)
{
	f32_4x Result;
	Result.P = _mm_set1_ps(*(f32 *)&EAll);
	return Result;
}

inline f32_4x
ZeroF32_4x()
{
	f32_4x Result;
	Result.P = _mm_setzero_ps();
	return Result;
}

inline f32_4x
operator+(f32_4x A, f32_4x B)
{
	f32_4x Result;
	Result.P = _mm_add_ps(A.P, B.P);
	return Result;
}

inline f32_4x
operator-(f32_4x A, f32_4x B)
{
	f32_4x Result;
	Result.P = _mm_sub_ps(A.P, B.P);
	return Result;
}

inline f32_4x
operator-(f32_4x A)
{
	f32_4x Result = ZeroF32_4x() - A;
	return Result;
}

inline f32_4x
operator*(f32_4x A, f32_4x B)
{
	f32_4x Result;
	Result.P = _mm_mul_ps(A.P, B.P);
	return Result;
}

inline f32_4x
operator/(f32_4x A, f32_4x B)
{
	f32_4x Result;
	Result.P = _mm_div_ps(A.P, B.P);
	return Result;
}

inline f32_4x &
operator+=(f32_4x &A, f32_4x B)
{
	A = A + B;
	return A;
}

inline f32_4x &
operator-=(f32_4x &A, f32_4x B)
{
	A = A - B;
	return A;
}

inline f32_4x &
operator*=(f32_4x &A, f32_4x B)
{
	A = A * B;
	return A;
}

inline f32_4x &
operator/=(f32_4x &A, f32_4x B)
{
	A = A / B;
	return A;
}

inline f32_4x
operator<(f32_4x A, f32_4x B)
{
	f32_4x Result;
	Result.P = _mm_cmplt_ps(A.P, B.P);
	return Result;
}

inline f32_4x
operator>(f32_4x A, f32_4x B)
{
	f32_4x Result;
	Result.P = _mm_cmpgt_ps(A.P, B.P);
	return Result;
}

inline b32
AnyTrue(f32_4x Mask)
{
	b32 Result = _mm_movemask_ps(Mask.P);
	return Result;
}

inline b32
AllTrue(f32_4x Mask)
{
	b32 Result = (_mm_movemask_ps(Mask.P) == 0x1111);
	return Result;
}

inline b32
AllFalse(f32_4x Mask)
{
	b32 Result = (_mm_movemask_ps(Mask.P) == 0);
	return Result;
}

inline f32_4x
Select(f32_4x A, f32_4x Mask, f32_4x B)
{
	f32_4x Result;
	Result.P = _mm_or_ps(_mm_andnot_ps(Mask.P, A.P), _mm_and_ps(Mask.P, B.P));
	return Result;
}

//
// NOTE: v3_4x
//

inline v3_4x
V3_4x(v3 A)
{
	v3_4x Result;
	Result.x = F32_4x(A.x);
	Result.y = F32_4x(A.y);
	Result.z = F32_4x(A.z);
	return Result;
}

inline v3_4x
V3_4x(v3 E0, v3 E1, v3 E2, v3 E3)
{
	v3_4x Result;
	Result.x = F32_4x(E0.x, E1.x, E2.x, E3.x);
	Result.y = F32_4x(E0.y, E1.y, E2.y, E3.y);
	Result.z = F32_4x(E0.z, E1.z, E2.z, E3.z);
	return Result;
}

inline v3_4x
ZeroV3_4x()
{
	v3_4x Result;
	Result.x = Result.y = Result.z = ZeroF32_4x();
	return Result;
}

inline v3_4x
operator*(f32 Scalar, v3_4x A)
{
	v3_4x Result;
	f32_4x Scalar_4x = F32_4x(Scalar);
	Result.x = Scalar_4x * A.x;
	Result.y = Scalar_4x * A.y;
	Result.z = Scalar_4x * A.z;
	return Result;
}

inline v3_4x
operator*(f32_4x Scalar_4x, v3_4x A)
{
	v3_4x Result;
	Result.x = Scalar_4x * A.x;
	Result.y = Scalar_4x * A.y;
	Result.z = Scalar_4x * A.z;
	return Result;
}

inline v3_4x
operator+(v3_4x A, v3_4x B)
{
	v3_4x Result;
	Result.x = A.x + B.x;
	Result.y = A.y + B.y;
	Result.z = A.z + B.z;
	return Result;
}

inline v3_4x
operator-(v3_4x A, v3_4x B)
{
	v3_4x Result;
	Result.x = A.x - B.x;
	Result.y = A.y - B.y;
	Result.z = A.z - B.z;
	return Result;
}

inline v3_4x
operator-(v3_4x A)
{
	v3_4x Result = ZeroV3_4x() - A;
	return Result;
}

inline v3_4x
operator*(v3_4x A, v3_4x B)
{
	v3_4x Result;
	Result.x = A.x * B.x;
	Result.y = A.y * B.y;
	Result.z = A.z * B.z;
	return Result;
}

inline v3_4x
operator/(v3_4x A, v3_4x B)
{
	v3_4x Result;
	Result.x = A.x / B.x;
	Result.y = A.y / B.y;
	Result.z = A.z / B.z;
	return Result;
}

inline v3_4x &
operator+=(v3_4x &A, v3_4x B)
{
	A = A + B;
	return A;
}

inline v3_4x &
operator-=(v3_4x &A, v3_4x B)
{
	A = A - B;
	return A;
}

inline v3_4x &
operator*=(v3_4x &A, v3_4x B)
{
	A = A * B;
	return A;
}

inline v3_4x &
operator/=(v3_4x &A, v3_4x B)
{
	A = A / B;
	return A;
}

inline v3
GetComponent(v3_4x A, u32 Index)
{
	v3 Result =
	{
		A.x.E[Index],
		A.y.E[Index],
		A.z.E[Index],
	};
	return Result;
}

//
// NOTE: v4_4x
//

inline v4_4x
V4_4x(v4 A)
{
	v4_4x Result;
	Result.x = F32_4x(A.x);
	Result.y = F32_4x(A.y);
	Result.z = F32_4x(A.z);
	Result.w = F32_4x(A.w);
	return Result;
}

inline v4_4x
V4_4x(v4 E0, v4 E1, v4 E2, v4 E3)
{
	v4_4x Result;
	Result.x = F32_4x(E0.x, E1.x, E2.x, E3.x);
	Result.y = F32_4x(E0.y, E1.y, E2.y, E3.y);
	Result.z = F32_4x(E0.z, E1.z, E2.z, E3.z);
	Result.w = F32_4x(E0.w, E1.w, E2.w, E3.w);
	return Result;
}

inline v4_4x
ZeroV4_4x()
{
	v4_4x Result;
	Result.x = Result.y = Result.z = Result.w = ZeroF32_4x();
	return Result;
}

inline v4_4x
operator*(f32 Scalar, v4_4x A)
{
	v4_4x Result;
	f32_4x Scalar_4x = F32_4x(Scalar);
	Result.x = Scalar_4x * A.x;
	Result.y = Scalar_4x * A.y;
	Result.z = Scalar_4x * A.z;
	Result.w = Scalar_4x * A.w;
	return Result;
}

inline v4_4x
operator*(f32_4x Scalar_4x, v4_4x A)
{
	v4_4x Result;
	Result.x = Scalar_4x * A.x;
	Result.y = Scalar_4x * A.y;
	Result.z = Scalar_4x * A.z;
	Result.w = Scalar_4x * A.w;
	return Result;
}

inline v4_4x
operator+(v4_4x A, v4_4x B)
{
	v4_4x Result;
	Result.x = A.x + B.x;
	Result.y = A.y + B.y;
	Result.z = A.z + B.z;
	Result.w = A.w + B.w;
	return Result;
}

inline v4_4x
operator-(v4_4x A, v4_4x B)
{
	v4_4x Result;
	Result.x = A.x - B.x;
	Result.y = A.y - B.y;
	Result.z = A.z - B.z;
	Result.w = A.w - B.w;
	return Result;
}

inline v4_4x
operator-(v4_4x A)
{
	v4_4x Result = ZeroV4_4x() - A;
	return Result;
}

inline v4_4x
operator*(v4_4x A, v4_4x B)
{
	v4_4x Result;
	Result.x = A.x * B.x;
	Result.y = A.y * B.y;
	Result.z = A.z * B.z;
	Result.w = A.w * B.w;
	return Result;
}

inline v4_4x
operator/(v4_4x A, v4_4x B)
{
	v4_4x Result;
	Result.x = A.x / B.x;
	Result.y = A.y / B.y;
	Result.z = A.z / B.z;
	Result.w = A.w / B.w;
	return Result;
}

inline v4_4x &
operator+=(v4_4x &A, v4_4x B)
{
	A = A + B;
	return A;
}

inline v4_4x &
operator-=(v4_4x &A, v4_4x B)
{
	A = A - B;
	return A;
}

inline v4_4x &
operator*=(v4_4x &A, v4_4x B)
{
	A = A * B;
	return A;
}

inline v4_4x &
operator/=(v4_4x &A, v4_4x B)
{
	A = A / B;
	return A;
}

inline v4
GetComponent(v4_4x A, u32 Index)
{
	v4 Result =
	{
		A.x.E[Index],
		A.y.E[Index],
		A.z.E[Index],
		A.w.E[Index],
	};
	return Result;
}
