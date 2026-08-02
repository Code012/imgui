
///////////////////////////////
// Range Types


//////////////////////////////
// Range Ops
internal U64 Dim1U64(Rng1U64 r)                         {U64 c = ((r.max > r.min) ? (r.max - r.min) : 0); return c;}

internal S64 Dim1S64(Rng1S64 r)                         {S64 c = ((r.max > r.min) ? (r.max - r.min) : 0); return c;}
internal S64 DeltaS64(Rng1S64 r)						{return r.max - r.min;}
