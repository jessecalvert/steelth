/*@H
* File: steelth_renderer_opengl.cpp
* Author: Jesse Calvert
* Created: November 27, 2017, 15:38
* Last modified: April 27, 2019, 18:29
*/

inline void
DrawMesh(opengl_state *State, u32 MeshIndex)
{
	opengl_mesh_info *MeshInfo = State->Meshes + MeshIndex;
	glBindVertexArray(MeshInfo->VAO);
	glDrawElements(GL_TRIANGLES, 3*MeshInfo->FaceCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

inline void
DrawCube(opengl_state *State)
{
	DrawMesh(State, State->Cube.Index);
}

inline void
DrawQuad(opengl_state *State)
{
	DrawMesh(State, State->Quad.Index);
}

#include "steelth_renderer_opengl_framebuffers.cpp"
#include "steelth_renderer_opengl_shaders.cpp"
#include "steelth_renderer_opengl_bake.cpp"

OPENGL_DEBUG_CALLBACK(OpenGLDebugCallback)
{
	stream *Info = (stream *)userParam;
	char *ErrorMessage = (char *)message;
	u32 Length = (u32)length;

	if(severity == GL_DEBUG_SEVERITY_HIGH)
	{
		Assert(!"OpenGL error.");
	}
	else if(severity == GL_DEBUG_SEVERITY_MEDIUM)
	{
		string OpenGLMessage = {Length, ErrorMessage};
		Outf(Info, "Severity Medium : %s", OpenGLMessage);
	}
	else if(severity == GL_DEBUG_SEVERITY_LOW)
	{
		string OpenGLMessage = {Length, ErrorMessage};
		Outf(Info, "Severity Low : %s", OpenGLMessage);
	}
}

internal opengl_info
OpenGLGetInfo()
{
	opengl_info Result = {};

	Result.Vendor = (char *)glGetString(GL_VENDOR);
	Result.Renderer = (char *)glGetString(GL_RENDERER);
	Result.Version = (char *)glGetString(GL_VERSION);
	Result.ShadingLanguageVersion = (char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
	Result.Extensions = (char *)glGetString(GL_EXTENSIONS);

	char *At = Result.Extensions;
	while(*At)
	{
		while(IsWhitespace(*At)) {++At;}
		char *End = At;
		while(*End && !IsWhitespace(*End)) {++End;}

		umm Count = End - At;

		if(0) {}
        else if(StringsAreEqual(Count, At, "GL_EXT_texture_sRGB")) {Result.GL_EXT_texture_sRGB=true;}
        else if(StringsAreEqual(Count, At, "GL_EXT_framebuffer_sRGB")) {Result.GL_EXT_framebuffer_sRGB=true;}
        else if(StringsAreEqual(Count, At, "GL_ARB_framebuffer_sRGB")) {Result.GL_EXT_framebuffer_sRGB=true;}

		At = End;
	}

    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &Result.MaxSamplersPerShader);
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &Result.MaxDrawbuffers);
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &Result.MaxComputeWorkGroupInvocations);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &Result.MaxComputeWorkGroupCount[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &Result.MaxComputeWorkGroupCount[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &Result.MaxComputeWorkGroupCount[2]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &Result.MaxComputeWorkGroupSize[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &Result.MaxComputeWorkGroupSize[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &Result.MaxComputeWorkGroupSize[2]);
    glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &Result.MaxComputeSharedMemorySize);

	return Result;
}

internal void
OpenGLSetUpVertexAttribs()
{
	// NOTE: A vertex array should be bound first!
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_format), (GLvoid *)OffsetOf(vertex_format, Position));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_format), (GLvoid *)OffsetOf(vertex_format, Normal));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_format), (GLvoid *)OffsetOf(vertex_format, TexCoords));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(vertex_format), (GLvoid *)OffsetOf(vertex_format, Color));
	glEnableVertexAttribArray(3);

	glVertexAttribIPointer(4, 1, GL_UNSIGNED_INT, sizeof(vertex_format), (GLvoid *)OffsetOf(vertex_format, TextureIndex));
	glEnableVertexAttribArray(4);

	glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_format), (GLvoid *)OffsetOf(vertex_format, RME));
	glEnableVertexAttribArray(5);
}

internal void
OpenGLFreeMesh(opengl_state *State, opengl_mesh_info *MeshInfo)
{
	if(MeshInfo->Mesh.Index)
	{
		glDeleteVertexArrays(1, &MeshInfo->VAO);
		glDeleteBuffers(1, &MeshInfo->VBO);
		glDeleteBuffers(1, &MeshInfo->EBO);
		MeshInfo->Mesh = {};
	}
}

