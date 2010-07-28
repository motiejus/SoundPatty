#include "logger.h"
int LogLevel = -1;
extern void log_mo (const int log_level, const char *file, const int line, const char *pretty_fn, const char *format, ...)
{
    if (log_level < LogLevel) return;

    va_list arg;
    va_start (arg, format);

    time_t rawtime; struct tm * timeinfo;
    char datetime [40];

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    strftime (datetime,40,"%Y-%m-%d %H:%M:%S,%s",timeinfo);
    datetime[24] = '\0'; // Cut to seconds^-4

    char *prefix;
    asprintf (&prefix, "%s [%x] (%s:%d) %s :: %s\n", datetime, (unsigned int)pthread_self(), file, line, pretty_fn, format);
    vfprintf (stderr, prefix, arg);
    va_end (arg);
}
