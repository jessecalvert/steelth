/*@H
* File: steelth_renderer_opengl_shaders.cpp
* Author: Jesse Calvert
* Created: April 23, 2019, 16:08
* Last modified: April 27, 2019, 18:29
*/

internal void
OpenGLSetShaderFilenames(opengl_state *State)
{
	State->ShaderFilenames[Shader_Header] = ConstZ("Shaders/steelth_header.glsl");
	State->ShaderFilenames[Shader_GBuffer] = ConstZ("Shaders/steelth_gbuffer.glsl");
	State->ShaderFilenames[Shader_Skybox] = ConstZ("Shaders/steelth_skybox.glsl");
	State->ShaderFilenames[Shader_Fresnel] = ConstZ("Shaders/steelth_fresnel.glsl");
	State->ShaderFilenames[Shader_LightPass] = ConstZ("Shaders/steelth_light_pass.glsl");
	State->ShaderFilenames[Shader_IndirectLightPass] = ConstZ("Shaders/steelth_indirect_light_pass.glsl");
	State->ShaderFilenames[Shader_Decal] = ConstZ("Shaders/steelth_decal.glsl");
	State->ShaderFilenames[Shader_Overlay] = ConstZ("Shaders/steelth_overlay.glsl");
	State->ShaderFilenames[Shader_Overbright] = ConstZ("Shaders/steelth_overbright.glsl");
	State->ShaderFilenames[Shader_Blur] = ConstZ("Shaders/steelth_blur.glsl");
	State->ShaderFilenames[Shader_Finalize] = ConstZ("Shaders/steelth_finalize.glsl");
	State->ShaderFilenames[Shader_DebugView] = ConstZ("Shaders/steelth_debug_view.glsl");
	State->ShaderFilenames[Shader_Surfels] = ConstZ("Shaders/steelth_surfels.glsl");
	State->ShaderFilenames[Shader_Probes] = ConstZ("Shaders/steelth_probes.glsl");
	State->ShaderFilenames[Shader_Irradiance_Volume] = ConstZ("Shaders/steelth_irradiance_volume.glsl");
	State->ShaderFilenames[Shader_BakeProbe] = ConstZ("Shaders/steelth_bake_probe.glsl");
	State->ShaderFilenames[Shader_Fog] = ConstZ("Shaders/steelth_fog.glsl");
	State->ShaderFilenames[Shader_FogResolve] = ConstZ("Shaders/steelth_fog_resolve.glsl");
	State->ShaderFilenames[Shader_ShadowMap] = ConstZ("Shaders/steelth_shadow_map.glsl");
}

internal GLuint
OpenGLCompileShader(opengl_state *State, char **ShaderSource, u32 ShaderSourceCount, GLenum ShaderType, shader_type Type)
{
	GLuint Result = 0;

	GLuint ShaderHandle = glCreateShader(ShaderType);
	glShaderSource(ShaderHandle, ShaderSourceCount, (const GLchar **)ShaderSource, 0);
	glCompileShader(ShaderHandle);

	GLint Compiled;
	GLchar InfoLog[512];
	glGetShaderiv(ShaderHandle, GL_COMPILE_STATUS, &Compiled);
	if(!Compiled)
	{
		glGetShaderInfoLog(ShaderHandle, ArrayCount(InfoLog), 0, InfoLog);

#if GAME_DEBUG
		char *ShaderTypeName = 0;
		if(ShaderType == GL_VERTEX_SHADER) {ShaderTypeName = "Vertex";}
		else if(ShaderType == GL_FRAGMENT_SHADER) {ShaderTypeName = "Fragment";}
		else if(ShaderType == GL_COMPUTE_SHADER) {ShaderTypeName = "Compute";}

		Outf(State->Info->Errors, "Failed to load shader '%s', shader type: %s\n%s",
			WrapZ(Names_shader_type[Type]),
			WrapZ(ShaderTypeName),
			WrapZ(InfoLog));

#endif

		Assert(State->ShadersLoaded);
	}
	else
	{
		Result = ShaderHandle;
	}

	return Result;
}

internal void
OpenGLGetShaderDefines(opengl_state *State, char *Buffer, u32 Length, shader_type Shader)
{
	switch(Shader)
	{
		case Shader_LightPass:
		{
			FormatString(Buffer, Length,
				"#define CASCADE_COUNT %d\n", CASCADE_COUNT);
		} break;

		case Shader_Fog:
		{
			FormatString(Buffer, Length,
				"#define CASCADE_COUNT %d\n"
				"#define IRRADIANCE_VOLUME_WIDTH %d\n"
				"#define IRRADIANCE_VOLUME_HEIGHT %d\n"
				"#define IRRADIANCE_VOXEL_DIM %f\n",
				CASCADE_COUNT,
				IRRADIANCE_VOLUME_WIDTH,
				IRRADIANCE_VOLUME_HEIGHT,
				IRRADIANCE_VOXEL_DIM);
		} break;

		case Shader_IndirectLightPass:
		{
			FormatString(Buffer, Length,
				"#define IRRADIANCE_VOLUME_WIDTH %d\n"
				"#define IRRADIANCE_VOLUME_HEIGHT %d\n"
				"#define IRRADIANCE_VOXEL_DIM %f\n",
				IRRADIANCE_VOLUME_WIDTH,
				IRRADIANCE_VOLUME_HEIGHT,
				IRRADIANCE_VOXEL_DIM);
		} break;

		case Shader_Surfels:
		{
			FormatString(Buffer, Length,
				"#define MAX_SURFELS %d\n"
				"#define MAX_PROBES %d\n"
				"#define CASCADE_COUNT %d\n",
				MAX_SURFELS,
				MAX_PROBES,
				CASCADE_COUNT);
		} break;

		case Shader_Probes:
		{
			FormatString(Buffer, Length,
				"#define MAX_SURFELS %d\n"
				"#define MAX_SURFELREFS %d\n"
				"#define MAX_PROBES %d\n",
				MAX_SURFELS,
				MAX_SURFELREFS,
				MAX_PROBES);
		} break;

		case Shader_Irradiance_Volume:
		{
				FormatString(Buffer, Length,
				"#define MAX_PROBES %d\n"
				"#define IRRADIANCE_VOLUME_WIDTH %d\n"
				"#define IRRADIANCE_VOLUME_HEIGHT %d\n"
				"#define IRRADIANCE_VOXEL_DIM %f\n",
				MAX_PROBES,
				IRRADIANCE_VOLUME_WIDTH,
				IRRADIANCE_VOLUME_HEIGHT,
				IRRADIANCE_VOXEL_DIM);
		} break;
	}
}

