/*@H
* File: render_test.cpp
* Author: Jesse Calvert
* Created: October 19, 2018, 11:39
* Last modified: April 10, 2019, 17:33
*/

#include "windows.h"

#include "render_test.h"

#include "win32_steelth_renderer.h"
#include "steelth_renderer.cpp"

global win32_state Win32State;
global b32 GlobalRunning;
global WINDOWPLACEMENT GlobalWindowPosition = { sizeof(GlobalWindowPosition) };
global v2i GlobalWindowSize;
global stream InfoStream;

//
// NOTE: Render test:
//

internal void
InitTestScene(test_scene *Scene, v2i ScreenResolution, memory_region *Region, render_memory_op_queue *Queue)
{
	Scene->Camera = {};
	Scene->Camera.Type = Camera_Perspective;
	f32 WorldNear = 0.01f;
	f32 WorldFar = 50.0f;
	f32 WorldHorizontalFOV = 90.0f*(Pi32/180.0f);
	f32 AspectRatio = (f32)ScreenResolution.x/(f32)ScreenResolution.y;

	ChangeCameraSettings(&Scene->Camera, WorldHorizontalFOV, AspectRatio, WorldNear, WorldFar);

	v3 Up = V3(0,1,0);
	v3 Forward = V3(0,0,-1);
	v3 Right = Cross(Forward, Up);
	f32 CameraDistance = 10.0f;

	v3 CameraFacingDirection = Normalize(V3(0.0f, -1.0f, -0.7f));
	v3 CameraLookAt = V3(0,0,0);
	v3 NewCameraP = CameraLookAt + -CameraDistance*CameraFacingDirection;
	v3 Z = -CameraFacingDirection;
	v3 X = NormalizeOr(Cross(Up, Z), Right);
	v3 Y = Cross(Z, X);
	quaternion NewCameraRotation = RotationQuaternion(X, Y, Z);

	MoveCamera(&Scene->Camera, NewCameraP);
	RotateCamera(&Scene->Camera, NewCameraRotation);

	u32 NextTextureHandle = 1;
	Scene->Smile = LoadPNG(Region, Queue, ConstZ("w:\\steelth\\data\\Textures\\smile.png"), NextTextureHandle++);
	Scene->Sword = LoadPNG(Region, Queue, ConstZ("w:\\steelth\\data\\Textures\\sword_icon.png"), NextTextureHandle++);
	Scene->Heart = LoadPNG(Region, Queue, ConstZ("w:\\steelth\\data\\Textures\\heart_icon.png"), NextTextureHandle++);

	random_series Series;
	RandomSeriesSeed(&Series, 1234);

	for(s32 Z = 0;
		Z < TEST_SCENE_DIM;
		++Z)
	{
		for(s32 X = 0;
			X < TEST_SCENE_DIM;
			++X)
		{
			Scene->Elements[Z][X] = (test_scene_element)RandomRangeU32(&Series, 0, Element_Count);
		}
	}
}

