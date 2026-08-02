/* date = November 18th 2025 9:20 pm */




///////////////////////////////
//- Character Classification & Conversion Functions

internal B32 
CharIsAlpha(U8 c)
{
	return CharIsAlphaUpper(c) || CharIsAlphaLower(c);
}
internal B32
CodepointIsAlpha(U32 c)
{
	return CodepointIsAlphaUpper(c) || CodepointIsAlphaLower(c);
}

internal B32 
CharIsAlphaUpper(U8 c)
{
	return c >= 'A' && c <= 'Z';
}
internal B32 
CodepointIsAlphaUpper(U32 c)
{
	return c >= 'A' && c <= 'Z';
}

internal B32
CharIsAlphaLower(U8 c)
{
	return c >= 'a' && c <= 'z';
}
internal B32
CodepointIsAlphaLower(U32 c)
{
	return c >= 'a' && c <= 'z';
}

internal B32 
CharIsDigit(U8 c)
{
	return c >= '0' && c <= '9';
}
internal B32
CodepointIsDigit(U32 c)
{
	return c >= '0' && c <= '9';
}

internal B32 
CharIsSpace(U8 c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}
internal B32
CodepointIsSpace(U32 c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

internal U8 
UpperFromChar(U8 c)
{
	return (c >= 'a' && c <= 'z') ? ('A' + (c - 'a')) : c;
}
internal U8 
LowerFromChar(U8 c)
{
	return (c >= 'A' && c <= 'Z') ? ('a' + (c - 'A')) : c;
}


////////////////////////////////
//- C-String

U64 
CString8Length(U8 const* cstr)
{
	U64 length = 0;
	U8 const* p = cstr;
	if (cstr)
	{
		for (;*p != 0; p += 1);
			length = (U64)(p-cstr);
	}
	return length;
}

internal char* 
CStrFromStr8(Arena* arena, String8 str)
{
	char* result = PushArray(arena, char, str.size+1);
	MemoryCopy(result, str.str, str.size);
	result[str.size] = '\0';
	return result;
}


////////////////////////////////
//- String Constructors

internal String8 
Str8(U8 *str, U64 size)
{
	String8 result = {str, size};
	return result;
}

internal String8 
Str8Range(U8 *first, U8* one_past_last)
{
	String8 result = {first, (U64)(one_past_last - first)};
	return result;
}

internal String8 
Str8Zero(void)
{
	String8 result = zero_struct;
	return result;
}

internal String8 
Str8CString(char *c)
{
	String8 result = {(U8 *)c, CString8Length((U8 *)c)};
	return result;
}

internal String8 
Str8CStringCapped(void *cstr, void *cap)
{
	char *ptr = (char *)cstr;
	char *opl = (char *)cap;
	for (;ptr < opl && *ptr != 0; ptr += 1);
	U64 size = (U64)(ptr - (char *)cstr);
	String8 result = Str8((U8*)cstr, size);
	return result;
}


///////////////////////////////
//- String Matching


internal B32 
Str8Match(String8 a, String8 b, StringMatchFlags flags)
{
	B32 result = 0;
	if (a.size == b.size && flags == 0)
	{
		result = MemoryMatch(a.str, b.str, b.size);
	}
	else if (a.size == b.size || (flags & StringMatchFlags_StartsWith))
	{
		B32 case_insensitive = (flags & StringMatchFlags_CaseInsensitive);
		U64 size 			 = Min(a.size, b.size);
		result = 1;
		for (U64 i = 0; i < size; i += 1)
		{
			U8 at = a.str[i];
			U8 bt = b.str[i];
			if (case_insensitive)
			{
				at = UpperFromChar(at);
				bt = UpperFromChar(bt);
			}
			if (at != bt)
			{
				result = 0;
				break;
			}
		}
	}
	return result;
}

//////////////////////////////
//- String Slicing


internal String8 
Str8Substr(String8 str, Rng1U64 range)
{
	range.min = ClampTop(range.min, str.size);
	range.max = ClampTop(range.max, str.size);
	str.str += range.min;
	str.size = Dim1U64(range);
	return str;
}

internal String8 Prefix8(String8 str, U64 size)
{
	return Str8Substr(str, (Rng1U64){0, size});
}

internal String8
Str8Skip(String8 str, U64 min)
{
 return Str8Substr(str, (Rng1U64){min, str.size});
}

////////////////////////////////
//- String Formatting & Copying

internal String8 
PushStr8FV(Arena* arena, char const* fmt, va_list args)
{
	va_list args2;
	va_copy(args2, args);
	U32 needed_bytes = sb_stbsp_vsnprintf(0, 0, fmt, args) + 1;
	String8 result = zero_struct;
	result.str = PushArrayNoZero(arena, U8, needed_bytes);
	result.size = sb_stbsp_vsnprintf((char*)result.str, needed_bytes, fmt, args2);
	result.str[result.size] = 0;
	va_end(args2);
	return result;
}
internal String8 
PushStr8F(Arena* arena, char const* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	String8 result = PushStr8FV(arena, fmt, args);
	va_end(args);
	return result;
}

////////////////////////////////
//- String List Construction Functions

internal String8Node* 
Str8ListPush(Arena* arena, String8List* list, String8 string)
{
	String8Node* node = PushArrayNoZero(arena, String8Node, 1);

	SLLQueuePush(list->first, list->last, node);
	list->node_count += 1;
	list->total_size += string.size;
	node->string = string;
	return node;
}
internal String8Node* 
Str8ListPushF(Arena* arena, String8List* list, char const* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	String8 string = PushStr8FV(arena, fmt, args);
	String8Node* result = Str8ListPush(arena, list, string);
	va_end(args);
	return result;
}


////////////////////////////////
//- String Splitting & Joining Types


// internal String8List Str8Split(Arena *arena, String8 string, U8 *split_chars, U64 split_char_count, StringSplitFlags flags, )

internal String8
Str8ListJoin(Arena *arena, String8List *list, StringJoin *optional_params)
{
	// setup join params
	StringJoin join = zero_struct;
	if (optional_params != NULL)
	{
		MemoryCopy(&join, optional_params, sizeof(join));
	}
	Assert(optional_params == 0); // checking if NULL check catches 0 as well, if it does then get rid of this

	// calculate size & allocate
	U64 sep_count = 0;
	if (list->node_count > 1)
	{
		sep_count = list->node_count - 1;
	}
	// fill, pre
	String8 result = zero_struct;
	result.size = join.pre.size + join.post.size + sep_count*join.sep.size + list->total_size;
	result.str = PushArrayNoZero(arena, U8, result.size+1);

	// fill, sep
	U8 *ptr = result.str;
	MemoryCopy(ptr, join.pre.str, join.pre.size);
	ptr += join.pre.size;
	for (String8Node *node = list->first;
		node != NULL;
		node = node->next)
	{
		MemoryCopy(ptr, node->string.str, node->string.size);
		ptr += node->string.size;
		if (node->next != NULL)
		{
			MemoryCopy(ptr, join.sep.str, join.sep.size);
			ptr += join.sep.size;
		}
	}
	// fill, post
	MemoryCopy(ptr, join.post.str, join.post.size);
	ptr += join.post.size;
	*ptr = 0;	// null-terminate
	return result;
}


////////////////////////////////
//- UTF-8 Decoding/Encoding

read_only global U8 UTF8Class[32] =		// lookup table that takes top-most 5 bits from utf8 string as index to determine its classification
{
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 	// ASCII 		(0-15) 
	0,0,0,0,0,0,0,0,					// Continuation (16-23)
	2,2,2,2,							// 2-byte start (24-27)
	3,3, 								// 3-byte start (28,29)
	4,									// 4-byte start (30)
	5,									// Invalid		(31)
};

internal UnicodeDecode UTF8Decode(U8* str, U64 buffer_size)
{
	UnicodeDecode result = {1, U32Max};
	U8 byte = str[0];
	U8 byte_class = UTF8Class[byte >> 3];	// get utf8class by checking top-5 bits
	switch(byte_class)
	{
		case 1:
		{
			result.codepoint = byte;
		} break;
		case 2:
		{
			if (buffer_size > 1)
			{
				U8 cont_byte = str[1];
				if (UTF8Class[cont_byte >> 3] == 0)	 // continuation byte check
				{
					// byte 	 = 110xxxxx
					// cont_byte = 10yyyyyy
					// codepoint = xxxxx yyyyyy
					result.codepoint = (byte & bitmask5);		// 000xxxxx
					result.codepoint <<= 6;						// xxxxx000000
					result.codepoint |= (cont_byte & bitmask6);	// xxxxx000000 | (00yyyyyy) ==> xxxxxyyyyyy
					result.inc = 2;
				}
			}
		} break;
		case 3:
		{
			if (buffer_size > 2)
			{
				U8 cont_byte[2] = {str[1], str[2]};
				if (UTF8Class[cont_byte[0] >> 3] == 0 && // continuation byte check
					UTF8Class[cont_byte[1] >> 3] == 0)
				{
					// byte 		= 1110xxxx
					// cont_byte[0] = 10yyyyyy
					// cont_byte[1] = 10zzzzzz
					result.codepoint = ((byte & bitmask4) << 12);			// xxxx 0000 0000 0000
					result.codepoint |= ((cont_byte[0] & bitmask6) << 6); 	// xxxx yyyy yy00 0000
					result.codepoint |= (cont_byte[1] & bitmask6);			// xxxx yyyy yyzz zzzz
					result.inc = 3;
				}
			}
		} break;
		case 4:
		{
			if (buffer_size > 3)
			{
				U8 cont_byte[3] = {str[1], str[2], str[3]}; // continuation byte check
				if (UTF8Class[cont_byte[0] >> 3] == 0 &&
					UTF8Class[cont_byte[1] >> 3] == 0 &&
					UTF8Class[cont_byte[2] >> 3] == 0)
				{	
					// byte 		= 11110www
					// cont_byte[0] = 10xxxxxx
					// cont_byte[1] = 10yyyyyy
					// cont_byte[2] = 10zzzzzz
					result.codepoint = ((byte & bitmask3) << 18);					// www0 0000 0000 0000 0000 0
					result.codepoint |= ((cont_byte[0] & bitmask6) << 12); 			// wwwx xxxx x000 0000 0000 0
					result.codepoint |= ((cont_byte[1] & bitmask6) << 6);			// wwwx xxxx xyyy yyy0 0000 0
					result.codepoint |= ((cont_byte[2] & bitmask6));				// wwwx xxxx xyyy yyyz zzzz z
					result.inc = 4;
				}
			}
		} break;
	}

	return result;
}

internal U32 
UTF8Encode(U8* str, U32 codepoint)
{
	U32 inc = 0;
	if (codepoint <= 0x7f) // 127
	{
		str[0] = (U8)codepoint;
		inc = 1;
	}
	else if (codepoint <= 0x7ff)	// 2047
	{
		// codepoint = xxxxx | yyyyyy (11 bits)
		str[0] = (bitmask2 << 6) | ((codepoint >> 6) & bitmask5);	// First byte 110xxxxx	
		str[1] = (bit8) | (codepoint & bitmask6);					// Second byte 10yyyyyy
		inc = 2;
	}
	else if (codepoint <= 0Xffff) // 65535
	{
		// codepoint = xxxx | yyyyyy | zzzzzz (16 bits)
		str[0] = (bitmask3 << 5) | ((codepoint >> 12) & bitmask4);	// First byte 1110xxxx
		str[1] = (bit8) | ((codepoint >> 6) & bitmask6);			// Second byte 10yyyyyy
		str[2] = (bit8) | (codepoint & bitmask6);					// Third byte 10zzzzzz
		inc = 3;
	}
	else if (codepoint <= 0x10FFFF) // 1114111
	{
		// codepoint = xxxyyyyyyzzzzzzwwwwww(21 bits)
		// encoding = 11110xxx 10yyyyyy 10zzzzzz 10wwwwww
		str[0] = (bitmask4 << 4) | ((codepoint >> 18) & bitmask3);
		str[1] = (bit8) | ((codepoint >> 12) & bitmask6);
		str[2] = (bit8) | ((codepoint >> 6) & bitmask6);
		str[3] = (bit8) | (codepoint & bitmask6);
		inc = 4;
	}
	else
	{
		str[0] = '?';
		inc = 1;
	}
	return inc;
}

internal String8 
PushString8FromCodepoint(Arena* arena, U32 codepoint)
{
	U8 temp[4];
	U32 inc = UTF8Encode(temp, codepoint);

	U8* mem = PushArray(arena, U8, inc);
	MemoryCopy(mem, temp, inc);

	String8 str;
	str.str = mem;
	str.size = inc;

	return str;
}

// `lead` must be the first byte of a UTF-8 codepoint but it will still work if a continuation byte is passed to it
internal S64 Utf8CodePointSize(U8 lead)
{
    // 0xxxxxxx
    // ASCII range (U+0000 – U+007F)
    // Single-byte codepoint
    if ((lead & 0x80) == 0x00) return 1;

    // 110xxxxx
    // Start of a 2-byte UTF-8 sequence
    if ((lead & 0xE0) == 0xC0) return 2;

    // 1110xxxx
    // Start of a 3-byte UTF-8 sequence
    if ((lead & 0xF0) == 0xE0) return 3;

    // 11110xxx
    // Start of a 4-byte UTF-8 sequence
    if ((lead & 0xF8) == 0xF0) return 4;

    // if lead is a continuation byte (10xxxxxx)
    return 1;
}

// `cursor` is a byte index *after* a codepoint.
internal S64 Utf8PrevCodePointSize(U8 *data, S64 cursor)
{
    S64 i = cursor - 1;

    // Walk backward over continuation bytes: 10xxxxxx
    while ((data[i] & 0xC0) == 0x80)
    {
        i--;
    }

    return cursor - i;
}