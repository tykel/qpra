/*
 * log.h -- Logging functions (header).
 *
 * Function declarations for log.c.
 *
 */

#ifndef QPRA_LOG_H
#define QPRA_LOG_H

void log_init(const char *);
void log_null(const char *, ...);
void log_verbose(const char *, ...);
void log_debug(const char *, ...);
void log_warn(const char *, ...);
void log_error(const char *, ...);
static void log_common(const char *, const char *, ...);
void log_end();

#ifndef LOG_LEVEL
#define LOG_LEVEL 1
#endif

#if LOG_LEVEL == 2
#define LOGV log_verbose
#define LOGD log_debug
#elif LOG_LEVEL == 1
#define LOGV log_null
#define LOGD log_debug
#else
#define LOGV log_null
#define LOGD log_null
#endif

#define LOGW log_warn
#define LOGE log_error

#endif

