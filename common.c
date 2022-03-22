#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <windows.h>
#include <securitybaseapi.h>
#include <winbase.h>
#include "libdatarec/testdisk/src/file_list.c"
#include "handler_component/handler_component.h"
#include "log/log.h"
#include "common.h"

#ifndef BOOL
#define BOOL int
#endif

// 带错误返回的calloc
#define CALLOC_WITH_CHECK(POINTER, POINTER_TYPE, NUM, SIZE)  (POINTER)=POINTER_TYPE calloc((NUM),(SIZE));\
                                                          if(NULL == (POINTER))\
                                                          {return EXIT_FAILURE;}

// 带错误说明的free
#define FREE_WITH_CHECK(POINTER) if(NULL != (POINTER))\
                                 {free(POINTER);}\
                                 else\
                                 {return EXIT_FAILURE;}

extern list_disk_t *hd_parse(list_disk_t *list_disk, const int verbose, const int testdisk_mode);
extern void hd_update_all_geometry(const list_disk_t * list_disk, const int verbose);
extern int delete_list_disk(list_disk_t *list_disk);
extern void autodetect_arch(disk_t *disk, const arch_fnct_t *arch);
extern void autoset_unit(disk_t *disk_car);
extern int interface_check_disk_capacity(disk_t *disk_car);
extern int interface_check_disk_access(disk_t *disk_car, char **current_cmd);
extern void part_free_list(list_part_t *list_part);
extern unsigned int delete_list_file(file_info_t *list);
extern dir_partition_t dir_partition_ntfs_init(disk_t *disk_car, const partition_t *partition, dir_data_t *dir_data, const int verbose, const int expert);

// 定义我的数据环境
my_Data_Context *g_my_Data_Ctx = NULL;

// 深度扫描文件支持列表
file_enable_t gSA_deep_Scan_File_Extension_List[]=
{
    // 图片
    {.file_hint=&file_hint_gif,  .enable=1},
    {.file_hint=&file_hint_png,  .enable=1},
    {.file_hint=&file_hint_bmp,  .enable=1},
    {.file_hint=&file_hint_jpg,  .enable=0},
    {.file_hint=&file_hint_raw,  .enable=1},
    {.file_hint=&file_hint_tiff, .enable=1},
    {.file_hint=&file_hint_pdf,  .enable=1},
    {.file_hint=&file_hint_psd,  .enable=1},

    // 音乐
    {.file_hint=&file_hint_ogg,  .enable=1},
    {.file_hint=&file_hint_aif,  .enable=1},
    {.file_hint=&file_hint_mid,  .enable=1},
    {.file_hint=&file_hint_mp3,  .enable=1},
    {.file_hint=&file_hint_flac, .enable=1},
    {.file_hint=&file_hint_ape,  .enable=1},

    // 视频
    {.file_hint=&file_hint_mkv,  .enable=1},
    {.file_hint=&file_hint_rm,   .enable=1},
    {.file_hint=&file_hint_flv,  .enable=1},
    {.file_hint=&file_hint_mov,  .enable=1},

    // 文档
    {.file_hint=&file_hint_zip,  .enable=1},
    {.file_hint=&file_hint_iso,  .enable=1},
    {.file_hint=&file_hint_gz,   .enable=1},
    {.file_hint=&file_hint_doc,  .enable=1},
    {.file_hint=&file_hint_xml,  .enable=1},
    {.file_hint=&file_hint_txt,  .enable=1},
    {.file_hint=&file_hint_exe,  .enable=0},

    {.file_hint=NULL,            .enable=1} 
};

// 深度扫描文件支持列表长度
const unsigned int ggSA_deep_Scan_File_Extension_List_Len = sizeof(gSA_deep_Scan_File_Extension_List)/sizeof(gSA_deep_Scan_File_Extension_List[0]) - 1;

