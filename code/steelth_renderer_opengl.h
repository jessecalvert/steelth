/*@H
* File: steelth_renderer_opengl.h
* Author: Jesse Calvert
* Created: December 18, 2017, 17:08
* Last modified: April 26, 2019, 17:17
*/

#pragma once

#define GL_SHADING_LANGUAGE_VERSION      	 	0x8B8C

#define GL_FRAMEBUFFER_SRGB               		0x8DB9
#define GL_SRGB8_ALPHA8                   		0x8C43

#define GL_ARRAY_BUFFER                   0x8892
#define GL_SHADER_STORAGE_BUFFER          0x90D2

#define GL_STREAM_DRAW                    0x88E0
#define GL_STREAM_READ                    0x88E1
#define GL_STREAM_COPY                    0x88E2
#define GL_STATIC_DRAW                    0x88E4
#define GL_STATIC_READ                    0x88E5
#define GL_STATIC_COPY                    0x88E6
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_DYNAMIC_READ                   0x88E9
#define GL_DYNAMIC_COPY                   0x88EA

#define GL_COMPILE_STATUS                 0x8B81
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPUTE_SHADER                 0x91B9
#define GL_LINK_STATUS                    0x8B82

#define GL_ELEMENT_ARRAY_BUFFER           0x8893

#define GL_BGRA                           0x80E1
#define GL_UNSIGNED_INT_8_8_8_8           0x8035
#define GL_CONSTANT_ALPHA                 0x8003
#define GL_FRAMEBUFFER                    0x8D40
#define GL_RENDERBUFFER                   0x8D41
#define GL_DEPTH24_STENCIL8               0x88F0
#define GL_DEPTH_ATTACHMENT               0x8D00
#define GL_STENCIL_ATTACHMENT             0x8D20
#define GL_DEPTH_STENCIL_ATTACHMENT       0x821A
#define GL_FRAMEBUFFER_COMPLETE           0x8CD5
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_CLAMP_TO_BORDER                0x812D
#define GL_DEPTH_COMPONENT16              0x81A5
#define GL_DEPTH_COMPONENT24              0x81A6
#define GL_DEPTH_COMPONENT32              0x81A7

#define GL_RGBA32F                        0x8814
#define GL_RGB32F                         0x8815
#define GL_RGBA16F                        0x881A
#define GL_RGB16F                         0x881B

#define GL_TEXTURE0                       0x84C0
#define GL_TEXTURE1                       0x84C1
#define GL_TEXTURE2                       0x84C2
#define GL_TEXTURE3                       0x84C3
#define GL_TEXTURE4                       0x84C4
#define GL_TEXTURE5                       0x84C5
#define GL_TEXTURE6                       0x84C6
#define GL_TEXTURE7                       0x84C7
#define GL_TEXTURE8                       0x84C8
#define GL_TEXTURE9                       0x84C9
#define GL_TEXTURE10                      0x84CA
#define GL_TEXTURE11                      0x84CB
#define GL_TEXTURE12                      0x84CC
#define GL_TEXTURE13                      0x84CD
#define GL_TEXTURE14                      0x84CE
#define GL_TEXTURE15                      0x84CF
#define GL_TEXTURE16                      0x84D0
#define GL_TEXTURE17                      0x84D1
#define GL_TEXTURE18                      0x84D2
#define GL_TEXTURE19                      0x84D3
#define GL_TEXTURE20                      0x84D4
#define GL_TEXTURE21                      0x84D5
#define GL_TEXTURE22                      0x84D6
#define GL_TEXTURE23                      0x84D7
#define GL_TEXTURE24                      0x84D8
#define GL_TEXTURE25                      0x84D9
#define GL_TEXTURE26                      0x84DA
#define GL_TEXTURE27                      0x84DB
#define GL_TEXTURE28                      0x84DC
#define GL_TEXTURE29                      0x84DD
#define GL_TEXTURE30                      0x84DE
#define GL_TEXTURE31                      0x84DF

