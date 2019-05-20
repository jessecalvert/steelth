/*@H
* File: steelth_renderer_opengl_framebuffers.h
* Author: Jesse Calvert
* Created: April 23, 2019, 16:26
* Last modified: April 26, 2019, 23:50
*/

#pragma once

enum texture_flags
{
	Texture_FilterNearest = 0x1,
	Texture_ClampToBorder = 0x2,
	Texture_ClampToEdge = 0x4,
	Texture_Float = 0x8,
	Texture_DepthBuffer = 0x10,
	Texture_Mipmap = 0x20,
	Texture_Multisample = 0x40,
};
struct texture_settings
{
	flag32(texture_flags) Flags;
	u32 Channels;
	u32 BytesPerChannel;
	u32 Samples;
	v4 BorderColor;
	u32 Width;
	u32 Height;
};

INTROSPECT()
enum framebuffer_type
{
	Framebuffer_Default,
	Framebuffer_GBuffer,
	Framebuffer_Fresnel,
	Framebuffer_Scene,

	Framebuffer_ShadowMap0,
	Framebuffer_ShadowMap1,

	Framebuffer_Overbright,
	Framebuffer_Overlay,
	Framebuffer_Blur0,
	Framebuffer_Blur1,
	Framebuffer_Fog,

	Framebuffer_Count,
};

enum framebuffer_texture_names_0
{
	FBT_Color,
	FBT_Depth,
};

enum framebuffer_texture_names_1
{
	FBT_Albedo,
	Ignored_0,
	FBT_Normal,
	FBT_Roughness_Metalness_Emission,
};

#define MAX_FRAMEBUFFER_TEXTURES 8

struct opengl_framebuffer
{
	framebuffer_type Type;
	GLuint Handle;
	v2i Resolution;

	u32 TextureCount;
	GLuint Textures[MAX_FRAMEBUFFER_TEXTURES];

	u32 SubRegionCount;
	rectangle2i SubRegions[64];
};
