#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include "common.h"
#include "./handler_component/handler_component.h"
#include "./log/log.h"

#define IN_RANGE(value, min, max) (((value)>(min))&&((value)<(max))?EXIT_SUCCESS:EXIT_FAILURE)

// 定义全局数据恢复上下文
dataRec_Context *g_dataRec_Ctx = NULL;

void Data_Cutter(char *data, int datasize)
{
    strcpy(data+datasize, "\0");

    return;
}

void Data_Cleaner(char *data, int datasize)
{
    memset(data, datasize, sizeof(char));

    return;
}

void Convert_Size_To_Unit(const uint64_t source_Size, char *size_With_Unit, int radix)
{
  if(source_Size<(uint64_t)10*1024)
  {
    sprintf(size_With_Unit, "%uB", (unsigned)source_Size);
  }
  else if(source_Size<(uint64_t)10*1024*1024)
  {
    sprintf(size_With_Unit, "%uKB", (unsigned)(source_Size/radix));
  }
  else if(source_Size<(uint64_t)10*1024*1024*1024)
  {
    sprintf(size_With_Unit, "%uMB", (unsigned)(source_Size/radix/radix));
  }
  else if(source_Size<(uint64_t)10*1024*1024*1024*1024)
  {
    sprintf(size_With_Unit, "%uGB", (unsigned)(source_Size/radix/radix/radix));
  }
  else
  {
    sprintf(size_With_Unit, "%uTB", (unsigned)(source_Size/radix/radix/radix/radix));
  }

  return;
}

int Autoset_Arch_Type(disk_t *disk)
{
    if (NULL != disk->arch_autodetected)
    {
        disk->arch = disk->arch_autodetected;

        return EXIT_SUCCESS;
    }
    
    return EXIT_FAILURE;
}

int Array_ID_Matcher(const void *match_Member, const int match_Member_Len, const void *array_Member, const int array_Member_Len, const int array_Len)
{
    int i = 0;
    char *general_Array_Member = NULL;

    if ((NULL == (match_Member)) &&
        (NULL == (array_Member)) &&
        (0 == (array_Member_Len & array_Len)))
    {
        return -1;
    }
    
    // 将数组转化成通用数组（可以按字节读取）
    general_Array_Member = (char *)array_Member;

    for (; i<array_Len; ++i)
    {
        if (0 == memcmp(match_Member, general_Array_Member, match_Member_Len+1))
        {
            Log_Debug("Match Success, Command ID Is %d\n", i);
            return i;
        }

        // 按照数组成员长度，来读取下一个待匹配成员数据
        general_Array_Member += array_Member_Len;
    }

    Log_Debug("Match Fail, Command ID Is -1\n");

    return -1;
}

