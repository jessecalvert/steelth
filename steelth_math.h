/*@H
* File: steelth_math.h
* Author: Jesse Calvert
* Created: October 9, 2017, 16:28
* Last modified: April 24, 2019, 13:56
*/

#pragma once
#include "steelth_intrinsics.h"

//
// NOTE: Scalar operations
//

inline f32
Clamp(f32 Value, f32 Min, f32 Max)
{
	f32 Result = Value;
	if(Value < Min)
	{
		Result = Min;
	}
	if(Value > Max)
	{
		Result = Max;
	}
	return Result;
}

inline s32
Clamp(s32 Value, s32 Min, s32 Max)
{
	s32 Result = Value;
	if(Value < Min)
	{
		Result = Min;
	}
	if(Value > Max)
	{
		Result = Max;
	}
	return Result;
}

inline f32
Clamp01(f32 Value)
{
	f32 Result = Clamp(Value, 0.0f, 1.0f);
	return Result;
}

inline f32
Lerp(f32 A, f32 t, f32 B)
{
	f32 Result = A + t*(B - A);
	return Result;
}

//
// NOTE: v2 operations
//

inline v2
V2(f32 x, f32 y)
{
	v2 Result = {x, y};
	return Result;
}

inline v2
operator-(v2 A)
{
	v2 Result = V2(-A.x, -A.y);
	return Result;
}

inline v2
operator-(v2 A, v2 B)
{
	v2 Result = V2(A.x - B.x, A.y - B.y);
	return Result;
}

inline v2
operator+(v2 A, v2 B)
{
	v2 Result = V2(A.x + B.x, A.y + B.y);
	return Result;
}

inline v2 &
operator+=(v2 &A, v2 B)
{
	A = A + B;
	return A;
}

inline v2 &
operator-=(v2 &A, v2 B)
{
	A = A - B;
	return A;
}

inline v2
operator*(f32 Scale, v2 Vector)
{
	v2 Result = V2(Scale*Vector.x, Scale*Vector.y);
	return Result;
}

inline v2
operator*(v2 Vector, f32 Scale)
{
	v2 Result = Scale*Vector;
	return Result;
}

inline b32
operator==(v2 A, v2 B)
{
	b32 Result = ((A.x == B.x) &&
	              (A.y == B.y));
	return Result;
}

inline v2
Hadamard(v2 A, v2 B)
{
	v2 Result = {A.x*B.x, A.y*B.y};
	return Result;
}

inline f32
Inner(v2 A, v2 B)
{
	f32 Result = A.x*B.x + A.y*B.y;
	return Result;
}

inline v2
Perp(v2 A)
{
	v2 Result = {-A.y, A.x};
	return Result;
}

inline v2
Lerp(v2 A, f32 t, v2 B)
{
	v2 Result = {A.x + t*(B.x - A.x), A.y + t*(B.y - A.y)};
	return Result;
}

inline f32
LengthSq(v2 A)
{
	f32 Result = Inner(A, A);
	return Result;
}

inline f32
Length(v2 A)
{
	f32 Result = SquareRoot(LengthSq(A));
	return Result;
}

inline v2
Normalize(v2 A)
{
	f32 LengthOfA = Length(A);
	v2 Result = (1.0f/LengthOfA)*A;
	return Result;
}

inline b32
SameDirection(v2 A, v2 B)
{
	b32 Result = (Inner(A, B) > 0.0f);
	return Result;
}

inline f32
DistanceToLine(v2 TestPoint, v2 LineUnit)
{
	f32 Result = Length(TestPoint - Inner(TestPoint, LineUnit)*LineUnit);
	return Result;
}

inline f32
DistanceToLineSq(v2 TestPoint, v2 LineStart, v2 LineEnd)
{
	v2 Line = LineEnd - LineStart;
	f32 Result = (Square(Inner(TestPoint, Perp(Line))))/(LengthSq(Line));
	return Result;
}

inline f32
DistanceToLine(v2 TestPoint, v2 LineStart, v2 LineEnd)
{
	f32 Result = SquareRoot(DistanceToLineSq(TestPoint, LineStart, LineEnd));
	return Result;
}

inline f32
SegmentClosestPointBarycentric(v2 TestPoint, v2 Head, v2 Tail)
{
	v2 B = Tail;
	v2 A = Head;
	v2 AB = B - A;
	v2 BP = TestPoint - B;
	v2 AP = TestPoint - A;

	f32 Result = 0.0f;
	if(SameDirection(AB, AP))
	{
		if(SameDirection(BP, AB))
		{
			// NOTE: [B]
			Result = 1.0f;
		}
		else
		{
			// NOTE: [A, B]
			Result = Inner(TestPoint - A, AB) / LengthSq(AB);
		}
	}
	else
	{
		// NOTE: [A]
		Result = 0.0f;
	}
	return Result;
}

inline v2
SegmentClosestPoint(v2 TestPoint, v2 Head, v2 Tail)
{
	f32 t = SegmentClosestPointBarycentric(TestPoint, Head, Tail);
	v2 Result = Lerp(Head, t, Tail);
	return Result;
}

//
// NOTE: v2i operations
//

inline v2i
V2i(s32 x, s32 y)
{
	v2i Result = {x, y};
	return Result;
}

inline v2i
V2ToV2i(v2 A)
{
	v2i Result = {(s32)A.x, (s32)A.y};
	return Result;
}

inline v2
V2iToV2(v2i A)
{
	v2 Result = {(f32)A.x, (f32)A.y};
	return Result;
}

inline v2
operator*(f32 Scale, v2i A)
{
	v2 Result = {Scale*A.x, Scale*A.y};
	return Result;
}

inline v2i
operator*(s32 Scale, v2i A)
{
	v2i Result = {Scale*A.x, Scale*A.y};
	return Result;
}

inline v2i
operator/(v2i A, s32 Denom)
{
	Assert(Denom != 0);
	v2i Result = {A.x/Denom, A.y/Denom};
	return Result;
}

inline v2i
operator-(v2i B)
{
	v2i Result = {-B.x, -B.y};
	return Result;
}

inline v2i
operator-(v2i A, v2i B)
{
	v2i Result = {A.x - B.x, A.y - B.y};
	return Result;
}

inline v2i
operator+(v2i A, v2i B)
{
	v2i Result = {A.x + B.x, A.y + B.y};
	return Result;
}

inline b32
operator==(v2i A, v2i B)
{
	b32 Result = ((A.x == B.x) &&
	              (A.y == B.y));
	return Result;
}

inline b32
operator!=(v2i A, v2i B)
{
	b32 Result = !(A == B);
	return Result;
}

inline v2i &
operator+=(v2i &A, v2i B)
{
	A = A + B;
	return A;
}