internal GLuint
OpenGLLoadCompileAndLinkShaderProgram(opengl_state *State, shader_type Header, shader_type Shader)
{
	GLuint Result = 0;

	b32 IsVSShader = IsVertexFragment(Shader);
	b32 IsComputeShader = IsCompute(Shader);
	Assert(IsVSShader != IsComputeShader);

	temporary_memory TempMem = BeginTemporaryMemory(&State->Region);

	char Defines[1024] = {};
	OpenGLGetShaderDefines(State, Defines, ArrayCount(Defines), Shader);

	char *ShaderSource = 0;
	char *HeaderSource = 0;
	while((ShaderSource == 0) || (HeaderSource == 0))
	{
		// TODO: Infinite loops if we can't open the files.
		ShaderSource = (char *)Platform->LoadEntireFileAndNullTerminate(&State->Region, State->ShaderFilenames[Shader]).Contents;
		HeaderSource = (char *)Platform->LoadEntireFileAndNullTerminate(&State->Region, State->ShaderFilenames[Header]).Contents;
	}

	GLuint ProgramIndex = 0;
	u32 ShaderCount = 0;
	GLuint Shaders[8] = {};
	if(IsVSShader)
	{
		char *VertexShader[16] = {"#version 430 core\n", "#define VERTEX_SHADER\n",};
		char *FragmentShader[16] = {"#version 430 core\n", "#define FRAGMENT_SHADER\n",};

		u32 Offset = 2;
		VertexShader[Offset] = Defines;
		FragmentShader[Offset] = Defines;

		Assert((Offset + 3) < ArrayCount(VertexShader));
		Assert((Offset + 3) < ArrayCount(FragmentShader));

		VertexShader[Offset + 1] = HeaderSource;
		VertexShader[Offset + 2] = ShaderSource;
		FragmentShader[Offset + 1] = HeaderSource;
		FragmentShader[Offset + 2] = ShaderSource;

		u32 VertexSize = Offset + 3;
		u32 FragmentSize = Offset + 3;

		Shaders[ShaderCount++] = OpenGLCompileShader(State, VertexShader, VertexSize, GL_VERTEX_SHADER, Shader);
		Shaders[ShaderCount++] = OpenGLCompileShader(State, FragmentShader, FragmentSize, GL_FRAGMENT_SHADER, Shader);
	}

	if(IsComputeShader)
	{
		char *ComputeShader[16] = {"#version 430 core\n", "#define COMPUTE_SHADER\n",};

		u32 Offset = 2;
		ComputeShader[Offset] = Defines;

		Assert((Offset + 3) < ArrayCount(ComputeShader));

		ComputeShader[Offset + 1] = HeaderSource;
		ComputeShader[Offset + 2] = ShaderSource;

		u32 ComputeSize = Offset + 3;

		Shaders[ShaderCount++] = OpenGLCompileShader(State, ComputeShader, ComputeSize, GL_COMPUTE_SHADER, Shader);
	}

	b32 Success = true;
	for(u32 Index = 0;
		Index < ShaderCount;
		++Index)
	{
		Success = (Success && (Shaders[Index] > 0));
	}

	if(Success)
	{
		ProgramIndex = glCreateProgram();
		for(u32 Index = 0;
			Index < ShaderCount;
			++Index)
		{
			glAttachShader(ProgramIndex, Shaders[Index]);
		}
		glLinkProgram(ProgramIndex);

		GLint Linked;
		GLchar InfoLog[512];
		glGetProgramiv(ProgramIndex, GL_LINK_STATUS, &Linked);
		if(!Linked)
		{
			glGetProgramInfoLog(ProgramIndex, ArrayCount(InfoLog), 0, InfoLog);
#if GAME_DEBUG
			Outf(State->Info->Errors, "Failed to link shader '%s'\n%s",
				WrapZ(Names_shader_type[Shader]),
				WrapZ(InfoLog));
#endif

			Assert(State->ShadersLoaded);
		}
		else
		{
			Result = ProgramIndex;
		}
	}

	for(u32 Index = 0;
		Index < ShaderCount;
		++Index)
	{
		glDeleteShader(Shaders[Index]);
	}

	EndTemporaryMemory(&TempMem);
	return Result;
}

internal void
OpenGLLinkSamplers(opengl_shader_common *Common,
	char *Sampler0 = 0,
	char *Sampler1 = 0,
	char *Sampler2 = 0,
	char *Sampler3 = 0,
	char *Sampler4 = 0,
	char *Sampler5 = 0,
	char *Sampler6 = 0,
	char *Sampler7 = 0,
	char *Sampler8 = 0,
	char *Sampler9 = 0,
	char *Sampler10 = 0,
	char *Sampler11 = 0,
	char *Sampler12 = 0,
	char *Sampler13 = 0,
	char *Sampler14 = 0,
	char *Sampler15 = 0)
{
    char *SamplerNames[] =
    {
        Sampler0,
        Sampler1,
        Sampler2,
        Sampler3,
        Sampler4,
        Sampler5,
        Sampler6,
        Sampler7,
        Sampler8,
        Sampler9,
        Sampler10,
        Sampler11,
        Sampler12,
        Sampler13,
        Sampler14,
        Sampler15,
    };

    for(u32 SamplerIndex = 0;
        SamplerIndex < ArrayCount(SamplerNames);
        ++SamplerIndex)
    {
        char *Name = SamplerNames[SamplerIndex];
        if(Name)
        {
            GLuint SamplerID = glGetUniformLocation(Common->Handle, Name);
            Assert(Common->SamplerCount < ArrayCount(Common->Samplers));
            Common->Samplers[Common->SamplerCount++] = SamplerID;
            // Assert(SamplerID != -1);
        }
    }
}

internal void
SetUpShaderCommon(opengl_shader_common *Common, GLuint ShaderHandle, shader_type Type)
{
	Common->Type = Type;
	Common->Handle = ShaderHandle;
	Common->Transform = glGetUniformLocation(Common->Handle, "Transform");
}

