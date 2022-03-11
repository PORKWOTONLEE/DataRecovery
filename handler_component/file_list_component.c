#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../libdatarec/include/common.h"
#include "../libdatarec/include/dir_common.h"
#include "../libdatarec/include/filegen.h"
#include "../libdatarec/include/photorec.h"
#include "../libdatarec/testdisk/src/file_list.c"
#include "../libdatarec/ntfsprogs/include/ntfs/volume.h"
#include "../libdatarec/include/datarec_inject.h"
#include "../common.h"
#include "../log/log.h"

#define HAVE_LIBNTFS 1
#include "../libdatarec/include/ntfs_inc.h"

#include "handler_component.h"

extern my_Data_Context *g_my_Data_Ctx;

static error_Code Page_Control(char *error_Buffer)
{
    data_Recovery_Context *data_Rec_Ctx = g_my_Data_Ctx->data_Rec_Ctx;
    
    // 根据第一个参数执行
    // （0）跳转到指定页数
    if (0 == Get_Param_Num())
    {
        data_Rec_Ctx->file_Ctx->current_Page = 0;

        return ERROR_NONE_OK;
    }
    else if (0 == atoi(Get_Param(0)))
    {
        if (atoi(Get_Param(1))>=0 && atoi(Get_Param(1))<=data_Rec_Ctx->file_Ctx->total_Page)
        {
            data_Rec_Ctx->file_Ctx->current_Page = atoi(Get_Param(1));
            
            return ERROR_NONE_OK;
        }
    }
    //（1）前一页
    else if (1 == atoi(Get_Param(0)))
    {
        if (((int)data_Rec_Ctx->file_Ctx->current_Page-1) < 0)
        {
            sprintf(error_Buffer, "Out Of Page");

            return ERROR_COMMON_ERROR;
        }
        --data_Rec_Ctx->file_Ctx->current_Page;
    }
    // （2）后一页
    else if (2 == atoi(Get_Param(0)))
    {
        if ((data_Rec_Ctx->file_Ctx->current_Page+1) > data_Rec_Ctx->file_Ctx->total_Page)
        {
            sprintf(error_Buffer, "Out Of Page");

            return ERROR_COMMON_ERROR;
        }
        ++data_Rec_Ctx->file_Ctx->current_Page;
    }

    sprintf(error_Buffer, "Check Page Param ERROR");

    return ERROR_COMMON_ERROR;
}

static error_Code File_List_Serializer(char *error_Buffer) 
{
    data_Recovery_Context *data_Rec_Ctx = g_my_Data_Ctx->data_Rec_Ctx;
    char *file_List_Serialized = NULL;
    cJSON *file_List_Json = NULL;
    char file_No_Buffer[25] = {};
    char file_Size_Buffer[25] = {};
    int min_File_No = 0;
    int max_File_No = 0;

    // 页面控制
    if (ERROR_NONE_OK != Page_Control(error_Buffer))
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
        cJSON_AddStringToObject(current_File_Info, "name",  data_Rec_Ctx->file_Ctx->info[i].path_Name);

        // 更新文件大小
        Convert_Size_To_Unit(data_Rec_Ctx->file_Ctx->info[i].size, (char *)&file_Size_Buffer, 1024);
        cJSON_AddStringToObject(current_File_Info, "size",  file_Size_Buffer);

        // 更新文件编号
        sprintf(file_No_Buffer,"file_No_%d", data_Rec_Ctx->file_Ctx->info[i].no);
        cJSON_AddItemToObject(file_List_Json, file_No_Buffer, current_File_Info); 
    }

    // 输出响应信息到buffer
    file_List_Serialized = cJSON_Print(file_List_Json);
    strcpy(error_Buffer, file_List_Serialized);

    // 清理临时数据
    free(file_List_Serialized);
    cJSON_Delete(file_List_Json);
    
    return ERROR_NONE_OK;
}