inline v2i
Hadamard(v2 A, v2i B)
{
	v2i Result = {(s32)(A.x*B.x), (s32)(A.y*B.y)};
	return Result;
}

inline f32
Length(v2i A)
{
	f32 Result = Length(V2iToV2(A));
	return Result;
}

//
// NOTE: v3i operations
//

inline v3i
V3i(s32 x, s32 y, s32 z)
{
	v3i Result = {x, y, z};
	return Result;
}

inline v3i
operator+(v3i A, v3i B)
{
	v3i Result = {A.x + B.x, A.y + B.y, A.z + B.z};
	return Result;
}

inline v3i
operator-(v3i A, v3i B)
{
	v3i Result = {A.x - B.x, A.y - B.y, A.z - B.z};
	return Result;
}

inline b32
operator==(v3i A, v3i B)
{
	b32 Result = ((A.x == B.x) && (A.y == B.y) && (A.z == B.z));
	return Result;
}

inline b32
operator!=(v3i A, v3i B)
{
	b32 Result = !(A == B);
	return Result;
}

//
// NOTE: v3 operations
//

inline v3
V3(f32 x, f32 y, f32 z)
{
	v3 Result = {x, y, z};
	return Result;
}

inline v3
V3(v2 xy, f32 z)
{
	v3 Result = {xy.x, xy.y, z};
	return Result;
}

inline v3
V3(f32 x, v2 yz)
{
	v3 Result = {x, yz.x, yz.y};
	return Result;
}

inline v3
operator+(v3 A, v3 B)
{
	v3 Result = {A.x + B.x, A.y + B.y, A.z + B.z};
	return Result;
}

inline v3
operator*(f32 Scalar, v3 Vector)
{
	v3 Result = {Scalar*Vector.x, Scalar*Vector.y, Scalar*Vector.z};
	return Result;
}

inline v3
operator*(v3 Vector, f32 Scalar)
{
	v3 Result = Scalar*Vector;
	return Result;
}

inline v3
operator-(v3 A, v3 B)
{
	v3 Result = A + (-1.0f)*B;
	return Result;
}

inline v3
operator-(v3 A)
{
	v3 Result = V3(-A.x, -A.y, -A.z);
	return Result;
}

inline v3 &
operator+=(v3 &A, v3 B)
{
	A = A + B;
	return A;
}

inline v3 &
operator-=(v3 &A, v3 B)
{
	A = A - B;
	return A;
}

inline v3 &
operator*=(v3 &A, f32 V)
{
	A = V*A;
	return A;
}

inline b32
operator==(v3 A, v3 B)
{
	b32 Result = ((A.x == B.x) &&
	              (A.y == B.y) &&
	              (A.z == B.z));
	return Result;
}

inline b32
operator!=(v3 A, v3 B)
{
	b32 Result = !(A == B);
	return Result;
}

inline f32
Inner(v3 A, v3 B)
{
	f32 Result = A.x*B.x + A.y*B.y + A.z*B.z;
	return Result;
}

inline v3
Lerp(v3 A, f32 t, v3 B)
{
	v3 Result = V3(Lerp(A.x, t, B.x),
	               Lerp(A.y, t, B.y),
	               Lerp(A.z, t, B.z));
	return Result;
}

inline f32
LengthSq(v3 A)
{
	f32 Result = Inner(A,A);
	return Result;
}

inline f32
Length(v3 A)
{
	f32 Result = SquareRoot(LengthSq(A));
	return Result;
}

inline f32
Distance(v3 A, v3 B)
{
	f32 Result = Length(B - A);
	return Result;
}

inline v3
Normalize(v3 A)
{
	f32 ALength = Length(A);
	Assert(ALength != 0.0f);
	v3 Result = (1.0f/ALength)*A;
	return Result;
}

inline f32
DistanceToLine(v3 TestPoint, v3 LineUnit)
{
	f32 Result = Length(TestPoint - Inner(TestPoint, LineUnit)*LineUnit);
	return Result;
}

inline f32
DistanceToLineSq(v3 TestPoint, v3 LineUnit)
{
	f32 Result = LengthSq(TestPoint - Inner(TestPoint, LineUnit)*LineUnit);
	return Result;
}

inline f32
DistanceToLine(v3 TestPoint, v3 LineStart, v3 LineEnd)
{
	v3 LineUnit = Normalize(LineEnd - LineStart);
	f32 Result = DistanceToLine(TestPoint - LineStart, LineUnit);
	return Result;
}

inline f32
DistanceToLineSq(v3 TestPoint, v3 LineStart, v3 LineEnd)
{
	v3 LineUnit = Normalize(LineEnd - LineStart);
	f32 Result = DistanceToLineSq(TestPoint - LineStart, LineUnit);
	return Result;
}

inline f32
SignedDistanceToPlane(v3 TestPoint, v3 UnitNormal)
{
	f32 Result = Inner(TestPoint, UnitNormal);
	return Result;
}

inline f32
DistanceToPlane(v3 TestPoint, v3 UnitNormal)
{
	f32 Result = AbsoluteValue(SignedDistanceToPlane(TestPoint, UnitNormal));
	return Result;
}

inline b32
IsUnit(v3 A)
{
	f32 Epsilon = 0.001f;
	f32 ALengthSq = LengthSq(A);
	b32 Result = ((ALengthSq < (1.0f + Square(Epsilon))) &&
	              (ALengthSq > (1.0f - Square(Epsilon))));
	return Result;
}

inline v3
NormalizeOr(v3 A, v3 Or)
{
	f32 ALength = Length(A);
	v3 Result = Or;
	if(ALength != 0.0f)
	{
		Result = (1.0f/ALength)*A;
	}
	return Result;
}

inline v3
NOZ(v3 A)
{
	f32 ALength = Length(A);
	v3 Result = {};
	if(ALength != 0.0f)
	{
		Result = (1.0f/ALength)*A;
	}
	return Result;
}

inline v3
Cross(v3 U, v3 V)
{
	v3 Result = V3(U.y*V.z - U.z*V.y,
	               U.z*V.x - U.x*V.z,
	               U.x*V.y - U.y*V.x);
	return Result;
}

inline b32
SameDirection(v3 A, v3 B)
{
	b32 Result = (Inner(A, B) > 0.0f);
	return Result;
}

inline v3
Hadamard(v3 A, v3 B)
{
	v3 Result = {A.x*B.x, A.y*B.y, A.z*B.z};
	return Result;
}

inline b32
AreParallel(v3 A, v3 B)
{
	f32 Epsilon = 0.00001f;
	b32 Result = (LengthSq(Cross(A, B)) < Epsilon);
	return Result;
}

