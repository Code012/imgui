#ifndef  BASE_MATH_HPP
#define BASE_MATH_HPP


////////////////////////////////
// Vector Types

struct Vec2F32
{
    F32 x;
    F32 y;
};


////////////////////////////////
// Range Types

// 1-range
// originally a union of min,max and v[2] but theres some nasty UB involving it so I had to change it
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


////////////////////////////////
// Range Ops

internal U64 Dim1U64(Rng1U64 r);

internal S64 Dim1S64(Rng1S64 r);
internal S64 DeltaS64(Rng1S64 r);



#endif // BASE_MATH_HPP