internal void
OpenGLLoadMesh(opengl_state *State, vertex_format *Vertices, v3u *Faces, renderer_mesh Mesh)
{
	opengl_mesh_info *MeshInfo = State->Meshes + Mesh.Index;
	OpenGLFreeMesh(State, MeshInfo);
	MeshInfo->Mesh = Mesh;
	MeshInfo->VertexCount = Mesh.VertexCount;
	MeshInfo->FaceCount = Mesh.FaceCount;

	glGenVertexArrays(1, &MeshInfo->VAO);
	glGenBuffers(1, &MeshInfo->VBO);
	glGenBuffers(1, &MeshInfo->EBO);

	glBindVertexArray(MeshInfo->VAO);

	glBindBuffer(GL_ARRAY_BUFFER, MeshInfo->VBO);
	glBufferData(GL_ARRAY_BUFFER, Mesh.VertexCount*sizeof(vertex_format), Vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, MeshInfo->EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, Mesh.FaceCount*sizeof(v3u), Faces, GL_STATIC_DRAW);

	OpenGLSetUpVertexAttribs();

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

internal opengl_state *
InitOpenGL(stream *Info)
{
	opengl_state *State = BootstrapPushStruct(opengl_state, Region, NonRestoredParams());

	State->OpenGLInfo = OpenGLGetInfo();
	State->Info = Info;
	OpenGLSetShaderFilenames(State);

#if GAME_DEBUG
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(OpenGLDebugCallback, Info);
#endif

	// TODO: Pass these in
	State->MaxTextureCount = 32;
	State->MaxMeshCount = 32;
	State->Meshes = PushArray(&State->Region, State->MaxMeshCount, opengl_mesh_info);

	glEnable(GL_MULTISAMPLE);

	glDepthRange(1.0f, 0.0f);
	glCullFace(GL_BACK);

	glGenVertexArrays(1, &State->VertexArray);
	glBindVertexArray(State->VertexArray);

	glGenBuffers(1, &State->ArrayBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, State->ArrayBuffer);

	glGenBuffers(1, &State->IndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, State->IndexBuffer);

	OpenGLSetUpVertexAttribs();

	glBindVertexArray(0);

	glGenTextures(1, &State->TextureArray);
	glBindTexture(GL_TEXTURE_2D_ARRAY, State->TextureArray);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, MAX_TEXTURE_DIM, MAX_TEXTURE_DIM, State->MaxTextureCount);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	vertex_format BasicQuadVertices[] =
	{
		{{0.0f, 0.0f, 0.0f}, {0,0,1}, {0.0f, 0.0f}, {0xFFFFFFFF}},
		{{0.0f, 1.0f, 0.0f}, {0,0,1}, {0.0f, 1.0f}, {0xFFFFFFFF}},
		{{1.0f, 1.0f, 0.0f}, {0,0,1}, {1.0f, 1.0f}, {0xFFFFFFFF}},
		{{1.0f, 0.0f, 0.0f}, {0,0,1}, {1.0f, 0.0f}, {0xFFFFFFFF}},
	};
	v3u BasicQuadFaces[] =
	{
		{0, 3, 2},
		{0, 2, 1},
	};

	// TODO: These are taking up indexes that the app could want to use!
	State->Quad = RendererMesh(25, ArrayCount(BasicQuadVertices), ArrayCount(BasicQuadFaces));
	OpenGLLoadMesh(State, BasicQuadVertices, BasicQuadFaces, State->Quad);

	vertex_format BasicCubeVertices[] =
	{
		{{-1.0f, -1.0f, -1.0f}, {}, {}, {0xFFFFFFFF}},
		{{ 1.0f, -1.0f, -1.0f}, {}, {}, {0xFFFFFFFF}},
		{{-1.0f,  1.0f, -1.0f}, {}, {}, {0xFFFFFFFF}},
		{{-1.0f, -1.0f,  1.0f}, {}, {}, {0xFFFFFFFF}},
		{{-1.0f,  1.0f,  1.0f}, {}, {}, {0xFFFFFFFF}},
		{{ 1.0f, -1.0f,  1.0f}, {}, {}, {0xFFFFFFFF}},
		{{ 1.0f,  1.0f, -1.0f}, {}, {}, {0xFFFFFFFF}},
		{{ 1.0f,  1.0f,  1.0f}, {}, {}, {0xFFFFFFFF}},
	};
	v3u BasicCubeFaces[] =
	{
		{0, 2, 1},
		{2, 6, 1},
		{0, 3, 4},
		{0, 4, 2},
		{0, 1, 5},
		{0, 5, 3},
		{2, 4, 7},
		{2, 7, 6},
		{5, 6, 7},
		{5, 1, 6},
		{3, 5, 4},
		{5, 7, 4},
	};

	// TODO: These are taking up indexes that the app could want to use!
	State->Cube = RendererMesh(0, ArrayCount(BasicCubeVertices), ArrayCount(BasicCubeFaces));
	OpenGLLoadMesh(State, BasicCubeVertices, BasicCubeFaces, State->Cube);

	// TODO: These are taking up indexes that the app could want to use!
	u32 WhitePixels[4] =
	{
		0xFFFFFFFF,
		0xFFFFFFFF,
		0xFFFFFFFF,
		0xFFFFFFFF,
	};
	State->WhiteTexture = RendererTexture(0, 2, 2);
	OpenGLLoadTexture(State, WhitePixels, State->WhiteTexture);
	State->WhiteTexture = RendererTexture(0, 1, 1);

	glEnable(GL_TEXTURE_3D);
	glGenTextures(1, &State->IrradianceVolume);

	glBindTexture(GL_TEXTURE_3D, State->IrradianceVolume);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, 6*IRRADIANCE_VOLUME_WIDTH, IRRADIANCE_VOLUME_HEIGHT, IRRADIANCE_VOLUME_WIDTH, 0, GL_RGB, GL_FLOAT, 0);
    glBindTexture(GL_TEXTURE_3D, 0);

	glGenBuffers(1, &State->SurfelBuffer);
	glGenBuffers(1, &State->SurfelRefBuffer);
	glGenBuffers(1, &State->ProbeBuffer);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, State->SurfelBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_SURFELS*sizeof(surface_element), 0, GL_DYNAMIC_COPY);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, State->SurfelRefBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_SURFELREFS*sizeof(u32), 0, GL_DYNAMIC_COPY);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, State->ProbeBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_PROBES*sizeof(light_probe), 0, GL_DYNAMIC_COPY);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	v2i BlueNoiseResolution = V2i(256, 256);
	v2i BlueNoiseFileDim = V2i(4, 2);
	texture_settings BlueNoiseSettings = TextureSettings(BlueNoiseResolution.x*BlueNoiseFileDim.x, BlueNoiseResolution.y*BlueNoiseFileDim.y, 4, 4, Texture_FilterNearest|Texture_Float);
	State->BlueNoiseTexture = OpenGLAllocateAndLoadTexture(BlueNoiseSettings);
	string BlueNoiseFilenames[] =
	{
		ConstZ("Textures/HDR_RGBA_0.png"),
		ConstZ("Textures/HDR_RGBA_1.png"),
		ConstZ("Textures/HDR_RGBA_2.png"),
		ConstZ("Textures/HDR_RGBA_3.png"),
		ConstZ("Textures/HDR_RGBA_4.png"),
		ConstZ("Textures/HDR_RGBA_5.png"),
		ConstZ("Textures/HDR_RGBA_6.png"),
		ConstZ("Textures/HDR_RGBA_7.png"),
	};

	for(s32 Y = 0;
		Y < BlueNoiseFileDim.y;
		++Y)
	{
		for(s32 X = 0;
			X < BlueNoiseFileDim.x;
			++X)
		{
			u32 Index = X + Y*BlueNoiseFileDim.x;
			f32 *BlueNoiseTexture = Platform->LoadHDRPNG(&State->Region, BlueNoiseFilenames[Index], 256, 256, 4);
			glTextureSubImage2D(State->BlueNoiseTexture, 0,
				X*BlueNoiseResolution.x, Y*BlueNoiseResolution.y,
				BlueNoiseResolution.x, BlueNoiseResolution.y,
				GL_RGBA, GL_FLOAT, BlueNoiseTexture);
		}
	}

	return State;
}

internal void
OpenGLChangeSettings(opengl_state *State, render_settings *NewSettings)
{
	TIMED_FUNCTION();

	if(!AreEqual(NewSettings, &State->CurrentRenderSettings))
	{
		render_settings OldSettings = State->CurrentRenderSettings;
		State->CurrentRenderSettings = *NewSettings;
		State->CurrentRenderSettings.SwapInterval = OldSettings.SwapInterval; // NOTE: This is set closer to the platform layer.

		// TODO: We don't have to destroy and recreate everything on
		// any change to render_settings.

		for(u32 FramebufferIndex = 1;
			FramebufferIndex < Framebuffer_Count;
			++FramebufferIndex)
		{
			opengl_framebuffer *Framebuffer = State->Framebuffers + FramebufferIndex;
			OpenGLFreeFramebuffer(Framebuffer);
		}

		// TODO: Depending on the settings, there are framebuffers and
		// shaders we don't have to load.
		OpenGLCreateAllFramebuffers(State);

		for(u32 ShaderIndex = Shader_Header + 1;
			ShaderIndex < Shader_Count;
			++ShaderIndex)
		{
			OpenGLReloadShader(State, (shader_type) ShaderIndex);
		}
		State->ShadersLoaded = true;
	}
}

internal void
NextJitter(opengl_state *State)
{
	++State->JitterIndex;

	v2 Halton23[] =
	{
		V2(1.0f/2.0f, 1.0f/3.0f),
		V2(1.0f/4.0f, 2.0f/3.0f),
		V2(3.0f/4.0f, 1.0f/9.0f),
		V2(1.0f/8.0f, 4.0f/9.0f),
		V2(5.0f/8.0f, 7.0f/9.0f),
		V2(3.0f/8.0f, 2.0f/9.0f),
		V2(7.0f/8.0f, 5.0f/9.0f),
		V2(1.0f/16.0f, 8.0f/9.0f),
		V2(9.0f/16.0f, 1.0f/27.0f),
	};
	v2 Jitter = Halton23[State->JitterIndex % ArrayCount(Halton23)];

	Jitter = 2.0f*Jitter - V2(1,1);
	Jitter.x /= State->CurrentRenderSettings.Resolution.x;
	Jitter.y /= State->CurrentRenderSettings.Resolution.x;
	State->Jitter = Jitter;
}

