/*@H
* File: win32_steelth_opengl.cpp
* Author: Jesse Calvert
* Created: February 2, 2018, 18:19
* Last modified: April 26, 2019, 17:18
*/

#include "windows.h"
#include "GL/gl.h"

#include "steelth.h"
#include "steelth_renderer.h"

#include "win32_steelth_renderer.h"

typedef const char * WINAPI wgl_get_extensions_string_ext();
global wgl_get_extensions_string_ext *wglGetExtensionsStringEXT;

typedef BOOL WINAPI wgl_choose_pixel_format_arb(HDC hdc,
                                                const int *piAttribIList,
                                                const FLOAT *pfAttribFList,
                                                UINT nMaxFormats,
                                                int *piFormats,
                                                UINT *nNumFormats);
global wgl_choose_pixel_format_arb *wglChoosePixelFormatARB;

typedef BOOL WINAPI wgl_swap_interval_ext(int Interval);
global wgl_swap_interval_ext *wglSwapIntervalEXT;

typedef HGLRC WINAPI wgl_create_context_attribs_arb(HDC DeviceContext, HGLRC ShareContext,
                                                    const int *Attribs);
global wgl_create_context_attribs_arb *wglCreateContextAttribsARB;

#if GAME_DEBUG
debug_table *GlobalDebugTable;
#endif

#include "GL/gl.h"

typedef ptrdiff_t GLsizeiptr;
typedef char GLchar;

typedef void type_glGenBuffers(GLsizei n, GLuint *buffers);
typedef void type_glBindBuffer(GLenum target, GLuint buffer);
typedef void type_glBufferData(GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage);
typedef GLuint type_glCreateShader(GLenum shaderType);
typedef void type_glShaderSource(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
typedef void type_glCompileShader(GLuint shader);
typedef void type_glGetShaderiv(GLuint shader, GLenum pname, GLint *params);
typedef void type_glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
typedef GLuint type_glCreateProgram(void);
typedef void type_glAttachShader(GLuint program, GLuint shader);
typedef void type_glLinkProgram(GLuint program);
typedef void type_glGetProgramiv(GLuint program, GLenum pname, GLint *params);
typedef void type_glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
typedef void type_glUseProgram(GLuint program);
typedef void type_glDeleteShader(GLuint shader);
typedef void type_glGenVertexArrays(GLsizei n, GLuint *arrays);
typedef void type_glBindVertexArray(GLuint array);
typedef void type_glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer);
typedef void type_glEnableVertexAttribArray(GLuint index);
typedef GLint type_glGetUniformLocation(GLuint program, const GLchar *name);
typedef void type_glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void type_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void type_glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void type_glUniform3fv(GLint location, GLsizei count, const GLfloat *value);
typedef void type_glUniform1f(GLint location, GLfloat v0);
typedef void type_glUniform1i(GLint location, GLint v0);
typedef void type_glGenerateMipmap(GLuint Target);
typedef void type_glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void type_glUniform4fv(GLint location, GLsizei count, const GLfloat *value);
typedef void type_glGenFramebuffers(GLsizei n, GLuint *ids);
typedef void type_glBindFramebuffer(GLenum target, GLuint framebuffer);
typedef void type_glGenRenderbuffers(GLsizei n, GLuint * renderbuffers);
typedef void type_glBindRenderbuffer(GLenum target, GLuint renderbuffer);
typedef void type_glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void type_glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void type_glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef GLenum type_glCheckFramebufferStatus(GLenum target);
typedef void type_glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level);
typedef void type_glActiveTexture(GLenum texture);
typedef void type_glUniform1fv(GLint location, GLsizei count, const GLfloat *value);
typedef void type_glUniform2fv(GLint location, GLsizei count, const GLfloat *value);
typedef void type_glDrawBuffers(GLsizei n, const GLenum *bufs);
typedef void type_glUniform2f(GLint location, GLfloat v0, GLfloat v1);
typedef void type_glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
typedef void type_glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void type_glGenQueries(GLsizei n, GLuint * ids);
typedef void type_glBeginQuery(GLenum target, GLuint id);
typedef void type_glEndQuery(GLenum target);
typedef void type_glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint * params);
typedef void type_glClearBufferfv(GLenum buffer, GLint drawbuffer, GLfloat * value);
typedef void type_glDeleteBuffers(GLsizei n, const GLuint * buffers);
typedef void type_glDeleteVertexArrays(GLsizei n, const GLuint *arrays);
typedef void type_glDeleteFramebuffers(GLsizei n, const GLuint *arrays);
typedef void type_glDeleteProgram(GLuint program);
typedef void type_glTexImage2DMultisample(GLenum target, GLsizei samples, GLint internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
typedef void type_glUniform2i(GLint location, GLint v0, GLint v1);
typedef void type_glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid * data);
typedef void type_glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
typedef void type_glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid * pointer);
typedef void type_glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid * data);
typedef void type_glGetIntegeri_v(GLenum pname, GLuint index, GLint * data);
typedef void type_glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);
typedef void type_glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);
typedef GLuint type_glGetProgramResourceIndex(GLuint program, GLenum programInterface, const char * name);
typedef void type_glShaderStorageBlockBinding(GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding);
typedef void type_glBindBufferBase(GLenum target, GLuint index, GLuint buffer);
typedef void type_glMemoryBarrier(GLbitfield barriers);
typedef void *type_glMapBuffer(GLenum target, GLenum access);
typedef GLboolean type_glUnmapBuffer(GLenum target);
typedef void type_glGetTextureImage(GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels);
typedef void type_glGetTextureLevelParameteriv(GLuint texture, GLint level, GLenum pname, GLint *params);
typedef void type_glTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);

