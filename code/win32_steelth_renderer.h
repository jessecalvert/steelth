/*@H
* File: win32_steelth_renderer.h
* Author: Jesse Calvert
* Created: October 24, 2018, 17:00
* Last modified: April 10, 2019, 17:33
*/

#pragma once

#define WIN32_LOAD_RENDERER(Name) platform_renderer *Name(HDC WindowDC, render_settings RenderSettings, platform *Platform_, stream *Info)
typedef WIN32_LOAD_RENDERER(win32_load_renderer);

internal win32_load_renderer *
Win32GetDefaultRenderer()
{
	HMODULE RendererDLL = LoadLibraryEx("win32_steelth_opengl.dll", 0, 0);
	win32_load_renderer *Result = (win32_load_renderer *)GetProcAddress(RendererDLL,
		"Win32LoadOpenGLRenderer");
	return Result;
}

internal platform_renderer *
Win32InitDefaultRenderer(HWND Window, render_settings RenderSettings, stream *Info)
{
	win32_load_renderer *Win32LoadRenderer = Win32GetDefaultRenderer();
	Assert(Win32LoadRenderer);
	platform_renderer *Renderer = Win32LoadRenderer(GetDC(Window), RenderSettings, Platform, Info);
	return Renderer;
}
