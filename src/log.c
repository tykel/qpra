/*
 * log.c -- Logging functions.
 *
 * Wrapper functions which allow easy logging, possibly multiplexed to a file.
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <pthread.h>
#include "log.h"

FILE *g_logfile = NULL;
int g_uselog = 0;
extern struct timespec ts_start;

#ifdef _WIN32
static uint32_t log_gettid()
{
    return 0;
}
#else
#include <unistd.h>
#include <sys/syscall.h>

static uint32_t log_gettid()
{
    return (uint32_t) syscall(SYS_gettid); 
}
#endif

static struct timespec log_getelapsedtime()
{
    struct timespec ts_now, temp; 
	clock_gettime(CLOCK_REALTIME, &ts_now);
	if ((ts_now.tv_nsec - ts_start.tv_nsec) < 0) {
		temp.tv_sec = ts_now.tv_sec - ts_start.tv_sec - 1;
		temp.tv_nsec = 1000000000 + ts_now.tv_nsec - ts_start.tv_nsec;
	} else {
		temp.tv_sec = ts_now.tv_sec - ts_start.tv_sec;
		temp.tv_nsec = ts_now.tv_nsec - ts_start.tv_nsec;
	}
	return temp;
}

void log_init(const char *logfile)
{
    g_logfile = fopen(logfile, "w");
    if(g_logfile == NULL)
        LOGE("Could not open logfile");
    else
        g_uselog = 1;
}

void log_null(const char *format, ...)
{
}

void log_verbose(const char *format, ...)
{
    va_list arg;
    
    va_start(arg, format);
    printf("[VERBOSE] ");
    vprintf(format, arg);
    printf("\n");

    if(g_uselog) {
        fprintf(g_logfile, "[VERBOSE] ");
        vfprintf(g_logfile, format, arg);
        fprintf(g_logfile, "\n");
    }
    va_end(arg);
}
void log_debug(const char *format, ...)
{
    va_list arg;
    uint32_t tid = log_gettid();
    struct timespec ts_e = log_getelapsedtime();    

    va_start(arg, format);
    printf("%08u.%03u [% 6u][DEBUG  ] ",
           ts_e.tv_sec, ts_e.tv_nsec / 1000000, tid);
    vprintf(format, arg);
    printf("\n");

    if(g_uselog) {
        fprintf(g_logfile, "[DEBUG  ] ");
        vfprintf(g_logfile, format, arg);
        fprintf(g_logfile, "\n");
    }
    va_end(arg);
}

void log_warn(const char *format, ...)
{
    va_list arg;
    
    va_start(arg, format);
    printf("[WARNING] ");
    vprintf(format, arg);
    printf("\n");

    if(g_uselog) {
        fprintf(g_logfile, "[WARNING] ");
        vfprintf(g_logfile, format, arg);
        fprintf(g_logfile, "\n");
    }
    va_end(arg);
}

void log_error(const char *format, ...)
{
    va_list arg;
    
    va_start(arg, format);
    fprintf(stderr, "[ERROR  ] ");
    vfprintf(stderr, format, arg);
    fprintf(stderr, "\n");

    if(g_uselog) {
        fprintf(g_logfile, "[ERROR  ] ");
        vfprintf(g_logfile, format, arg);
        fprintf(g_logfile, "\n");
    }
    va_end(arg);
}

void log_end(void)
{
    if(g_logfile)
        fclose(g_logfile);
    g_logfile = NULL;
    g_uselog = 0;
}

