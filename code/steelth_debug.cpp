/*@H
* File: steelth_debug.cpp
* Author: Jesse Calvert
* Created: November 6, 2017, 17:31
* Last modified: April 13, 2019, 16:33
*/

#include "steelth_debug.h"

internal debug_id
CreateIDFromGUID(char *GUID)
{
	debug_id Result = {};
	Result.Value[0] = GUID;
	Result.Value[1] = 0;

	Result.Parsed.HashValue = 281;

	string Name = {};
	string Filename = {};
	Filename.Text = GUID;

	u32 PipeCount = 0;
	char *Scan = GUID;
	while(*Scan)
	{
		if(*Scan == '|')
		{
			switch(PipeCount)
			{
				case 0:
				{
					Filename.Length = (u32)(Scan - GUID);
					Result.Parsed.LineNumber = StringToS32(Scan + 1);
				} break;

				case 1:
				{
					Result.Parsed.HashValue += 2371*StringToS32(Scan + 1);
				} break;

				case 2:
				{
					Name.Text = (Scan + 1);
				} break;

				InvalidDefaultCase;
			}

			++PipeCount;
		}

		Result.Parsed.HashValue += 7661*(*Scan) + 23;
		++Scan;
	}
	Assert(Name.Text);
	Name.Length = (u32)(Scan - Name.Text);

	CopySize(Name.Text, Result.Parsed.Name, Name.Length);
	CopySize(Filename.Text, Result.Parsed.Filename, Filename.Length);

	Result.IsValid = true;
	return Result;
}

internal debug_time_block *
CreateNewTimeBlock(debug_state *State, debug_event *BlockBegin)
{
	debug_time_block *Block = State->TimeBlockFreeList;
	if(Block)
	{
		State->TimeBlockFreeList = Block->Next;
		ZeroStruct(*Block);
	}
	else
	{
		Block = PushStruct(&State->Region, debug_time_block);
	}

	Block->ID = CreateIDFromGUID(BlockBegin->GUID);
	Block->ThreadID = BlockBegin->ThreadID;
	Block->TickStart = BlockBegin->Tick;
	DLIST_INIT(Block);

	return Block;
}

internal debug_time_block *
CreateNewFrameTimeBlock(debug_state *State)
{
	debug_event FrameDummy = {};
	FrameDummy.GUID = "a|0|0|FrameDummyGUID";
	debug_time_block *Result = CreateNewTimeBlock(State, &FrameDummy);
	Result->Depth = 0;
	return Result;
}

internal void
FreeTimeBlockRecursive_(debug_state *State, debug_time_block *Block)
{
	if(Block)
	{
		if(Block->FirstChild)
		{
			Block->FirstChild->Prev->Next = 0;
			FreeTimeBlockRecursive_(State, Block->FirstChild);
		}

		debug_time_block *NextToFree = Block->Next;
		Block->Next = State->TimeBlockFreeList;
		State->TimeBlockFreeList = Block;
		FreeTimeBlockRecursive_(State, NextToFree);
	}
}

internal void
FreeTimeBlockEntireTree(debug_state *State, debug_time_block *Block)
{
	if(Block)
	{
		Assert(Block->Parent == 0);
		Block->Next = 0;
		FreeTimeBlockRecursive_(State, Block);
	}
}

internal void
ClearStatHashTable(debug_state *State, u32 FrameIndex)
{
	for(u32 HashIndex = 0;
		HashIndex < ArrayCount(State->StatHashTable[FrameIndex]);
		++HashIndex)
	{
		debug_time_stat *Stat = State->StatHashTable[FrameIndex][HashIndex];
		while(Stat)
		{
			debug_time_stat *NextStat = Stat->NextInHash;
			Stat->NextInHash = State->TimeStatFreeList;
			State->TimeStatFreeList = Stat;
			Stat = NextStat;
		}
		State->StatHashTable[FrameIndex][HashIndex] = 0;
	}
}

internal debug_time_stat *
GetTimeStat(debug_state *State, debug_id ID, u32 FrameIndex)
{
	Assert(ID.IsValid);
	debug_parsed_guid Parsed = ID.Parsed;
	u32 HashIndex = Parsed.HashValue % ArrayCount(State->StatHashTable[0]);
	debug_time_stat *Stat = State->StatHashTable[FrameIndex][HashIndex];
	while(Stat)
	{
		if(Stat->ID == ID)
		{
			break;
		}
		Stat = Stat->NextInHash;
	}
	if(!Stat)
	{
		Stat = State->TimeStatFreeList;
		if(Stat)
		{
			State->TimeStatFreeList = Stat->NextInHash;
			ZeroStruct(*Stat);
		}
		else
		{
			Stat = PushStruct(&State->Region, debug_time_stat);
		}

		Stat->NextInHash = State->StatHashTable[FrameIndex][HashIndex];
		State->StatHashTable[State->CurrentFrame][HashIndex] = Stat;
		Stat->ID = ID;
		Stat->MinTicks = 0xFFFFFFFFFFFFFFFF;
	}

	return Stat;
}

internal void
BlockCalculateTicksUsed(debug_time_block *Block)
{
	Block->TicksUsed = Block->TickEnd - Block->TickStart;
	debug_time_block *Child = Block->FirstChild;
	if(Child)
	{
		do
		{
			Block->TicksUsed -= (Child->TickEnd - Child->TickStart);
			Child = Child->Next;
		} while(Child != Block->FirstChild);
	}
}

internal debug_stored_event *
StoreEvent(debug_state *State, debug_event *Event, debug_id ID)
{
	debug_stored_event *Result = State->FirstFreeStoredEvent;
	if(Result)
	{
		State->FirstFreeStoredEvent = Result->NextFree;
		ZeroStruct(*Result);
	}
	else
	{
		Result = PushStruct(&State->Region, debug_stored_event);
	}

	Result->ID = ID;
	Result->Type = Event->Type;
	Result->Tick = Event->Tick;
	Result->Value_u64 = Event->Value_u64;

	return Result;
}