internal void
OpenGLReloadShader(opengl_state *State, shader_type Type)
{
	GLuint ShaderHandle = OpenGLLoadCompileAndLinkShaderProgram(State, Shader_Header, Type);

	if(ShaderHandle)
	{
		opengl_shader_common *Common = State->Shaders[Type];
		if(Common)
		{
			glDeleteProgram(Common->Handle);
			Common->Handle = 0;
			Common->Transform = -1;

			for(u32 Index = 0;
				Index < Common->SamplerCount;
				++Index)
			{
				Common->Samplers[Index] = 0;
			}

			Common->SamplerCount = 0;
		}

		switch(Type)
		{
			case Shader_GBuffer:
			{
				shader_gbuffer *GBuffer = (shader_gbuffer *)State->Shaders[Type];
				if(!GBuffer)
				{
					GBuffer = PushStruct(&State->Region, shader_gbuffer);
					State->Shaders[Type] = (opengl_shader_common *)GBuffer;
				}

				SetUpShaderCommon(&GBuffer->Common, ShaderHandle, Type);

	            GBuffer->Projection = glGetUniformLocation(GBuffer->Common.Handle, "Projection");
				GBuffer->View = glGetUniformLocation(GBuffer->Common.Handle, "View");
				GBuffer->Model = glGetUniformLocation(GBuffer->Common.Handle, "Model");
				GBuffer->NormalMatrix = glGetUniformLocation(GBuffer->Common.Handle, "NormalMatrix");
				GBuffer->MeshTextureIndex = glGetUniformLocation(GBuffer->Common.Handle, "MeshTextureIndex");

				GBuffer->Jitter = glGetUniformLocation(GBuffer->Common.Handle, "Jitter");
				GBuffer->FarPlane = glGetUniformLocation(GBuffer->Common.Handle, "FarPlane");
				GBuffer->Orthogonal = glGetUniformLocation(GBuffer->Common.Handle, "Orthogonal");

				GBuffer->Color = glGetUniformLocation(GBuffer->Common.Handle, "Color");
				GBuffer->Roughness = glGetUniformLocation(GBuffer->Common.Handle, "Roughness");
				GBuffer->Metalness = glGetUniformLocation(GBuffer->Common.Handle, "Metalness");
				GBuffer->Emission = glGetUniformLocation(GBuffer->Common.Handle, "Emission");

				OpenGLLinkSamplers(&GBuffer->Common, "TextureArray");
			} break;

			case Shader_ShadowMap:
			{
				shader_shadow_map *ShadowMap = (shader_shadow_map *)State->Shaders[Type];
				if(!ShadowMap)
				{
					ShadowMap = PushStruct(&State->Region, shader_shadow_map);
					State->Shaders[Type] = (opengl_shader_common *)ShadowMap;
				}

				SetUpShaderCommon(&ShadowMap->Common, ShaderHandle, Type);

	            ShadowMap->Projection = glGetUniformLocation(ShadowMap->Common.Handle, "Projection");
				ShadowMap->View = glGetUniformLocation(ShadowMap->Common.Handle, "View");
				ShadowMap->Model = glGetUniformLocation(ShadowMap->Common.Handle, "Model");

				ShadowMap->FarPlane = glGetUniformLocation(ShadowMap->Common.Handle, "FarPlane");
				ShadowMap->Orthogonal = glGetUniformLocation(ShadowMap->Common.Handle, "Orthogonal");
			} break;

			case Shader_BakeProbe:
			{
				shader_bake_probe *ProbeShader = (shader_bake_probe *)State->Shaders[Type];
				if(!ProbeShader)
				{
					ProbeShader = PushStruct(&State->Region, shader_bake_probe);
					State->Shaders[Type] = (opengl_shader_common *)ProbeShader;
				}

				SetUpShaderCommon(&ProbeShader->Common, ShaderHandle, Type);

	            ProbeShader->Projection = glGetUniformLocation(ProbeShader->Common.Handle, "Projection");
				ProbeShader->View = glGetUniformLocation(ProbeShader->Common.Handle, "View");
				ProbeShader->MeshTextureIndex = glGetUniformLocation(ProbeShader->Common.Handle, "MeshTextureIndex");

				ProbeShader->FarPlane = glGetUniformLocation(ProbeShader->Common.Handle, "FarPlane");
				ProbeShader->Orthogonal = glGetUniformLocation(ProbeShader->Common.Handle, "Orthogonal");

				OpenGLLinkSamplers(&ProbeShader->Common, "TextureArray");
			} break;


			case Shader_Fresnel:
			{
				shader_fresnel *Fresnel = (shader_fresnel *)State->Shaders[Type];
				if(!Fresnel)
				{
					Fresnel = PushStruct(&State->Region, shader_fresnel);
					State->Shaders[Type] = (opengl_shader_common *)Fresnel;
				}

				SetUpShaderCommon(&Fresnel->Common, ShaderHandle, Type);

				Fresnel->ViewRay = glGetUniformLocation(Fresnel->Common.Handle, "ViewRay");

				OpenGLLinkSamplers(&Fresnel->Common, "AlbedoTexture", "DepthTexture", "NormalTexture", "RMETexture");
			} break;

			case Shader_LightPass:
			{
				shader_light_pass *LightPass = (shader_light_pass *)State->Shaders[Type];
				if(!LightPass)
				{
					LightPass = PushStruct(&State->Region, shader_light_pass);
					State->Shaders[Type] = (opengl_shader_common *)LightPass;
				}

				SetUpShaderCommon(&LightPass->Common, ShaderHandle, Type);

	            LightPass->Projection = glGetUniformLocation(LightPass->Common.Handle, "Projection");
				LightPass->View = glGetUniformLocation(LightPass->Common.Handle, "View");
				LightPass->Model = glGetUniformLocation(LightPass->Common.Handle, "Model");

				LightPass->InvView = glGetUniformLocation(LightPass->Common.Handle, "InvView");
				LightPass->ViewRay = glGetUniformLocation(LightPass->Common.Handle, "ViewRay");

				LightPass->LightData.IsDirectional = glGetUniformLocation(LightPass->Common.Handle, "Light.IsDirectional");
				LightPass->LightData.Color = glGetUniformLocation(LightPass->Common.Handle, "Light.Color");
				LightPass->LightData.Intensity = glGetUniformLocation(LightPass->Common.Handle, "Light.Intensity");
				LightPass->LightData.Direction = glGetUniformLocation(LightPass->Common.Handle, "Light.Direction");
				LightPass->LightData.FuzzFactor = glGetUniformLocation(LightPass->Common.Handle, "Light.FuzzFactor");

				LightPass->LightData.P = glGetUniformLocation(LightPass->Common.Handle, "Light.P");
				LightPass->LightData.Size = glGetUniformLocation(LightPass->Common.Handle, "Light.Size");
				LightPass->LightData.LinearAttenuation = glGetUniformLocation(LightPass->Common.Handle, "Light.LinearAttenuation");
				LightPass->LightData.QuadraticAttenuation = glGetUniformLocation(LightPass->Common.Handle, "Light.QuadraticAttenuation");
				LightPass->LightData.Radius = glGetUniformLocation(LightPass->Common.Handle, "Light.Radius");

				LightPass->CastsShadow = glGetUniformLocation(LightPass->Common.Handle, "CastsShadow");

				LightPass->LightProjection = glGetUniformLocation(LightPass->Common.Handle, "LightProjection");
				LightPass->LightView = glGetUniformLocation(LightPass->Common.Handle, "LightView");
				LightPass->LightFarPlane = glGetUniformLocation(LightPass->Common.Handle, "LightFarPlane");
				LightPass->CascadeBounds = glGetUniformLocation(LightPass->Common.Handle, "CascadeBounds");
				LightPass->SMRegionScale = glGetUniformLocation(LightPass->Common.Handle, "SMRegionScale");
				LightPass->SMRegionOffset = glGetUniformLocation(LightPass->Common.Handle, "SMRegionOffset");

				OpenGLLinkSamplers(&LightPass->Common, "AlbedoTexture", "DepthTexture", "NormalTexture", "RMETexture", "FresnelTexture", "ShadowMap", "BlueNoise");
			} break;

			case Shader_IndirectLightPass:
			{
				shader_indirect_light_pass *IndirectLightPass = (shader_indirect_light_pass *)State->Shaders[Type];
				if(!IndirectLightPass)
				{
					IndirectLightPass = PushStruct(&State->Region, shader_indirect_light_pass);
					State->Shaders[Type] = (opengl_shader_common *)IndirectLightPass;
				}

				SetUpShaderCommon(&IndirectLightPass->Common, ShaderHandle, Type);

				IndirectLightPass->ViewRay = glGetUniformLocation(IndirectLightPass->Common.Handle, "ViewRay");
				IndirectLightPass->InvView = glGetUniformLocation(IndirectLightPass->Common.Handle, "InvView");
				IndirectLightPass->SkyRadiance = glGetUniformLocation(IndirectLightPass->Common.Handle, "SkyRadiance");

				OpenGLLinkSamplers(&IndirectLightPass->Common, "AlbedoTexture", "DepthTexture", "NormalTexture", "RMETexture", "FresnelTexture", "IrradianceVolume");
			} break;

			case Shader_Decal:
			{
				shader_decal *Decal = (shader_decal *)State->Shaders[Type];
				if(!Decal)
				{
					Decal = PushStruct(&State->Region, shader_decal);
					State->Shaders[Type] = (opengl_shader_common *)Decal;
				}

				SetUpShaderCommon(&Decal->Common, ShaderHandle, Type);

	            Decal->Projection = glGetUniformLocation(Decal->Common.Handle, "Projection");
				Decal->View = glGetUniformLocation(Decal->Common.Handle, "View");
				Decal->Model = glGetUniformLocation(Decal->Common.Handle, "Model");

				Decal->ViewRay = glGetUniformLocation(Decal->Common.Handle, "ViewRay");
				Decal->InvView = glGetUniformLocation(Decal->Common.Handle, "InvView");

				Decal->DecalTextureIndex = glGetUniformLocation(Decal->Common.Handle, "DecalTextureIndex");
				Decal->DecalTextureScale = glGetUniformLocation(Decal->Common.Handle, "DecalTextureScale");

				Decal->DecalData.Projection = glGetUniformLocation(Decal->Common.Handle, "Decal.Projection");
				Decal->DecalData.View = glGetUniformLocation(Decal->Common.Handle, "Decal.View");

				OpenGLLinkSamplers(&Decal->Common, "TextureArray", "DepthTexture");
			} break;

			case Shader_Overlay:
			{
				shader_overlay *Overlay = (shader_overlay *)State->Shaders[Type];
				if(!Overlay)
				{
					Overlay = PushStruct(&State->Region, shader_overlay);
					State->Shaders[Type] = (opengl_shader_common *)Overlay;
				}

				SetUpShaderCommon(&Overlay->Common, ShaderHandle, Type);

				Overlay->IsSDFTexture = glGetUniformLocation(Overlay->Common.Handle, "IsSDFTexture");

				OpenGLLinkSamplers(&Overlay->Common, "TextureArray");
			} break;

			case Shader_Overbright:
			{
				shader_overbright *Overbright = (shader_overbright *)State->Shaders[Type];
				if(!Overbright)
				{
					Overbright = PushStruct(&State->Region, shader_overbright);
					State->Shaders[Type] = (opengl_shader_common *)Overbright;
				}

				SetUpShaderCommon(&Overbright->Common, ShaderHandle, Type);

				OpenGLLinkSamplers(&Overbright->Common, "SceneTexture");
			} break;

			case Shader_Blur:
			{
				shader_blur *Blur = (shader_blur *)State->Shaders[Type];
				if(!Blur)
				{
					Blur = PushStruct(&State->Region, shader_blur);
					State->Shaders[Type] = (opengl_shader_common *)Blur;
				}

				SetUpShaderCommon(&Blur->Common, ShaderHandle, Type);

				Blur->Kernel = glGetUniformLocation(Blur->Common.Handle, "Kernel");

				OpenGLLinkSamplers(&Blur->Common, "Texture");
			} break;

			case Shader_Finalize:
			{
				shader_finalize *Finalize = (shader_finalize *)State->Shaders[Type];
				if(!Finalize)
				{
					Finalize = PushStruct(&State->Region, shader_finalize);
					State->Shaders[Type] = (opengl_shader_common *)Finalize;
				}

				SetUpShaderCommon(&Finalize->Common, ShaderHandle, Type);

				OpenGLLinkSamplers(&Finalize->Common, "SceneTexture", "OverlayTexture", "BloomTexture", "FogTexture");
			} break;

			case Shader_Skybox:
			{
				shader_skybox *Skybox = (shader_skybox *)State->Shaders[Type];
				if(!Skybox)
				{
					Skybox = PushStruct(&State->Region, shader_skybox);
					State->Shaders[Type] = (opengl_shader_common *)Skybox;
				}

				SetUpShaderCommon(&Skybox->Common, ShaderHandle, Type);

				Skybox->SkyRadiance = glGetUniformLocation(Skybox->Common.Handle, "SkyRadiance");
			} break;

			case Shader_Fog:
			{
				shader_fog *FogShader = (shader_fog *)State->Shaders[Type];
				if(!FogShader)
				{
					FogShader = PushStruct(&State->Region, shader_fog);
					State->Shaders[Type] = (opengl_shader_common *)FogShader;
				}

				SetUpShaderCommon(&FogShader->Common, ShaderHandle, Type);

				FogShader->View = glGetUniformLocation(FogShader->Common.Handle, "View");
				FogShader->InvView = glGetUniformLocation(FogShader->Common.Handle, "InvView");
				FogShader->ViewRay = glGetUniformLocation(FogShader->Common.Handle, "ViewRay");
				FogShader->SkyRadiance = glGetUniformLocation(FogShader->Common.Handle, "SkyRadiance");

				FogShader->LightData.IsDirectional = glGetUniformLocation(FogShader->Common.Handle, "Light.IsDirectional");
				FogShader->LightData.Color = glGetUniformLocation(FogShader->Common.Handle, "Light.Color");
				FogShader->LightData.Intensity = glGetUniformLocation(FogShader->Common.Handle, "Light.Intensity");
				FogShader->LightData.Direction = glGetUniformLocation(FogShader->Common.Handle, "Light.Direction");
				FogShader->LightData.FuzzFactor = glGetUniformLocation(FogShader->Common.Handle, "Light.FuzzFactor");

				FogShader->LightData.P = glGetUniformLocation(FogShader->Common.Handle, "Light.P");
				FogShader->LightData.Size = glGetUniformLocation(FogShader->Common.Handle, "Light.Size");
				FogShader->LightData.LinearAttenuation = glGetUniformLocation(FogShader->Common.Handle, "Light.LinearAttenuation");
				FogShader->LightData.QuadraticAttenuation = glGetUniformLocation(FogShader->Common.Handle, "Light.QuadraticAttenuation");
				FogShader->LightData.Radius = glGetUniformLocation(FogShader->Common.Handle, "Light.Radius");

				FogShader->LightProjection = glGetUniformLocation(FogShader->Common.Handle, "LightProjection");
				FogShader->LightView = glGetUniformLocation(FogShader->Common.Handle, "LightView");
				FogShader->LightFarPlane = glGetUniformLocation(FogShader->Common.Handle, "LightFarPlane");
				FogShader->CascadeBounds = glGetUniformLocation(FogShader->Common.Handle, "CascadeBounds");
				FogShader->SMRegionScale = glGetUniformLocation(FogShader->Common.Handle, "SMRegionScale");
				FogShader->SMRegionOffset = glGetUniformLocation(FogShader->Common.Handle, "SMRegionOffset");

				OpenGLLinkSamplers(&FogShader->Common, "SceneColor", "DepthTexture", "ShadowMap", "IrradianceVolume", "BlueNoise");
			} break;

			case Shader_FogResolve:
			{
				shader_fog_resolve *FogResolve = (shader_fog_resolve *)State->Shaders[Type];
				if(!FogResolve)
				{
					FogResolve = PushStruct(&State->Region, shader_fog_resolve);
					State->Shaders[Type] = (opengl_shader_common *)FogResolve;
				}

				SetUpShaderCommon(&FogResolve->Common, ShaderHandle, Type);

				OpenGLLinkSamplers(&FogResolve->Common, "FogColor", "DepthTexture");
			} break;

			case Shader_DebugView:
			{
				shader_debug_view *DebugView = (shader_debug_view *)State->Shaders[Type];
				if(!DebugView)
				{
					DebugView = PushStruct(&State->Region, shader_debug_view);
					State->Shaders[Type] = (opengl_shader_common *)DebugView;
				}

				SetUpShaderCommon(&DebugView->Common, ShaderHandle, Type);

				DebugView->LOD = glGetUniformLocation(DebugView->Common.Handle, "LOD");
				OpenGLLinkSamplers(&DebugView->Common, "Texture");
			} break;

			case Shader_Surfels:
			{
				shader_surfels *Surfels = (shader_surfels *)State->Shaders[Type];
				if(!Surfels)
				{
					Surfels = PushStruct(&State->Region, shader_surfels);
					State->Shaders[Type] = (opengl_shader_common *)Surfels;
				}

				SetUpShaderCommon(&Surfels->Common, ShaderHandle, Type);

				Surfels->Initialize = glGetUniformLocation(Surfels->Common.Handle, "Initialize");

				Surfels->View = glGetUniformLocation(Surfels->Common.Handle, "View");

				Surfels->LightData.IsDirectional = glGetUniformLocation(Surfels->Common.Handle, "Light.IsDirectional");
				Surfels->LightData.Color = glGetUniformLocation(Surfels->Common.Handle, "Light.Color");
				Surfels->LightData.Intensity = glGetUniformLocation(Surfels->Common.Handle, "Light.Intensity");
				Surfels->LightData.Direction = glGetUniformLocation(Surfels->Common.Handle, "Light.Direction");
				Surfels->LightData.FuzzFactor = glGetUniformLocation(Surfels->Common.Handle, "Light.FuzzFactor");

				Surfels->LightData.P = glGetUniformLocation(Surfels->Common.Handle, "Light.P");
				Surfels->LightData.Size = glGetUniformLocation(Surfels->Common.Handle, "Light.Size");
				Surfels->LightData.LinearAttenuation = glGetUniformLocation(Surfels->Common.Handle, "Light.LinearAttenuation");
				Surfels->LightData.QuadraticAttenuation = glGetUniformLocation(Surfels->Common.Handle, "Light.QuadraticAttenuation");
				Surfels->LightData.Radius = glGetUniformLocation(Surfels->Common.Handle, "Light.Radius");

				Surfels->SkyRadiance = glGetUniformLocation(Surfels->Common.Handle, "SkyRadiance");

				Surfels->LightProjection = glGetUniformLocation(Surfels->Common.Handle, "LightProjection");
				Surfels->LightView = glGetUniformLocation(Surfels->Common.Handle, "LightView");
				Surfels->LightFarPlane = glGetUniformLocation(Surfels->Common.Handle, "LightFarPlane");
				Surfels->CascadeBounds = glGetUniformLocation(Surfels->Common.Handle, "CascadeBounds");
				Surfels->SMRegionScale = glGetUniformLocation(Surfels->Common.Handle, "SMRegionScale");
				Surfels->SMRegionOffset = glGetUniformLocation(Surfels->Common.Handle, "SMRegionOffset");

				OpenGLLinkSamplers(&Surfels->Common, "ShadowMap");
			} break;

			case Shader_Probes:
			{
				shader_probes *Probes = (shader_probes *)State->Shaders[Type];
				if(!Probes)
				{
					Probes = PushStruct(&State->Region, shader_probes);
					State->Shaders[Type] = (opengl_shader_common *)Probes;
				}

				SetUpShaderCommon(&Probes->Common, ShaderHandle, Type);
			} break;

			case Shader_Irradiance_Volume:
			{
				shader_irradiance_volume *Irradiance = (shader_irradiance_volume *)State->Shaders[Type];
				if(!Irradiance)
				{
					Irradiance = PushStruct(&State->Region, shader_irradiance_volume);
					State->Shaders[Type] = (opengl_shader_common *)Irradiance;
				}

				SetUpShaderCommon(&Irradiance->Common, ShaderHandle, Type);

				Irradiance->ProbeCount = glGetUniformLocation(Irradiance->Common.Handle, "ProbeCount");
				Irradiance->Center = glGetUniformLocation(Irradiance->Common.Handle, "Center");
			} break;

			InvalidDefaultCase;
		}

#if GAME_DEBUG
		Outf(State->Info, "Shader loaded (%s)", WrapZ(Names_shader_type[Type]));
#endif
	}
}

