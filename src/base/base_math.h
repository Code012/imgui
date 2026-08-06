#ifndef  BASE_MATH_HPP
#define BASE_MATH_HPP

// TODO(S): CLEAN UP 

////////////////////////////////
// Vector Types

typedef union Vec2S32 Vec2S32;
union Vec2S32
{
    struct
    {
        S32 x;
        S32 y;
    };
    S32 v[2];
};

typedef union Vec2F32 Vec2F32;
union Vec2F32
{
    struct
    {
        F32 x;
        F32 y;
    };
    F32 v[2];
};

typedef union Vec3F32 Vec3F32;
union Vec3F32
{
    struct 
    {
        F32 x;
        F32 y;
        F32 z;
    };
    struct
    {
        Vec2F32 xy;
        F32 _z0;
    };
    struct
    {
        F32 _x0;
        Vec2F32 yz;
    };
    F32 v[3];
};

typedef union Vec4U8 Vec4U8;
union Vec4U8
{
    struct
    {
        U8 x;
        U8 y;
        U8 z;
        U8 w;
    };
    U8 v[4];
};

typedef union Vec4F32 Vec4F32;
union Vec4F32
{
    struct
    {
        F32 x;
        F32 y;
        F32 z;
        F32 w;
    };
    struct
    {
        Vec2F32 xy;
        Vec2F32 zw;
    };
    struct
    {
        Vec3F32 xyz;
        F32 _z0;
    };
    struct
    {
        F32 _x0;
        Vec3F32 yzw;
    };
    F32 v[4];
};

////////////////////////////////
// Range Types

// 1-range
typedef union Rng1U64 Rng1U64; 
union Rng1U64
{

    struct
    {
        U64 min;
        U64 max;
    };
    U64 v[2];
};

typedef union Rng1S64 Rng1S64;
union Rng1S64
{

    struct 
    {
        S64 min;
        S64 max;
    };
    S64 v[2];
};


// 2-range
typedef union Rng2S32 Rng2S32; 
union Rng2S32
{
    struct
    {
        Vec2S32 min;
        Vec2S32 max;
    };
    struct
    {
        Vec2S32 p0;
        Vec2S32 p1;
    };
    struct
    {
        S32 x0;
        S32 y0;
        S32 x1;
        S32 y1;
    };
    Vec2S32 v[2];
};

#define V4U8(x, y, z, w) Vec_4U8((x), (y), (z), (w))
internal Vec4U8 Vec_4U8(U8 x, U8 y, U8 z, U8 w) {Vec4U8 v = {x, y, z, w}; return v;}
internal Vec4U8 Vec4U8FromU32(U32 hex) 
{
    Vec4U8 v = V4U8((hex >> 24) & 0XFF,
                    (hex >> 16) & 0XFF,
                    (hex >> 8)  & 0XFF,
                    (hex >> 0)  & 0XFF);

    return v;
}

#define V2S32(x, y) Vec_2S32((x), (y))
internal Vec2S32 Vec_2S32(S32 x, S32 y) {Vec2S32 v = {x, y}; return v;}
internal B32 Vec2S32Equal(Vec2S32 a, Vec2S32 b) { B32 c = (a.x == b.x && a.y == b.y); return c;}
internal Vec2S32 Scale2S32(Vec2S32 v, S32 s) {Vec2S32 c = {v.x*s, v.y*s}; return c;} 


#define V4F32(x, y, z, w) Vec_4F32((x), (y), (z), (w))
internal Vec4F32 Vec_4F32(F32 x, F32 y, F32 z, F32 w) {Vec4F32 v = {x, y, z, w}; return v;}

////////////////////////////////
// Range Ops

internal U64 Dim1U64(Rng1U64 r);

internal S64 Dim1S64(Rng1S64 r);
internal S64 DeltaS64(Rng1S64 r);

internal Vec2S32 Dim2S32(Rng2S32 r)             {Vec2S32 dim = {((r.max.x > r.min.x) ? (r.max.x - r.min.x) : 0), ((r.max.y > r.min.y) ? (r.max.y - r.min.y) : 0)}; return dim;}
internal Rng2S32 Intersect2S32(Rng2S32 a, Rng2S32 b)    {Rng2S32 c; c.p0.x = Max(a.min.x, b.min.x); c.p0.y = Max(a.min.y, b.min.y); c.p1.x = Min(a.max.x, b.max.x); c.p1.y = Min(a.max.y, b.max.y); return c;}
internal Rng2S32 Union2S32(Rng2S32 a, Rng2S32 b) {Rng2S32 c; c.p0.x = Min(a.min.x, b.min.x); c.p0.y = Min(a.min.y, b.min.y); c.p1.x = Max(a.max.x, b.max.x); c.p1.y = Max(a.max.y, b.max.y); return c; }

internal B32 Overlap2S32(Rng2S32 a, Rng2S32 b) {return (b.max.x >= a.min.x && b.min.x <= a.max.x) && (b.max.y >= a.min.y && b.min.y <= a.max.y); }

/*
internal Vec4F32
rgba_from_u32(U32 hex)
{
  Vec4F32 result = v4f32(((hex&0xff000000)>>24)/255.f,
                         ((hex&0x00ff0000)>>16)/255.f,
                         ((hex&0x0000ff00)>> 8)/255.f,
                         ((hex&0x000000ff)>> 0)/255.f);
  return result;
}
*/


internal U32 PackARGBFromRGBA(Vec4U8 color)
{
    U32 result = 0;
    result |= (U32)color.w << 24; // A
    result |= (U32)color.x << 16; // R
    result |= (U32)color.y << 8;  // G
    result |= (U32)color.z << 0;  // B    
    return result;
}
// internal Vec4F32 V4F32_RGBA_FROM_U32_RGBA(U32 hex)
// {
//     Vec4F32 result = V4F32(((hex&0xFF000000)>>24)/255.f,
//                            ((hex&0x00FF0000)>>16)/255.f,
//                            ((hex&0x0000FF00)>> 8)/255.f,
//                            ((hex&0x000000FF)>> 0)/255.f);
//     return result;
// }
// internal U32 U32_RGBA_From_V4F32_RGBA(Vec4F32 color)
// {
//     U32 result = 0;
//     result |= (U32)((U8)(color.x*255.f)) << 24;
//     result |= (U32)((U8)(color.y*255.f)) << 16;
//     result |= (U32)((U8)(color.z*255.f)) << 8;
//     result |= (U32)((U8)(color.w*255.f)) << 0;
//     return result;
// }
// internal U32 U32_ARGB_From_V4F32_RGBA(Vec4F32 color)
// {
//     U32 result = 0; 
//     result |= (U32)((U8)(color.w*255.f)) << 24;
//     result |= (U32)((U8)(color.x*255.f)) << 16;
//     result |= (U32)((U8)(color.y*255.f)) << 8;
//     result |= (U32)((U8)(color.z*255.f)) << 0;
//     return result;
// }



#endif // BASE_MATH_HPP
