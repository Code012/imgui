////////////////////////////////
// Arena Functions

// arena creation/destruction
internal Arena *
ArenaAlloc_(ArenaParams* params)
{
	U64 size = params->reserve_size;
	B32 ignore_reserve_granularity = params->ignore_reserve_granularity;
	if (!ignore_reserve_granularity)	// do reserve granularity
	{
		U64 reserve_size_roundup_granularity = MiB(64);
		size += reserve_size_roundup_granularity-1;
		size -= size%reserve_size_roundup_granularity;
	}
	
	void *block = ReserveMemory(size);
	U64 initial_commit_size = ARENA_COMMIT_GRANULARITY;
	Assert(initial_commit_size >= ARENA_HEADER_SIZE);
	CommitMemory(block, initial_commit_size);
	Arena *arena = (Arena *)block;
	arena->pos = ARENA_HEADER_SIZE;
	arena->commit_pos = initial_commit_size;
	arena->reserve_size = size;

	return arena;
}

internal void 
ArenaRelease(Arena **arena)
{
	ReleaseMemory(*arena, (*arena)->reserve_size);
	*arena = NULL;
}

// arena push/pop/pos core functions
void *
ArenaPush(Arena *arena, U64 size, U64 align, B32 zero)
{
	void *result = NULL;
	if (arena->pos + size <= arena->reserve_size)
	{
		U8 *base = (U8 *)arena;

		U64 post_align_pos = (arena->pos + (align-1));
		post_align_pos -= post_align_pos%align;

		U64 align_adjust = post_align_pos - arena->pos;
		result = base + arena->pos + align_adjust;
		arena->pos += size + align_adjust;
		if (arena->commit_pos < arena->pos)
		{
			U64 size_to_commit = arena->pos - arena->commit_pos;
			size_to_commit += ARENA_COMMIT_GRANULARITY - 1;
			size_to_commit -= size_to_commit%ARENA_COMMIT_GRANULARITY;
			CommitMemory(base + arena->commit_pos, size_to_commit);
			arena->commit_pos += size_to_commit;
		}
	}

	if (zero)
	{
		MemoryZero(result, size);
	}

	return result;
}

internal U64 
ArenaPos(Arena *arena)
{
	return arena->pos;
}

internal void 
ArenaPopTo(Arena *arena, U64 pos)
{
	U64 min_pos = ARENA_HEADER_SIZE;
	U64 new_pos = ClampBot(min_pos, pos);
	arena->pos = new_pos;

	U64 pos_aligned_to_commit_chunks = arena->pos + ARENA_COMMIT_GRANULARITY-1;
	pos_aligned_to_commit_chunks -= pos_aligned_to_commit_chunks%ARENA_COMMIT_GRANULARITY;
	// if the gap is >= ARENA_DECOMMIT_THRESHOLD then actually decommit, otherwise reuse committed memory
	if (pos_aligned_to_commit_chunks + ARENA_DECOMMIT_THRESHOLD <= arena->commit_pos)
	{
		U8 *base = (U8 *)arena;
		U64 size_to_decommit = arena->commit_pos - pos_aligned_to_commit_chunks;
		DecommitMemory(base + pos_aligned_to_commit_chunks, size_to_decommit);
		arena->commit_pos -= size_to_decommit;
	}
}

// arena push/pop helpers
internal void 
ArenaClear(Arena *arena)
{
	ArenaPopTo(arena, ARENA_HEADER_SIZE);
}
internal void 
ArenaPop(Arena *arena, U64 amt)
{
	U64 min_pos = ARENA_HEADER_SIZE;
	U64 size_to_pop_clamped = ClampTop(amt, arena->pos);
	U64 new_pos = arena->pos - size_to_pop_clamped;
	U64 new_pos_clamped = ClampBot(min_pos, new_pos);
	ArenaPopTo(arena, new_pos);
}

// temporary arena scopes

//~ Scratch Arena Pool 
perthread_static Arena *ScratchArenaPool[2] = zero_struct;

internal ArenaTemp 
ArenaTempBegin(Arena *arena)
{
	U64 pos = ArenaPos(arena);
	ArenaTemp temp = {arena, pos};
	return temp;
}

internal void 
ArenaTempEnd(ArenaTemp temp)
{
	ArenaPopTo(temp.arena, temp.pos);
}


internal ArenaTemp 
ArenaGetScratch(Arena **excluded_arenas, U32 excluded_count)
{
	// init on first time
	if (ScratchArenaPool[0] == NULL)
	{
		for (U64 i = 0; i < ArrayCount(ScratchArenaPool); i += 1)
		{
			ScratchArenaPool[i] = ArenaAlloc();
		}
	}

	// get non-conflicting arena
	ArenaTemp result = zero_struct;

	for (U64 i = 0; i < ArrayCount(ScratchArenaPool); i += 1)
	{
		Arena *candidate = ScratchArenaPool[i];
		B32 is_valid_candidate = 1;
		for(U32 j = 0; j < excluded_count; j += 1)
		{
			if (candidate == excluded_arenas[i])
			{
				is_valid_candidate = 0;
				break;
			}
		}

		if (is_valid_candidate)
		{
			result = ArenaTempBegin(candidate);
			break;
		}
	}

	return result;
}

//- Log arena
perthread_static Arena* log_arena;