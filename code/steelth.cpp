/*@H
* File: steelth.cpp
* Author: Jesse Calvert
* Created: October 6, 2017, 16:31
* Last modified: April 18, 2019, 14:29
*/

#include "steelth.h"

#include "steelth_renderer.cpp"
#include "steelth_assets.cpp"
#include "steelth_asset_rendering.cpp"
#include "steelth_aabb.cpp"
#include "steelth_convex_hull.cpp"
#include "steelth_gjk.cpp"
#include "steelth_rigid_body.cpp"
#include "steelth_animation.cpp"
#include "steelth_particles.cpp"
#include "steelth_ui.cpp"
#include "steelth_entity_manager.cpp"
#include "steelth_world.cpp"
#include "steelth_world_mode.cpp"
#include "steelth_title_screen.cpp"
#include "steelth_editor.cpp"

internal task_memory *
BeginTaskWithMemory(game_state *GameState)
{
	task_memory *Result = 0;

	for(u32 Index = 0;
		Index < ArrayCount(GameState->Tasks);
		++Index)
	{
		task_memory *Task = GameState->Tasks + Index;
		if(!Task->InUse)
		{
			Assert(!Task->Region.CurrentBlock);
			Task->MemoryFlush = BeginTemporaryMemory(&Task->Region);
			Task->InUse = true;
			Result = Task;
			break;
		}
	}

	return Result;
}

internal void
EndTaskWithMemory(task_memory *TaskMemory)
{
	EndTemporaryMemory(&TaskMemory->MemoryFlush);

	CompletePreviousWritesBeforeFutureWrites;
	TaskMemory->InUse = false;
}

internal void
SetGameMode(game_state *GameState, game_mode Mode)
{
	Platform->CompleteAllWork(GameState->HighPriorityQueue);
	Platform->CompleteAllWork(GameState->LowPriorityQueue);
	Clear(&GameState->ModeRegion);
	GameState->CurrentMode = Mode;
}

internal void
ConsoleConsumeStream(memory_region *Region, console *Console, stream *Stream, v4 Color)
{
	for(stream_chunk *Chunk = Stream->First;
		Chunk;
		Chunk = Chunk->Next)
	{
		string *String = (string *)Chunk->Buffer.Contents;

		char Buffer[1024];
		b32 FirstLine = true;
		b32 Finished = false;
		char *Begin = String->Text;
		char *End = String->Text;
		while(!Finished)
		{
			if((*End == '\n') ||
			   ((End - String->Text) >= String->Length))
			{
				string ConsoleString = {};
				string StringPiece = {};
				StringPiece.Text = Begin;
				StringPiece.Length = (u32)(End - Begin);

				if(FirstLine)
				{
					ConsoleString = FormatString(Buffer, ArrayCount(Buffer), "%s(%d): %s",
						Chunk->Filename,
						Chunk->LineNumber,
						StringPiece);
					FirstLine = false;
				}
				else
				{
					ConsoleString = FormatString(Buffer, ArrayCount(Buffer), "    %s",
						StringPiece);
				}

				console_entry *Entry = PushStruct(Region, console_entry);
				Entry->String = PushString(Region, ConsoleString);
				Entry->Color = Color;
				DLIST_INSERT(&Console->Sentinel, Entry);

				Begin = End + 1;
			}

			if((End - String->Text) >= String->Length)
			{
				Finished = true;
			}
			++End;
		}

	}

	Stream->First = Stream->Last = 0;
}

internal void
ConsoleConsumeAllStreams(memory_region *Region, console *Console)
{
	ConsoleConsumeStream(Region, Console, Console->Info, V4(1,1,1,1));
	ConsoleConsumeStream(Region, Console, Console->Info->Errors, V4(1, 0.25f, 0.25f, 1.0f));
}

internal void
DisplayConsole(game_state *GameState, render_group *Group,
	console *Console, v2 Min, v2 Max, v2i ScreenResolution)
{
	SetRenderFlags(Group, RenderFlag_TopMost|RenderFlag_AlphaBlended);
	v4 BackgroundColor = V4(0.1f, 0.1f, 0.5f, 0.75f);
	PushQuad(Group, Min, Max, BackgroundColor);

	ui_layout Layout = BeginUILayout(&GameState->UI, Asset_LiberationMonoFont, 18.0f, V4(1,1,1,1), Rectangle2(Min, Max));

	TemporaryClipRectChange(Group, {V2ToV2i(Min), V2ToV2i(Max)});
	SetRenderFlags(Group, RenderFlag_SDF|RenderFlag_TopMost|RenderFlag_AlphaBlended);

	u32 EntryCount = (u32)FloorReal32ToInt32((Max.y - Min.y)/Layout.LineHeight);
	console_entry *Entry = Console->Sentinel.Next;
	u32 EmptyEntries = 0;
	for(u32 Index = 0;
		Index < EntryCount;
		++Index)
	{
		if(Entry->Next == &Console->Sentinel)
		{
			EmptyEntries = (EntryCount - Index) - 1;
			break;
		}
		else
		{
			Entry = Entry->Next;
		}
	}

	for(u32 Index = 0;
		Index < EmptyEntries;
		++Index)
	{
		BeginRow(&Layout);
		EndRow(&Layout);
	}

	for(;
		Entry != &Console->Sentinel;
		Entry = Entry->Prev)
	{
		BeginRow(&Layout);
		Layout.Color = Entry->Color;
		Label(&Layout, Entry->String);
		EndRow(&Layout);
	}

	EndUILayout(&Layout);
}

