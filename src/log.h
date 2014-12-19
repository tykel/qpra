/*
 * log.h -- Logging functions (header).
 *
 * Function declarations for log.c.
 *
 */

#ifndef QPRA_LOG_H
#define QPRA_LOG_H

void log_init(const char *);
void log_debug(const char *, ...);
void log_warn(const char *, ...);
void log_error(const char *, ...);
static void log_common(const char *, const char *, ...);
void log_end();

#define LOGD log_debug
#define LOGW log_warn
#define LOGE log_error

#endif