static disk_t *Access_Physical_Disk(int physical_Disk_No)
{
    physical_Disk_Context *physical_Disk_Ctx = g_my_Data_Ctx->data_Rec_Ctx->physical_Disk_Ctx;
    testdisk_Context *testdisk_Ctx = g_my_Data_Ctx->testdisk_Ctx;
    int i = 0;
    
    if (physical_Disk_No>=0 && physical_Disk_No<physical_Disk_Ctx->total_Num)
    {
        PHYSICAL_DISK_LIST_ITERATOR(testdisk_Ctx->physical_Disk_List)
        {
            if (i == physical_Disk_No)
            {
                return list_Walker->disk;
            }

            ++i;
        }
    }

    return NULL;
}

static partition_t *Accecc_Logical_Disk(int physical_Disk_No, int logical_Disk_No)
{
    physical_Disk_Context *physical_Disk_Ctx = g_my_Data_Ctx->data_Rec_Ctx->physical_Disk_Ctx;
    logical_Disk_Context *logical_Disk_Ctx = g_my_Data_Ctx->data_Rec_Ctx->logical_Disk_Ctx;
    testdisk_Context *testdisk_Ctx = g_my_Data_Ctx->testdisk_Ctx;
    int min_Logical_Disk_no = 0;
    int max_Logical_Disk_no = 0;
    int i = 0;
    int j = 0;
    
    if ((physical_Disk_No>=0) && (physical_Disk_No<physical_Disk_Ctx->total_Num))
    {
        PHYSICAL_DISK_LIST_ITERATOR(testdisk_Ctx->physical_Disk_List)
        {
            if (i == physical_Disk_No)
            {
                max_Logical_Disk_no = min_Logical_Disk_no + physical_Disk_Ctx->logDisk_Num_In_Per_phyDisk[i];
                break;
            }
            min_Logical_Disk_no += physical_Disk_Ctx->logDisk_Num_In_Per_phyDisk[i];
            ++i;
        }
    }

    if ((logical_Disk_No>=min_Logical_Disk_no) && (logical_Disk_No<max_Logical_Disk_no))
    {
        LOGICAL_DISK_LIST_ITERATOR(testdisk_Ctx->logical_Disk_List[physical_Disk_No])
        {
            if (j == logical_Disk_No)
            {
                return list_Walker->part;
            }

            ++j;
        }
    }

    return NULL;
}

static error_Code Read_Partition_Data(char *error_Buffer, disk_t *disk, const partition_t *partition)
{
    testdisk_File_Context *testdisk_File_Ctx = g_my_Data_Ctx->testdisk_Ctx->testdisk_File_Ctx;
    struct ntfs_dir_struct *ls = NULL;
    int verbose = 1;

    // 检查文件数据和文件列表可用性
    if (NULL != testdisk_File_Ctx->list.list.next)
    {
        Log_Debug("Clear List Data\n");
        delete_list_file(&testdisk_File_Ctx->list);
    }
    if (NULL != testdisk_File_Ctx->data.local_dir)
    {
        Log_Debug("Clear Dir Data\n");
        testdisk_File_Ctx->data.close(&testdisk_File_Ctx->data);
    }

    // NTFS分区文件夹数据初始化 
    dir_partition_t result = dir_partition_ntfs_init(disk, partition, &testdisk_File_Ctx->data, verbose, 0);
    
    // 设定路径
    testdisk_File_Ctx->data.local_dir = DEFAULT_QUICKSEARCH_FILE_PATH;

    switch(result) {
        case DIR_PART_ENOSYS:
            sprintf(error_Buffer, "Unsupport Filesystem");

            return ERROR_COMMON_ERROR;
            break;
        case DIR_PART_EIO:
            sprintf(error_Buffer, "Filesystem Was Damaged");

            return ERROR_COMMON_ERROR;
            break;
        default:
            {
                ls = (struct ntfs_dir_struct *)(testdisk_File_Ctx->data).private_dir_data;
                TD_INIT_LIST_HEAD(&testdisk_File_Ctx->list.list);

                // 搜索丢失文件
                Quick_Scan_File(error_Buffer, ls->vol, &testdisk_File_Ctx->list);
            }
            break;
    }

    return ERROR_NONE_OK;
}

