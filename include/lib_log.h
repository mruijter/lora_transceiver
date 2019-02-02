#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>

#if !defined PRGNAME
#define PRGNAME "logger"
#endif
#define FACILITY LOG_LOCAL0
#define PRIORITY LOG_NOTICE

int log (int level, const char *fmt, ...);