internal void
OpenGLClipSetup(rectangle2i ClipRect)
{
	u32 Width = ClipRect.Max.x - ClipRect.Min.x;
	u32 Height = ClipRect.Max.y - ClipRect.Min.y;
	glScissor(ClipRect.Min.x, ClipRect.Min.y, ClipRect.Max.x, ClipRect.Max.y);
}

internal void
OpenGLRenderSetup(opengl_state *State, render_setup *Setup,
	b32 ShadowMapping)
{
	TIMED_FUNCTION();

	if(!ShadowMapping)
	{
		OpenGLClipSetup(Setup->ClipRect);
	}

	if(Setup->Flags & RenderFlag_TopMost)
	{
		glDisable(GL_DEPTH_TEST);
		// glDisable(GL_CULL_FACE);
	}
	else
	{
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
	}

	if(Setup->Flags & RenderFlag_AlphaBlended)
	{
		glEnable(GL_BLEND);
	}
	else
	{
		glDisable(GL_BLEND);
	}
}

internal void
OpenGLRenderQuad(opengl_state *State, v2 Min, v2 Max)
{
	v2 Scale = Max - Min;
	mat4 Transform =
	{
		2.0f*Scale.x, 0.0f, 0.0f, 0.0f,
		0.0f, 2.0f*Scale.y, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		2.0f*Min.x - 1.0f, 2.0f*Min.y - 1.0f, 0.0f, 1.0f,
	};

	SetUniform(State->CurrentShader->Transform, Transform);
	DrawQuad(State);
}