void File_Info_Clear(void)
{
    data_Recovery_Context *data_Rec_Ctx = g_my_Data_Ctx->data_Rec_Ctx;
    
    memset(data_Rec_Ctx->file_Ctx->info, 0, FILE_NUM*sizeof(file_Info));
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

char *Convert_Status_To_Char(status_Code status)
{
    switch (status)
    {
        case IDLE:
            return "IDLE";
            break;
        case QUICK_SCANNING:
            return "QUICK_SCANNING";
            break;
        case DEEP_SCANNING:
            return "DEEP_SCANNING";
            break;
        default:
            return "IDLE";
            break;
    }
}

char *Convert_List_Available_To_Char(scan_File_List_Available_Code available)
{
    switch (available)
    {
        case LIST_NOT_READY:
            return "LIST_NOT_READY";
            break;
        case LIST_READY:
            return "LIST_READY";
            break;
        default:
            return "LIST_NOT_READY";
            break;
    }
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

static disk_t *Access_Disk(int disk_No)
{
    int i = 0;
    
    if (disk_No>=0 && disk_No<g_my_Data_Ctx->data_Rec_Ctx->disk_Ctx->total_Num)
    {
        DISK_LIST_ITERATOR(g_my_Data_Ctx->testdisk_Ctx->disk_List)
        {
            if (i == disk_No)
            {
                return list_Walker->disk;
            }

            ++i;
        }
    }

    return NULL;
}

static partition_t *Accecc_Partition(int disk_No, int partition_No)
{
    disk_Context *disk_Ctx = g_my_Data_Ctx->data_Rec_Ctx->disk_Ctx;
    partition_Context *partition_Ctx = g_my_Data_Ctx->data_Rec_Ctx->partition_Ctx;
    testdisk_Context *testdisk_Ctx = g_my_Data_Ctx->testdisk_Ctx;
    int min_Partition_No = 0;
    int max_Partition_No = 0;
    int i = 0;
    int j = 0;
    
    if ((disk_No>=0) && (disk_No<disk_Ctx->total_Num))
    {
        DISK_LIST_ITERATOR(testdisk_Ctx->disk_List)
        {
            if (i == disk_No)
            {
                max_Partition_No = min_Partition_No + disk_Ctx->partition_In_Per_Disk[i];
                break;
            }
            min_Partition_No += disk_Ctx->partition_In_Per_Disk[i];
            ++i;
        }
    }

    if ((partition_No>=min_Partition_No) && (partition_No<max_Partition_No))
    {
        PARTITION_LIST_ITERATOR(testdisk_Ctx->partition_List[disk_No])
        {
            if (j == partition_No-min_Partition_No)
            {
                return list_Walker->part;
            }

            ++j;
        }
    }

    return NULL;
}

void Move_File(const char *dest, const char *src)
{
    MoveFile(src, dest);

    return;
}

void Delete_File(const char *file_Name)
{
    DeleteFile(file_Name);

    return;
}

void Set_Request_Buffer(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);

    vsprintf(g_my_Data_Ctx->server_Ctx->request_Ctx->request_Buffer, format, ap);

    va_end(ap);
}

char *Get_Request_Buffer(void)
{
    return g_my_Data_Ctx->server_Ctx->request_Ctx->request_Buffer;
}

void Request_Buffer_Reset(void)
{
    memset(g_my_Data_Ctx->server_Ctx->request_Ctx->request_Buffer, 0, REQUEST_BUFFER_SIZE);

    return;
}

void Set_DataRec(char *datarec)
{
    strcpy(g_my_Data_Ctx->server_Ctx->request_Ctx->datarec, datarec);

    return;
}

char *Get_DataRec(void)
{
    return g_my_Data_Ctx->server_Ctx->request_Ctx->datarec;
}

void Set_Action(char *action)
{
    strcpy(g_my_Data_Ctx->server_Ctx->request_Ctx->action, action);

    return;
}

char *Get_Action(void)
{
    return g_my_Data_Ctx->server_Ctx->request_Ctx->action;
}

void Set_Command(char *command)
{
    strcpy(g_my_Data_Ctx->server_Ctx->request_Ctx->command, command);

    return;
}

char *Get_Command(void)
{
    return g_my_Data_Ctx->server_Ctx->request_Ctx->command;
}

void Set_Command_ID(int command_ID)
{
    g_my_Data_Ctx->server_Ctx->request_Ctx->command_ID = command_ID;

    return;
}

unsigned int Get_Command_ID(void)
{
    return g_my_Data_Ctx->server_Ctx->request_Ctx->command_ID;
}

void Set_Param(unsigned int param_No, char *param)
{
    strcpy((char *)&g_my_Data_Ctx->server_Ctx->request_Ctx->param[param_No], param);

    return;
}

char *Get_Char_Param(unsigned int param_No)
{
    if ((0 != Get_Param_Num()) && (param_No>=0) && (param_No<=Get_Param_Num()))
    {
        return (char *)&g_my_Data_Ctx->server_Ctx->request_Ctx->param[param_No];
    }

    return NULL;
}

int Get_Int_Param(unsigned int param_No)
{
    return atoi(Get_Char_Param(param_No));
}

void Set_Param_Num(int param_Num)
{
    g_my_Data_Ctx->server_Ctx->request_Ctx->param_Num = param_Num;

    return;
}

void Param_Reset(void)
{
    memset(g_my_Data_Ctx->server_Ctx->request_Ctx->param,
           0, 
           sizeof(g_my_Data_Ctx->server_Ctx->request_Ctx->param));

    return;
}

unsigned int Get_Param_Num(void)
{
    return g_my_Data_Ctx->server_Ctx->request_Ctx->param_Num;
}

void Set_Respond_Buffer(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);

    vsprintf(g_my_Data_Ctx->server_Ctx->respond_Ctx->respond_Buffer, format, ap);

    va_end(ap);
}

