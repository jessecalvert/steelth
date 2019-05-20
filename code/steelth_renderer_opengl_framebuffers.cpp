/*@H
* File: steelth_renderer_opengl_framebuffers.cpp
* Author: Jesse Calvert
* Created: April 23, 2019, 16:26
* Last modified: April 26, 2019, 23:50
*/

internal texture_settings
TextureSettings(u32 Width, u32 Height, u32 Channels, u32 BytesPerChannel, flag32(texture_flags) Flags, u32 Samples = 0, v4 BorderColor = V4(1,1,1,1))
{
	texture_settings Settings = {};
	Settings.Width = Width;
	Settings.Height = Height;
	Settings.BorderColor = BorderColor;
	Settings.Channels = Channels;
	Settings.BytesPerChannel = BytesPerChannel;
	Settings.Samples = Samples;
	Settings.Flags = Flags;
	return Settings;
}

internal GLuint
OpenGLAllocateAndLoadTexture(texture_settings Settings, void *Pixels = 0)
{
	Assert(!((Settings.Flags & Texture_ClampToBorder) &&
			 (Settings.Flags & Texture_ClampToEdge)));
	Assert(!((Settings.Flags & Texture_FilterNearest) &&
			 (Settings.Flags & Texture_Mipmap)));

	GLuint Result;
	GLenum TextureTarget = (Settings.Flags & Texture_Multisample) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

	glGenTextures(1, &Result);
	glBindTexture(TextureTarget, Result);

	GLenum WrapParam = GL_REPEAT;
	if(Settings.Flags & Texture_ClampToBorder)
	{
		WrapParam = GL_CLAMP_TO_BORDER;
		glTexParameterfv(TextureTarget, GL_TEXTURE_BORDER_COLOR, Settings.BorderColor.E);
	}
	else if(Settings.Flags & Texture_ClampToEdge)
	{
		WrapParam = GL_CLAMP_TO_EDGE;
	}

	GLenum MinFilter = GL_LINEAR;
	GLenum MagFilter = GL_LINEAR;
	if(Settings.Flags & Texture_FilterNearest)
	{
		MinFilter = GL_NEAREST;
		MagFilter = GL_NEAREST;
	}
	else if(Settings.Flags & Texture_Mipmap)
	{
		MinFilter = GL_LINEAR_MIPMAP_LINEAR;
	}

	if(!(Settings.Flags & Texture_Multisample))
	{
	    glTexParameteri(TextureTarget, GL_TEXTURE_WRAP_S, WrapParam);
	    glTexParameteri(TextureTarget, GL_TEXTURE_WRAP_T, WrapParam);
	    glTexParameteri(TextureTarget, GL_TEXTURE_MIN_FILTER, MinFilter);
	    glTexParameteri(TextureTarget, GL_TEXTURE_MAG_FILTER, MagFilter);
	}

	GLint InternalFormat = 0;
	GLenum PixelFormat = 0;
	GLenum PixelType = 0;

    if(Settings.Flags & Texture_DepthBuffer)
    {
    	Assert(Settings.Flags & Texture_Float);
    	Assert(Settings.Channels == 1);
    	Assert(Settings.BytesPerChannel == 4);
    	InternalFormat = GL_DEPTH_COMPONENT32F;
    	PixelFormat = GL_DEPTH_COMPONENT;
    	PixelType = GL_FLOAT;
    }
    else
    {
    	b32 Float = Settings.Flags & Texture_Float;

    	switch(Settings.Channels)
    	{
    		case 1:
    		{
    			if(Float)
    			{
    				PixelType = GL_FLOAT;
    				switch(Settings.BytesPerChannel)
    				{
    					case 2:
    					{
							InternalFormat = GL_R16F;
    					} break;

    					case 4:
    					{
							InternalFormat = GL_R32F;
    					} break;

    					InvalidDefaultCase;
    				}
    			}
    			else
    			{
					InternalFormat = GL_RED;
					switch(Settings.BytesPerChannel)
    				{
    					case 1:
    					{
    						PixelType = GL_UNSIGNED_BYTE;
    					} break;

    					case 4:
    					{
    						PixelType = GL_UNSIGNED_INT;
    					} break;

    					InvalidDefaultCase;
    				}
    			}

    			PixelFormat = GL_RED;
    		} break;

    		case 2:
    		{
    			if(Float)
    			{
    				PixelType = GL_FLOAT;
    				switch(Settings.BytesPerChannel)
    				{
    					case 2:
    					{
							InternalFormat = GL_RG16F;
    					} break;

    					case 4:
    					{
							InternalFormat = GL_RG32F;
    					} break;

    					InvalidDefaultCase;
    				}
    			}
    			else
    			{
					InternalFormat = GL_RG;
					switch(Settings.BytesPerChannel)
    				{
    					case 1:
    					{
    						PixelType = GL_UNSIGNED_SHORT;
    					} break;

    					InvalidDefaultCase;
    				}
    			}

    			PixelFormat = GL_RG;
    		} break;

    		case 3:
    		{
    			if(Float)
    			{
    				PixelType = GL_FLOAT;
    				switch(Settings.BytesPerChannel)
    				{
    					case 2:
    					{
							InternalFormat = GL_RGB16F;
    					} break;

    					case 4:
    					{
							InternalFormat = GL_RGB32F;
    					} break;

    					InvalidDefaultCase;
    				}
    			}
    			else
    			{
					InternalFormat = GL_RGB;
					switch(Settings.BytesPerChannel)
    				{
    					case 1:
    					{
    						PixelType = GL_UNSIGNED_INT;
    					} break;

    					InvalidDefaultCase;
    				}
    			}

    			PixelFormat = GL_RGB;
    		} break;

    		case 4:
    		{
				if(Float)
    			{
    				PixelType = GL_FLOAT;
    				switch(Settings.BytesPerChannel)
    				{
    					case 2:
    					{
							InternalFormat = GL_RGBA16F;
    					} break;

    					case 4:
    					{
							InternalFormat = GL_RGBA32F;
    					} break;

    					InvalidDefaultCase;
    				}
    			}
    			else
    			{
					InternalFormat = GL_RGBA;
					switch(Settings.BytesPerChannel)
    				{
    					case 1:
    					{
    						PixelType = GL_UNSIGNED_INT_8_8_8_8;
    					} break;

    					InvalidDefaultCase;
    				}
    			}

    			PixelFormat = GL_RGBA;
    		} break;

    		InvalidDefaultCase;
    	}
    }

    if(Settings.Flags & Texture_Multisample)
    {
		glTexImage2DMultisample(TextureTarget, Settings.Samples, InternalFormat, Settings.Width, Settings.Height, GL_TRUE);
    }
    else
    {
		glTexImage2D(TextureTarget, 0, InternalFormat, Settings.Width, Settings.Height, 0, PixelFormat, PixelType, Pixels);
    }

	if(Settings.Flags & Texture_Mipmap)
	{
		glGenerateMipmap(TextureTarget);
	}

	glBindTexture(TextureTarget, 0);

	return Result;
}

