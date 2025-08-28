/*
 * Zephyr HAL stubs for thread/semaphore API
 * Minimal no-op implementations to satisfy linking.
 */

#include "hal_thread.h"
#include "lib_memory.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(libiec_hal_thread, LOG_LEVEL_ERR);

struct sThread {
    ThreadExecutionFunction function;
    void* parameter;
    bool autodestroy;
};

PAL_API Thread
Thread_create(ThreadExecutionFunction function, void* parameter, bool autodestroy)
{
    LOG_ERR("Thread_create(func=%p, param=%p, autodestroy=%d)", function, parameter, (int)autodestroy);
    Thread t = (Thread)GLOBAL_CALLOC(1, sizeof(struct sThread));
    if (t) {
        t->function = function;
        t->parameter = parameter;
        t->autodestroy = autodestroy;
    }
    return t;
}

PAL_API void
Thread_start(Thread thread)
{
    LOG_ERR("Thread_start(thread=%p)", thread);
    (void)thread;
    /* no-op */
}

PAL_API void
Thread_destroy(Thread thread)
{
    LOG_ERR("Thread_destroy(thread=%p)", thread);
    if (thread) {
        GLOBAL_FREEMEM(thread);
    }
}

PAL_API void
Thread_sleep(int millies)
{
    LOG_ERR("Thread_sleep(ms=%d)", millies);
    (void)millies;
    /* no-op */
}

PAL_API Semaphore
Semaphore_create(int initialValue)
{
    LOG_ERR("Semaphore_create(init=%d)", initialValue);
    (void)initialValue;
    /* allocate dummy object to avoid NULL deref if used */
    return GLOBAL_CALLOC(1, sizeof(int));
}

PAL_API void
Semaphore_wait(Semaphore self)
{
    LOG_ERR("Semaphore_wait(sem=%p)", self);
    (void)self;
    /* no-op */
}

PAL_API void
Semaphore_post(Semaphore self)
{
    LOG_ERR("Semaphore_post(sem=%p)", self);
    (void)self;
    /* no-op */
}

PAL_API void
Semaphore_destroy(Semaphore self)
{
    LOG_ERR("Semaphore_destroy(sem=%p)", self);
    if (self) {
        GLOBAL_FREEMEM(self);
    }
}
