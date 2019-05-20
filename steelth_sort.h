/*@H
* File: steelth_sort.h
* Author: Jesse Calvert
* Created: December 18, 2017, 17:10
* Last modified: February 13, 2018, 17:08
*/

#pragma once

struct sort_entry
{
	f32 Value;
	u64 Index;
};

inline b32
CheckSort(sort_entry *Entries, u32 EntryCount)
{
	b32 Result = true;
	for(u32 EntryIndex = 1;
		(EntryIndex < EntryCount) && Result;
		++EntryIndex)
	{
		sort_entry *CurrentEntry = Entries + EntryIndex;
		sort_entry *LastEntry = Entries + (EntryIndex - 1);
		if(CurrentEntry->Value < LastEntry->Value)
		{
			Result = false;
		}
	}

	return Result;
}

internal void
MergeSortRecursive(sort_entry *Entries, sort_entry *ScratchSpace, u32 EntryCount)
{
	if(EntryCount <= 1)
	{
		// NOTE: Sorting is easy!
	}
	else if(EntryCount == 2)
	{
		sort_entry *FirstEntry = Entries + 0;
		sort_entry *SecondEntry = Entries + 1;
		if(SecondEntry->Value < FirstEntry->Value)
		{
			sort_entry TempEntry = *FirstEntry;
			*FirstEntry = *SecondEntry;
			*SecondEntry = TempEntry;
		}
	}
	else
	{
		u32 HalfEntryCount = EntryCount / 2;
		MergeSortRecursive(Entries, ScratchSpace, HalfEntryCount);
		MergeSortRecursive(Entries + HalfEntryCount, ScratchSpace, EntryCount - HalfEntryCount);

		sort_entry *FirstHalfAt = Entries;
		sort_entry *SecondHalfAt = Entries + HalfEntryCount;
		sort_entry *At = ScratchSpace;
		for(u32 Index = 0;
			Index < EntryCount;
			++Index)
		{
			if(FirstHalfAt == Entries + HalfEntryCount)
			{
				*At++ = *SecondHalfAt++;
			}
			else if(SecondHalfAt == (Entries + EntryCount))
			{
				*At++ = *FirstHalfAt++;
			}
			else
			{
				if(SecondHalfAt->Value < FirstHalfAt->Value)
				{
					*At++ = *SecondHalfAt++;
				}
				else
				{
					*At++ = *FirstHalfAt++;
				}
			}
		}

		Assert(At == ScratchSpace + EntryCount);

		// TODO: Can we remove this copy?
		sort_entry *Dest = Entries;
		sort_entry *Source = ScratchSpace;
		while(EntryCount--)
		{
			*Dest++ = *Source++;
		}
	}
}

internal void
Sort(memory_region *Region, sort_entry *Entries, u32 EntryCount)
{
	// NOTE: Sorts from smallest Value to largest.

	TIMED_FUNCTION();

#if 1
	// NOTE: MergeSortRecursive path.
	temporary_memory TempMem = BeginTemporaryMemory(Region);

	sort_entry *ScratchSpace = PushArray(Region, EntryCount, sort_entry);
	MergeSortRecursive(Entries, ScratchSpace, EntryCount);

	EndTemporaryMemory(&TempMem);
#endif

	Assert(CheckSort(Entries, EntryCount));
}