internal debug_ui_element *
GetOrCreateUIElement(debug_state *State, u32 FrameIndex, debug_event *Event, debug_id ID)
{
	debug_ui_element *Result = 0;
	debug_ui_element *Parent = State->LastOpenUIElement;

	Result = State->LastOpenUIElement->FirstChild;
	while(Result)
	{
		if(Result->StoredEvent->ID == ID)
		{
			break;
		}

		if((Event->Type == Event_BeginDataBlock) &&
		   (StringCompare(WrapZ(Result->StoredEvent->ID.Parsed.Name), WrapZ(ID.Parsed.Name))))
		{
			break;
		}

		Result = Result->Next;
		if(Result == State->LastOpenUIElement->FirstChild)
		{
			Result = 0;
			break;
		}
	}

	if(!Result)
	{
		Result = State->FirstFreeUIElement;
		if(Result)
		{
			State->FirstFreeUIElement = Result->NextFree;
			ZeroStruct(*Result);
		}
		else
		{
			Result = PushStruct(&State->Region, debug_ui_element);
		}

		Result->Depth = Parent->Depth + 1;
		Result->Open = false;
		Result->Parent = Parent;

		debug_stored_event *StoredEvent = StoreEvent(State, Event, ID);
		Result->StoredEvent = StoredEvent;

		if(Parent->FirstChild)
		{
			DLIST_INSERT(Parent->FirstChild->Prev, Result);
		}
		else
		{
			DLIST_INIT(Result);
			Parent->FirstChild = Result;
		}
	}

	Result->FrameIndex = FrameIndex;

	return Result;
}

inline u32
GetThreadIndex(debug_state *State, u32 ThreadID)
{
	u32 Result = 0;
	b32 FoundIndex = false;
	for(u32 Index = 0;
		Index < State->ThreadCount;
		++Index)
	{
		u32 ID = State->ThreadIndexes[Index];
		if(ID == ThreadID)
		{
			Result = Index;
			FoundIndex = true;
			break;
		}
	}

	if(!FoundIndex)
	{
		Result = State->ThreadCount++;
		State->ThreadIndexes[Result] = ThreadID;
	}

	return Result;
}

internal void
CollateDebugRecords(debug_state *State, debug_event *EventArray, u32 EventCount)
{
	for(u32 EventIndex = 0;
		EventIndex < EventCount;
		++EventIndex)
	{
		debug_event *Event = EventArray + EventIndex;
		switch(Event->Type)
		{
			case Event_FrameMark:
			{
				for(u32 ThreadIndex = 0;
					ThreadIndex < State->ThreadCount;
					++ThreadIndex)
				{
					if(State->LastOpenBlock[ThreadIndex])
					{
						Assert(!State->LastOpenBlock[ThreadIndex]->Parent);
					}
				}

				debug_time_block *EndingFrameBlock = State->LastOpenBlock[0];
				EndingFrameBlock->ID = CreateIDFromGUID(Event->GUID);
				EndingFrameBlock->TickEnd = Event->Tick;
				EndingFrameBlock->TickStart = EndingFrameBlock->TickEnd - Event->Value_u64;
				BlockCalculateTicksUsed(EndingFrameBlock);

				for(u32 ThreadIndex = 0;
					ThreadIndex < State->ThreadCount;
					++ThreadIndex)
				{
					State->LastOpenBlock[ThreadIndex] = 0;
				}

				u32 LastFrame = State->CurrentFrame;
				State->CurrentFrame = (State->CurrentFrame + 1) % ArrayCount(State->FrameBlockTimings);
				if(((State->ViewingFrame + 1) % ArrayCount(State->FrameBlockTimings)) == LastFrame)
				{
					if(State->CurrentFrame)
					{
						State->ViewingFrame = State->CurrentFrame - 1;
					}
					else
					{
						State->ViewingFrame = ArrayCount(State->FrameBlockTimings) - 1;
					}

				}
				ClearStatHashTable(State, State->CurrentFrame);
			} break;

			case Event_BeginBlock:
			{
				u32 ThreadIndex = GetThreadIndex(State, Event->ThreadID);

				if(!State->LastOpenBlock[ThreadIndex])
				{
					if(ThreadIndex == 0)
					{
						State->LastOpenBlock[ThreadIndex] = CreateNewFrameTimeBlock(State);
						State->LastOpenBlock[ThreadIndex]->FrameIndex = State->CurrentFrame;

						debug_time_block *OldFrame = State->FrameBlockTimings[State->CurrentFrame];
						FreeTimeBlockEntireTree(State, OldFrame);
						State->FrameBlockTimings[State->CurrentFrame] = State->LastOpenBlock[ThreadIndex];
					}
					else
					{
						State->LastOpenBlock[ThreadIndex] = State->FrameBlockTimings[State->CurrentFrame];
					}
				}

				debug_time_block *TimeBlock = CreateNewTimeBlock(State, Event);
				TimeBlock->FrameIndex = State->CurrentFrame;

				if(State->LastOpenBlock[ThreadIndex]->FirstChild)
				{
					DLIST_INSERT(State->LastOpenBlock[ThreadIndex]->FirstChild->Prev, TimeBlock);
				}
				else
				{
					State->LastOpenBlock[ThreadIndex]->FirstChild = TimeBlock;
				}
				TimeBlock->Parent = State->LastOpenBlock[ThreadIndex];
				TimeBlock->Depth = State->LastOpenBlock[ThreadIndex]->Depth + 1;

				State->LastOpenBlock[ThreadIndex] = TimeBlock;
			} break;

			case Event_EndBlock:
			{
				u32 ThreadIndex = GetThreadIndex(State, Event->ThreadID);

				Assert(State->LastOpenBlock[ThreadIndex]->Parent);
				debug_time_block *OpenTimeBlock = State->LastOpenBlock[ThreadIndex];
				Assert(OpenTimeBlock->TickEnd == 0);
				OpenTimeBlock->TickEnd = Event->Tick;
				BlockCalculateTicksUsed(OpenTimeBlock);

				debug_time_stat *Stat = GetTimeStat(State, OpenTimeBlock->ID, State->CurrentFrame);
				Stat->MaxTicks = Maximum(OpenTimeBlock->TicksUsed, Stat->MaxTicks);
				Stat->MinTicks = Minimum(OpenTimeBlock->TicksUsed, Stat->MinTicks);
				Stat->TotalTicks += OpenTimeBlock->TicksUsed;
				++Stat->Count;

				State->LastOpenBlock[ThreadIndex] = State->LastOpenBlock[ThreadIndex]->Parent;
			} break;

			case Event_BeginDataBlock:
			{
				debug_ui_element *UIElement = GetOrCreateUIElement(State, State->CurrentFrame, Event, CreateIDFromGUID(Event->GUID));
				State->LastOpenUIElement = UIElement;
			} break;

			case Event_EndDataBlock:
			{
				Assert(State->LastOpenUIElement->Parent);
				State->LastOpenUIElement = State->LastOpenUIElement->Parent;
			} break;

			default:
			{
				debug_ui_element *UIElement = GetOrCreateUIElement(State, State->CurrentFrame, Event, CreateIDFromGUID(Event->GUID));
			}
		}
	}
}

