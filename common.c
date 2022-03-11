#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include "./common.h"
#include "./handler_component/handler_component.h"
#include "./log/log.h"

#define CALLOC_WITH_CHECK(POINTER, POINTER_TYPE, NUM, SIZE)  (POINTER)=POINTER_TYPE calloc((NUM),(SIZE));\
                                                          if(NULL == (POINTER))\
                                                          {return EXIT_FAILURE;}
#define FREE_WITH_CHECK(POINTER) if(NULL != (POINTER))\
                                 {free(POINTER);}\
                                 else\
                                 {return EXIT_FAILURE;}

// 定义我的数据上下文
my_Data_Context *g_my_Data_Ctx = NULL;

void File_Info_Clear(void)
{
    data_Recovery_Context *data_Rec_Ctx = g_my_Data_Ctx->data_Rec_Ctx;
    
    memset(data_Rec_Ctx->file_Ctx->info, 0, FILE_INFO_NUM*sizeof(file_Info));
}

static void Server_Context_Clear(void)
{
    server_Context *server_Ctx =g_my_Data_Ctx->server_Ctx;

    // 清空server环境
    // 清空接收环境
    memset(server_Ctx->receive_Ctx->receive_Buffer, 0, RECEIVE_BUFFER_SIZE);
    strcpy(server_Ctx->receive_Ctx->datarec, " ");
    strcpy(server_Ctx->receive_Ctx->action,  " ");
    strcpy(server_Ctx->receive_Ctx->command, " ");
    server_Ctx->receive_Ctx->command_ID = 0;
    memset(server_Ctx->receive_Ctx->param, 0, sizeof(server_Ctx->receive_Ctx->param));
    server_Ctx->receive_Ctx->param_Num = 0;

    // 清空响应环境
    memset(server_Ctx->respond_Ctx->respond_Buffer, 0, RESPONSE_BUFFER_SIZE);
}

void Data_Cleaner(void)
{
    Server_Context_Clear();

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
        Log_Debug("Match Fail, Something Check Fail\n");

        return -1;
    }
    
    // 将数组转化成通用数组（可以按字节读取）
    general_Array_Member = (char *)array_Member;

    // 开始匹配
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

char *Get_Action(void)
{
    receive_Context *receive_Ctx = g_my_Data_Ctx->server_Ctx->receive_Ctx;

    return receive_Ctx->action;
}

unsigned int Get_Param_Num(void)
{
    receive_Context *receive_Ctx = g_my_Data_Ctx->server_Ctx->receive_Ctx;

    return receive_Ctx->param_Num;
}

char *Get_Param(unsigned int param_No)
{
    receive_Context *receive_Ctx = g_my_Data_Ctx->server_Ctx->receive_Ctx;

    if ((0 != Get_Param_Num()) && (param_No>=0) && (param_No<=Get_Param_Num()))
    {
        return receive_Ctx->param[param_No];
    }

    return NULL;
}

static int Server_Context_Init(void)
{
    // 初始化服务器环境
    CALLOC_WITH_CHECK(g_my_Data_Ctx->server_Ctx, (server_Context *), 1, sizeof(server_Context));

    // 初始化数据接收环境
    CALLOC_WITH_CHECK(g_my_Data_Ctx->server_Ctx->receive_Ctx, (receive_Context *), 1, sizeof(receive_Context));

    // 初始化数据接收buffer
    CALLOC_WITH_CHECK(g_my_Data_Ctx->server_Ctx->receive_Ctx->receive_Buffer, (char *), RECEIVE_BUFFER_SIZE, sizeof(char));

    // 初始化数据响应环境
    CALLOC_WITH_CHECK(g_my_Data_Ctx->server_Ctx->respond_Ctx, (respond_Context *), 1, sizeof(respond_Context));

    // 初始化数据响应buffer
    CALLOC_WITH_CHECK(g_my_Data_Ctx->server_Ctx->respond_Ctx->respond_Buffer, (char *), RESPONSE_BUFFER_SIZE, sizeof(char));

    return EXIT_SUCCESS;
}

static int Testdisk_Context_Init(void)
{
    // 初始化testdisk环境
    CALLOC_WITH_CHECK(g_my_Data_Ctx->testdisk_Ctx, (testdisk_Context *), 1, sizeof(testdisk_Context));

    // 初始化testdisk恢复文件环境
    CALLOC_WITH_CHECK(g_my_Data_Ctx->testdisk_Ctx->testdisk_File_Ctx, (testdisk_File_Context *), 1, sizeof(testdisk_File_Context));

    // 初始化photorec恢复文件环境
    CALLOC_WITH_CHECK(g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx, (photorec_File_Context*), 1, sizeof(photorec_File_Context));

    return EXIT_SUCCESS;
}

