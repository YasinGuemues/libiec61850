/*
 * Zephyr HAL stubs for memory API
 * Minimal no-op implementations to satisfy linking while HAL is developed.
 */

#include "lib_memory.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(libiec_hal_mem, LOG_LEVEL_ERR);

static MemoryExceptionHandler s_handler;
static void* s_handler_param;

PAL_API void
Memory_installExceptionHandler(MemoryExceptionHandler handler, void* parameter)
{
    LOG_ERR("Memory_installExceptionHandler(handler=%p, param=%p)", handler, parameter);
    /* keep for potential future use, but no allocations will trigger it */
    s_handler = handler;
    s_handler_param = parameter;
}

PAL_API void*
Memory_malloc(size_t size)
{
    LOG_ERR("Memory_malloc(size=%u)", (unsigned)size);
    /* non-functional stub: return NULL to indicate no memory */
    (void)size;
    return NULL;
}

PAL_API void*
Memory_calloc(size_t nmemb, size_t size)
{
    LOG_ERR("Memory_calloc(nmemb=%u,size=%u)", (unsigned)nmemb, (unsigned)size);
    (void)nmemb; (void)size;
    return NULL;
}

PAL_API void *
Memory_realloc(void *ptr, size_t size)
{
    LOG_ERR("Memory_realloc(ptr=%p,size=%u)", ptr, (unsigned)size);
    (void)ptr; (void)size;
    return NULL;
}

PAL_API void
Memory_free(void* memb)
{
    LOG_ERR("Memory_free(ptr=%p)", memb);
    (void)memb;
}

