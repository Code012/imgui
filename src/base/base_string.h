/* date = November 11th 2025 8:53 pm */

#ifndef BASE_STRING_HPP
#define BASE_STRING_HPP


// TODO: (me): Implement Unicode Conversions (when you need them)

////////////////////////////////
//- String Types

typedef struct String8 String8;
struct String8
{
    U8  *str;
    U64  size;
};

////////////////////////////////
//- String List & Array Types

typedef struct String8Node String8Node; 
struct String8Node
{
    String8Node *next;
    String8      string;
};

typedef struct String8List String8List; 
struct String8List
{
    String8Node *first;
    String8Node *last;
    U64          node_count;
    U64          total_size;
};

typedef struct String8Array String8Array;
struct String8Array
{
    String8 *v;
    U64      count;
    U64      total_size;
};

////////////////////////////////
//- String Matching, Splitting & Joining Types
//TODO: (sb): String splitting 

typedef U32 StringMatchFlags; 
enum StringMatchFlags
{
    StringMatchFlags_None             = (1u << 0),
    StringMatchFlags_CaseInsensitive  = (1u << 1),
    StringMatchFlags_StartsWith       = (1u << 2)
};

typedef U32 StringSplitFlags;
enum StringSplitFlags
{
    StringSplitFlags_None        = (1u << 0),
    StringSplitFlags_KeepEmpties = (1u << 1)
};

typedef struct StringJoin StringJoin;
struct StringJoin
{
    String8 pre;
    String8 sep;   // seperator
    String8 post;
};


////////////////////////////////
//- UTF Decoding Types

typedef struct UnicodeDecode UnicodeDecode;
struct UnicodeDecode
{
    U32 inc;
    U32 codepoint;
};

///////////////////////////////
//- Character Classification & Conversion Functions
// TODO: (sb): implement as needed
internal B32 CharIsAlpha(U8 c);
internal B32 CodepointIsAlpha(U32 c);
internal B32 CharIsAlphaUpper(U8 c);
internal B32 CodepointIsAlphaUpper(U32 c);
internal B32 CharIsAlphaLower(U8 c);
internal B32 CodepointIsAlphaLower(U32 c);
internal B32 CharIsDigit(U8 c);
internal B32 CodepointIsDigit(U32 c);
internal B32 CharIsSpace(U8 c);
internal B32 CodepointIsSpace(U32 c);
internal U8 UpperFromChar(U8 c);
internal U8 LowerFromChar(U8 c);


////////////////////////////////
//- C-String 

U64 CString8Length(U8 const* cstr);

internal char* CStrFromStr8(Arena* arena, String8 str);

////////////////////////////////
//- String Constructors

#define Str8Lit(S) Str8((U8*)(S), sizeof(S) - 1)
#define Str8Varg(S) (int)((S).size), ((S).str)     // for variadic functions where the format specifier is "%.*s" meaning an int value (width) is provided before the char string.
#define Str8LitComp(s) {(U8 *)(s), sizeof(s)-1}    // for defining in arrays

#define Str8Array(S,C) Str8((U8*)(S), sizeof(*(S)*(C)))
#define Str8ArrayFixed(S,C) Str8((U8*)(S), sizeof(S))
#define Str8Struct(S) Str8((U8*)(S), sizeof(*(S)))   // struct view

internal String8 Str8(U8 *str, U64 size);
internal String8 Str8Range(U8 *first, U8* one_past_last);   // memory view
internal String8 Str8Zero(void);
internal String8 Str8CString(char *c);
internal String8 Str8CStringCapped(void *cstr, void *cap);

///////////////////////////////
//- String Matching

internal B32 Str8Match(String8 a, String8 b, StringMatchFlags flags);

//////////////////////////////
//- String Slicing

internal String8 Str8Substr(String8 str, Rng1U64 range);
internal String8 Prefix8(String8 str, U64 size);
internal String8 Str8Skip(String8 str, U64 min);
////////////////////////////////
//- String Formatting & Copying

internal String8 PushStr8FV(Arena* arena, char const* fmt, va_list args);
internal String8 PushStr8F(Arena* arena, char const* fmt, ...);

////////////////////////////////
//- String List Construction Functions

internal String8Node* Str8ListPush(Arena* arena, String8List* list, String8 string);
internal String8Node* Str8ListPushF(Arena* arena, String8List* list, char const* fmt, ...);

////////////////////////////////
//- String Splitting & Joining

// internal String8List Str8Split(Arena *arena, String8 string, U8 *split_chars, U64 split_char_count, StringSplitFlags flags, )
internal String8  Str8ListJoin(Arena *arena, String8List *list, StringJoin *optional_params);

////////////////////////////////
//- UTF-8 Decoding/Encoding
internal UnicodeDecode UTF8Decode(U8* str, U64 max);
internal U32 UTF8Encode(U8* str, U32 codepoint);

internal U32 Utf8FromCodepoint(U8 *out, U32 codepoint);

internal S64 Utf8CodePointSize(U8 lead);
internal S64 Utf8PrevCodePointSize(U8 *data, S64 cursor);

#endif // BASE_STRING_HPP