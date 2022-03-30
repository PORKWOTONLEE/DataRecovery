#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <io.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include "../libdatarec/include/common.h"
#include "../libdatarec/include/dir_common.h"
#include "../libdatarec/include/filegen.h"
#include "../libdatarec/include/photorec.h"
#include "../libdatarec/3rd_lib/ntfsprogs/include/ntfs/volume.h"
#include "../libdatarec/include/datarec_inject.h"
#include "../common.h"
#include "../log/log.h"

#define HAVE_LIBNTFS 1
#include "../libdatarec/include/ntfs_inc.h"

#include "handler_component.h"

extern my_Data_Context *g_my_Data_Ctx;
extern int Quick_Scan_File(ntfs_volume *vol, file_info_t *dir_list);
extern int Deep_Scan_File(testdisk_Context *testdisk_Ctx);
extern int io_redir_add_redir(disk_t *disk_car, const uint64_t org_offset, const unsigned int size, const uint64_t new_offset, const void *mem);
extern int io_redir_del_redir(disk_t *disk_car, uint64_t org_offset);
extern int is_part_ntfs(const partition_t *partition);
extern file_enable_t *gSA_deep_Scan_File_Extension_List;
extern unsigned int ggSA_deep_Scan_File_Extension_List_Len;

static error_Code Read_Partition_Data(void)
{
    struct ntfs_dir_struct *ls = NULL;
    int result;

    // 清空testdisk恢复文件结构体
    Testdisk_File_Context_Free(); 

    // NTFS分区文件夹数据初始化 
    Testdisk_File_Context_Init(&result); 
    
    // 设定路径
    g_my_Data_Ctx->testdisk_Ctx->testdisk_File_Ctx->data.local_dir = DEFAULT_QUICKSEARCH_FILE_PATH;
    strcpy(g_my_Data_Ctx->data_Rec_Ctx->file_Ctx->destination, DEFAULT_QUICKSEARCH_FILE_PATH);

    switch(result) {
        case DIR_PART_ENOIMP:
            Set_Respond_Buffer("Support For This Filesystem Is  Not Implenment");

            return ERROR_COMMON_ERROR;
            break;
        case DIR_PART_ENOSYS:
            Set_Respond_Buffer("Unsupport Filesystem");

            return ERROR_COMMON_ERROR;
            break;
        case DIR_PART_EIO:
            Set_Respond_Buffer("Filesystem Was Damaged");

            return ERROR_COMMON_ERROR;
            break;
        default:
            // 搜索丢失文件
            ls = (struct ntfs_dir_struct *)(g_my_Data_Ctx->testdisk_Ctx->testdisk_File_Ctx->data).private_dir_data;

            Quick_Scan_File(ls->vol, &g_my_Data_Ctx->testdisk_Ctx->testdisk_File_Ctx->list);
            break;
    }

    return ERROR_NONE_OK;
}

static error_Code Check_Partiton_Need_Redir(error_Code (* Read_Partition_Data)(void))
{
    if (Get_Scan_Partition()->sb_offset!=0 && Get_Scan_Partition()->sb_size>0)
    {
        // 逻辑磁盘重定向
        io_redir_add_redir(Get_Scan_Disk(),
                           Get_Scan_Partition()->part_offset+Get_Scan_Partition()->sborg_offset,
                           Get_Scan_Partition()->sb_size,
                           Get_Scan_Partition()->part_offset+Get_Scan_Partition()->sb_offset,
                           NULL);

        // 判断逻辑磁盘类型
        if(Get_Scan_Partition()->upart_type==UP_NTFS ||
           (is_part_ntfs(Get_Scan_Partition()) && 
           Get_Scan_Partition()->upart_type!=UP_EXFAT))
        {
            return Read_Partition_Data();
        }
        else
        {
            Set_Respond_Buffer("Unsupport Filesystem");

            return ERROR_COMMON_ERROR;
        }

        // 取消逻辑磁盘重定向
        io_redir_del_redir(Get_Scan_Disk(), Get_Scan_Partition()->part_offset+Get_Scan_Partition()->sborg_offset);
    }
    else
    {
        if(Get_Scan_Partition()->upart_type==UP_NTFS ||
           (is_part_ntfs(Get_Scan_Partition()) && 
           Get_Scan_Partition()->upart_type!=UP_EXFAT))
        {
            return Read_Partition_Data();
        }
        else
        {
            Set_Respond_Buffer("Unsupport Filesystem");

            return ERROR_COMMON_ERROR;
        }
    }
}

