#include <stdint.h>
#include <time.h>
#include <string.h>

/* POSIX signature */
struct tm *lb_gmtime_r(const time_t *t, struct tm *out)
{
#if defined(_POSIX_VERSION) || defined(__GLIBC__)
    return gmtime_r(t, out);
#else
    struct tm *tmp = gmtime(t);
    if (!tmp) return NULL;
    memcpy(out, tmp, sizeof(struct tm));
    return out;
#endif
}

static inline int is_leap(int y) {
    return ((y % 4) == 0 && (y % 100) != 0) || (y % 400) == 0;
}

/* GNU extension signature */
time_t lb_timegm(struct tm *tm)
{
    int y  = tm->tm_year + 1900;
    int m  = tm->tm_mon + 1;   /* 1..12 */
    int d  = tm->tm_mday;
    int hh = tm->tm_hour, mm = tm->tm_min, ss = tm->tm_sec;

    static const int mdays[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    long days = 0;

    for (int year = 1970; year < y; ++year)
        days += 365 + is_leap(year);

    for (int month = 1; month < m; ++month) {
        days += mdays[month-1];
        if (month == 2 && is_leap(y)) days += 1;
    }

    days += (d - 1);

    long long secs = (long long)days * 86400LL + hh * 3600 + mm * 60 + ss;
    return (time_t)secs;
}

struct tm *lb_localtime_r(const time_t *t, struct tm *out) {
    /* On embedded: treat localtime as UTC */
    return lb_gmtime_r(t, out);
}