/*@H
* File: steelth_debug_interface.h
* Author: Jesse Calvert
* Created: October 9, 2017, 11:44
* Last modified: April 17, 2019, 18:04
*/

#pragma once

#define DEBUG_FRAME_END(Name) void Name(game_memory *Memory, game_render_commands *RenderCommands, game_input *Input, u64 CountsPerSecond, memory_block_stats *MemStats)
typedef DEBUG_FRAME_END(debug_frame_end);

#if GAME_DEBUG

enum debug_event_type
{
	Event_None,

	Event_FrameMark,

	Event_BeginBlock,
	Event_EndBlock,
	Event_BeginDataBlock,
	Event_EndDataBlock,

	Event_TopClocksList,
	Event_FrameSlider,
	Event_FrameBarGraph,
	Event_Texture,

	Event_AABBTree,

	Event_char,
	Event_b32,
	Event_f32,
	Event_u8,
	Event_u16,
	Event_u32,
	Event_u64,
	Event_s8,
	Event_s16,
	Event_s32,
	Event_s64,
	Event_v2,
	Event_v2i,
	Event_v3,
	Event_v3i,
	Event_v3u,
	Event_v4,
	Event_rectangle2,
	Event_rectangle3,
	Event_mat2,
	Event_mat3,
	Event_mat4,
	Event_quaternion,
	Event_void,
	Event_string,
	Event_enum,

	Event_memory_region,

	Event_Count,
};

struct debug_event
{
	char *GUID;
	debug_event_type Type;
	u32 ThreadID;
	u64 Tick;

	union
	{
		u64 Value_u64;
		void *Value_ptr;
	};
};

struct debug_table
{
	memory_region *Region;
	ticket_mutex RegionMutex;

	b32 Recording;
	u32 ArrayIndex;
	volatile u32 EventCount;
	debug_event DebugEvents[2][4096*4096];
};
extern debug_table *GlobalDebugTable;

#define DEBUG_NAME__(A, B, C, D) A "|" #B "|" #C "|" D
#define DEBUG_NAME_(A, B, C, D) DEBUG_NAME__(A, B, C, D)
#define DEBUG_NAME(Name) DEBUG_NAME_(__FILE__, __LINE__, __COUNTER__, Name)

#define TIMED_BLOCK__(GUID, Number) timed_block TimedBlock_##Number(GUID)
#define TIMED_BLOCK_(GUID, Number) TIMED_BLOCK__(GUID, Number)
#define TIMED_BLOCK(Name) TIMED_BLOCK_(DEBUG_NAME(Name), __COUNTER__)
#define TIMED_FUNCTION() TIMED_BLOCK_(DEBUG_NAME(__FUNCTION__), __COUNTER__)

#define BEGIN_BLOCK_(GUID) {RECORD_DEBUG_EVENT(GUID, Event_BeginBlock);}
#define BEGIN_BLOCK(Name) BEGIN_BLOCK_(DEBUG_NAME(Name))
#define END_BLOCK_(GUID) {RECORD_DEBUG_EVENT(GUID, Event_EndBlock);}
#define END_BLOCK() END_BLOCK_(DEBUG_NAME("END_BLOCK"))

#define DEBUG_DATA_BLOCK__(GUID, Number) data_block DATA_BLOCK_##Number(GUID)
#define DEBUG_DATA_BLOCK_(GUID, Number) DEBUG_DATA_BLOCK__(GUID, Number)
#define DEBUG_DATA_BLOCK(Name) DEBUG_DATA_BLOCK_(DEBUG_NAME(Name), __COUNTER__)

#define BEGIN_DATA_BLOCK_(GUID) {RECORD_DEBUG_EVENT(GUID, Event_BeginDataBlock);}
#define BEGIN_DATA_BLOCK(Name) BEGIN_DATA_BLOCK_(DEBUG_NAME(Name))
#define END_DATA_BLOCK_(GUID) {RECORD_DEBUG_EVENT(GUID, Event_EndDataBlock);}
#define END_DATA_BLOCK() END_DATA_BLOCK_(DEBUG_NAME("END_DATA_BLOCK"))

#define FRAME_MARK(CycleCount) {RECORD_DEBUG_EVENT(DEBUG_NAME("FRAME_MARK"), Event_FrameMark); \
	Event->Value_u64 = CycleCount; }

