/*@H
* File: simple_meta.cpp
* Author: Jesse Calvert
* Created: October 10, 2017, 16:18
* Last modified: April 10, 2019, 17:31
*/

#undef GAME_DEBUG
#include "steelth_types.h"
#include "steelth_platform.h"
#undef Assert
#define Assert(Expression) if(!(Expression)) *(int *)0 = 0

#include "steelth_shared.h"
#include "steelth_memory.h"
#include "simple_meta.h"

#include "stdio.h"

ALLOCATE_MEMORY(CRTAllocate)
{
	umm TotalSize = Size + sizeof(platform_memory_block);
	umm BaseOffset = sizeof(platform_memory_block);
	platform_memory_block *NewBlock = (platform_memory_block *)calloc(1, TotalSize);
	Assert(NewBlock);
	NewBlock->Base = (u8 *)NewBlock + BaseOffset;
	NewBlock->Size = Size;

	return NewBlock;
}

DEALLOCATE_MEMORY(CRTDeallocate)
{
	if(Block)
	{
		free(Block);
	}
}

umm CRTGetFileSize(FILE *File)
{
	umm Result = 0;
	s32 CurrentPosition = ftell(File);
	s32 Error = fseek(File, 0, SEEK_SET);
	if(Error == 0)
	{
		umm Beginning = ftell(File);
		Error = fseek(File, 0, SEEK_END);
		if(Error == 0)
		{
			umm End = ftell(File);
			Result = (End - Beginning);
		}
	}

	fseek(File, CurrentPosition, SEEK_SET);
	return Result;
}

LOAD_ENTIRE_FILE_AND_NULL_TERMINATE(CRTLoadTextFileAndNullTerminate)
{
	buffer Result = {};

	FILE *File = 0;
	errno_t Error = fopen_s(&File, Filename.Text, "rb");
	if(Error == 0)
	{
		Result.Size = CRTGetFileSize(File);
		if(Result.Size)
		{
			Result.Contents = PushArray(Region, Result.Size + 1, u8);
			umm TotalRead = 0;

			// while(TotalRead < Result.Size)
			{
				TotalRead += fread(Result.Contents + TotalRead, 1, Result.Size - TotalRead, File);
			}
			Assert(TotalRead == Result.Size);
		}

		fclose(File);
	}

	return Result;
}

internal b32
IsComplexStruct(state *State, string StructName)
{
	b32 Result = true;
	for(string_list_node *SpecialStruct = State->SpecialStructNames.Next;
		Result && (SpecialStruct != &State->SpecialStructNames);
		SpecialStruct = SpecialStruct->Next)
	{
		Result = !StringCompare(SpecialStruct->String, StructName);
	}

	return Result;
}