internal void
DebugDrawSingleFrameTimings(debug_state *State, render_group *Group,
	u32 FrameIndex, v2 Min, v2 Max, game_input *Input, u32 Depth)
{
	v3 Colors[] =
	{
		{1.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, 0.0f, 1.0f},
		{1.0f, 0.0f, 1.0f},
		{0.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 0.0f},
		{1.0f, 1.0f, 1.0f},
	};

	v2 MouseP = Input->MouseP;

	f32 BorderWidth = 2.0f;
	PushRectangleOutline(Group, Min, Max, V4(0.75f, 0.75f, 0.75f, 0.75f), BorderWidth);

	v2 ButtonGap = V2(0.0f, 12.0f);
	v2 ButtonUpCenter = V2(Max.x, Min.y) + ButtonGap;
	v2 ButtonDownCenter = ButtonUpCenter + ButtonGap;
	v2 ButtonDim = V2(10.0f, 10.0f);
	rectangle2 DepthUpButton = Rectangle2CenterDim(ButtonUpCenter, ButtonDim);
	rectangle2 DepthDownButton = Rectangle2CenterDim(ButtonDownCenter, ButtonDim);
	v4 ButtonUpColor = V4(1,1,0,1);
	v4 ButtonUpColorPressed = V4(0.5f*ButtonUpColor.xyz, 1.0f);
	v4 ButtonDownColor = V4(1,0,1,1);
	v4 ButtonDownColorPressed = V4(0.5f*ButtonDownColor.xyz, 1.0f);

	ui_interaction DepthUpInteraction = {};
	DepthUpInteraction.Type = UI_Interaction_DepthChange;
	DepthUpInteraction.DepthChange = 1;

	ui_interaction DepthDownInteraction = {};
	DepthDownInteraction.Type = UI_Interaction_DepthChange;
	DepthDownInteraction.DepthChange = -1;

	b32 UpSelected = ButtonInteractionSelected(&State->DebugUI, DepthUpButton, DepthUpInteraction, Input);
	b32 DownSelected = ButtonInteractionSelected(&State->DebugUI, DepthDownButton, DepthDownInteraction, Input);

	PushQuad(Group, DepthUpButton.Min, DepthUpButton.Max, UpSelected ? ButtonUpColorPressed : ButtonUpColor);
	PushQuad(Group, DepthDownButton.Min, DepthDownButton.Max, DownSelected ? ButtonDownColorPressed : ButtonDownColor);

	TemporaryClipRectChange(Group, {V2ToV2i(Min), V2ToV2i(Max)});
	PushQuad(Group, Min, Max, V4(0.1f, 0.1f, 0.1f, 0.5f));

	debug_time_block *FrameBlock = State->FrameBlockTimings[FrameIndex];

	v2 GraphMin = Min;
	v2 GraphMax = Max;
	v2 GraphDim = GraphMax - GraphMin;
	f32 BarWidth = GraphDim.y;
	f32 OutlineWidth = 1.0f;
	u64 FrameStart = FrameBlock->TickStart;
	f32 MaxHeight = GraphDim.x;
	v2 AtMin = GraphMin;

	if(FrameBlock && FrameBlock->TickEnd)
	{
		u64 LongestFrameTime = (FrameBlock->TickEnd - FrameBlock->TickStart);

		u32 StackSize = 0;
		debug_time_block *BlockStack[1024] = {};
		BlockStack[StackSize++] = FrameBlock->FirstChild;

		while(StackSize)
		{
			debug_time_block *Block = BlockStack[--StackSize];

			do
			{
				if(Block->Depth == Depth)
				{
					u64 CycleCount = (Block->TickEnd - Block->TickStart);
					u64 CycleStart = (Block->TickStart - FrameStart);
					f32 RectHeight = ((f32)CycleCount/(f32)LongestFrameTime)*MaxHeight;
					f32 RectStartY = ((f32)CycleStart/(f32)LongestFrameTime)*MaxHeight;
					f32 RectWidth = BarWidth/State->ThreadCount;
					f32 RectStartX = RectWidth*(State->ThreadCount - GetThreadIndex(State, Block->ThreadID) - 1);
					debug_parsed_guid *Parsed = &Block->ID.Parsed;

					if(RectWidth > 0.1f)
					{
						v2 Dim = V2(RectWidth, RectHeight);
						v3 Color = Colors[(Parsed->HashValue + Block->Depth) % ArrayCount(Colors)];

						rectangle2 DrawRect = {};
						DrawRect.Min = AtMin + V2(RectStartY, RectStartX);
						DrawRect.Max = DrawRect.Min + V2(Dim.y, Dim.x);

						PushRectangleOutline(Group, DrawRect.Min, DrawRect.Max, V4(Color, 1.0f), OutlineWidth);
						PushQuad(Group, DrawRect.Min, DrawRect.Max, V4(Color, 0.50f));

						if(InsideRectangle(DrawRect, MouseP))
						{
							Tooltipf(&State->DebugUI, "%s : %.3f ms",
								WrapZ(Parsed->Name),
								(f32)CycleCount/(f32)State->CountsPerSecond);
						}
					}
				}
				else
				{
					if(Block->FirstChild)
					{
						Assert(StackSize < ArrayCount(BlockStack));
						BlockStack[StackSize++] = Block->FirstChild;
					}
				}

				Block = Block->Next;
			} while(Block != Block->Parent->FirstChild);
		}
	}
}