#define OPENGL_DEBUG_CALLBACK(Name) void WINAPI Name(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, void *userParam)
typedef OPENGL_DEBUG_CALLBACK(opengl_debug_callback);
typedef void type_glDebugMessageCallback(opengl_debug_callback *callback, const void *userParam);


#define OpenGLFunctionDeclaration(Name) global type_##Name *Name;
OpenGLFunctionDeclaration(glGenBuffers);
OpenGLFunctionDeclaration(glBindBuffer);
OpenGLFunctionDeclaration(glBufferData);
OpenGLFunctionDeclaration(glCreateShader);
OpenGLFunctionDeclaration(glShaderSource);
OpenGLFunctionDeclaration(glCompileShader);
OpenGLFunctionDeclaration(glGetShaderiv);
OpenGLFunctionDeclaration(glGetShaderInfoLog);
OpenGLFunctionDeclaration(glCreateProgram);
OpenGLFunctionDeclaration(glAttachShader);
OpenGLFunctionDeclaration(glLinkProgram);
OpenGLFunctionDeclaration(glGetProgramiv);
OpenGLFunctionDeclaration(glGetProgramInfoLog);
OpenGLFunctionDeclaration(glUseProgram);
OpenGLFunctionDeclaration(glDeleteShader);
OpenGLFunctionDeclaration(glGenVertexArrays);
OpenGLFunctionDeclaration(glBindVertexArray);
OpenGLFunctionDeclaration(glVertexAttribPointer);
OpenGLFunctionDeclaration(glEnableVertexAttribArray);
OpenGLFunctionDeclaration(glGetUniformLocation);
OpenGLFunctionDeclaration(glUniform4f);
OpenGLFunctionDeclaration(glUniformMatrix4fv);
OpenGLFunctionDeclaration(glUniform3f);
OpenGLFunctionDeclaration(glUniform3fv);
OpenGLFunctionDeclaration(glUniform1f);
OpenGLFunctionDeclaration(glUniform1i);
OpenGLFunctionDeclaration(glGenerateMipmap);
OpenGLFunctionDeclaration(glUniformMatrix3fv);
OpenGLFunctionDeclaration(glUniform4fv);
OpenGLFunctionDeclaration(glGenFramebuffers);
OpenGLFunctionDeclaration(glBindFramebuffer);
OpenGLFunctionDeclaration(glGenRenderbuffers);
OpenGLFunctionDeclaration(glBindRenderbuffer);
OpenGLFunctionDeclaration(glRenderbufferStorage);
OpenGLFunctionDeclaration(glFramebufferTexture2D);
OpenGLFunctionDeclaration(glFramebufferRenderbuffer);
OpenGLFunctionDeclaration(glCheckFramebufferStatus);
OpenGLFunctionDeclaration(glFramebufferTexture);
OpenGLFunctionDeclaration(glActiveTexture);
OpenGLFunctionDeclaration(glUniform1fv);
OpenGLFunctionDeclaration(glUniform2fv);
OpenGLFunctionDeclaration(glDrawBuffers);
OpenGLFunctionDeclaration(glUniform2f);
OpenGLFunctionDeclaration(glBlitFramebuffer);
OpenGLFunctionDeclaration(glFramebufferTexture1D);
OpenGLFunctionDeclaration(glGenQueries);
OpenGLFunctionDeclaration(glBeginQuery);
OpenGLFunctionDeclaration(glEndQuery);
OpenGLFunctionDeclaration(glGetQueryObjectuiv);
OpenGLFunctionDeclaration(glClearBufferfv);
OpenGLFunctionDeclaration(glDeleteBuffers);
OpenGLFunctionDeclaration(glDeleteVertexArrays);
OpenGLFunctionDeclaration(glDeleteFramebuffers);
OpenGLFunctionDeclaration(glDeleteProgram);
OpenGLFunctionDeclaration(glTexImage2DMultisample);
OpenGLFunctionDeclaration(glUniform2i);
OpenGLFunctionDeclaration(glTexSubImage3D);
OpenGLFunctionDeclaration(glTexStorage3D);
OpenGLFunctionDeclaration(glVertexAttribIPointer);
OpenGLFunctionDeclaration(glDebugMessageCallback);
OpenGLFunctionDeclaration(glTexImage3D);
OpenGLFunctionDeclaration(glGetIntegeri_v);
OpenGLFunctionDeclaration(glDispatchCompute);
OpenGLFunctionDeclaration(glBindImageTexture);
OpenGLFunctionDeclaration(glGetProgramResourceIndex);
OpenGLFunctionDeclaration(glShaderStorageBlockBinding);
OpenGLFunctionDeclaration(glBindBufferBase);
OpenGLFunctionDeclaration(glMemoryBarrier);
OpenGLFunctionDeclaration(glMapBuffer);
OpenGLFunctionDeclaration(glUnmapBuffer);
OpenGLFunctionDeclaration(glGetTextureImage);
OpenGLFunctionDeclaration(glGetTextureLevelParameteriv);
OpenGLFunctionDeclaration(glTextureSubImage2D);