internal void
CRTWriteGeneratedCode(state *State, string DestFilename)
{
	char FilenameBuffer[256];
		string Path = FormatString(FilenameBuffer, ArrayCount(FilenameBuffer), "%s\\%s",
			State->CWD,
			DestFilename);

	FILE *File = 0;
	errno_t Error = fopen_s(&File, Path.Text, "w");
	if(Error == 0)
	{
		umm CountWritten = 0;
		char Buffer[256];

		//
		// NOTE: File header.
		//

		string FileHeader = FormatString(Buffer, ArrayCount(Buffer),
R"MULTILINE(/*@H
* File: %s
* Author: simple_meta.exe
* Created: November 27, 2017, 11:00
* Last modified: November 27, 2017, 11:00
*/

#pragma once

#define MAX_CIRCULAR_REFERENCE_DEPTH 2
)MULTILINE", DestFilename);
		CountWritten = fwrite(FileHeader.Text, FileHeader.Length, 1, File);
		Assert(CountWritten == 1);

		//
		// NOTE: Includes.
		//

		for(string_list_node *SourceNode = State->SourceSentinel.Next;
			SourceNode != &State->SourceSentinel;
			SourceNode = SourceNode->Next)
		{
			string Filename = SourceNode->String;
			if(Filename.Text[Filename.Length - 1] == 'h')
			{
				string Includes = FormatString(Buffer, ArrayCount(Buffer),
R"MULTILINE(
#include "%s")MULTILINE",
				Filename);
				CountWritten = fwrite(Includes.Text, Includes.Length, 1, File);
				Assert(CountWritten == 1);
			}
		}

		CountWritten = fwrite("\n", 1, 1, File);
		Assert(CountWritten == 1);

		//
		// NOTE: Enum names.
		//

		for(enum_information_list_node *Node = State->EnumInfoSentinel.Prev;
			Node != &State->EnumInfoSentinel;
			Node = Node->Prev)
		{
			enum_information EnumInfo = Node->EnumInfo;

			if(!EnumInfo.IsFlags)
			{
				string Header = FormatString(Buffer, ArrayCount(Buffer),
R"MULTILINE(
global char * Names_%s[] =
{)MULTILINE",
				EnumInfo.Name);
				CountWritten = fwrite(Header.Text, Header.Length, 1, File);
				Assert(CountWritten == 1);

				for(u32 MemberIndex = 0;
					MemberIndex < EnumInfo.EnumCount;
					++MemberIndex)
				{
					enum_member *Member = EnumInfo.Enums + MemberIndex;
					Assert(Member->Value == MemberIndex);
					string String = FormatString(Buffer, ArrayCount(Buffer),
						"\n    \"%s\",",
						Member->Name);
					CountWritten = fwrite(String.Text, String.Length, 1, File);
					Assert(CountWritten == 1);
				}

				string Footer = FormatString(Buffer, ArrayCount(Buffer),
R"MULTILINE(
};
)MULTILINE");
				CountWritten = fwrite(Footer.Text, Footer.Length, 1, File);
				Assert(CountWritten == 1);
			}
		}

		//
		// NOTE: Function headers.
		//

		for(struct_information_list_node *Node = State->StructInfoSentinel.Prev;
			Node != &State->StructInfoSentinel;
			Node = Node->Prev)
		{
			struct_information StructInfo = Node->StructInfo;

			b32 IsNewStruct = IsComplexStruct(State, StructInfo.Name);
			if(IsNewStruct)
			{
				string Header = FormatString(Buffer, ArrayCount(Buffer),
R"MULTILINE(
inline void RecordDebugValue_(%s *ValuePtr, char *GUID, u32 Depth=0);)MULTILINE",
				StructInfo.Name);
				CountWritten = fwrite(Header.Text, Header.Length, 1, File);
				Assert(CountWritten == 1);
			}
		}

		//
		// NOTE: Function bodies.
		//

		CountWritten = fwrite("\n", 1, 1, File);
		Assert(CountWritten == 1);

		for(enum_information_list_node *Node = State->EnumInfoSentinel.Prev;
			Node != &State->EnumInfoSentinel;
			Node = Node->Prev)
		{
			enum_information EnumInfo = Node->EnumInfo;

			if(EnumInfo.IsFlags)
			{
				string Header = FormatString(Buffer, ArrayCount(Buffer),
R"MULTILINE(
inline void RecordDebugValue_(%s *ValuePtr, char *GUID)
{
	u32 *SimpleValue = (u32 *)ValuePtr;
	DEBUG_VALUE_GUID(SimpleValue, GUID);
}
)MULTILINE",
				EnumInfo.Name);
				CountWritten = fwrite(Header.Text, Header.Length, 1, File);
				Assert(CountWritten == 1);
			}
			else
			{
				string Header = FormatString(Buffer, ArrayCount(Buffer),
R"MULTILINE(
inline void RecordDebugValue_(%s *ValuePtr, char *GUID)
{
	switch(*ValuePtr)
	{)MULTILINE",
				EnumInfo.Name);
				CountWritten = fwrite(Header.Text, Header.Length, 1, File);
				Assert(CountWritten == 1);

				for(u32 MemberIndex = 0;
					MemberIndex < EnumInfo.EnumCount;
					++MemberIndex)
				{
					enum_member *Member = EnumInfo.Enums + MemberIndex;
					string String = FormatString(Buffer, ArrayCount(Buffer),
						"\n        case %d: {RECORD_DEBUG_EVENT(DEBUG_NAME(\"%s\"), Event_enum);} break;",
						Member->Value,
						Member->Name);
					CountWritten = fwrite(String.Text, String.Length, 1, File);
					Assert(CountWritten == 1);
				}

				string Footer = FormatString(Buffer, ArrayCount(Buffer),
R"MULTILINE(
	}
}
)MULTILINE",
				EnumInfo.Name);
				CountWritten = fwrite(Footer.Text, Footer.Length, 1, File);
				Assert(CountWritten == 1);
			}
		}

		for(struct_information_list_node *Node = State->StructInfoSentinel.Prev;
			Node != &State->StructInfoSentinel;
			Node = Node->Prev)
		{
			struct_information StructInfo = Node->StructInfo;

			b32 IsNewStruct = IsComplexStruct(State, StructInfo.Name);
			if(IsNewStruct)
			{
				string Header = FormatString(Buffer, ArrayCount(Buffer),
R"MULTILINE(

inline void
RecordDebugValue_(%s *ValuePtr, char *GUID, u32 Depth)
{
	if(Depth > MAX_CIRCULAR_REFERENCE_DEPTH) return;)MULTILINE",
				StructInfo.Name);
				CountWritten = fwrite(Header.Text, Header.Length, 1, File);
				Assert(CountWritten == 1);

				for(u32 MemberIndex = 0;
					MemberIndex < StructInfo.MemberCount;
					++MemberIndex)
				{
					member *Member = StructInfo.Members + MemberIndex;
					// if(!StringCompare(Member->Type, StructInfo.Name))
					{
						b32 IsComplex = IsComplexStruct(State, Member->Type);
						if(IsComplex)
						{
							string DataBlock = FormatString(Buffer, ArrayCount(Buffer),
								"\n    {DEBUG_DATA_BLOCK(\"%s\");", Member->Name);
							CountWritten = fwrite(DataBlock.Text, DataBlock.Length, 1, File);
							Assert(CountWritten == 1);

							string DebugValueMember;
							if(Member->IsPointer)
							{
								DebugValueMember = FormatString(Buffer, ArrayCount(Buffer),
									"\n        if(ValuePtr->%s) {RecordDebugValue_(ValuePtr->%s, DEBUG_NAME(\"%s\"), Depth+1);}\n    }", Member->Name, Member->Name, Member->Name, Member->Name);
							}
							else
							{
								DebugValueMember = FormatString(Buffer, ArrayCount(Buffer), "\n        DEBUG_VALUE_NAME(&ValuePtr->%s, \"%s\");\n    }", Member->Name, Member->Name);
							}

							CountWritten = fwrite(DebugValueMember.Text, DebugValueMember.Length, 1, File);
							Assert(CountWritten == 1);
						}
						else
						{
							string DebugValueMember;
							if(Member->IsPointer)
							{
								DebugValueMember = FormatString(Buffer, ArrayCount(Buffer),
									"\n    DEBUG_VALUE_NAME(ValuePtr->%s, \"%s\");", Member->Name, Member->Name, Member->Name);
							}
							else
							{
								DebugValueMember = FormatString(Buffer, ArrayCount(Buffer), "\n    DEBUG_VALUE_NAME(&ValuePtr->%s, \"%s\");", Member->Name, Member->Name);
							}

							CountWritten = fwrite(DebugValueMember.Text, DebugValueMember.Length, 1, File);
							Assert(CountWritten == 1);
						}
					}
				}

				string Footer = FormatString(Buffer, ArrayCount(Buffer),
R"MULTILINE(
})MULTILINE");
				CountWritten = fwrite(Footer.Text, Footer.Length, 1, File);
				Assert(CountWritten == 1);
			}
		}

		fclose(File);
	}
}