internal void
RenderTestUpdateAndRender(render_test_state *State, f32 Simdt, game_render_commands *RenderCommands, v2i ScreenResolution)
{
	test_scene *Scene = &State->Scene;

	//
	// NOTE: Overlay render.
	//

	render_group Group_ = BeginRender(0, RenderCommands, Target_Overlay);
	render_group *Group = &Group_;

	camera ScreenCamera = DefaultScreenCamera(ScreenResolution);

	SetCamera(Group, &ScreenCamera);
	SetRenderFlags(Group, RenderFlag_NotLighted|RenderFlag_AlphaBlended|RenderFlag_TopMost);

	PushTexture(Group, V2(100, 100), V2(100, 0), V2(0, 100), Scene->Smile);
	PushTexture(Group, V2(120, 100), V2(100, 0), V2(0, 100), Scene->Smile);
	PushTexture(Group, V2(140, 100), V2(100, 0), V2(0, 100), Scene->Smile);
	PushTexture(Group, V2(200, 100), V2(100, 0), V2(0, 100), Scene->Sword);
	PushTexture(Group, V2(300, 100), V2(100, 0), V2(0, 100), Scene->Heart);

	EndRender(Group);

	//
	// NOTE: Scene render.
	//

	v4 SceneClearColor = {1.0f, 0.2f, 0.2f, 1.0f};
	Group_ = BeginRender(0, RenderCommands, Target_Scene, SceneClearColor);
	Group = &Group_;

	v3 LightColor = V3(1.0f, 1.0f, 1.0f);
	f32 Intensity = 1.0f;
	v3 WorldLightDirection = NOZ(V3(0.5f, -1.5f, -1.0f));
	f32 LightArcDegrees = 0.0f;
	PushDirectionalLight(Group, WorldLightDirection, LightColor, Intensity, LightArcDegrees);
	PushAmbientLight(Group, 0.03f, V3(1,1,1));

	v3 Forward = V3(0,0,-1);
	f32 CameraDistance = 10.0f;
	quaternion CameraRotation = RotationQuaternion(V3(0, 1, 0), 0.5f*Simdt);
	quaternion NewCameraRotation = CameraRotation*Scene->Camera.Rotation;
	v3 CameraFacingDirection = RotateBy(Forward, NewCameraRotation);
	v3 CameraLookAt = V3(0,0,0);
	v3 NewCameraP = CameraLookAt + -CameraDistance*CameraFacingDirection;

	MoveCamera(&Scene->Camera, NewCameraP);
	RotateCamera(&Scene->Camera, NewCameraRotation);

	SetCamera(Group, &Scene->Camera);

	random_series Series = {};
	RandomSeriesSeed(&Series, 3728);

	v3 CubeDim = V3(1,1,1);
	f32 WallHeight = 2.0f;
	for(s32 Z = 0;
		Z < TEST_SCENE_DIM;
		++Z)
	{
		for(s32 X = 0;
			X < TEST_SCENE_DIM;
			++X)
		{
			test_scene_element Element = Scene->Elements[Z][X];

			v3 P = V3(CubeDim.x * (X - 0.5f*TEST_SCENE_DIM),
					  0.2f*Random01(&Series),
					  CubeDim.z * (Z - 0.5f*TEST_SCENE_DIM));
			PushCube(Group, P, CubeDim, 1.0f, 0.0f, V4(0.25f, 1.0f, 0.25f, 1.0f));

			switch(Element)
			{
				case Element_Wall:
				{
					v3 CubeP = P + V3(0, 0.5f*WallHeight, 0);
					PushCube(Group, CubeP, 0.9f*CubeDim + V3(0, WallHeight, 0), 0.2f, 0.9f, V4(1.0f, 0.25f, 0.25f, 1.0f));
				} break;

				case Element_Tree:
				{
					PushUprightTexture(Group, P + V3(0, CubeDim.y, 0), V2(1,1), Scene->Sword, 0.5f, 0.75f);
				} break;
				case Element_Ground:
				{
					u32 CoverCount = 60;
					for(u32 CoverIndex = 0;
						CoverIndex < CoverCount;
						++CoverIndex)
					{
						v3 CoverP = V3(RandomRangeF32(&Series, P.x - 0.5f*CubeDim.x, P.x + 0.5f*CubeDim.x),
									   P.y + 0.7f*CubeDim.y,
									   RandomRangeF32(&Series, P.z - 0.5f*CubeDim.z, P.z + 0.5f*CubeDim.z));
						PushUprightTexture(Group, CoverP, V2(0.2f, 0.2f), Scene->Heart, 1.0f, 0.0f);
					}
				} break;

				InvalidDefaultCase;
			}
		}
	}

	EndRender(Group);
}

internal void
InitRenderTestState(render_test_state *State, render_memory_op_queue *RenderOpQueue, v2i ScreenResolution)
{
	InitTestScene(&State->Scene, ScreenResolution, &State->Region, RenderOpQueue);
	State->Initialized = true;
}

//
// NOTE: Win32 code:
//

