/*
 * Zephyr HAL stubs for time API
 * Minimal no-op implementations to satisfy linking.
 */

/*
 * Zephyr HAL for time API
 */
#include "hal_time.h"
#include <zephyr/kernel.h>
#include <time.h>


PAL_API msSinceEpoch
Hal_getTimeInMs(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
        return (msSinceEpoch)(ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL);
    }
    return 0;
}

PAL_API nsSinceEpoch
Hal_getTimeInNs(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
        return (nsSinceEpoch)((nsSinceEpoch)ts.tv_sec * 1000000000LL + ts.tv_nsec);
    }
    return 0;
}

PAL_API bool
Hal_setTimeInNs(nsSinceEpoch nsTime)
{
    struct timespec ts;
    ts.tv_sec = (time_t)(nsTime / 1000000000LL);
    ts.tv_nsec = (long)(nsTime % 1000000000LL);
    return (clock_settime(CLOCK_REALTIME, &ts) == 0);
}

PAL_API msSinceEpoch
Hal_getMonotonicTimeInMs(void)
{
    return (msSinceEpoch)k_uptime_get();
}

PAL_API nsSinceEpoch
Hal_getMonotonicTimeInNs(void)
{
    return (nsSinceEpoch)k_uptime_get() * 1000000LL;
}