internal state *
MakeState()
{
	state *Result = BootstrapPushStruct(state, Region);
	DLIST_INIT(&Result->SourceSentinel);
	DLIST_INIT(&Result->StructInfoSentinel);
	DLIST_INIT(&Result->EnumInfoSentinel);
	DLIST_INIT(&Result->SpecialStructNames);
	return Result;
}

inline b32
ParserNotAtEnd(parser *Parser)
{
	b32 Result = (Parser->Index < Parser->String.Length);
	return Result;
}

inline char
ParserGetChar(parser *Parser, u32 Offset = 0)
{
	char Result = 0;
	if(Parser->Index + Offset < Parser->String.Length)
	{
		Result = Parser->String.Text[Parser->Index + Offset];
	}
	return Result;
}

inline void
EatWhitespace(parser *Parser)
{
	while(ParserNotAtEnd(Parser) && IsWhitespace(ParserGetChar(Parser)))
	{
		++Parser->Index;
	}
}

inline u32
ParserEatUntil(parser *Parser, char Character)
{
	u32 Result = 0;
	while(ParserNotAtEnd(Parser) && (ParserGetChar(Parser) != Character))
	{
		++Parser->Index;
		++Result;
	}

	return Result;
}

inline u32
ParserEatPreprocessorDirective(parser *Parser)
{
	u32 Result = 0;
	char LastNonWhitespaceCharOnLine = 0;
	while(ParserNotAtEnd(Parser))
	{
		if(!IsWhitespace(ParserGetChar(Parser)))
		{
			LastNonWhitespaceCharOnLine = ParserGetChar(Parser);
		}
		if(ParserGetChar(Parser) == '\n')
		{
			if(LastNonWhitespaceCharOnLine != '\\')
			{
				break;
			}
		}

		++Parser->Index;
		++Result;
	}
	return Result;
}

