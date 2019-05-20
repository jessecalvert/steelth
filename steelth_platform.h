/*@H
* File: steelth_platform.h
* Author: Jesse Calvert
* Created: November 6, 2017, 17:07
* Last modified: April 25, 2019, 16:46
*/

#pragma once

enum key
{
	Key_LeftClick,
	Key_RightClick,

	Key_Minus,
	Key_Plus,
	Key_OpenBracket,
	Key_CloseBracket,
	Key_Tab,

	Key_A,
	Key_B,
	Key_C,
	Key_D,
	Key_E,
	Key_F,
	Key_G,
	Key_H,
	Key_I,
	Key_J,
	Key_K,
	Key_L,
	Key_M,
	Key_N,
	Key_O,
	Key_P,
	Key_Q,
	Key_R,
	Key_S,
	Key_T,
	Key_U,
	Key_V,
	Key_W,
	Key_X,
	Key_Y,
	Key_Z,

	Key_1,
	Key_2,
	Key_3,
	Key_4,
	Key_5,
	Key_6,
	Key_7,
	Key_8,
	Key_9,
	Key_0,

	Key_Up,
	Key_Down,
	Key_Left,
	Key_Right,
	Key_Space,
	Key_Enter,

	Key_Ctrl,
	Key_Shift,

	Key_F1,
	Key_F2,
	Key_F3,
	Key_F4,
	Key_F5,
	Key_F6,
	Key_F7,
	Key_F8,
	Key_F9,
	Key_F10,
	Key_F11,
	Key_F12,

	Key_Esc,
	Key_Tilde,

	Key_Count,
};

struct key_state
{
	b32 Pressed;
	b32 WasPressed;
	b32 WasReleased;
	u32 TransitionCount;
};

struct game_input
{
	key_state KeyStates[Key_Count];
	v2 MouseP;
	v2 dMouseP;
	f32 MouseWheelDelta;

	r32 LastFramedt_;
	r32 Simdt;

	v2i ScreenResolution;
};

struct bitmap_id
{
	u32 Value;
};

struct font_id
{
	u32 Value;
};

struct mesh_id
{
	u32 Value;
};

struct platform_server_connection
{
	void *Handle_;
};

struct game_memory
{
	struct game_state *GameState;
	struct platform *Platform;
	struct platform_work_queue *HighPriorityQueue;
	struct platform_work_queue *LowPriorityQueue;
	struct render_memory_op_queue *RenderOpQueue;

	struct stream *InfoStream;

	b32 NewAssetFile;

	platform_server_connection *ServerConnection;

#if GAME_DEBUG
	struct debug_table *DebugTable;
	struct debug_state *DebugState;
#endif
};

struct game_render_commands;
#define GAME_UPDATE_AND_RENDER(Name) void Name(game_memory *Memory, game_render_commands *RenderCommands, game_input *Input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

struct platform_file
{
	string Name;
	void *Handle_;
};

struct platform_file_list
{
	u32 Count;
	platform_file Files[32];
};

struct memory_block_stats
{
	u32 BlockCount;
	umm TotalUsed;
	umm TotalSize;
};

struct platform_memory_block
{
	platform_memory_block *BlockPrev;
	u8 *Base;
	umm Size;
	umm Used;
	flag32(region_flags) Flags;
	u32 _Padding;
};

struct memory_region;
#define LOAD_ENTIRE_FILE(Name) buffer Name(memory_region *Region, string Filename)
typedef LOAD_ENTIRE_FILE(load_entire_file);

#define LOAD_ENTIRE_FILE_AND_NULL_TERMINATE(Name) buffer Name(memory_region *Region, string Filename)
typedef LOAD_ENTIRE_FILE_AND_NULL_TERMINATE(load_entire_file_and_null_terminate);

#define LOAD_HDR_PNG(Name) f32 *Name(memory_region *Region, string Filename, u32 Width, u32 Height, u32 Channels)
typedef LOAD_HDR_PNG(load_hdr_png);

#define OPEN_ALL_FILES(Name) platform_file_list Name(string Extension)
typedef OPEN_ALL_FILES(open_all_files);

#define READ_FROM_FILE(Name) void Name(void *Buffer, platform_file *File, u32 Offset, u32 Length)
typedef READ_FROM_FILE(read_from_file);

#define WRITE_INTO_FILE(Name) void Name(void *Buffer, platform_file *File, u32 Offset, u32 Length)
typedef WRITE_INTO_FILE(write_info_file);

#define CLOSE_FILE(Name) void Name(platform_file *File)
typedef CLOSE_FILE(close_file);

#define ALLOCATE_MEMORY(Name) platform_memory_block *Name(umm Size, flag32(region_flags) Flags)
typedef ALLOCATE_MEMORY(allocate_memory);

#define DEALLOCATE_MEMORY(Name) void Name(platform_memory_block *Block)
typedef DEALLOCATE_MEMORY(deallocate_memory);

#define WORK_QUEUE_CALLBACK(Name) void Name(platform_work_queue *Queue, void *Data)
typedef WORK_QUEUE_CALLBACK(work_queue_callback);

#define ADD_WORK(Name) void Name(platform_work_queue *Queue, work_queue_callback *Callback, void *Data)
typedef ADD_WORK(add_work);

#define COMPLETE_ALL_WORK(Name) void Name(platform_work_queue *Queue)
typedef COMPLETE_ALL_WORK(complete_all_work);

#define CONNECT_TO_SERVER(Name) platform_server_connection *Name(string ServiceHost)
typedef CONNECT_TO_SERVER(connect_to_server);

#define DISCONNECT_FROM_SERVER(Name) void Name(platform_server_connection *Connection)
typedef DISCONNECT_FROM_SERVER(disconnect_from_server);

#define SEND_DATA(Name) b32 Name(platform_server_connection *Connection, u32 Size, void *Contents)
typedef SEND_DATA(send_data);

struct platform
{
	load_entire_file *LoadEntireFile;
	load_entire_file_and_null_terminate *LoadEntireFileAndNullTerminate;

	allocate_memory *AllocateMemory;
	deallocate_memory *DeallocateMemory;

	open_all_files *OpenAllFiles;
	read_from_file *ReadFromFile;
	write_info_file *WriteIntoFile;
	close_file *CloseFile;

	add_work *AddWork;
	complete_all_work *CompleteAllWork;

	connect_to_server *ConnectToServer;
	disconnect_from_server *DisconnectFromServer;
	send_data *SendData;

	load_hdr_png *LoadHDRPNG;
};

global platform *Platform;