char *Get_Respond_Buffer(void)
{
    return g_my_Data_Ctx->server_Ctx->respond_Ctx->respond_Buffer;
}

void Respond_Buffer_Reset(void)
{
    memset(g_my_Data_Ctx->server_Ctx->respond_Ctx->respond_Buffer, 0, RESPONSE_BUFFER_SIZE);

    return;
}

void Set_Result(error_Code result)
{
    g_my_Data_Ctx->server_Ctx->respond_Ctx->result = result;

    return;
}

error_Code Get_Result(void)
{
    return g_my_Data_Ctx->server_Ctx->respond_Ctx->result;
}

void Server_Context_Reset(void)
{
    server_Context *server_Ctx =g_my_Data_Ctx->server_Ctx;

    // 重置请求环境
    Request_Buffer_Reset();
    Set_DataRec("");
    Set_Action("");
    Set_Command("");
    Set_Command_ID(0);
    Param_Reset();
    Set_Param_Num(0);

    // 重置响应环境
    Respond_Buffer_Reset();
    Set_Result(ERROR_NONE_OK);

    return;
}

void Set_Status(status_Code status)
{
    g_my_Data_Ctx->data_Rec_Ctx->info_Ctx->status = status;

    return;
}

status_Code Get_Status(void)
{
    return g_my_Data_Ctx->data_Rec_Ctx->info_Ctx->status;
}

void Set_Control(control_Code control)
{
    g_my_Data_Ctx->data_Rec_Ctx->info_Ctx->process_Ctx->control = control;

    return;
}

control_Code Get_Control(void)
{
    return g_my_Data_Ctx->data_Rec_Ctx->info_Ctx->process_Ctx->control;
}

void Set_Percent(double percent)
{
    static double percent_cmp;

    if (percent > percent_cmp)
    {
        percent_cmp = percent;
        g_my_Data_Ctx->data_Rec_Ctx->info_Ctx->process_Ctx->percent = percent;
    }
    else if (0.0 == percent)
    {
        percent_cmp = percent;
        g_my_Data_Ctx->data_Rec_Ctx->info_Ctx->process_Ctx->percent = percent;
    }

    return;
}

double Get_Percent(void)
{
    return g_my_Data_Ctx->data_Rec_Ctx->info_Ctx->process_Ctx->percent;
}

void Info_Context_Reset(void)
{
    // 重置datarec状态
    Set_Status(IDLE);

    // 重置搜索流程
    Set_Control(SCAN_CONTINUE);
    Set_Quick_Scan_File_List_Available(LIST_NOT_READY);
    Set_Deep_Scan_File_List_Available(LIST_NOT_READY);
    Set_Percent(0.0);

    return;
}

void Disk_List_Init(char *error_Buffer)
{
    int testdisk_Mode = TESTDISK_O_RDWR|TESTDISK_O_READAHEAD_8K; // 磁盘读取模式
    int verbose = 1;
    
    g_my_Data_Ctx->testdisk_Ctx->disk_List = hd_parse(g_my_Data_Ctx->testdisk_Ctx->disk_List, verbose, testdisk_Mode);

    if (NULL == g_my_Data_Ctx->testdisk_Ctx->disk_List)
    {
        Set_Respond_Buffer("Can't Parse Disk, Please Run With Administator Access");
        
        return ;
    }

    hd_update_all_geometry(g_my_Data_Ctx->testdisk_Ctx->disk_List, verbose);

    return ;
}