inline u32
EatNumber(parser *Parser)
{
	u32 Result = 0;

	b32 IsHex = ((ParserGetChar(Parser, 0) == '0') &&
				 ((ParserGetChar(Parser, 1) == 'x') || (ParserGetChar(Parser, 1) == 'X')));
	if(IsHex)
	{
		Parser->Index += 2;
		Result += 2;

		while(IsHexNumber(ParserGetChar(Parser)))
		{
			++Parser->Index;
			++Result;
		}
	}
	else
	{
		while(IsDecimalNumber(ParserGetChar(Parser)))
		{
			++Parser->Index;
			++Result;
		}

		if(ParserGetChar(Parser) == '.')
		{
			++Parser->Index;
			++Result;

			while(IsDecimalNumber(ParserGetChar(Parser)))
			{
				++Parser->Index;
				++Result;
			}

			if(ParserGetChar(Parser) == 'f')
			{
				++Parser->Index;
				++Result;
			}
		}
	}

	return Result;
}

inline u32
EatIdentifier(parser *Parser)
{
	u32 Result = 0;
	Assert((ParserGetChar(Parser) == '_') || IsAlpha(ParserGetChar(Parser)));
	++Parser->Index;
	++Result;

	while((ParserGetChar(Parser) == '_') ||
		  IsAlpha(ParserGetChar(Parser)) ||
		  IsDecimalNumber(ParserGetChar(Parser)))
	{
		++Parser->Index;
		++Result;
	}

	return Result;
}

#define TOKEN_CASE(Character, type, TokenLen) \
case Character: \
{ \
	Result.Type = type; \
	Result.String.Text = Parser->String.Text + Parser->Index; \
	Result.String.Length = TokenLen; \
	Parser->Index += TokenLen; \
} break

