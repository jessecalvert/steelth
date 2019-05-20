/*@H
* File: steelth_renderer_opengl_shaders.h
* Author: Jesse Calvert
* Created: April 23, 2019, 16:10
* Last modified: April 27, 2019, 18:30
*/

#pragma once

#define CASCADE_COUNT 4
struct light_space_cascades
{
	f32 FarPlane[CASCADE_COUNT];
	mat4 Projection[CASCADE_COUNT];
	mat4 View[CASCADE_COUNT];
	b32 Orthogonal[CASCADE_COUNT];
	f32 CascadeBounds[CASCADE_COUNT + 1];
};

struct opengl_shader_common
{
	shader_type Type;
	GLuint Handle;

	GLint Transform;

	u32 SamplerCount;
	GLint Samplers[32];
};

struct light_shader_data
{
	GLint IsDirectional;
	GLint Color;
	GLint Intensity;

	GLint Direction;
	GLint FuzzFactor;
	GLint P;
	GLint Size;
	GLint LinearAttenuation;
	GLint QuadraticAttenuation;
	GLint Radius;
};

struct decal_shader_data
{
	GLint Projection;
	GLint View;
};

struct shader_gbuffer
{
	opengl_shader_common Common;

	GLint Projection;
	GLint View;
	GLint Model;

	GLint NormalMatrix;
	GLint MeshTextureIndex;

	GLint Jitter;
	GLint FarPlane;
	GLint Orthogonal;

	GLint Roughness;
	GLint Metalness;
	GLint Emission;
	GLint Color;
};

struct shader_shadow_map
{
	opengl_shader_common Common;

	GLint Projection;
	GLint View;
	GLint Model;

	GLint FarPlane;
	GLint Orthogonal;
};

struct shader_bake_probe
{
	opengl_shader_common Common;

	GLint Projection;
	GLint View;

	GLint MeshTextureIndex;

	GLint FarPlane;
	GLint Orthogonal;
};

struct shader_skybox
{
	opengl_shader_common Common;

	GLint SkyRadiance;
};

struct shader_fresnel
{
	opengl_shader_common Common;

	GLint ViewRay;
};

struct shader_light_pass
{
	opengl_shader_common Common;

	GLint Projection;
	GLint View;
	GLint Model;

	GLint InvView;
	GLint ViewRay;

	light_shader_data LightData;
	GLint CastsShadow;

	GLint LightProjection;
	GLint LightView;
	GLint LightFarPlane;
	GLint CascadeBounds;
	GLint SMRegionScale;
	GLint SMRegionOffset;
};

struct shader_indirect_light_pass
{
	opengl_shader_common Common;

	GLint ViewRay;
	GLint InvView;
	GLint SkyRadiance;
};

struct shader_decal
{
	opengl_shader_common Common;

	GLint Projection;
	GLint View;
	GLint Model;

	GLint ViewRay;
	GLint InvView;

	decal_shader_data DecalData;
	GLint DecalTextureIndex;
	GLint DecalTextureScale;
};

struct shader_overlay
{
	opengl_shader_common Common;

	GLint IsSDFTexture;
};

struct shader_overbright
{
	opengl_shader_common Common;
};

struct shader_blur
{
	opengl_shader_common Common;

	GLint Kernel;
};

struct shader_fog
{
	opengl_shader_common Common;

	GLint ViewRay;
	GLint InvView;
	GLint View;

	GLint SkyRadiance;

	light_shader_data LightData;

	GLint LightProjection;
	GLint LightView;
	GLint LightFarPlane;
	GLint CascadeBounds;
	GLint SMRegionScale;
	GLint SMRegionOffset;
};

struct shader_fog_resolve
{
	opengl_shader_common Common;
};

struct shader_finalize
{
	opengl_shader_common Common;
};

struct shader_debug_view
{
	opengl_shader_common Common;

	GLint LOD;
};

struct shader_surfels
{
	opengl_shader_common Common;

	GLint Initialize;

	GLint View;

	light_shader_data LightData;
	GLint SkyRadiance;

	GLint LightProjection;
	GLint LightView;
	GLint LightFarPlane;
	GLint CascadeBounds;
	GLint SMRegionScale;
	GLint SMRegionOffset;
};

struct shader_probes
{
	opengl_shader_common Common;
};

struct shader_irradiance_volume
{
	opengl_shader_common Common;

	GLint ProbeCount;
	GLint Center;
};