inline v2
SegmentBarycentricCoords(v3 Q, v3 A, v3 B)
{
	v2 Result = {};
	v3 AB = B-A;
	Result.x = Inner(B-Q, AB)/(Inner(AB, AB));
	Result.y = 1.0f - Result.x;
	return Result;
}

inline v3
SegmentClosestPoint(v3 Q, v3 A, v3 B)
{
	v2 BarycentricCoords = SegmentBarycentricCoords(Q, A, B);
	v3 Result = BarycentricCoords.x*A + BarycentricCoords.y*B;
	if(BarycentricCoords.x < 0.0f)
	{
		Result = B;
	}
	if(BarycentricCoords.y < 0.0f)
	{
		Result = A;
	}

	return Result;
}

inline v2
SegmentClosestPointBarycentricCoords(v3 Q, v3 A, v3 B)
{
	v2 Result = SegmentBarycentricCoords(Q, A, B);
	if(Result.x < 0.0f)
	{
		Result = {0, 1};
	}
	if(Result.y < 0.0f)
	{
		Result = {1, 0};
	}

	return Result;
}

inline f32
DistanceToSegmentSq(v3 TestPoint, v3 Head, v3 Tail)
{
	v3 ClosestPoint = SegmentClosestPoint(TestPoint, Head, Tail);
	f32 Result = LengthSq(TestPoint - ClosestPoint);
	return Result;
}

inline f32
DistanceToSegment(v3 TestPoint, v3 Head, v3 Tail)
{
	f32 Result = SquareRoot(DistanceToSegmentSq(TestPoint, Head, Tail));
	return Result;
}

inline f32
TriangleArea(v3 A, v3 B, v3 C)
{
	f32 Result = 0.5f*Length(Cross(B-A, C-A));
	return Result;
}

inline f32
SignedTriangleArea(v3 A, v3 B, v3 C, v3 Normal)
{
	Assert(IsUnit(Normal));
	v3 CrossProduct = Cross(B-A, C-A);
	f32 Result = 0.5f*Inner(CrossProduct, Normal);
	return Result;
}

inline v3
TriangleBarycentricCoords(v3 Q, v3 A, v3 B, v3 C)
{
	v3 Result = {};

	v3 Normal = Normalize(Cross(B-A, C-A));
	f32 AreaABC = SignedTriangleArea(A, B, C, Normal);
	f32 AreaQBC = SignedTriangleArea(Q, B, C, Normal);
	f32 AreaQCA = SignedTriangleArea(Q, C, A, Normal);
	Result.x = AreaQBC/AreaABC;
	Result.y = AreaQCA/AreaABC;
	Result.z = 1.0f - Result.x - Result.y;

	return Result;
}

inline v3
TriangleClosestPointBarycentricCoords(v3 Q, v3 A, v3 B, v3 C)
{
	v3 Result = {};

	v2 uvAB = SegmentBarycentricCoords(Q, A, B);
	v2 uvBC = SegmentBarycentricCoords(Q, B, C);
	v2 uvCA = SegmentBarycentricCoords(Q, C, A);

	if(uvCA.x <= 0.0f && uvAB.y <= 0.0f )
	{
		// NOTE: [A]
		Result = {1,0,0};
	}
	else if(uvAB.x <= 0.0f && uvBC.y <= 0.0f)
	{
		// NOTE: [B]
		Result = {0,1,0};
	}
	else if(uvBC.x <= 0.0f && uvCA.y <= 0.0f)
	{
		// NOTE: [C]
		Result = {0,0,1};
	}
	else
	{
		v3 uvwABC = TriangleBarycentricCoords(Q, A, B, C);

		if(uvAB.x > 0.0f && uvAB.y > 0.0f && uvwABC.z <= 0.0f)
		{
			// NOTE: [A, B]
			Result = {uvAB.x, uvAB.y, 0};
		}
		else if(uvBC.x > 0.0f && uvBC.y > 0.0f && uvwABC.x <= 0.0f)
		{
			// NOTE: [B, C]
			Result = {0, uvBC.x, uvBC.y};
		}
		else if(uvCA.x > 0.0f && uvCA.y > 0.0f && uvwABC.y <= 0.0f)
		{
			// NOTE: [C, A]
			Result = {uvCA.y, 0, uvCA.x};
		}
		else
		{
			Assert(uvwABC.x > 0.0f && uvwABC.y > 0.0f && uvwABC.z > 0.0f);
			// NOTE: [A, B, C]
			Result = {uvwABC.x, uvwABC.y, uvwABC.z};
		}
	}

	return Result;
}

inline v3
TriangleClosestPoint(v3 Q, v3 A, v3 B, v3 C)
{
	v3 BarycentricCoords = TriangleClosestPointBarycentricCoords(Q, A, B, C);
	v3 Result = BarycentricCoords.x*A + BarycentricCoords.y*B + BarycentricCoords.z*C;
	return Result;
}

inline b32
ProjectedPointInsideTriangle(v3 TestPoint, v3 A, v3 B, v3 C)
{
	v3 N = Cross(B-A, C-A);
	v3 N_AB = Cross(B-A, N);
	v3 N_BC = Cross(C-B, N);
	v3 N_CA = Cross(A-C, N);
	b32 Result = (!SameDirection(TestPoint-A, N_AB) &&
				  !SameDirection(TestPoint-B, N_BC) &&
				  !SameDirection(TestPoint-C, N_CA));
	return Result;
}

inline f32
SignedTetrahedronVolume(v3 A, v3 B, v3 C, v3 D)
{
	f32 Result = (1.0f/6.0f)*Inner(B-A, Cross(C-A, D-A));
	return Result;
}

inline v4
TetrahedronBarycentricCoords(v3 Q, v3 A, v3 B, v3 C, v3 D)
{
	v4 Result = {};
	f32 ABCD = SignedTetrahedronVolume(A, B, C, D);
	Result.x = SignedTetrahedronVolume(Q, B, C, D)/ABCD;
	Result.y = SignedTetrahedronVolume(A, Q, C, D)/ABCD;
	Result.z = SignedTetrahedronVolume(A, B, Q, D)/ABCD;
	Result.w = 1.0f - Result.x - Result.y - Result.z;

	return Result;
}

//
// NOTE: v4 operations
//

inline v4
V4(f32 x, f32 y, f32 z, f32 w)
{
	v4 Result = {x, y, z, w};
	return Result;
}

inline v4
V4(v2 V, f32 z, f32 w)
{
	v4 Result = V4(V.x, V.y, z, w);
	return Result;
}

inline v4
V4(v3 V, f32 w)
{
	v4 Result = V4(V.x, V.y, V.z, w);
	return Result;
}

inline b32
operator==(v4 A, v4 B)
{
	b32 Result = ((A.x == B.x) &&
	              (A.y == B.y) &&
	              (A.z == B.z) &&
	              (A.w == B.w));
	return Result;
}