internal void
DebugDrawFrameTimings(debug_state *State, render_group *Group, v2 Min, v2 Max, game_input *Input)
{
	v2 MouseP = Input->MouseP;

	v2 GraphMin = Min;
	v2 GraphMax = Max;
	v2 GraphDim = GraphMax - GraphMin;
	f32 Spacing = 0.0f;
	f32 BarWidthAndSpacing = GraphDim.x / ArrayCount(State->FrameBlockTimings);
	f32 BarWidth = BarWidthAndSpacing - Spacing;
	Assert(BarWidth > 0.0f);

	f32 BorderWidth = 2.0f;
	PushRectangleOutline(Group, Min, Max, V4(0.75f, 0.75f, 0.75f, 0.75f), BorderWidth);

	TemporaryClipRectChange(Group, {V2ToV2i(Min), V2ToV2i(Max)});
	PushQuad(Group, Min, Max, V4(0.1f, 0.1f, 0.1f, 0.5f));

	u64 LongestFrameTime = 0;
	u32 FrameCount = 0;
	for(u32 FrameIndex = 0;
		FrameIndex < ArrayCount(State->FrameBlockTimings);
		++FrameIndex)
	{
		debug_time_block *FrameBlock = State->FrameBlockTimings[FrameIndex];
		if(FrameBlock)
		{
			++FrameCount;
			u64 FrameTime = (FrameBlock->TickEnd - FrameBlock->TickStart);
			if(FrameTime > LongestFrameTime)
			{
				LongestFrameTime = FrameTime;
			}
		}
	}

	v2 AtMin = GraphMin;
	for(u32 FrameIndex = 0;
		FrameIndex < ArrayCount(State->FrameBlockTimings);
		++FrameIndex)
	{
		debug_time_block *FrameBlock = State->FrameBlockTimings[FrameIndex];
		if(FrameBlock && FrameBlock->TickEnd)
		{
			u64 FrameTime = (FrameBlock->TickEnd - FrameBlock->TickStart);
			f32 Height = GraphDim.y * (f32) FrameTime / (f32) LongestFrameTime;
			v2 FrameRectDim = V2(BarWidth, Height);
			v2 Border = V2(1.0f, 1.0f);

			if((2*Border.x < FrameRectDim.x) && (2*Border.y < FrameRectDim.y))
			{
				rectangle2 FrameRect = Rectangle2(AtMin + Border, AtMin + FrameRectDim - Border);
				v4 Color = (State->ViewingFrame == FrameIndex) ?
					V4(1.0f, 0.5f, 0.5f, 1.0f) :
					V4(0.75f, 0.75f, 0.75f, 1.0f);
				PushQuad(Group, FrameRect.Min, FrameRect.Max, Color);
			}

			rectangle2 DisplayRect = Rectangle2(AtMin, AtMin + V2(BarWidth, GraphDim.y));

			ui_interaction FrameSelectInteraction = {};
			FrameSelectInteraction.Type = UI_Interaction_FrameSelect;
			FrameSelectInteraction.FrameSelect = FrameIndex;
			v4 SelectionColor = V4(1.0f, 0.25f, 0.25f, 1.0f);

			b32 ShowOutline = ButtonInteractionSelected(&State->DebugUI, DisplayRect, FrameSelectInteraction, Input);
			if(ShowOutline)
			{
				PushRectangleOutline(Group, DisplayRect.Min, DisplayRect.Max, SelectionColor, 3.0f);
			}
		}

		AtMin.x += BarWidthAndSpacing;
	}
}

internal void
GatherStatTimingSortEntries(debug_state *State, u32 FrameIndex,
	u32 *SortEntryCount, sort_entry *StatSortEntries)
{
	for(u32 HashIndex = 0;
		HashIndex < ArrayCount(State->StatHashTable[FrameIndex]);
		++HashIndex)
	{
		debug_time_stat *Stat = State->StatHashTable[FrameIndex][HashIndex];
		while(Stat)
		{
			Assert(*SortEntryCount < ArrayCount(State->StatHashTable[FrameIndex]));
			sort_entry *SortEntry = StatSortEntries + *SortEntryCount;
			*SortEntryCount += 1;
			SortEntry->Index = (u64)Stat;
			SortEntry->Value = (f32)(Stat->TotalTicks);

			Stat = Stat->NextInHash;
		}
	}
}

internal void
DebugDisplayTopClocks(debug_state *State, render_group *Group, u32 FrameIndex, v2 Min, v2 Max, v2 MouseP)
{
	u32 SortEntryCount = 0;
	// TODO: Hopefully this is enough space.
	sort_entry StatSortEntries[ArrayCount(State->StatHashTable[FrameIndex])] = {};
	GatherStatTimingSortEntries(State, FrameIndex, &SortEntryCount, StatSortEntries);
	Sort(&State->TempRegion, StatSortEntries, SortEntryCount);

	f32 BorderWidth = 2.0f;
	PushRectangleOutline(Group, Min, Max, V4(0.75f, 0.75f, 0.75f, 0.75f), BorderWidth);

	TemporaryClipRectChange(Group, {V2ToV2i(Min), V2ToV2i(Max)});
	PushQuad(Group, Min, Max, V4(0.1f, 0.1f, 0.1f, 0.5f));

	ui_layout Layout = BeginUILayout(&State->DebugUI, Asset_LiberationMonoFont, 16.0f, V4(0.0f, 1.0f, 1.0f, 1.0f), Rectangle2(Min, Max));

	u32 PercentColumnWidth = 7;
	u32 NameColumnWidth = 32;
	u32 TotalMSColumnWidth = 8;
	u32 CountColumnWidth = 6;
	u32 AverageMSColumnWidth = 14;

	BeginRow(&Layout);
	v4 OldColor = Layout.Color;
	Layout.Color = V4(1.0f, 1.0f, 0.0f, 1.0f);
	Labelf(&Layout, "%*s:%-*s:%*s:%*s:%*s",
		PercentColumnWidth, WrapZ("%"),
		NameColumnWidth, WrapZ("Name"),
		TotalMSColumnWidth, WrapZ("Total"),
		CountColumnWidth, WrapZ("Count"),
		AverageMSColumnWidth, WrapZ("Average"));
	EndRow(&Layout);
	Layout.Color = OldColor;

	debug_time_block *FrameBlock = State->FrameBlockTimings[FrameIndex];
	u64 FrameTime = (FrameBlock->TickEnd - FrameBlock->TickStart);

	for(s32 Index = SortEntryCount - 1;
		Index >= 0;
		--Index)
	{
		sort_entry *SortEntry = StatSortEntries + Index;
		debug_time_stat *Stat = PointerFromU32(debug_time_stat, SortEntry->Index);
		debug_parsed_guid *Parsed = &Stat->ID.Parsed;

		f32 TotalMS = (f32)Stat->TotalTicks/(f32)State->CountsPerSecond;
		f32 AverageMS = TotalMS/Stat->Count;
		f32 PercentOfFrame = 100.0f*(f32)Stat->TotalTicks / (f32)FrameTime;

		BeginRow(&Layout);
		if(Hoverf(&Layout, "%%%-*.2f%-*s:%*.3f:%*d:%*.5f",
				  PercentColumnWidth, PercentOfFrame,
				  NameColumnWidth, WrapZ(Parsed->Name),
				  TotalMSColumnWidth, TotalMS,
				  CountColumnWidth, Stat->Count,
				  AverageMSColumnWidth, AverageMS))
		{
			f32 MinMS = (f32)Stat->MinTicks/(f32)State->CountsPerSecond;
			f32 MaxMS = (f32)Stat->MaxTicks/(f32)State->CountsPerSecond;
			Tooltipf(&State->DebugUI, "%s L:%d, %f - %f",
					 WrapZ(Parsed->Filename),
					 Parsed->LineNumber,
					 MinMS,
					 MaxMS);
		}
		EndRow(&Layout);
	}
	EndUILayout(&Layout);
}

