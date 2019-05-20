/*@H
* File: steelth.h
* Author: Jesse Calvert
* Created: November 6, 2017
* Last modified: April 18, 2019, 14:30
*/

#pragma once

#include "steelth_types.h"
#include "steelth_shared.h"
#include "steelth_platform.h"
#include "steelth_debug_interface.h"

#include "steelth_intrinsics.h"
#include "steelth_simd.h"
#include "steelth_math.h"
#include "steelth_memory.h"
#include "steelth_stream.h"

#include "steelth_renderer.h"
#include "steelth_config.h"
#include "steelth_file_formats.h"
#include "steelth_assets.h"

#include "steelth_sort.h"
#include "steelth_random.h"

#define COLOR_IRON V3(0.56f, 0.57f, 0.58f)
#define COLOR_GOLD V3(1.0f, 0.71f, 0.29f)
#define COLOR_COPPER V3(0.95f, 0.64f, 0.54f)
#define COLOR_ALUMINIUM V3(0.91f, 0.92f, 0.92f)
#define COLOR_SILVER V3(0.95f, 0.93f, 0.88f)

struct task_memory
{
	b32 InUse;

	memory_region Region;
	temporary_memory MemoryFlush;
};

internal task_memory *BeginTaskWithMemory(game_state *TranState);
internal void EndTaskWithMemory(task_memory *TaskMemory);

#include "steelth_aabb.h"
#include "steelth_gjk.h"
#include "steelth_rigid_body.h"
#include "steelth_convex_hull.h"
#include "steelth_animation.h"
#include "steelth_particles.h"
#include "steelth_ui.h"

#include "steelth_world.h"
#include "steelth_world_mode.h"
#include "steelth_entity_manager.h"
#include "steelth_title_screen.h"
#include "steelth_editor.h"

enum game_mode
{
	GameMode_None,

	GameMode_WorldMode,
	GameMode_TitleScreen,

	GameMode_Count,
};

struct console_entry
{
	console_entry *Next;
	console_entry *Prev;

	string String;
	v4 Color;
};

struct console
{
	b32 Expanded;
	stream *Info;
	console_entry Sentinel;
};

enum dev_mode
{
	DevMode_None,
	DevMode_Editor,
};

struct game_state
{
	memory_region Region;
	memory_region ModeRegion;

	temporary_memory FrameRegionTemp;
	memory_region FrameRegion;

	task_memory Tasks[32];

	struct assets *Assets;

	platform_work_queue *HighPriorityQueue;
	platform_work_queue *LowPriorityQueue;

	f32 TimeScale;

	game_mode CurrentMode;
	union
	{
		game_mode_world *WorldMode;
		game_mode_title_screen *TitleScreen;
	};

	ui_state UI;
	dev_mode DevMode;
	b32 UseDebugCamera;
	console Console;
	game_editor *Editor;
};

internal void SetGameMode(game_state *GameState, game_mode Mode);

#if GAME_DEBUG
#include "generated.h"
#endif
