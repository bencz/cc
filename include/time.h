#define time_t int

typedef struct _tm {
    int tm_sec;  /* Seconds: 0-59 (K&R says 0-61?) */
    int tm_min;  /* Minutes: 0-59 */
    int tm_hour; /* Hours since midnight: 0-23 */
    int tm_mday; /* Day of the month: 1-31 */
    int tm_mon;  /* Months *since* january: 0-11 */
    int tm_year; /* Years since 1900 */
    int tm_wday; /* Days since Sunday (0-6) */
    int tm_yday; /* Days since Jan. 1: 0-365 */
    int tm_isdst;/* +1 Daylight Savings Time, 0 No DST, -1 don't know */
} tm;

int clock();
time_t time(time_t *timer);
time_t mktime(tm *tp);
tm *localtime(const time_t *timer);