inline void
SetUniform(GLint UniformLocation, b32 Boolean)
{
	glUniform1i(UniformLocation, Boolean);
}

inline void
SetUniform(GLint UniformLocation, u32 UInt)
{
	glUniform1i(UniformLocation, UInt);
}

inline void
SetUniform(GLint UniformLocation, r32 Value)
{
	glUniform1f(UniformLocation, Value);
}

inline void
SetUniform(GLint UniformLocation, mat3 Matrix3)
{
	glUniformMatrix3fv(UniformLocation, 1, GL_FALSE, Matrix3.M);
}

inline void
SetUniform(GLint UniformLocation, mat4 Matrix4)
{
	glUniformMatrix4fv(UniformLocation, 1, GL_FALSE, Matrix4.M);
}

inline void
SetUniform(GLint UniformLocation, v2 Vector2)
{
	glUniform2f(UniformLocation, Vector2.x, Vector2.y);
}

inline void
SetUniform(GLint UniformLocation, v2i Vector2i)
{
	glUniform2i(UniformLocation, Vector2i.x, Vector2i.y);
}

inline void
SetUniform(GLint UniformLocation, v3 Vector3)
{
	glUniform3fv(UniformLocation, 1, Vector3.E);
}

inline void
SetUniform(GLint UniformLocation, v4 Vector4)
{
	glUniform4fv(UniformLocation, 1, Vector4.E);
}

