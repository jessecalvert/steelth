/*@H
* File: win32_steelth.cpp
* Author: Jesse Calvert
* Created: October 6, 2017, 17:42
* Last modified: April 26, 2019, 19:24
*/

// #include "windows.h"
#include "winsock2.h"
#include "ws2tcpip.h"
#include "Windowsx.h"

#include "steelth.h"
#include "steelth_renderer.h"
#include "win32_steelth.h"

global win32_state Win32State;
global b32 GlobalRunning;
global WINDOWPLACEMENT GlobalWindowPosition = { sizeof(GlobalWindowPosition) };
global v2i GlobalWindowSize;
global stream InfoStream;
global memory_region Win32Region;

internal void
Win32ProcessKey(key_state *KeyState, b32 Pressed)
{
	if(KeyState->Pressed != Pressed)
	{
		KeyState->Pressed = Pressed;
		++KeyState->TransitionCount;
		if(Pressed)
		{
			KeyState->WasPressed = true;
		}
		else
		{
			KeyState->WasReleased = true;
		}
	}
}

internal void
Win32ToggleFullscreen(HWND Window)
{
    // NOTE: this follows Raymond Chen's prescription for fullscreen
    // toggling, see:
    // http://blogs.msdn.com/b/oldnewthing/archive/2010/04/12/9994016.aspx
    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if (Style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
        if (GetWindowPlacement(Window, &GlobalWindowPosition) &&
            GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
        {
            SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window, HWND_TOP,
                         MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &GlobalWindowPosition);
        SetWindowPos(Window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

internal v2i
Win32GetScreenResolution()
{
	v2i Result = {};

	WNDCLASS WindowClass = {};
	WindowClass.style = CS_OWNDC;
	WindowClass.lpfnWndProc = DefWindowProcA;
	WindowClass.hInstance = GetModuleHandle(0);
	WindowClass.lpszClassName = "ResolutionFinder";

	RegisterClass(&WindowClass);
	HWND DummyWindow =  CreateWindow("ResolutionFinder",
			                         "Steelth",
			                         0,
			                         CW_USEDEFAULT,
			                         CW_USEDEFAULT,
			                         CW_USEDEFAULT,
			                         CW_USEDEFAULT,
			                         0,
			                         0,
			                         WindowClass.hInstance,
			                         0);

	if(DummyWindow)
	{
		HDC DeviceContext = GetDC(DummyWindow);

		Result.x = GetDeviceCaps(DeviceContext, HORZRES);
		Result.y = GetDeviceCaps(DeviceContext, VERTRES);

		ReleaseDC(DummyWindow, DeviceContext);
		DestroyWindow(DummyWindow);
	}

	return Result;
}

inline char *
Win32GetLoopPlaybackFilename()
{
	char *Filename = "w:/steelth/build/loop_data_0.sld";
	return Filename;
}

internal void
Win32FreeBlock(win32_memory_block *Win32Block)
{
	DLIST_REMOVE(Win32Block);
	BOOL Success = VirtualFree(Win32Block, 0, MEM_RELEASE);
	Assert(Success);
}

internal void
Win32FreeAllBlocksByMask(u64 Mask)
{
	BeginTicketMutex(&Win32State.MemoryMutex);
	for(win32_memory_block *BlockIter = Win32State.MemorySentinel.Next;
		BlockIter != &Win32State.MemorySentinel;
		)
	{
		win32_memory_block *Block = BlockIter;
		BlockIter = BlockIter->Next;

		if((Block->LoopingFlags & Mask) == Mask)
		{
			Win32FreeBlock(Block);
		}
	}
	EndTicketMutex(&Win32State.MemoryMutex);
}

internal void
Win32ClearAllBlockFlags()
{
	BeginTicketMutex(&Win32State.MemoryMutex);
	for(win32_memory_block *Block = Win32State.MemorySentinel.Next;
		Block != &Win32State.MemorySentinel;
		Block = Block->Next)
	{
		Block->LoopingFlags = 0;
	}
	EndTicketMutex(&Win32State.MemoryMutex);
}

internal void
Win32BeginInputRecording()
{
	char *Filename = Win32GetLoopPlaybackFilename();
	Win32State.RecordingHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if(Win32State.RecordingHandle != INVALID_HANDLE_VALUE)
	{
		DWORD BytesWritten;

		Win32State.LoopRecording = true;

		BeginTicketMutex(&Win32State.MemoryMutex);
		win32_memory_block *Sentinel = &Win32State.MemorySentinel;
		for(win32_memory_block *SourceBlock = Sentinel->Next;
			SourceBlock != Sentinel;
			SourceBlock = SourceBlock->Next)
		{
			if(!(SourceBlock->Block.Flags & RegionFlag_NotRestored))
			{
				win32_saved_memory_block DestBlock = {};
				u8 *Base = SourceBlock->Block.Base;
				DestBlock.BasePtr = (umm)Base;
				DestBlock.Size = SourceBlock->Block.Size;
				WriteFile(Win32State.RecordingHandle, &DestBlock, sizeof(DestBlock), &BytesWritten, 0);
				Assert(BytesWritten == sizeof(DestBlock));
				Assert(DestBlock.Size < U32Maximum);
				WriteFile(Win32State.RecordingHandle, Base, (u32)DestBlock.Size, &BytesWritten, 0);
				Assert(BytesWritten == DestBlock.Size);
			}
		}
		EndTicketMutex(&Win32State.MemoryMutex);

		win32_saved_memory_block NullTerminatingBlock = {};
		WriteFile(Win32State.RecordingHandle, &NullTerminatingBlock, sizeof(NullTerminatingBlock), &BytesWritten, 0);
		Assert(BytesWritten == sizeof(NullTerminatingBlock));
	}
}

internal void
Win32EndInputRecording()
{
	CloseHandle(Win32State.RecordingHandle);
	Win32State.LoopRecording = false;
}

internal void
Win32BeginLoopPlayback()
{
	Win32FreeAllBlocksByMask(Win32Memory_AllocatedDuringLoop);

	char *Filename = Win32GetLoopPlaybackFilename();
	Win32State.PlaybackHandle = CreateFileA(Filename, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
	if(Win32State.PlaybackHandle != INVALID_HANDLE_VALUE)
	{
		Win32State.LoopPlayback = true;

		while(1)
		{
			win32_saved_memory_block Block = {};
			DWORD BytesRead;
			ReadFile(Win32State.PlaybackHandle, &Block, sizeof(Block), &BytesRead, 0);
			Assert(BytesRead == sizeof(Block));
			if(Block.BasePtr != 0)
			{
				u8 *Base = (u8 *)Block.BasePtr;
				Assert(Block.Size < U32Maximum);
				ReadFile(Win32State.PlaybackHandle, Base, (u32)Block.Size, &BytesRead, 0);
				Assert(BytesRead == Block.Size);
			}
			else
			{
				break;
			}
		}
	}
}

internal void
Win32EndLoopPlayback()
{
	CloseHandle(Win32State.PlaybackHandle);
	Win32State.LoopPlayback = false;
}

internal void
Win32RecordInput(game_input *Input)
{
	DWORD BytesWritten;
	WriteFile(Win32State.RecordingHandle, Input, sizeof(*Input), &BytesWritten, 0);
	Assert(BytesWritten == sizeof(*Input));
}

internal void
Win32LoopPlayback(game_input *NewInput)
{
	DWORD BytesRead;
	if(ReadFile(Win32State.PlaybackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0))
	{
		if(BytesRead == 0)
		{
			// NOTE: Reached the end of the loop; restarting.
			Win32EndLoopPlayback();
			Win32BeginLoopPlayback();
			ReadFile(Win32State.PlaybackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0);
		}
	}
}

#define WIN32_PROCESS_KEY(VkCode, GameKey) \
case (VkCode): { \
key_state *KeyState = &Input->KeyStates[(GameKey)]; \
Win32ProcessKey(KeyState, IsDown);} break;

internal void
Win32ProcessKeyboardInput(MSG *Message, game_input *Input)
{
	b32 AltKeyWasDown = (Message->lParam & (1<<29));
	b32 WasDown = ((Message->lParam & (1<<30)) != 0);
    b32 IsDown = ((Message->lParam & (1<<31)) == 0);

    if(WasDown != IsDown)
	{
		switch(Message->wParam)
		{
			WIN32_PROCESS_KEY('A', Key_A);
			WIN32_PROCESS_KEY('B', Key_B);
			WIN32_PROCESS_KEY('C', Key_C);
			WIN32_PROCESS_KEY('D', Key_D);
			WIN32_PROCESS_KEY('E', Key_E);
			WIN32_PROCESS_KEY('F', Key_F);
			WIN32_PROCESS_KEY('G', Key_G);
			WIN32_PROCESS_KEY('H', Key_H);
			WIN32_PROCESS_KEY('I', Key_I);
			WIN32_PROCESS_KEY('J', Key_J);
			WIN32_PROCESS_KEY('K', Key_K);
			WIN32_PROCESS_KEY('L', Key_L);
			WIN32_PROCESS_KEY('M', Key_M);
			WIN32_PROCESS_KEY('N', Key_N);
			WIN32_PROCESS_KEY('O', Key_O);
			WIN32_PROCESS_KEY('P', Key_P);
			WIN32_PROCESS_KEY('Q', Key_Q);
			WIN32_PROCESS_KEY('R', Key_R);
			WIN32_PROCESS_KEY('S', Key_S);
			WIN32_PROCESS_KEY('T', Key_T);
			WIN32_PROCESS_KEY('U', Key_U);
			WIN32_PROCESS_KEY('V', Key_V);
			WIN32_PROCESS_KEY('W', Key_W);
			WIN32_PROCESS_KEY('X', Key_X);
			WIN32_PROCESS_KEY('Y', Key_Y);
			WIN32_PROCESS_KEY('Z', Key_Z);

			WIN32_PROCESS_KEY('1', Key_1);
			WIN32_PROCESS_KEY('2', Key_2);
			WIN32_PROCESS_KEY('3', Key_3);
			WIN32_PROCESS_KEY('4', Key_4);
			WIN32_PROCESS_KEY('5', Key_5);
			WIN32_PROCESS_KEY('6', Key_6);
			WIN32_PROCESS_KEY('7', Key_7);
			WIN32_PROCESS_KEY('8', Key_8);
			WIN32_PROCESS_KEY('9', Key_9);
			WIN32_PROCESS_KEY('0', Key_0);

			WIN32_PROCESS_KEY(VK_OEM_MINUS, Key_Minus);
			WIN32_PROCESS_KEY(VK_OEM_PLUS, Key_Plus);
			WIN32_PROCESS_KEY(VK_OEM_3, Key_Tilde);
			WIN32_PROCESS_KEY(VK_OEM_4, Key_OpenBracket);
			WIN32_PROCESS_KEY(VK_OEM_6, Key_CloseBracket);
			WIN32_PROCESS_KEY(VK_TAB, Key_Tab);

			WIN32_PROCESS_KEY(VK_UP, Key_Up);
			WIN32_PROCESS_KEY(VK_DOWN, Key_Down);
			WIN32_PROCESS_KEY(VK_LEFT, Key_Left);
			WIN32_PROCESS_KEY(VK_RIGHT, Key_Right);
			WIN32_PROCESS_KEY(VK_SPACE, Key_Space);
			WIN32_PROCESS_KEY(VK_CONTROL, Key_Ctrl);
			WIN32_PROCESS_KEY(VK_SHIFT, Key_Shift);
			case VK_RETURN:
			{
				if(AltKeyWasDown && IsDown)
				{
					HWND Window = Message->hwnd;
					Win32ToggleFullscreen(Window);
				}
				else
				{
					key_state *KeyState = &Input->KeyStates[Key_Enter];
					Win32ProcessKey(KeyState, IsDown);
				}
			} break;

			WIN32_PROCESS_KEY(VK_F1, Key_F1);
			WIN32_PROCESS_KEY(VK_F2, Key_F2);
			WIN32_PROCESS_KEY(VK_F3, Key_F3);

			case VK_F4:
			{
				if(AltKeyWasDown)
				{
					GlobalRunning = false;
				}
				else
				{
					key_state *KeyState = &Input->KeyStates[Key_F4];
					Win32ProcessKey(KeyState, IsDown);
				}
			} break;

			WIN32_PROCESS_KEY(VK_F5, Key_F5);
			WIN32_PROCESS_KEY(VK_F6, Key_F6);
			WIN32_PROCESS_KEY(VK_F7, Key_F7);
			WIN32_PROCESS_KEY(VK_F8, Key_F8);
			WIN32_PROCESS_KEY(VK_F9, Key_F9);

			case VK_F10:
			{
				if(IsDown)
				{
					if(Win32State.LoopPlayback)
					{
						Win32EndLoopPlayback();
						Win32FreeAllBlocksByMask(Win32Memory_DeallocatedDuringLoop);
						Win32ClearAllBlockFlags();
					}
					else
					{
						if(Win32State.LoopRecording)
						{
							Win32EndInputRecording();
							Win32BeginLoopPlayback();
						}
						else
						{
							Win32BeginInputRecording();
						}
					}
				}
			} break;

			WIN32_PROCESS_KEY(VK_F11, Key_F11);
			WIN32_PROCESS_KEY(VK_F12, Key_F12);
			WIN32_PROCESS_KEY(VK_ESCAPE, Key_Esc);
		}
	}
}

internal void
Win32ProcessMouseInputs(MSG *Message, game_input *Input, v2i WindowDim)
{
	WPARAM wParam = Message->wParam;
	LPARAM lParam = Message->lParam;

	// TODO: Keep track of the MouseP where the buttons were clicked for
	// better accuracy.
	switch(Message->message)
	{
		case WM_LBUTTONDOWN:
		{
			Win32ProcessKey(&Input->KeyStates[Key_LeftClick], true);
		} break;

		case WM_RBUTTONDOWN:
		{
			Win32ProcessKey(&Input->KeyStates[Key_RightClick], true);
		} break;

		case WM_LBUTTONUP:
		{
			Win32ProcessKey(&Input->KeyStates[Key_LeftClick], false);
		} break;

		case WM_RBUTTONUP:
		{
			Win32ProcessKey(&Input->KeyStates[Key_RightClick], false);
		} break;

		case WM_MOUSEMOVE:
		{
			// NOTE: The mouse origin is in the bottom-left of the screen,
			// with y going up and x going right.
			v2i MousePInt = {GET_X_LPARAM(lParam), WindowDim.y - GET_Y_LPARAM(lParam)};
			v2 MouseP = V2((f32)MousePInt.x, (f32)MousePInt.y);
			Input->MouseP = MouseP;
		} break;

        case WM_MOUSEWHEEL:
        {
        	s32 WheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        	Input->MouseWheelDelta += ((f32)WheelDelta)/WHEEL_DELTA;
        } break;
	}
}

internal LRESULT CALLBACK
Win32WindowProc(HWND Window, UINT Message,
                WPARAM wParam, LPARAM lParam)
{
	LRESULT Result = 0;
    switch (Message)
    {
        case WM_CREATE:
        {
            // Initialize the window.
        } break;

        case WM_PAINT:
		{
        	// Paint the window's client area.
			PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            EndPaint(Window, &Paint);
        } break;

        case WM_SIZE:
        {
            // Set the size and position of the window.
        	s32 NewWidth = (s32)(lParam & 0xFFFF);
        	s32 NewHeight = (s32)(lParam >> 16);
        	GlobalWindowSize = {NewWidth, NewHeight};
        } break;

        case WM_DESTROY:
        {
            // Clean up window-specific data objects.
        	GlobalRunning = false;
        } break;

        case WM_CLOSE:
        {
        	GlobalRunning = false;
        } break;

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        {
        	Assert(!"All the keyboard messages should have been handled already!");
        } break;

        default:
        {
            Result = DefWindowProc(Window, Message, wParam, lParam);
        }
    }

	return Result;
}

internal FILETIME
Win32GetLastWriteTime(char *Filename)
{
	WIN32_FILE_ATTRIBUTE_DATA FileData = {};
	GetFileAttributesEx(Filename, GetFileExInfoStandard, &FileData);
	return FileData.ftLastWriteTime;
}

internal b32
Win32FileHasBeenWritten(FILETIME LastWriteTime, char *Filename)
{
	FILETIME NewWriteFileTime = Win32GetLastWriteTime(Filename);
	ULARGE_INTEGER NewWriteTime = {};
	NewWriteTime.LowPart = NewWriteFileTime.dwLowDateTime;
	NewWriteTime.HighPart = NewWriteFileTime.dwHighDateTime;

	ULARGE_INTEGER OldWriteTime = {};
	OldWriteTime.LowPart = LastWriteTime.dwLowDateTime;
	OldWriteTime.HighPart = LastWriteTime.dwHighDateTime;

	b32 Result = (OldWriteTime.QuadPart != NewWriteTime.QuadPart);
	return Result;

}

internal b32
Win32NewDLLAvailable(hotloadable_code *Code)
{
	b32 Result = Win32FileHasBeenWritten(Code->LastWriteTime, Code->DLLName);
	return Result;
}

inline u32
Win32GetFileSize(char *Filename)
{
	WIN32_FILE_ATTRIBUTE_DATA FileData = {};
	GetFileAttributesEx(Filename, GetFileExInfoStandard, &FileData);
	Assert(FileData.nFileSizeHigh == 0);
	u32 FileSize = FileData.nFileSizeLow;
	return FileSize;
}

internal b32
Win32DLLLockIsActive(char *LockName)
{
	b32 Result = (Win32GetFileSize(LockName) == 0);
	return Result;
}

internal void
Win32UnloadCodeDLL(hotloadable_code *Code)
{
	switch(Code->Type)
	{
		case CodeType_Game:
		{
			game_code *Game = &Code->Game;
			Game->GameUpdateAndRender = 0;
#if GAME_DEBUG
			Game->DEBUGFrameEnd = 0;
#endif
		} break;

#if 0
		case CodeType_Renderer:
		{
			renderer_code *Renderer = &Code->Renderer;
			Renderer->Render = 0;
		} break;
#endif

		InvalidDefaultCase;
	}
	FreeLibrary(Code->Handle);
	Code->LastWriteTime = {};
}

internal void
Win32LoadCode(hotloadable_code *Code)
{
	if(Win32DLLLockIsActive(Code->LockName))
	{
		b32 FileCopied = CopyFile(Code->DLLName, Code->DLLNameTemp, 0);
		if(FileCopied)
		{
			Code->Handle = LoadLibraryEx(Code->DLLNameTemp, 0, 0);
			if(Code->Handle)
			{
				switch(Code->Type)
				{
					case CodeType_Game:
					{
						game_code *Game = &Code->Game;
						Game->GameUpdateAndRender = (game_update_and_render *)GetProcAddress(Code->Handle, "GameUpdateAndRender");
#if GAME_DEBUG
						Game->DEBUGFrameEnd = (debug_frame_end *)GetProcAddress(Code->Handle, "DebugFrameEnd");
#endif
					} break;

#if 0
					case CodeType_Renderer:
					{
						renderer_code *Renderer = &Code->Renderer;
						Renderer->Render = (renderer_render *)GetProcAddress(Code->Handle, "Render");
					} break;
#endif
				}

				Code->LastWriteTime = Win32GetLastWriteTime(Code->DLLName);
			}
			else
			{
				Outf(InfoStream.Errors, "Failed to load temporary DLL!");
			}
		}
		else
		{
			Outf(InfoStream.Errors, "Failed to copy DLL!");
		}
	}
}

internal b32
Win32FileIsReady(char *Filename)
{
	b32 Result = false;

	HANDLE FileHandle = CreateFile(Filename, GENERIC_READ, FILE_SHARE_READ, 0,
	                               OPEN_EXISTING, 0, 0);
	if(FileHandle != INVALID_HANDLE_VALUE)
	{
		Result = true;
		CloseHandle(FileHandle);
	}

	return Result;
}

internal u8 *
Win32OpenFileOfSize(memory_region *Region, u32 Size, char *Filename)
{
	u8 *Result = 0;

	HANDLE FileHandle = CreateFile(Filename, GENERIC_READ, FILE_SHARE_READ, 0,
	                               OPEN_EXISTING, 0, 0);
	if(FileHandle != INVALID_HANDLE_VALUE)
	{
		DWORD BytesRead = 0;
		Result = PushSize(Region, Size);
		b32 FileRead = ReadFile(FileHandle, Result, Size, &BytesRead, 0);
		Assert(FileRead);
		CloseHandle(FileHandle);
	}
	else
	{
		Outf(InfoStream.Errors, "Could not open file! (%s)", WrapZ(Filename));
	}

	return Result;
}

internal
LOAD_ENTIRE_FILE_AND_NULL_TERMINATE(Win32LoadEntireFileAndNullTerminate)
{
	char FilenameCString[512] = {};
	Assert(ArrayCount(FilenameCString) > (Filename.Length + 1));
	CopySize(Filename.Text, FilenameCString, Filename.Length);

	u32 FileSize = Win32GetFileSize(FilenameCString) + 1;
	buffer Result = {};
	Result.Contents = Win32OpenFileOfSize(Region, FileSize, FilenameCString);
	Result.Used = Result.Size = FileSize;
	if(Result.Contents)
	{
		Result.Contents[FileSize - 1] = 0;
	}

	return Result;
}

internal
LOAD_ENTIRE_FILE(Win32LoadEntireFile)
{
	char FilenameCString[512] = {};
	Assert(ArrayCount(FilenameCString) > (Filename.Length + 1));
	CopySize(Filename.Text, FilenameCString, Filename.Length);

	u32 FileSize = Win32GetFileSize(FilenameCString);
	buffer Result = {};
	Result.Contents = Win32OpenFileOfSize(Region, FileSize, FilenameCString);
	Result.Used = Result.Size = FileSize;
	return Result;
}

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
internal
LOAD_HDR_PNG(Win32LoadHDRPNG)
{
	f32 *Result = 0;

	buffer FileContents = Win32LoadEntireFile(Region, Filename);
	if(FileContents.Contents)
	{
		s32 WidthInFile, HeightInFile, ChannelsInFile;
		Result = (f32 *)stbi_loadf_from_memory(FileContents.Contents, (s32)FileContents.Size, &WidthInFile, &HeightInFile, &ChannelsInFile, Channels);

		Assert(WidthInFile == (s32)Width);
		Assert(HeightInFile == (s32)Height);
		Assert(ChannelsInFile == (s32)Channels);
	}

	return Result;
}

internal void
RegisterFileHotload(win32_hotloader *Hotloader, hotloadable_file FileInit)
{
	Assert(Hotloader->FileCount < ArrayCount(Hotloader->Files));
	hotloadable_file *File = Hotloader->Files + Hotloader->FileCount++;
	*File = FileInit;

	File->NeedsReloading = false;
	File->LastWriteTime = Win32GetLastWriteTime(File->Filename);
}

internal void
Win32RegisterHotloads(win32_hotloader *Hotloader)
{
	RegisterFileHotload(Hotloader,
		{"Shaders/steelth_header.glsl", FileFlag_Header, File_Shader, Shader_Header});
	RegisterFileHotload(Hotloader,
		{"Shaders/steelth_overlay.glsl", 0, File_Shader, Shader_Overlay});
	RegisterFileHotload(Hotloader,
		{"Shaders/steelth_finalize.glsl", 0, File_Shader, Shader_Finalize});
	RegisterFileHotload(Hotloader,
		{"Shaders/steelth_overbright.glsl", 0, File_Shader, Shader_Overbright});
	RegisterFileHotload(Hotloader,
		{"Shaders/steelth_blur.glsl", 0, File_Shader, Shader_Blur});
	RegisterFileHotload(Hotloader,
		{"Shaders/steelth_debug_view.glsl", 0, File_Shader, Shader_DebugView});
	RegisterFileHotload(Hotloader,
		{"Shaders/steelth_gbuffer.glsl", 0, File_Shader, Shader_GBuffer});
	RegisterFileHotload(Hotloader,
		{"Shaders/steelth_fresnel.glsl", 0, File_Shader, Shader_Fresnel});
	RegisterFileHotload(Hotloader,
		{"Shaders/steelth_light_pass.glsl", 0, File_Shader, Shader_LightPass});
	RegisterFileHotload(Hotloader,
		{"Shaders/steelth_indirect_light_pass.glsl", 0, File_Shader, Shader_IndirectLightPass});
	RegisterFileHotload(Hotloader,
		{"Shaders/steelth_decal.glsl", 0, File_Shader, Shader_Decal});
	RegisterFileHotload(Hotloader,
		{"Shaders/steelth_surfels.glsl", 0, File_Shader, Shader_Surfels});
	RegisterFileHotload(Hotloader,
		{"Shaders/steelth_probes.glsl", 0, File_Shader, Shader_Probes});
	RegisterFileHotload(Hotloader,
		{"Shaders/steelth_irradiance_volume.glsl", 0, File_Shader, Shader_Irradiance_Volume});
	RegisterFileHotload(Hotloader,
		{"Shaders/steelth_skybox.glsl", 0, File_Shader, Shader_Skybox});
	RegisterFileHotload(Hotloader,
		{"Shaders/steelth_fog.glsl", 0, File_Shader, Shader_Fog});
	RegisterFileHotload(Hotloader,
		{"Shaders/steelth_fog_resolve.glsl", 0, File_Shader, Shader_FogResolve});
	RegisterFileHotload(Hotloader,
		{"Shaders/steelth_shadow_map.glsl", 0, File_Shader, Shader_ShadowMap});

	RegisterFileHotload(Hotloader,
		{"asset_pack.sla", 0, File_Assets});
}

internal void
Win32CheckFilesForHotload(win32_hotloader *Hotloader, game_memory *GameMemory, render_memory_op_queue *RenderOpQueue)
{
	for(u32 Index = 0;
		Index < Hotloader->FileCount;
		++Index)
	{
		hotloadable_file *File = Hotloader->Files + Index;
		if(Win32FileHasBeenWritten(File->LastWriteTime, File->Filename))
		{
			File->NeedsReloading = true;
			File->LastWriteTime = Win32GetLastWriteTime(File->Filename);

			if(File->Flags & FileFlag_Header)
			{
				for(u32 TestIndex = 0;
					TestIndex < Hotloader->FileCount;
					++TestIndex)
				{
					hotloadable_file *TestFile = Hotloader->Files + TestIndex;
					if(TestFile->Tag == File->Tag)
					{
						TestFile->NeedsReloading = true;
						TestFile->LastWriteTime = Win32GetLastWriteTime(TestFile->Filename);
					}
				}
			}
		}
	}

	for(u32 Index = 0;
		Index < Hotloader->FileCount;
		++Index)
	{
		hotloadable_file *File = Hotloader->Files + Index;
		if(File->NeedsReloading && Win32FileIsReady(File->Filename))
		{
			switch(File->Tag)
			{
				case File_Shader:
				{
					if(!(File->Flags & FileFlag_Header))
					{
						render_memory_op Op = {};
						Op.Type = RenderOp_ReloadShader;
						Op.ReloadShader.ShaderType = File->Shader;
						AddRenderOp(RenderOpQueue, &Op);
					}

					File->NeedsReloading = false;
				} break;

				case File_Assets:
				{
					GameMemory->NewAssetFile = true;
					File->NeedsReloading = false;
				} break;

				InvalidDefaultCase;
			}
		}
	}
}

internal r32
Win32GetTimeSince(LARGE_INTEGER CountsPerSecond, LARGE_INTEGER OldTime)
{
	LARGE_INTEGER CurrentTime;
	QueryPerformanceCounter(&CurrentTime);
	u64 CountsSinceLast = CurrentTime.QuadPart - OldTime.QuadPart;

	r32 Result = (r32)CountsSinceLast / (r32)CountsPerSecond.QuadPart;
	return Result;
}

internal
CLOSE_FILE(Win32CloseFile)
{
	CloseHandle(File->Handle_);
}

internal
OPEN_ALL_FILES(Win32OpenAllFiles)
{
    platform_file_list Result = {};

    char Filename[128] = {};
    FormatString(Filename, ArrayCount(Filename), "*%s", Extension);

    WIN32_FIND_DATA FindData = {};
	HANDLE FindHandle = FindFirstFileA(Filename, &FindData);
    if(FindHandle != INVALID_HANDLE_VALUE)
    {
    	do
    	{
		    HANDLE FileHandle = CreateFile(FindData.cFileName, GENERIC_READ|GENERIC_WRITE,
		    	FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
		    if(FileHandle != INVALID_HANDLE_VALUE)
		    {
		    	Assert(Result.Count < ArrayCount(Result.Files));
		    	u32 FileIndex = Result.Count++;
		    	Result.Files[FileIndex].Handle_ = FileHandle;
		    	Result.Files[FileIndex].Name = PushString(&Win32Region, WrapZ(FindData.cFileName));

		    }

		} while(FindNextFileA(FindHandle, &FindData));

	    FindClose(FindHandle);
    }

    return Result;
}

internal
READ_FROM_FILE(Win32ReadFromFile)
{
    if(Buffer)
    {
        HANDLE FileHandle = File->Handle_;
        OVERLAPPED Overlapped = {};
        Overlapped.Offset = Offset;
        Overlapped.OffsetHigh = 0;

        DWORD BytesRead = 0;
        b32 Success = ReadFile(FileHandle, Buffer, Length, &BytesRead, &Overlapped);
        Assert(Success);
        Assert(BytesRead == Length);
    }
}

internal
WRITE_INTO_FILE(Win32WriteIntoFile)
{
	if(Buffer)
	{
        HANDLE FileHandle = File->Handle_;
        OVERLAPPED Overlapped = {};
        Overlapped.Offset = Offset;
        Overlapped.OffsetHigh = 0;

        DWORD BytesWritten = 0;
		b32 Success = WriteFile(FileHandle, Buffer, Length, &BytesWritten, &Overlapped);
		Assert(Success);
        Assert(BytesWritten == Length);
	}
}

inline b32
Win32IsLooping()
{
	b32 Result = (Win32State.LoopRecording || Win32State.LoopPlayback);
	return Result;
}

internal
ALLOCATE_MEMORY(Win32AllocateMemory)
{
	umm PageSize = Win32State.SystemInfo.dwPageSize;
	umm TotalSize = Size + sizeof(win32_memory_block);
	umm BaseOffset = sizeof(win32_memory_block);
	umm ProtectOffset = 0;

	if(Flags & RegionFlag_UnderflowChecking)
	{
		BaseOffset = 2*PageSize;
		TotalSize = Size + 2*PageSize;
		ProtectOffset = PageSize;
	}
	else if(Flags & RegionFlag_OverflowChecking)
	{
		umm InflatedSize = AlignPow2(Size, PageSize);
		TotalSize = InflatedSize + 2*PageSize;
		BaseOffset = PageSize + InflatedSize - Size;
		ProtectOffset = PageSize + InflatedSize;
	}

	win32_memory_block *NewBlock = (win32_memory_block *)VirtualAlloc(0, TotalSize,
		MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	Assert(NewBlock);
	NewBlock->LoopingFlags = (Win32IsLooping() && !(Flags & RegionFlag_NotRestored)) ? Win32Memory_AllocatedDuringLoop : 0;

	platform_memory_block *Block = &NewBlock->Block;
	Block->Base = (u8 *)Block + BaseOffset;
	Block->Size = Size;
	Block->Flags = Flags;
	Assert(Block->Used == 0);
	Assert(Block->BlockPrev == 0);

	BeginTicketMutex(&Win32State.MemoryMutex);
	DLIST_INSERT(&Win32State.MemorySentinel, NewBlock);
	EndTicketMutex(&Win32State.MemoryMutex);

	if(ProtectOffset)
	{
		DWORD OldProtect = 0;
		VirtualProtect((u8 *)Block + ProtectOffset, PageSize, PAGE_NOACCESS, &OldProtect);
	}

	return Block;
}

internal
DEALLOCATE_MEMORY(Win32DeallocateMemory)
{
	if(Block)
	{
		win32_memory_block *Win32Block = (win32_memory_block *)Block;
		if(Win32IsLooping() &&
			!(Win32Block->LoopingFlags & Win32Memory_AllocatedDuringLoop) &&
			!(Block->Flags & RegionFlag_NotRestored))
		{
			Win32Block->LoopingFlags |= Win32Memory_DeallocatedDuringLoop;
		}
		else
		{
			BeginTicketMutex(&Win32State.MemoryMutex);
			Win32FreeBlock(Win32Block);
			EndTicketMutex(&Win32State.MemoryMutex);
		}
	}
}

internal b32
Win32ThreadQueueDoWork(platform_work_queue *Queue)
{
	b32 GoToSleep = false;

	u32 EntryIndex = Queue->FirstEntry;
	u32 NextEntryIndex = (Queue->FirstEntry + 1) % ArrayCount(Queue->Entries);
	if(EntryIndex != Queue->OnePastLastEntry)
	{
		u32 Index = InterlockedCompareExchange(&Queue->FirstEntry, NextEntryIndex, EntryIndex);

		if(Index == EntryIndex)
		{
			platform_work_queue_entry *Entry = Queue->Entries + Index;
			Entry->Callback(Queue, Entry->Data);
			InterlockedIncrement(&Queue->WorkCompleted);
		}
	}
	else
	{
		GoToSleep = true;
	}

	return GoToSleep;
}

struct thread_startup
{
	platform_work_queue *Queue;
};

DWORD WINAPI
Win32ThreadProcedure(LPVOID LParam)
{
	thread_startup *Startup = (thread_startup *) LParam;

	u32 TestThreadID = GetThreadID();
	Assert(TestThreadID == GetCurrentThreadId());

	while(1)
	{
		if(Win32ThreadQueueDoWork(Startup->Queue))
		{
			WaitForSingleObjectEx(Startup->Queue->SemaphoreHandle, INFINITE, 0);
		}
	}

	return 0;
}

internal void
Win32CreateWorkQueue(thread_startup *Startup, u32 ThreadCount)
{
	Startup->Queue->FirstEntry = 0;
	Startup->Queue->OnePastLastEntry = 0;

	Startup->Queue->SemaphoreHandle = CreateSemaphoreEx(0, 0, ThreadCount, 0, 0, SEMAPHORE_ALL_ACCESS);

	for(u32 Index = 0;
		Index < ThreadCount;
		++Index)
	{
		DWORD ThreadID;
		HANDLE ThreadHandle = CreateThread(0, 0, Win32ThreadProcedure, Startup, 0, &ThreadID);
		CloseHandle(ThreadHandle);
	}
}

ADD_WORK(Win32AddWorkEntry)
{
	// TODO: For now, only the main thread can add work!
	Assert(Queue->OnePastLastEntry != (Queue->FirstEntry - 1));
	platform_work_queue_entry *Entry = Queue->Entries + Queue->OnePastLastEntry;
	Entry->Callback = Callback;
	Entry->Data = Data;
	Queue->OnePastLastEntry = (Queue->OnePastLastEntry + 1) % ArrayCount(Queue->Entries);
	++Queue->WorkGoal;
	_WriteBarrier();
	ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);
}

COMPLETE_ALL_WORK(Win32CompleteAllWork)
{
	while(Queue->WorkGoal != Queue->WorkCompleted)
	{
		Win32ThreadQueueDoWork(Queue);
	}

	Queue->WorkGoal = 0;
	Queue->WorkCompleted = 0;
}

internal void
Win32GetExecutablePath()
{
	DWORD SizeOfFilename = GetModuleFileNameA(0, Win32State.EXEFullPath, ArrayCount(Win32State.EXEFullPath));
	Win32State.EXEDirectory.Text = Win32State.EXEFullPath;
	for(char *Scan = Win32State.EXEFullPath;
		*Scan;
		++Scan)
	{
		if(*Scan == '\\')
		{
			Win32State.EXEDirectory.Length = SafeTruncateU64ToU32(Scan - Win32State.EXEFullPath);
		}
	}
}

internal void
Win32BuildEXEPathFilename(char *Filename, char *DestBuffer, u32 Size)
{
	FormatString(DestBuffer, Size, "%s\\%s", Win32State.EXEDirectory, WrapZ(Filename));
}

DISCONNECT_FROM_SERVER(Win32DisconnectFromServer)
{
	win32_server_connection *Win32Connection = (win32_server_connection *)Connection;

	if(Win32Connection)
	{
		if(Win32Connection->ServerAddrInfo)
		{
			freeaddrinfo(Win32Connection->ServerAddrInfo);
		}
		if(Win32Connection->Socket != INVALID_SOCKET)
		{
			closesocket(Win32Connection->Socket);
		}

		VirtualFree(Win32Connection, 0, MEM_RELEASE);

		Outf(&InfoStream, "Disconnect from Server.");
	}
}

internal b32
Win32WSAStartup()
{
	b32 Result = false;

	s32 HiVersion = 2;
	s32 LoVersion = 2;
	WORD VersionRequest = MAKEWORD(HiVersion, LoVersion);
	WSADATA WSAData = {};
	s32 Error = WSAStartup(VersionRequest, &WSAData);

	if(!Error)
	{
		if((LOBYTE(WSAData.wVersion) == LoVersion) &&
		   (HIBYTE(WSAData.wVersion) == HiVersion))
		{
			Result = true;
		}
		else
		{
			Outf(InfoStream.Errors, "Unable to find a usable version of Winsock.dll!");
		}
	}
	else
	{
		Outf(InfoStream.Errors, "Startup failed! (%d)", Error);
	}

	return Result;
}

CONNECT_TO_SERVER(Win32ConnectToServer)
{
	win32_server_connection *Win32Connection = (win32_server_connection *)
		VirtualAlloc(0, sizeof(win32_server_connection), MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	b32 Connected = false;

	addrinfo Hints = {};
	Hints.ai_family = AF_UNSPEC;
	Hints.ai_socktype = SOCK_STREAM;
	Hints.ai_protocol = IPPROTO_TCP;

	u32 Attempts = 0;
	s32 Error = getaddrinfo(ServiceHost.Text, DEFAULT_PORT, &Hints, &Win32Connection->ServerAddrInfo);
	if(!Error)
	{
		addrinfo *Info = Win32Connection->ServerAddrInfo;
		while(Info)
		{
			Attempts++;

			Win32Connection->Socket = socket(Info->ai_family, Info->ai_socktype, Info->ai_protocol);
			if(Win32Connection->Socket != INVALID_SOCKET)
			{
				Error = connect(Win32Connection->Socket, Info->ai_addr, (int)Info->ai_addrlen);
				if(!Error)
				{
					Connected = true;

					char HostName[128];
					char ServiceName[128];
					getnameinfo(
						Info->ai_addr,
						(socklen_t)Info->ai_addrlen,
						HostName,
						ArrayCount(HostName),
						ServiceName,
						ArrayCount(ServiceName),
						0);

					char IPName[128];
					InetNtop(Info->ai_family, Info->ai_addr, IPName, ArrayCount(IPName));

					Outf(&InfoStream, "Connected to Server: %s:%s : %s",
						WrapZ(IPName),
						WrapZ(ServiceName),
						WrapZ(HostName));
					break;
				}
				else
				{
					closesocket(Win32Connection->Socket);
					Info = Info->ai_next;
				}
			}
			else
			{
				Outf(InfoStream.Errors, "socket failed! (%d)", WSAGetLastError());
				Info = Info->ai_next;
			}
		}

		if(!Info)
		{
			Outf(InfoStream.Errors, "connect failed! Attemps: %d\n", Attempts);
		}
	}
	else
	{
		Outf(InfoStream.Errors, "getaddrinfo failed! (%d)", Error);
	}

	platform_server_connection *Result = &Win32Connection->PlatformConnection;

	if(!Connected)
	{
		Win32DisconnectFromServer(Result);
		Result = 0;
	}

	return Result;
}

SEND_DATA(Win32SendData)
{
	Assert(Size < DEFAULT_BUFFER_LENGTH);

	b32 Result = false;

	win32_server_connection *Win32Connection = (win32_server_connection *)Connection;
	Assert(Win32Connection->Socket != INVALID_SOCKET);

	int BytesSent = send(Win32Connection->Socket, (char *)Contents, Size, 0);
	if(BytesSent != SOCKET_ERROR)
	{
		Outf(&InfoStream, "Bytes sent %d/%d", BytesSent, Size);
		Result = true;
	}
	else
	{
		Outf(InfoStream.Errors, "send failed! (%d)", WSAGetLastError());
	}

	return Result;
}

#if GAME_DEBUG
global debug_table GlobalDebugTable_;
debug_table *GlobalDebugTable = &GlobalDebugTable_;
#endif

int CALLBACK WinMain(HINSTANCE Instance,
                     HINSTANCE PreviousInstance,
                     LPSTR CommandLine,
                     s32 CommandShow)
{
	GetSystemInfo(&Win32State.SystemInfo);

	Win32GetExecutablePath();

	memory_region InfoStreamMemoryRegion = {};
	stream ErrorStream = CreateStream(&InfoStreamMemoryRegion, 0);
	InfoStream = CreateStream(&InfoStreamMemoryRegion, &ErrorStream);

	hotloadable_code *GameCode = &Win32State.Code[CodeType_Game];
	GameCode->Type = CodeType_Game;
	Win32BuildEXEPathFilename("steelth.dll", GameCode->DLLName, ArrayCount(GameCode->DLLName));
	Win32BuildEXEPathFilename("steelth_temp.dll", GameCode->DLLNameTemp, ArrayCount(GameCode->DLLNameTemp));
	Win32BuildEXEPathFilename("Lock.tmp", GameCode->LockName, ArrayCount(GameCode->LockName));

	Win32RegisterHotloads(&Win32State.Hotloader);

	LARGE_INTEGER CountsPerSecond;
	QueryPerformanceFrequency(&CountsPerSecond);

	DLIST_INIT(&Win32State.MemorySentinel);

	b32 WSAStartup = Win32WSAStartup();

	GlobalWindowSize = Win32GetScreenResolution();

	WNDCLASS WindowClass = {};
	WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
	WindowClass.lpfnWndProc = Win32WindowProc;
	WindowClass.hInstance = Instance;
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);;
    WindowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	WindowClass.lpszClassName = "MainWindowClass";

	RegisterClass(&WindowClass);
	HWND Window =  CreateWindowExA(0,
							 	WindowClass.lpszClassName,
		                        "Steelth",
		                        WS_OVERLAPPEDWINDOW,
		                        CW_USEDEFAULT,
		                        CW_USEDEFAULT,
		                        GlobalWindowSize.x,
		                        GlobalWindowSize.y,
		                        0,
		                        0,
		                        Instance,
		                        0);

	if(Window)
	{
		Win32ToggleFullscreen(Window);
		ShowWindow(Window, SW_SHOW);

		LPVOID BaseAddress = 0;
#if GAME_DEBUG
		BaseAddress = (LPVOID)Terabytes(2);
#endif

		Win32LoadCode(GameCode);

		game_memory GameMemory = {};
		GameMemory.InfoStream = &InfoStream;

		platform Win32Platform = {};
		Platform = &Win32Platform;

		Platform->LoadEntireFile = Win32LoadEntireFile;
		Platform->LoadEntireFileAndNullTerminate = Win32LoadEntireFileAndNullTerminate;
        Platform->OpenAllFiles = Win32OpenAllFiles;
        Platform->ReadFromFile = Win32ReadFromFile;
        Platform->WriteIntoFile = Win32WriteIntoFile;
        Platform->CloseFile = Win32CloseFile;
        Platform->AllocateMemory = Win32AllocateMemory;
        Platform->DeallocateMemory = Win32DeallocateMemory;
        Platform->AddWork = Win32AddWorkEntry;
        Platform->CompleteAllWork = Win32CompleteAllWork;
        Platform->ConnectToServer = Win32ConnectToServer;
        Platform->DisconnectFromServer = Win32DisconnectFromServer;
        Platform->SendData = Win32SendData;
        Platform->LoadHDRPNG = Win32LoadHDRPNG;
		GameMemory.Platform = Platform;

		HDC DeviceContext = GetDC(Window);
		render_settings RenderSettings = {};
		RenderSettings.Resolution = GlobalWindowSize;
		RenderSettings.ShadowMapResolution = V2i(4096, 4096);
		// RenderSettings.ShadowMapResolution = V2i(1024, 1024);
		RenderSettings.RenderScale = 1.0f;
		RenderSettings.SwapInterval = 2;
		RenderSettings.Bloom = false;

		platform_renderer *Renderer = Win32InitDefaultRenderer(Window, RenderSettings, &InfoStream);
		Renderer->Platform = Platform;

#if GAME_DEBUG
		memory_region DebugTableRegion = {};
		GlobalDebugTable->Region = &DebugTableRegion;
        GameMemory.DebugTable = GlobalDebugTable;
        Renderer->DebugTable = GlobalDebugTable;
		DEBUGSetRecording(true);
#endif

		u32 HighPriorityThreadCount = 0;
		u32 LowPriorityThreadCount = 1;
		if(Win32State.SystemInfo.dwNumberOfProcessors > 1)
		{
			HighPriorityThreadCount = Win32State.SystemInfo.dwNumberOfProcessors - 1;
			LowPriorityThreadCount = Win32State.SystemInfo.dwNumberOfProcessors / 2;
		}

		platform_work_queue HighPriorityQueue = {};
		thread_startup HighStartup = {};
		HighStartup.Queue = &HighPriorityQueue;
		Win32CreateWorkQueue(&HighStartup, HighPriorityThreadCount);

		platform_work_queue LowPriorityQueue = {};
		thread_startup LowStartup = {};
		LowStartup.Queue = &LowPriorityQueue;
		Win32CreateWorkQueue(&LowStartup, LowPriorityThreadCount);

        GameMemory.HighPriorityQueue = &HighPriorityQueue;
        GameMemory.LowPriorityQueue = &LowPriorityQueue;

        render_memory_op_queue RenderMemoryOpQueue = {};
		render_memory_op RenderMemoryOpQueueMemory[256] = {};
        InitRenderMemoryOpQueue(&RenderMemoryOpQueue, sizeof(RenderMemoryOpQueueMemory), RenderMemoryOpQueueMemory);

        GameMemory.RenderOpQueue = &RenderMemoryOpQueue;

		s32 ScreenRefreshRate = GetDeviceCaps(DeviceContext, VREFRESH);

		u32 CurrentSwapInterval = RenderSettings.SwapInterval;
		s32 RefreshRate = ScreenRefreshRate / CurrentSwapInterval;
		r32 dtForFrameTarget = 1.0f / (r32)RefreshRate;

		TIMECAPS TimeCaps = {};
		timeGetDevCaps(&TimeCaps, sizeof(TimeCaps));
		timeBeginPeriod(TimeCaps.wPeriodMin);

		game_input Input = {};

		b32 CursorIsVisible = true;
		ShowCursor(CursorIsVisible);

		LARGE_INTEGER LastFrameEndTime;
		QueryPerformanceCounter(&LastFrameEndTime);
		r32 LastFrameFPS = 1.0f / dtForFrameTarget;
		Input.LastFramedt_ = 1.0f / LastFrameFPS;

		GlobalRunning = true;
		u64 FrameCycleStart = __rdtsc();

		while(GlobalRunning)
		{
			{DEBUG_DATA_BLOCK("Platform");
				DEBUG_VALUE(&GlobalRunning);
			};

#if GAME_DEBUG
			BEGIN_BLOCK("Check Hotloads");

			Win32CheckFilesForHotload(&Win32State.Hotloader, &GameMemory, &RenderMemoryOpQueue);

			END_BLOCK();
#endif
			//
			//
			//

			BEGIN_BLOCK("Read Input");

			game_render_commands *RenderCommands = 0;
			if(Renderer->BeginFrame)
			{
				RenderCommands = Renderer->BeginFrame(Renderer, RenderSettings, &InfoStream);
			}

			if(CurrentSwapInterval != RenderSettings.SwapInterval)
			{
				CurrentSwapInterval = RenderSettings.SwapInterval;
				RefreshRate = ScreenRefreshRate / CurrentSwapInterval;

				dtForFrameTarget = 1.0f / (f32)RefreshRate;
				Input.Simdt = dtForFrameTarget;
			}

			Input.Simdt = dtForFrameTarget;
			Input.ScreenResolution = GlobalWindowSize;

			for(u32 KeyIndex = 0;
			    KeyIndex < Key_Count;
			    ++KeyIndex)
			{
				key_state *KeyState = Input.KeyStates + KeyIndex;
				KeyState->TransitionCount = 0;
				KeyState->WasPressed = false;
				KeyState->WasReleased = false;
			}

			Input.MouseWheelDelta = 0.0f;
			v2 LastMouseP = Input.MouseP;

			BEGIN_BLOCK("PeekMessage");

			MSG Message;
			while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&Message);
				if((Message.message == WM_KEYDOWN) ||
				   (Message.message == WM_KEYUP) ||
				   (Message.message == WM_SYSKEYDOWN) ||
				   (Message.message == WM_SYSKEYUP))
				{
        			Win32ProcessKeyboardInput(&Message, &Input);
				}
				else
				{
					Win32ProcessMouseInputs(&Message, &Input, GlobalWindowSize);
					DispatchMessage(&Message);
				}
			}
			Input.dMouseP = Input.MouseP - LastMouseP;

			if(Win32State.LoopRecording)
			{
				Win32RecordInput(&Input);
			}
			if(Win32State.LoopPlayback)
			{
				Win32LoopPlayback(&Input);
			}

			END_BLOCK();

			END_BLOCK();

			if(GameCode->Game.GameUpdateAndRender)
			{
				TIMED_BLOCK("GameUpdateAndRender");

				GameCode->Game.GameUpdateAndRender(&GameMemory, RenderCommands, &Input);
			}

			//
			//
			//

#if GAME_DEBUG
			BEGIN_BLOCK("Debug Frame End");

			b32 GameNeedsReloading = false;
			b32 RendererNeedsReloading = false;
			if(Win32NewDLLAvailable(GameCode))
			{
				if(Win32DLLLockIsActive(GameCode->LockName))
				{
					DEBUGSetRecording(false);
					GameNeedsReloading = true;
				}
			}

#if 0
			if(Win32NewDLLAvailable(RendererCode))
			{
				if(Win32DLLLockIsActive(RendererCode->LockName))
				{
					DEBUGSetRecording(false);
					RendererNeedsReloading = true;
				}
			}
#endif

			if(GameCode->Game.DEBUGFrameEnd)
			{
				Win32CompleteAllWork(&HighPriorityQueue);
				Win32CompleteAllWork(&LowPriorityQueue);

				memory_block_stats MemStats = {};
				BeginTicketMutex(&Win32State.MemoryMutex);
				for(win32_memory_block *Block = Win32State.MemorySentinel.Next;
					Block != &Win32State.MemorySentinel;
					Block = Block->Next)
				{
					platform_memory_block *PlatBlock = &Block->Block;
					MemStats.TotalUsed += PlatBlock->Used;
					MemStats.TotalSize += PlatBlock->Size;
					++MemStats.BlockCount;
				}
				EndTicketMutex(&Win32State.MemoryMutex);

				GameCode->Game.DEBUGFrameEnd(&GameMemory, RenderCommands, &Input, CountsPerSecond.QuadPart, &MemStats);
			}

			if(GameNeedsReloading || RendererNeedsReloading)
			{
				Win32CompleteAllWork(&HighPriorityQueue);
				Win32CompleteAllWork(&LowPriorityQueue);

				if(GameNeedsReloading)
				{
					Win32UnloadCodeDLL(GameCode);
					Win32LoadCode(GameCode);
					Outf(&InfoStream, "Game code reloaded.");
				}

#if 0
				if(RendererNeedsReloading)
				{
					Win32UnloadCodeDLL(RendererCode);
					Win32LoadCode(RendererCode);
				}
#endif

				DEBUGSetRecording(true);
			}

			END_BLOCK();
#endif

			//
			//
			//

			BEGIN_BLOCK("OpenGL Render");

			{DEBUG_DATA_BLOCK("OpenGL Renderer");
				{DEBUG_DATA_BLOCK("Settings");
					DEBUG_VALUE(&RenderSettings);
				}
			}

			RenderSettings.Resolution = GlobalWindowSize;
			if(Renderer->ProcessMemoryOpQueue)
			{
				Renderer->ProcessMemoryOpQueue(Renderer, &RenderMemoryOpQueue);
			}
			if(Renderer->EndFrame)
			{
				Renderer->EndFrame(Renderer, &Input);
			}

			END_BLOCK();

			//
			//
			//

			BEGIN_BLOCK("Finish Thread Work");
			Win32CompleteAllWork(&HighPriorityQueue);
			END_BLOCK();

			//
			//
			//

			BEGIN_BLOCK("Wait for vBlank");

			SwapBuffers(DeviceContext);

			END_BLOCK();

			//
			//
			//

			BEGIN_BLOCK("Frame Wait");

			r32 FrameTime = Win32GetTimeSince(CountsPerSecond, LastFrameEndTime);

			LastFrameFPS = (1.0f / FrameTime);
			Input.LastFramedt_ = FrameTime;
			LARGE_INTEGER CurrentTimeCounts;
			QueryPerformanceCounter(&CurrentTimeCounts);
			LastFrameEndTime = CurrentTimeCounts;

			END_BLOCK();

			//
			//
			//

			u64 LastFrameCycleStart = FrameCycleStart;
			FrameCycleStart = __rdtsc();
			u64 FrameCycles = FrameCycleStart - LastFrameCycleStart;

			FRAME_MARK(FrameCycles);
		}

		timeEndPeriod(TimeCaps.wPeriodMin);
	}

	return 0;
}