internal b32
Win32SetSwapInterval(u32 SwapInterval)
{
	b32 Set = false;
	if(wglSwapIntervalEXT)
	{
		wglSwapIntervalEXT(SwapInterval);
		Set = true;
	}

	return Set;
}

#define WGL_DRAW_TO_WINDOW_ARB                  0x2001
#define WGL_ACCELERATION_ARB                    0x2003
#define WGL_SUPPORT_OPENGL_ARB                  0x2010
#define WGL_DOUBLE_BUFFER_ARB                   0x2011
#define WGL_PIXEL_TYPE_ARB                      0x2013

#define WGL_TYPE_RGBA_ARB                       0x202B
#define WGL_FULL_ACCELERATION_ARB               0x2027

#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB        0x20A9

#define WGL_COLOR_BITS_ARB                      0x2014
#define WGL_DEPTH_BITS_ARB                      0x2022
#define WGL_STENCIL_BITS_ARB                    0x2023

#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define WGL_SAMPLE_BUFFERS_ARB            0x2041
#define WGL_SAMPLES_ARB                   0x2042

internal void
Win32SetPixelFormat(HDC DeviceContext)
{
	s32 SuggestedPixelFormatIndex = 0;
	GLuint ExtendedPick = 0;

	if(wglChoosePixelFormatARB)
	{
		s32 AttributeList[] =
		{
		    WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
		    WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		    WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		    WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		    WGL_COLOR_BITS_ARB, 32,
		    WGL_DEPTH_BITS_ARB, 24,
		    WGL_STENCIL_BITS_ARB, 8,
            WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
            WGL_SAMPLE_BUFFERS_ARB, 1,
            WGL_SAMPLES_ARB, 8,
		    0,
		};

		wglChoosePixelFormatARB(DeviceContext, AttributeList, 0, 1, &SuggestedPixelFormatIndex, &ExtendedPick);
	}

	if(!ExtendedPick)
	{
		PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
		DesiredPixelFormat.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		DesiredPixelFormat.nVersion = 1;
		DesiredPixelFormat.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
		DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
		DesiredPixelFormat.cColorBits = 32;
		DesiredPixelFormat.cDepthBits = 24;
		DesiredPixelFormat.cStencilBits = 8;

		SuggestedPixelFormatIndex = ChoosePixelFormat(DeviceContext, &DesiredPixelFormat);
	}

	PIXELFORMATDESCRIPTOR ChosenPixelFormatDescription;
	DescribePixelFormat(DeviceContext, SuggestedPixelFormatIndex, sizeof(PIXELFORMATDESCRIPTOR), &ChosenPixelFormatDescription);

	SetPixelFormat(DeviceContext, SuggestedPixelFormatIndex, &ChosenPixelFormatDescription);
}