internal void
DebugDisplayMemoryRegion(debug_state *State, render_group *Group, memory_region *Region, v2 Min, v2 Max, v2 MouseP, debug_id ID)
{
	region_stats RegionStats = GetRegionStats(Region);

	f32 BorderWidth = 2.0f;
	PushRectangleOutline(Group, Min, Max, V4(0.75f, 0.75f, 0.75f, 0.75f), BorderWidth);

	TemporaryClipRectChange(Group, {V2ToV2i(Min), V2ToV2i(Max)});
	PushQuad(Group, Min, Max, V4(0.1f, 0.1f, 0.1f, 0.5f));

	f32 Height = Max.y - Min.y;
	f32 TotalWidth = (Max.x - Min.x);
	v2 At = V2(Max.x, Min.y);

	platform_memory_block *Block = Region->CurrentBlock;
	while(Block)
	{
		f32 BlockWidth = TotalWidth * (f32)Block->Size/(f32)RegionStats.TotalSize;
		f32 UsedWidth = BlockWidth*(f32)Block->Used/(f32)Block->Size;
		At.x -= BlockWidth;
		rectangle2 UsedRect = Rectangle2(At, At + V2(UsedWidth, Height));
		rectangle2 BlockRect = Rectangle2(At, At + V2(BlockWidth, Height));

		PushQuad(Group, UsedRect.Min, UsedRect.Max, V4(1,1,0,1.0f));
		PushRectangleOutline(Group, BlockRect.Min, BlockRect.Max, V4(1,0,0,1), 1.0f);

		if(InsideRectangle(BlockRect, MouseP))
		{
			Tooltipf(&State->DebugUI, "%s : %d / %d",
				WrapZ(ID.Parsed.Name),
				Block->Used,
				Block->Size);
		}

		Block = Block->BlockPrev;
	}
}

internal void
DisplayTexture(debug_state *State, render_group *Group, debug_stored_event *StoredEvent, v2 Min, v2 Max)
{
	f32 BorderWidth = 2.0f;
	PushRectangleOutline(Group, Min, Max, V4(0.75f, 0.75f, 0.75f, 0.75f), BorderWidth);
	renderer_texture DebugTexture = {};
	DebugTexture.Index = *(u32 *)StoredEvent->Value_ptr;
	v2 P = 0.5f*(Min + Max);
	v2 Dim = Max - Min;
	PushTexture(Group, P, V2(Dim.x, 0), V2(0, Dim.y), DebugTexture);
}

internal ui_interaction
GetUIElementInteraction(debug_ui_element *UIElement)
{
	ui_interaction Result = {};
	Result.UIElement = UIElement;
	switch(UIElement->StoredEvent->Type)
	{
		case Event_b32:
		case Event_u8:
		case Event_u16:
		case Event_u32:
		case Event_u64:
		case Event_s8:
		case Event_s16:
		case Event_s32:
		case Event_s64:
		{
			Result.Type = UI_Interaction_ChangeValue;
		} break;

		case Event_f32:
		{
			Result.Type = UI_Interaction_DragValue;
		} break;

		default:
		{
			Result.Type = UI_Interaction_Collapse;
		}
	}

	return Result;
}

internal void
DisplayResizeableUIElement(debug_state *State, render_group *Group, debug_ui_element *UIElement, ui_layout *Layout,
	game_input *Input, debug_stored_event *StoredEvent)
{
	debug_event_type Type = StoredEvent->Type;
	v2 MouseP = Input->MouseP;

	f32 FontAscent = GetFontAscent(Group, Layout->Font, Layout->LineHeight);
	v2 TopLeft = Layout->AtY + V2(Layout->IndentWidth, FontAscent);

	v2 Dimension = Dim(UIElement->DisplayRect);
	if(Dimension == V2(0,0))
	{
		switch(Type)
		{
			case Event_TopClocksList: {UIElement->DisplayRect = Rectangle2(TopLeft - V2(0.0f, 400.0f), TopLeft + V2(1600.0f, 0.0f));} break;
			case Event_FrameSlider: {UIElement->DisplayRect = Rectangle2(TopLeft - V2(0.0f, 100.0f), TopLeft + V2(1600.0f, 0.0f));} break;
			case Event_FrameBarGraph: {UIElement->DisplayRect = Rectangle2(TopLeft - V2(0.0f, 160.0f), TopLeft + V2(1600.0f, 0.0f));} break;
			case Event_AABBTree: {UIElement->DisplayRect = Rectangle2(TopLeft - V2(0.0f, 400.0f), TopLeft + V2(400.0f, 0.0f));} break;
			case Event_memory_region: {UIElement->DisplayRect = Rectangle2(TopLeft - V2(0.0f, 50.0f), TopLeft + V2(800.0f, 0.0f));} break;
			case Event_Texture: {UIElement->DisplayRect = Rectangle2(TopLeft - V2(0.0f, 256.0f), TopLeft + V2(256.0f, 0.0f));} break;
			InvalidDefaultCase;
		}
	}
	else
	{
		UIElement->DisplayRect.Min = TopLeft - V2(0.0f, Dimension.y);
		UIElement->DisplayRect.Max = TopLeft + V2(Dimension.x, 0.0f);
	}

	v2 ResizeButtonP = UIElement->DisplayRect.Min + V2(Dimension.x, 0.0f);
	v2 RectMin = UIElement->DisplayRect.Min;
	v2 RectMax = UIElement->DisplayRect.Max;

	switch(Type)
	{
		case Event_TopClocksList:
		{
			DebugDisplayTopClocks(State, Group, State->ViewingFrame, RectMin, RectMax, MouseP);
		} break;

		case Event_FrameSlider:
		{
			DebugDrawFrameTimings(State, Group, RectMin, RectMax, Input);
		} break;

		case Event_FrameBarGraph:
		{
			DebugDrawSingleFrameTimings(State, Group, State->ViewingFrame, RectMin, RectMax, Input, State->ViewingDepth);
		} break;

		case Event_AABBTree:
		{
			if(State->GameState->CurrentMode == GameMode_WorldMode)
			{
				DEBUGDisplayAABBTreeDiagram(&State->GameState->WorldMode->Physics.DynamicAABBTree, Group, RectMin, RectMax);
			}
		} break;

		case Event_memory_region:
		{
			DebugDisplayMemoryRegion(State, Group, (memory_region *)StoredEvent->Value_ptr, RectMin, RectMax, MouseP,
				StoredEvent->ID);
		} break;

		case Event_Texture:
		{
			DisplayTexture(State, Group, StoredEvent, RectMin, RectMax);
		} break;

		InvalidDefaultCase;
	}

	Layout->AtY.y -= ((RectMax.y - RectMin.y));

	v2 ResizeButtonDim = V2(10.0f, 10.0f);
	rectangle2 ResizeRect = Rectangle2CenterDim(ResizeButtonP, ResizeButtonDim);
	v4 ResizeColor = V4(1,1,1,1);
    v4 ResizeColorOn = V4(1.0f, 0.25f, 0.25f, 1.0f);

	ui_interaction ResizeInteraction = {};
	ResizeInteraction.Type = UI_Interaction_Resize;
	ResizeInteraction.UIElement = UIElement;
	b32 Selected = ButtonInteractionSelected(&State->DebugUI, ResizeRect, ResizeInteraction, Input);
	PushQuad(Group, ResizeRect.Min, ResizeRect.Max, Selected ? ResizeColorOn : ResizeColor);
	if(Selected)
	{
		Tooltip(&State->DebugUI, WrapZ("Resize"));
	}
}