static int Update_DataRec_Ctx(char *error_Buffer)
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
            else if (i > FILE_INFO_NUM)
            {
                break;
            }

            // 更新文件序号
            data_Rec_Ctx->file_Ctx->info[i].no = i;

            // 更新文件名称
            strcpy(data_Rec_Ctx->file_Ctx->info[i].path_Name, file_info->name);

            // 更新文件大小
            data_Rec_Ctx->file_Ctx->info[i].size = file_info->st_size;

            // 更新文件inode
            data_Rec_Ctx->file_Ctx->info[i].inode = (uint64_t)file_info->st_ino;

            // 更新文件上次访问时间
            data_Rec_Ctx->file_Ctx->info[i].last_Access_Time = file_info->td_atime;

            // 更新文件上次修改时间
            data_Rec_Ctx->file_Ctx->info[i].last_Modify_Time = file_info->td_mtime;

            // 更新文件上次改变时间
            data_Rec_Ctx->file_Ctx->info[i].last_Change_Time = file_info->td_ctime;

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
        sprintf(error_Buffer, "data_Rec_Ctx/file_Ctx Is Null Pointer");

        return ERROR_COMMON_ERROR;
    }
}

static error_Code Check_Partiton_Need_Redir(char *error_Buffer,
                                     disk_t *disk,
                                     partition_t *partition,
                                     int (* Read_Partition_Data)(char*, disk_t*, partition_t*))
{
    if (partition->sb_offset!=0 && partition->sb_size>0)
    {
        // 逻辑磁盘重定向
        io_redir_add_redir(disk,
                           partition->part_offset+partition->sborg_offset,
                           partition->sb_size,
                           partition->part_offset+partition->sb_offset,
                           NULL);

        // 判断逻辑磁盘类型
        if(partition->upart_type==UP_NTFS ||
           (is_part_ntfs(partition) && 
            partition->upart_type!=UP_EXFAT))
        {
            return Read_Partition_Data(error_Buffer, disk, partition);
        }
        else
        {
            sprintf(error_Buffer, "Unsupport Filesystem");

            return ERROR_COMMON_ERROR;
        }

        // 取消逻辑磁盘重定向
        io_redir_del_redir(disk, partition->part_offset+partition->sborg_offset);
    }
    else
    {
        if(partition->upart_type==UP_NTFS ||
           (is_part_ntfs(partition) && 
           partition->upart_type!=UP_EXFAT))
        {
            return Read_Partition_Data(error_Buffer, disk, partition);
        }
        else
        {
            sprintf(error_Buffer, "Unsupport Filesystem");

            return ERROR_COMMON_ERROR;
        }
    }
}

static error_Code Update_Testdisk_Ctx(char *error_Buffer)
{
    disk_t      *disk      = Access_Physical_Disk(atoi(Get_Param(1))); 
    partition_t *partition = Accecc_Logical_Disk(atoi(Get_Param(1)), atoi(Get_Param(2)));

    if ((NULL == disk) || (NULL == partition))
    {
        sprintf(error_Buffer, "Please Check Physical Disk Or Logical Disk No");

        return ERROR_COMMON_ERROR;
    }

    return Check_Partiton_Need_Redir(error_Buffer,
                                     disk,
                                     partition,
                                     Read_Partition_Data);
}

static error_Code Get_File_List(char *error_Buffer)
{
    return File_List_Serializer(error_Buffer);
}

static error_Code Update_QuickSearch_File_List(char *error_Buffer)
{
    if ((ERROR_NONE_OK == Update_Testdisk_Ctx(error_Buffer)) && 
        (ERROR_NONE_OK == Update_DataRec_Ctx(error_Buffer)))
    {
        sprintf(error_Buffer, "Update QuickSearch File List Success");

        return ERROR_NONE_OK;
    }

    return ERROR_COMMON_ERROR;
}

