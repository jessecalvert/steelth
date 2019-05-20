/*@H
* File: asset_builder.cpp
* Author: Jesse Calvert
* Created: December 18, 2017, 16:57
* Last modified: April 10, 2019, 17:31
*/

#ifdef GAME_DEBUG
	#undef GAME_DEBUG
#endif

#include "steelth.h"
#include <cstdio>
#include <cstdlib>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "steelth_convex_hull.cpp"

struct intermediate_data_buffer
{
	header_sla_v3 Header;
	buffer AssetHeaderData;
	buffer MetaData;
	buffer Data;
};

#pragma pack(push, 1)
struct bitmap_file_header
{
	u16 MagicValue;
	u32 Size;
	u16 Reserved0;
	u16 Reserved1;
	u32 ImageDataOffset;
};

struct bitmap_information_header
{
	u32 Size;
	u32 Width;
	u32 Height;
	u16 ColorPlanes;
	u16 BitsPerPixel;
	u32 Compression;
	u32 ImageSize;
	u32 HorizontalResolution;
	u32 VerticalResolution;
	u32 ColorsUsed;
	u32 ColorsImportant;

	u32 RedMask;
	u32 GreenMask;
	u32 BlueMask;
	u32 AlphaMask;
};
#pragma pack(pop)

#include "windows.h"
internal
ALLOCATE_MEMORY(Win32AllocateMemory)
{
	// NOTE: we don't want the platform_memory_block to change cache line alignment
	// Assert(sizeof(platform_memory_block) == 64);

	umm PageSize = 4096; // TODO: Query system for this.
	umm TotalSize = Size + sizeof(platform_memory_block);
	umm BaseOffset = sizeof(platform_memory_block);
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

	platform_memory_block *NewBlock = (platform_memory_block *)VirtualAlloc(0, TotalSize,
		MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	Assert(NewBlock);
	NewBlock->Base = (u8 *)NewBlock + BaseOffset;
	NewBlock->Size = Size;
	Assert(NewBlock->Used == 0);
	Assert(NewBlock->BlockPrev == 0);

	if(ProtectOffset)
	{
		DWORD OldProtect = 0;
		VirtualProtect((u8 *)NewBlock + ProtectOffset, PageSize, PAGE_NOACCESS, &OldProtect);
	}

	return NewBlock;
}

internal
DEALLOCATE_MEMORY(Win32DeallocateMemory)
{
	if(Block)
	{
		BOOL Success = VirtualFree(Block, 0, MEM_RELEASE);
		Assert(Success);
	}
}

internal u64
GetFileSize(char *Filename)
{
	u64 Result = 0;
	FILE *FileHandle;

	fopen_s(&FileHandle, Filename , "rb");
	if(FileHandle)
	{
		fseek(FileHandle, 0, SEEK_END);
		Result = ftell(FileHandle);
		fclose(FileHandle);
	}

	return Result;
}

internal u8 *
ReadEntireFileAndNullTerminate(char *Filename)
{
	u8 *Result = 0;
	FILE *FileHandle;
	u64 Size = GetFileSize(Filename);

	fopen_s(&FileHandle, Filename , "rb");
	if(FileHandle)
	{
		Result = (u8 *)calloc(1, Size + 1);
		fread(Result, Size, 1, FileHandle);
		Result[Size] = 0;

		fclose(FileHandle);
	}

	return Result;
}

struct loaded_bitmap
{
	u32 *Pixels;
	renderer_texture TextureHandle;
};

internal loaded_bitmap *
LoadBitmap(char *Filename)
{
	loaded_bitmap *Result = 0;

	u8 *BitmapFileData = ReadEntireFileAndNullTerminate(Filename);
	if(BitmapFileData)
	{
		bitmap_file_header *Header = (bitmap_file_header *)BitmapFileData;
		bitmap_information_header *InfoHeader = (bitmap_information_header *) (Header + 1);

		Assert(Header->MagicValue == 0x4D42);
		Assert((InfoHeader->Compression == 3) || (InfoHeader->Compression == 0));

		if(InfoHeader->BitsPerPixel == 32)
		{
			Assert(InfoHeader->RedMask == 0xFF000000);
			Assert(InfoHeader->GreenMask == 0x00FF0000);
			Assert(InfoHeader->BlueMask == 0x0000FF00);
			Assert(InfoHeader->AlphaMask == 0x000000FF);

			Result = (loaded_bitmap *)calloc(1, sizeof(loaded_bitmap));
			u32 *Pixels = (u32 *)(BitmapFileData + Header->ImageDataOffset);
			Result->TextureHandle = RendererTexture(0, InfoHeader->Width, InfoHeader->Height);
			Result->Pixels = Pixels;
		}
		else
		{
			Assert(InfoHeader->BitsPerPixel == 24);

			Result = (loaded_bitmap *)calloc(1, sizeof(loaded_bitmap));
			Result->TextureHandle = RendererTexture(0, InfoHeader->Width, InfoHeader->Height);
			u32 PixelCount = Result->TextureHandle.Width*Result->TextureHandle.Height;
			Result->Pixels = (u32 *)calloc(PixelCount, sizeof(u32));

			u8 *Source = BitmapFileData + Header->ImageDataOffset;
			u32 *Dest = Result->Pixels;
			for(u32 Count = 0;
				Count < PixelCount;
				++Count)
			{
				u32 Blue = *Source++;
				u32 Green = *Source++;
				u32 Red = *Source++;
				u32 Alpha = 0xFF;

				*Dest++ = ((Red << 24) |
						   (Green << 16) |
						   (Blue << 8) |
						   (Alpha << 0));
			}
		}
	}

	return Result;
}

inline u32
Sample(loaded_bitmap *Bitmap, r32 x, r32 y)
{
	x *= Bitmap->TextureHandle.Width;
	y *= Bitmap->TextureHandle.Height;
	u32 X0 = FloorReal32ToInt32(x);
	r32 tx = x - X0;
	u32 Y0 = FloorReal32ToInt32(y);
	r32 ty = y - Y0;

	u32 SampleOffsets[4] =
	{
		Y0*Bitmap->TextureHandle.Width + X0,
		Y0*Bitmap->TextureHandle.Width + X0 + 1,
		(Y0 + 1)*Bitmap->TextureHandle.Width + X0,
		(Y0 + 1)*Bitmap->TextureHandle.Width + X0 + 1,
	};

	r32 R[4] =
	{
		((Bitmap->Pixels[SampleOffsets[0]] & 0xFF000000) >> 24) / 255.0f,
		((Bitmap->Pixels[SampleOffsets[1]] & 0xFF000000) >> 24) / 255.0f,
		((Bitmap->Pixels[SampleOffsets[2]] & 0xFF000000) >> 24) / 255.0f,
		((Bitmap->Pixels[SampleOffsets[3]] & 0xFF000000) >> 24) / 255.0f,
	};

	r32 G[4] =
	{
		((Bitmap->Pixels[SampleOffsets[0]] & 0x00FF0000) >> 16) / 255.0f,
		((Bitmap->Pixels[SampleOffsets[1]] & 0x00FF0000) >> 16) / 255.0f,
		((Bitmap->Pixels[SampleOffsets[2]] & 0x00FF0000) >> 16) / 255.0f,
		((Bitmap->Pixels[SampleOffsets[3]] & 0x00FF0000) >> 16) / 255.0f,
	};

	r32 B[4] =
	{
		((Bitmap->Pixels[SampleOffsets[0]] & 0x0000FF00) >> 8) / 255.0f,
		((Bitmap->Pixels[SampleOffsets[1]] & 0x0000FF00) >> 8) / 255.0f,
		((Bitmap->Pixels[SampleOffsets[2]] & 0x0000FF00) >> 8) / 255.0f,
		((Bitmap->Pixels[SampleOffsets[3]] & 0x0000FF00) >> 8) / 255.0f,
	};

	r32 SR = Lerp(Lerp(R[0], tx, R[1]), ty, Lerp(R[2], tx, R[3]));
	r32 SG = Lerp(Lerp(G[0], tx, G[1]), ty, Lerp(G[2], tx, G[3]));
	r32 SB = Lerp(Lerp(B[0], tx, B[1]), ty, Lerp(B[2], tx, B[3]));

	u32 Result = ((((u32) (SR*255.0)) << 24) |
				  (((u32) (SG*255.0)) << 16) |
				  (((u32) (SB*255.0)) << 8) |
				  (0x000000FF));
	return Result;
}

inline r32
GetHeightFromHeightMap(loaded_bitmap *HeightMap, s32 X, s32 Y)
{
	if(X < 0) {X = 0;}
	if(X >= (s32)HeightMap->TextureHandle.Width) {X = HeightMap->TextureHandle.Width - 1;}
	if(Y < 0) {Y = 0;}
	if(Y >= (s32)HeightMap->TextureHandle.Height) {Y = HeightMap->TextureHandle.Height - 1;}

	u32 TexelColor = HeightMap->Pixels[X + Y*HeightMap->TextureHandle.Width];
	r32 Height = ((TexelColor & 0xFF000000) >> 24) / 255.0f;
	return Height;
}

internal loaded_bitmap *
CreateNormalFromHeightMap(loaded_bitmap *HeightMap)
{
	loaded_bitmap *Result = (loaded_bitmap *)calloc(1, sizeof(loaded_bitmap));
	Result->TextureHandle.Width = HeightMap->TextureHandle.Width;
	Result->TextureHandle.Height = HeightMap->TextureHandle.Height;
	Result->Pixels = (u32 *)calloc(Result->TextureHandle.Width*Result->TextureHandle.Height, sizeof(u32));

	v2 Offsets[] =
	{
		{-1, -1}, {0, -1}, {1, -1},
		{-1, 0}, {1, 0},
		{-1, 1}, {0, 1}, {1, 1},
	};

	u32 *DestRow = Result->Pixels;
	for(u32 Y = 0;
		Y < Result->TextureHandle.Height;
		++Y)
	{
		u32 *Dest = DestRow;
		for(u32 X = 0;
			X < Result->TextureHandle.Width;
			++X)
		{
			v3 Normal = V3(0, 0, 1.0f);
			r32 BaseHeight = GetHeightFromHeightMap(HeightMap, X, Y);
			for(u32 OffsetIndex = 0;
				OffsetIndex < ArrayCount(Offsets);
				++OffsetIndex)
			{
				v2 Offset = Offsets[OffsetIndex];
				r32 HeightSample = GetHeightFromHeightMap(HeightMap, (s32)(X + Offset.x), (s32)(Y + Offset.y));
				r32 HeightDifference = BaseHeight - HeightSample;
				r32 Fudge = 0.1f;
				Normal += Fudge * HeightDifference * Normalize(V3(Offset, 0.0f));
				Assert(Normal.z > 0.0f);
			}

			Normal = Normalize(Normal);

			*Dest++ = ((((u32)(255.0f*(0.5f*Normal.x + 0.5f))) << 24) |
					   (((u32)(255.0f*(0.5f*Normal.y + 0.5f))) << 16) |
					   (((u32)(255.0f*(0.5f*Normal.z + 0.5f))) << 8) |
					   (0xFF));
		}

		DestRow += Result->TextureHandle.Width;
	}

	return Result;
}

#define PushStructToBuffer(Buffer, type) (type *)PushSizeToBuffer(Buffer, sizeof(type))
internal u8 *
PushSizeToBuffer(buffer *Buffer, umm Size)
{
	u8 *Result = Buffer->Contents + Buffer->Used;
	Buffer->Used += Size;
	Assert(Buffer->Used < Buffer->Size);
	return Result;
}

internal void
WriteLoadedBitmap(intermediate_data_buffer *Buffer,
	asset_name Name, loaded_bitmap *BMP)
{
	Assert(BMP);
	header_sla_v3 *Header = &Buffer->Header;
	Header->TextureCount += 1;

	asset_header_sla_v3 *AssetHeader = PushStructToBuffer(&Buffer->AssetHeaderData, asset_header_sla_v3);
	AssetHeader->Type = AssetType_Bitmap;
	AssetHeader->Name = Name;
	AssetHeader->MetaDataOffset = Buffer->MetaData.Used;
	AssetHeader->MetaDataSize = sizeof(bitmap_sla_v3);
	AssetHeader->DataOffset = Buffer->Data.Used;

	bitmap_sla_v3 *BitmapData = PushStructToBuffer(&Buffer->MetaData, bitmap_sla_v3);
	BitmapData->Width = BMP->TextureHandle.Width;
	BitmapData->Height = BMP->TextureHandle.Height;

	AssetHeader->DataSize = BitmapData->Width * BitmapData->Height * sizeof(u32);
	u8 *DataDest = PushSizeToBuffer(&Buffer->Data, AssetHeader->DataSize);
	CopySize(BMP->Pixels, DataDest, AssetHeader->DataSize);
}

#define FlipEndian(a) ((a << 24) | ((a << 8) & 0x00FF0000) | ((a >> 8) & 0x0000FF00) | (a >> 24))

internal void
WritePNG(intermediate_data_buffer *Buffer,
	asset_name Name, char *Filename)
{
	u8 *PNGFileData = ReadEntireFileAndNullTerminate(Filename);
	if(PNGFileData)
	{
		stbi_set_flip_vertically_on_load(true);
		s32 Width = 0;
		s32 Height = 0;
		s32 Channels = 0;
		u32 *LoadedPNG = (u32 *)stbi_load_from_memory(PNGFileData, (int)GetFileSize(Filename), &Width, &Height,
			&Channels, 4);

		u32 PixelCount = Width * Height;
		for(u32 PixelIndex = 0;
			PixelIndex < PixelCount;
			++PixelIndex)
		{
			u32 Color = LoadedPNG[PixelIndex];
			LoadedPNG[PixelIndex] = FlipEndian(Color);
		}

		loaded_bitmap PNG = {};
		PNG.Pixels = LoadedPNG;
		PNG.TextureHandle = RendererTexture(0, Width, Height);

		WriteLoadedBitmap(Buffer, Name, &PNG);
	}
}

#if 0
internal void
WriteFloatPNG(header_sla *Header, intermediate_data_buffer *Buffer,
	asset_name Name, char *Filename, b32 FilterNearest)
{
	u8 *PNGFileData = ReadEntireFileAndNullTerminate(Filename);
	if(PNGFileData)
	{
		// stbi_set_flip_vertically_on_load(true);
		s32 Width = 0;
		s32 Height = 0;
		s32 Channels = 0;
		f32 *LoadedPNG = (f32 *)stbi_loadf_from_memory(PNGFileData, (int)GetFileSize(Filename), &Width, &Height,
			&Channels, 4);

		bitmap_sla *MetaData = (bitmap_sla *)(Buffer->MetaDataBuffer + Buffer->MetaDataOffset);
		Buffer->MetaDataOffset += sizeof(bitmap_sla);
		Header->TextureCount += 1;

		MetaData->Name = Name;
		MetaData->BytesPerChannel = 4;
		MetaData->Channels = Channels;
		MetaData->Width = Width;
		MetaData->Height = Height;
		MetaData->FilterNearest = FilterNearest;
		MetaData->PixelsOffset = Buffer->DataOffset;

		u32 DataSize = MetaData->Width * MetaData->Height * MetaData->Channels * MetaData->BytesPerChannel;
		memcpy(Buffer->DataBuffer + Buffer->DataOffset, LoadedPNG, DataSize);
		Buffer->DataOffset += DataSize;

		MetaData->PixelsSize = DataSize;
	}
}
#endif

internal void
WriteBMP(intermediate_data_buffer *Buffer,
	asset_name Name, char *Filename)
{
	loaded_bitmap *BMP = LoadBitmap(Filename);
	WriteLoadedBitmap(Buffer, Name, BMP);
}

inline b32
ValidCoords(v2i Coords, s32 Width, s32 Height)
{
	b32 Result = ((Coords.x >= 0) &&
				  (Coords.y >= 0) &&
				  (Coords.x < Width) &&
				  (Coords.y < Height));
	return Result;
}

inline void
TestCoords(u8 *Bitmap, s32 Width, s32 Height, v2i Coords, v2i From, f32 *Distance, u8 Value)
{
	u8 TestValue = 0;
	if(ValidCoords(Coords, Width, Height))
	{
		TestValue = Bitmap[Coords.x + Coords.y*Width];
	}

	if(TestValue != Value)
	{
		f32 CurrentDistance = Length(From - Coords);
		if(CurrentDistance < *Distance)
		{
			*Distance = CurrentDistance;
		}
	}
}

inline u8
GetSignedDistanceToDifference(u8 *Bitmap, s32 Width, s32 Height, v2i From, s32 MaxDistance)
{
	f32 Distance = Real32Maximum;
	u8 Value = 0;
	if(ValidCoords(From, Width, Height))
	{
		Value = Bitmap[From.x + From.y * Width];
	}

	if((Value == 0) || (Value == 255))
	{
		for(s32 SearchDistance = 1;
			SearchDistance < MaxDistance;
			++SearchDistance)
		{
			s32 Dim = SearchDistance*2 + 1;
			for(s32 Y = 0;
				Y < Dim;
				++Y)
			{
				v2i LeftCoords = From + V2i(-SearchDistance, -SearchDistance + Y);
				v2i RightCoords = From + V2i(SearchDistance, -SearchDistance + Y);
				TestCoords(Bitmap, Width, Height, LeftCoords, From, &Distance, Value);
				TestCoords(Bitmap, Width, Height, RightCoords, From, &Distance, Value);
			}

			for(s32 X = 1;
				X < (Dim - 1);
				++X)
			{
				v2i TopCoords = From + V2i(-SearchDistance + X, SearchDistance);
				v2i BottomCoords = From + V2i(-SearchDistance + X, -SearchDistance);
				TestCoords(Bitmap, Width, Height, TopCoords, From, &Distance, Value);
				TestCoords(Bitmap, Width, Height, BottomCoords, From, &Distance, Value);
			}
		}
	}
	else
	{
		Distance = ((255.0f - Value) / 255.0f);
	}

	Distance = Clamp(Distance, 0.0f, (f32)MaxDistance);
	Distance /= (f32)MaxDistance;
	f32 ResultF32 = Lerp(0.5f, Distance, 0.0f);
	if(Value > 128)
	{
		ResultF32 = 1.0f - ResultF32;
	}

	u8 Result = (u8)(ResultF32*255.0f);

	return Result;
}

internal void
WriteSignedDistanceFieldFont(intermediate_data_buffer *Buffer, asset_name Name, char *Filename, f32 DesiredHeight, u32 PixelsDim)
{
	header_sla_v3 *Header = &Buffer->Header;
	Header->FontCount += 1;

	asset_header_sla_v3 *AssetHeader = PushStructToBuffer(&Buffer->AssetHeaderData, asset_header_sla_v3);
	AssetHeader->Type = AssetType_Font;
	AssetHeader->Name = Name;
	AssetHeader->MetaDataOffset = Buffer->MetaData.Used;
	AssetHeader->MetaDataSize = sizeof(font_sla_v3);
	AssetHeader->DataOffset = 0;
	AssetHeader->DataSize = 0;

	font_sla_v3 *MetaData = PushStructToBuffer(&Buffer->MetaData, font_sla_v3);
	u8 *FontFile = ReadEntireFileAndNullTerminate(Filename);
	Assert(FontFile);

	stbtt_fontinfo FontInfo = {};
	int Error = stbtt_InitFont(&FontInfo, FontFile, 0);
	Assert(Error);

	const f32 FontHeight = DesiredHeight;
	s32 Apron = (s32)(FontHeight/4);

	MetaData->Scale = stbtt_ScaleForPixelHeight(&FontInfo, FontHeight);
	s32 Ascent, Descent, Linegap;
	stbtt_GetFontVMetrics(&FontInfo, &Ascent, &Descent, &Linegap);
	MetaData->Ascent = MetaData->Scale*Ascent;
	MetaData->Descent = MetaData->Scale*Descent;
	MetaData->Linegap = MetaData->Scale*Linegap;
	MetaData->Apron = Apron;

	for(u32 Codepoint = 0;
		Codepoint < 128;
		++Codepoint)
	{
		s32 LeftSideBearing, Advance;
		stbtt_GetCodepointHMetrics(&FontInfo, Codepoint, &Advance, &LeftSideBearing);
		MetaData->Advance[Codepoint] = MetaData->Scale*Advance;
		MetaData->Offset[Codepoint].x = MetaData->Scale*LeftSideBearing;

		for(u32 NextCodepoint = 0;
		NextCodepoint < 128;
		++NextCodepoint)
		{
			MetaData->Kerning[Codepoint][NextCodepoint] =
				MetaData->Scale*stbtt_GetCodepointKernAdvance(&FontInfo, Codepoint, NextCodepoint);
		}
	}

	v2i Min, Max;
	stbtt_GetFontBoundingBox(&FontInfo, &Min.x, &Min.y, &Max.x, &Max.y);

	u32 BufferWidth = (u32)(FontHeight + 1);
	u8 *TempTempBuffer = (u8 *)calloc(BufferWidth, BufferWidth);

	u8 *TempBuffers[128];
	v2i BufferDim[128] = {};
	for(u32 Codepoint = 0;
		Codepoint < 128;
		++Codepoint)
	{
		s32 x0, x1, y0, y1;
		stbtt_GetCodepointBitmapBox(&FontInfo, Codepoint, MetaData->Scale, MetaData->Scale,
			&x0, &y0, &x1, &y1);
		BufferDim[Codepoint] = V2i(x1 - x0, y1 - y0);
		TempBuffers[Codepoint] = (u8 *)calloc(1, BufferDim[Codepoint].x*BufferDim[Codepoint].y*sizeof(u8));

		MetaData->Offset[Codepoint].y = (f32)-y1;
		// MetaData->Offset[Codepoint].x += MetaData->Scale*x0;

		stbtt_MakeCodepointBitmap(&FontInfo, TempTempBuffer,
			BufferWidth, BufferWidth, BufferWidth,
			MetaData->Scale, MetaData->Scale, Codepoint);

		if(Codepoint > ' ')
		{
			u8 *DestRow = TempBuffers[Codepoint];
			u8 *SourceRow = TempTempBuffer + (BufferDim[Codepoint].y - 1)*BufferWidth;
			for(s32 Y = 0;
				Y < BufferDim[Codepoint].y;
				++Y)
			{
				u8 *Dest = DestRow;
				u8 *Source = SourceRow;
				for(s32 X = 0;
					X < BufferDim[Codepoint].x;
					++X)
				{
					u8 Value = *Source++;
					*Dest++ = Value;
				}

				DestRow += BufferDim[Codepoint].x;
				SourceRow -= BufferWidth;
			}
		}
	}

	u32 *Pixels = (u32 *)calloc(PixelsDim, PixelsDim*sizeof(u32));
	s32 CharSpacing = 5;
	s32 XPos = 0;
	s32 YPos = 0;
	s32 MaxHeight = 0;
	for(u32 Codepoint = 0;
		Codepoint < 128;
		++Codepoint)
	{
		s32 HorizontalSpaceLeft = PixelsDim - XPos;
		if(HorizontalSpaceLeft < (BufferDim[Codepoint].x + 2*Apron))
		{
			XPos = 0;
			YPos += MaxHeight + CharSpacing;
			MaxHeight = 0;
			Assert((YPos + BufferDim[Codepoint].y + 2*Apron) < (s32)PixelsDim);
		}

		u32 *DestRow = Pixels + YPos*PixelsDim + XPos;
		for(s32 Y = 0;
			Y < (BufferDim[Codepoint].y + 2*Apron);
			++Y)
		{
			u32 *Dest = DestRow;
			for(s32 X = 0;
				X < (BufferDim[Codepoint].x + 2*Apron);
				++X)
			{
				u8 DistanceValue = GetSignedDistanceToDifference(TempBuffers[Codepoint], BufferDim[Codepoint].x, BufferDim[Codepoint].y, V2i(X - Apron, Y - Apron), Apron);
				u32 PixelColor = ((0xFF << 24) |
								  (0xFF << 16) |
								  (0xFF << 8) |
								  (DistanceValue << 0));

				*Dest++ = PixelColor;
			}

			DestRow += PixelsDim;
		}

		MetaData->TexCoordMin[Codepoint] = V2((r32)XPos/(r32)PixelsDim, (r32)YPos/(r32)PixelsDim);
		MetaData->TexCoordMax[Codepoint] = V2((r32)(XPos + BufferDim[Codepoint].x + 2*Apron)/(r32)PixelsDim,
											  (r32)(YPos + BufferDim[Codepoint].y + 2*Apron)/(r32)PixelsDim);

		if(MaxHeight < BufferDim[Codepoint].y + 2*Apron)
		{
			MaxHeight = BufferDim[Codepoint].y + 2*Apron;
		}
		XPos += BufferDim[Codepoint].x + CharSpacing + 2*Apron;
	}

	MetaData->BitmapHeader.Type = AssetType_Bitmap;
	MetaData->BitmapHeader.Name = Name;
	MetaData->BitmapHeader.MetaDataOffset = AssetHeader->MetaDataOffset + OffsetOf(font_sla_v3, BitmapSLA);
	MetaData->BitmapHeader.MetaDataSize = sizeof(bitmap_sla_v3);
	MetaData->BitmapHeader.DataOffset = Buffer->Data.Used;
	MetaData->BitmapHeader.DataSize = PixelsDim * PixelsDim * sizeof(u32);
	MetaData->BitmapSLA.Width = PixelsDim;
	MetaData->BitmapSLA.Height = PixelsDim;
	u8 *PixelsDest = PushSizeToBuffer(&Buffer->Data, MetaData->BitmapHeader.DataSize);
	CopySize(Pixels, PixelsDest, MetaData->BitmapHeader.DataSize);

	free(TempTempBuffer);
	for(u32 Codepoint = 0;
		Codepoint < 128;
		++Codepoint)
	{
		free(TempBuffers[Codepoint]);
	}
}

#if 0
internal void
WriteFont(header_sla_v3 *Header, intermediate_data_buffer *Buffer, asset_name Name, char *Filename)
{
	font_sla *MetaData = (font_sla *)(Buffer->MetaDataBuffer + Buffer->MetaDataOffset);
	Buffer->MetaDataOffset += sizeof(font_sla);
	Header->FontCount += 1;
	MetaData->Name = Name;

	u8 *FontFile = ReadEntireFileAndNullTerminate(Filename);
	Assert(FontFile);

	stbtt_fontinfo FontInfo = {};
	int Error = stbtt_InitFont(&FontInfo, FontFile, 0);
	Assert(Error);

	const f32 FontHeight = 100.0f;
	MetaData->Scale = stbtt_ScaleForPixelHeight(&FontInfo, FontHeight);
	s32 Ascent, Descent, Linegap;
	stbtt_GetFontVMetrics(&FontInfo, &Ascent, &Descent, &Linegap);
	MetaData->Ascent = MetaData->Scale*Ascent;
	MetaData->Descent = MetaData->Scale*Descent;
	MetaData->Linegap = MetaData->Scale*Linegap;

	for(u32 Codepoint = 0;
		Codepoint < 128;
		++Codepoint)
	{
		s32 LeftSideBearing, Advance;
		stbtt_GetCodepointHMetrics(&FontInfo, Codepoint, &Advance, &LeftSideBearing);
		MetaData->Advance[Codepoint] = MetaData->Scale*Advance;
		MetaData->Offset[Codepoint].x = MetaData->Scale*LeftSideBearing;

		for(u32 NextCodepoint = 0;
		NextCodepoint < 128;
		++NextCodepoint)
		{
			MetaData->Kerning[Codepoint][NextCodepoint] =
				MetaData->Scale*stbtt_GetCodepointKernAdvance(&FontInfo, Codepoint, NextCodepoint);
		}
	}

	v2i Min, Max;
	stbtt_GetFontBoundingBox(&FontInfo, &Min.x, &Min.y, &Max.x, &Max.y);

	u32 BufferWidth = (u32)(FontHeight + 1);
	u8 *TempTempBuffer = (u8 *)calloc(BufferWidth, BufferWidth);

	u8 *TempBuffers[128];
	v2i BufferDim[128] = {};
	for(u32 Codepoint = 0;
		Codepoint < 128;
		++Codepoint)
	{
		s32 x0, x1, y0, y1;
		stbtt_GetCodepointBitmapBox(&FontInfo, Codepoint, MetaData->Scale, MetaData->Scale,
			&x0, &y0, &x1, &y1);
		BufferDim[Codepoint] = V2i(x1 - x0, y1 - y0);
		TempBuffers[Codepoint] = (u8 *)calloc(1, BufferDim[Codepoint].x*BufferDim[Codepoint].y*sizeof(u8));

		MetaData->Offset[Codepoint].y = (f32)-y1;
		// MetaData->Offset[Codepoint].x += MetaData->Scale*x0;

		stbtt_MakeCodepointBitmap(&FontInfo, TempTempBuffer,
			BufferWidth, BufferWidth, BufferWidth,
			MetaData->Scale, MetaData->Scale, Codepoint);

		if(Codepoint > ' ')
		{
			u8 *DestRow = TempBuffers[Codepoint];
			u8 *SourceRow = TempTempBuffer + (BufferDim[Codepoint].y - 1)*BufferWidth;
			for(s32 Y = 0;
				Y < BufferDim[Codepoint].y;
				++Y)
			{
				u8 *Dest = DestRow;
				u8 *Source = SourceRow;
				for(s32 X = 0;
					X < BufferDim[Codepoint].x;
					++X)
				{
					*Dest++ = *Source++;
				}

				DestRow += BufferDim[Codepoint].x;
				SourceRow -= BufferWidth;
			}
		}
	}

	u32 PixelsDim = 1024;
	u32 *Pixels = (u32 *)calloc(PixelsDim, PixelsDim * sizeof(u32));
	s32 CharSpacing = 5;
	s32 XPos = 0;
	s32 YPos = 0;
	s32 MaxHeight = 0;
	for(u32 Codepoint = 0;
		Codepoint < 128;
		++Codepoint)
	{
		s32 HorizontalSpaceLeft = PixelsDim - XPos;
		if(HorizontalSpaceLeft < BufferDim[Codepoint].x)
		{
			XPos = 0;
			YPos += MaxHeight + CharSpacing;
			MaxHeight = 0;
			Assert((YPos + BufferDim[Codepoint].y) < (s32)PixelsDim);
		}

		u8 *Source = TempBuffers[Codepoint];
		u32 *DestRow = Pixels + YPos*PixelsDim + XPos;
		for(s32 Y = 0;
			Y < BufferDim[Codepoint].y;
			++Y)
		{
			u32 *Dest = DestRow;
			for(s32 X = 0;
				X < BufferDim[Codepoint].x;
				++X)
			{
				u32 PixelColor = ((0xFF << 24) |
								  (0xFF << 16) |
								  (0xFF << 8) |
								  ((*Source++) << 0));
				*Dest++ = PixelColor;
			}

			DestRow += PixelsDim;
		}

		MetaData->TexCoordMin[Codepoint] = V2((r32)XPos/(r32)PixelsDim, (r32)YPos/(r32)PixelsDim);
		MetaData->TexCoordMax[Codepoint] = V2((r32)(XPos + BufferDim[Codepoint].x)/(r32)PixelsDim,
											  (r32)(YPos + BufferDim[Codepoint].y)/(r32)PixelsDim);

		if(MaxHeight < BufferDim[Codepoint].y)
		{
			MaxHeight = BufferDim[Codepoint].y;
		}
		XPos += BufferDim[Codepoint].x + CharSpacing;
	}

	MetaData->BitmapSLA.Name = Name;
	MetaData->BitmapSLA.PixelsOffset = Buffer->DataOffset;
	MetaData->BitmapSLA.Width = PixelsDim;
	MetaData->BitmapSLA.Height = PixelsDim;
	u32 DataSize = PixelsDim * PixelsDim * sizeof(u32);
	memcpy(Buffer->DataBuffer + Buffer->DataOffset, Pixels, DataSize);
	Buffer->DataOffset += DataSize;

	MetaData->BitmapSLA.PixelsSize = DataSize;

	free(TempTempBuffer);
	for(u32 Codepoint = 0;
		Codepoint < 128;
		++Codepoint)
	{
		free(TempBuffers[Codepoint]);
	}
}
#endif

internal void
WriteMesh(intermediate_data_buffer *Buffer, asset_name Name,
	vertex_format *Vertices, u32 VertexCount,
	v3u *Faces, u32 FaceCount)
{
	Assert(Vertices);
	Assert(Faces);
	header_sla_v3 *Header = &Buffer->Header;
	Header->MeshCount += 1;

	asset_header_sla_v3 *AssetHeader = PushStructToBuffer(&Buffer->AssetHeaderData, asset_header_sla_v3);
	AssetHeader->Type = AssetType_Mesh;
	AssetHeader->Name = Name;
	AssetHeader->MetaDataOffset = Buffer->MetaData.Used;
	AssetHeader->MetaDataSize = sizeof(mesh_sla_v3);
	AssetHeader->DataOffset = Buffer->Data.Used;

	mesh_sla_v3 *MetaData = PushStructToBuffer(&Buffer->MetaData, mesh_sla_v3);
	MetaData->VertexCount = VertexCount;
	MetaData->FaceCount = FaceCount;

	MetaData->VerticesSize = MetaData->VertexCount*sizeof(vertex_format);
	MetaData->FacesSize = MetaData->FaceCount*sizeof(v3u);
	AssetHeader->DataSize = MetaData->VerticesSize + MetaData->FacesSize;

	u8 *DataDest = PushSizeToBuffer(&Buffer->Data, AssetHeader->DataSize);
	CopySize(Vertices, DataDest, MetaData->VerticesSize);
	CopySize(Faces, DataDest + MetaData->VerticesSize, MetaData->FacesSize);
}

QH_SUPPORT(CapsuleSupport)
{
	// NOTE: Dim = V3(1.0f, 2.0f, 1.0f);

	v3 Result = {};

	f32 Radius = 0.5f;
	v3 A = V3(0, -0.5f, 0);
	v3 B = V3(0, 0.5f, 0);
	v3 NormalizedDirection = Normalize(Direction);
	if(SameDirection(V3(0,1,0), Direction))
	{
		Result = B + Radius*NormalizedDirection;
	}
	else
	{
		Result = A + Radius*NormalizedDirection;
	}

	return Result;
}

QH_SUPPORT(SphereSupport)
{
	f32 Radius = 1.0f;

	v3 NormalizedDirection = Normalize(Direction);
	v3 SphereSupport = NormalizedDirection*Radius;

	return SphereSupport;
}

QH_SUPPORT(RoundedBoxSupport)
{
	f32 Radius = 0.05f;
	v3 Dim = V3(1,1,1) - 2.0f*V3(Radius, Radius, Radius);
	v3 HalfDim = 0.5f*Dim;

	v3 CubePoints[] =
	{
		V3(HalfDim.x, HalfDim.y, HalfDim.z),
		V3(-HalfDim.x, HalfDim.y, HalfDim.z),
		V3(HalfDim.x, -HalfDim.y, HalfDim.z),
		V3(HalfDim.x, HalfDim.y, -HalfDim.z),
		V3(-HalfDim.x, -HalfDim.y, HalfDim.z),
		V3(HalfDim.x, -HalfDim.y, -HalfDim.z),
		V3(-HalfDim.x, HalfDim.y, -HalfDim.z),
		V3(-HalfDim.x, -HalfDim.y, -HalfDim.z),
	};

	v3 NormalizedDirection = Normalize(Direction);
	v3 SphereSupport = NormalizedDirection*Radius;

	f32 BestDistance = 0.0f;
	v3 CubeSupport = {};
	for(u32 Index = 0;
		Index < ArrayCount(CubePoints);
		++Index)
	{
		v3 Point = CubePoints[Index];
		f32 Distance = Inner(Direction, Point);
		if(Distance > BestDistance)
		{
			BestDistance = Distance;
			CubeSupport = Point;
		}
	}

	v3 Result = CubeSupport + SphereSupport;
	return Result;
}

internal u32
AddOrGetVertex_(v3 Point, v3 Normal, vertex_format *Vertices, u32 *VertexCount)
{
	u32 Result = 0;
	b32 Found = false;
	for(u32 Index = 0;
		!Found && (Index < *VertexCount);
		++Index)
	{
		vertex_format *Vertex = Vertices + Index;
		if(Vertex->Position == Point)
		{
			Result = Index;
			Vertex->Normal += Normal;
			Found = true;
		}
	}

	if(!Found)
	{
		Result = (*VertexCount)++;
		vertex_format *Vertex = Vertices + Result;
		Vertex->Position = Point;
		Vertex->Normal = Normal;
		Vertex->Color = 0xFFFFFFFF;
	}

	return Result;
}

internal void
WriteMeshQHConvexHull(intermediate_data_buffer *Buffer, asset_name Name, memory_region *Region, qh_convex_hull *Hull, b32 NormalsFromCenter)
{
	temporary_memory TempMem = BeginTemporaryMemory(Region);

	u32 VertexCount = 0;
	u32 FaceCount = 0;
	u32 MaxFaces = 10*Hull->FaceCount;
	vertex_format *Vertices = PushArray(Region, Hull->VertexCount, vertex_format);
	v3u *Faces = PushArray(Region, MaxFaces, v3u);

	for(qh_face *Face = Hull->FaceSentinel.Next;
		Face != &Hull->FaceSentinel;
		Face = Face->Next)
	{
		qh_half_edge *FirstEdge = Face->FirstEdge;
		qh_half_edge *LastEdge = FirstEdge->Next;
		qh_half_edge *Edge = LastEdge->Next;

		u32 FirstVertexIndex = AddOrGetVertex_(FirstEdge->Origin, Face->Normal, Vertices, &VertexCount);
		u32 LastVertexIndex = AddOrGetVertex_(LastEdge->Origin, Face->Normal, Vertices, &VertexCount);

		do
		{
			u32 VertexIndex = AddOrGetVertex_(Edge->Origin, Face->Normal, Vertices, &VertexCount);

			Assert(FaceCount < MaxFaces);
			Faces[FaceCount++] = {FirstVertexIndex, LastVertexIndex, VertexIndex};

			LastVertexIndex = VertexIndex;
			Edge = Edge->Next;
		} while(Edge != Face->FirstEdge);
	}

	for(u32 Index = 0;
		Index < VertexCount;
		++Index)
	{
		vertex_format *Vertex = Vertices + Index;
		if(NormalsFromCenter)
		{
			Vertex->Normal = Normalize(Vertex->Position);
		}
		else
		{
			Vertex->Normal = Normalize(Vertex->Normal);
		}
	}

	Assert(VertexCount == Hull->VertexCount);

	WriteMesh(Buffer, Name, Vertices, VertexCount, Faces, FaceCount);

	EndTemporaryMemory(&TempMem);
}

internal void
WriteMeshQHConvexHull(intermediate_data_buffer *Buffer, asset_name Name, u32 MaxSteps, qh_support *SupportFunction, b32 NormalsFromCenter = false)
{
	Assert(MaxSteps > 3);

	memory_region Region = {};
	temporary_memory TempMem = BeginTemporaryMemory(&Region);

	v3 Directions[] =
	{
		V3(1,0,0),
		V3(0,1,0),
		V3(0,0,1),
		V3(-1,0,0),
		V3(0,-1,0),
		V3(0,0,-1),
	};

	v3 Points[] =
	{
		SupportFunction(Directions[0]),
		SupportFunction(Directions[1]),
		SupportFunction(Directions[2]),
		SupportFunction(Directions[3]),
		SupportFunction(Directions[4]),
		SupportFunction(Directions[5]),
	};

	qh_convex_hull *Hull = QuickHullInitialTetrahedron(&Region, Points, ArrayCount(Points), false);

	while(MaxSteps-- && QuickHullStep(&Region, Hull, SupportFunction)) {};

	WriteMeshQHConvexHull(Buffer, Name, &Region, Hull, NormalsFromCenter);

	EndTemporaryMemory(&TempMem);
}

#if 0
internal void
BuildSphereMeshData(memory_region *TempRegion, f32 Radius, u32 LayerCount, u32 WedgeCount)
{
	v3 Center = V3(0, 0, 0);

	temporary_memory TempMem = BeginTemporaryMemory(TempRegion);

	u32 VertexCount = (LayerCount - 1)*WedgeCount + 2;
	u32 FaceCount = 2*(LayerCount - 1)*WedgeCount;

	r32 VerticalAngleDelta = Pi32 / LayerCount;
	r32 HorizontalAngleDelta = 2.0f*Pi32 / WedgeCount;

	v3u *Faces = PushArray(TempRegion, FaceCount, v3u);
	vertex_format *Vertices = PushArray(TempRegion, VertexCount, vertex_format);
	for(u32 LayerIndex = 0;
		LayerIndex <= LayerCount;
		++LayerIndex)
	{
		if(LayerIndex == 0)
		{
			v3 RadiusVector = SphereRadiusVector(Radius, 0.0f, 0.0f);
			Vertices[0].Position = RadiusVector;
			Vertices[0].Normal = (1.0f/Radius)*RadiusVector;
			Vertices[0].TexCoords = V2(0.0f, 0.0f);
			Vertices[0].Color = 0xFFFFFFFF;
		}
		else if(LayerIndex == LayerCount)
		{
			v3 RadiusVector = SphereRadiusVector(Radius, Pi32, 0.0f);
			Vertices[VertexCount - 1].Position = RadiusVector;
			Vertices[VertexCount - 1].Normal = (1.0f/Radius)*RadiusVector;
			Vertices[VertexCount - 1].TexCoords = V2(0.0f, 1.0f);
			Vertices[VertexCount - 1].Color = 0xFFFFFFFF;

			for(u32 WedgeIndex = 0;
				WedgeIndex < WedgeCount;
				++WedgeIndex)
			{
				u32 FaceIndex = (FaceCount - 1) - WedgeIndex;
				u32 FirstVertexIndex = VertexCount - 2 - WedgeIndex;
				u32 SecondVertexIndex = (WedgeIndex == (WedgeCount - 1)) ?
					VertexCount - 2 :
					VertexCount - 3 - WedgeIndex;
				Faces[FaceIndex] =
				{
				 	VertexCount - 1,
				 	FirstVertexIndex,
				 	SecondVertexIndex,
				};
			}
		}
		else
		{
			r32 Phi = VerticalAngleDelta*LayerIndex;
			for(u32 WedgeIndex = 0;
				WedgeIndex < WedgeCount;
				++WedgeIndex)
			{
				r32 Theta = HorizontalAngleDelta*WedgeIndex;
				v3 RadiusVector = SphereRadiusVector(Radius, Phi, Theta);
				v3 LastRadiusVector = SphereRadiusVector(Radius, Phi, Theta - HorizontalAngleDelta);
				u32 VertexIndex = 1 + WedgeCount*(LayerIndex - 1) + WedgeIndex;
				u32 LastVertexIndex = (WedgeIndex == 0) ? (WedgeCount*LayerIndex) : (VertexIndex - 1);

				Vertices[VertexIndex].Position = RadiusVector;
				Vertices[VertexIndex].Normal = (1.0f/Radius)*RadiusVector;
				Vertices[VertexIndex].TexCoords = V2(Theta/(2.0f*Pi32), Phi/Pi32);
				Vertices[VertexIndex].Color = 0xFFFFFFFF;

				if(LayerIndex == 1)
				{
					u32 FaceIndex = WedgeIndex;
					Faces[FaceIndex] = {0, LastVertexIndex, VertexIndex};
				}
				else
				{
					u32 FaceIndex = WedgeCount + 2*WedgeCount*(LayerIndex - 2) + 2*WedgeIndex;
					Faces[FaceIndex] = {LastVertexIndex - WedgeCount, LastVertexIndex, VertexIndex};
					Faces[FaceIndex + 1] = {LastVertexIndex - WedgeCount, VertexIndex, VertexIndex - WedgeCount};
				}
			}
		}
	}

	EndTemporaryMemory(&TempMem);
}
#endif

internal intermediate_data_buffer
BeginSLA()
{
	umm BufferSize = Gigabytes(1);

	intermediate_data_buffer Buffer = {};
	Buffer.AssetHeaderData.Contents = (u8 *)calloc(1, BufferSize);
	Buffer.AssetHeaderData.Size = BufferSize;
	Buffer.MetaData.Contents = (u8 *)calloc(1, BufferSize);
	Buffer.MetaData.Size = BufferSize;
	Buffer.Data.Contents = (u8 *)calloc(1, BufferSize);
	Buffer.Data.Size = BufferSize;

	Buffer.Header.Header.MagicCode = MAGIC_CODE;
	Buffer.Header.Header.Version = 3;

	return Buffer;
}

internal void
FinishSLA(intermediate_data_buffer *Buffer, char *Filename)
{
	header_sla_v3 *Header = &Buffer->Header;
	Header->DataOffset = sizeof(header_sla_v3);
	Header->DataSize = Buffer->Data.Used;
	Header->MetaDataOffset = Header->DataOffset + Buffer->Data.Used;
	Header->MetaDataSize = Buffer->MetaData.Used;
	Header->AssetHeadersOffset = Header->MetaDataOffset + Buffer->MetaData.Used;
	Header->AssetHeadersSize = Buffer->AssetHeaderData.Used;

	b32 EnoughSpace = true;
	if(Buffer->AssetHeaderData.Used > Buffer->AssetHeaderData.Size)
	{
		printf("Asset Header buffer overflow!\n");
		EnoughSpace = false;
	}
	if(Buffer->Data.Used > Buffer->Data.Size)
	{
		printf("Data buffer overflow!\n");
		EnoughSpace = false;
	}
	if(Buffer->MetaData.Used > Buffer->MetaData.Size)
	{
		printf("Metadata buffer overflow!\n");
		EnoughSpace = false;
	}

	if(EnoughSpace)
	{
		FILE *Out;
		fopen_s(&Out, Filename, "wb");
		fwrite(Header, sizeof(header_sla_v3), 1, Out);
		fwrite(Buffer->Data.Contents, Buffer->Data.Used, 1, Out);
		fwrite(Buffer->MetaData.Contents, Buffer->MetaData.Used, 1, Out);
		fwrite(Buffer->AssetHeaderData.Contents, Buffer->AssetHeaderData.Used, 1, Out);
		fclose(Out);
	}
}

internal void
WriteTextureFiles()
{
	intermediate_data_buffer Buffer = BeginSLA();
	WriteBMP(&Buffer, Asset_Circle, "Textures\\circle.bmp");
	WritePNG(&Buffer, Asset_Smile, "Textures\\smile.png");
	WritePNG(&Buffer, Asset_Arrow, "Textures\\arrow.png");
	WritePNG(&Buffer, Asset_GoldIcon, "Textures\\gold_icon.png");
	WritePNG(&Buffer, Asset_HeartIcon, "Textures\\heart_icon.png");
	WritePNG(&Buffer, Asset_ManaIcon, "Textures\\mana_icon.png");
	WritePNG(&Buffer, Asset_SwordIcon, "Textures\\sword_icon.png");
	WritePNG(&Buffer, Asset_ClockIcon, "Textures\\clock_icon.png");
	FinishSLA(&Buffer, "asset_pack_textures.sla");
}

internal void
WriteFontFiles()
{
	u32 PixelsDim = 1024;
	f32 DesiredHeight = 75.0f;

	intermediate_data_buffer Buffer = BeginSLA();
	WriteSignedDistanceFieldFont(&Buffer, Asset_LiberationMonoFont, "Fonts\\LiberationMono-Regular.ttf", DesiredHeight, PixelsDim);
	WriteSignedDistanceFieldFont(&Buffer, Asset_TimesFont, "Fonts\\times.ttf", DesiredHeight, PixelsDim);
	FinishSLA(&Buffer, "asset_pack_fonts.sla");
}

internal void
WriteMeshFiles()
{
	intermediate_data_buffer Buffer = BeginSLA();
	WriteMeshQHConvexHull(&Buffer, Asset_RoundedBox, 256, RoundedBoxSupport);
	WriteMeshQHConvexHull(&Buffer, Asset_Sphere, 512, SphereSupport, true);
	WriteMeshQHConvexHull(&Buffer, Asset_Capsule, 256, CapsuleSupport);
	FinishSLA(&Buffer, "asset_pack_meshes.sla");
}

s32 main()
{
	platform Plat = {};
	Plat.AllocateMemory = Win32AllocateMemory;
	Plat.DeallocateMemory = Win32DeallocateMemory;
	Platform = &Plat;

	WriteTextureFiles();
	WriteFontFiles();
	WriteMeshFiles();
}