#if GAME_DEBUG
debug_table *GlobalDebugTable;
#endif

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	Platform = Memory->Platform;
#if GAME_DEBUG
	GlobalDebugTable = Memory->DebugTable;
#endif

	TIMED_FUNCTION();

	game_state *GameState = Memory->GameState;
	if(!GameState)
	{
		GameState = Memory->GameState = BootstrapPushStruct(game_state, Region);
		GameState->TimeScale = 1.0f;

		DLIST_INIT(&GameState->Console.Sentinel);
		GameState->Console.Info = Memory->InfoStream;

		GameState->HighPriorityQueue = Memory->HighPriorityQueue;
		GameState->LowPriorityQueue = Memory->LowPriorityQueue;
		GameState->Assets = InitializeAssets(GameState, Megabytes(32), Memory->RenderOpQueue, Memory->InfoStream);
	}

	if(Memory->NewAssetFile)
	{
		NotImplemented;
#if 0
		ReloadAssetFile(GameState, GameState->Assets);
		Memory->NewAssetFile = false;
#endif
	}

	{DEBUG_DATA_BLOCK("Physics");
		if(GameState) {DEBUG_VALUE(&GameState->TimeScale);}
	}

	{DEBUG_DATA_BLOCK("Profiler");
		DEBUG_UI_ELEMENT(Event_FrameSlider, "Frame Slider");
		DEBUG_UI_ELEMENT(Event_TopClocksList, "Top Clocks");
		DEBUG_UI_ELEMENT(Event_FrameBarGraph, "Frame Graph");
	}

	{DEBUG_DATA_BLOCK("Memory");
		DEBUG_VALUE(&GameState->Region);
		DEBUG_VALUE(&GameState->ModeRegion);
		DEBUG_VALUE(&GameState->FrameRegion);
		assets *Assets = GameState->Assets;
		DEBUG_VALUE(&Assets->Region);
	}

	if(GameState->CurrentMode == GameMode_None)
	{
		// PlayTitleScreen(GameState);
		PlayWorldMode(GameState);
	}

	GameState->FrameRegionTemp = BeginTemporaryMemory(&GameState->FrameRegion);

	Input->Simdt = GameState->TimeScale*Input->Simdt;

	if(Input->KeyStates[Key_Tilde].WasPressed)
	{
		GameState->Console.Expanded = !GameState->Console.Expanded;
	}
	if(Input->KeyStates[Key_F1].WasPressed)
	{
		GameState->DevMode = DevMode_None;
	}
	if(Input->KeyStates[Key_F2].WasPressed)
	{
		GameState->DevMode = DevMode_Editor;
	}
	if(Input->KeyStates[Key_F3].WasPressed)
	{
		GameState->UseDebugCamera = !GameState->UseDebugCamera;
	}

#if 0
	if(Input->KeyStates[Key_F2].WasPressed)
	{
		if(Memory->ServerConnection)
		{
			Platform->DisconnectFromServer(Memory->ServerConnection);
			Memory->ServerConnection = 0;
		}
		else
		{
			Memory->ServerConnection = Platform->ConnectToServer(ConstZ("jesse-DESKTOP"));
		}
	}

	if(Input->KeyStates[Key_F3].WasPressed)
	{
		if(Memory->ServerConnection)
		{
			char SendBuffer[512];
			string SendString = FormatString(SendBuffer, ArrayCount(SendBuffer), "Hello Server! Current TimeScale: %f", GameState->TimeScale);
			Platform->SendData(Memory->ServerConnection, SendString.Length, SendString.Text);
		}
	}
#endif

	switch(GameState->CurrentMode)
	{
		case GameMode_WorldMode:
		{
			UpdateAndRenderWorldMode(GameState, GameState->WorldMode, RenderCommands, Input);
		} break;

		case GameMode_TitleScreen:
		{
			UpdateAndRenderTitleScreen(GameState, GameState->TitleScreen, RenderCommands, Input);
		} break;

		InvalidDefaultCase;
	}

	v2i ScreenResolution = Input->ScreenResolution;
	render_group Group_ = BeginRender(GameState->Assets, RenderCommands, Target_Overlay, {}, Render_ClearDepth);
	render_group *Group = &Group_;
	camera ScreenCamera = DefaultScreenCamera(ScreenResolution);
	SetCamera(Group, &ScreenCamera);
	BeginFrameUI(&GameState->UI, Group, Input);

	if(GameState->DevMode == DevMode_Editor)
	{
		if(GameState->CurrentMode == GameMode_WorldMode)
		{
			UpdateAndRenderEditor(GameState, GameState->Editor, &GameState->UI, GameState->WorldMode, Input);
		}
	}
	if(GameState->Console.Expanded)
	{
		ConsoleConsumeAllStreams(&GameState->Region, &GameState->Console);

		f32 Height = 500.0f;
		f32 Width = 1500.0f;
		f32 Bottom = ScreenResolution.y - Height;
		f32 Left = 0.5f*(ScreenResolution.x - Width);
		v2 Min = V2(Left, Bottom);
		v2 Max = Min + V2(Width, Height);
		DisplayConsole(GameState, Group, &GameState->Console, Min, Max, Input->ScreenResolution);
	}

	EndFrameUI(&GameState->UI);
	EndRender(Group);

	EndTemporaryMemory(&GameState->FrameRegionTemp);

	CheckMemory(&GameState->Region);
	CheckMemory(&GameState->ModeRegion);
	CheckMemory(&GameState->FrameRegion);
}

#if GAME_DEBUG
#include "steelth_debug.cpp"
#else
extern "C" DEBUG_FRAME_END(DebugFrameEnd)
{
}
#endif