static int DataRec_Context_Init(void)
{
    // 初始化数据恢复环境
    CALLOC_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx, (data_Recovery_Context *), 1, sizeof(data_Recovery_Context));

    // 初始化物理磁盘环境
    CALLOC_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->physical_Disk_Ctx, (physical_Disk_Context *), 1, sizeof(physical_Disk_Context));

    // 初始化物理磁盘信息
    CALLOC_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->physical_Disk_Ctx->info, (physical_Disk_Info *), PHYSICAL_DISK_INFO_NUM, sizeof(physical_Disk_Info));

    // 初始化逻辑磁盘环境
    CALLOC_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->logical_Disk_Ctx, (logical_Disk_Context *), 1, sizeof(logical_Disk_Context));

    // 初始化逻辑磁盘信息
    CALLOC_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->logical_Disk_Ctx->info, (logical_Disk_Info *), LOGICAL_DISK_INFO_NUM, sizeof(logical_Disk_Info));

    // 初始化文件列表环境
    CALLOC_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->file_Ctx, (recovery_File_Context *), 1, sizeof(recovery_File_Context));

    // 初始化文件列表信息
    CALLOC_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->file_Ctx->info, (file_Info *), FILE_INFO_NUM, sizeof(file_Info));

    return EXIT_SUCCESS;
}

static int DataRec_Context_Free(void)
{
    // 释放文件列表信息
    FREE_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->file_Ctx->info);

    // 释放文件列表结构体
    FREE_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->file_Ctx);

    // 释放逻辑磁盘信息
    FREE_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->logical_Disk_Ctx->info);

    // 释放逻辑磁盘结构体
    FREE_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->logical_Disk_Ctx);

    // 释放物理磁盘信息
    FREE_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->physical_Disk_Ctx->info);

    // 释放物理磁盘结构体
    FREE_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->physical_Disk_Ctx);

    FREE_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx);

    return EXIT_SUCCESS;
}

static int Testdisk_Context_Free(void)
{
    // 释放photorec恢复文件结构体
    FREE_WITH_CHECK(g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx);

    // 释放testdisk恢复文件结构体
    FREE_WITH_CHECK(g_my_Data_Ctx->testdisk_Ctx->testdisk_File_Ctx);

    FREE_WITH_CHECK(g_my_Data_Ctx->testdisk_Ctx);

    return EXIT_SUCCESS;
}

static int Server_Context_Free(void)
{
    // 释放数据接收buffer
    FREE_WITH_CHECK(g_my_Data_Ctx->server_Ctx->receive_Ctx->receive_Buffer);

    // 释放数据接收结构体
    FREE_WITH_CHECK(g_my_Data_Ctx->server_Ctx->receive_Ctx);

    // 释放数据响应buffer
    FREE_WITH_CHECK(g_my_Data_Ctx->server_Ctx->respond_Ctx->respond_Buffer);

    // 释放数据响应结构体
    FREE_WITH_CHECK(g_my_Data_Ctx->server_Ctx->respond_Ctx);

    FREE_WITH_CHECK(g_my_Data_Ctx->server_Ctx);

    return EXIT_SUCCESS;
}

int My_Data_Context_Init(void)
{
    // 初始化我的数据结构体
    CALLOC_WITH_CHECK(g_my_Data_Ctx, (my_Data_Context *), 1, sizeof(my_Data_Context));
    if ((EXIT_SUCCESS == Server_Context_Init()) &&
        (EXIT_SUCCESS == Testdisk_Context_Init()) &&
        (EXIT_SUCCESS == DataRec_Context_Init()))
    {
        Log_Info("My_Data_Context_Init: Success\n"); 

        return EXIT_SUCCESS;
    }
    else 
    {
        Log_Info("My_Data_Context_Init: Fail\n"); 

        return EXIT_FAILURE;
    }
}

int My_Data_Context_Free(void)
{
    // 和初始化顺序相反
    
    // 释放我的数据结构体
    if ((EXIT_SUCCESS == DataRec_Context_Init()) &&
        (EXIT_SUCCESS == Testdisk_Context_Init()) &&
        (EXIT_SUCCESS == Server_Context_Init()))
    {
        FREE_WITH_CHECK(g_my_Data_Ctx);
        Log_Info("My_Data_Context_Free: Success\n"); 

        return EXIT_SUCCESS;
    }
    else 
    {
        Log_Info("My_DataRec_Context_Free: Fail\n"); 

        return EXIT_FAILURE;
    }
}