inline token
GetNextToken(parser *Parser)
{
	token Result = {};

	EatWhitespace(Parser);

	if(Parser->Index < Parser->String.Length)
	{
		switch(ParserGetChar(Parser))
		{
			TOKEN_CASE('(', Token_OpenParen, 1);
			TOKEN_CASE(')', Token_CloseParen, 1);
			TOKEN_CASE('[', Token_OpenBracket, 1);
			TOKEN_CASE(']', Token_CloseBracket, 1);
			TOKEN_CASE('{', Token_OpenBrace, 1);
			TOKEN_CASE('}', Token_CloseBrace, 1);
			TOKEN_CASE(';', Token_SemiColon, 1);
			TOKEN_CASE(':', Token_Colon, 1);
			TOKEN_CASE(',', Token_Comma, 1);
			TOKEN_CASE('?', Token_QuestionMark, 1);
			TOKEN_CASE('~', Token_BitwiseNot, 1);
			TOKEN_CASE('^', Token_BitwiseXor, 1);
			TOKEN_CASE('%', Token_Mod, 1);

			case '#':
			{
				Result.Type = Token_PreprocessorDirective;
				Result.String.Text = Parser->String.Text + Parser->Index;
				Result.String.Length = ParserEatPreprocessorDirective(Parser);
			} break;

			case '=':
			{
				switch(ParserGetChar(Parser, 1))
				{
					TOKEN_CASE('=', Token_EqualsEquals, 2);
					default:
					{
						Result.Type = Token_Equals;
						Result.String.Text = Parser->String.Text + Parser->Index;
						Result.String.Length = 1;
						++Parser->Index;
					}
				}
			} break;

			case '+':
			{
				switch(ParserGetChar(Parser, 1))
				{
					TOKEN_CASE('=', Token_PlusEquals, 2);
					TOKEN_CASE('+', Token_PlusPlus, 2);
					default:
					{
						Result.Type = Token_Plus;
						Result.String.Text = Parser->String.Text + Parser->Index;
						Result.String.Length = 1;
						++Parser->Index;
					}
				}
			} break;

			case '-':
			{
				switch(ParserGetChar(Parser, 1))
				{
					TOKEN_CASE('=', Token_MinusEquals, 2);
					TOKEN_CASE('-', Token_MinusMinus, 2);
					TOKEN_CASE('>', Token_Arrow, 2);
					default:
					{
						Result.Type = Token_Minus;
						Result.String.Text = Parser->String.Text + Parser->Index;
						Result.String.Length = 1;
						++Parser->Index;
					}
				}
			} break;

			case '/':
			{
				switch(ParserGetChar(Parser, 1))
				{
					TOKEN_CASE('=', Token_DivideEquals, 2);

					case '/':
					{
						Result.Type = Token_Comment;
						Result.String.Text = Parser->String.Text + Parser->Index;
						Result.String.Length = ParserEatUntil(Parser, '\n');
					} break;

					case '*':
					{
						Result.Type = Token_Comment;
						Result.String.Text = Parser->String.Text + Parser->Index;
						Parser->Index += 2;
						Result.String.Length = 2;
						do
						{
							Result.String.Length += ParserEatUntil(Parser, '*') + 1;
							++Parser->Index;
						} while(ParserGetChar(Parser, 0) != '/');

						++Parser->Index;
						++Result.String.Length;
					} break;

					default:
					{
						Result.Type = Token_Divide;
						Result.String.Text = Parser->String.Text + Parser->Index;
						Result.String.Length = 1;
						++Parser->Index;
					}
				}
			} break;

			case '*':
			{
				switch(ParserGetChar(Parser, 1))
				{
					TOKEN_CASE('=', Token_TimesEquals, 2);
					default:
					{
						Result.Type = Token_Star;
						Result.String.Text = Parser->String.Text + Parser->Index;
						Result.String.Length = 1;
						++Parser->Index;
					}
				}
			} break;

			case '!':
			{
				switch(ParserGetChar(Parser, 1))
				{
					TOKEN_CASE('=', Token_NotEquals, 2);
					default:
					{
						Result.Type = Token_Not;
						Result.String.Text = Parser->String.Text + Parser->Index;
						Result.String.Length = 1;
						++Parser->Index;
					}
				}
			} break;

			case '<':
			{
				switch(ParserGetChar(Parser, 1))
				{
					TOKEN_CASE('=', Token_LessThanOrEqual, 2);
					TOKEN_CASE('<', Token_BitwiseShiftLeft, 2);
					default:
					{
						Result.Type = Token_LessThan;
						Result.String.Text = Parser->String.Text + Parser->Index;
						Result.String.Length = 1;
						++Parser->Index;
					}
				}
			} break;

			case '>':
			{
				switch(ParserGetChar(Parser, 1))
				{
					TOKEN_CASE('=', Token_GreaterThanOrEqual, 2);
					TOKEN_CASE('>', Token_BitwiseShiftRight, 2);
					default:
					{
						Result.Type = Token_GreaterThan;
						Result.String.Text = Parser->String.Text + Parser->Index;
						Result.String.Length = 1;
						++Parser->Index;
					}
				}
			} break;

			case '.':
			{
				if((ParserGetChar(Parser, 1) == '.') &&
				   (ParserGetChar(Parser, 2) == '.'))
				{
					Result.Type = Token_Ellipsis;
					Result.String.Text = Parser->String.Text + Parser->Index;
					Result.String.Length = 3;
					Parser->Index += 3;
				}
				else
				{
					if(IsDecimalNumber(ParserGetChar(Parser, 1)))
					{
						Result.Type = Token_Number;
						Result.String.Text = Parser->String.Text + Parser->Index;
						Result.String.Length += EatNumber(Parser);
					}
					else
					{
						Result.Type = Token_Dot;
						Result.String.Text = Parser->String.Text + Parser->Index;
						Result.String.Length = 1;
						++Parser->Index;
					}
				}
			} break;

			case '|':
			{
				switch(ParserGetChar(Parser, 1))
				{
					TOKEN_CASE('|', Token_Or, 2);
					default:
					{
						Result.Type = Token_BitwiseOr;
						Result.String.Text = Parser->String.Text + Parser->Index;
						Result.String.Length = 1;
						++Parser->Index;
					}
				}
			} break;

			case '&':
			{
				switch(ParserGetChar(Parser, 1))
				{
					TOKEN_CASE('&', Token_And, 2);
					default:
					{
						Result.Type = Token_BitwiseAnd;
						Result.String.Text = Parser->String.Text + Parser->Index;
						Result.String.Length = 1;
						++Parser->Index;
					}
				}
			} break;

			case '"':
			{
				Result.Type = Token_String;
				Result.String.Text = Parser->String.Text + Parser->Index;
				++Parser->Index;
				Result.String.Length = ParserEatUntil(Parser, '"') + 2;
				++Parser->Index;
			} break;

			case '\'':
			{
				Result.Type = Token_Character;
				Result.String.Text = Parser->String.Text + Parser->Index;
				++Parser->Index;
				Result.String.Length = ParserEatUntil(Parser, '\'') + 2;
				++Parser->Index;
			} break;

			default:
			{
				if(IsDecimalNumber(ParserGetChar(Parser)))
				{
					Result.Type = Token_Number;
					Result.String.Text = Parser->String.Text + Parser->Index;
					Result.String.Length += EatNumber(Parser);
				}
				else
				{
					Result.Type = Token_Identifier;
					Result.String.Text = Parser->String.Text + Parser->Index;
					Result.String.Length += EatIdentifier(Parser);
				}
			}
		}

	}

	return Result;
}

