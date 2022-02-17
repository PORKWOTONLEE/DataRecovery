#ifndef __LOG_H__
#define __LOG_H__

#include <stdarg.h>

int Log_Open(const char *filename, const int mode, int *errsv);
int Log_Redirect(const unsigned int level, const char *format, ...) __attribute__((format(printf, 2, 3)));
int Log_Handler(const char *_format, va_list ap) __attribute__((format(printf, 1, 0)));
int Log_Flush(void);
int Log_Close(void);

// Log刷新频率，每n条刷新一次
#define LOG_FRESH_FREQUENCY 1

#define LOG_NONE   0
#define LOG_CREATE 1
#define LOG_APPEND 2

// Log等级
#define LOG_LEVEL_INFO     (1 << 0) 
#define LOG_LEVEL_DEBUG    (1 << 1) 

// Log信息重定向
#define Log_Info(FORMAT, ARGS...)  Log_Redirect(LOG_LEVEL_INFO,FORMAT,##ARGS)
#define Log_Debug(FORMAT, ARGS...) Log_Redirect(LOG_LEVEL_DEBUG,FORMAT,##ARGS)

#endif
