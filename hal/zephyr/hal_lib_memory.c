/*
 * Zephyr HAL for libIEC memory API
 * Backed by Zephyr's heap (k_malloc/k_calloc/k_free).
 * Provides a realloc fallback when native k_realloc is unavailable.
 */

#include "lib_memory.h"
#include <zephyr/kernel.h>
#include <string.h>

typedef struct {
    size_t size;
} MemHdr;

static inline MemHdr* memhdr_from_user(void* p) {
    return (MemHdr*)((uint8_t*)p - sizeof(MemHdr));
}
static inline void* user_from_memhdr(MemHdr* h) {
    return (void*)((uint8_t*)h + sizeof(MemHdr));
}

static MemoryExceptionHandler s_handler;
static void* s_handler_param;

static inline void no_mem(void)
{
    if (s_handler)
        s_handler(s_handler_param);
}

PAL_API void
Memory_installExceptionHandler(MemoryExceptionHandler handler, void* parameter)
{
    s_handler = handler;
    s_handler_param = parameter;
}

PAL_API void* Memory_malloc(size_t size)
{
    if (size == 0) size = 1;
    MemHdr* h = (MemHdr*)k_malloc(sizeof(MemHdr) + size);
    void* p = h ? user_from_memhdr(h) : NULL;
    return p;
}

PAL_API void* Memory_calloc(size_t nmemb, size_t size)
{
    size_t total;
    if (__builtin_mul_overflow(nmemb, size, &total)) {
        no_mem();
        return NULL;
    }
    if (total == 0) total = 1;
    MemHdr* h = (MemHdr*)k_malloc(sizeof(MemHdr) + total);
    if (h) {
        h->size = total;
        memset(user_from_memhdr(h), 0, total);
    }
    void* p = h ? user_from_memhdr(h) : NULL;
    if (!p) no_mem();
    return p;
}

PAL_API void* Memory_realloc(void* ptr, size_t size)
{
    if (ptr == NULL)
        return Memory_malloc(size);
    if (size == 0) {
        Memory_free(ptr);
        return NULL;
    }
    MemHdr* oldh = memhdr_from_user(ptr);
    size_t oldsz = oldh->size;
    if (size == oldsz)
        return ptr;
    MemHdr* nh = (MemHdr*)k_malloc(sizeof(MemHdr) + size);
    if (!nh) {
        no_mem();
        return NULL;
    }
    nh->size = size;
    void* np = user_from_memhdr(nh);
    size_t cpsz = (oldsz < size) ? oldsz : size;
    memcpy(np, ptr, cpsz);
    k_free(oldh);
    return np;
}

PAL_API void Memory_free(void* memb)
{
    if (!memb) return;
    MemHdr* h = memhdr_from_user(memb);
    k_free(h);
}