internal source
LoadSource(state *State, string Filename)
{
	source Result = {};

	char Buffer[512];
	string SourcePath = FormatString(Buffer, ArrayCount(Buffer), "%s\\%s",
		State->CWD,
		Filename);
	buffer SourceCString = Platform->LoadEntireFileAndNullTerminate(&State->Region, SourcePath);
	if(SourceCString.Contents)
	{
		string Source = {};
		Source.Length = (u32)SourceCString.Size;
		Source.Text = (char *)SourceCString.Contents;
		Result.Filename = Filename;
		Result.Code = Source;
	}

	return Result;
}

internal enum_information
ParseEnum(parser *Parser)
{
	enum_information Result = {};

	token EnumIdentifier = GetNextToken(Parser);
	Assert(EnumIdentifier.Type == Token_Identifier);
	Result.Name = EnumIdentifier.String;

	while(GetNextToken(Parser).Type != Token_OpenBrace) {};
	u32 EnumDepth = 1;
	u32 EnumValue = 0;

	while(EnumDepth)
	{
		token EnumToken = GetNextToken(Parser);
		if(EnumToken.Type == Token_OpenBrace)
		{
			++EnumDepth;
		}
		if(EnumToken.Type == Token_CloseBrace)
		{
			--EnumDepth;
		}

		if(EnumToken.Type == Token_Identifier)
		{
			Assert(Result.EnumCount < ArrayCount(Result.Enums));
			enum_member *NewMember = Result.Enums + Result.EnumCount++;
			NewMember->Name = EnumToken.String;

			for(token Token = GetNextToken(Parser);
			 	Token.Type != Token_Comma;
			 	Token = GetNextToken(Parser))
			{
				if(Token.Type == Token_Equals)
				{
					token Number = GetNextToken(Parser);
					Assert(Number.Type == Token_Number);
					EnumValue = StringHexToU32(Number.String.Text);
					Result.IsFlags = true;
				}
			}

			NewMember->Value = EnumValue++;
		}
	}

	return Result;
}