inline v4
operator+(v4 A, v4 B)
{
	v4 Result = {A.x + B.x, A.y + B.y, A.z + B.z, A.w + B.w};
	return Result;
}

inline v4
operator*(f32 Scalar, v4 Vector)
{
	v4 Result = {Scalar*Vector.x, Scalar*Vector.y, Scalar*Vector.z, Scalar*Vector.w};
	return Result;
}

inline v4
operator*(v4 Vector, f32 Scalar)
{
	v4 Result = Scalar*Vector;
	return Result;
}

inline v4
operator-(v4 A, v4 B)
{
	v4 Result = A + (-1.0f)*B;
	return Result;
}

inline v4
operator-(v4 A)
{
	v4 Result = V4(-A.x, -A.y, -A.z, -A.w);
	return Result;
}

inline v4 &
operator+=(v4 &A, v4 B)
{
	A = A + B;
	return A;
}

inline v4
Lerp(v4 A, f32 t, v4 B)
{
	v4 Result = V4(Lerp(A.x, t, B.x),
	               Lerp(A.y, t, B.y),
	               Lerp(A.z, t, B.z),
	               Lerp(A.w, t, B.w));
	return Result;
}

inline f32
Inner(v4 A, v4 B)
{
	f32 Result = A.x*B.x + A.y*B.y + A.z*B.z + A.w*B.w;
	return Result;
}

inline v4
Hadamard(v4 A, v4 B)
{
	v4 Result = {A.x*B.x, A.y*B.y, A.z*B.z, A.w*B.w};
	return Result;
}

//
// NOTE: rectangle2 operations
//

inline rectangle2
Rectangle2(v2 Min, v2 Max)
{
	Assert(Min.x <= Max.x);
	Assert(Min.y <= Max.y);
	rectangle2 Result = {Min, Max};
	return Result;
}

inline rectangle2
Rectangle2CenterDim(v2 Center, v2 Dim)
{
	v2 Min = Center - 0.5f*Dim;
	v2 Max = Center + 0.5f*Dim;
	rectangle2 Result = Rectangle2(Min, Max);
	return Result;
}

inline rectangle2
operator+(v2 Offset, rectangle2 Rect)
{
	rectangle2 Result = Rect;
	Result.Min += Offset;
	Result.Max += Offset;
	return Result;
}

inline rectangle2
operator+(rectangle2 Rect, v2 Offset)
{
	rectangle2 Result = Offset + Rect;
	return Result;
}

inline v2
Dim(rectangle2 Rect)
{
	v2 Result = Rect.Max - Rect.Min;
	return Result;
}

inline v2
Center(rectangle2 Rect)
{
	v2 Result = 0.5f*(Rect.Max + Rect.Min);
	return Result;
}

inline b32
RectangleIntersect(rectangle2 A, rectangle2 B)
{
	b32 Result = !(((A.Max.x < B.Min.x) || (B.Max.x < A.Min.x)) ||
	               ((A.Max.y < B.Min.y) || (B.Max.y < A.Min.y)));
	return Result;
}

inline b32
InsideRectangle(rectangle2 Rect, v2 P)
{
	b32 Result = (((P.x >= Rect.Min.x) && (P.x <= Rect.Max.x)) &&
	              ((P.y >= Rect.Min.y) && (P.y <= Rect.Max.y)));
	return Result;
}

inline b32
Contains(rectangle2 A, rectangle2 B)
{
	b32 Result = (InsideRectangle(A, B.Min) && InsideRectangle(A, B.Max));
	return Result;
}

inline rectangle2
Combine(rectangle2 A, rectangle2 B)
{
	rectangle2 Result = A;
	for(u32 DimensionIndex = 0;
		DimensionIndex < 2;
		++DimensionIndex)
	{
		if(B.Min.E[DimensionIndex] < Result.Min.E[DimensionIndex])
		{
			Result.Min.E[DimensionIndex] = B.Min.E[DimensionIndex];
		}
		if(B.Max.E[DimensionIndex] > Result.Max.E[DimensionIndex])
		{
			Result.Max.E[DimensionIndex] = B.Max.E[DimensionIndex];
		}
	}

	return Result;
}

inline f32
Perimeter(rectangle2 A)
{
	v2 Dim = A.Max - A.Min;
	f32 Result = 2.0f*(Dim.x + Dim.y);
	return Result;
}

inline rectangle2
ClipTo(rectangle2 Rect, rectangle2 Clip)
{
	rectangle2 Result = Rect;
	Result.Min.x = Maximum(Result.Min.x, Clip.Min.x);
	Result.Min.y = Maximum(Result.Min.y, Clip.Min.y);
	Result.Max.x = Minimum(Result.Max.x, Clip.Max.x);
	Result.Max.y = Minimum(Result.Max.y, Clip.Max.y);
	return Result;
}

inline rectangle2
Expand(rectangle2 Rect, v2 V)
{
	rectangle2 Result = Rect;
	Result.Min -= V;
	Result.Max += V;
	return Result;
}

//
//	NOTE: rectangle2i operations
//

inline rectangle2i
Rectangle2i(v2i Min, v2i Max)
{
	Assert(Min.x <= Max.x);
	Assert(Min.y <= Max.y);
	rectangle2i Result = {Min, Max};
	return Result;
}

inline rectangle2i
operator+(v2i Offset, rectangle2i Rect)
{
	rectangle2i Result = Rect;
	Result.Min += Offset;
	Result.Max += Offset;
	return Result;
}

inline rectangle2i
operator+(rectangle2i Rect, v2i Offset)
{
	rectangle2i Result = Offset + Rect;
	return Result;
}

inline v2i
Dim(rectangle2i Rect)
{
	v2i Result = Rect.Max - Rect.Min;
	return Result;
}

inline v2i
Center(rectangle2i Rect)
{
	v2i Result = (Rect.Max + Rect.Min)/2;
	return Result;
}

//
// NOTE: rectangle3 operations
//

inline rectangle3
Rectangle3(v3 Min, v3 Max)
{
	Assert(Min.x <= Max.x);
	Assert(Min.y <= Max.y);
	Assert(Min.z <= Max.z);
	rectangle3 Result = {Min, Max};
	return Result;
}

inline rectangle3
Rectangle3CenterDim(v3 Center, v3 Dim)
{
	v3 Min = Center - 0.5f*Dim;
	v3 Max = Center + 0.5f*Dim;
	rectangle3 Result = Rectangle3(Min, Max);
	return Result;
}

inline rectangle3
InvertedInfinityRectangle3()
{
	rectangle3 Result =
	{
		V3(Real32Maximum, Real32Maximum, Real32Maximum),
		V3(Real32Minimum, Real32Minimum, Real32Minimum)
	};

	return Result;
}