#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_COLOR_ATTACHMENT1              0x8CE1
#define GL_COLOR_ATTACHMENT2              0x8CE2
#define GL_COLOR_ATTACHMENT3              0x8CE3
#define GL_COLOR_ATTACHMENT4              0x8CE4
#define GL_COLOR_ATTACHMENT5              0x8CE5
#define GL_COLOR_ATTACHMENT6              0x8CE6
#define GL_COLOR_ATTACHMENT7              0x8CE7
#define GL_COLOR_ATTACHMENT8              0x8CE8
#define GL_COLOR_ATTACHMENT9              0x8CE9
#define GL_COLOR_ATTACHMENT10             0x8CEA
#define GL_COLOR_ATTACHMENT11             0x8CEB
#define GL_COLOR_ATTACHMENT12             0x8CEC
#define GL_COLOR_ATTACHMENT13             0x8CED
#define GL_COLOR_ATTACHMENT14             0x8CEE
#define GL_COLOR_ATTACHMENT15             0x8CEF
#define GL_COLOR_ATTACHMENT16             0x8CF0
#define GL_COLOR_ATTACHMENT17             0x8CF1
#define GL_COLOR_ATTACHMENT18             0x8CF2
#define GL_COLOR_ATTACHMENT19             0x8CF3
#define GL_COLOR_ATTACHMENT20             0x8CF4
#define GL_COLOR_ATTACHMENT21             0x8CF5
#define GL_COLOR_ATTACHMENT22             0x8CF6
#define GL_COLOR_ATTACHMENT23             0x8CF7
#define GL_COLOR_ATTACHMENT24             0x8CF8
#define GL_COLOR_ATTACHMENT25             0x8CF9
#define GL_COLOR_ATTACHMENT26             0x8CFA
#define GL_COLOR_ATTACHMENT27             0x8CFB
#define GL_COLOR_ATTACHMENT28             0x8CFC
#define GL_COLOR_ATTACHMENT29             0x8CFD
#define GL_COLOR_ATTACHMENT30             0x8CFE
#define GL_COLOR_ATTACHMENT31             0x8CFF

#define GL_READ_FRAMEBUFFER               0x8CA8
#define GL_DRAW_FRAMEBUFFER               0x8CA9

#define GL_RG                             0x8227
#define GL_RG_INTEGER                     0x8228
#define GL_R8                             0x8229
#define GL_R16                            0x822A
#define GL_RG8                            0x822B
#define GL_RG16                           0x822C
#define GL_R16F                           0x822D
#define GL_R32F                           0x822E
#define GL_RG16F                          0x822F
#define GL_RG32F                          0x8230
#define GL_R8I                            0x8231
#define GL_R8UI                           0x8232
#define GL_R16I                           0x8233
#define GL_R16UI                          0x8234
#define GL_R32I                           0x8235
#define GL_R32UI                          0x8236
#define GL_RG8I                           0x8237
#define GL_RG8UI                          0x8238
#define GL_RG16I                          0x8239
#define GL_RG16UI                         0x823A
#define GL_RG32I                          0x823B
#define GL_RG32UI                         0x823C
#define GL_SAMPLES_PASSED                 0x8914
#define GL_QUERY_RESULT                   0x8866
#define GL_QUERY_RESULT_AVAILABLE         0x8867
#define GL_QUERY_RESULT_NO_WAIT           0x9194

#define GL_DEPTH_COMPONENT32F             0x8CAC
#define GL_DEPTH32F_STENCIL8              0x8CAD
#define GL_DEPTH_STENCIL                  0x84F9
#define GL_FLOAT_32_UNSIGNED_INT_24_8_REV 0x8DAD

#define GL_STENCIL_INDEX1                 0x8D46
#define GL_STENCIL_INDEX4                 0x8D47
#define GL_STENCIL_INDEX8                 0x8D48
#define GL_STENCIL_INDEX16                0x8D49

#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 0x8B4D
#define GL_MAX_DRAW_BUFFERS               0x8824
#define GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS 0x90EB
#define GL_MAX_COMPUTE_WORK_GROUP_COUNT   0x91BE
#define GL_MAX_COMPUTE_WORK_GROUP_SIZE    0x91BF
#define GL_MAX_COMPUTE_SHARED_MEMORY_SIZE 0x8262

#define GL_TEXTURE_BUFFER_SIZE            0x919E

#define GL_READ_ONLY                  0x88B8
#define GL_WRITE_ONLY                 0x88B9
#define GL_READ_WRITE                 0x88BA

#define GL_MULTISAMPLE                    0x809D
#define GL_TEXTURE_2D_MULTISAMPLE         0x9100
#define GL_TEXTURE_2D_ARRAY               0x8C1A
#define GL_TEXTURE_3D                     0x806F
#define GL_TEXTURE_WRAP_R                 0x8072