void Disk_List_Free(void)
{
    if (NULL != g_my_Data_Ctx->testdisk_Ctx->disk_List)
    {
        delete_list_disk(g_my_Data_Ctx->testdisk_Ctx->disk_List);
    }
    g_my_Data_Ctx->testdisk_Ctx->disk_List= NULL;

    return;
}

void Partition_List_Init()
{
    char *current_Cmd = NULL;
    int verbose = 1;
    int saveheader = 0;
    unsigned int i = 0;

    // 遍历物理磁盘链表
    DISK_LIST_ITERATOR(g_my_Data_Ctx->testdisk_Ctx->disk_List)
    {
        disk_t *current_Disk = list_Walker->disk;

        // 逻辑磁盘配置
        autodetect_arch(current_Disk, NULL);
        autoset_unit(current_Disk);

        // 逻辑磁盘读取前检查
        if (0==interface_check_disk_capacity(current_Disk) &&
            0==interface_check_disk_access(current_Disk, &current_Cmd) &&
            0==Autoset_Arch_Type(current_Disk))
        {
            // 更新逻辑磁盘链表头数组
            g_my_Data_Ctx->testdisk_Ctx->partition_List[i] = current_Disk->arch->read_part(current_Disk, verbose, saveheader);

            ++i; 
        }
    }

    return;
}

void Partition_List_Free(void)
{
    unsigned int i = 0;

    DISK_LIST_ITERATOR(g_my_Data_Ctx->testdisk_Ctx->disk_List)
    {
        disk_t *current_Disk = list_Walker->disk;

        if (NULL != g_my_Data_Ctx->testdisk_Ctx->partition_List[i])
        {
            part_free_list(g_my_Data_Ctx->testdisk_Ctx->partition_List[i]);
        }

        g_my_Data_Ctx->testdisk_Ctx->partition_List[i]= NULL;
    }

    return;
}

void File_List_Init(void)
{
    TD_INIT_LIST_HEAD(&g_my_Data_Ctx->testdisk_Ctx->testdisk_File_Ctx->list.list);

    return;
}

void File_List_Free(void)
{
    if (NULL != g_my_Data_Ctx->testdisk_Ctx->testdisk_File_Ctx->list.list.next)
    {
        Log_Debug("Clear List\n");
        delete_list_file(&g_my_Data_Ctx->testdisk_Ctx->testdisk_File_Ctx->list);
    }

    return;
}

void File_Data_Init(int *result)
{
    testdisk_File_Context *testdisk_File_Ctx = g_my_Data_Ctx->testdisk_Ctx->testdisk_File_Ctx;

    *result = dir_partition_ntfs_init(Get_Scan_Disk(), Get_Scan_Partition(), &testdisk_File_Ctx->data, 0, 0);
}

void File_Data_Free(void)
{
    if (NULL != g_my_Data_Ctx->testdisk_Ctx->testdisk_File_Ctx->data.local_dir)
    {
        Log_Debug("Clear Dir\n");
        g_my_Data_Ctx->testdisk_Ctx->testdisk_File_Ctx->data.close(&g_my_Data_Ctx->testdisk_Ctx->testdisk_File_Ctx->data);
    }

    return;
}

void Testdisk_File_Context_Init(int *result)
{
    File_List_Init();

    File_Data_Init(result);

    return;
}

void Testdisk_File_Context_Free(void)
{
    File_List_Free();

    File_Data_Free();

    return;
}

void Set_Scan_Disk(unsigned int disk_No)
{
    g_my_Data_Ctx->testdisk_Ctx->scan_Object_Ctx->disk = Access_Disk(Get_Int_Param(disk_No)); 

    return;
}

disk_t *Get_Scan_Disk(void)
{
    return g_my_Data_Ctx->testdisk_Ctx->scan_Object_Ctx->disk; 
}

void Set_Scan_Partition(unsigned int disk_No, unsigned int partition_No)
{
    g_my_Data_Ctx->testdisk_Ctx->scan_Object_Ctx->partition = Accecc_Partition(Get_Int_Param(disk_No), 
                                                                               Get_Int_Param(partition_No));

    return;
}