inline rectangle3
operator+(v3 Offset, rectangle3 Rect)
{
	rectangle3 Result = Rect;
	Result.Min += Offset;
	Result.Max += Offset;
	return Result;
}

inline rectangle3
operator+(rectangle3 Rect, v3 Offset)
{
	rectangle3 Result = Offset + Rect;
	return Result;
}

inline v3
Dim(rectangle3 Rect)
{
	v3 Result = Rect.Max - Rect.Min;
	return Result;
}

inline v3
Center(rectangle3 Rect)
{
	v3 Result = 0.5f*(Rect.Max + Rect.Min);
	return Result;
}

inline b32
RectangleIntersect(rectangle3 A, rectangle3 B)
{
	b32 Result = !(((A.Max.x < B.Min.x) || (B.Max.x < A.Min.x)) ||
	               ((A.Max.y < B.Min.y) || (B.Max.y < A.Min.y)) ||
	               ((A.Max.z < B.Min.z) || (B.Max.z < A.Min.z)));
	return Result;
}

inline b32
InsideRectangle(rectangle3 Rect, v3 P)
{
	b32 Result = (((P.x >= Rect.Min.x) && (P.x <= Rect.Max.x)) &&
	              ((P.y >= Rect.Min.y) && (P.y <= Rect.Max.y)) &&
	              ((P.z >= Rect.Min.z) && (P.z <= Rect.Max.z)));
	return Result;
}

inline b32
Contains(rectangle3 A, rectangle3 B)
{
	b32 Result = (InsideRectangle(A, B.Min) && InsideRectangle(A, B.Max));
	return Result;
}

inline f32
SurfaceArea(rectangle3 Rect)
{
	v3 Dimension = Dim(Rect);
	f32 Result = 2.0f * (Dimension.x*Dimension.y + Dimension.x*Dimension.z + Dimension.y*Dimension.z);
	return Result;
}

inline rectangle3
Combine(rectangle3 A, rectangle3 B)
{
	rectangle3 Result = A;
	for(u32 DimensionIndex = 0;
		DimensionIndex < 3;
		++DimensionIndex)
	{
		if(B.Min.E[DimensionIndex] < Result.Min.E[DimensionIndex])
		{
			Result.Min.E[DimensionIndex] = B.Min.E[DimensionIndex];
		}
		if(B.Max.E[DimensionIndex] > Result.Max.E[DimensionIndex])
		{
			Result.Max.E[DimensionIndex] = B.Max.E[DimensionIndex];
		}
	}

	return Result;
}

//
// NOTE: mat2 operations
//

inline mat2
Mat2(v2 C0, v2 C1)
{
	mat2 Result = {};
	Result.C[0] = C0;
	Result.C[1] = C1;
	return Result;
}

inline mat2
IdentityMat2()
{
	mat2 Result =
	{
		1, 0,
		0, 1,
	};
	return Result;
}

inline mat2
operator+(mat2 A, mat2 B)
{
	mat2 Result = Mat2(A.C[0] + B.C[0],
	                   A.C[1] + B.C[1]);
	return Result;
}

inline v2
Row(mat2 A, u32 RowIndex)
{
	Assert(RowIndex < 2);
	v2 Result = V2(A.M[RowIndex], A.M[RowIndex + 2]);
	return Result;
}

inline mat2
operator*(mat2 A, mat2 B)
{
	mat2 Result = {};
	for(u32 Y = 0;
	    Y < 2;
	    ++Y)
	{
		for(u32 X = 0;
		    X < 2;
		    ++X)
		{
			u32 Index = Y + 2*X;
			Result.M[Index] = Inner(B.C[X], Row(A, Y));
		}
	}

	return Result;
}

inline f32
Determinant(mat2 A)
{
	f32 Result = A.M[0]*A.M[3] - A.M[1]*A.M[2];
	return Result;
}

inline mat2
Inverse(mat2 A)
{
	f32 Det = Determinant(A);
	Assert(Det != 0.0f);
	f32 InvDet = 1.0f/Det;
	mat2 Result =
	{
		InvDet*A.M[3], InvDet*(-A.M[2]),
		InvDet*(-A.M[1]), InvDet*A.M[0],
	};
	return Result;
}

inline mat2
Transpose(mat2 A)
{
	mat2 Result =
	{
		A.M[0], A.M[2],
		A.M[1], A.M[3],
	};
	return Result;
}

inline v2
operator*(mat2 A, v2 V)
{
	v2 Result = V2(Inner(Row(A, 0), V),
	               Inner(Row(A, 1), V));
	return Result;
}

inline mat2
RotationMat2(f32 Rotation)
{
	mat2 Result =
	{
		Cos(Rotation), Sin(Rotation),
		-Sin(Rotation), Cos(Rotation),
	};
	return Result;
}

inline f32
RotationAngleFromMat2(mat2 RotationMat)
{
	f32 Result = Arccos(RotationMat.M[0]);
	if(RotationMat.M[1] < 0.0f)
	{
		Result = 2.0f*Pi32 - Result;
	}
	return Result;
}

inline mat2
ReOrthonormalize(mat2 A)
{
	v2 C0 = Normalize(A.C[0]);
	v2 C1 = Perp(C0);
	mat2 Result = Mat2(C0, C1);
	return Result;
}

inline mat2
Lerp(mat2 A, f32 t, mat2 B)
{
	mat2 Result = {};
	Result.C[0] = Lerp(A.C[0], t, B.C[0]);
	Result.C[1] = Lerp(A.C[1], t, B.C[1]);
	return Result;
}

//
// NOTE: mat3 operations
//

inline mat3
Mat3(v3 C0, v3 C1, v3 C2)
{
	mat3 Result = {};
	Result.C[0] = C0;
	Result.C[1] = C1;
	Result.C[2] = C2;
	return Result;
}

inline mat3
IdentityMat3()
{
	mat3 Result =
	{
		1, 0, 0,
		0, 1, 0,
		0, 0, 1,
	};
	return Result;
}

inline mat3
operator+(mat3 A, mat3 B)
{
	mat3 Result = Mat3(A.C[0] + B.C[0],
	                   A.C[1] + B.C[1],
	                   A.C[2] + B.C[2]);
	return Result;
}

inline mat3
operator-(mat3 A, mat3 B)
{
	mat3 Result = Mat3(A.C[0] - B.C[0],
	                   A.C[1] - B.C[1],
	                   A.C[2] - B.C[2]);
	return Result;
}

inline mat3 &
operator+=(mat3 &A, mat3 B)
{
	A = A + B;
	return A;
}

