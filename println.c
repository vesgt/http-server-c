#include "println.h"
#include <stdarg.h>
#include <time.h>
#include <stdio.h>

void get_local_iso_date(char *date, size_t size) {
    time_t now = time(NULL);
    struct tm time;

    localtime_r(&now, &time);
    strftime(
        date,
        size,
        "%Y-%m-%d %H:%M:%S",
        &time
    );
}

void println(const char *format, ...) {
    va_list args;

    va_start(args, format);

    char date[64];
    get_local_iso_date(date, sizeof(date));
    printf("[%s] ", date);
    vprintf(format, args);
    printf("\n");

    va_end(args);
}