static int Update_DataRec_Ctx(void)
{
    testdisk_File_Context *testdisk_File_Ctx = g_my_Data_Ctx->testdisk_Ctx->testdisk_File_Ctx;
    data_Recovery_Context *data_Rec_Ctx = g_my_Data_Ctx->data_Rec_Ctx;
    const struct td_list_head *file_walker = NULL;
    unsigned int i = 0;

    if ((NULL!=data_Rec_Ctx) &&
        (NULL!=data_Rec_Ctx->file_Ctx))
    {
        // 遍历文件夹
        td_list_for_each(file_walker, &testdisk_File_Ctx->list.list)
        {
            const file_info_t *file_info = td_list_entry_const(file_walker, const file_info_t, list);

            // 如果文件状态不是删除，则跳过
            if (0 != (file_info->status & FILE_STATUS_DELETED))
            {
                continue;
            }
            else if (i > FILE_NUM)
            {
                break;
            }

            // 更新文件序号
            data_Rec_Ctx->file_Ctx->info[i].no = i;

            // 更新文件名称
            strcpy(data_Rec_Ctx->file_Ctx->info[i].name, file_info->name);

            // 更新文件大小
            data_Rec_Ctx->file_Ctx->info[i].size = file_info->st_size;

            // 更新文件inode
            data_Rec_Ctx->file_Ctx->info[i].inode = (uint64_t)file_info->st_ino;

            ++i;
        }

        // 更新文件总数
        data_Rec_Ctx->file_Ctx->total_Num = i;

        // 更新文件总页数/每页10个文件
        data_Rec_Ctx->file_Ctx->total_Page = (0 == i%10)?(i/10):(i/10+1);

        // 更新文件当前页数
        data_Rec_Ctx->file_Ctx->current_Page = 0;

        return ERROR_NONE_OK;
    }
    else
    {
        Set_Respond_Buffer("data_Rec_Ctx/file_Ctx Is Null Pointer");

        return ERROR_COMMON_ERROR;
    }
}


static error_Code Page_Control(void)
{
    data_Recovery_Context *data_Rec_Ctx = g_my_Data_Ctx->data_Rec_Ctx;
    
    // 根据第一个参数执行
    if (1 == Get_Param_Num())
    {
        data_Rec_Ctx->file_Ctx->current_Page = 0;

        return ERROR_NONE_OK;
    }
    // 上一页
    else if (0 == Get_Int_Param(1))
    {
        if (((int)data_Rec_Ctx->file_Ctx->current_Page-1) < 0)
        {
            Set_Respond_Buffer("Out Of Page");

            return ERROR_COMMON_ERROR;
        }
        --data_Rec_Ctx->file_Ctx->current_Page;

        return ERROR_NONE_OK;
    }
    // 下一页
    else if (1 == Get_Int_Param(1))
    {
        if ((data_Rec_Ctx->file_Ctx->current_Page+1) > data_Rec_Ctx->file_Ctx->total_Page)
        {
            Set_Respond_Buffer("Out Of Page");

            return ERROR_COMMON_ERROR;
        }
        ++data_Rec_Ctx->file_Ctx->current_Page;

        return ERROR_NONE_OK;
    }
    // 跳转到指定页数
    else if (2 == Get_Int_Param(1))
    {
        if (Get_Int_Param(2)>=0 && Get_Int_Param(2)<=data_Rec_Ctx->file_Ctx->total_Page)
        {
            data_Rec_Ctx->file_Ctx->current_Page = Get_Int_Param(2);
            
            return ERROR_NONE_OK;
        }
    }

    Set_Respond_Buffer("Check Page Param ERROR");

    return ERROR_COMMON_ERROR;
}