inline v3
Row(mat3 A, u32 RowIndex)
{
	Assert(RowIndex < 3);
	v3 Result = V3(A.M[RowIndex], A.M[RowIndex + 3], A.M[RowIndex + 6]);
	return Result;
}

inline mat3
operator*(mat3 A, mat3 B)
{
	mat3 Result = {};
	for(u32 Y = 0;
	    Y < 3;
	    ++Y)
	{
		for(u32 X = 0;
		    X < 3;
		    ++X)
		{
			u32 Index = Y + 3*X;
			Result.M[Index] = Inner(B.C[X], Row(A, Y));
		}
	}

	return Result;
}

inline v3
operator*(mat3 A, v3 V)
{
	v3 Result = V3(Inner(Row(A, 0), V),
	               Inner(Row(A, 1), V),
	               Inner(Row(A, 2), V));
	return Result;
}

inline mat3
operator*(f32 Scale, mat3 A)
{
	mat3 Result = {};
	Result.C[0] = Scale*A.C[0];
	Result.C[1] = Scale*A.C[1];
	Result.C[2] = Scale*A.C[2];
	return Result;
}

inline mat3
Transpose(mat3 A)
{
	mat3 Result =
	{
		A.M[0], A.M[3], A.M[6],
		A.M[1], A.M[4], A.M[7],
		A.M[2], A.M[5], A.M[8],
	};
	return Result;
}

inline mat3
RotationMat3(f32 Pitch, f32 Yaw)
{
	mat3 PitchRotation =
	{
		1, 0, 0,
		0, Cos(Pitch), Sin(Pitch),
		0, -Sin(Pitch), Cos(Pitch),
	};
	mat3 YawRotation =
	{
		Cos(Yaw), 0, -Sin(Yaw),
		0, 1, 0,
		Sin(Yaw), 0, Cos(Yaw),
	};
	mat3 Result = YawRotation*PitchRotation;
	return Result;
}

inline f32
Determinant(mat3 M)
{
	f32 Result = M.M[0]*(M.M[4]*M.M[8]-M.M[7]*M.M[5]) -
				 M.M[3]*(M.M[1]*M.M[8]-M.M[7]*M.M[2]) +
				 M.M[6]*(M.M[1]*M.M[5]-M.M[4]*M.M[2]);
	return Result;
}

inline mat3
Inverse(mat3 M)
{
	f32 Det = Determinant(M);
	Assert(Det != 0);
	mat3 Result = {};
	Result.M[0] = Determinant(Mat2({M.a11, M.a21}, {M.a12, M.a22}));
	Result.M[1] = Determinant(Mat2({M.a12, M.a22}, {M.a10, M.a20}));
	Result.M[2] = Determinant(Mat2({M.a10, M.a20}, {M.a11, M.a21}));
	Result.M[3] = Determinant(Mat2({M.a02, M.a22}, {M.a01, M.a21}));
	Result.M[4] = Determinant(Mat2({M.a00, M.a20}, {M.a02, M.a22}));
	Result.M[5] = Determinant(Mat2({M.a01, M.a21}, {M.a00, M.a20}));
	Result.M[6] = Determinant(Mat2({M.a01, M.a11}, {M.a02, M.a12}));
	Result.M[7] = Determinant(Mat2({M.a02, M.a12}, {M.a00, M.a10}));
	Result.M[8] = Determinant(Mat2({M.a00, M.a10}, {M.a01, M.a11}));

	Result = (1.0f/Det)*Result;
	return Result;
}

inline mat3
Outer(v3 A, v3 B)
{
	mat3 Result = {};
	Result.C[0] = B.x*A;
	Result.C[1] = B.y*A;
	Result.C[2] = B.z*A;

	return Result;
}

//
// NOTE: mat4 operations
//

inline mat4
IdentityMat4()
{
	mat4 Result =
	{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	};
	return Result;
}

inline v4
Row(mat4 A, u32 RowIndex)
{
	Assert(RowIndex < 4);
	v4 Result = V4(A.M[RowIndex], A.M[RowIndex + 4], A.M[RowIndex + 8], A.M[RowIndex + 12]);
	return Result;
}

inline mat4
operator*(mat4 A, mat4 B)
{
	TIMED_BLOCK("mat4*mat4");

	mat4 Result = {};
	for(u32 Y = 0;
	    Y < 4;
	    ++Y)
	{
		for(u32 X = 0;
		    X < 4;
		    ++X)
		{
			u32 Index = Y + 4*X;
			Result.M[Index] = Inner(B.C[X], Row(A, Y));
		}
	}

	return Result;
}

inline v4
operator*(mat4 A, v4 V)
{
	TIMED_BLOCK("mat4*v4");

	v4 Result = V4(Inner(Row(A, 0), V),
	               Inner(Row(A, 1), V),
	               Inner(Row(A, 2), V),
	               Inner(Row(A, 3), V));
	return Result;
}

inline mat4
RotationMat4(f32 Roll)
{
	mat4 Result =
	{
		Cos(Roll), Sin(Roll), 0, 0,
		-Sin(Roll), Cos(Roll), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	};
	return Result;
}

inline mat4
RotationMat4(mat2 M)
{
	mat4 Result =
	{
		M.M[0], M.M[1], 0, 0,
		M.M[2], M.M[3], 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	};
	return Result;
}

inline mat4
RotationMat4(f32 Pitch, f32 Yaw)
{
	mat4 PitchRotation =
	{
		1, 0, 0, 0,
		0, Cos(Pitch), Sin(Pitch), 0,
		0, -Sin(Pitch), Cos(Pitch), 0,
		0, 0, 0, 1,
	};
	mat4 YawRotation =
	{
		Cos(Yaw), 0, -Sin(Yaw), 0,
		0, 1, 0, 0,
		Sin(Yaw), 0, Cos(Yaw), 0,
		0, 0, 0, 1,
	};
	mat4 Result = PitchRotation*YawRotation;
	return Result;
}

inline mat4
TranslationMat4(v2 V)
{
	mat4 Result =
	{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		V.x, V.y, 0, 1,
	};
	return Result;
}

inline mat4
TranslationMat4(v3 V)
{
	mat4 Result =
	{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		V.x, V.y, V.z, 1,
	};
	return Result;
}

inline mat4
ScaleMat4(f32 Scale)
{
	mat4 Result =
	{
		Scale, 0, 0, 0,
		0, Scale, 0, 0,
		0, 0, Scale, 0,
		0, 0, 0, 1,
	};
	return Result;
}

inline mat4
ScaleMat4(v2 Scale)
{
	mat4 Result =
	{
		Scale.x, 0, 0, 0,
		0, Scale.y, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	};
	return Result;
}

