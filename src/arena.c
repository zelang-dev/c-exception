#include "raii.h"

#ifndef THRESHOLD
    #define THRESHOLD 10
#endif

struct arena_s {
    raii_type type;
    arena_t prev;
    char *avail;
    char *limit;
    void *base;
    size_t bytes;
    size_t total;
};

union header {
    arena_t tag;
    values_type slot;
};

static arena_t free_list = NULL;
static int num_free = 0;

arena_t arena_init(void) {
    arena_t arena = try_malloc(sizeof(*arena));
    arena->prev = NULL;
    arena->limit = arena->avail = NULL;
    arena->base = NULL;
    arena->total = 0;
    arena->bytes = 0;
    arena->type = RAII_ARENA + RAII_STRUCT;
    return arena;
}

void arena_free(arena_t arena) {
    if (is_empty(arena))
        return;

    if (is_type(arena, RAII_ARENA + RAII_STRUCT)) {
        memset(arena, -1, sizeof(arena_t));
        RAII_FREE(arena->base);
        RAII_FREE(arena);
        arena = NULL;
    }
}

void *malloc_arena(arena_t arena, long nbytes) {
    RAII_ASSERT(arena);
    RAII_ASSERT(nbytes > 0);
    if (arena_capacity(arena) == arena_total(arena))
        num_free = 0;

    nbytes = ((nbytes + sizeof(union header) - 1) /
              (sizeof(union header))) * (sizeof(union header));
    while (nbytes > arena->limit - arena->avail) {
        arena_t ptr;
        char *limit;
        if ((ptr = free_list) != NULL) {
            free_list = free_list->prev;
            num_free--;
            limit = ptr->limit;
        } else {
            long m = sizeof(union header) + nbytes + THRESHOLD * 1024;
            arena->total += m;
            ptr = try_realloc(arena->base, arena->total);
            arena->base = ptr;
            limit = (char *)arena->base + arena->total;
        }

        *ptr = *arena;
        arena->avail = (char *)((union header *)ptr + 1);
        arena->limit = limit;
        arena->prev = ptr;
        ptr->type = RAII_ARENA;
    }

    if (arena->prev)
        arena->prev->bytes = arena->bytes;

    arena->bytes = nbytes;
    arena->avail += nbytes;

    return arena->avail - nbytes;
}

void *calloc_arena(arena_t arena, long count, long nbytes) {
    RAII_ASSERT(count > 0);
    void *ptr = malloc_arena(arena, count * nbytes);
    memset(ptr, 0, count * nbytes);
    return ptr;
}

void arena_clear(arena_t arena) {
    if (is_empty(arena))
        return;

    while (arena->prev && is_type(arena->prev, RAII_ARENA)) {
        arena_t tmp = arena->prev;
        if (num_free < THRESHOLD) {
            arena->prev->prev = free_list;
            free_list = arena->prev;
            num_free++;
            if (is_zero(free_list->bytes))
                arena->avail -= arena->bytes;
            else
                arena->avail -= free_list->bytes;
        } else {
            arena->avail = (char *)((union header *)arena->base + 1);
            arena->limit = (char *)arena->base + arena->total;
            arena->bytes = 0;
            arena->prev = NULL;
        }

        arena = tmp;
    }
}

void arena_print(const arena_t arena) {
    printf("capacity: %zu, total: %zu, free_list:: %d\n", arena_capacity(arena), arena_total(arena), num_free);
}

RAII_INLINE size_t arena_capacity(const arena_t arena) {
    return !is_empty(arena) && is_type(arena, RAII_ARENA + RAII_STRUCT) ? arena->limit - arena->avail : 0;
}

RAII_INLINE size_t arena_total(const arena_t arena) {
    return !is_empty(arena) && is_type(arena, RAII_ARENA + RAII_STRUCT) ? MAX(arena->total, sizeof(union header)) - sizeof(union header) : 0;
}