internal void
Win32LoadOpenGLExtensions()
{
	WNDCLASS WindowClass = {};
	WindowClass.style = CS_OWNDC;
	WindowClass.lpfnWndProc = DefWindowProcA;
	WindowClass.hInstance = GetModuleHandle(0);
	WindowClass.lpszClassName = "WGLExtensionsLoader";

	RegisterClass(&WindowClass);
	HWND DummyWindow =  CreateWindow("WGLExtensionsLoader",
			                         "WGLDummy",
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
		HDC DummyDeviceContext = GetDC(DummyWindow);
		Win32SetPixelFormat(DummyDeviceContext);

		HGLRC DummyRenderingContext = wglCreateContext(DummyDeviceContext);
		b32 DummyContextCreated = wglMakeCurrent(DummyDeviceContext, DummyRenderingContext);
		if(DummyContextCreated)
		{
			wglChoosePixelFormatARB = (wgl_choose_pixel_format_arb *)
				wglGetProcAddress("wglChoosePixelFormatARB)");
			wglCreateContextAttribsARB = (wgl_create_context_attribs_arb *)
				wglGetProcAddress("wglCreateContextAttribsARB");
			wglSwapIntervalEXT = (wgl_swap_interval_ext *)
				wglGetProcAddress("wglSwapIntervalEXT");
        	wglGetExtensionsStringEXT = (wgl_get_extensions_string_ext *)
        		wglGetProcAddress("wglGetExtensionsStringEXT");
			wglSwapIntervalEXT = wglSwapIntervalEXT;

        	if(wglGetExtensionsStringEXT)
        	{
        		// TODO: Check extensions.
        		char *Extensions = (char *)wglGetExtensionsStringEXT();

				#define Win32GetOpenGLFunction(Name) Name = (type_##Name *)wglGetProcAddress(#Name)
        		Win32GetOpenGLFunction(glGenBuffers);
        		Win32GetOpenGLFunction(glBindBuffer);
        		Win32GetOpenGLFunction(glBufferData);
        		Win32GetOpenGLFunction(glCreateShader);
        		Win32GetOpenGLFunction(glShaderSource);
        		Win32GetOpenGLFunction(glCompileShader);
        		Win32GetOpenGLFunction(glGetShaderiv);
        		Win32GetOpenGLFunction(glGetShaderInfoLog);
        		Win32GetOpenGLFunction(glCreateProgram);
        		Win32GetOpenGLFunction(glAttachShader);
        		Win32GetOpenGLFunction(glLinkProgram);
        		Win32GetOpenGLFunction(glGetProgramiv);
        		Win32GetOpenGLFunction(glGetProgramInfoLog);
        		Win32GetOpenGLFunction(glUseProgram);
        		Win32GetOpenGLFunction(glDeleteShader);
        		Win32GetOpenGLFunction(glGenVertexArrays);
        		Win32GetOpenGLFunction(glBindVertexArray);
        		Win32GetOpenGLFunction(glVertexAttribPointer);
        		Win32GetOpenGLFunction(glEnableVertexAttribArray);
        		Win32GetOpenGLFunction(glGetUniformLocation);
        		Win32GetOpenGLFunction(glUniform4f);
        		Win32GetOpenGLFunction(glUniformMatrix4fv);
        		Win32GetOpenGLFunction(glUniform3f);
        		Win32GetOpenGLFunction(glUniform3fv);
        		Win32GetOpenGLFunction(glUniform4fv);
        		Win32GetOpenGLFunction(glUniform1f);
        		Win32GetOpenGLFunction(glUniform1i);
        		Win32GetOpenGLFunction(glGenerateMipmap);
        		Win32GetOpenGLFunction(glUniformMatrix3fv);
        		Win32GetOpenGLFunction(glGenFramebuffers);
        		Win32GetOpenGLFunction(glBindFramebuffer);
        		Win32GetOpenGLFunction(glGenRenderbuffers);
        		Win32GetOpenGLFunction(glBindRenderbuffer);
        		Win32GetOpenGLFunction(glRenderbufferStorage);
        		Win32GetOpenGLFunction(glFramebufferTexture2D);
        		Win32GetOpenGLFunction(glFramebufferRenderbuffer);
        		Win32GetOpenGLFunction(glCheckFramebufferStatus);
        		Win32GetOpenGLFunction(glFramebufferTexture);
        		Win32GetOpenGLFunction(glActiveTexture);
        		Win32GetOpenGLFunction(glUniform1fv);
        		Win32GetOpenGLFunction(glUniform2fv);
        		Win32GetOpenGLFunction(glDrawBuffers);
        		Win32GetOpenGLFunction(glUniform2f);
        		Win32GetOpenGLFunction(glBlitFramebuffer);
        		Win32GetOpenGLFunction(glFramebufferTexture1D);
        		Win32GetOpenGLFunction(glGenQueries);
        		Win32GetOpenGLFunction(glBeginQuery);
        		Win32GetOpenGLFunction(glEndQuery);
        		Win32GetOpenGLFunction(glGetQueryObjectuiv);
        		Win32GetOpenGLFunction(glClearBufferfv);
        		Win32GetOpenGLFunction(glDeleteBuffers);
        		Win32GetOpenGLFunction(glDeleteVertexArrays);
        		Win32GetOpenGLFunction(glDeleteFramebuffers);
        		Win32GetOpenGLFunction(glDeleteProgram);
        		Win32GetOpenGLFunction(glTexImage2DMultisample);
        		Win32GetOpenGLFunction(glUniform2i);
        		Win32GetOpenGLFunction(glTexSubImage3D);
        		Win32GetOpenGLFunction(glTexStorage3D);
        		Win32GetOpenGLFunction(glVertexAttribIPointer);
        		Win32GetOpenGLFunction(glDebugMessageCallback);
        		Win32GetOpenGLFunction(glTexImage3D);
        		Win32GetOpenGLFunction(glGetIntegeri_v);
        		Win32GetOpenGLFunction(glDispatchCompute);
        		Win32GetOpenGLFunction(glBindImageTexture);
        		Win32GetOpenGLFunction(glGetProgramResourceIndex);
        		Win32GetOpenGLFunction(glShaderStorageBlockBinding);
        		Win32GetOpenGLFunction(glBindBufferBase);
        		Win32GetOpenGLFunction(glMemoryBarrier);
        		Win32GetOpenGLFunction(glMapBuffer);
        		Win32GetOpenGLFunction(glUnmapBuffer);
        		Win32GetOpenGLFunction(glGetTextureImage);
        		Win32GetOpenGLFunction(glGetTextureLevelParameteriv);
        		Win32GetOpenGLFunction(glTextureSubImage2D);
        	}

			wglMakeCurrent(0, 0);
		}

		wglDeleteContext(DummyRenderingContext);
	    ReleaseDC(DummyWindow, DummyDeviceContext);
		DestroyWindow(DummyWindow);
	}
}

