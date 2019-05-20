/*@H
* File: simple_meta.h
* Author: Jesse Calvert
* Created: October 10, 2017, 17:29
* Last modified: November 27, 2017, 15:20
*/

#pragma once

/* TODO:
	- Deal with arrays as members of structs.
		- double arrays and arrays of pointers are messed, treated as void *.
*/

enum token_type
{
	Token_Null,

	Token_OpenParen,
	Token_CloseParen,
	Token_OpenBracket,
	Token_CloseBracket,
	Token_OpenBrace,
	Token_CloseBrace,

	Token_Equals,
	Token_PlusEquals,
	Token_MinusEquals,
	Token_TimesEquals,
	Token_DivideEquals,

	Token_SemiColon,
	Token_Comma,
	Token_Dot,
	Token_Arrow,

	Token_Star,
	Token_Plus,
	Token_Minus,
	Token_Divide,
	Token_Mod,
	Token_PlusPlus,
	Token_MinusMinus,

	Token_EqualsEquals,
	Token_NotEquals,
	Token_LessThan,
	Token_LessThanOrEqual,
	Token_GreaterThan,
	Token_GreaterThanOrEqual,

	Token_QuestionMark,
	Token_Colon,
	Token_Ellipsis,

	Token_Not,
	Token_Or,
	Token_And,

	Token_BitwiseOr,
	Token_BitwiseAnd,
	Token_BitwiseNot,
	Token_BitwiseXor,
	Token_BitwiseShiftLeft,
	Token_BitwiseShiftRight,

	Token_String,
	Token_Character,
	Token_Number,
	Token_Identifier,

	Token_PreprocessorDirective,
	Token_Comment,

	Token_Count,
};
struct token
{
	token_type Type;
	string String;
};

struct parser
{
	string String;
	u32 Index;
};

struct source
{
	string Code;
	string Filename;
};

struct member
{
	string Name;
	string Type;
	b32 IsPointer;
};

struct enum_member
{
	string Name;
	u32 Value;
};

#define MAX_STRUCT_MEMBERS 64
struct enum_information
{
	string Name;
	b32 IsFlags;

	u32 EnumCount;
	enum_member Enums[MAX_STRUCT_MEMBERS];
};

struct enum_information_list_node
{
	enum_information_list_node *Next;
	enum_information_list_node *Prev;

	enum_information EnumInfo;
};

struct struct_information
{
	string Name;

	u32 MemberCount;
	member Members[MAX_STRUCT_MEMBERS];
};

struct struct_information_list_node
{
	struct_information_list_node *Next;
	struct_information_list_node *Prev;

	struct_information StructInfo;
};

struct string_list_node
{
	string_list_node *Next;
	string_list_node *Prev;

	string String;
};

struct state
{
	memory_region Region;
	string CWD;
	string_list_node SourceSentinel;
	struct_information_list_node StructInfoSentinel;
	enum_information_list_node EnumInfoSentinel;
	string_list_node SpecialStructNames;
};