internal struct_information
ParseStruct(parser *Parser)
{
	struct_information Result = {};

	token StructIdentifier = GetNextToken(Parser);

	Assert(StructIdentifier.Type == Token_Identifier);
	Result.Name = StructIdentifier.String;

	while(GetNextToken(Parser).Type != Token_OpenBrace) {};
	u32 StructDepth = 1;

	while(StructDepth)
	{
		token StructToken = GetNextToken(Parser);
		if(StructToken.Type == Token_OpenBrace)
		{
			++StructDepth;
		}
		if(StructToken.Type == Token_CloseBrace)
		{
			--StructDepth;
		}

		if(StructToken.Type == Token_Identifier)
		{
			if(StringStartsWith(StructToken.String, "struct"))
			{

			}
			else if(StringStartsWith(StructToken.String, "union"))
			{

			}
			else
			{
				token MemberToken = GetNextToken(Parser);

				// NOTE: Eat member functions.
				if(MemberToken.Type == Token_OpenParen)
				{
					ParserEatUntil(Parser, '{');
					ParserEatUntil(Parser, '}');
					++Parser->Index;
				}
				else
				{
					b32 IsPointer = false;
					while(MemberToken.Type != Token_Identifier)
					{
						if(MemberToken.Type == Token_Star)
						{
							IsPointer = true;
						}

						MemberToken = GetNextToken(Parser);
					}

					Assert(Result.MemberCount < ArrayCount(Result.Members));
					member *NewMember = Result.Members + Result.MemberCount++;
					NewMember->Name = MemberToken.String;
					NewMember->Type = StructToken.String;

					for(token Token = GetNextToken(Parser);
					 	Token.Type != Token_SemiColon;
					 	Token = GetNextToken(Parser))
					{
						if(Token.Type == Token_OpenBracket)
						{
							IsPointer = true;
						}
						else if(Token.Type == Token_Comma)
						{
							token NextMemberToken = GetNextToken(Parser);
							while(NextMemberToken.Type != Token_Identifier) {NextMemberToken = GetNextToken(Parser);}
							Assert(Result.MemberCount < ArrayCount(Result.Members));
							member *NewMember = Result.Members + Result.MemberCount++;
							NewMember->Name = NextMemberToken.String;
							NewMember->Type = StructToken.String;
							NewMember->IsPointer = IsPointer;
						}
					}

					NewMember->IsPointer = IsPointer;
				}
			}
		}
	}

	return Result;
}