partition_t *Get_Scan_Partition(void)
{
    return g_my_Data_Ctx->testdisk_Ctx->scan_Object_Ctx->partition; 
}

error_Code Scan_Object_Context_Init(unsigned int disk_No, unsigned int partition_No)
{
    Set_Scan_Disk(disk_No);
    Set_Scan_Partition(disk_No, partition_No);

    if ((NULL == g_my_Data_Ctx->testdisk_Ctx->scan_Object_Ctx->disk) ||
        (NULL == g_my_Data_Ctx->testdisk_Ctx->scan_Object_Ctx->partition))
    {
        Set_Respond_Buffer("Please Check Physical Disk Or Logical Disk No");

        return ERROR_COMMON_ERROR;
    }

    return ERROR_NONE_OK;
}

void Scan_Object_Context_Free(void)
{
    g_my_Data_Ctx->testdisk_Ctx->scan_Object_Ctx->disk = NULL;
    g_my_Data_Ctx->testdisk_Ctx->scan_Object_Ctx->partition = NULL;

    return;
}

void Param_Init(void)
{
    g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->param.cmd_device            = Get_Scan_Disk()->device;
    g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->param.cmd_run               = NULL;
    g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->param.disk                  = Get_Scan_Disk();
    g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->param.partition             = Get_Scan_Partition();
    g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->param.recup_dir             = DEFAULT_DEEPSEARCH_FILE_RECUP_PATH;
    g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->param.carve_free_space_only = 1;
    g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->param.blocksize             = Get_Scan_Partition()->blocksize;

    return;
}

void Option_Init(void)
{
    g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->options.paranoid            = 1;
    g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->options.keep_corrupted_file = 0;
    g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->options.mode_ext2           = 0;
    g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->options.expert              = 0;
    g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->options.lowmem              = 0;
    g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->options.verbose             = 0;
    g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->options.list_file_format    = g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->file_Extension_Surpport_List;

    return;
}

void Data_Init(void)
{
    TD_INIT_LIST_HEAD(&g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->data.list);
    if (td_list_empty(&g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->data.list))
    {
        init_search_space(&g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->data, 
                          g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->param.disk, 
                          g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->param.partition);
    }

    return;
}

void Remove_Used_Space(void)
{
    if (g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->param.carve_free_space_only > 0)
    {
        g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->param.blocksize = 
        remove_used_space(g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->param.disk,
                          g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->param.partition,
                          &g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->data);
    }

    return;
}

void Set_Quick_Scan_File_List_Available(scan_File_List_Available_Code available)
{
    g_my_Data_Ctx->data_Rec_Ctx->info_Ctx->quick_Scan_File_List = available;

    return;
}

scan_File_List_Available_Code Get_Quick_Scan_File_List_Available(void)
{
    return g_my_Data_Ctx->data_Rec_Ctx->info_Ctx->quick_Scan_File_List;
}

void Set_Deep_Scan_File_List_Available(scan_File_List_Available_Code available)
{
    g_my_Data_Ctx->data_Rec_Ctx->info_Ctx->deep_Scan_File_List = available;

    return;
}

scan_File_List_Available_Code Get_Deep_Scan_File_List_Available(void)
{
    return g_my_Data_Ctx->data_Rec_Ctx->info_Ctx->deep_Scan_File_List;
}

void Apply_File_Extension_Support_Config(void)
{
    photorec_File_Context *photorec_File_Ctx = g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx;
    char config_Str_Buffer[100];
    FILE *fp;
    char enable;
    unsigned int i = 0;

    fp = fopen(".\\file_Extension_Support_List.cfg", "r");
    if (NULL == fp)
    {
        Log_Info("file Extension Support List Config Do Not Exist, Apply Default Config\n");

        return;
    }
    
    fseek(fp, 0, SEEK_SET);

    fread(config_Str_Buffer, sizeof(char), ggSA_deep_Scan_File_Extension_List_Len, fp);

    do
    {
        photorec_File_Ctx->file_Extension_Surpport_List[i].enable = ((0==atoi(&config_Str_Buffer[i])?0:1));
        
        ++i;

    }while(i<ggSA_deep_Scan_File_Extension_List_Len);

    fclose(fp);

    Log_Info("file Extension Support List Config Applied\n");

    return;
}

