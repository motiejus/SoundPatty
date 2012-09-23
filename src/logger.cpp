#include "logger.h"

#ifndef HAVE_ASPRINTF
// asprintf stolen from: http://mingw-users.1079350.n2.nabble.com/
// Query-regarding-offered-alternative-to-asprintf-td6329481.html
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

int asprintf( char **, char *, ... );
int vasprintf( char **, char *, va_list );
FILE *ftrash = NULL;

int vasprintf( char **sptr, char *fmt, va_list argv )
{
        if(!ftrash) ftrash = fopen("nul", "wb"); /* on windows, /dev/null = nul */
        if(!ftrash) fprintf(stderr, "this shouldn't happen\n");
        int wanted = vfprintf( ftrash, fmt, argv );

        if( (wanted < 0) || ((*sptr = (char*)malloc( 1 + wanted )) == NULL) )
            return -1;
        (*sptr)[wanted] = '\0';
        return vsnprintf( *sptr, wanted, fmt, argv );
}

int asprintf( char **sptr, char *fmt, ... )
{
        int retval;
        va_list argv;
        va_start( argv, fmt );
        retval = vasprintf( sptr, fmt, argv );
               
        va_end( argv );
        return retval;
} 
#endif

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
                (char*)format)
            )
    {
        printf ("FATAL: Memory allocation failed!");
    }
    delete fileline;

    vfprintf (stderr, prefix, arg);
    va_end (arg);
}