internal void
FreeStoredEvent(debug_state *State, debug_stored_event *StoredEvent)
{
	if(StoredEvent)
	{
		StoredEvent->NextFree = State->FirstFreeStoredEvent;
		State->FirstFreeStoredEvent = StoredEvent;
	}
}

internal void
FreeUIElementRecursive(debug_state *State, debug_ui_element *UIElement)
{
	if(UIElement)
	{
		if(UIElement->FirstChild)
		{
			FreeUIElementRecursive(State, UIElement->FirstChild);
		}

		debug_ui_element *NextUIElement = (UIElement == UIElement->Next) ? 0 : UIElement->Next;

		DLIST_REMOVE(UIElement);
		UIElement->NextFree = State->FirstFreeUIElement;
		State->FirstFreeUIElement = UIElement;

		FreeStoredEvent(State, UIElement->StoredEvent);
		FreeUIElementRecursive(State, NextUIElement);
	}
}

internal void
FreeUIElement(debug_state *State, debug_ui_element *UIElement)
{
	if(UIElement)
	{
		if(UIElement->FirstChild)
		{
			FreeUIElementRecursive(State, UIElement->FirstChild);
			UIElement->FirstChild = 0;
		}

		if(UIElement->Parent)
		{
			if(UIElement->Parent->FirstChild == UIElement)
			{
				UIElement->Parent->FirstChild = UIElement->Next;
			}
		}

		DLIST_REMOVE(UIElement);
		UIElement->NextFree = State->FirstFreeUIElement;
		State->FirstFreeUIElement = UIElement;

		FreeStoredEvent(State, UIElement->StoredEvent);
	}
}

