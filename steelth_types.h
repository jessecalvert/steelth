/*@H
* File: steelth_types.h
* Author: Jesse Calvert
* Created: October 26, 2018, 15:51
* Last modified: April 13, 2019, 16:30
*/

#pragma once

//
// NOTE: Compilers
//

#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif

#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif

#if COMPILER_MSVC
#include <intrin.h>
#elif COMPILER_LLVM
#include <x86intrin.h>
#else
#error SEE/NEON optimizations are not available for this compiler yet!!!!
#endif

#include <stdint.h>
#include <limits.h>
#include <float.h>
#include <stdarg.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8 s8;
typedef int16 s16;
typedef int32 s32;
typedef int64 s64;
typedef bool32 b32;

typedef uint8 u8;
typedef uint16 u16;
typedef uint32 u32;
typedef uint64 u64;

typedef uintptr_t uintptr;
typedef intptr_t intptr;

typedef size_t memory_index;

typedef float real32;
typedef double real64;

typedef real32 r32;
typedef real64 r64;

typedef r32 f32;
typedef r64 f64;

typedef uintptr_t umm;

#define flag32(type) u32
#define enum32(type) u32

#define Real32Maximum FLT_MAX
#define Real32Minimum -FLT_MAX

#define U32Maximum (UINT_MAX)
#define S32Maximum (INT_MAX)

#ifndef internal
    #define internal static
#endif
#define local_persist static
#define global static

#define Pi32 3.14159265359f

#define INTROSPECT(...)

#if GAME_DEBUG
#define Assert(Expression) if(!(Expression)) *(int *)0 = 0
#else
#define Assert(Expression)
#endif

#define CTAssert(Expression) static_assert(Expression, "Assert failed: " #Expression)

#define InvalidCodePath Assert(!"InvalidCodePath")
#define InvalidDefaultCase default: {InvalidCodePath;} break
#define NotImplemented Assert(!"Not Implemented!")

// TODO: should these all use 64-bit?
#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))

#define AlignPow2(Value, Alignment) ((Value + ((Alignment) - 1)) & ~((Alignment) - 1))
#define Align4(Value) ((Value + 3) & ~3)
#define Align8(Value) ((Value + 7) & ~7)
#define Align16(Value) ((Value + 15) & ~15)

#define U32FromPointer(Pointer) ((u32)(memory_index)(Pointer))
#define PointerFromU32(type, Value) (type *)((memory_index)Value)

#define OffsetOf(type, Member) (umm)&(((type *)0)->Member)

#define Swap(A, B, type) {type Temp = A; A = B; B = Temp;}

union v2
{
	r32 E[2];
	struct
	{
		r32 x, y;
	};
	struct
	{
		r32 u, v;
	};
};

union v2i
{
	s32 E[2];
	struct
	{
		s32 x, y;
	};
	struct
	{
		s32 u, v;
	};
};

union v3
{
	r32 E[3];
	struct
	{
		r32 x, y, z;
	};
	struct
	{
		r32 r, g, b;
	};
	struct
	{
		r32 Ignored_0;
		v2 yz;
	};
	struct
	{
		v2 xy;
		r32 Ignored_1;
	};
};

union v3i
{
	s32 E[3];
	struct
	{
		s32 x, y, z;
	};
	struct
	{
		s32 r, g, b;
	};
	struct
	{
		s32 Ignored_0;
		v2i yz;
	};
	struct
	{
		v2i xy;
		s32 Ignored_1;
	};
};

union v3u
{
	u32 E[3];
	struct
	{
		u32 x, y, z;
	};
	struct
	{
		u32 r, g, b;
	};
};

union v4
{
	r32 E[4];
	struct
	{
		r32 x, y, z, w;
	};
	struct
	{
		r32 r, g, b, a;
	};
	struct
	{
		v2 xy;
		v2 zw;
	};
	struct
	{
		r32 Ignored_0;
		v3 yzw;
	};
	struct
	{
		v3 xyz;
		r32 Ignored_1;
	};
	struct
	{
		v3 rgb;
		r32 Ignored_2;
	};
};

struct rectangle2
{
	v2 Min;
	v2 Max;
};

struct rectangle2i
{
	v2i Min;
	v2i Max;
};

struct rectangle3
{
	v3 Min;
	v3 Max;
};

union mat2
{
	r32 M[4];
	struct
	{
		v2 C[2];
	};
};

union mat3
{
	r32 M[9];
	struct
	{
		r32 a00, a10, a20;
		r32 a01, a11, a21;
		r32 a02, a12, a22;
	};
	struct
	{
		v3 C[3];
	};
};

union mat4
{
	r32 M[16];
	struct
	{
		v4 C[4];
	};
};

union quaternion
{
	r32 Q[4];
	struct
	{
		r32 r, i, j ,k;
	};
	struct
	{
		r32 Ignored_3;
		v3 xyz;
	};
};

