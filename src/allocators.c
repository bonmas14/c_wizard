#include <stdlib.h>

typedef enum {
    ALLOCATOR_ALLOCATE,
    ALLOCATOR_REALLOCATE,
    ALLOCATOR_DEALLOCATE,
    ALLOCATOR_DELETE
} Allocator_Message;

#define ALLOCATOR_PROC(name)\
    void *name(void *p, u64 size, Allocator_Message message, void *data)

typedef ALLOCATOR_PROC(Allocator_Proc);

typedef struct {
    Allocator_Proc *proc;
    void           *data;
} Allocator;

#define mem_alloc(alloc, size)        (alloc).proc(NULL, size, ALLOCATOR_ALLOCATE,   (alloc).data)
#define mem_realloc(alloc, ptr, size) (alloc).proc(ptr,  size, ALLOCATOR_REALLOCATE, (alloc).data)
#define mem_free(alloc, ptr)          (alloc).proc(ptr,  0,    ALLOCATOR_DEALLOCATE, (alloc).data)
#define mem_delete(alloc)             (alloc).proc(NULL, 0,    ALLOCATOR_DELETE,     (alloc).data)


Allocator get_stdlib_allocator(void);
Allocator get_temporary_allocator(void);
Allocator create_arena_allocator(u64 size);


/// Stdlib Allocator
ALLOCATOR_PROC(stdlib_allocator_proc) {
    UNUSED(data);

    switch (message) {
        case ALLOCATOR_ALLOCATE:
            return calloc(1, size);
        case ALLOCATOR_REALLOCATE:
            return realloc((u8*)p, size);
        case ALLOCATOR_DEALLOCATE:
            free((u8*)p);
            break;
        case ALLOCATOR_DELETE:
            break;
    }

    return NULL;
}

Allocator get_stdlib_allocator(void) {
    return CLITERAL(Allocator) { stdlib_allocator_proc, NULL };
}


/// Temp Allocator

#define TEMP_SIZE MB(50)

struct {
    b32 initialized;
    u64 index;
    u8 data[TEMP_SIZE];
} __temp_alloc;

void temp_reset(void) {
    __temp_alloc.index = 0;
}

void *temp_allocate(u64 size) {
    if ((__temp_alloc.index + size) > TEMP_SIZE) {
        TraceLog(LOG_ERROR, "Temp allocator wrapped!");
        temp_reset();
    }

    if ((__temp_alloc.index + size) > TEMP_SIZE) {
        TraceLog(LOG_ERROR, "Too much space requested!");
        return NULL;
    }


    void *pos = (u8*)__temp_alloc.data + __temp_alloc.index;
    __temp_alloc.index += size;
    memset(pos, 0x00, size);
    return pos;
}

ALLOCATOR_PROC(temp_allocator_proc) {
    UNUSED(data);
    UNUSED(p);

    if (!__temp_alloc.initialized) {
        __temp_alloc.initialized = true; 
        temp_reset(); 
    }

    switch (message) {
        case ALLOCATOR_ALLOCATE:
            return temp_allocate(size);
        case ALLOCATOR_REALLOCATE:
        case ALLOCATOR_DEALLOCATE:
            break;
        case ALLOCATOR_DELETE:
            temp_reset();
            break;
    }

    return NULL;
}

Allocator get_temporary_allocator(void) {
    return CLITERAL(Allocator) { temp_allocator_proc, NULL };
}

/// Arena allocator

typedef struct Arena {
    u64 size;
    u64 occupied;
    struct Arena *next;
    u8 data[1]; // you dont have zero size arrays in MSVC...
} Arena;

Arena *arena_create(u64 size) {
    Allocator alloc = get_stdlib_allocator();

    Arena *arena = (Arena *)mem_alloc(alloc, size);

    if (!arena) {
        TraceLog(LOG_FATAL, "Buy mem, failed to create arena!");
        return NULL;
    }

    memset(arena, 0, sizeof(Arena));
    arena->size = size - sizeof(Arena);

    return arena;
}

void arena_delete(Arena *arena) {
    if (arena->next != NULL) {
        arena_delete(arena->next);
    }

    Allocator alloc = get_stdlib_allocator();
    mem_free(alloc, arena);
}

void *arena_allocate(u64 size, Arena *arena) {
    if (size <= (arena->size - arena->occupied)) {
        u8* pos = arena->data + arena->occupied;
        arena->occupied += size;
        return (void*)pos;
    }

    if (!arena->next) {
        arena->next = arena_create((arena->size + sizeof(Arena)) * 2LL);
    } 

    return arena_allocate(size, arena->next);
}

ALLOCATOR_PROC(arena_allocator_proc) {
    UNUSED(p);
    assert(data != NULL);

    Arena *arena = (Arena*)data;
    
    switch (message) {
        case ALLOCATOR_ALLOCATE:
            return arena_allocate(size, arena);
        case ALLOCATOR_REALLOCATE:
            TraceLog(LOG_ERROR, "Arena doesn't reallocate.");
            break;
        case ALLOCATOR_DEALLOCATE:
            TraceLog(LOG_ERROR, "Arena doesn't free it's memory, please destroy arena itself.");
            break;
        case ALLOCATOR_DELETE:
            arena_delete(arena);
            break;
    }

    return NULL;
}

Allocator create_arena_allocator(u64 size) {
    return CLITERAL(Allocator) { arena_allocator_proc, arena_create(size) };
}