inline void
SetUniformArray(GLint UniformLocation, f32 *Array, u32 Count)
{
	glUniform1fv(UniformLocation, Count, Array);
}

inline void
SetUniformArray(GLint UniformLocation, v2 *Array, u32 Count)
{
	glUniform2fv(UniformLocation, Count, (f32 *)Array);
}

inline void
SetUniformArray(GLint UniformLocation, v3 *Array, u32 Count)
{
	glUniform3fv(UniformLocation, Count, (f32 *)Array);
}

inline void
SetUniformArray(GLint UniformLocation, mat4 *Array, u32 Count)
{
	glUniformMatrix4fv(UniformLocation, Count, GL_FALSE, (f32 *)Array);
}

inline void
BindTexture(u32 SlotIndex, GLint TextureHandle, GLenum Target = GL_TEXTURE_2D)
{
	glActiveTexture(GL_TEXTURE0 + SlotIndex);
	glBindTexture(Target, TextureHandle);
}

internal void
SetShaderMeshUniforms(opengl_shader_common *Shader, render_command_mesh_data *MeshData)
{
	switch(Shader->Type)
	{
		case Shader_GBuffer:
		{
			shader_gbuffer *GBuffer = (shader_gbuffer *) Shader;
			SetUniform(GBuffer->Model, MeshData->Model);
			SetUniform(GBuffer->NormalMatrix, MeshData->NormalMatrix);
			SetUniform(GBuffer->Roughness, MeshData->Material.Roughness);
			SetUniform(GBuffer->Metalness, MeshData->Material.Metalness);
			SetUniform(GBuffer->Emission, MeshData->Material.Emission);
			SetUniform(GBuffer->Color, MeshData->Material.Color);
			SetUniform(GBuffer->MeshTextureIndex, MeshData->TextureIndex);
		} break;

		case Shader_ShadowMap:
		{
			shader_shadow_map *ShadowMap = (shader_shadow_map *) Shader;
			SetUniform(ShadowMap->Model, MeshData->Model);
		} break;

		InvalidDefaultCase;
	}
}