#define GL_SHADER_STORAGE_BLOCK           0x92E6
#define GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT 0x00000001
#define GL_ELEMENT_ARRAY_BARRIER_BIT      0x00000002
#define GL_UNIFORM_BARRIER_BIT            0x00000004
#define GL_TEXTURE_FETCH_BARRIER_BIT      0x00000008
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT 0x00000020
#define GL_COMMAND_BARRIER_BIT            0x00000040
#define GL_PIXEL_BUFFER_BARRIER_BIT       0x00000080
#define GL_TEXTURE_UPDATE_BARRIER_BIT     0x00000100
#define GL_BUFFER_UPDATE_BARRIER_BIT      0x00000200
#define GL_FRAMEBUFFER_BARRIER_BIT        0x00000400
#define GL_TRANSFORM_FEEDBACK_BARRIER_BIT 0x00000800
#define GL_ATOMIC_COUNTER_BARRIER_BIT     0x00001000
#define GL_ALL_BARRIER_BITS               0xFFFFFFFF
#define GL_SHADER_STORAGE_BARRIER_BIT     0x00002000

#define GL_DEBUG_OUTPUT                   0x92E0
#define GL_DEBUG_OUTPUT_SYNCHRONOUS       0x8242
#define GL_DEBUG_SEVERITY_HIGH            0x9146
#define GL_DEBUG_SEVERITY_MEDIUM          0x9147
#define GL_DEBUG_SEVERITY_LOW             0x9148

#include "steelth_renderer_opengl_framebuffers.h"
#include "steelth_renderer_opengl_shaders.h"
#include "steelth_renderer_camera.h"
#include "steelth_renderer_opengl_bake.h"

struct opengl_info
{
	char *Vendor;
	char *Renderer;
	char *Version;
	char *ShadingLanguageVersion;
	char *Extensions;

	b32 GL_EXT_texture_sRGB;
	b32 GL_EXT_framebuffer_sRGB;

	GLint MaxSamplersPerShader;
	GLint MaxDrawbuffers;

	GLint MaxComputeWorkGroupInvocations;
	GLint MaxComputeWorkGroupCount[3];
	GLint MaxComputeWorkGroupSize[3];
	GLint MaxComputeSharedMemorySize;
};

struct opengl_mesh_info
{
	renderer_mesh Mesh;

	u32 FaceCount;
	u32 VertexCount;

	GLuint VAO;
	GLuint VBO;
	GLuint EBO;
};

#define IRRADIANCE_VOLUME_WIDTH 64
#define IRRADIANCE_VOLUME_HEIGHT 32
#define IRRADIANCE_VOXEL_DIM 3.0f

struct irradiance_volume
{
	// NOTE: One volume holds texel colors for the each of the six directions.
	//	The X size is 6 times the width to hold the data.
	//
	//  		                                X-Axis
	//          -------------------------------------------------------------------
	//	Y-Axis  |\          \          \          \          \          \          \
	//          | \------------------------------------------------------------------
	//          \ |   X +    |   X -    |   Y +    |   Y -    |    Z +    |   Z -   |
	//	  Z-Axis \-------------------------------------------------------------------
	//
	v3 *Voxels;
};

struct opengl_state
{
	platform_renderer PlatformRenderer;
	render_settings CurrentRenderSettings;
	game_render_commands RenderCommands;

	memory_region Region;
	opengl_info OpenGLInfo;
	struct stream *Info;

	render_setup *CurrentSceneRenderSetup;
	opengl_shader_common *CurrentShader;
	opengl_shader_common *Shaders[Shader_Count];

	framebuffer_type CurrentFramebuffer;
	opengl_framebuffer Framebuffers[Framebuffer_Count];

	GLuint VertexArray;
	GLuint ArrayBuffer;
	GLuint IndexBuffer;
	GLuint IrradianceVolume;

	u32 JitterIndex;
	v2 Jitter;

	GLuint BlueNoiseTexture;
	renderer_texture WhiteTexture;
	renderer_mesh Quad;
	renderer_mesh Cube;

	opengl_baked_data *BakedData;
	GLuint SurfelBuffer;
	GLuint ProbeBuffer;
	GLuint SurfelRefBuffer;

	b32 Wireframes;

	b32 ShadersLoaded;
	string ShaderFilenames[Shader_Count];

	u32 MaxTextureCount;
	GLuint TextureArray;
	u32 MaxMeshCount;
	opengl_mesh_info *Meshes;

	b32 DebugTurnOffDirectLighting;
	b32 DebugDisplayFramebuffers;
	b32 DebugReadIrradianceResults;
};

inline b32
OpenGLUniformLocationIsValid(GLint UniformLoc)
{
	b32 Result = (UniformLoc != -1);
	return Result;
}