struct oriented_box
{
	v3 P;
	v3 Dim;
	quaternion Rotation;
};

struct sphere
{
	v3 Center;
	r32 Radius;
};

struct string
{
	u32 Length;
	char *Text;
};

struct buffer
{
	umm Size;
	umm Used;
	u8 *Contents;
};

struct ticket_mutex
{
	u32 volatile Ticket;
	u32 volatile Serving;
};

struct entity_id
{
	u32 Value;
};

inline b32
operator==(entity_id A, entity_id B)
{
	b32 Result = (A.Value == B.Value);
	return Result;
}

inline b32
operator!=(entity_id A, entity_id B)
{
	b32 Result = !(A == B);
	return Result;
}

inline u32
SafeTruncateU64ToU32(u64 Value)
{
    // TODO: defines for maximum values, like UInt32Max
    Assert(Value <= 0xFFFFFFFF);
    u32 Result = (u32)Value;
    return Result;
}

inline u16
SafeTruncateU64ToU16(u64 Value)
{
    // TODO: defines for maximum values, like UInt32Max
    Assert(Value <= 0xFFFF);
    u16 Result = (u16)Value;
    return Result;
}

inline u8
SafeTruncateU64ToU8(u64 Value)
{
    // TODO: defines for maximum values, like UInt32Max
    Assert(Value <= 0xFF);
    u8 Result = (u8)Value;
    return Result;
}

inline u8
SafeTruncateS32ToU8(s32 Value)
{
    // TODO: defines for maximum values, like UInt32Max
    Assert(Value <= 0xFF);
    Assert(Value >= 0);
    u8 Result = (u8)Value;
    return Result;
}

#define DLIST_INIT(Sentinel) (Sentinel)->Next = (Sentinel)->Prev = (Sentinel)

#define DLIST_INSERT(Sentinel, Node) \
(Node)->Next = (Sentinel)->Next; \
(Node)->Prev = (Sentinel); \
(Node)->Next->Prev = (Node); \
(Node)->Prev->Next = (Node)

#define DLIST_REMOVE(Node) \
(Node)->Prev->Next = (Node)->Next; \
(Node)->Next->Prev = (Node)->Prev; \
(Node)->Next = (Node)->Prev = 0

#define DLIST_MERGE_LISTS(SentinelA, SentinelB) \
if((SentinelB)->Next != (SentinelB)) \
{ \
	(SentinelA)->Next->Prev = (SentinelB)->Prev; \
	(SentinelB)->Next->Prev = (SentinelA); \
	(SentinelB)->Prev->Next = (SentinelA)->Next; \
	(SentinelA)->Next = (SentinelB)->Next; \
	(SentinelB)->Next = (SentinelB)->Prev = (SentinelB); \
}

#define FOR_EACH(Name, Container, type) for(type *Name = GetFirst(Container); Name;	Name = GetNext(Container, Name))

inline b32
IsPowerOf2(u32 Value)
{
	b32 Result = (Value & (Value - 1)) == 0;
	return Result;
}

#ifdef COMPILER_MSVC
#define CompletePreviousWritesBeforeFutureWrites _WriteBarrier();
#define CompletePreviousReadsBeforeFutureReads _ReadBarrier();
inline u32 AtomicCompareExchangeU32(u32 volatile *Value, u32 New, u32 Expected)
{
    u32 Result = _InterlockedCompareExchange((long volatile *)Value, New, Expected);

    return(Result);
}
inline u64 AtomicExchangeU64(u64 volatile *Value, u64 New)
{
    u64 Result = _InterlockedExchange64((__int64 volatile *)Value, New);

    return(Result);
}
inline u64 AtomicAddU64(u64 volatile *Value, u64 Addend)
{
    // NOTE: Returns the original value prior to adding
    u64 Result = _InterlockedExchangeAdd64((__int64 volatile *)Value, Addend);

    return(Result);
}
inline u32 AtomicAddU32(u32 volatile *Value, u32 Addend)
{
    // NOTE: Returns the original value prior to adding
    u32 Result = _InterlockedExchangeAdd((long volatile *)Value, Addend);

    return(Result);
}
inline u32 GetThreadID(void)
{
    u8 *ThreadLocalStorage = (u8 *)__readgsqword(0x30);
    u32 ThreadID = *(u32 *)(ThreadLocalStorage + 0x48);

    return(ThreadID);
}

// TODO: Other compilers?
#endif

inline void
BeginTicketMutex(ticket_mutex *Mutex)
{
	u32 Ticket = AtomicAddU32(&Mutex->Ticket, 1);
	while(Ticket != Mutex->Serving) {_mm_pause();}
}

inline void
EndTicketMutex(ticket_mutex *Mutex)
{
	AtomicAddU32(&Mutex->Serving, 1);
}
