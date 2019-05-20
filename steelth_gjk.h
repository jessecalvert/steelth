/*@H
* File: steelth_gjk.h
* Author: Jesse Calvert
* Created: February 9, 2018, 12:12
* Last modified: February 9, 2018, 17:29
*/

#pragma once

struct minkowski_difference_support
{
	v3 SupportA;
	v3 SupportB;
	v3 Support;
};

inline minkowski_difference_support
operator*(f32 Scale, minkowski_difference_support A)
{
	minkowski_difference_support Result =
	{
		Scale*A.SupportA,
		Scale*A.SupportB,
		Scale*A.Support
	};
	return Result;
}

inline minkowski_difference_support
operator+(minkowski_difference_support A, minkowski_difference_support B)
{
	minkowski_difference_support Result =
	{
		A.SupportA + B.SupportA,
		A.SupportB + B.SupportB,
		A.Support + B.Support
	};
	return Result;
}

inline minkowski_difference_support
Lerp(minkowski_difference_support A, f32 t, minkowski_difference_support B)
{
	minkowski_difference_support Result = {};
	Result.SupportA = Lerp(A.SupportA, t, B.SupportA);
	Result.SupportB = Lerp(A.SupportB, t, B.SupportB);
	Result.Support = Lerp(A.Support, t, B.Support);
	return Result;
}

struct minkowski_simplex
{
	u32 Size;
	minkowski_difference_support Points[4];
};

struct gjk_result
{
	minkowski_simplex Simplex;
	minkowski_difference_support ClosestPoint;
	b32 ContainsOrigin;
};
