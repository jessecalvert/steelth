/*@H
* File: slaedit.cpp
* Author: Jesse Calvert
* Created: December 4, 2018, 14:32
* Last modified: April 10, 2019, 18:51
*/

#include "steelth.h"
#include "stdio.h"

internal void
ParseArgs(s32 ArgCount, char **ArgsCString, string *ArgsOut)
{
	for(u32 Index = 0;
		Index < (u32)ArgCount;
		++Index)
	{
		ArgsOut[Index] = WrapZ(ArgsCString[Index]);
	}
}

internal void
Print(string String)
{
	printf("%.*s", String.Length, String.Text);
}

internal void
Printf(char *Format, ...)
{
	char Buffer[1024] = {};
	va_list ArgList;
	va_start(ArgList, Format);
	string String = FormatStringList(Buffer, ArrayCount(Buffer), Format, ArgList);
	va_end(ArgList);

	Print(String);
}

#include "windows.h"
internal
ALLOCATE_MEMORY(Win32AllocateMemory)
{
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
		Printf("Could not open file! (%s)\n", WrapZ(Filename));
	}

	return Result;
}

internal b32
Win32WriteEntireFile(void *Data, u32 Size, string Filename)
{
	b32 Sucess = true;
	char FilenameCString[512];
	FormatString(FilenameCString, ArrayCount(FilenameCString), "%s", Filename);
	HANDLE OutFile = CreateFileA(FilenameCString, GENERIC_WRITE, 0, 0, CREATE_NEW, 0, 0);
	if(OutFile != INVALID_HANDLE_VALUE)
	{
		DWORD BytesWritten;
		WriteFile(OutFile, Data, Size, &BytesWritten, 0);
		if(BytesWritten != Size)
		{
			Printf("Unable to write all bytes! (%s) Wrote %d/%d\n", Filename, BytesWritten, Size);
			Sucess = false;
		}

		CloseHandle(OutFile);
	}
	else
	{
		Printf("Unable to create file! (%s)\n", Filename);
		Sucess = false;
	}

	return Sucess;
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
	Result.Used = Result.Size = FileSize;
	if(Result.Contents)
	{
		Result.Contents[FileSize - 1] = 0;
	}

	return Result;
}

struct tab_layout
{
	u32 Tabs;
	u32 TabLength;
};

internal tab_layout
BeginTabLayout()
{
	tab_layout Result = {};
	Result.TabLength = 2;
	return Result;
}

internal void
EndTabLayout(tab_layout *Layout)
{
	Assert(Layout->Tabs == 0);
}

internal void
BeginIndent(tab_layout *Layout)
{
	Layout->Tabs++;
}

internal void
EndIndent(tab_layout *Layout)
{
	Assert(Layout->Tabs != 0);
	Layout->Tabs--;
}

internal void
TabPrint(tab_layout *Layout, string Text)
{
	for(u32 Index = 0;
		Index < Layout->Tabs;
		++Index)
	{
		for(u32 SpaceIndex = 0;
			SpaceIndex < Layout->TabLength;
			++SpaceIndex)
		{
			Print(ConstZ(" "));
		}
	}

	Printf("%s\n", Text);
}

internal void
TabPrintf(tab_layout *Layout, char *Format, ...)
{
	char Buffer[1024] = {};
	va_list ArgList;
	va_start(ArgList, Format);
	string String = FormatStringList(Buffer, ArrayCount(Buffer), Format, ArgList);
	va_end(ArgList);
	TabPrint(Layout, String);
}