internal void
OpenGLLoadBufferData(opengl_state *State, game_render_commands *RenderCommands)
{
	TIMED_FUNCTION();

	glBindBuffer(GL_ARRAY_BUFFER, State->ArrayBuffer);
	glBufferData(GL_ARRAY_BUFFER,
		RenderCommands->VertexCount*sizeof(vertex_format),
		RenderCommands->VertexArray,
		GL_STREAM_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, State->IndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		RenderCommands->IndexCount*sizeof(RenderCommands->IndexArray[0]),
		RenderCommands->IndexArray,
		GL_STREAM_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

internal void
OpenGLExecuteRenderCommands(opengl_state *State, opengl_shader_common *Shader,
	game_render_commands *RenderCommands, render_target_type Target,
	light_space_cascades *Cascades = 0, u32 CascadeIndex = 0)
{
	b32 ShadowMapping = (Cascades != 0);

	u32 BytesRead = 0;
	u32 DepthPeelBeginSnapshot = 0;
	while(BytesRead < RenderCommands->Size[Target])
	{
		render_entry_header *Header = (render_entry_header *)(RenderCommands->Buffer[Target] + BytesRead);
		BytesRead += sizeof(render_entry_header);
		switch(Header->Type)
		{
			case RenderCommandType_render_entry_set_target:
			{
				render_entry_set_target *Entry = (render_entry_set_target *)(Header + 1);
				BytesRead += sizeof(render_entry_set_target);

				Assert(Entry->Target == Target);
			} break;

			case RenderCommandType_render_entry_full_clear:
			{
				render_entry_full_clear *Entry = (render_entry_full_clear *)(Header + 1);
				BytesRead += sizeof(render_entry_full_clear);

				f32 MaxDepth = 1.0f;
				glClearBufferfv(GL_DEPTH, 0, &MaxDepth);

				switch(Shader->Type)
				{
					case Shader_GBuffer:
					{
						glClearBufferfv(GL_COLOR, 0, Entry->Color.E); // Albedo
						glClearBufferfv(GL_COLOR, 1, V3(0,0,0).E); // Normals
						glClearBufferfv(GL_COLOR, 2, V2(1.0f, 0).E); // Roughness_Metalness_Emission
					} break;

					case Shader_ShadowMap:
					{
						glClearBufferfv(GL_COLOR, 0, V2(MaxDepth, Square(MaxDepth)).E); // Depth Depth_2
					} break;

					case Shader_Overlay:
					{
						glClearBufferfv(GL_COLOR, 0, Entry->Color.E); // Albedo
					} break;
				}
			} break;

			case RenderCommandType_render_entry_clear_depth:
			{
				f32 MaxDepth = 1.0f;
				glClearBufferfv(GL_DEPTH, 0, &MaxDepth);
			} break;

			case RenderCommandType_render_entry_meshes:
			{
				TIMED_BLOCK("OpenGL Render Meshes");

				render_entry_meshes *Entry = (render_entry_meshes *)(Header + 1);
				render_setup *Setup = &Entry->Setup;
				OpenGLRenderSetup(State, Setup, ShadowMapping);

				Assert(Setup->Target == Target_Scene);
				Assert(!(Setup->Flags & RenderFlag_SDF));
				SetShaderCameraUniforms(State, Shader, Setup, Cascades, CascadeIndex);
				BindTexture(0, State->TextureArray, GL_TEXTURE_2D_ARRAY);

				if(State->Wireframes)
				{
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				}

				for(u32 MeshIndex = 0;
					MeshIndex < Entry->MeshCount;
					++MeshIndex)
				{
					render_command_mesh_data *MeshData = RenderCommands->MeshArray + Entry->MeshArrayOffset + MeshIndex;
					SetShaderMeshUniforms(Shader, MeshData);
					DrawMesh(State, MeshData->MeshIndex);
				}

				BindTexture(0, 0, GL_TEXTURE_2D_ARRAY);

				if(State->Wireframes)
				{
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				}

				BytesRead += sizeof(render_entry_meshes);
			} break;

			case RenderCommandType_render_entry_quads:
			{
				TIMED_BLOCK("OpenGL Render Quads");

				render_entry_quads *Entry = (render_entry_quads *)(Header + 1);
				render_setup *Setup = &Entry->Setup;
				OpenGLRenderSetup(State, Setup, ShadowMapping);
				SetShaderCameraUniforms(State, Shader, Setup, Cascades, CascadeIndex);

				glBindVertexArray(State->VertexArray);
				// glBindBuffer(GL_ARRAY_BUFFER, State->ArrayBuffer);
				// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, State->IndexBuffer);

				BindTexture(0, State->TextureArray, GL_TEXTURE_2D_ARRAY);
				glDrawElements(GL_TRIANGLES, 6*Entry->QuadCount, GL_UNSIGNED_INT, (GLvoid *)(Entry->IndexArrayOffset*sizeof(u32)));

				BindTexture(0, 0, GL_TEXTURE_2D_ARRAY);
				glBindVertexArray(0);

				BytesRead += sizeof(render_entry_quads);
			} break;

			InvalidDefaultCase;
		}
	}
}

internal void
OpenGLMemoryOp(opengl_state *State, render_memory_op *Op)
{
	switch(Op->Type)
	{
		case RenderOp_LoadMesh:
		{
			load_mesh_op *LoadMeshOp = &Op->LoadMesh;
			renderer_mesh Mesh = LoadMeshOp->Mesh;
			OpenGLLoadMesh(State, (vertex_format *)LoadMeshOp->VertexMemory,
				(v3u *)LoadMeshOp->FaceMemory, Mesh);
		} break;

		case RenderOp_LoadTexture:
		{
			load_texture_op *LoadTextureOp = &Op->LoadTexture;
			renderer_texture Texture = LoadTextureOp->Texture;
			OpenGLLoadTexture(State, LoadTextureOp->Pixels, Texture);
		} break;

		case RenderOp_ReloadShader:
		{
			reload_shader_op *ReloadShaderOp = &Op->ReloadShader;
			OpenGLReloadShader(State, ReloadShaderOp->ShaderType);
		} break;

		case RenderOp_BakeProbes:
		{
			bake_probes_op *BakeProbesOp = &Op->BakeProbes;
			OpenGLBakeLightProbes(State,
				BakeProbesOp->Geometry, State->WhiteTexture,
				BakeProbesOp->ProbePositions, BakeProbesOp->ProbeCount,
				BakeProbesOp->Solution);
		} break;

		InvalidDefaultCase;
	}
}

internal void
OpenGLProcessMemoryOpQueue(opengl_state *State, render_memory_op_queue *RenderOpQueue)
{
	BeginTicketMutex(&RenderOpQueue->Mutex);
	render_memory_op *FirstRenderOp = RenderOpQueue->First;
	render_memory_op *LastRenderOp = RenderOpQueue->Last;
	RenderOpQueue->First = RenderOpQueue->Last = 0;
	EndTicketMutex(&RenderOpQueue->Mutex);

	if(FirstRenderOp)
	{
		for(render_memory_op *RenderOp = FirstRenderOp;
			RenderOp;
			RenderOp = RenderOp->Next)
		{
			OpenGLMemoryOp(State, RenderOp);
		}

		BeginTicketMutex(&RenderOpQueue->Mutex);
		LastRenderOp->Next = RenderOpQueue->FirstFree;
		RenderOpQueue->FirstFree = FirstRenderOp;
		EndTicketMutex(&RenderOpQueue->Mutex);
	}
}

internal opengl_framebuffer *
OpenGLBlur(opengl_state *State, opengl_framebuffer *Source, u32 KernelCount = 5)
{
	opengl_framebuffer *Result = 0;

	shader_blur *Blur = (shader_blur *) BeginShader(State, Shader_Blur);
	opengl_framebuffer *BlurDest = State->Framebuffers + Framebuffer_Blur0;
	opengl_framebuffer *BlurSource = Source;

	Assert(BlurDest->Resolution == BlurSource->Resolution);

	s32 Kernels[] = {0, 1, 2, 2, 3};

	Assert(KernelCount <= ArrayCount(Kernels));
	for(u32 Index = 0;
		Index < KernelCount;
		++Index)
	{
		OpenGLBindFramebuffer(State, BlurDest);
		BindTexture(0, BlurSource->Textures[FBT_Color]);
		SetUniform(Blur->Kernel, Kernels[Index]);
		OpenGLRenderQuad(State, V2(0,0), V2(1,1));
		BindTexture(0, 0);

		Result = BlurDest;
		BlurSource = State->Framebuffers + Framebuffer_Blur0 + (Index % 2);
		BlurDest = State->Framebuffers + Framebuffer_Blur0 + ((Index + 1) % 2);
	}

	EndShader(State);

	return Result;
}

internal opengl_framebuffer *
OpenGLBlurShadowMap(opengl_state *State, opengl_framebuffer *Source, u32 KernelCount = 5)
{
	opengl_framebuffer *Result = 0;

	shader_blur *Blur = (shader_blur *) BeginShader(State, Shader_Blur);
	opengl_framebuffer *BlurDest = State->Framebuffers + Framebuffer_ShadowMap1;
	opengl_framebuffer *BlurSource = Source;

	Assert(BlurDest->Resolution == BlurSource->Resolution);

	s32 Kernels[] = {0, 1, 2, 2, 3};

	Assert(KernelCount <= ArrayCount(Kernels));
	for(u32 Index = 0;
		Index < KernelCount;
		++Index)
	{
		OpenGLBindFramebuffer(State, BlurDest);
		BindTexture(0, BlurSource->Textures[FBT_Color]);
		SetUniform(Blur->Kernel, Kernels[Index]);
		OpenGLRenderQuad(State, V2(0,0), V2(1,1));
		BindTexture(0, 0);

		Result = BlurDest;
		BlurSource = State->Framebuffers + Framebuffer_ShadowMap0 + (Index % 2);
		BlurDest = State->Framebuffers + Framebuffer_ShadowMap0 + ((Index + 1) % 2);
	}

	EndShader(State);

	return Result;
}

internal v2
DebugNextFramebufferPlacement(v2 At, f32 Spacing, v2 Dim)
{
	v2 Result = At + V2(Dim.x + Spacing, 0);
	if((Result.x + Dim.x) > 1.0f)
	{
		Result.x = Spacing;
		Result.y = At.y + Spacing + Dim.y;
	}

	return Result;
}

internal void
DebugFramebufferDisplay(opengl_state *State)
{
	opengl_framebuffer *SceneBuffer = State->Framebuffers + Framebuffer_Scene;
	opengl_framebuffer *GBuffer = State->Framebuffers + Framebuffer_GBuffer;
	opengl_framebuffer *OverlayFramebuffer = State->Framebuffers + Framebuffer_Overlay;
	opengl_framebuffer *OverbrightFramebuffer = State->Framebuffers + Framebuffer_Overbright;
	opengl_framebuffer *FresnelFramebuffer = State->Framebuffers + Framebuffer_Fresnel;
	opengl_framebuffer *ShadowMap = State->Framebuffers + Framebuffer_ShadowMap0;
	opengl_framebuffer *FogBuffer = State->Framebuffers + Framebuffer_Fog;

	f32 Spacing = 0.005f;
	v2 At = V2(Spacing, Spacing);
	v2 Dim = V2(0.2f, 0.2f);

	OpenGLBindFramebuffer(State, 0);
	shader_debug_view *Debug = (shader_debug_view *) BeginShader(State, Shader_DebugView);

	// BindTexture(0, GBuffer->Textures[FBT_Albedo]);
	// OpenGLRenderQuad(State, At, At + Dim);
	// At = DebugNextFramebufferPlacement(At, Spacing, Dim);

	// BindTexture(0, GBuffer->Textures[FBT_Depth]);
	// OpenGLRenderQuad(State, At, At + Dim);
	// At = DebugNextFramebufferPlacement(At, Spacing, Dim);

	// BindTexture(0, GBuffer->Textures[FBT_Normal]);
	// OpenGLRenderQuad(State, At, At + Dim);
	// At = DebugNextFramebufferPlacement(At, Spacing, Dim);

	// BindTexture(0, GBuffer->Textures[FBT_Roughness_Metalness_Emission]);
	// OpenGLRenderQuad(State, At, At + Dim);
	// At = DebugNextFramebufferPlacement(At, Spacing, Dim);

	BindTexture(0, ShadowMap->Textures[FBT_Depth]);
	OpenGLRenderQuad(State, At, At + Dim);
	At = DebugNextFramebufferPlacement(At, Spacing, Dim);

	BindTexture(0, ShadowMap->Textures[FBT_Color]);
	OpenGLRenderQuad(State, At, At + Dim);
	At = DebugNextFramebufferPlacement(At, Spacing, Dim);

	// BindTexture(0, FresnelFramebuffer->Textures[FBT_Color]);
	// OpenGLRenderQuad(State, At, At + Dim);
	// At = DebugNextFramebufferPlacement(At, Spacing, Dim);

	// BindTexture(0, SceneBuffer->Textures[FBT_Color]);
	// OpenGLRenderQuad(State, At, At + Dim);
	// At = DebugNextFramebufferPlacement(At, Spacing, Dim);

	// BindTexture(0, OverbrightFramebuffer->Textures[FBT_Color]);
	// OpenGLRenderQuad(State, At, At + Dim);
	// At = DebugNextFramebufferPlacement(At, Spacing, Dim);

	// BindTexture(0, FogBuffer->Textures[FBT_Color]);
	// OpenGLRenderQuad(State, At, At + Dim);
	// At = DebugNextFramebufferPlacement(At, Spacing, Dim);

	BindTexture(0, 0);
}

internal void
OpenGLBeginFrame(opengl_state *State, render_settings *RenderSettings)
{
	OpenGLChangeSettings(State, RenderSettings);
	State->CurrentSceneRenderSetup = 0;
}

internal void
OpenGLEndFrame(opengl_state *State, game_input *Input)
{
	TIMED_FUNCTION();

	if(Input->KeyStates[Key_1].WasPressed)
	{
		State->DebugDisplayFramebuffers = !State->DebugDisplayFramebuffers;
	}
	if(Input->KeyStates[Key_3].WasPressed)
	{
		State->DebugTurnOffDirectLighting = !State->DebugTurnOffDirectLighting;
	}
	if(Input->KeyStates[Key_4].WasPressed)
	{
		State->DebugReadIrradianceResults = !State->DebugReadIrradianceResults;
	}

	{DEBUG_DATA_BLOCK("OpenGL Renderer");
		DEBUG_VALUE(&State->Wireframes);
	}

	CheckMemory(&State->Region);
	NextJitter(State);
	Assert(State->BakedData);

	opengl_framebuffer *SceneBuffer = State->Framebuffers + Framebuffer_Scene;
	opengl_framebuffer *GBuffer = State->Framebuffers + Framebuffer_GBuffer;
	opengl_framebuffer *OverlayFramebuffer = State->Framebuffers + Framebuffer_Overlay;
	opengl_framebuffer *OverbrightFramebuffer = State->Framebuffers + Framebuffer_Overbright;
	opengl_framebuffer *FresnelFramebuffer = State->Framebuffers + Framebuffer_Fresnel;
	opengl_framebuffer *ShadowMap = State->Framebuffers + Framebuffer_ShadowMap0;
	opengl_framebuffer *FogBuffer = State->Framebuffers + Framebuffer_Fog;

	{DEBUG_DATA_BLOCK("Memory");
		memory_region *OpenGLRegion = &State->Region;
		DEBUG_VALUE(OpenGLRegion);
	}

	{
		// NOTE: Render GBuffer
		OpenGLLoadBufferData(State, &State->RenderCommands);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glCullFace(GL_BACK);
		glEnable(GL_SCISSOR_TEST);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_DEPTH_TEST);

		OpenGLBindFramebuffer(State, GBuffer);
		opengl_shader_common *Shader = BeginShader(State, Shader_GBuffer);
		OpenGLExecuteRenderCommands(State, Shader, &State->RenderCommands, Target_Scene);
		EndShader(State);

		OpenGLBindFramebuffer(State, OverlayFramebuffer);
		Shader = BeginShader(State, Shader_Overlay);
		OpenGLExecuteRenderCommands(State, Shader, &State->RenderCommands, Target_Overlay);
		EndShader(State);

		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glDisable(GL_SCISSOR_TEST);
	}

	render_setup *SceneSetup = State->CurrentSceneRenderSetup;
	{
		// NOTE: Decal Pass
		OpenGLBindFramebuffer(State, GBuffer);
		shader_decal *DecalShader = (shader_decal *) BeginShader(State, Shader_Decal);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		BindTexture(0, State->TextureArray, GL_TEXTURE_2D_ARRAY);
		BindTexture(1, GBuffer->Textures[FBT_Depth]);
		SetUniform(DecalShader->ViewRay, SceneSetup->ViewRay);
		SetUniform(DecalShader->InvView, SceneSetup->InvView);

		SetUniform(DecalShader->Projection, SceneSetup->Projection);
		SetUniform(DecalShader->View, SceneSetup->View);

		for(u32 Index = 0;
			Index < State->RenderCommands.DecalCount;
			++Index)
		{
			decal *Decal = State->RenderCommands.Decals + Index;

			mat4 Model = TranslationMat4(Decal->Box.P)*RotationMat4(Decal->Box.Rotation)*ScaleMat4(0.5f*Decal->Box.Dim);
			SetUniform(DecalShader->Model, Model);

			v2 TexScale = V2((f32)Decal->TextureHandle.Width/(f32)MAX_TEXTURE_DIM,
						     (f32)Decal->TextureHandle.Height/(f32)MAX_TEXTURE_DIM);
			SetUniform(DecalShader->DecalTextureIndex, Decal->TextureHandle.Index);
			SetUniform(DecalShader->DecalTextureScale, TexScale);

			mat4 DecalProjection = OrthogonalProjectionMat4(0.5f*Decal->Box.Dim.x, 0.5f*Decal->Box.Dim.y,
				-0.5f*Decal->Box.Dim.z, 0.5f*Decal->Box.Dim.z);
			mat4 DecalView = RotationMat4(Conjugate(Decal->Box.Rotation))*TranslationMat4(-Decal->Box.P);
			SetUniform(DecalShader->DecalData.Projection, DecalProjection);
			SetUniform(DecalShader->DecalData.View, DecalView);

			DrawCube(State);
		}

		BindTexture(0, 0);
		BindTexture(1, 0);
		EndShader(State);

		glDisable(GL_BLEND);
	}

	{
		// NOTE: Calculate Fresnel
		OpenGLBindFramebuffer(State, FresnelFramebuffer);
		shader_fresnel *Fresnel = (shader_fresnel *) BeginShader(State, Shader_Fresnel);
		glClearBufferfv(GL_COLOR, 0, V3(0,0,0).E);

		BindTexture(0, GBuffer->Textures[FBT_Albedo]);
		BindTexture(1, GBuffer->Textures[FBT_Depth]);
		BindTexture(2, GBuffer->Textures[FBT_Normal]);
		BindTexture(3, GBuffer->Textures[FBT_Roughness_Metalness_Emission]);

		SetUniform(Fresnel->ViewRay, SceneSetup->ViewRay);

		OpenGLRenderQuad(State, V2(0,0), V2(1,1));

		BindTexture(0, 0);
		BindTexture(1, 0);
		BindTexture(2, 0);
		BindTexture(3, 0);
		EndShader(State);
	}

	if(State->RenderCommands.Fullbright)
	{
		OpenGLBindFramebuffer(State, FogBuffer);
		glClearBufferfv(GL_COLOR, 0, V4(0,0,0,0).E);

		OpenGLBindFramebuffer(State, SceneBuffer);
		shader_light_pass *LightPass = (shader_light_pass *) BeginShader(State, Shader_LightPass);
		glClearBufferfv(GL_COLOR, 0, V3(0,0,0).E);

		glBlendFunc(GL_ONE, GL_ONE);
		glEnable(GL_BLEND);
		glCullFace(GL_FRONT);
		glEnable(GL_CULL_FACE);

		BindTexture(0, GBuffer->Textures[FBT_Albedo]);
		BindTexture(1, GBuffer->Textures[FBT_Depth]);
		BindTexture(2, GBuffer->Textures[FBT_Normal]);
		BindTexture(3, GBuffer->Textures[FBT_Roughness_Metalness_Emission]);
		BindTexture(4, FresnelFramebuffer->Textures[FBT_Color]);

		SetUniform(LightPass->Projection, SceneSetup->Projection);
		SetUniform(LightPass->View, SceneSetup->View);
		SetUniform(LightPass->InvView, SceneSetup->InvView);
		SetUniform(LightPass->ViewRay, SceneSetup->ViewRay);

		SetUniform(LightPass->LightData.IsDirectional, true);
		SetUniform(LightPass->LightData.Color, V3(1,1,1));
		SetUniform(LightPass->LightData.Direction, V3(0,0,0));
		SetUniform(LightPass->LightData.FuzzFactor, 1.0f);

		SetUniform(LightPass->Model, TranslationMat4(SceneSetup->P));
		SetUniform(LightPass->CastsShadow, false);

		DrawCube(State);

		BindTexture(0, 0);
		BindTexture(1, 0);
		BindTexture(2, 0);
		BindTexture(3, 0);
		BindTexture(4, 0);
		BindTexture(5, 0);
		EndShader(State);

		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);
	}
	else
	{
		// NOTE: Shadow pass.
		directional_light *ShadowCaster = 0;
		for(u32 LightIndex = 0;
			LightIndex < State->RenderCommands.LightCount;
			++LightIndex)
		{
			light *Light = State->RenderCommands.Lights + LightIndex;
			if(Light->Type == LightType_Directional)
			{
				ShadowCaster = &Light->Directional;
			}
		}

		light_space_cascades Cascades = {};
		v2 SMRegionScales[CASCADE_COUNT] = {};
		v2 SMRegionOffsets[CASCADE_COUNT] = {};
		if(ShadowCaster)
		{
			SMRegionScales[0] = V2(0.5f, 0.5f);
			SMRegionOffsets[0] = V2(0, 0);
			SMRegionScales[1] = V2(0.5f, 0.5f);
			SMRegionOffsets[1] = V2(0.5f, 0);
			SMRegionScales[2] = V2(0.5f, 0.5f);
			SMRegionOffsets[2] = V2(0, 0.5f);
			SMRegionScales[3] = V2(0.5f, 0.5f);
			SMRegionOffsets[3] = V2(0.5f, 0.5f);

			v2i FullRes = ShadowMap->Resolution;
			v2i HalfRes = FullRes/2;
			ShadowMap->SubRegionCount = 0;
			for(u32 CascadeIndex = 0;
				CascadeIndex < CASCADE_COUNT;
				++CascadeIndex)
			{
				rectangle2i *SubRegions = ShadowMap->SubRegions + ShadowMap->SubRegionCount++;
				SubRegions->Min = Hadamard(SMRegionOffsets[CascadeIndex], FullRes);
				SubRegions->Max = SubRegions->Min + Hadamard(SMRegionScales[CascadeIndex], FullRes);
			}

			Cascades = CalculateLightSpaceMatrices(SceneSetup, ShadowCaster);

			// NOTE: Execute all render commands.
			glCullFace(GL_BACK);
			glEnable(GL_SCISSOR_TEST);
			glDepthFunc(GL_LEQUAL);
			glEnable(GL_DEPTH_TEST);

			for(u32 CascadeIndex = 0;
				CascadeIndex < ShadowMap->SubRegionCount;
				++CascadeIndex)
			{
				OpenGLBindFramebuffer(State, ShadowMap, ShadowMap->SubRegions[CascadeIndex]);
				OpenGLClipSetup(ShadowMap->SubRegions[CascadeIndex]);
				opengl_shader_common *Shader = BeginShader(State, Shader_ShadowMap);
				OpenGLExecuteRenderCommands(State, Shader, &State->RenderCommands, Target_Scene,
					&Cascades, CascadeIndex);
				EndShader(State);
			}

			glDisable(GL_CULL_FACE);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
			glDisable(GL_SCISSOR_TEST);
		}

		{
			// NOTE: Skybox
			OpenGLBindFramebuffer(State, SceneBuffer);
			shader_skybox *SkyboxShader = (shader_skybox *) BeginShader(State, Shader_Skybox);
			glClearBufferfv(GL_COLOR, 0, V3(0,0,0).E);

			SetUniform(SkyboxShader->SkyRadiance, State->RenderCommands.SkyRadiance);

			OpenGLRenderQuad(State, V2(0,0), V2(1,1));

			EndShader(State);
		}

		lighting_solution *Solution = State->RenderCommands.LightingSolution;
		if(Solution)
		{
			// NOTE: Load GI buffers
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, State->SurfelBuffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, Solution->SurfelCount*sizeof(surface_element), Solution->Surfels
				, GL_DYNAMIC_COPY);

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, State->SurfelRefBuffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, Solution->SurfelRefCount*sizeof(u32), Solution->SurfelRefs, GL_DYNAMIC_COPY);

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

			// NOTE: Light surfels
			shader_surfels *Surfels = (shader_surfels *) BeginShader(State, Shader_Surfels);

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, State->SurfelBuffer);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, State->ProbeBuffer);
			BindTexture(0, ShadowMap->Textures[FBT_Color]);

			SetUniform(Surfels->View, SceneSetup->View);
			SetUniform(Surfels->SkyRadiance, State->RenderCommands.SkyRadiance);

			SetUniform(Surfels->Initialize, true);
			glDispatchCompute(Solution->SurfelCount, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			SetUniform(Surfels->Initialize, false);

			// TODO: The surfels need to reference a probe from the previous frame, so we can't upload new
			//	probes until now. This will make sending probes and surfels in small batches more difficult.
			//	We will need a different way for surfels to store their previous indirect light; maybe do it
			// 	at the end of the frame instead of the beginning of the next?
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, State->ProbeBuffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, Solution->ProbeCount*sizeof(light_probe), Solution->Probes, GL_DYNAMIC_COPY);

			for(u32 LightIndex = 0;
				LightIndex < State->RenderCommands.LightCount;
				++LightIndex)
			{
				light *Light = State->RenderCommands.Lights + LightIndex;
				switch(Light->Type)
				{
					case LightType_Point:
					{
						point_light *PointLight = &Light->Point;

						SetUniform(Surfels->LightData.IsDirectional, false);
						SetUniform(Surfels->LightData.Color, PointLight->Color);
						SetUniform(Surfels->LightData.Intensity, PointLight->Intensity);
						SetUniform(Surfels->LightData.P, PointLight->P);
						SetUniform(Surfels->LightData.Size, PointLight->Size);
						SetUniform(Surfels->LightData.LinearAttenuation, PointLight->LinearAttenuation);
						SetUniform(Surfels->LightData.QuadraticAttenuation, PointLight->QuadraticAttenuation);
						SetUniform(Surfels->LightData.Radius, PointLight->Radius);
					} break;

					case LightType_Directional:
					{
						directional_light *DirectLight = &Light->Directional;
						SetUniform(Surfels->LightData.IsDirectional, true);
						SetUniform(Surfels->LightData.Color, DirectLight->Color);
						SetUniform(Surfels->LightData.Intensity, DirectLight->Intensity);
						SetUniform(Surfels->LightData.Direction, DirectLight->Direction);

						SetUniformArray(Surfels->LightProjection, Cascades.Projection, CASCADE_COUNT);
						SetUniformArray(Surfels->LightView, Cascades.View, CASCADE_COUNT);
						SetUniformArray(Surfels->LightFarPlane, Cascades.FarPlane, CASCADE_COUNT);
						SetUniformArray(Surfels->CascadeBounds, Cascades.CascadeBounds, CASCADE_COUNT);
						SetUniformArray(Surfels->SMRegionScale, SMRegionScales, CASCADE_COUNT);
						SetUniformArray(Surfels->SMRegionOffset, SMRegionOffsets, CASCADE_COUNT);
					} break;

					case LightType_Ambient:
					{

					} break;

					InvalidDefaultCase;
				}

				if(Light->Type != LightType_Ambient)
				{
					glDispatchCompute(Solution->SurfelCount, 1, 1);
					glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
				}
			}

			BindTexture(0, 0);
			EndShader(State);

			// NOTE: Light probes
			shader_probes *ProbeShader = (shader_probes *) BeginShader(State, Shader_Probes);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, State->SurfelBuffer);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, State->SurfelRefBuffer);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, State->ProbeBuffer);

			glDispatchCompute(Solution->ProbeCount, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			EndShader(State);

			// NOTE: Build irradiance volume
			shader_irradiance_volume *Irradiance = (shader_irradiance_volume *) BeginShader(State, Shader_Irradiance_Volume);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, State->ProbeBuffer);
			glBindImageTexture(1, State->IrradianceVolume, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

			SetUniform(Irradiance->ProbeCount, (s32)Solution->ProbeCount);
			SetUniform(Irradiance->Center, SceneSetup->P);

			glDispatchCompute(IRRADIANCE_VOLUME_WIDTH, IRRADIANCE_VOLUME_HEIGHT, IRRADIANCE_VOLUME_WIDTH);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			EndShader(State);

			if(State->DebugReadIrradianceResults)
			{
				void *Buffer = 0;

				glBindBuffer(GL_SHADER_STORAGE_BUFFER, State->SurfelBuffer);
				Buffer = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
				CopySize(Buffer, Solution->Surfels, Solution->SurfelCount*sizeof(surface_element));
				Assert(glUnmapBuffer(GL_SHADER_STORAGE_BUFFER));

				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

				glBindBuffer(GL_SHADER_STORAGE_BUFFER, State->ProbeBuffer);
				Buffer = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
				CopySize(Buffer, Solution->Probes, Solution->ProbeCount*sizeof(light_probe));
				Assert(glUnmapBuffer(GL_SHADER_STORAGE_BUFFER));

				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

				glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
			}

			{
				// NOTE: Indirect light pass
	#if 0
				{
					// NOTE: Software irradiance pass
					LightSurfels(State, Solution, SceneSetup->P, State->RenderCommands.Lights, State->RenderCommands.LightCount);

					temporary_memory TempMem = BeginTemporaryMemory(&State->Region);
					irradiance_volume IrradianceVolume = BuildIrradianceVolume(&State->Region,
						Solution->LightProbes, Solution->LightProbeCount,
						SceneSetup->P);

					glBindTexture(GL_TEXTURE_3D, State->IrradianceVolume);
				    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, 6*IRRADIANCE_VOLUME_WIDTH, IRRADIANCE_VOLUME_HEIGHT, IRRADIANCE_VOLUME_WIDTH, 0, GL_RGB, GL_FLOAT, IrradianceVolume.Voxels);
				    glBindTexture(GL_TEXTURE_3D, 0);

					EndTemporaryMemory(&TempMem);
				}
	#endif
				OpenGLBindFramebuffer(State, SceneBuffer);
				shader_indirect_light_pass *IndirectLightPass = (shader_indirect_light_pass *) BeginShader(State, Shader_IndirectLightPass);

				BindTexture(0, GBuffer->Textures[FBT_Albedo]);
				BindTexture(1, GBuffer->Textures[FBT_Depth]);
				BindTexture(2, GBuffer->Textures[FBT_Normal]);
				BindTexture(3, GBuffer->Textures[FBT_Roughness_Metalness_Emission]);
				BindTexture(4, FresnelFramebuffer->Textures[FBT_Color]);
				BindTexture(5, State->IrradianceVolume, GL_TEXTURE_3D);

				SetUniform(IndirectLightPass->ViewRay, SceneSetup->ViewRay);
				SetUniform(IndirectLightPass->InvView, SceneSetup->InvView);
				SetUniform(IndirectLightPass->SkyRadiance, State->RenderCommands.SkyRadiance);

				OpenGLRenderQuad(State, V2(0,0), V2(1,1));

				BindTexture(0, 0);
				BindTexture(1, 0);
				BindTexture(2, 0);
				BindTexture(3, 0);
				BindTexture(4, 0);
				BindTexture(5, 0, GL_TEXTURE_3D);
				EndShader(State);
			}
		}

		{
			// NOTE: Light pass.
			opengl_framebuffer *ShadowMapBlurred = OpenGLBlurShadowMap(State, ShadowMap);

			OpenGLBindFramebuffer(State, SceneBuffer);
			shader_light_pass *LightPass = (shader_light_pass *) BeginShader(State, Shader_LightPass);

			glBlendFunc(GL_ONE, GL_ONE);
			glEnable(GL_BLEND);
			glCullFace(GL_FRONT);
			glEnable(GL_CULL_FACE);

			BindTexture(0, GBuffer->Textures[FBT_Albedo]);
			BindTexture(1, GBuffer->Textures[FBT_Depth]);
			BindTexture(2, GBuffer->Textures[FBT_Normal]);
			BindTexture(3, GBuffer->Textures[FBT_Roughness_Metalness_Emission]);
			BindTexture(4, FresnelFramebuffer->Textures[FBT_Color]);
			BindTexture(5, ShadowMapBlurred->Textures[FBT_Color]);
			BindTexture(5, ShadowMap->Textures[FBT_Color]);
			BindTexture(6, State->BlueNoiseTexture);

			SetUniform(LightPass->Projection, SceneSetup->Projection);
			SetUniform(LightPass->View, SceneSetup->View);
			SetUniform(LightPass->InvView, SceneSetup->InvView);
			SetUniform(LightPass->ViewRay, SceneSetup->ViewRay);

			if(!State->DebugTurnOffDirectLighting)
			{
				for(u32 LightIndex = 0;
					LightIndex < State->RenderCommands.LightCount;
					++LightIndex)
				{
					light *Light = State->RenderCommands.Lights + LightIndex;
					switch(Light->Type)
					{
						case LightType_Point:
						{
							point_light *PointLight = &Light->Point;
							v3 LightP = (SceneSetup->View * V4(PointLight->P, 1.0f)).xyz;

							SetUniform(LightPass->LightData.IsDirectional, false);
							SetUniform(LightPass->LightData.Color, PointLight->Color);
							SetUniform(LightPass->LightData.Intensity, PointLight->Intensity);
							SetUniform(LightPass->LightData.P, LightP);
							SetUniform(LightPass->LightData.Size, PointLight->Size);
							SetUniform(LightPass->LightData.LinearAttenuation, PointLight->LinearAttenuation);
							SetUniform(LightPass->LightData.QuadraticAttenuation, PointLight->QuadraticAttenuation);
							SetUniform(LightPass->LightData.Radius, PointLight->Radius);

							SetUniform(LightPass->Model, TranslationMat4(PointLight->P)*ScaleMat4(PointLight->Radius));
							SetUniform(LightPass->CastsShadow, false);
						} break;

						case LightType_Directional:
						{
							directional_light *DirectionalLight = &Light->Directional;
							v3 LightDirection = (SceneSetup->View * V4(DirectionalLight->Direction, 0.0f)).xyz;
							f32 FuzzFactor = Tan(0.5f*DirectionalLight->ArcDegrees*Pi32/180.0f);

							SetUniform(LightPass->LightData.IsDirectional, true);
							SetUniform(LightPass->LightData.Color, DirectionalLight->Color);
							SetUniform(LightPass->LightData.Intensity, DirectionalLight->Intensity);
							SetUniform(LightPass->LightData.Direction, LightDirection);
							SetUniform(LightPass->LightData.FuzzFactor, FuzzFactor);

							SetUniform(LightPass->Model, TranslationMat4(SceneSetup->P));

							SetUniform(LightPass->CastsShadow, true);

							SetUniformArray(LightPass->LightProjection, Cascades.Projection, CASCADE_COUNT);
							SetUniformArray(LightPass->LightView, Cascades.View, CASCADE_COUNT);
							SetUniformArray(LightPass->LightFarPlane, Cascades.FarPlane, CASCADE_COUNT);
							SetUniformArray(LightPass->CascadeBounds, Cascades.CascadeBounds, CASCADE_COUNT);
							SetUniformArray(LightPass->SMRegionScale, SMRegionScales, CASCADE_COUNT);
							SetUniformArray(LightPass->SMRegionOffset, SMRegionOffsets, CASCADE_COUNT);
						} break;

						case LightType_Ambient:
						{
							ambient_light *AmbientLight = &Light->Ambient;

							SetUniform(LightPass->LightData.IsDirectional, true);
							SetUniform(LightPass->LightData.Color, AmbientLight->Color);
							SetUniform(LightPass->LightData.Direction, V3(0,0,0));
							SetUniform(LightPass->LightData.FuzzFactor, AmbientLight->Coeff);

							SetUniform(LightPass->Model, TranslationMat4(SceneSetup->P));
							SetUniform(LightPass->CastsShadow, false);
						} break;

						InvalidDefaultCase;
					}

					DrawCube(State);
				}
			}

			BindTexture(0, 0);
			BindTexture(1, 0);
			BindTexture(2, 0);
			BindTexture(3, 0);
			BindTexture(4, 0);
			BindTexture(5, 0);
			BindTexture(6, 0);
			EndShader(State);

			glDisable(GL_BLEND);
			glDisable(GL_CULL_FACE);
		}

		// NOTE: Fog pass
		OpenGLBindFramebuffer(State, FogBuffer);
		shader_fog *FogShader = (shader_fog *) BeginShader(State, Shader_Fog);
		glClearBufferfv(GL_COLOR, 0, V4(0,0,0,0).E);

		SetUniform(FogShader->ViewRay, SceneSetup->ViewRay);
		SetUniform(FogShader->View, SceneSetup->View);
		SetUniform(FogShader->InvView, SceneSetup->InvView);
		SetUniform(FogShader->SkyRadiance, State->RenderCommands.SkyRadiance);

		if(ShadowCaster)
		{
			v3 LightDirection = (SceneSetup->View * V4(ShadowCaster->Direction, 0.0f)).xyz;
			f32 FuzzFactor = Tan(0.5f*ShadowCaster->ArcDegrees*Pi32/180.0f);

			SetUniform(FogShader->LightData.IsDirectional, true);
			SetUniform(FogShader->LightData.Color, ShadowCaster->Color);
			SetUniform(FogShader->LightData.Intensity, ShadowCaster->Intensity);
			SetUniform(FogShader->LightData.Direction, LightDirection);
			SetUniform(FogShader->LightData.FuzzFactor, FuzzFactor);
		}

		SetUniformArray(FogShader->LightProjection, Cascades.Projection, CASCADE_COUNT);
		SetUniformArray(FogShader->LightView, Cascades.View, CASCADE_COUNT);
		SetUniformArray(FogShader->LightFarPlane, Cascades.FarPlane, CASCADE_COUNT);
		SetUniformArray(FogShader->CascadeBounds, Cascades.CascadeBounds, CASCADE_COUNT);
		SetUniformArray(FogShader->SMRegionScale, SMRegionScales, CASCADE_COUNT);
		SetUniformArray(FogShader->SMRegionOffset, SMRegionOffsets, CASCADE_COUNT);

		BindTexture(0, SceneBuffer->Textures[FBT_Color]);
		BindTexture(1, GBuffer->Textures[FBT_Depth]);
		BindTexture(2, ShadowMap->Textures[FBT_Color]);
		BindTexture(3, State->IrradianceVolume, GL_TEXTURE_3D);
		BindTexture(4, State->BlueNoiseTexture);

		OpenGLRenderQuad(State, V2(0,0), V2(1,1));

		BindTexture(4, 0);
		BindTexture(3, 0, GL_TEXTURE_3D);
		BindTexture(2, 0);
		BindTexture(1, 0);
		BindTexture(0, 0);

		EndShader(State);

		opengl_framebuffer *FogBlurred = OpenGLBlur(State, FogBuffer, 2);
		// opengl_framebuffer *FogBlurred = FogBuffer;

		OpenGLBindFramebuffer(State, SceneBuffer);
		shader_fog_resolve *FogResolve = (shader_fog_resolve *) BeginShader(State, Shader_FogResolve);
		BindTexture(0, FogBlurred->Textures[FBT_Color]);
		BindTexture(1, GBuffer->Textures[FBT_Depth]);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);

		OpenGLRenderQuad(State, V2(0,0), V2(1,1));

		BindTexture(1, 0);
		BindTexture(0, 0);
		glDisable(GL_BLEND);
		EndShader(State);
	}

	opengl_framebuffer *BloomFramebuffer = 0;
	{
		// NOTE: Overbright/bloom.
		OpenGLBindFramebuffer(State, OverbrightFramebuffer);
		shader_overbright *Overbright = (shader_overbright *) BeginShader(State, Shader_Overbright);
		BindTexture(0, SceneBuffer->Textures[FBT_Color]);
		OpenGLRenderQuad(State, V2(0,0), V2(1,1));
		BindTexture(0, 0);

		if(State->CurrentRenderSettings.Bloom)
		{
			BloomFramebuffer = OpenGLBlur(State, OverbrightFramebuffer);
		}
	}

	{
		// NOTE: Composite to screen.
		OpenGLBindFramebuffer(State, 0);
		shader_finalize *Finalize = (shader_finalize *) BeginShader(State, Shader_Finalize);
		glClearBufferfv(GL_COLOR, 0, V4(0,0,0,0).E);

		BindTexture(0, SceneBuffer->Textures[FBT_Color]);
		BindTexture(1, OverlayFramebuffer->Textures[FBT_Color]);
		BindTexture(2, BloomFramebuffer ? BloomFramebuffer->Textures[FBT_Color] : 0);

		OpenGLRenderQuad(State, V2(0,0), V2(1,1));

		BindTexture(0, 0);
		BindTexture(1, 0);
		BindTexture(2, 0);
		EndShader(State);
	}

	if(State->DebugDisplayFramebuffers)
	{
		DebugFramebufferDisplay(State);
	}
}