inline mat4
ScaleMat4(v3 Scale)
{
	mat4 Result =
	{
		Scale.x, 0, 0, 0,
		0, Scale.y, 0, 0,
		0, 0, Scale.z, 0,
		0, 0, 0, 1,
	};
	return Result;
}

inline mat4
TransformationMat4(v3 P, mat2 Rotation, v2 Scale, v2 Offset)
{
	v2 A = Rotation*Hadamard(Scale, Offset);
	mat4 Result =
	{
		Scale.x*Rotation.M[0], Scale.x*Rotation.M[1], 0, 0,
		Scale.y*Rotation.M[2], Scale.y*Rotation.M[3], 0, 0,
		0, 0, 1, 0,
		A.x + P.x, A.y + P.y, P.z, 1,
	};

	return Result;
}

inline mat4
TransformationMat4(v2 P, mat2 Rotation, v2 Scale, v2 Offset)
{
	mat4 Result = TransformationMat4(V3(P, 0.0f), Rotation, Scale, Offset);
	return Result;
}

inline mat4
OrthogonalProjectionMat4(f32 HalfWidth, f32 HalfHeight, f32 Near, f32 Far)
{
	mat4 Result = {
		1.0f/HalfWidth, 0, 0, 0,
		0, 1.0f/HalfHeight, 0, 0,
		0, 0, 2.0f/(Far - Near), 0,
		0, 0, (Far + Near)/(Far - Near), 1,
	};
	return Result;
}

inline mat4
InvOrthogonalProjectionMat4(f32 HalfWidth, f32 HalfHeight, f32 Near, f32 Far)
{
	mat4 Result = {
		HalfWidth, 0, 0, 0,
		0, HalfHeight, 0, 0,
		0, 0, 0.5f*(Far - Near), 0,
		0, 0, 0.5f*(-Far - Near), 1,
	};
	return Result;
}

inline mat4
PerspectiveProjectionMat4(f32 HorizontalFOV, f32 AspectRatio, f32 Near, f32 Far)
{
	f32 TanHalfHorizFOV = Tan(0.5f*HorizontalFOV);
	f32 InvTanHalfHorizFOV = 1.0f/TanHalfHorizFOV;

	mat4 Result = {
		InvTanHalfHorizFOV, 0, 0, 0,
		0, AspectRatio*InvTanHalfHorizFOV, 0, 0,
		0, 0, (Far + Near)/(Far - Near), -1.0f,
		0, 0, 2.0f*(Far*Near)/(Far - Near), 0,
	};
	return Result;
}

inline mat4
InvPerspectiveProjectionMat4(f32 HorizontalFOV, f32 AspectRatio, f32 Near, f32 Far)
{
	f32 TanHalfHorizFOV = Tan(0.5f*HorizontalFOV);

	mat4 Result = {
		TanHalfHorizFOV, 0, 0, 0,
		0, TanHalfHorizFOV/AspectRatio, 0, 0,
		0, 0, 0, (Far - Near)/(2.0f*Far*Near),
		0, 0, -1, (Far + Near)/(2.0f*Far*Near),
	};
	return Result;
}

//
// NOTE: quaternion operations
//

inline quaternion
Q(f32 r, f32 i, f32 j, f32 k)
{
	quaternion Result = {r, i, j, k};
	return Result;
}

inline quaternion
Q(f32 r, v3 V)
{
	quaternion Result = Q(r, V.x, V.y, V.z);
	return Result;
}

inline quaternion
NoRotation()
{
	quaternion Result = Q(1,0,0,0);
	return Result;
}

inline quaternion
operator+(quaternion A, quaternion B)
{
	quaternion Result = Q(A.r + B.r,
	                      A.i + B.i,
	                      A.j + B.j,
	                      A.k + B.k);
	return Result;
}

inline quaternion
operator*(quaternion A, quaternion B)
{
	quaternion Result = Q(A.r*B.r - A.i*B.i - A.j*B.j - A.k*B.k,
	                      A.r*B.i + A.i*B.r + A.j*B.k - A.k*B.j,
	                      A.r*B.j + A.j*B.r + A.k*B.i - A.i*B.k,
	                      A.r*B.k + A.k*B.r + A.i*B.j - A.j*B.i);
	return Result;
}

inline quaternion
operator*(f32 A, quaternion B)
{
	quaternion Result = Q(A*B.r, A*B.i, A*B.j, A*B.k);
	return Result;
}

inline f32
LengthSq(quaternion A)
{
	f32 Result = (A.r*A.r + A.i*A.i + A.j*A.j + A.k*A.k);
	return Result;
}

inline f32
Length(quaternion A)
{
	f32 Result = SquareRoot(LengthSq(A));
	return Result;
}

inline b32
IsUnit(quaternion A)
{
	f32 Epsilon = 0.001f;

	b32 Result = ((LengthSq(A) < (1.0f + Square(Epsilon))) &&
	              (LengthSq(A) > (1.0f - Square(Epsilon))));
	return Result;
}

inline quaternion
Normalize(quaternion A)
{
	f32 LengthA = Length(A);
	quaternion Result = Q(A.r/LengthA,
	                      A.i/LengthA,
	                      A.j/LengthA,
	                      A.k/LengthA);
	return Result;
}

inline quaternion
NormalizeOrNoRotation(quaternion A)
{
	f32 Epsilon = 0.001f;

	quaternion Result = Q(1,0,0,0);
	f32 LengthASq = LengthSq(A);
	if(LengthASq > Square(Epsilon))
	{
		f32 LengthA = SquareRoot(LengthASq);
		Result = Q(A.r/LengthA,
	               A.i/LengthA,
	               A.j/LengthA,
	               A.k/LengthA);
	}

	return Result;
}

inline quaternion
NormalizeIfNeeded(quaternion A)
{
	quaternion Result = IsUnit(A) ? A : Normalize(A);
	return Result;
}

inline quaternion
Conjugate(quaternion A)
{
	quaternion Result = Q(A.r, -A.i, -A.j, -A.k);
	return Result;
}

inline quaternion
Inverse(quaternion A)
{
	quaternion Result = {};
	if(IsUnit(A))
	{
		Result = Conjugate(A);
	}
	else
	{
		Result = (1.0f/LengthSq(A))*Conjugate(A);
	}
	return Result;
}

inline quaternion
RotationQuaternion(v3 Axis, f32 Angle)
{
	quaternion Result = {1, 0, 0, 0};
	if((Angle != 0) &&
	   (LengthSq(Axis) > 0))
	{
		Assert(IsUnit(Axis));
		f32 HalfAngle = 0.5f*Angle;
		f32 SinHalfAngle = Sin(HalfAngle);
		Result = Q(Cos(HalfAngle),
	                Axis.x*SinHalfAngle,
	                Axis.y*SinHalfAngle,
		            Axis.z*SinHalfAngle);
	}

	return Result;
}

