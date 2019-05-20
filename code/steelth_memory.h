/*@H
* File: steelth_memory.h
* Author: Jesse Calvert
* Created: November 9, 2017, 11:36
* Last modified: April 10, 2019, 17:31
*/

#pragma once
#include "steelth_shared.h"

enum region_flags
{
	RegionFlag_UnderflowChecking = 0x1,
	RegionFlag_OverflowChecking = 0x2,
	RegionFlag_NotRestored = 0x4,
};
struct memory_region
{
	platform_memory_block *CurrentBlock;

	flag32(region_flags) Flags;
	umm MinimumBlockSize;
	u32 TemporaryCount;
};

struct temporary_memory
{
	memory_region *Region;
	umm UsedMarked;
	u32 TemporaryCountMarked;
	platform_memory_block *Block;
};

inline temporary_memory
BeginTemporaryMemory(memory_region *RegionInit)
{
	temporary_memory Result = {};
	Result.Region = RegionInit;
	Result.UsedMarked = Result.Region->CurrentBlock ? Result.Region->CurrentBlock->Used : 0;
	Result.TemporaryCountMarked = Result.Region->TemporaryCount++;
	Result.Block = RegionInit->CurrentBlock;
	return Result;
}

inline void
FreeLastBlock(memory_region *Region)
{
	platform_memory_block *Block = Region->CurrentBlock;
	Region->CurrentBlock = Block->BlockPrev;
	Platform->DeallocateMemory(Block);
}

inline void
EndTemporaryMemory(temporary_memory *TempMem)
{
	memory_region *Region = TempMem->Region;
	Region->TemporaryCount -= 1;
	Assert(Region->TemporaryCount == TempMem->TemporaryCountMarked);

	while(TempMem->Block != Region->CurrentBlock)
	{
		FreeLastBlock(Region);
	}

	if(Region->CurrentBlock)
	{
		Region->CurrentBlock->Used = TempMem->UsedMarked;
	}
}

#define ZeroStruct(Struct) ZeroSize(&(Struct), sizeof(Struct))
#define ZeroArray(Array, Count) ZeroSize((Array), (Count)*sizeof(Array[0]))
inline void
ZeroSize(void *Result, umm Size)
{
	u8 *ToZero = (u8 *)Result;
	while(Size--)
	{
		*ToZero++ = 0;
	}
}

inline umm
GetAlignmentOffset(memory_region *Region, umm Alignment)
{
	umm AlignmentOffset = 0;
	umm CurrentPlace = (umm)Region->CurrentBlock->Base + Region->CurrentBlock->Used;
	umm AlignmentMask = Alignment - 1;
	if(AlignmentMask & CurrentPlace)
	{
		AlignmentOffset = Alignment - (AlignmentMask & CurrentPlace);
	}

	return AlignmentOffset;
}

enum push_param_flag
{
	PushFlag_ClearToZero = 0x1,
};

struct push_params
{
	flag32(push_param_flag) Flags;
	u32 Alignment;
};

inline push_params
DefaultPushParams()
{
	push_params Result = {};
	Result.Flags |= PushFlag_ClearToZero;
	Result.Alignment = 4;
	return Result;
}

inline push_params
Align(u32 Alignment)
{
	push_params Result = DefaultPushParams();
	Result.Alignment = Alignment;
	return Result;
}

inline push_params
AlignNoClear(u32 Alignment)
{
	push_params Result = DefaultPushParams();
	Result.Alignment = Alignment;
	Result.Flags &= ~PushFlag_ClearToZero;
	return Result;
}

inline push_params
NoClear()
{
	push_params Result = DefaultPushParams();
	Result.Flags &= ~PushFlag_ClearToZero;
	return Result;
}

inline umm
GetEffectiveSize(memory_region *Region, umm DesiredSize, push_params Params)
{
	umm EffectiveSize = 0;
	umm AlignmentOffset = GetAlignmentOffset(Region, Params.Alignment);

	EffectiveSize = DesiredSize + AlignmentOffset;
	return EffectiveSize;
}