internal b32
Win32InitOpenGLWithExtensions(HDC DeviceContext)
{
	b32 Result = false;
	Win32SetPixelFormat(DeviceContext);

	// NOTE: Init modern OpenGL context.
	s32 ContextAttributeList[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
    	WGL_CONTEXT_MINOR_VERSION_ARB, 3,
    	WGL_CONTEXT_FLAGS_ARB, 0 // NOTE: Enable for testing WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
#if GAME_DEBUG
    	|WGL_CONTEXT_DEBUG_BIT_ARB
#endif
    	,
    	WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
		0,
	};

	if(wglCreateContextAttribsARB)
	{
		HGLRC RenderingContext = wglCreateContextAttribsARB(DeviceContext, 0, ContextAttributeList);
		Result = wglMakeCurrent(DeviceContext, RenderingContext);
	}

	return Result;
}

internal void
InitOpenGLContext(HDC DeviceContext)
{
	Win32LoadOpenGLExtensions();
	b32 Success = Win32InitOpenGLWithExtensions(DeviceContext);
	Assert(Success);
}

#include "steelth_renderer_opengl.h"
#include "steelth_renderer_opengl.cpp"

RENDERER_PROCESS_MEMORY_OP_QUEUE(ProcessMemoryOpQueue)
{
	opengl_state *State = (opengl_state *)Renderer;
	Assert(State);
	OpenGLProcessMemoryOpQueue(State, RenderOpQueue);
}