internal void
DumpSLA_v3(memory_region *Region, buffer FileData)
{
	header_sla_v3 *Header = (header_sla_v3 *)(FileData.Contents + 0);

	tab_layout Layout = BeginTabLayout();
	TabPrint(&Layout, ConstZ("Header:"));
	BeginIndent(&Layout);
	b32 MagicCodeMatches = (Header->Header.MagicCode == MAGIC_CODE);
	TabPrintf(&Layout, "MagicCode : %d %s", Header->Header.MagicCode, MagicCodeMatches ? WrapZ("MATCH") : WrapZ("NO MATCH"));
	TabPrintf(&Layout, "Version : %d", Header->Header.Version);
	TabPrintf(&Layout, "MeshCount : %d", Header->MeshCount);
	TabPrintf(&Layout, "TextureCount : %d", Header->TextureCount);
	TabPrintf(&Layout, "FontCount : %d", Header->FontCount);
	TabPrintf(&Layout, "EntityInfoCount : %d", Header->EntityInfoCount);

	TabPrintf(&Layout, "DataOffset : %d", Header->DataOffset);
	TabPrintf(&Layout, "DataSize : %d", Header->DataSize);

	TabPrintf(&Layout, "MetaDataOffset : %d", Header->MetaDataOffset);
	TabPrintf(&Layout, "MetaDataSize : %d", Header->MetaDataSize);

	TabPrintf(&Layout, "AssetHeadersOffset : %d", Header->AssetHeadersOffset);
	TabPrintf(&Layout, "AssetHeadersSize : %d", Header->AssetHeadersSize);

	u32 TotalAssetCount = Header->TextureCount + Header->FontCount + Header->MeshCount + Header->EntityInfoCount;
	TabPrintf(&Layout, "Total Assets : %d", TotalAssetCount);
	EndIndent(&Layout);

	TabPrint(&Layout, WrapZ("Assets:"));
	BeginIndent(&Layout);

	asset_header_sla_v3 *AssetHeaders = (asset_header_sla_v3 *)(FileData.Contents + Header->AssetHeadersOffset);
	u8 *MetaData = FileData.Contents + Header->MetaDataOffset;
	u8 *Data = FileData.Contents + Header->DataOffset;

	u32 AssetArraySize = (TotalAssetCount*sizeof(asset_header_sla_v3));
	if(Header->AssetHeadersSize == AssetArraySize)
	{
		for(u32 AssetIndex = 0;
			AssetIndex < TotalAssetCount;
			++AssetIndex)
		{
			asset_header_sla_v3 *AssetHeader = AssetHeaders + AssetIndex;
			TabPrint(&Layout, WrapZ("AssetHeader:"));
			BeginIndent(&Layout);
			TabPrintf(&Layout, "Type : %s %d", WrapZ(Names_asset_type[AssetHeader->Type]), AssetHeader->Type);
			TabPrintf(&Layout, "Name : %s %d", WrapZ(Names_asset_name[AssetHeader->Name]), AssetHeader->Name);
			TabPrintf(&Layout, "MetaDataOffset : %d", AssetHeader->MetaDataOffset);
			TabPrintf(&Layout, "MetaDataSize : %d", AssetHeader->MetaDataSize);
			TabPrintf(&Layout, "DataOffset : %d", AssetHeader->DataOffset);
			TabPrintf(&Layout, "DataSize : %d", AssetHeader->DataSize);

			switch(AssetHeader->Type)
			{
				case AssetType_Bitmap:
				{
					TabPrint(&Layout, WrapZ("Bitmap:"));
					BeginIndent(&Layout);
					bitmap_sla_v3 *Bitmap = (bitmap_sla_v3 *)(MetaData + AssetHeader->MetaDataOffset);
					if(AssetHeader->MetaDataSize != sizeof(bitmap_sla_v3))
					{
						TabPrintf(&Layout, "Error: Wrong metadata size! (Should be %d, but was %d)",
							sizeof(bitmap_sla_v3), AssetHeader->MetaDataSize);
					}

					TabPrintf(&Layout, "Width : %d", Bitmap->Width);
					TabPrintf(&Layout, "Height : %d", Bitmap->Height);

					u32 DataSize = Bitmap->Width*Bitmap->Height*sizeof(u32);
					if(AssetHeader->DataSize != DataSize)
					{
						TabPrintf(&Layout, "Error: Wrong data size! (Should be %d, but was %d)",
							DataSize, AssetHeader->DataSize);
					}

					EndIndent(&Layout);
				} break;

				case AssetType_Font:
				{
					TabPrint(&Layout, WrapZ("Font:"));
					BeginIndent(&Layout);
					font_sla_v3 *Font = (font_sla_v3 *)(MetaData + AssetHeader->MetaDataOffset);
					if(AssetHeader->MetaDataSize != sizeof(font_sla_v3))
					{
						TabPrintf(&Layout, "Error: Wrong metadata size! (Should be %d, but was %d)",
							sizeof(font_sla_v3), AssetHeader->MetaDataSize);
					}

					TabPrintf(&Layout, "Scale : %f", Font->Scale);
					TabPrintf(&Layout, "Ascent : %f", Font->Ascent);
					TabPrintf(&Layout, "Descent : %f", Font->Descent);
					TabPrintf(&Layout, "Linegap : %f", Font->Linegap);
					TabPrintf(&Layout, "Apron : %d", Font->Apron);

					// v2 Offset[128];
					// f32 Advance[128];
					// f32 Kerning[128][128];
					// v2 TexCoordMin[128];
					// v2 TexCoordMax[128];

					asset_header_sla_v3 *BitmapHeader = &Font->BitmapHeader;
					TabPrint(&Layout, WrapZ("BitmapHeader:"));
					BeginIndent(&Layout);
					TabPrintf(&Layout, "Type : %s %d", WrapZ(Names_asset_type[BitmapHeader->Type]), BitmapHeader->Type);
					TabPrintf(&Layout, "Name : %s %d", WrapZ(Names_asset_name[BitmapHeader->Name]), BitmapHeader->Name);
					TabPrintf(&Layout, "MetaDataOffset : %d", BitmapHeader->MetaDataOffset);
					TabPrintf(&Layout, "MetaDataSize : %d", BitmapHeader->MetaDataSize);
					if(BitmapHeader->MetaDataSize != sizeof(bitmap_sla_v3))
					{
						TabPrintf(&Layout, "Error: Wrong metadata size! (Should be %d, but was %d)",
							sizeof(bitmap_sla_v3), BitmapHeader->MetaDataSize);
					}

					TabPrintf(&Layout, "DataOffset : %d", BitmapHeader->DataOffset);
					TabPrintf(&Layout, "DataSize : %d", BitmapHeader->DataSize);

					bitmap_sla_v3 *BitmapSLA = &Font->BitmapSLA;
					TabPrint(&Layout, WrapZ("FontBitmap:"));
					BeginIndent(&Layout);
					TabPrintf(&Layout, "Width : %d", BitmapSLA->Width);
					TabPrintf(&Layout, "Height : %d", BitmapSLA->Height);

					u32 DataSize = BitmapSLA->Width*BitmapSLA->Height*sizeof(u32);
					if(BitmapHeader->DataSize != DataSize)
					{
						TabPrintf(&Layout, "Error: Wrong data size! (Should be %d, but was %d)",
							DataSize, BitmapHeader->DataSize);
					}

					EndIndent(&Layout);
					EndIndent(&Layout);

					EndIndent(&Layout);
				} break;

				case AssetType_Mesh:
				{
					TabPrint(&Layout, WrapZ("Mesh:"));
					BeginIndent(&Layout);
					mesh_sla_v3 *Mesh = (mesh_sla_v3 *)(MetaData + AssetHeader->MetaDataOffset);
					if(AssetHeader->MetaDataSize != sizeof(mesh_sla_v3))
					{
						TabPrintf(&Layout, "Error: Wrong metadata size! (Should be %d, but was %d)",
							sizeof(mesh_sla_v3), AssetHeader->MetaDataSize);
					}

					TabPrintf(&Layout, "VertexCount : %d", Mesh->VertexCount);
					TabPrintf(&Layout, "FaceCount : %d", Mesh->FaceCount);
					TabPrintf(&Layout, "VerticesSize : %d", Mesh->VerticesSize);
					TabPrintf(&Layout, "FacesSize : %d", Mesh->FacesSize);
					u32 DataSize = Mesh->VertexCount*sizeof(vertex_format) + Mesh->FaceCount*sizeof(v3u);
					if(AssetHeader->DataSize != DataSize)
					{
						TabPrintf(&Layout, "Error: Wrong data size! (Should be %d, but was %d)",
							DataSize, AssetHeader->DataSize);
					}

					EndIndent(&Layout);
				} break;

				case AssetType_EntityInfo:
				{
					TabPrint(&Layout, WrapZ("EntityInfo:"));
					BeginIndent(&Layout);
					entity_info *EntityInfo = (entity_info *)(MetaData + AssetHeader->MetaDataOffset);
					if(AssetHeader->MetaDataSize != sizeof(entity_info))
					{
						TabPrintf(&Layout, "Error: Wrong metadata size! (Should be %d, but was %d)",
							sizeof(entity_info), AssetHeader->MetaDataSize);
					}

					TabPrintf(&Layout, "Type : %s", WrapZ(Names_entity_type[EntityInfo->Type]));
					string DisplayName = EntityInfo->DisplayName;
					DisplayName.Text = EntityInfo->Buffer;
					TabPrintf(&Layout, "DisplayName : %s", DisplayName);
					TabPrintf(&Layout, "PieceCount : %d", EntityInfo->PieceCount);
					TabPrintf(&Layout, "Color : %f, %f, %f, %f",
						EntityInfo->Color.r,
						EntityInfo->Color.g,
						EntityInfo->Color.b,
						EntityInfo->Color.a);
					EndIndent(&Layout);
				} break;

				default:
				{

				} break;
			}
			EndIndent(&Layout);
		}
	}
	else
	{
		TabPrintf(&Layout, "Asset header array is the wrong size! (Should be %d, but was %d)",
			AssetArraySize, Header->AssetHeadersSize);
	}
	EndIndent(&Layout);

	EndTabLayout(&Layout);
}