inline quaternion
RotationQuaternion(v3 From, v3 To)
{
	quaternion Result = Q(1,0,0,0);
	f32 InnerProduct = Inner(From, To);
	if(InnerProduct < 1.0f)
	{
		if(InnerProduct <= -1.0f)
		{
			v3 Axis = Cross(V3(1,0,0), From);
			if(LengthSq(Axis) == 0.0f)
			{
				Axis = Cross(V3(0,1,0), From);
			}

			Result = RotationQuaternion(Axis, Pi32);
		}
		else
		{
			Result.xyz = Cross(From, To);
			Result.r = SquareRoot(LengthSq(From)*LengthSq(To)) + Inner(From, To);
			Result = NormalizeIfNeeded(Result);
		}
	}

	return Result;
}

inline quaternion
RotationQuaternion(v3 X, v3 Y, v3 Z)
{
	// TODO: No real idea how or if this works.
	quaternion Q = {};

	f32 Trace = X.x + Y.y + Z.z;
	if(Trace > 0.0f)
	{
		f32 S = 0.5f/SquareRoot(Trace + 1.0f);
		Q.r = 0.25f/S;
		Q.i = (Y.z - Z.y)*S;
		Q.j = (Z.x - X.z)*S;
		Q.k = (X.y - Y.x)*S;
	}
	else if((X.x > Y.y) && (X.x > Z.z))
	{
		f32 S = 2.0f * SquareRoot(1.0f + X.x - Y.y - Z.z);
		Q.r = (Y.z - Z.y)/S;
		Q.i = 0.25f * S;
		Q.j = (Y.x + X.y)/S;
		Q.k = (Z.x + X.z)/S;
	}
	else if(Y.y > Z.z)
	{
		f32 S = 2.0f * SquareRoot(1.0f + Y.y - X.x - Z.z);
		Q.r = (Z.x - X.z)/S;
		Q.i = (Y.x + X.y)/S;
		Q.j = 0.25f * S;
		Q.k = (Z.y + Y.z)/S;
	}
	else
	{
		f32 S = 2.0f * SquareRoot(1.0f + Z.z - X.x - Y.y );
		Q.r = (X.y - Y.x)/S;
		Q.i = (Z.x + X.z)/S;
		Q.j = (Z.y + Y.z)/S;
		Q.k = 0.25f * S;
	}

	return Q;
}

inline mat4
RotationMat4(quaternion Q)
{
	f32 TwoRI = 2.0f*Q.r*Q.i;
	f32 TwoRJ = 2.0f*Q.r*Q.j;
	f32 TwoRK = 2.0f*Q.r*Q.k;
	f32 TwoIJ = 2.0f*Q.i*Q.j;
	f32 TwoIK = 2.0f*Q.i*Q.k;
	f32 TwoJK = 2.0f*Q.j*Q.k;
	mat4 Result =
	{
		1.0f - 2.0f*Square(Q.j) - 2.0f*Square(Q.k), TwoIJ + TwoRK, TwoIK - TwoRJ, 0,
		TwoIJ - TwoRK, 1.0f - 2.0f*Square(Q.i) - 2.0f*Square(Q.k), TwoJK + TwoRI, 0,
		TwoIK + TwoRJ, TwoJK - TwoRI, 1.0f - 2.0f*Square(Q.i) - 2.0f*Square(Q.j), 0,
		0, 0, 0, 1
	};
	return Result;
}

inline mat3
RotationMat3(quaternion Q)
{
	Assert(IsUnit(Q));
	f32 TwoRI = 2.0f*Q.r*Q.i;
	f32 TwoRJ = 2.0f*Q.r*Q.j;
	f32 TwoRK = 2.0f*Q.r*Q.k;
	f32 TwoIJ = 2.0f*Q.i*Q.j;
	f32 TwoIK = 2.0f*Q.i*Q.k;
	f32 TwoJK = 2.0f*Q.j*Q.k;
	mat3 Result =
	{
		1.0f - 2.0f*Square(Q.j) - 2.0f*Square(Q.k), TwoIJ + TwoRK, TwoIK - TwoRJ,
		TwoIJ - TwoRK, 1.0f - 2.0f*Square(Q.i) - 2.0f*Square(Q.k), TwoJK + TwoRI,
		TwoIK + TwoRJ, TwoJK - TwoRI, 1.0f - 2.0f*Square(Q.i) - 2.0f*Square(Q.j),
	};
	return Result;
}

inline v3
RotateBy(v3 V, quaternion Rotation)
{
	v3 Result = (Rotation*Q(0.0f, V)*Conjugate(Rotation)).xyz;
	return Result;
}

inline quaternion
Lerp(quaternion A, f32 t, quaternion B)
{
	// NOTE: This is a non-constant speed lerp!
	quaternion Result = Normalize(Q(Lerp(A.r, t, B.r),
	                                Lerp(A.i, t, B.i),
	                                Lerp(A.j, t, B.j),
	                                Lerp(A.k, t, B.k)));
	return Result;
}

inline quaternion
GetRotationComponent(quaternion A, v3 Axis)
{
	quaternion Result = Q(1,0,0,0);

	f32 RotationAngle = 2.0f*Arccos(A.r);
	if(RotationAngle != 0.0f)
	{
		f32 InvHalfSinAngle = 1.0f/Sin(0.5f*RotationAngle);
		v3 RotationAxis = V3(A.i*InvHalfSinAngle, A.j*InvHalfSinAngle, A.k*InvHalfSinAngle);
		v3 NewRotationAxis = NOZ(Inner(RotationAxis, Axis)*Axis);
		if(LengthSq(NewRotationAxis) == 0.0f)
		{
			Result = A;
		}
		else
		{
			f32 NewRotationAngle = RotationAngle*Inner(RotationAxis, Axis);
			Result = RotationQuaternion(Axis, NewRotationAngle);
		}
	}

	return Result;
}

inline mat4
ViewMat4(v3 P, quaternion Rotation)
{
	mat4 Result = RotationMat4(Conjugate(Rotation))*TranslationMat4(-P);
	return Result;
}

inline mat4
InvViewMat4(v3 P, quaternion Rotation)
{
	mat4 Result = TranslationMat4(P)*RotationMat4(Rotation);
	return Result;
}

//
// NOTE: sphere operations
//

inline sphere
Sphere(v3 Center, f32 Radius)
{
	sphere Result = {Center, Radius};
	return Result;
}

inline v3
SphereRadiusVector(f32 Radius, f32 Phi, f32 Theta)
{
	v3 Result = Radius*V3(Sin(Phi)*Cos(Theta), -Cos(Phi), Sin(Phi)*Sin(Theta));
	return Result;
}