internal void
SetShaderCameraUniforms(opengl_state *State, opengl_shader_common *Shader,
	render_setup *Setup, light_space_cascades *Cascades, u32 CascadeIndex)
{
	switch(Shader->Type)
	{
		case Shader_GBuffer:
		{
			shader_gbuffer *GBuffer = (shader_gbuffer *) Shader;
			SetUniform(GBuffer->Projection, Setup->Projection);
			SetUniform(GBuffer->View, Setup->View);
			SetUniform(GBuffer->FarPlane, Setup->FarPlane);
			SetUniform(GBuffer->Jitter, State->Jitter);
			SetUniform(GBuffer->Orthogonal, false);

			if(Setup->Target == Target_Scene)
			{
				if(Setup->Flags & RenderFlag_SDF)
				{
					NotImplemented;
				}
				else
				{
					SetUniform(GBuffer->Color, V4(1,1,1,1));
					SetUniform(GBuffer->MeshTextureIndex, 0);
					SetUniform(GBuffer->Model, IdentityMat4());
					SetUniform(GBuffer->NormalMatrix, IdentityMat3());
					SetUniform(GBuffer->Roughness, 0.0f);
					SetUniform(GBuffer->Metalness, 0.0f);
					SetUniform(GBuffer->Emission, 0.0f);
				}
			}
		} break;

		case Shader_ShadowMap:
		{
			Assert(Cascades);
			shader_shadow_map *ShadowMap = (shader_shadow_map *) Shader;
			SetUniform(ShadowMap->Projection, Cascades->Projection[CascadeIndex]);
			SetUniform(ShadowMap->View, Cascades->View[CascadeIndex]);
			SetUniform(ShadowMap->FarPlane, Cascades->FarPlane[CascadeIndex]);
			SetUniform(ShadowMap->Orthogonal, Cascades->Orthogonal[CascadeIndex]);

			if(Setup->Target == Target_Scene)
			{
				if(Setup->Flags & RenderFlag_SDF)
				{
					NotImplemented;
				}
				else
				{
					SetUniform(ShadowMap->Model, IdentityMat4());
				}
			}
		} break;

		case Shader_Overlay:
		{
			shader_overlay *Overlay = (shader_overlay *) Shader;
			SetUniform(Overlay->Common.Transform, Setup->Projection*Setup->View);
			if(Setup->Flags & RenderFlag_SDF)
			{
				SetUniform(Overlay->IsSDFTexture, true);
			}
			else
			{
				SetUniform(Overlay->IsSDFTexture, false);
			}
		} break;

		InvalidDefaultCase;
	}

	if(Setup->Target == Target_Scene)
	{
		State->CurrentSceneRenderSetup = Setup;
	}
}

internal opengl_shader_common *
BeginShader(opengl_state *State, shader_type Type)
{
	TIMED_FUNCTION();

	opengl_shader_common *Shader = State->Shaders[Type];

	Assert(Shader);
	glUseProgram(Shader->Handle);
	State->CurrentShader = Shader;

	for(u32 SamplerIndex = 0;
		SamplerIndex < Shader->SamplerCount;
		++SamplerIndex)
	{
		SetUniform(Shader->Samplers[SamplerIndex], SamplerIndex);
	}

	return Shader;
}

internal void
EndShader(opengl_state *State)
{
	glUseProgram(0);
	State->CurrentShader = 0;
}

internal void
LightSurfels(opengl_state *State, lighting_solution *Solution, v3 CameraP, light *Lights, u32 LightCount)
{
	TIMED_FUNCTION();

	for(u32 SurfelIndex = 0;
		SurfelIndex < Solution->SurfelCount;
		++SurfelIndex)
	{
		surface_element *Surfel = Solution->Surfels + SurfelIndex;
		v3 ViewDirection = (Surfel->P - CameraP);

		Surfel->LitColor = V3(0,0,0);
		light_probe *Probe = Solution->Probes + Surfel->ClosestProbe;
		v3 NormalSq = Hadamard(Surfel->Normal, Surfel->Normal);

		Surfel->LitColor = NormalSq.x * Probe->LightContributions[(Surfel->Normal.x < 0.0f) ? 1 : 0].xyz +
						NormalSq.y * Probe->LightContributions[(Surfel->Normal.y < 0.0f) ? 3 : 2].xyz +
						NormalSq.z * Probe->LightContributions[(Surfel->Normal.z < 0.0f) ? 5 : 4].xyz;
		Surfel->LitColor = 0.1f*Hadamard(Surfel->LitColor, Surfel->Albedo);

		for(u32 LightIndex = 0;
			LightIndex < LightCount;
			++LightIndex)
		{
			light *Light = Lights + LightIndex;
			if(Light->Type == LightType_Ambient)
			{
				// v3 Ambient = Light->Ambient.Coeff * Light->Ambient.Color;
				// Surfel->LitColor += Hadamard(Ambient, Surfel->Albedo);
			}
			else
			{
				v3 LightDirection = {};
				f32 Attenuation = 1.0f;
				f32 Intensity = 1.0f;
				v3 Color = {};
				f32 Shadow = 1.0f;

				if(Light->Type == LightType_Directional)
				{
					LightDirection = -(Light->Directional.Direction);
					Intensity = Light->Directional.Intensity;
					Color = Light->Directional.Color;

#if 0
					ray_cast_result RayCast = AABBRayCastQuery(&WorldMode->Physics.StaticAABBTree,
						Surfel->P, LightDirection);
					if(RayCast.RayHit)
					{
						Shadow = 0.0f;
					}
#endif
				}
				else
				{
					Assert(Light->Type == LightType_Point);
					LightDirection = (Light->Point.P - Surfel->P);
					f32 Distance = Length(LightDirection);
					LightDirection = (1.0f/Distance) * LightDirection;
					Attenuation = Maximum(Light->Point.Intensity + Light->Point.LinearAttenuation*Distance, 0.0f);
					Intensity = Light->Point.Intensity;
					Color = Light->Point.Color;

#if 0
					ray_cast_result RayCast = AABBRayCastQuery(&WorldMode->Physics.StaticAABBTree,
						Light->Point.P, -LightDirection);
					if(RayCast.RayHit)
					{
						if(LengthSq(RayCast.HitPoint - Surfel->P) > Square(0.01f))
						{
							Shadow = 0.0f;
						}
					}
#endif
				}

				v3 HalfwayDirection = NOZ(LightDirection + ViewDirection);

				f32 Diffuse = Maximum(Inner(LightDirection, Surfel->Normal), 0.0f);

				Surfel->LitColor += Shadow * Attenuation * Diffuse * Intensity *
					Hadamard(Color, Surfel->Albedo);
			}
		}
	}

	for(u32 ProbeIndex = 0;
		ProbeIndex < Solution->ProbeCount;
		++ProbeIndex)
	{
		light_probe *Probe = Solution->Probes + ProbeIndex;

		for(u32 ProbeDirIndex = 0;
			ProbeDirIndex < ArrayCount(ProbeDirections);
			++ProbeDirIndex)
		{
			Probe->LightContributions[ProbeDirIndex].xyz = V3(0,0,0);
		}

		for(u32 SurfelRefIndex = Probe->FirstSurfel;
			SurfelRefIndex < Probe->OnePastLastSurfel;
			++SurfelRefIndex)
		{
			u32 SurfelIndex = Solution->SurfelRefs[SurfelRefIndex];
			surface_element *Surfel = Solution->Surfels + SurfelIndex;
			v3 SurfelDirection = (Surfel->P - Probe->P);
			f32 SurfelDist = Length(SurfelDirection);
			SurfelDirection = (1.0f/SurfelDist)*SurfelDirection;

			for(u32 ProbeDirIndex = 0;
				ProbeDirIndex < ArrayCount(ProbeDirections);
				++ProbeDirIndex)
			{
				v3 Direction = ProbeDirections[ProbeDirIndex];
				f32 ProbeNormCos = Inner(Direction, SurfelDirection);
				if(ProbeNormCos > 0.0f)
				{
					f32 Contrib = ProbeNormCos;
					f32 SurfelNormCos = Maximum(Inner(Surfel->Normal, -SurfelDirection), 0.0f);
					f32 SolidAngle = Surfel->Area*SurfelNormCos / Square(SurfelDist);

					Probe->LightContributions[ProbeDirIndex].xyz += Contrib * SolidAngle * Surfel->LitColor;
				}
			}
		}
	}
}