internal void
DumpSLA_v2(buffer FileData)
{
	header_sla_v2 *Header = (header_sla_v2 *)(FileData.Contents + 0);

	tab_layout Layout = BeginTabLayout();
	TabPrint(&Layout, ConstZ("Header:"));
	BeginIndent(&Layout);
	b32 MagicCodeMatches = (Header->Header.MagicCode == MAGIC_CODE);
	TabPrintf(&Layout, "MagicCode : %d %s", Header->Header.MagicCode, MagicCodeMatches ? WrapZ("MATCH") : WrapZ("NO MATCH"));
	TabPrintf(&Layout, "Version : %d", Header->Header.Version);
	TabPrintf(&Layout, "MeshCount : %d", Header->MeshCount);
	TabPrintf(&Layout, "TextureCount : %d", Header->TextureCount);
	TabPrintf(&Layout, "FontCount : %d", Header->FontCount);
	TabPrintf(&Layout, "DataOffset : %d", Header->DataOffset);

	u32 TotalAssetCount = 1 +
						  Header->TextureCount +
						  Header->FontCount +
						  Header->MeshCount;
	TabPrintf(&Layout, "Total Assets : %d", TotalAssetCount);
	EndIndent(&Layout);

	TabPrint(&Layout, WrapZ("Bitmaps:"));
	BeginIndent(&Layout);
	bitmap_sla_v2 *AllBitmapData = (bitmap_sla_v2 *)(FileData.Contents + sizeof(header_sla_v2));
	for(u32 TextureIndex = 0;
		TextureIndex < Header->TextureCount;
		++TextureIndex)
	{
		bitmap_sla_v2 *BitmapData = AllBitmapData + TextureIndex;
		TabPrintf(&Layout, "Name : %s %d", WrapZ(Names_asset_name[BitmapData->Name]), BitmapData->Name);
		BeginIndent(&Layout);
		TabPrintf(&Layout, "Width : %d", BitmapData->Width);
		TabPrintf(&Layout, "Height : %d", BitmapData->Height);
		TabPrintf(&Layout, "PixelsSize : %d", BitmapData->PixelsSize);
		TabPrintf(&Layout, "PixelsOffset : %d", BitmapData->PixelsOffset);
		EndIndent(&Layout);
	}
	EndIndent(&Layout);

	TabPrint(&Layout, WrapZ("Fonts:"));
	BeginIndent(&Layout);
	font_sla_v2 *AllFontData = (font_sla_v2 *)(FileData.Contents + sizeof(header_sla_v2) + Header->TextureCount*sizeof(bitmap_sla_v2));
	for(u32 FontIndex = 0;
		FontIndex < Header->FontCount;
		++FontIndex)
	{
		font_sla_v2 *FontData = AllFontData + FontIndex;
		TabPrintf(&Layout, "Name : %s %d", WrapZ(Names_asset_name[FontData->Name]), FontData->Name);
		BeginIndent(&Layout);
		TabPrintf(&Layout, "Scale : %f", FontData->Scale);
		TabPrintf(&Layout, "Ascent : %f", FontData->Ascent);
		TabPrintf(&Layout, "Descent : %f", FontData->Descent);
		TabPrintf(&Layout, "Linegap : %f", FontData->Linegap);

		// v2 Offset[128];
		// f32 Advance[128];
		// f32 Kerning[128][128];
		// v2 TexCoordMin[128];
		// v2 TexCoordMax[128];

		TabPrintf(&Layout, "Apron : %d", FontData->Apron);

		bitmap_sla_v2 *BitmapSLA = &FontData->BitmapSLA;
		TabPrintf(&Layout, "BitmapName : %s %d", WrapZ(Names_asset_name[BitmapSLA->Name]), BitmapSLA->Name);
		BeginIndent(&Layout);
		TabPrintf(&Layout, "Width : %d", BitmapSLA->Width);
		TabPrintf(&Layout, "Height : %d", BitmapSLA->Height);
		TabPrintf(&Layout, "PixelsSize : %d", BitmapSLA->PixelsSize);
		TabPrintf(&Layout, "PixelsOffset : %d", BitmapSLA->PixelsOffset);
		EndIndent(&Layout);

		EndIndent(&Layout);
	}
	EndIndent(&Layout);

	TabPrint(&Layout, WrapZ("Meshes:"));
	BeginIndent(&Layout);
	mesh_sla_v2 *AllMeshData = (mesh_sla_v2 *)(FileData.Contents + sizeof(header_sla_v2) + Header->TextureCount*sizeof(bitmap_sla_v2) + Header->FontCount*sizeof(font_sla_v2));
	for(u32 MeshIndex = 0;
		MeshIndex < Header->MeshCount;
		++MeshIndex)
	{
		mesh_sla_v2 *MeshData = AllMeshData + MeshIndex;
		TabPrintf(&Layout, "Name : %s %d", WrapZ(Names_asset_name[MeshData->Name]), MeshData->Name);
		BeginIndent(&Layout);
		TabPrintf(&Layout, "VertexCount : %d", MeshData->VertexCount);
		TabPrintf(&Layout, "VerticesOffset : %d", MeshData->VerticesOffset);
		TabPrintf(&Layout, "VerticesSize : %d", MeshData->VerticesSize);
		TabPrintf(&Layout, "FaceCount : %d", MeshData->FaceCount);
		TabPrintf(&Layout, "FacesOffset : %d", MeshData->FacesOffset);
		TabPrintf(&Layout, "FacesSize : %d", MeshData->FacesSize);
		EndIndent(&Layout);
	}
	EndIndent(&Layout);

	EndTabLayout(&Layout);
}

