/*
 * Zephyr HAL stubs for time API
 * Minimal no-op implementations to satisfy linking.
 */

#include "hal_time.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(libiec_hal_time, LOG_LEVEL_ERR);

PAL_API msSinceEpoch
Hal_getTimeInMs(void)
{
    LOG_ERR("Hal_getTimeInMs()");
    return 0;
}

PAL_API nsSinceEpoch
Hal_getTimeInNs(void)
{
    LOG_ERR("Hal_getTimeInNs()");
    return 0;
}

PAL_API bool
Hal_setTimeInNs(nsSinceEpoch nsTime)
{
    LOG_ERR("Hal_setTimeInNs(ns=%llu)", (unsigned long long)nsTime);
    (void)nsTime;
    return false;
}

PAL_API msSinceEpoch
Hal_getMonotonicTimeInMs(void)
{
    LOG_ERR("Hal_getMonotonicTimeInMs()");
    return 0;
}

PAL_API nsSinceEpoch
Hal_getMonotonicTimeInNs(void)
{
    LOG_ERR("Hal_getMonotonicTimeInNs()");
    return 0;
}