static error_Code File_List_Serializer(void) 
{
    data_Recovery_Context *data_Rec_Ctx = g_my_Data_Ctx->data_Rec_Ctx;
    char *file_List_Serialized = NULL;
    cJSON *file_List_Json = NULL;
    char file_No_Buffer[25] = {};
    char file_Size_Buffer[25] = {};
    int min_File_No = 0;
    int max_File_No = 0;

    // 页面控制
    if (ERROR_NONE_OK != Page_Control())
    {
        return ERROR_COMMON_ERROR;
    }

    file_List_Json = cJSON_CreateObject();

    // 更新文件总数
    cJSON_AddNumberToObject(file_List_Json, "total_Num", data_Rec_Ctx->file_Ctx->total_Num); 

    // 更新文件总页数
    cJSON_AddNumberToObject(file_List_Json, "total_Page", data_Rec_Ctx->file_Ctx->total_Page); 

    // 更新文件当前页数
    cJSON_AddNumberToObject(file_List_Json, "current_Page", data_Rec_Ctx->file_Ctx->current_Page); 

    // 设置当前页面文件起始编号
    min_File_No = 10*(data_Rec_Ctx->file_Ctx->current_Page);
    max_File_No = 10*(data_Rec_Ctx->file_Ctx->current_Page)+10;

    for (int i=min_File_No; i<max_File_No; ++i)
    {
        cJSON *current_File_Info = NULL;

        current_File_Info = cJSON_CreateObject();

        // 更新文件序号
        cJSON_AddNumberToObject(current_File_Info, "no",  data_Rec_Ctx->file_Ctx->info[i].no);

        // 更新文件名称
        cJSON_AddStringToObject(current_File_Info, "name",  data_Rec_Ctx->file_Ctx->info[i].name);

        // 更新文件大小
        Convert_Size_To_Unit(data_Rec_Ctx->file_Ctx->info[i].size, (char *)&file_Size_Buffer, 1024);
        cJSON_AddStringToObject(current_File_Info, "size",  file_Size_Buffer);

        // 更新文件编号
        sprintf(file_No_Buffer,"file_No_%d", data_Rec_Ctx->file_Ctx->info[i].no);
        cJSON_AddItemToObject(file_List_Json, file_No_Buffer, current_File_Info); 
    }

    // 输出响应信息到buffer
    file_List_Serialized = cJSON_Print(file_List_Json);
    Set_Respond_Buffer(file_List_Serialized);

    // 清理临时数据
    free(file_List_Serialized);
    cJSON_Delete(file_List_Json);
    
    return ERROR_NONE_OK;
}

static void Get_File_Extension(char *name, char *extension)
{
    char *result = NULL;
    static unsigned int first_Dot = 0;

    result = strchr(name, '.');

    if (NULL != result)
    {
        ++ first_Dot;
        Get_File_Extension(result+1, extension);
    }
    else if ((NULL==result) && (0!=first_Dot))
    {
        first_Dot = 0;
        strcpy(extension, name);
    }
    else if (0 == first_Dot)
    {
        strcpy(extension, "unknown");
    }

    return;
}

static void Make_Extension_Directory(char *dir, const char *extension)
{
    char extension_Dir[100];

    sprintf(extension_Dir, "%s\\%s", dir, extension);

    if (0 != access(extension_Dir, 0))
    {
        mkdir(extension_Dir, 777);
    }
}

static void Delete_Diectory(char *directory)
{
    char path[1000];
    struct dirent *dp;
    DIR *dir;

    dir = opendir(directory);
    if (NULL == dir)
    {
        Delete_File(directory);

        return;
    }
    else
    {
        while (NULL != (dp = readdir(dir)))
        {
            if (0 == strcmp(dp->d_name, ".") ||
                0 == strcmp(dp->d_name, ".."))
            {
                continue;
            }

            sprintf(path, "%s\\%s", directory, dp->d_name);

            Delete_Diectory(path);
        }

        remove(directory);

        return;
    }
}

static void Deep_Scan_File_Clean(void)
{
    photorec_File_Context *photorec_File_Ctx = g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx;
    struct dirent *dp;
    DIR *dir;
    char path[100];

    dir = opendir(DEFAULT_DEEPSEARCH_FILE_PATH);
    if (NULL == dir)
    {
        return;
    }

    // 遍历读取分割文件夹文件
    while (NULL != (dp = readdir(dir)))
    {
        if (0 == strcmp(dp->d_name, ".") ||
                0 == strcmp(dp->d_name, ".."))
        {
            continue;
        }

        sprintf(path, "%s\\%s", DEFAULT_DEEPSEARCH_FILE_PATH, dp->d_name);

        Delete_Diectory(path);
    }

    return;
}