void Save_File_Extension_Support_List_Config(void)
{
    photorec_File_Context *photorec_File_Ctx = g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx;
    char config_Str_Buffer[100];
    FILE *fp;
    

    fp = fopen(".\\file_Extension_Support_List.cfg", "w");

    for (int i=0; NULL!=photorec_File_Ctx->file_Extension_Surpport_List[i].file_hint; ++i)
    {
        sprintf(&config_Str_Buffer[i], "%d", ((0==photorec_File_Ctx->file_Extension_Surpport_List[i].enable)?0:1));
    }

    fseek(fp, 0, SEEK_SET);

    fwrite(config_Str_Buffer, sizeof(char), ggSA_deep_Scan_File_Extension_List_Len, fp);

    fclose(fp);

    Log_Info("file Extension Support List Config Saved\n");

    return;
}

bool IsProcessRunAsAdmin()
{
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;

    BOOL b = AllocateAndInitializeSid(
            &NtAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &AdministratorsGroup);

    if (b)
    {
        CheckTokenMembership(NULL, AdministratorsGroup, &b);
    }

    return  b == TRUE ;
}

int Run_As_Admin(void)
{
    SHELLEXECUTEINFO se;
    memset(&se, 0, sizeof(SHELLEXECUTEINFO));
    se.cbSize = sizeof(SHELLEXECUTEINFO);
    se.lpVerb = "runas";
    se.lpFile = ".\\datarec.exe"; //要打开的进程路径
    se.lpParameters = "Administrator"; //管理员权限
    se.nShow = SW_HIDE;//打开的进程不显示界面
    se.fMask = 0;//不要进程句柄

    if(ShellExecuteEx(&se))
    {
        return EXIT_SUCCESS;
    }
    else
    {
        return EXIT_FAILURE;
    }
}

void Photorec_Context_Init(void)
{
    // 初始化param
    Param_Init();

    // 初始化options
    Option_Init(); 

    // 初始化data
    Data_Init();

    // 移除已用空间
    Remove_Used_Space();

    return;
}

static int Server_Context_Calloc(void)
{
    // 初始化服务器环境
    CALLOC_WITH_CHECK(g_my_Data_Ctx->server_Ctx, (server_Context *), 1, sizeof(server_Context));

    // 初始化请求环境
    CALLOC_WITH_CHECK(g_my_Data_Ctx->server_Ctx->request_Ctx, (request_Context *), 1, sizeof(request_Context));

    // 初始化请求buffer
    CALLOC_WITH_CHECK(g_my_Data_Ctx->server_Ctx->request_Ctx->request_Buffer, (char *), REQUEST_BUFFER_SIZE, sizeof(char));

    // 初始化响应环境
    CALLOC_WITH_CHECK(g_my_Data_Ctx->server_Ctx->respond_Ctx, (respond_Context *), 1, sizeof(respond_Context));

    // 初始化响应buffer
    CALLOC_WITH_CHECK(g_my_Data_Ctx->server_Ctx->respond_Ctx->respond_Buffer, (char *), RESPONSE_BUFFER_SIZE, sizeof(char));

    return EXIT_SUCCESS;
}

static int Testdisk_Context_Calloc(void)
{
    // 初始化testdisk环境
    CALLOC_WITH_CHECK(g_my_Data_Ctx->testdisk_Ctx, (testdisk_Context *), 1, sizeof(testdisk_Context));

    // 初始化testdisk扫描环境
    CALLOC_WITH_CHECK(g_my_Data_Ctx->testdisk_Ctx->scan_Object_Ctx, (scan_Object_Context *), 1, sizeof(scan_Object_Context));

    // 初始化testdisk恢复文件环境
    CALLOC_WITH_CHECK(g_my_Data_Ctx->testdisk_Ctx->testdisk_File_Ctx, (testdisk_File_Context *), 1, sizeof(testdisk_File_Context));

    // 初始化photorec恢复文件环境
    CALLOC_WITH_CHECK(g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx, (photorec_File_Context*), 1, sizeof(photorec_File_Context));

    return EXIT_SUCCESS;
}

