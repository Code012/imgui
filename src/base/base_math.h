#ifndef  BASE_MATH_HPP
#define BASE_MATH_HPP


////////////////////////////////
// Vector Types

typedef struct Vec2S32 Vec2S32;
struct Vec2S32
{
    S32 x;
    S32 y;
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

#define V2S32(x, y) Vec_2S32((x), (y))
internal Vec2S32 Vec_2S32(S32 x, S32 y) {Vec2S32 v = {x, y}; return v;}
internal B32 Vec2S32Equal(Vec2S32 a, Vec2S32 b) { B32 c = (a.x == b.x && a.y == b.y); return c;}
internal Vec2S32 Scale2S32(Vec2S32 v, S32 s) {Vec2S32 c = {v.x*s, v.y*s}; return c;} 

////////////////////////////////
// Range Ops

internal U64 Dim1U64(Rng1U64 r);

internal S64 Dim1S64(Rng1S64 r);
internal S64 DeltaS64(Rng1S64 r);

internal Vec2S32 Dim2S32(Rng2S32 r)             {Vec2S32 dim = {((r.max.x > r.min.x) ? (r.max.x - r.min.x) : 0), ((r.max.y > r.min.y) ? (r.max.y - r.min.y) : 0)}; return dim;}
internal Rng2S32 Intersect2S32(Rng2S32 a, Rng2S32 b)    {Rng2S32 c; c.p0.x = Max(a.min.x, b.min.x); c.p0.y = Max(a.min.y, b.min.y); c.p1.x = Min(a.max.x, b.max.x); c.p1.y = Min(a.max.y, b.max.y); return c;}
internal Rng2S32 Union2S32(Rng2S32 a, Rng2S32 b) {Rng2S32 c; c.p0.x = Min(a.min.x, b.min.x); c.p0.y = Min(a.min.y, b.min.y); c.p1.x = Max(a.max.x, b.max.x); c.p1.y = Max(a.max.y, b.max.y); return c; }

internal B32 Overlap2S32(Rng2S32 a, Rng2S32 b) {return (b.max.x >= a.min.x && b.min.x <= a.max.x) && (b.max.y >= a.min.y && b.min.y <= a.max.y); }



#endif // BASE_MATH_HPP