internal void
OpenGLLoadTexture(opengl_state *State, void *Pixels, renderer_texture Texture)
{
#if 0
	texture_settings Settings = TextureSettings(Width, Height, 4, 1, 0);

	u64 TextureHandle = OpenGLAllocateAndLoadTexture(Settings, Pixels);
	u32 TextureHandle32 = (u32) TextureHandle;
	Assert(TextureHandle32 == TextureHandle);

	renderer_texture Result = {};
	Result.Index = TextureHandle32;
	Result.Width = Width;
	Result.Height = Height;
	return Result;

#else
	Assert(Texture.Width <= MAX_TEXTURE_DIM);
	Assert(Texture.Height <= MAX_TEXTURE_DIM);
	Assert(Texture.Index < State->MaxTextureCount);

	glBindTexture(GL_TEXTURE_2D_ARRAY, State->TextureArray);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, Texture.Index,
		Texture.Width, Texture.Height, 1,
		GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, Pixels);

#endif
}

internal void
OpenGLCreateFramebuffer(opengl_framebuffer *Framebuffer, texture_settings *Settings, u32 TextureCount, v2i Resolution)
{
	TIMED_FUNCTION();

	Assert(TextureCount <= ArrayCount(Framebuffer->Textures));

	glGenFramebuffers(1, &Framebuffer->Handle);
	glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer->Handle);

	Framebuffer->Resolution = Resolution;
	Framebuffer->TextureCount = TextureCount;

	u32 ColorAttachments = 0;
	GLuint Attachments[MAX_FRAMEBUFFER_TEXTURES] = {};

	b32 DepthAttachment = false;
	for(u32 TextureIndex = 0;
		TextureIndex < TextureCount;
		++TextureIndex)
	{
		texture_settings *TextureSettings = Settings + TextureIndex;
		TextureSettings->Width = Resolution.x;
		TextureSettings->Height = Resolution.y;
		Framebuffer->Textures[TextureIndex] = OpenGLAllocateAndLoadTexture(*TextureSettings, 0);
		GLenum TextureTarget = (TextureSettings->Flags & Texture_Multisample) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

		if(TextureSettings->Flags & Texture_DepthBuffer)
		{
			Assert(!DepthAttachment);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
				TextureTarget, Framebuffer->Textures[TextureIndex], 0);
			DepthAttachment = true;
		}
		else
		{
			Attachments[ColorAttachments] = GL_COLOR_ATTACHMENT0 + ColorAttachments;
			glFramebufferTexture2D(GL_FRAMEBUFFER, Attachments[ColorAttachments],
				TextureTarget, Framebuffer->Textures[TextureIndex], 0);
			++ColorAttachments;
		}
	}

	glDrawBuffers(ColorAttachments, Attachments);

	Assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

