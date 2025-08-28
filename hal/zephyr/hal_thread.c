/*
 * Zephyr HAL for thread/semaphore API
 */

#include "hal_thread.h"
#include "lib_memory.h"
#include <zephyr/kernel.h>

/* Thread wrapper and small static pool for stacks/threads */
struct sThread {
    ThreadExecutionFunction function;
    void* parameter;
    bool autodestroy;
    int slot;
};

#define HAL_THREAD_STACK_SIZE 4096
#define HAL_THREAD_MAX_SLOTS  4

K_THREAD_STACK_DEFINE(hal_thread_stack0, HAL_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(hal_thread_stack1, HAL_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(hal_thread_stack2, HAL_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(hal_thread_stack3, HAL_THREAD_STACK_SIZE);

static struct k_thread hal_threads[HAL_THREAD_MAX_SLOTS];
static k_thread_stack_t *const hal_stacks[HAL_THREAD_MAX_SLOTS] = {
    hal_thread_stack0, hal_thread_stack1, hal_thread_stack2, hal_thread_stack3
};
static volatile bool hal_slot_used[HAL_THREAD_MAX_SLOTS];

static void hal_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p2); ARG_UNUSED(p3);
    Thread t = (Thread)p1;
    if (t && t->function)
        (void)t->function(t->parameter);
    if (t) {
        int s = t->slot;
        if (t->autodestroy)
            GLOBAL_FREEMEM(t);
        if (s >= 0 && s < HAL_THREAD_MAX_SLOTS)
            hal_slot_used[s] = false;
    }
}

static int hal_alloc_slot(void)
{
    for (int i = 0; i < HAL_THREAD_MAX_SLOTS; ++i) {
        if (!hal_slot_used[i]) {
            hal_slot_used[i] = true;
            return i;
        }
    }
#if defined(CONFIG_LOG) || defined(LIBIEC_ZEPHYR_DEBUG)
    printk("[libiec][thr] no free thread slot (max=%d)\n", HAL_THREAD_MAX_SLOTS);
#endif
    return -1;
}

PAL_API Thread
Thread_create(ThreadExecutionFunction function, void* parameter, bool autodestroy)
{
    Thread t = (Thread)GLOBAL_CALLOC(1, sizeof(struct sThread));
    if (t) {
        t->function = function;
        t->parameter = parameter;
        t->autodestroy = autodestroy;
        t->slot = -1;
    }
    return t;
}

PAL_API void
Thread_start(Thread thread)
{
    if (!thread || !thread->function)
        return;

    if (thread->slot < 0) {
        int s = hal_alloc_slot();
        if (s < 0) {
#if defined(CONFIG_LOG) || defined(LIBIEC_ZEPHYR_DEBUG)
            printk("[libiec][thr] Thread_start: allocation failed\n");
#endif
            return; /* no slot available */
        }
        thread->slot = s;
    }

    int s = thread->slot;
    k_thread_create(&hal_threads[s], hal_stacks[s], HAL_THREAD_STACK_SIZE,
                    hal_thread_entry, thread, NULL, NULL,
                    K_PRIO_PREEMPT(8), 0, K_NO_WAIT);
#if defined(CONFIG_LOG) || defined(LIBIEC_ZEPHYR_DEBUG)
    printk("[libiec][thr] thread started slot=%d func=%p param=%p\n", s, thread->function, thread->parameter);
#endif
}

PAL_API void
Thread_destroy(Thread thread)
{
    if (thread) {
        /* Cannot force-stop Zephyr threads here; rely on autodestroy/exit */
        GLOBAL_FREEMEM(thread);
    }
}

PAL_API void
Thread_sleep(int millies)
{
    k_msleep(millies);
}

struct hal_sem_wrap { struct k_sem sem; };

PAL_API Semaphore
Semaphore_create(int initialValue)
{
    struct hal_sem_wrap *w = (struct hal_sem_wrap*)GLOBAL_CALLOC(1, sizeof(struct hal_sem_wrap));
    if (!w) return NULL;
    k_sem_init(&w->sem, initialValue, K_SEM_MAX_LIMIT);
    return (Semaphore)w;
}

PAL_API void
Semaphore_wait(Semaphore self)
{
    if (!self) return;
    struct hal_sem_wrap *w = (struct hal_sem_wrap*)self;
    (void)k_sem_take(&w->sem, K_FOREVER);
}

PAL_API void
Semaphore_post(Semaphore self)
{
    if (!self) return;
    struct hal_sem_wrap *w = (struct hal_sem_wrap*)self;
    k_sem_give(&w->sem);
}

PAL_API void
Semaphore_destroy(Semaphore self)
{
    if (!self) return;
    GLOBAL_FREEMEM(self);
}