internal irradiance_volume
BuildIrradianceVolume(memory_region *Region, light_probe *Probes, u32 ProbeCount, v3 Center)
{
	TIMED_FUNCTION();

	u32 VoxelCount = IRRADIANCE_VOLUME_HEIGHT * IRRADIANCE_VOLUME_WIDTH * IRRADIANCE_VOLUME_WIDTH * 6;
	v3 RegionDim = V3(IRRADIANCE_VOLUME_WIDTH * IRRADIANCE_VOXEL_DIM,
					  IRRADIANCE_VOLUME_HEIGHT * IRRADIANCE_VOXEL_DIM,
					  IRRADIANCE_VOLUME_WIDTH * IRRADIANCE_VOXEL_DIM);
	v3 RegionMin = Center - 0.5f*RegionDim;
	v3 VoxelDim = V3(IRRADIANCE_VOXEL_DIM, IRRADIANCE_VOXEL_DIM, IRRADIANCE_VOXEL_DIM);

	irradiance_volume Result = {};
	Result.Voxels = PushArray(Region, VoxelCount, v3);

	for(u32 Z = 0;
		Z < IRRADIANCE_VOLUME_WIDTH;
		++Z)
	{
		for(u32 Y = 0;
			Y < IRRADIANCE_VOLUME_HEIGHT;
			++Y)
		{
			for(u32 X = 0;
				X < IRRADIANCE_VOLUME_WIDTH;
				++X)
			{
				v3 *VoxelPX = Result.Voxels + Z*IRRADIANCE_VOLUME_HEIGHT*6*IRRADIANCE_VOLUME_WIDTH +
											  Y*6*IRRADIANCE_VOLUME_WIDTH +
											  X;
				v3 *VoxelNX = VoxelPX + IRRADIANCE_VOLUME_WIDTH;
				v3 *VoxelPY = VoxelNX + IRRADIANCE_VOLUME_WIDTH;
				v3 *VoxelNY = VoxelPY + IRRADIANCE_VOLUME_WIDTH;
				v3 *VoxelPZ = VoxelNY + IRRADIANCE_VOLUME_WIDTH;
				v3 *VoxelNZ = VoxelPZ + IRRADIANCE_VOLUME_WIDTH;

				v3 VoxelP = RegionMin + Hadamard(V3((f32)X, (f32)Y, (f32)Z), VoxelDim);

				Assert(ProbeCount >= 4);
				light_probe *ClosestProbes[4] = {};
				f32 MinDistSq[4] = {Real32Maximum, Real32Maximum, Real32Maximum, Real32Maximum};
				for(u32 ProbeIndex = 0;
					ProbeIndex < ProbeCount;
					++ProbeIndex)
				{
					light_probe *Probe = Probes + ProbeIndex;
					f32 DistSq = LengthSq(Probe->P - VoxelP);
					if(DistSq < MinDistSq[3])
					{
						MinDistSq[3] = DistSq;
						ClosestProbes[3] = Probe;

						if(MinDistSq[3] < MinDistSq[2])
						{
							Swap(MinDistSq[3], MinDistSq[2], f32);
							Swap(ClosestProbes[3], ClosestProbes[2], light_probe *);
						}
						if(MinDistSq[2] < MinDistSq[1])
						{
							Swap(MinDistSq[2], MinDistSq[1], f32);
							Swap(ClosestProbes[2], ClosestProbes[1], light_probe *);
						}
						if(MinDistSq[1] < MinDistSq[0])
						{
							Swap(MinDistSq[1], MinDistSq[0], f32);
							Swap(ClosestProbes[1], ClosestProbes[0], light_probe *);
						}
					}
				}

#if 1
				if(MinDistSq[0] == 0.0f)
				{
					*VoxelPX = ClosestProbes[0]->LightContributions[0].xyz;
					*VoxelNX = ClosestProbes[0]->LightContributions[1].xyz;
					*VoxelPY = ClosestProbes[0]->LightContributions[2].xyz;
					*VoxelNY = ClosestProbes[0]->LightContributions[3].xyz;
					*VoxelPZ = ClosestProbes[0]->LightContributions[4].xyz;
					*VoxelNZ = ClosestProbes[0]->LightContributions[5].xyz;
				}
				else
				{
					f32 Numerator = 1.0f/((1.0f/MinDistSq[0]) +
								 	 	  (1.0f/MinDistSq[1]) +
								 	 	  (1.0f/MinDistSq[2]) +
								 	 	  (1.0f/MinDistSq[3]));
					*VoxelPX = (Numerator/MinDistSq[0])*ClosestProbes[0]->LightContributions[0].xyz +
							   (Numerator/MinDistSq[1])*ClosestProbes[1]->LightContributions[0].xyz +
							   (Numerator/MinDistSq[2])*ClosestProbes[2]->LightContributions[0].xyz +
							   (Numerator/MinDistSq[3])*ClosestProbes[3]->LightContributions[0].xyz;
					*VoxelNX = (Numerator/MinDistSq[0])*ClosestProbes[0]->LightContributions[1].xyz +
							   (Numerator/MinDistSq[1])*ClosestProbes[1]->LightContributions[1].xyz +
							   (Numerator/MinDistSq[2])*ClosestProbes[2]->LightContributions[1].xyz +
							   (Numerator/MinDistSq[3])*ClosestProbes[3]->LightContributions[1].xyz;
					*VoxelPY = (Numerator/MinDistSq[0])*ClosestProbes[0]->LightContributions[2].xyz +
							   (Numerator/MinDistSq[1])*ClosestProbes[1]->LightContributions[2].xyz +
							   (Numerator/MinDistSq[2])*ClosestProbes[2]->LightContributions[2].xyz +
							   (Numerator/MinDistSq[3])*ClosestProbes[3]->LightContributions[2].xyz;
					*VoxelNY = (Numerator/MinDistSq[0])*ClosestProbes[0]->LightContributions[3].xyz +
							   (Numerator/MinDistSq[1])*ClosestProbes[1]->LightContributions[3].xyz +
							   (Numerator/MinDistSq[2])*ClosestProbes[2]->LightContributions[3].xyz +
							   (Numerator/MinDistSq[3])*ClosestProbes[3]->LightContributions[3].xyz;
					*VoxelPZ = (Numerator/MinDistSq[0])*ClosestProbes[0]->LightContributions[4].xyz +
							   (Numerator/MinDistSq[1])*ClosestProbes[1]->LightContributions[4].xyz +
							   (Numerator/MinDistSq[2])*ClosestProbes[2]->LightContributions[4].xyz +
							   (Numerator/MinDistSq[3])*ClosestProbes[3]->LightContributions[4].xyz;
					*VoxelNZ = (Numerator/MinDistSq[0])*ClosestProbes[0]->LightContributions[5].xyz +
							   (Numerator/MinDistSq[1])*ClosestProbes[1]->LightContributions[5].xyz +
							   (Numerator/MinDistSq[2])*ClosestProbes[2]->LightContributions[5].xyz +
							   (Numerator/MinDistSq[3])*ClosestProbes[3]->LightContributions[5].xyz;
				}

#else
				*VoxelPX = ClosestProbes[0]->LightContributions[0];
				*VoxelNX = ClosestProbes[0]->LightContributions[1];
				*VoxelPY = ClosestProbes[0]->LightContributions[2];
				*VoxelNY = ClosestProbes[0]->LightContributions[3];
				*VoxelPZ = ClosestProbes[0]->LightContributions[4];
				*VoxelNZ = ClosestProbes[0]->LightContributions[5];
#endif
			}
		}
	}

	return Result;
}