internal void
DumpSLA(string Filename)
{
	memory_region Region = {};
	temporary_memory TempMem = BeginTemporaryMemory(&Region);
	buffer FileData = Platform->LoadEntireFile(&Region, Filename);
	if(FileData.Size)
	{
		header_sla *Header = (header_sla *)(FileData.Contents + 0);

		switch(Header->Version)
		{
			case 2:
			{
				DumpSLA_v2(FileData);
			} break;

			case 3:
			{
				DumpSLA_v3(&Region, FileData);
			} break;

			default:
			{
				Printf("Can't open file (%s), bad version (%d)", Filename, Header->Version);
			}
		}


	}

	EndTemporaryMemory(&TempMem);
}

internal void
CreateEmptySLA(string Filename)
{
	header_sla_v3 Header = {};
	Header.Header.MagicCode = MAGIC_CODE;
	Header.Header.Version = 3;

	Header.DataOffset = sizeof(header_sla_v3);
	Header.DataSize = 0;
	Header.MetaDataOffset = sizeof(header_sla_v3);
	Header.MetaDataSize = 0;
	Header.AssetHeadersOffset = sizeof(header_sla_v3);
	Header.AssetHeadersSize = 0;

	b32 FileWritten = Win32WriteEntireFile(&Header, sizeof(Header), Filename);
}