internal void
DebugDisplayUIElement(debug_state *State, render_group *Group, debug_ui_element *UIElement,
	ui_layout *Layout, game_input *Input, u32 FrameIndex, b32 IsRoot = false)
{
	v2 MouseP = Input->MouseP;

	if(UIElement)
	{
		s32 FrameLifetime = 16;
		s32 FrameDelta = FrameIndex - UIElement->FrameIndex;
		if(UIElement->FrameIndex > FrameIndex)
		{
			FrameDelta += ArrayCount(State->FrameBlockTimings);
		}

		if(IsRoot || (FrameDelta < FrameLifetime))
		{
			debug_parsed_guid *Parsed = &UIElement->StoredEvent->ID.Parsed;
			string MenuItemText = {};
			char Buffer[256];
			switch(UIElement->StoredEvent->Type)
			{
				case Event_b32:
				{
					MenuItemText = FormatString(Buffer, ArrayCount(Buffer), "%s: %s", WrapZ(Parsed->Name), *(b32 *)UIElement->StoredEvent->Value_ptr ? WrapZ("true") : WrapZ("false"));
				} break;

				case Event_f32:
				{
					MenuItemText = FormatString(Buffer, ArrayCount(Buffer), "%s: %f", WrapZ(Parsed->Name), *(f32 *)UIElement->StoredEvent->Value_ptr);
				} break;

				case Event_char:
				{
					MenuItemText = FormatString(Buffer, ArrayCount(Buffer), "%s: %s", WrapZ(Parsed->Name), WrapZ((char *)UIElement->StoredEvent->Value_ptr));
				} break;

				case Event_u8:
				case Event_u16:
				case Event_u32:
				{
					MenuItemText = FormatString(Buffer, ArrayCount(Buffer), "%s: %d", WrapZ(Parsed->Name), *(u32 *)UIElement->StoredEvent->Value_ptr);
				} break;

				case Event_u64:
				{
					MenuItemText = FormatString(Buffer, ArrayCount(Buffer), "%s: %d", WrapZ(Parsed->Name), SafeTruncateU64ToU32(*(u64 *)UIElement->StoredEvent->Value_ptr));
				} break;

				case Event_v2:
				{
					v2 Vector2 = *(v2 *)UIElement->StoredEvent->Value_ptr;
					MenuItemText = FormatString(Buffer, ArrayCount(Buffer), "%s: (%f, %f)", WrapZ(Parsed->Name), Vector2.x, Vector2.y);
				} break;

				case Event_v2i:
				{
					v2i Vector2 = *(v2i *)UIElement->StoredEvent->Value_ptr;
					MenuItemText = FormatString(Buffer, ArrayCount(Buffer), "%s: (%d, %d)", WrapZ(Parsed->Name), Vector2.x, Vector2.y);
				} break;

				case Event_v3:
				{
					v3 Vector3 = *(v3 *)UIElement->StoredEvent->Value_ptr;
					MenuItemText = FormatString(Buffer, ArrayCount(Buffer), "%s: (%f, %f, %f)", WrapZ(Parsed->Name), Vector3.x, Vector3.y, Vector3.z);
				} break;

				case Event_v3i:
				{
					v3i Vector3 = *(v3i *)UIElement->StoredEvent->Value_ptr;
					MenuItemText = FormatString(Buffer, ArrayCount(Buffer), "%s: (%d, %d, %d)", WrapZ(Parsed->Name), Vector3.x, Vector3.y, Vector3.z);
				} break;

				case Event_v3u:
				{
					v3u Vector3 = *(v3u *)UIElement->StoredEvent->Value_ptr;
					MenuItemText = FormatString(Buffer, ArrayCount(Buffer), "%s: (%u, %u, %u)", WrapZ(Parsed->Name), Vector3.x, Vector3.y, Vector3.z);
				} break;

				case Event_v4:
				{
					v4 Vector4 = *(v4 *)UIElement->StoredEvent->Value_ptr;
					MenuItemText = FormatString(Buffer, ArrayCount(Buffer), "%s: (%f, %f, %f, %f)", WrapZ(Parsed->Name), Vector4.x, Vector4.y, Vector4.z, Vector4.w);
				} break;

				case Event_mat2:
				{
					mat2 Matrix2 = *(mat2 *)UIElement->StoredEvent->Value_ptr;
					MenuItemText = FormatString(Buffer, ArrayCount(Buffer), "%s: [%f %f], [%f %f]", WrapZ(Parsed->Name), Matrix2.M[0], Matrix2.M[1], Matrix2.M[2], Matrix2.M[3]);
				} break;

				case Event_mat3:
				{
					mat3 Matrix3 = *(mat3 *)UIElement->StoredEvent->Value_ptr;
					MenuItemText = FormatString(Buffer, ArrayCount(Buffer), "%s: [%f %f %f], [%f %f %f], [%f %f %f]", WrapZ(Parsed->Name), Matrix3.M[0], Matrix3.M[1], Matrix3.M[2], Matrix3.M[3], Matrix3.M[4], Matrix3.M[5], Matrix3.M[6], Matrix3.M[7], Matrix3.M[8]);
				} break;

				case Event_mat4:
				{
					mat4 Matrix4 = *(mat4 *)UIElement->StoredEvent->Value_ptr;
					MenuItemText = FormatString(Buffer, ArrayCount(Buffer), "%s: [%f %f %f %f], [%f %f %f %f], [%f %f %f %f], [%f %f %f %f]", WrapZ(Parsed->Name), Matrix4.M[0], Matrix4.M[1], Matrix4.M[2], Matrix4.M[3], Matrix4.M[4], Matrix4.M[5], Matrix4.M[6], Matrix4.M[7], Matrix4.M[8], Matrix4.M[9], Matrix4.M[10], Matrix4.M[11], Matrix4.M[12], Matrix4.M[13], Matrix4.M[14], Matrix4.M[15]);
				} break;

				case Event_rectangle2:
				{
					rectangle2 Rect = *(rectangle2 *)UIElement->StoredEvent->Value_ptr;
					MenuItemText = FormatString(Buffer, ArrayCount(Buffer), "%s: Min(%f, %f), Max:(%f, %f)", WrapZ(Parsed->Name), Rect.Min.x, Rect.Min.y, Rect.Max.x, Rect.Max.y);
				} break;

				case Event_rectangle3:
				{
					rectangle3 Rect = *(rectangle3 *)UIElement->StoredEvent->Value_ptr;
					MenuItemText = FormatString(Buffer, ArrayCount(Buffer), "%s: Min(%f, %f, %f), Max:(%f, %f, %f)", WrapZ(Parsed->Name), Rect.Min.x, Rect.Min.y, Rect.Min.z, Rect.Max.x, Rect.Max.y, Rect.Max.z);
				} break;

				case Event_quaternion:
				{
					quaternion Q = *(quaternion *)UIElement->StoredEvent->Value_ptr;
					MenuItemText = FormatString(Buffer, ArrayCount(Buffer), "%s: %fr + %fi + %fj + %fk)", WrapZ(Parsed->Name), Q.r, Q.i, Q.j, Q.k);
				} break;

				case Event_void:
				{
					u32 DisplayPtr = U32FromPointer(UIElement->StoredEvent->Value_ptr);
					MenuItemText = FormatString(Buffer, ArrayCount(Buffer), "%s: %x", WrapZ(Parsed->Name), DisplayPtr);
				} break;

				case Event_TopClocksList:
				case Event_FrameBarGraph:
				case Event_FrameSlider:
				case Event_AABBTree:
				case Event_memory_region:
				case Event_Texture:
				{
					DisplayResizeableUIElement(State, Group, UIElement, Layout, Input, UIElement->StoredEvent);
				} break;

				default:
				{
					MenuItemText = FormatString(Buffer, ArrayCount(Buffer), "%s", WrapZ(Parsed->Name));
				}
			}

			if(MenuItemText.Length)
			{
				BeginRow(Layout);
				rectangle2 TextRect = GetTextBounds(Group, Layout->Font, Layout->AtX, Layout->LineHeight, MenuItemText);
				v4 OldColor = Layout->Color;

				ui_interaction Interaction = GetUIElementInteraction(UIElement);
				if(InsideRectangle(TextRect, MouseP))
				{
					State->DebugUI.HotInteraction = Interaction;
					if(State->DebugUI.NextInteraction.Type == UI_Interaction_None)
					{
						Layout->Color = V4(1.0f, 0.25f, 0.25f, 1.0f);
					}
					if(Input->KeyStates[Key_LeftClick].WasPressed)
					{
						State->DebugUI.NextInteraction = Interaction;
						UIElement->P = MouseP;
					}
				}
				if(State->DebugUI.NextInteraction == Interaction)
				{
					Layout->Color = V4(1.0f, 0.25f, 0.25f, 1.0f);
				}

				Label(Layout, MenuItemText);
				EndRow(Layout);
				Layout->Color = OldColor;
			}

			if(UIElement->Open && UIElement->FirstChild)
			{
				BeginIndent(Layout);
				DebugDisplayUIElement(State, Group, UIElement->FirstChild, Layout, Input, FrameIndex);
				EndIndent(Layout);
			}

			if(UIElement->Parent && UIElement->Next != UIElement->Parent->FirstChild)
			{
				DebugDisplayUIElement(State, Group, UIElement->Next, Layout, Input, FrameIndex);
			}
		}
		else
		{
			if(UIElement->Parent && UIElement->Next != UIElement->Parent->FirstChild)
			{
				DebugDisplayUIElement(State, Group, UIElement->Next, Layout, Input, FrameIndex);
			}

			FreeUIElement(State, UIElement);
		}
	}
}

internal void
DebugStart(debug_state *State, render_group *Group, game_input *Input)
{
	BeginFrameUI(&State->DebugUI, Group, Input);
}

internal void
DebugInteract(debug_state *State, ui_state *UI, game_input *Input)
{
	if(Input->KeyStates[Key_LeftClick].WasReleased)
	{
		if(UI->NextInteraction == UI->HotInteraction)
		{
			ui_interaction *Interaction = &UI->NextInteraction;
			debug_ui_element *UIElement = Interaction->UIElement;

			switch(Interaction->Type)
			{
				case UI_Interaction_Collapse:
				{
					UIElement->Open = !UIElement->Open;
				} break;

				case UI_Interaction_ChangeValue:
				{
					switch(UIElement->StoredEvent->Type)
					{
						case Event_b32:
						{
							*(b32 *)UIElement->StoredEvent->Value_ptr = !*(b32 *)UIElement->StoredEvent->Value_ptr;
						} break;

						case Event_u8:
						case Event_u16:
						case Event_u32:
						case Event_u64:
						{
							if(Input->KeyStates[Key_Shift].Pressed)
							{
								u32 *U32Value = (u32 *)UIElement->StoredEvent->Value_ptr;
								if(*U32Value != 0)
								{
									--(*U32Value);
								}
							}
							else
							{
								(*(u32 *)UIElement->StoredEvent->Value_ptr)++;
							}
						} break;

						case Event_s8:
						case Event_s16:
						case Event_s32:
						case Event_s64:
						{
							if(Input->KeyStates[Key_Shift].Pressed)
							{
								(*(s32 *)UIElement->StoredEvent->Value_ptr)--;
							}
							else
							{
								(*(s32 *)UIElement->StoredEvent->Value_ptr)++;
							}
						} break;

						InvalidDefaultCase;
					}
				} break;

				case UI_Interaction_FrameSelect:
				{
					State->ViewingFrame = Interaction->FrameSelect;
				} break;

				case UI_Interaction_DepthChange:
				{
					s32 NewDepth = State->ViewingDepth + Interaction->DepthChange;
					if(NewDepth < 0) {NewDepth = 0;}
					State->ViewingDepth = NewDepth;
				} break;
			}
		}
	}

	if(Input->KeyStates[Key_LeftClick].Pressed)
	{
		debug_ui_element *UIElement = UI->NextInteraction.UIElement;

		switch(UI->NextInteraction.Type)
		{
			case UI_Interaction_DragValue:
			{
				f32 DragScale = 0.001f;
				*(f32 *)UIElement->StoredEvent->Value_ptr += DragScale*(Input->MouseP.y - UIElement->P.y);
				UIElement->P = Input->MouseP;
			} break;

			case UI_Interaction_Resize:
			{
				rectangle2 *DisplayRect = &UIElement->DisplayRect;
				v2 ResizeButtonP = Input->MouseP;
				if(ResizeButtonP.x < DisplayRect->Min.x) {ResizeButtonP.x = DisplayRect->Min.x;}
				if(ResizeButtonP.y > DisplayRect->Max.y) {ResizeButtonP.y = DisplayRect->Max.y;}
				DisplayRect->Min.y = ResizeButtonP.y;
				DisplayRect->Max.x = ResizeButtonP.x;
			} break;
		}
	}
}