#define DEBUGSetRecording(Value) GlobalDebugTable->Recording = (Value)

// TODO: Atomic add!
#define RECORD_DEBUG_EVENT(GUIDInit, TypeInit) \
	Assert(GlobalDebugTable->EventCount < ArrayCount(GlobalDebugTable->DebugEvents[0])); \
	u32 Increment = GlobalDebugTable->Recording ? 1 : 0; \
	u32 EventIndex = AtomicAddU32(&GlobalDebugTable->EventCount, Increment); \
	debug_event *Event = GlobalDebugTable->DebugEvents[GlobalDebugTable->ArrayIndex] + EventIndex; \
	Event->GUID = GUIDInit; \
	Event->Type = TypeInit; \
	Event->ThreadID = GetThreadID(); \
	Event->Tick = __rdtsc();

class data_block
{
public:
	char *GUID;
	data_block(char *GUIDInit)
	{
		GUID = GUIDInit;
		BEGIN_DATA_BLOCK_(GUID);
	}

	~data_block()
	{
		END_DATA_BLOCK_(GUID);
	}
};

class timed_block
{
public:
	char *GUID;
	timed_block(char *GUIDInit)
	{
		GUID = GUIDInit;
		BEGIN_BLOCK_(GUID);
	}

	~timed_block()
	{
		END_BLOCK_(GUID);
	}
};

#define REGISTER_DEBUG_VALUE(type) \
inline void \
RecordDebugValue_(type *ValuePtr, char *GUID, u32 Depth=0) \
{ \
	RECORD_DEBUG_EVENT(GUID, Event_##type); \
	Event->Value_ptr = ValuePtr; \
}

#define DEBUG_VALUE(ValueInit) RecordDebugValue_(ValueInit, DEBUG_NAME(#ValueInit));
#define DEBUG_VALUE_NAME(ValueInit, Name) RecordDebugValue_(ValueInit, DEBUG_NAME(Name));
#define DEBUG_VALUE_GUID(ValueInit, GUID) RecordDebugValue_(ValueInit, GUID);

REGISTER_DEBUG_VALUE(b32);
REGISTER_DEBUG_VALUE(char);
REGISTER_DEBUG_VALUE(f32);
REGISTER_DEBUG_VALUE(u8);
REGISTER_DEBUG_VALUE(u16);
REGISTER_DEBUG_VALUE(u32);
REGISTER_DEBUG_VALUE(u64);
REGISTER_DEBUG_VALUE(s8);
REGISTER_DEBUG_VALUE(s16);
// REGISTER_DEBUG_VALUE(s32);
REGISTER_DEBUG_VALUE(s64);
REGISTER_DEBUG_VALUE(v2);
REGISTER_DEBUG_VALUE(v2i);
REGISTER_DEBUG_VALUE(v3);
REGISTER_DEBUG_VALUE(v3i);
REGISTER_DEBUG_VALUE(v3u);
REGISTER_DEBUG_VALUE(v4);
REGISTER_DEBUG_VALUE(mat2);
REGISTER_DEBUG_VALUE(mat3);
REGISTER_DEBUG_VALUE(mat4);
REGISTER_DEBUG_VALUE(rectangle2);
REGISTER_DEBUG_VALUE(rectangle3);
REGISTER_DEBUG_VALUE(quaternion);
REGISTER_DEBUG_VALUE(void);
REGISTER_DEBUG_VALUE(memory_region);

#define DEBUG_UI_ELEMENT(UIType, Name) {RECORD_DEBUG_EVENT(DEBUG_NAME(Name), UIType);}
#define DEBUG_UI_ELEMENT_VALUE(UIType, Name, ValuePtr) {RECORD_DEBUG_EVENT(DEBUG_NAME(Name), UIType); Event->Value_ptr = ValuePtr;}

#else

#define TIMED_FUNCTION(...)
#define TIMED_BLOCK(...)
#define BEGIN_BLOCK(...)
#define END_BLOCK(...)
#define FRAME_MARK(...)
#define DEBUG_DATA_BLOCK(...)
#define DEBUG_B32(...)
#define DEBUG_F32(...)
#define DEBUG_UI_ELEMENT(...)
#define DEBUG_UI_ELEMENT_VALUE(...)
#define DEBUG_VALUE(...)
#define DEBUG_VALUE_NAME(...)
#endif