internal rectangle3
BoundFrustrum(v3 ViewRayDirection, f32 Near, f32 Far,
	quaternion CameraRotation, v3 CameraP,
	quaternion LightRotation)
{
	v3 NearRay = Near*ViewRayDirection;
	v3 ViewRay = Far*ViewRayDirection;
	v3 FrustrumPointsLightSpace[8] =
	{
		RotateBy(RotateBy(NearRay, CameraRotation) + CameraP, Conjugate(LightRotation)),
		RotateBy(RotateBy(Hadamard(NearRay, V3(-1,1,1)), CameraRotation) + CameraP, Conjugate(LightRotation)),
		RotateBy(RotateBy(Hadamard(NearRay, V3(-1,-1,1)), CameraRotation) + CameraP, Conjugate(LightRotation)),
		RotateBy(RotateBy(Hadamard(NearRay, V3(1,-1,1)), CameraRotation) + CameraP, Conjugate(LightRotation)),
		RotateBy(RotateBy(ViewRay, CameraRotation) + CameraP, Conjugate(LightRotation)),
		RotateBy(RotateBy(Hadamard(ViewRay, V3(-1,1,1)), CameraRotation) + CameraP, Conjugate(LightRotation)),
		RotateBy(RotateBy(Hadamard(ViewRay, V3(-1,-1,1)), CameraRotation) + CameraP, Conjugate(LightRotation)),
		RotateBy(RotateBy(Hadamard(ViewRay, V3(1,-1,1)), CameraRotation) + CameraP, Conjugate(LightRotation)),
	};

	v3 LightMin = V3(Real32Maximum, Real32Maximum, Real32Maximum);
	v3 LightMax = V3(Real32Minimum, Real32Minimum, Real32Minimum);
	for(u32 PointIndex = 0;
		PointIndex < ArrayCount(FrustrumPointsLightSpace);
		++PointIndex)
	{
		v3 LightSpace = FrustrumPointsLightSpace[PointIndex];
		if(LightSpace.x < LightMin.x) {LightMin.x = LightSpace.x;}
		if(LightSpace.y < LightMin.y) {LightMin.y = LightSpace.y;}
		if(LightSpace.z < LightMin.z) {LightMin.z = LightSpace.z;}
		if(LightSpace.x > LightMax.x) {LightMax.x = LightSpace.x;}
		if(LightSpace.y > LightMax.y) {LightMax.y = LightSpace.y;}
		if(LightSpace.z > LightMax.z) {LightMax.z = LightSpace.z;}
	}

	rectangle3 Result = Rectangle3(LightMin, LightMax);
	v3 RectDim = Dim(Result);
	Assert(RectDim.x > 0.0f);
	Assert(RectDim.y > 0.0f);
	Assert(RectDim.z > 0.0f);
	return Result;
}

internal light_space_cascades
CalculateLightSpaceMatrices(render_setup *Setup, directional_light *Light)
{
	TIMED_FUNCTION();

	light_space_cascades Result = {};
	Assert(Light && Setup);

	f32 CascadeBoundsNormalized[CASCADE_COUNT + 1] =
	{
		0.0f,
		0.25f,
		0.5f,
		0.75f,
		1.0f,
	};

	for(u32 CascadeIndex = 0;
		CascadeIndex < CASCADE_COUNT + 1;
		++CascadeIndex)
	{
		Result.CascadeBounds[CascadeIndex] = Lerp(Setup->NearPlane, CascadeBoundsNormalized[CascadeIndex], Setup->FarPlane);
	}

	quaternion LightRotation = RotationQuaternion(V3(0,0,-1), Light->Direction);
	f32 PullBack = 50.0f;
	v3 ViewRayDirection = (-1.0f/Setup->ViewRay.z)*Setup->ViewRay;
	for(u32 CascadeIndex = 0;
		CascadeIndex < CASCADE_COUNT;
		++CascadeIndex)
	{
		rectangle3 LightSpaceBounds = BoundFrustrum(ViewRayDirection,
			Result.CascadeBounds[CascadeIndex], Result.CascadeBounds[CascadeIndex+1],
			Setup->CameraRotation, Setup->P, LightRotation);

		v3 LightCenter = Center(LightSpaceBounds);
		v3 LightPLightSpace = V3(LightCenter.xy, LightSpaceBounds.Max.z + PullBack);
		v3 LightDim = Dim(LightSpaceBounds);

		Result.FarPlane[CascadeIndex] = LightDim.z + PullBack;
		Result.Projection[CascadeIndex] = OrthogonalProjectionMat4(0.5f*LightDim.x, 0.5f*LightDim.y, 0, Result.FarPlane[CascadeIndex]);
		Result.Orthogonal[CascadeIndex] = true;
		v3 LightWorldP = RotateBy(LightPLightSpace, LightRotation);
		Result.View[CascadeIndex] = ViewMat4(LightWorldP, LightRotation);
	}

	return Result;
}