internal void
DebugEnd(debug_state *State, render_group *Group, game_input *Input)
{
	DebugInteract(State, &State->DebugUI, Input);

	EndFrameUI(&State->DebugUI);
}

internal debug_state *
DebugInit(game_memory *Memory, u64 CountsPerSecond)
{
	debug_state *DebugState = Memory->DebugState = BootstrapPushStruct(debug_state, Region);

	DebugState->CountsPerSecond = CountsPerSecond;

	DebugState->GameState = Memory->GameState;

	debug_stored_event *RootDummy = PushStruct(&DebugState->Region, debug_stored_event);
	RootDummy->ID = CreateIDFromGUID("a|0|0|RootDummy");
	DebugState->RootUIElement.StoredEvent = RootDummy;

	DebugState->RootUIElement.Open = false;
	DLIST_INIT(&DebugState->RootUIElement);
	DebugState->LastOpenUIElement = &DebugState->RootUIElement;

	u32 MainThreadID = GetThreadID();
	DebugState->ThreadIndexes[DebugState->ThreadCount++] = MainThreadID;

	DebugState->ViewingDepth = 1;
	return DebugState;
}

extern "C" DEBUG_FRAME_END(DebugFrameEnd)
{
	game_state *GameState = Memory->GameState;
	if(GameState)
	{
		debug_state *DebugState = Memory->DebugState;
		if(!DebugState)
		{
			DebugState = DebugInit(Memory, CountsPerSecond);
		}

		if(Input->KeyStates[Key_F9].WasPressed)
		{
			DebugState->PauseCollation = !DebugState->PauseCollation;
		}

		v2i ScreenResolution = Input->ScreenResolution;
		DebugState->ScreenCamera = DefaultScreenCamera(ScreenResolution);

		assets *DebugAssets = GameState->Assets;
		render_group RenderGroup_ = BeginRender(DebugAssets, RenderCommands, Target_Overlay, {}, Render_ClearDepth);
		render_group *RenderGroup = &RenderGroup_;
		SetCamera(RenderGroup, &DebugState->ScreenCamera);
		SetRenderFlags(RenderGroup, RenderFlag_NotLighted|RenderFlag_TopMost|RenderFlag_AlphaBlended|RenderFlag_SDF);

		DebugStart(DebugState, RenderGroup, Input);

		debug_table *DebugTable = Memory->DebugTable;
		u32 ArrayIndex = DebugTable->ArrayIndex;
		u32 EventCount = DebugTable->EventCount;
		debug_event *EventArray = DebugTable->DebugEvents[ArrayIndex];
		if(!DebugState->PauseCollation)
		{
			CollateDebugRecords(DebugState, EventArray, EventCount);
		}
		DebugTable->ArrayIndex = (DebugTable->ArrayIndex + 1) % ArrayCount(DebugTable->DebugEvents);
		DebugTable->EventCount = 0;

		f32 FrameFPS = 1.0f/Input->LastFramedt_;
		f32 FrameTimeMS = Input->LastFramedt_*1000;
		f32 SimFPS = 1.0f / Input->Simdt;
		debug_parsed_guid *RootParsed = &DebugState->RootUIElement.StoredEvent->ID.Parsed;
		FormatString(RootParsed->Name, ArrayCount(RootParsed->Name),
			"%.2fms : FPS %.2f : SimFPS %.2f : %d Blocks : %d/%d",
			FrameTimeMS, FrameFPS, SimFPS,
			MemStats->BlockCount, (u32)MemStats->TotalUsed, (u32)MemStats->TotalSize);

		v2 At = V2(0.0f, (f32)ScreenResolution.y);
		ui_layout Layout = BeginUILayout(&DebugState->DebugUI, Asset_LiberationMonoFont, 18.0f, V4(0.0f, 1.0f, 1.0f, 1.0f),
			Rectangle2(At, At));
		DebugDisplayUIElement(DebugState, RenderGroup, &DebugState->RootUIElement, &Layout, Input, DebugState->CurrentFrame, true);
		EndUILayout(&Layout);

		DebugEnd(DebugState, RenderGroup, Input);

		EndRender(RenderGroup);

		if(GameState->CurrentMode == GameMode_WorldMode)
		{
			// NOTE: Physics debug display.
			game_mode_world *WorldMode = GameState->WorldMode;

			RenderGroup_ = BeginRender(DebugAssets, RenderCommands, Target_Overlay, {}, Render_ClearDepth);
			RenderGroup = &RenderGroup_;
			SetCamera(RenderGroup, &WorldMode->WorldCamera);
			SetRenderFlags(RenderGroup, RenderFlag_NotLighted|RenderFlag_TopMost);

			DEBUGDisplayDynamicAABBTreeRegions(&WorldMode->Physics, RenderGroup);
			DEBUGDisplayStaticAABBTreeRegions(&WorldMode->Physics, RenderGroup);
			DEBUGDisplayContactInfo(&WorldMode->Physics, RenderGroup);

			EndRender(RenderGroup);
		}

		CheckMemory(&DebugState->Region);
		CheckMemory(&DebugState->TempRegion);

		{DEBUG_DATA_BLOCK("Memory");
			DEBUG_VALUE(&DebugState->Region);
			DEBUG_VALUE(&DebugState->TempRegion);
		}
	}
}