RENDERER_BEGIN_FRAME(BeginFrame)
{
#if GAME_DEBUG
	GlobalDebugTable = Renderer->DebugTable;
#endif

	Platform = Renderer->Platform;

	opengl_state *State = (opengl_state *)Renderer;
	Assert(State);
	OpenGLBeginFrame(State, &RenderSettings);

	if(State->CurrentRenderSettings.SwapInterval != RenderSettings.SwapInterval)
	{
		if(wglSwapIntervalEXT)
		{
			wglSwapIntervalEXT(RenderSettings.SwapInterval);
			State->CurrentRenderSettings.SwapInterval = RenderSettings.SwapInterval;
		}
	}

	game_render_commands *RenderCommands = &State->RenderCommands;
	RenderCommands->Settings = RenderSettings;
	for(u32 TargetIndex = 0;
		TargetIndex < ArrayCount(RenderCommands->Size);
		++TargetIndex)
	{
		RenderCommands->Size[TargetIndex] = 0;
	}
	RenderCommands->VertexCount = 0;
	RenderCommands->IndexCount = 0;
	RenderCommands->MeshCount = 0;
	RenderCommands->LightCount = 0;
	RenderCommands->DecalCount = 0;
	RenderCommands->LightingSolution = 0;
	RenderCommands->Fullbright = false;

	return RenderCommands;
}

RENDERER_END_FRAME(EndFrame)
{
	opengl_state *State = (opengl_state *)Renderer;
	Assert(State);
	OpenGLEndFrame(State, Input);

	// TODO: This is just to help timing for profiling reasons. With this, we wait
	//	until opengl is done rendering before we move to the next frame.
	glFinish();
}

extern "C" WIN32_LOAD_RENDERER(Win32LoadOpenGLRenderer)
{
	Platform = Platform_;

	InitOpenGLContext(WindowDC);

	Assert(wglSwapIntervalEXT);
	{
		wglSwapIntervalEXT(RenderSettings.SwapInterval);
	}

	opengl_state *State = InitOpenGL(Info);

	game_render_commands *RenderCommands = &State->RenderCommands;

    // TODO: Pass these in.
    u32 MaxQuads = 65536;
    u32 MaxMeshes = 256;
    u32 MaxLights = 256;
    u32 MaxDecals = 256;

	for(u32 TargetIndex = 0;
		TargetIndex < ArrayCount(RenderCommands->MaxSize);
		++TargetIndex)
	{
    	RenderCommands->MaxSize[TargetIndex] = Megabytes(4);
    	RenderCommands->Buffer[TargetIndex] = PushSize(&State->Region, RenderCommands->MaxSize[TargetIndex]);
    }

    RenderCommands->MaxVertexCount = 4*MaxQuads;
    RenderCommands->MaxIndexCount = 6*MaxQuads;
    RenderCommands->MaxMeshCount = MaxMeshes;
    RenderCommands->MaxLights = MaxLights;
    RenderCommands->MaxDecals = MaxDecals;

    RenderCommands->VertexArray = PushArray(&State->Region, RenderCommands->MaxVertexCount, vertex_format);
    RenderCommands->IndexArray = PushArray(&State->Region, RenderCommands->MaxIndexCount, u32);
	RenderCommands->MeshArray = PushArray(&State->Region, RenderCommands->MaxMeshCount, render_command_mesh_data);
	RenderCommands->Lights = PushArray(&State->Region, RenderCommands->MaxLights, light);
	RenderCommands->Decals = PushArray(&State->Region, RenderCommands->MaxDecals, decal);

    RenderCommands->WhiteTexture = State->WhiteTexture;

	platform_renderer *Renderer = (platform_renderer *)State;
    Renderer->ProcessMemoryOpQueue = ProcessMemoryOpQueue;
    Renderer->BeginFrame = BeginFrame;
    Renderer->EndFrame = EndFrame;

	return Renderer;
}