internal void
OpenGLCreateAllFramebuffers(opengl_state *State)
{
	v2i Resolution = State->CurrentRenderSettings.Resolution;
	v2i DownScaleRes = V2ToV2i(State->CurrentRenderSettings.RenderScale*Resolution);
	v2i ShadowMapRes = State->CurrentRenderSettings.ShadowMapResolution;

	{
		opengl_framebuffer *SceneBuffer = State->Framebuffers + Framebuffer_Scene;
		SceneBuffer->Type = Framebuffer_Scene;
		texture_settings Settings[] =
		{
			TextureSettings(0, 0, 3, 2, Texture_Float|Texture_ClampToEdge), // Color
		};

		u32 TextureCount = ArrayCount(Settings);
		OpenGLCreateFramebuffer(SceneBuffer, Settings, TextureCount, DownScaleRes);
	}

	{
		opengl_framebuffer *GBuffer = State->Framebuffers + Framebuffer_GBuffer;
		GBuffer->Type = Framebuffer_GBuffer;
		texture_settings Settings[] =
		{
			TextureSettings(0, 0, 4, 1, Texture_ClampToEdge), // Albedo
			TextureSettings(0, 0, 1, 4, Texture_Float|Texture_DepthBuffer|Texture_ClampToBorder, 0, V4(1,1,1,1)), // Depth
			TextureSettings(0, 0, 3, 2, Texture_Float|Texture_ClampToEdge), // Normal
			TextureSettings(0, 0, 3, 2, Texture_Float|Texture_ClampToEdge), // Roughness_Metalness_Emission
		};

		u32 TextureCount = ArrayCount(Settings);
		OpenGLCreateFramebuffer(GBuffer, Settings, TextureCount, DownScaleRes);
	}

	for(u32 Index = 0;
		Index < 2;
		++Index)
	{
		opengl_framebuffer *ShadowMap = State->Framebuffers + Framebuffer_ShadowMap0 + Index;
		ShadowMap->Type = (framebuffer_type)(Framebuffer_ShadowMap0 + Index);
		texture_settings Settings[] =
		{
			TextureSettings(0, 0, 2, 4, Texture_Float|Texture_ClampToBorder, 0, V4(1,1,1,1)), // Depth
			TextureSettings(0, 0, 1, 4, Texture_Float|Texture_DepthBuffer|Texture_ClampToBorder, 0, V4(1,1,1,1)), // Depth
		};

		u32 TextureCount = ArrayCount(Settings);
		OpenGLCreateFramebuffer(ShadowMap, Settings, TextureCount, ShadowMapRes);
	}

	{
		opengl_framebuffer *Fresnel = State->Framebuffers + Framebuffer_Fresnel;
		Fresnel->Type = Framebuffer_Fresnel;
		texture_settings Settings[] =
		{
			TextureSettings(0, 0, 3, 2, Texture_Float|Texture_ClampToEdge), // Color
		};

		u32 TextureCount = ArrayCount(Settings);
		OpenGLCreateFramebuffer(Fresnel, Settings, TextureCount, DownScaleRes);
	}

	// if(State->CurrentRenderSettings.Bloom)
	{
		{
			opengl_framebuffer *OverbrightFramebuffer = State->Framebuffers + Framebuffer_Overbright;
			OverbrightFramebuffer->Type = Framebuffer_Overbright;
			texture_settings Settings[] =
			{
				TextureSettings(0, 0, 3, 2, Texture_Float|Texture_ClampToEdge), // Color
			};

			u32 TextureCount = ArrayCount(Settings);
			OpenGLCreateFramebuffer(OverbrightFramebuffer, Settings, TextureCount, DownScaleRes/2);
		}
	}

	for(u32 Index = 0;
		Index < 2;
		++Index)
	{
		opengl_framebuffer *BlurFramebuffer = State->Framebuffers + Framebuffer_Blur0 + Index;
		BlurFramebuffer->Type = (framebuffer_type)(Framebuffer_Blur0 + Index);
		texture_settings Settings[] =
		{
			TextureSettings(0, 0, 4, 2, Texture_Float|Texture_ClampToEdge), // Color
		};

		u32 TextureCount = ArrayCount(Settings);
		OpenGLCreateFramebuffer(BlurFramebuffer, Settings, TextureCount, DownScaleRes/2);
	}

	{
		opengl_framebuffer *FogBuffer = State->Framebuffers + Framebuffer_Fog;
		FogBuffer->Type = Framebuffer_Fog;
		texture_settings Settings[] =
		{
			TextureSettings(0, 0, 3, 2, Texture_Float|Texture_FilterNearest|Texture_ClampToEdge), // Color
		};

		u32 TextureCount = ArrayCount(Settings);
		OpenGLCreateFramebuffer(FogBuffer, Settings, TextureCount, DownScaleRes/2);
	}

	{
		opengl_framebuffer *OverlayFramebuffer = State->Framebuffers + Framebuffer_Overlay;
		OverlayFramebuffer->Type = Framebuffer_Overlay;
		texture_settings Settings[] =
		{
			TextureSettings(0, 0, 4, 1, Texture_ClampToEdge), // Color
		};

		u32 TextureCount = ArrayCount(Settings);
		OpenGLCreateFramebuffer(OverlayFramebuffer, Settings, TextureCount, Resolution);
	}
}