static void Deep_Scan_File_Classify(void)
{
    photorec_File_Context *photorec_File_Ctx = g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx;
    struct dirent *dp;
    DIR *dir;
    char *deep_Scan_File_Path_Root = DEFAULT_DEEPSEARCH_FILE_PATH;
    char *deep_Scan_File_Path_Recup_Root = ".\\tmp\\deep\\recup.";
    char deep_Scan_File_Path_Recup[100];
    char source_Path[100];
    char destination_Path[100];
    char file_Extension[10];
    unsigned int i = 1;

    do
    {
        // 构建分割文件夹路径
        sprintf(deep_Scan_File_Path_Recup, "%s%d", deep_Scan_File_Path_Recup_Root, i);

        // 读取分割文件夹
        dir = opendir(deep_Scan_File_Path_Recup);
        if (NULL == dir)
        {
            break;
        }

        // 遍历读取分割文件夹文件
        while (NULL != (dp = readdir(dir)))
        {
            if (0 == strcmp(dp->d_name, ".") ||
                0 == strcmp(dp->d_name, ".."))
            {
                continue;
            }

            // 获取文件拓展名
            Get_File_Extension(dp->d_name, (char *)&file_Extension);

            // 创建拓展名对应文件夹
            Make_Extension_Directory(deep_Scan_File_Path_Root, file_Extension);

            sprintf(source_Path, "%s\\%s", deep_Scan_File_Path_Recup, dp->d_name);
            sprintf(destination_Path, 
                    "%s\\%s\\%s", 
                    deep_Scan_File_Path_Root, 
                    file_Extension, dp->d_name);

            // 移动文件
            Move_File(destination_Path, source_Path);
        }

        Delete_Diectory(deep_Scan_File_Path_Recup);
        ++i;

    }while(1);
}

static void Quick_Scan_File_With_Param(void)
{
    Set_Percent(0.0);
    Set_Status(QUICK_SCANNING);
    Set_Quick_Scan_File_List_Available(LIST_NOT_READY);
    Set_Control(SCAN_CONTINUE);

    Check_Partiton_Need_Redir(Read_Partition_Data);
    Set_Percent(50.0);

    // 判断扫描停止后，提前终止线程
    if (SCAN_STOP == Get_Control())
    {
        Set_Percent(0.0);
        Set_Status(IDLE);
    Set_Quick_Scan_File_List_Available(LIST_NOT_READY);

        return;
    }

    Update_DataRec_Ctx();

    Set_Percent(100.0);
    Set_Status(IDLE);
    Set_Quick_Scan_File_List_Available(LIST_READY);

    return;
}

static void Deep_Scan_File_With_Param(void)
{
    Set_Percent(0.0);
    Set_Status(DEEP_SCANNING);
    Set_Deep_Scan_File_List_Available(LIST_NOT_READY);
    Set_Control(SCAN_CONTINUE);

    Photorec_Context_Init(); 

    Deep_Scan_File_Clean();

    Deep_Scan_File(g_my_Data_Ctx->testdisk_Ctx);

    // 判断扫描停止后，提前终止线程
    if (SCAN_STOP == Get_Control())
    {
        Set_Percent(0.0);
        Set_Status(IDLE);
        Set_Deep_Scan_File_List_Available(LIST_NOT_READY);

        return;
    }

    Deep_Scan_File_Classify();

    Set_Percent(100.0);
    Set_Status(IDLE);
    Set_Deep_Scan_File_List_Available(LIST_READY);
}

static error_Code Scan_File_BackGround(void (* Scan_File_Function)(void))
{
    pthread_t tid;

    if (IDLE != Get_Status())
    {
        Set_Respond_Buffer("Scan Aborted, DataRec Busy");

        return ERROR_COMMON_ERROR;
    }

    // 新建线程
    if(!pthread_create(&tid, NULL, (void *)Scan_File_Function, NULL))
    {
        Set_Respond_Buffer("Scan Started");

        return ERROR_NONE_OK;
    }
    else
    {
        Set_Respond_Buffer("Scan Aborted, Thread Creat Fail");

        return ERROR_COMMON_ERROR;
    }
}

