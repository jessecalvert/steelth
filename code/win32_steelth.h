/*@H
* File: win32_steelth.h
* Author: Jesse Calvert
* Created: November 6, 2017, 17: 9
* Last modified: April 13, 2019, 18:28
*/

#pragma once

#include "win32_steelth_renderer.h"

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH

enum code_type
{
	CodeType_Game,

	CodeType_Count,
};

struct game_code
{
	game_update_and_render *GameUpdateAndRender;

#if GAME_DEBUG
	debug_frame_end *DEBUGFrameEnd;
#endif
};

struct hotloadable_code
{
	code_type Type;
	union
	{
		game_code Game;
	};

	HMODULE Handle;
	FILETIME LastWriteTime;

	char DLLName[WIN32_STATE_FILE_NAME_COUNT];
	char DLLNameTemp[WIN32_STATE_FILE_NAME_COUNT];
	char LockName[WIN32_STATE_FILE_NAME_COUNT];
};

enum hotloadable_file_flags
{
	FileFlag_Header = 0x1,
};
enum hotloadable_file_tag
{
	File_Shader,
	File_Assets,
	// File_DLL,
};
struct hotloadable_file
{
	char *Filename;
	flag32(hotloadable_file_flags) Flags;
	hotloadable_file_tag Tag;
	shader_type Shader;

	FILETIME LastWriteTime;
	b32 NeedsReloading;
};

struct win32_hotloader
{
	u32 FileCount;
	hotloadable_file Files[128];
};

struct platform_work_queue_entry
{
	work_queue_callback *Callback;
	void *Data;
};

struct platform_work_queue
{
	u32 volatile FirstEntry;
	u32 volatile OnePastLastEntry;

	u32 volatile WorkGoal;
	u32 volatile WorkCompleted;

	platform_work_queue_entry Entries[256];

	HANDLE SemaphoreHandle;
};

enum win32_memory_block_flags
{
	Win32Memory_AllocatedDuringLoop = 0x1,
	Win32Memory_DeallocatedDuringLoop = 0x2,
};
struct win32_memory_block
{
	platform_memory_block Block;

	win32_memory_block *Next;
	win32_memory_block *Prev;
	flag32(win32_memory_block_flags) LoopingFlags;
	u32 _Padding;
};
CTAssert(sizeof(win32_memory_block) == 64);

struct win32_saved_memory_block
{
	umm BasePtr;
	umm Size;
};

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFFER_LENGTH 512

struct win32_server_connection
{
	platform_server_connection PlatformConnection;

	SOCKET Socket;
	addrinfo *ServerAddrInfo;
};

struct win32_state
{
	SYSTEM_INFO SystemInfo;

	char EXEFullPath[WIN32_STATE_FILE_NAME_COUNT];
	string EXEDirectory;

	ticket_mutex MemoryMutex;
	win32_memory_block MemorySentinel;

	b32 LoopRecording;
	b32 LoopPlayback;
	HANDLE RecordingHandle;
	HANDLE PlaybackHandle;

	win32_hotloader Hotloader;
	hotloadable_code Code[CodeType_Count];
};
