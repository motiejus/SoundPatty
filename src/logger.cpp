#include "logger.h"

#ifdef HAVE_PTHREAD
#define LOG_LINE "%s [%x] %-19s %-5s - %s\n"
#else
#define LOG_LINE "%s [?] %-19s %-5s - %s\n"
#endif

int LogLevel = 3;
const char *LogLevels[] = {"FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"};
extern void log_mo (const int log_level, const char *file, const int line,
        const char *pretty_fn, const char *format, ...) {
    if (log_level > LogLevel) return;
    (void)pretty_fn;

    va_list arg;
    va_start (arg, format);

    struct timeval tv; struct timezone tz; struct tm *tm; time_t rawtime;
    gettimeofday(&tv, &tz);
    tm = localtime(&tv.tv_sec);

    /* Now we have tv.tv_usec
    printf(" %d:%02d:%02d %d \n", tm->tm_hour, tm->tm_min,
            tm->tm_sec, tv.tv_usec);
    */
    char datetime [40];
    time ( &rawtime );
    tm = localtime ( &rawtime );

    strftime (datetime,40,"%Y-%m-%d %H:%M:%S",tm);
    sprintf (datetime, "%s,%d", datetime, (int)tv.tv_usec);
    datetime[24] = '\0'; // Cut to seconds^-4

    char *prefix, *fileline;

    fileline = (char*)malloc(strlen(file)+27);
    sprintf(fileline, "%s(%d)", file, line);
    // &fileline[7] due to skipping first 7 characters
    // (with default waf build it's '../src/'
    if ( -1 == asprintf (&prefix, LOG_LINE,
                datetime,
#ifdef HAVE_PTHREAD
                (unsigned int)pthread_self(),
#endif
                &fileline[7],
                LogLevels[log_level],
                format)
            )
    {
        printf ("FATAL: Memory allocation failed!");
    }
    delete fileline;

    vfprintf (stderr, prefix, arg);
    va_end (arg);
}
