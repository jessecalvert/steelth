/*@H
* File: render_test.h
* Author: Jesse Calvert
* Created: October 19, 2018, 11:41
* Last modified: April 10, 2019, 17:31
*/

#pragma once

#include "steelth_types.h"
#include "steelth_shared.h"
#include "steelth_platform.h"
#include "steelth_debug_interface.h"
#include "steelth_memory.h"
#include "steelth_stream.h"
#include "steelth_math.h"
#include "steelth_random.h"

#include "steelth_renderer.h"

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH

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
};
CTAssert(sizeof(win32_memory_block) == 64);

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
};

enum test_scene_element
{
	Element_Ground,
	Element_Wall,
	Element_Tree,

	Element_Count,
};
#define TEST_SCENE_DIM 32
struct test_scene
{
	test_scene_element Elements[TEST_SCENE_DIM][TEST_SCENE_DIM];

	renderer_texture Smile;
	renderer_texture Heart;
	renderer_texture Sword;

	camera Camera;
};

struct render_test_state
{
	memory_region Region;

	test_scene Scene;

	b32 Initialized;
};

internal renderer_texture LoadPNG(memory_region *Region, render_memory_op_queue *RenderMemoryOpQueue, string Filename, u32 Index);