static error_Code Check_Partition_Type(void)
{
    if(Get_Scan_Partition()->upart_type==UP_NTFS ||
            (is_part_ntfs(Get_Scan_Partition()) && 
             Get_Scan_Partition()->upart_type!=UP_EXFAT))
    {
        return ERROR_NONE_OK;
    }
    else
    {
        Set_Respond_Buffer("Unsupport Filesystem");

        return ERROR_COMMON_ERROR;
    }
}

static error_Code Start_QuickScan_File(void)
{
    if (ERROR_COMMON_ERROR == Scan_Object_Context_Init(1, 2))
    {
        return ERROR_COMMON_ERROR;
    }

    if (ERROR_COMMON_ERROR == Check_Partition_Type())
    {
        return ERROR_COMMON_ERROR;
    }

    return Scan_File_BackGround(Quick_Scan_File_With_Param);
}

static error_Code Start_DeepScan_File(void)
{
    if (ERROR_COMMON_ERROR == Scan_Object_Context_Init(1, 2))
    {
        return ERROR_COMMON_ERROR;
    }

    return Scan_File_BackGround(Deep_Scan_File_With_Param);
}

static error_Code Stop_Scan_Process(void)
{
    if (IDLE != Get_Status())
    {
        Set_Control(SCAN_STOP);

        Set_Respond_Buffer("Scan Process Stopped");
    }
    else
    {
        Set_Respond_Buffer("Scan Process Do Not Start");
    }

    return ERROR_NONE_OK;
}

static error_Code Set_DeepScan_File_Extension(void)
{
    photorec_File_Context *photorec_File_Ctx = g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx;
    char *extension = Get_Char_Param(1);
    int enable = Get_Int_Param(2);
    int i = 0;

    for (; NULL!=photorec_File_Ctx->file_Extension_Surpport_List[i].file_hint; ++i)
    {
        if (0 == strcmp(extension, photorec_File_Ctx->file_Extension_Surpport_List[i].file_hint->extension))
        {
            break;
        }
    }

    if (i<ggSA_deep_Scan_File_Extension_List_Len)
    {
        photorec_File_Ctx->file_Extension_Surpport_List[i].enable = enable;
    }
    else
    {
        Set_Respond_Buffer("Do Not Support This File Extension");

        return ERROR_COMMON_ERROR;
    }

    Save_File_Extension_Support_List_Config();

    Set_Respond_Buffer("File Extension %s Is Set To %s", 
                       photorec_File_Ctx->file_Extension_Surpport_List[i].file_hint->extension, 
                       (1==photorec_File_Ctx->file_Extension_Surpport_List[i].enable?"Enable":"Disable"));

    return ERROR_NONE_OK;
}

static error_Code Get_Quick_Scan_File_List(void)
{
    return File_List_Serializer();
}

error_Code Scan_Handler(char *error_Buffer)
{
    request_Context *request_Ctx = g_my_Data_Ctx->server_Ctx->request_Ctx;

    if (0 == strcmp("set", Get_Action()))
    {
        if (0 != request_Ctx->param_Num)
        {
            if (QUICK_SCAN == Get_Int_Param(0))
            {
                return Start_QuickScan_File();
            }
            else if (DEEP_SCAN == Get_Int_Param(0))
            {
                return Start_DeepScan_File();
            }
            else if (STOP_SCAN == Get_Int_Param(0))
            {
                return Stop_Scan_Process();
            }
            else if (SET_FILE_EXTENSION == Get_Int_Param(0))
            {
                return Set_DeepScan_File_Extension();
            }
        }
    }
    else if (0 == strcmp("get", Get_Action()))
    {
        if (0 != request_Ctx->param_Num)
        {
            return Get_Quick_Scan_File_List();
        }
    }

    Set_Respond_Buffer("Check File List Action ERROR");

    return ERROR_COMMON_ERROR;
}