int DataRec_Context_Init(void)
{
    // 初始化数据恢复结构体
    if (NULL == g_dataRec_Ctx)
    {
        g_dataRec_Ctx = (dataRec_Context *)calloc(1, sizeof(dataRec_Context));
        if (NULL == g_dataRec_Ctx)
        {
            Log_Debug("DataRec_Context_Init: g_dataRec_Ctx Creat Fail\n"); 

            return EXIT_FAILURE;
        }
    }

    // 初始化server回传Buffer
    if (NULL == g_dataRec_Ctx->mg_websocket_write_Buffer)
    {
        size_t error_Info_Buffer_Size = 2048;
        g_dataRec_Ctx->mg_websocket_write_Buffer = (error_Info_Buffer *)calloc(1, error_Info_Buffer_Size);
        if (NULL == g_dataRec_Ctx->mg_websocket_write_Buffer)
        {
            Log_Debug("DataRec_Context_Init: mg_websocket_write_Buffer Creat Fail\n"); 

            return EXIT_FAILURE;
        }
    }

    // 初始化testdisk结构体
    if (NULL == g_dataRec_Ctx->testdisk_Ctx)
    {
        g_dataRec_Ctx->testdisk_Ctx = (testdisk_Context *)calloc(1, sizeof(testdisk_Context));
        if (NULL == g_dataRec_Ctx->testdisk_Ctx)
        {
            Log_Debug("DataRec_Context_Init: mg_websocket_write_Buffer Creat Fail\n"); 

            return EXIT_FAILURE;
        }
    }

    // 初始化物理磁盘结构体
    if (NULL == g_dataRec_Ctx->physical_Disk_Ctx)
    {
        g_dataRec_Ctx->physical_Disk_Ctx = (physical_Disk_Context *)calloc(1, sizeof(physical_Disk_Context));
        if (NULL == g_dataRec_Ctx->physical_Disk_Ctx)
        {
            Log_Debug("DataRec_Context_Init: physical_Disk_Ctx Creat Fail\n"); 

            return EXIT_FAILURE;
        }

        // 初始化物理磁盘信息
        if (NULL == g_dataRec_Ctx->physical_Disk_Ctx->info)
        {
            g_dataRec_Ctx->physical_Disk_Ctx->info = (physical_Disk_Info *)calloc(physical_Disk_Info_Num, sizeof(physical_Disk_Info));
            if (NULL == g_dataRec_Ctx->physical_Disk_Ctx->info)
            {
                Log_Debug("DataRec_Context_Init: physical_Disk_Ctx Creat Fail\n"); 

                return EXIT_FAILURE;
            }
        }
    }

    // 初始化逻辑磁盘结构体
    if (NULL == g_dataRec_Ctx->logical_Disk_Ctx)
    {
        g_dataRec_Ctx->logical_Disk_Ctx = (logical_Disk_Context *)calloc(1, sizeof(logical_Disk_Context));
        if (NULL == g_dataRec_Ctx->logical_Disk_Ctx)
        {
            Log_Debug("DataRec_Context_Init: logical_Disk_Ctx Creat Fail\n"); 

            return EXIT_FAILURE;
        }

        // 初始化逻辑磁盘信息
        if (NULL == g_dataRec_Ctx->logical_Disk_Ctx->info)
        {
            g_dataRec_Ctx->logical_Disk_Ctx->info = (logical_Disk_Info *)calloc(logical_Disk_Info_Num, sizeof(logical_Disk_Info));
            if (NULL == g_dataRec_Ctx->logical_Disk_Ctx->info)
            {
                Log_Debug("DataRec_Context_Init: logical_Disk_Ctx Creat Fail\n"); 

                return EXIT_FAILURE;
            }
        }
    }

    Log_Info("DataRec_Context_Init: Success\n"); 

    return EXIT_SUCCESS;
}

int DataRec_Context_Free(void)
{
    if (NULL != g_dataRec_Ctx)
    {
        // 清理server回传buffer
        if (NULL != g_dataRec_Ctx->mg_websocket_write_Buffer)
        {
            free(g_dataRec_Ctx->mg_websocket_write_Buffer);
        }

        // 清理testdisk结构体
        if (NULL != g_dataRec_Ctx->testdisk_Ctx)
        {
            free(g_dataRec_Ctx->testdisk_Ctx);
        }

        // 清理物理磁盘结构体   
        if (NULL != g_dataRec_Ctx->physical_Disk_Ctx)
        {
            free(g_dataRec_Ctx->physical_Disk_Ctx);
        }

        // 清理逻辑磁盘结构体   
        if (NULL != g_dataRec_Ctx->logical_Disk_Ctx)
        {
            free(g_dataRec_Ctx->logical_Disk_Ctx);
        }

        // 清理数据恢复结构体
        free(g_dataRec_Ctx);

        Log_Info("DataRec_Context_Free: Success\n"); 

        return EXIT_SUCCESS;
    }

    Log_Info("DataRec_Context_Free: Fail\n"); 

    return EXIT_FAILURE;
}