internal void
ParseSource(state *State, string Filename, string DestFilename)
{
	b32 Parsed = false;
	for(string_list_node *Node = State->SourceSentinel.Next;
		!Parsed && (Node != &State->SourceSentinel);
		Node = Node->Next)
	{
		Parsed = StringCompare(Node->String, Filename);
	}

	if(!Parsed)
	{
		source Source = LoadSource(State, Filename);
		if(Source.Code.Length)
		{
			string_list_node *NewNode = PushStruct(&State->Region, string_list_node);
			NewNode->String = Filename;
			DLIST_INSERT(&State->SourceSentinel, NewNode);

			parser Parser = {};
			Parser.String = Source.Code;

			u32 ScopeDepth = 0;
			for(token Token = GetNextToken(&Parser);
				Token.Type != Token_Null;
				Token = GetNextToken(&Parser))
			{
				switch(Token.Type)
				{
					case Token_OpenBrace:
					{
						++ScopeDepth;
					} break;
					case Token_CloseBrace:
					{
						--ScopeDepth;
					} break;

					case Token_Identifier:
					{
						if(StringCompare(Token.String, "INTROSPECT"))
						{
							while(Token.Type != Token_CloseParen) {Token = GetNextToken(&Parser);}
							Token = GetNextToken(&Parser);

							if((ScopeDepth == 0) &&
								(StringCompare(Token.String, "struct") || StringCompare(Token.String, "union")))
							{
								struct_information_list_node *NewNode = PushStruct(&State->Region, struct_information_list_node);
								NewNode->StructInfo = ParseStruct(&Parser);
								DLIST_INSERT(&State->StructInfoSentinel, NewNode);
							}
							else if((ScopeDepth == 0) &&
								(StringCompare(Token.String, "enum")))
							{
								enum_information_list_node *NewNode = PushStruct(&State->Region, enum_information_list_node);
								NewNode->EnumInfo = ParseEnum(&Parser);
								DLIST_INSERT(&State->EnumInfoSentinel, NewNode);
							}
						}
						else if((ScopeDepth == 0) &&
							     StringCompare(Token.String, "REGISTER_DEBUG_VALUE"))
						{
							token Token = GetNextToken(&Parser);
							while(Token.Type != Token_Identifier)
							{
								Token = GetNextToken(&Parser);
							}

							string_list_node *NewNode = PushStruct(&State->Region, string_list_node);
							NewNode->String = Token.String;
							DLIST_INSERT(&State->SpecialStructNames, NewNode);
						}
					} break;

					case Token_PreprocessorDirective:
					{
						if(StringStartsWith(Token.String, "#include"))
						{
							parser DirectiveParser = {};
							DirectiveParser.String = Token.String;
							DirectiveParser.Index = 1;
							for(token DirectiveToken = GetNextToken(&DirectiveParser);
								DirectiveToken.Type != Token_Null;
								DirectiveToken = GetNextToken(&DirectiveParser))
							{
								if(DirectiveToken.Type == Token_String)
								{
									string IncludeFilename = {};
									IncludeFilename.Text = DirectiveToken.String.Text + 1;
									IncludeFilename.Length = DirectiveToken.String.Length - 2;

									if(!StringCompare(IncludeFilename, DestFilename))
									{
										ParseSource(State, IncludeFilename, DestFilename);
									}
								}
							}
						}
					} break;
				}
			}
		}
	}
}

s32 main(s32 ArgumentCount, char **Arguments)
{
	platform Platform_ = {};
	Platform = &Platform_;
	Platform->LoadEntireFileAndNullTerminate = CRTLoadTextFileAndNullTerminate;
	Platform->AllocateMemory = CRTAllocate;
	Platform->DeallocateMemory = CRTDeallocate;

	state *State = MakeState();

	State->CWD = WrapZ(Arguments[1]);
	s32 SourceCount = StringToS32(Arguments[2]);
	string DestFilename = WrapZ(Arguments[3 + SourceCount]);
	Assert(ArgumentCount == (SourceCount + 4));

	for(s32 SourceIndex = 0;
		SourceIndex < SourceCount;
		++SourceIndex)
	{
		string SourceFilename = WrapZ(Arguments[3 + SourceIndex]);
		printf("Preprocessing %s ... \n", Arguments[3 + SourceIndex]);
		ParseSource(State, SourceFilename, DestFilename);
	}

	CRTWriteGeneratedCode(State, DestFilename);
	printf("Done.\n");

	return 0;
}