internal r32
Win32GetTimeSince(LARGE_INTEGER CountsPerSecond, LARGE_INTEGER OldTime)
{
	LARGE_INTEGER CurrentTime;
	QueryPerformanceCounter(&CurrentTime);
	u64 CountsSinceLast = CurrentTime.QuadPart - OldTime.QuadPart;

	r32 Result = (r32)CountsSinceLast / (r32)CountsPerSecond.QuadPart;
	return Result;
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

inline b32
Win32IsLooping()
{
	b32 Result = (Win32State.LoopRecording || Win32State.LoopPlayback);
	return Result;
}

internal
ALLOCATE_MEMORY(Win32AllocateMemory)
{
	// NOTE: we don't want the platform_memory_block to change cache line alignment
	Assert(sizeof(win32_memory_block) == 64);

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

internal void
Win32FreeBlock(win32_memory_block *Win32Block)
{
	DLIST_REMOVE(Win32Block);
	BOOL Success = VirtualFree(Win32Block, 0, MEM_RELEASE);
	Assert(Success);
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

inline u32
Win32GetFileSize(char *Filename)
{
	WIN32_FILE_ATTRIBUTE_DATA FileData = {};
	GetFileAttributesEx(Filename, GetFileExInfoStandard, &FileData);
	Assert(FileData.nFileSizeHigh == 0);
	u32 FileSize = FileData.nFileSizeLow;
	return FileSize;
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
	Result.Size = FileSize;
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
	Result.Size = FileSize;
	return Result;
}

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define FlipEndian(a) ((a << 24) | ((a << 8) & 0x00FF0000) | ((a >> 8) & 0x0000FF00) | (a >> 24))

internal renderer_texture
LoadPNG(memory_region *Region, render_memory_op_queue *RenderMemoryOpQueue, string Filename, u32 Index)
{
	buffer PNGFileData = Platform->LoadEntireFile(Region, Filename);
	Assert(PNGFileData.Contents);

	stbi_set_flip_vertically_on_load(true);
	s32 Width = 0;
	s32 Height = 0;
	s32 Channels = 0;
	u32 *LoadedPNG = (u32 *)stbi_load_from_memory(PNGFileData.Contents, (int)PNGFileData.Size, &Width, &Height,
		&Channels, 4);

	u32 PixelCount = Width * Height;
	for(u32 PixelIndex = 0;
		PixelIndex < PixelCount;
		++PixelIndex)
	{
		u32 Color = LoadedPNG[PixelIndex];
		LoadedPNG[PixelIndex] = FlipEndian(Color);
	}

	renderer_texture Result = RendererTexture(Index, Width, Height);

	render_memory_op Op = {};
	Op.Type = RenderOp_LoadTexture;
	load_texture_op *LoadTexture = &Op.LoadTexture;
	LoadTexture->Texture = Result;
	LoadTexture->Pixels = LoadedPNG;

	AddRenderOp(RenderMemoryOpQueue, &Op);

	return Result;
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

        default:
        {
            Result = DefWindowProc(Window, Message, wParam, lParam);
        }
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

	memory_region InfoStreamMemoryRegion = {};
	stream ErrorStream = CreateStream(&InfoStreamMemoryRegion, 0);
	InfoStream = CreateStream(&InfoStreamMemoryRegion, &ErrorStream);

	LARGE_INTEGER CountsPerSecond;
	QueryPerformanceFrequency(&CountsPerSecond);

	DLIST_INIT(&Win32State.MemorySentinel);

	// GlobalWindowSize = Win32GetScreenResolution();
	// GlobalWindowSize = V2i(800, 600);
	GlobalWindowSize = V2i(1280, 720);

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
		                        "Render Test!",
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
		// Win32ToggleFullscreen(Window);
		ShowWindow(Window, SW_SHOW);

		LPVOID BaseAddress = 0;
#if GAME_DEBUG
		BaseAddress = (LPVOID)Terabytes(2);
#endif

		platform Win32Platform = {};
		Platform = &Win32Platform;

		Platform->LoadEntireFile = Win32LoadEntireFile;
		Platform->LoadEntireFileAndNullTerminate = Win32LoadEntireFileAndNullTerminate;
        Platform->AllocateMemory = Win32AllocateMemory;
        Platform->DeallocateMemory = Win32DeallocateMemory;

		HDC DeviceContext = GetDC(Window);
		render_settings RenderSettings = {};
		RenderSettings.Resolution = GlobalWindowSize;
		RenderSettings.RenderScale = 1.0f;
		RenderSettings.SwapInterval = 1;
		RenderSettings.Bloom = true;
		RenderSettings.DesiredDepthPeelCount = 8;
		RenderSettings.DesiredMultisamples = 4;

		platform_renderer *Renderer = Win32InitDefaultRenderer(Window, RenderSettings, &InfoStream);
		Renderer->Platform = Platform;

#if GAME_DEBUG
		memory_region DebugTableRegion = {};
		GlobalDebugTable->Region = &DebugTableRegion;
        Renderer->DebugTable = GlobalDebugTable;
		DEBUGSetRecording(false);
#endif

        render_memory_op_queue RenderMemoryOpQueue = {};
		render_memory_op RenderMemoryOpQueueMemory[256] = {};
        InitRenderMemoryOpQueue(&RenderMemoryOpQueue, sizeof(RenderMemoryOpQueueMemory), RenderMemoryOpQueueMemory);

		s32 ScreenRefreshRate = GetDeviceCaps(DeviceContext, VREFRESH);

		u32 CurrentSwapInterval = RenderSettings.SwapInterval;
		s32 RefreshRate = ScreenRefreshRate / CurrentSwapInterval;
		r32 dtForFrameTarget = 1.0f / (r32)RefreshRate;

		TIMECAPS TimeCaps = {};
		timeGetDevCaps(&TimeCaps, sizeof(TimeCaps));
		timeBeginPeriod(TimeCaps.wPeriodMin);

		b32 CursorIsVisible = true;
		ShowCursor(CursorIsVisible);

		render_test_state RenderTestState = {};
		InitRenderTestState(&RenderTestState, &RenderMemoryOpQueue, GlobalWindowSize);

		LARGE_INTEGER LastFrameEndTime;
		QueryPerformanceCounter(&LastFrameEndTime);
		f32 LastFrameFPS = 1.0f / dtForFrameTarget;
		f32 LastFramedt_ = 1.0f / LastFrameFPS;

		u32 FrameCount = 0;
		f32 FPSAccum = 0.0f;

		GlobalRunning = true;
		while(GlobalRunning)
		{
			MSG Message;
			while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&Message);
				DispatchMessage(&Message);
			}

			game_render_commands *RenderCommands = 0;
			if(Renderer->BeginFrame)
			{
				RenderCommands = Renderer->BeginFrame(Renderer, RenderSettings, &InfoStream);
			}

			RenderTestUpdateAndRender(&RenderTestState, dtForFrameTarget, RenderCommands, GlobalWindowSize);

			RenderSettings.Resolution = GlobalWindowSize;
			if(Renderer->ProcessMemoryOpQueue)
			{
				Renderer->ProcessMemoryOpQueue(Renderer, &RenderMemoryOpQueue);
			}
			if(Renderer->EndFrame)
			{
				Renderer->EndFrame(Renderer);
			}

			SwapBuffers(DeviceContext);

			r32 FrameTime = Win32GetTimeSince(CountsPerSecond, LastFrameEndTime);

			LastFrameFPS = (1.0f / FrameTime);
			LastFramedt_ = FrameTime;
			LARGE_INTEGER CurrentTimeCounts;
			QueryPerformanceCounter(&CurrentTimeCounts);
			LastFrameEndTime = CurrentTimeCounts;

			++FrameCount;
			FPSAccum += LastFrameFPS;
			if(FrameCount > 100)
			{
				char Buffer[128];
				FormatString(Buffer, ArrayCount(Buffer), "avg fps : %f\n", (FPSAccum/FrameCount));
				OutputDebugString(Buffer);
				FrameCount = 0;
				FPSAccum = 0.0f;
			}
		}

		timeEndPeriod(TimeCaps.wPeriodMin);
	}

	return 0;
}