internal void
OpenGLFreeFramebuffer(opengl_framebuffer *Framebuffer)
{
	glDeleteTextures(Framebuffer->TextureCount, Framebuffer->Textures);
	glDeleteFramebuffers(1, &Framebuffer->Handle);
	Framebuffer->Handle = 0;
	for(u32 Index = 0;
		Index < Framebuffer->TextureCount;
		++Index)
	{
		Framebuffer->Textures[Index] = 0;
	}

	Framebuffer->TextureCount = 0;
}

internal void
OpenGLBindFramebuffer(opengl_state *State, opengl_framebuffer *Framebuffer, rectangle2i Viewport)
{
	TIMED_FUNCTION();

	GLuint Handle = 0;
	if(Framebuffer)
	{
		Handle = Framebuffer->Handle;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, Handle);

	u32 Width = Viewport.Max.x - Viewport.Min.x;
	u32 Height = Viewport.Max.y - Viewport.Min.y;
	glViewport(Viewport.Min.x, Viewport.Min.y, Width, Height);
	glScissor(Viewport.Min.x, Viewport.Min.y, Width, Height);

	State->CurrentFramebuffer = Framebuffer ? Framebuffer->Type : Framebuffer_Default;
}

internal void
OpenGLBindFramebuffer(opengl_state *State, opengl_framebuffer *Framebuffer)
{
	TIMED_FUNCTION();

	rectangle2i Viewport = {};
	if(Framebuffer)
	{
		Viewport.Max = Framebuffer->Resolution;
	}
	else
	{
		Viewport.Max = State->CurrentRenderSettings.Resolution;
	}

	OpenGLBindFramebuffer(State, Framebuffer, Viewport);
}