#define PushStruct(Region, type, ...) (type *)PushSize_((Region), sizeof(type), ## __VA_ARGS__)
#define PushArray(Region, Count, type, ...) (type *)PushSize_((Region), (Count)*sizeof(type), ## __VA_ARGS__)
#define PushSize(Region, DesiredSize, ...) (u8 *)PushSize_((Region), (DesiredSize), ## __VA_ARGS__)
inline u8 *
PushSize_(memory_region *Region, umm DesiredSize, push_params Params = DefaultPushParams())
{
	Assert(IsPowerOf2(Params.Alignment));
	Assert(Params.Alignment <= 4096);
	Assert(Params.Alignment > 0);
	Assert(!((Region->Flags & RegionFlag_OverflowChecking) &&
		(Region->Flags & RegionFlag_UnderflowChecking)));

	umm Size = DesiredSize;
	if(Region->CurrentBlock)
	{
		Size = GetEffectiveSize(Region, DesiredSize, Params);
	}

	if(!(Region->CurrentBlock) ||
		((Region->CurrentBlock->Size - Region->CurrentBlock->Used) < Size))
	{
		Size = DesiredSize;
		if(!Region->MinimumBlockSize)
		{
			if((Region->Flags & RegionFlag_OverflowChecking) ||
				(Region->Flags & RegionFlag_UnderflowChecking))
			{
				Region->MinimumBlockSize = 0;
				Size = AlignPow2(Size, (umm)Params.Alignment);
			}
			else
			{
				Region->MinimumBlockSize = 1024*1024;
			}
		}
		umm NewBlockSize = Maximum(Region->MinimumBlockSize, Size);
		platform_memory_block *NewBlock = Platform->AllocateMemory(NewBlockSize, Region->Flags);
		NewBlock->BlockPrev = Region->CurrentBlock;
		Region->CurrentBlock = NewBlock;
	}

	umm AlignmentOffset = GetAlignmentOffset(Region, Params.Alignment);
	u8 *Result = Region->CurrentBlock->Base + Region->CurrentBlock->Used + AlignmentOffset;
	Region->CurrentBlock->Used += Size;

	Assert(Size >= DesiredSize);

	if(Params.Flags & PushFlag_ClearToZero)
	{
		ZeroSize(Result, DesiredSize);
	}

	return Result;
}

struct bootstrap_params
{
	flag32(region_flags) Flags;
	umm MinimumBlockSize;
};

inline bootstrap_params
DefaultBootstrapParams()
{
	bootstrap_params Result = {};
	// Result.Flags |= RegionFlag_UnderflowChecking;
	// Result.Flags |= RegionFlag_OverflowChecking;
	return Result;
}

inline bootstrap_params
UnderflowCheckingParams()
{
	bootstrap_params Result = DefaultBootstrapParams();
	Result.Flags |= RegionFlag_UnderflowChecking;
	return Result;
}

inline bootstrap_params
OverflowCheckingParams()
{
	bootstrap_params Result = DefaultBootstrapParams();
	Result.Flags |= RegionFlag_OverflowChecking;
	return Result;
}

inline bootstrap_params
NonRestoredParams()
{
	bootstrap_params Result = DefaultBootstrapParams();
	Result.Flags |= RegionFlag_NotRestored;
	return Result;
}

inline bootstrap_params
MinimumBlockSizeParams(umm MinimumBlockSize)
{
	bootstrap_params Result = DefaultBootstrapParams();
	Result.MinimumBlockSize = MinimumBlockSize;
	return Result;
}

#define BootstrapPushStruct(type, Member, ...) (type *)BootstrapPushSize_(sizeof(type), OffsetOf(type, Member), ## __VA_ARGS__)
inline void *
BootstrapPushSize_(umm StructSize, umm OffsetToArena,
                   bootstrap_params BootstrapParams = DefaultBootstrapParams(),
                   push_params Params = DefaultPushParams())
{
    memory_region Bootstrap = {};
    Bootstrap.Flags = BootstrapParams.Flags;
    Bootstrap.MinimumBlockSize = BootstrapParams.MinimumBlockSize;
    void *Struct = PushSize(&Bootstrap, StructSize, Params);
    memory_region *MemoryRegion = (memory_region *)((u8 *)Struct + OffsetToArena);
    *MemoryRegion = Bootstrap;

    return(Struct);
}

inline void
CheckMemory(memory_region *Region)
{
	Assert(Region->TemporaryCount == 0);
}

inline void
CopySize(void *Source, void *Dest, umm Size)
{
	u8 *DestPtr = (u8 *)Dest;
	u8 *SourcePtr = (u8 *)Source;
	while(Size)
	{
		*DestPtr++ = *SourcePtr++;
		--Size;
	}
}

inline void
Clear(memory_region *Region)
{
	while(Region->CurrentBlock)
	{
		b32 IsFinalBlock = (Region->CurrentBlock->BlockPrev == 0);
		FreeLastBlock(Region);
		if(IsFinalBlock)
		{
			break;
		}
	}
}

struct region_stats
{
	umm TotalSize;
	umm TotalUsed;
	u32 BlockCount;
};

inline region_stats
GetRegionStats(memory_region *Region)
{
	region_stats Result = {};
	platform_memory_block *Block = Region->CurrentBlock;
	while(Block)
	{
		Result.TotalSize += Block->Size;
		Result.TotalUsed += Block->Used;
		++Result.BlockCount;
		Block = Block->BlockPrev;
	}

	return Result;
}

inline string
PushString(memory_region *Region, string String)
{
	string Result = {};
	Result.Text = PushArray(Region, String.Length, char);
	while(Result.Length != String.Length)
	{
		Result.Text[Result.Length] = String.Text[Result.Length];
		Result.Length++;
	}
	return Result;
}

inline string
PushString(memory_region *Region, char *CString)
{
	string Result = PushString(Region, WrapZ(CString));
	return Result;
}

inline b32
MemoryCompare(void *A, void *B, umm Size)
{
	b32 Result = true;
	u8 *APtr = (u8 *)A;
	u8 *BPtr = (u8 *)B;
	while(Result && Size)
	{
		Result = (*APtr++ == *BPtr++);
		--Size;
	}

	return Result;
}