static int DataRec_Context_Calloc(void)
{
    // 初始化数据恢复环境
    CALLOC_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx, (data_Recovery_Context *), 1, sizeof(data_Recovery_Context));

    // 初始化硬盘环境
    CALLOC_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->disk_Ctx, (disk_Context *), 1, sizeof(disk_Context));

    // 初始化硬盘信息
    CALLOC_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->disk_Ctx->info, (disk_Info *), DISK_NUM, sizeof(disk_Info));

    // 初始化逻辑分区环境
    CALLOC_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->partition_Ctx, (partition_Context *), 1, sizeof(partition_Context));

    // 初始化逻辑分区信息
    CALLOC_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->partition_Ctx->info, (partition_Info *), PARTITION_NUM, sizeof(partition_Info));

    // 初始化文件环境
    CALLOC_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->file_Ctx, (file_Context *), 1, sizeof(file_Context));

    // 初始化文件信息
    CALLOC_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->file_Ctx->info, (file_Info *), FILE_NUM, sizeof(file_Info));

    // 初始化datarec信息
    CALLOC_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->info_Ctx, (info_Context *), 1, sizeof(info_Context));

    // 初始化datarec信息
    CALLOC_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->info_Ctx->process_Ctx, (process_Context *), 1, sizeof(process_Context));

    return EXIT_SUCCESS;
}

static int DataRec_Context_Free(void)
{
    // 释放datarec流程环境
    FREE_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->info_Ctx->process_Ctx);

    // 释放datarec信息环境
    FREE_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->info_Ctx);

    // 释放文件信息
    FREE_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->file_Ctx->info);

    // 释放文件环境
    FREE_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->file_Ctx);

    // 释放逻辑分区信息
    FREE_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->partition_Ctx->info);

    // 释放逻辑分区环境
    FREE_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->partition_Ctx);

    // 释放硬盘信息
    FREE_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->disk_Ctx->info);

    // 释放硬盘环境
    FREE_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx->disk_Ctx);

    FREE_WITH_CHECK(g_my_Data_Ctx->data_Rec_Ctx);

    return EXIT_SUCCESS;
}

static int Testdisk_Context_Free(void)
{
    // 释放photorec恢复文件环境
    FREE_WITH_CHECK(g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx);

    // 释放testdisk恢复文件环境
    FREE_WITH_CHECK(g_my_Data_Ctx->testdisk_Ctx->testdisk_File_Ctx);

    // 释放testdisk扫描环境
    FREE_WITH_CHECK(g_my_Data_Ctx->testdisk_Ctx->scan_Object_Ctx);

    FREE_WITH_CHECK(g_my_Data_Ctx->testdisk_Ctx);

    return EXIT_SUCCESS;
}

static int Server_Context_Free(void)
{
    // 释放请求buffer
    FREE_WITH_CHECK(g_my_Data_Ctx->server_Ctx->request_Ctx->request_Buffer);

    // 释放请求环境
    FREE_WITH_CHECK(g_my_Data_Ctx->server_Ctx->request_Ctx);

    // 释放响应buffer
    FREE_WITH_CHECK(g_my_Data_Ctx->server_Ctx->respond_Ctx->respond_Buffer);

    // 释放响应环境
    FREE_WITH_CHECK(g_my_Data_Ctx->server_Ctx->respond_Ctx);

    FREE_WITH_CHECK(g_my_Data_Ctx->server_Ctx);

    return EXIT_SUCCESS;
}

int My_Data_Context_Init(void)
{
    // 为我的数据环境开辟空间
    CALLOC_WITH_CHECK(g_my_Data_Ctx, (my_Data_Context *), 1, sizeof(my_Data_Context));
    if ((EXIT_SUCCESS == Server_Context_Calloc()) &&
        (EXIT_SUCCESS == Testdisk_Context_Calloc()) &&
        (EXIT_SUCCESS == DataRec_Context_Calloc()))
    {
        Server_Context_Reset();
        Info_Context_Reset();

        g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx->file_Extension_Surpport_List = gSA_deep_Scan_File_Extension_List;

        Apply_File_Extension_Support_Config();

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
    
    // 释放我的数据环境
    if ((EXIT_SUCCESS == DataRec_Context_Free()) &&
        (EXIT_SUCCESS == Testdisk_Context_Free()) &&
        (EXIT_SUCCESS == Server_Context_Free()))
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

