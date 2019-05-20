/*@H
* File: steelth_intrinsics.h
* Author: Jesse Calvert
* Created: October 12, 2017, 11:44
* Last modified: February 9, 2018, 11:15
*/

#pragma once

#include <math.h>

inline s32
SignOf(s32 Value)
{
    s32 Result = (Value >= 0) ? 1 : -1;
    return Result;
}

inline f32
SignOf(f32 Value)
{
    f32 Result = (Value >= 0) ? 1.0f : -1.0f;
    return Result;
}

inline r32
Square(r32 Value)
{
    r32 Result = Value*Value;
    return Result;
}

inline r32
Pow(r32 Base, r32 Exp)
{
    r32 Result = powf(Base, Exp);
    return Result;
}

inline f32
SquareRoot(f32 Real32)
{
    f32 Result = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(Real32)));
    return Result;
}

inline u32
RotateLeft(u32 Value, s32 Amount)
{
#if COMPILER_MSVC
    u32 Result = _rotl(Value, Amount);
#else
    //TODO: actually port this to other compiler platforms
    Amount &= 31;
    u32 Result = ((Value << Amount) | (Value >> (32 - Amount)));
#endif
    return Result;
}

inline u32
RotateRight(u32 Value, s32 Amount)
{
#if COMPILER_MSVC
    u32 Result = _rotr(Value, Amount);
#else
    // TODO: actually port this to other compiler platforms
    Amount &= 31;
    u32 Result = ((Value >> Amount) | (Value << (32 - Amount)));
#endif
    return Result;
}

inline f32
AbsoluteValue(f32 Real32)
{
    f32 Result = (r32) fabs(Real32);
    return Result;
}

inline s32
RoundF32ToS32(f32 Value)
{
    s32 Result = _mm_cvtss_si32(_mm_set_ss(Value));
    return Result;
}

inline u32
RoundF32ToU32(f32 Value)
{
    u32 Result = (u32)_mm_cvtss_si32(_mm_set_ss(Value));
    return Result;
}

inline s32
FloorReal32ToInt32(f32 Value)
{
    s32 Result = (s32)floorf(Value);
    return Result;
}

inline s32
CeilReal32ToInt32(f32 Value)
{
    s32 Result = (s32)ceilf(Value);
    return Result;
}

inline s32
TruncateReal32ToInt32(f32 Value)
{
    s32 Result = (s32) (Value);
    return Result;
}

inline f32
Sin(f32 Angle)
{
    f32 Result = sinf(Angle);
    return Result;
}

inline f32
Cos(f32 Angle)
{
    f32 Result = cosf(Angle);
    return Result;
}

inline f32
Tan(f32 Angle)
{
    f32 Result = tanf(Angle);
    return Result;
}

inline f32
Arccos(f32 Value)
{
    f32 Result = (r32) acos(Value);
    return Result;
}

inline f32
ATan2(f32 Y, f32 X)
{
    f32 Result = (r32) atan2(Y, X);
    return Result;
}

inline f32
Log(f32 A)
{
    f32 Result = logf(A);
    return Result;
}

inline r32
LogBase2(r32 A)
{
    r32 Result = log2f(A);
    return Result;
}

inline r32
LogBase10(r32 A)
{
    r32 Result = log10f(A);
    return Result;
}

struct bit_scan_result
{
    bool32 Found;
    u32 Index;
};

inline bit_scan_result
FindLeastSignificantSetBit(u32 Value)
{
    bit_scan_result Result = {};

#if COMPILER_MSVC
    Result.Found = _BitScanForward((unsigned long *)&Result.Index, Value);
#else
    for (u32 Test = 0; Test < 32; ++Test)
    {
        if(Value & (1 << Test))
        {
            Result.Index = Test;
            Result.Found = true;
            break;
        }
    }
#endif

    return Result;
}

