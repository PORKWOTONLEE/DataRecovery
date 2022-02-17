#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include "log.h"

static FILE *log_handle = NULL;
static int f_status = EXIT_SUCCESS;
static int log_times = 0;
static unsigned int log_levels = LOG_LEVEL_INFO | LOG_LEVEL_DEBUG;

int Log_Open(const char *filename, const int mode, int *errsv)
{
    if (LOG_NONE == mode)
    {
        log_handle = NULL;
        return EXIT_FAILURE;
    }

    // 打开日志文件
    log_handle = fopen(filename, ((mode==LOG_CREATE)?"w":"a"));
    *errsv = errno;
    if (fprintf(log_handle, "\n")<=0 || fflush(log_handle)!=0)
    {
        fclose(log_handle);
        log_handle = fopen(filename, "w");
        *errsv = errno;
    }

    // 再次判断打开情况
    if (NULL == log_handle)
    {
        return EXIT_FAILURE;
    }
    else
    {
        return EXIT_SUCCESS;
    }
}

int Log_Redirect(const unsigned int level, const char *format, ...)
{
    if (NULL == log_handle)
    {
        return EXIT_SUCCESS;
    }

    // 判断Log等级是否有效
    if (0 == (log_levels & level))
    {
        return EXIT_FAILURE;
    }

    // 输出Log信息
    {
        int log_handle_result;
        va_list parg;
        va_start(parg, format);
        log_handle_result = Log_Handler(format, parg);
        va_end(parg);

        // 清空缓冲区，将日志写入文件，写入频率LOG_FRESH_FREQUENCY可调整
        ++log_times;
        if (LOG_FRESH_FREQUENCY == log_times)
        {
            Log_Flush();
            log_times = 0;
        }

        return log_handle_result;
    }
}

int Log_Handler(const char *_format, va_list parg)
{
    // 将格式化语句+可变参数输出到缓冲区
    int log_handle_result;
    log_handle_result = vfprintf(log_handle, _format, parg);

    if (log_handle_result < 0)
    {
        f_status = EXIT_FAILURE;
    }

    return f_status;
}

int Log_Flush(void)
{
    return fflush(log_handle);
}

int Log_Close(void)
{
    if (NULL != log_handle)
    {
        if (fclose(log_handle))
        {
            f_status = EXIT_SUCCESS;
            log_handle = NULL;
        }
    }

    return f_status;
}

