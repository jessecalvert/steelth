/*@H
* File: steelth_stream.h
* Author: Jesse Calvert
* Created: July 2, 2018, 14:19
* Last modified: April 10, 2019, 17:31
*/

#pragma once

struct stream_chunk
{
	buffer Buffer;

	string Filename;
	u32 LineNumber;

	stream_chunk *Next;
};

struct stream
{
	struct memory_region *Region;
	stream *Errors;

	buffer Buffer;

	stream_chunk *First;
	stream_chunk *Last;

	b32 Underflow;
	ticket_mutex Mutex;
};

internal stream
CreateStream(memory_region *Region, stream *Errors)
{
	stream Result = {};
	Result.Region = Region;
	Result.Errors = Errors;
	return Result;
}

#define AppendStruct(Stream, Struct) AppendChunk(Stream, sizeof(Struct), &(Struct))
internal stream_chunk *
AppendChunk(stream *Stream, umm Size, void *Contents)
{
	BeginTicketMutex(&Stream->Mutex);
	stream_chunk *Chunk = PushStruct(Stream->Region, stream_chunk);

	Chunk->Buffer.Used = Chunk->Buffer.Size = Size;
	Chunk->Buffer.Contents = (u8 *)Contents;
	Chunk->Next = 0;

	Stream->Last = ((Stream->Last ? Stream->Last->Next : Stream->First) = Chunk);
	Stream->Underflow = false;
	EndTicketMutex(&Stream->Mutex);

	return Chunk;
}

#define Consume(Stream, type) (type *)ConsumeSize(Stream, sizeof(type))
internal u8 *
ConsumeSize(stream *Stream, umm Size)
{
	u8 *Result = 0;

	BeginTicketMutex(&Stream->Mutex);

	if((!Stream->Buffer.Size) && Stream->First)
	{
		stream_chunk *NewChunk = Stream->First;
		Stream->First = Stream->First->Next;

		Stream->Buffer = NewChunk->Buffer;
	}

	if(Stream->Buffer.Size >= Size)
	{
		Result = Stream->Buffer.Contents;
		Stream->Buffer.Contents = Stream->Buffer.Contents + Size;
		Stream->Buffer.Size -= Size;
	}
	else
	{
		Stream->Buffer = {};
		Stream->First = Stream->Last = 0;
		Stream->Underflow = true;
	}

	EndTicketMutex(&Stream->Mutex);

	return Result;
}

internal string *
ConsumeString(stream *Stream)
{
	string *Result = Consume(Stream, string);
	if(Result)
	{
		void *TextData = ConsumeSize(Stream, Result->Length);
	}

	return Result;
}

#define Outf(...) Outf_(__FILE__, __LINE__, __VA_ARGS__)
internal void
Outf_(char *Filename, u32 LineNumber, stream *Dest, char *Format, ...)
{
	if(Dest)
	{
		char Buffer[1024];

		va_list ArgList;
		va_start(ArgList, Format);
		string FormattedString = FormatStringList(Buffer, ArrayCount(Buffer), Format, ArgList);
		va_end(ArgList);

		u32 ContentsSize = FormattedString.Length + sizeof(string);
		u8 *Contents = PushSize(Dest->Region, ContentsSize);
		string *StreamString = (string *)Contents;
		StreamString->Length = FormattedString.Length;
		StreamString->Text = (char *)(Contents + sizeof(string));
		CopySize(FormattedString.Text, StreamString->Text, FormattedString.Length);

		stream_chunk *Chunk = AppendChunk(Dest, ContentsSize, Contents);
		Chunk->Filename = PushString(Dest->Region, Filename);
		Chunk->LineNumber = LineNumber;
	}
}
