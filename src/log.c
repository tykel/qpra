/*
 * log.c -- Logging functions.
 *
 * Wrapper functions which allow easy logging, possibly multiplexed to a file.
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include "log.h"

FILE *g_logfile = NULL;
int g_uselog = 0;

void log_init(const char *logfile)
{
    g_logfile = fopen(logfile, "w");
    if(g_logfile == NULL)
        LOGE("Could not open logfile");
    else
        g_uselog = 1;
}

void log_debug(const char *format, ...)
{
    va_list arg;
    
    va_start(arg, format);
    log_common("[DEBUG] ", format, arg);
    va_end(arg);
}

void log_warn(const char *format, ...)
{
    va_list arg;
    
    va_start(arg, format);
    log_common("[WARNING] ", format, arg);
    va_end(arg);
}

void log_error(const char *format, ...)
{
    va_list arg;
    
    va_start(arg, format);
    log_common("[ERROR] ", format, arg);
    va_end(arg);
}

static void log_common(const char *prefix, const char *format, ...)
{
    va_list arg;

    va_start(arg, format);
    
    printf(prefix);
    vprintf(format, arg);
    printf("\n");

    if(g_uselog) {
        fprintf(g_logfile, prefix);
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