s32 main(s32 ArgCount, char **ArgsCString)
{
	platform Plat = {};
	Plat.AllocateMemory = Win32AllocateMemory;
	Plat.DeallocateMemory = Win32DeallocateMemory;
	Plat.LoadEntireFile = Win32LoadEntireFile;
	Platform = &Plat;

	string Args[16];
	ParseArgs(ArgCount, ArgsCString, Args);

	b32 DisplayUsage = true;
	if(ArgCount > 1)
	{
		if(StringCompare(Args[1], ConstZ("-dump")))
		{
			if(ArgCount == 3)
			{
				DisplayUsage = false;
				string Filename = Args[2];
				DumpSLA(Filename);
			}
		}
		else if(StringCompare(Args[1], ConstZ("-create")))
		{
			if(ArgCount == 3)
			{
				DisplayUsage = false;
				string Filename = Args[2];
				CreateEmptySLA(Filename);
			}
		}
	}

	if(DisplayUsage)
	{
		Print(ConstZ("slaedit usage:\n"));
		Print(ConstZ("    slaedit -dump [Filename]\n"));
		Print(ConstZ("        dump the contents of a .sla file [Filename] in a readable format.\n"));
		Print(ConstZ("    slaedit -create [Filename]\n"));
		Print(ConstZ("        Create a new and empty .sla file [Filename].\n"));
	}
}