static error_Code Update_DeepSearch_File_List(char *error_Buffer)
{
    disk_t      *disk      = Access_Physical_Disk(atoi(Get_Param(1))); 
    partition_t *partition = Accecc_Logical_Disk(atoi(Get_Param(1)), atoi(Get_Param(2)));

    if ((NULL == disk) || (NULL == partition))
    {
        sprintf(error_Buffer, "Please Check Physical Disk Or Logical Disk No");

        return ERROR_COMMON_ERROR;
    }

    photorec_File_Context *photorec_File_Ctx = g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx;

    // 初始化ph_param
    photorec_File_Ctx->param.cmd_device            = disk->device;
    photorec_File_Ctx->param.cmd_run               = NULL;
    photorec_File_Ctx->param.disk                  = disk;
    photorec_File_Ctx->param.partition             = partition;
    photorec_File_Ctx->param.recup_dir             = DEFAULT_DEEPSEARCH_FILE_PATH;
    photorec_File_Ctx->param.carve_free_space_only = 0;
    photorec_File_Ctx->param.blocksize             = partition->blocksize;

    // 初始化ph_options
    photorec_File_Ctx->options.paranoid            = 1;
    photorec_File_Ctx->options.keep_corrupted_file = 0;
    photorec_File_Ctx->options.mode_ext2           = 0;
    photorec_File_Ctx->options.expert              = 0;
    photorec_File_Ctx->options.lowmem              = 0;
    photorec_File_Ctx->options.verbose             = 0;
    photorec_File_Ctx->options.list_file_format    = array_file_enable;

    unsigned int array_file_list_len = sizeof(array_file_enable)/sizeof(file_enable_t);
    for (int i=0; i<array_file_list_len; ++i)
    {
        if (NULL!=array_file_enable[i].file_hint)
        {
            if (NULL!=array_file_enable[i].file_hint->extension)
            {
                if (0 == strcmp(array_file_enable[i].file_hint->extension, "gif"))
                {
                    Log_Info("enable gif search\n");
                    array_file_enable[i].enable = 1;
                }
            }
            else
            {
                Log_Info("Extension Is NULL\n");
            }
        }
        else
        {
            Log_Info("Hint Is NULL\n");
        }
    }

    // 初始化data
    TD_INIT_LIST_HEAD(&photorec_File_Ctx->data.list);
    if (td_list_empty(&photorec_File_Ctx->data.list))
    {
        init_search_space(&photorec_File_Ctx->data, 
                photorec_File_Ctx->param.disk, 
                photorec_File_Ctx->param.partition);
    }
    if (photorec_File_Ctx->param.carve_free_space_only > 0)
    {
        photorec_File_Ctx->param.blocksize = remove_used_space(photorec_File_Ctx->param.disk,
                                                               photorec_File_Ctx->param.partition,
                                                               &photorec_File_Ctx->data);
    }

    Deep_Scan_File(error_Buffer,
                   &photorec_File_Ctx->param,
                   &photorec_File_Ctx->options,
                   &photorec_File_Ctx->data);

    free_list_search_space(&photorec_File_Ctx->data);

    //sprintf(error_Buffer, "Update Success");

    return ERROR_NONE_OK;
}

static error_Code Update_File_List(char *error_Buffer)
{
    if (0 == (atoi(Get_Param(0))))
    {
        return Update_QuickSearch_File_List(error_Buffer);
    }
    else if (1 == atoi((Get_Param(0))))
    {
        return Update_DeepSearch_File_List(error_Buffer);
    }

    sprintf(error_Buffer, "Check File List Param ERROR");

    return ERROR_COMMON_ERROR;
}

error_Code File_List_Handler(char *error_Buffer)
{
    if (0 == strcmp("set", Get_Action()))
    {
        return Update_File_List(error_Buffer);
    }
    else if (0 == strcmp("get", Get_Action()))
    {
        return Get_File_List(error_Buffer);
    }

    sprintf(error_Buffer, "Check File List Action ERROR");

    return ERROR_COMMON_ERROR;
}
