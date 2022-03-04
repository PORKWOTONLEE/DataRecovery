#include <stdlib.h>
#include <string.h>
#include "../common.h"
#include "../log/log.h"
#include "../libtestdisk/include/dir_common.h"
#include "../libtestdisk/ntfsprogs/include/ntfs/volume.h"
#include "../libtestdisk/include/datarec_inject.h"

#define HAVE_LIBNTFS 1
#include "../libtestdisk/include/ntfs_inc.h"

#include "handler_component.h"

extern my_Data_Context *g_my_Data_Ctx;

static int Page_Control(char *error_Buffer)
{
    data_Recovery_Context *data_Rec_Ctx = g_my_Data_Ctx->data_Rec_Ctx;
    
    // 根据第一个参数执行
    // （0）跳转到指定页数
    Log_Debug("Here");
    if (0 == Get_Param_Num())
    {
        data_Rec_Ctx->file_Ctx->current_Page = 0;

        return EXIT_SUCCESS;
    }
    else if (0 == atoi(Get_Param(0)))
    {
        if (atoi(Get_Param(1))>=0 && atoi(Get_Param(1))<=data_Rec_Ctx->file_Ctx->total_Page)
        {
            data_Rec_Ctx->file_Ctx->current_Page = atoi(Get_Param(1));
            
            return EXIT_SUCCESS;
        }
    }
    //（1）前一页
    else if (1 == atoi(Get_Param(0)))
    {
        if (((int)data_Rec_Ctx->file_Ctx->current_Page-1) < 0)
        {
            sprintf(error_Buffer, "Out Of Page");

            return EXIT_FAILURE;
        }
        --data_Rec_Ctx->file_Ctx->current_Page;
    }
    // （2）后一页
    else if (2 == atoi(Get_Param(0)))
    {
        if ((data_Rec_Ctx->file_Ctx->current_Page+1) > data_Rec_Ctx->file_Ctx->total_Page)
        {
            sprintf(error_Buffer, "Out Of Page");

            return EXIT_FAILURE;
        }
        ++data_Rec_Ctx->file_Ctx->current_Page;
    }

    return EXIT_SUCCESS;
}

static int File_List_Serializer(char *error_Buffer) 
{
    data_Recovery_Context *data_Rec_Ctx = g_my_Data_Ctx->data_Rec_Ctx;
    char *file_List_Serialized = NULL;
    cJSON *file_List_Json = NULL;
    char file_No_Buffer[25] = {};
    char file_Size_Buffer[25] = {};
    int min_File_No = 0;
    int max_File_No = 0;

    // 页面控制
    if (EXIT_FAILURE == Page_Control(error_Buffer))
    {
        return EXIT_FAILURE;
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
    
    return EXIT_SUCCESS;
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

static int Scan_NTFS_Partition_Undelete_File(char *error_Buffer, disk_t *disk, const partition_t *partition)
{
    undelete_File_Context *undelete_File_Ctx = g_my_Data_Ctx->testdisk_Ctx->undelete_File_Ctx;
    struct ntfs_dir_struct *ls = NULL;
    int verbose = 1;

    // 检查文件数据和文件列表可用性
    if (NULL != undelete_File_Ctx->list.list.next)
    {
        Log_Debug("Clear List Data\n");
        delete_list_file(&(undelete_File_Ctx->list));
    }
    if (NULL != undelete_File_Ctx->data.local_dir)
    {
        Log_Debug("Clear Dir Data\n");
        undelete_File_Ctx->data.close(&(undelete_File_Ctx->data));
    }

    // NTFS分区文件夹数据初始化 
    dir_partition_t result = dir_partition_ntfs_init(disk, partition, &(undelete_File_Ctx->data), verbose, 0);
    
    // 设定路径
    undelete_File_Ctx->data.local_dir = DEFAULT_RECOVERY_FILE_PATH;

    switch(result) {
        case DIR_PART_ENOSYS:
            sprintf(error_Buffer, "Unsupport Filesystem");

            return EXIT_FAILURE;
            break;
        case DIR_PART_EIO:
            sprintf(error_Buffer, "Filesystem Was Damaged");

            return EXIT_FAILURE;
            break;
        default:
            {
                ls = (struct ntfs_dir_struct *)(undelete_File_Ctx->data).private_dir_data;
                TD_INIT_LIST_HEAD(&((undelete_File_Ctx->list).list));

                // 搜索丢失文件
                Quick_Scan_Undelete_File(error_Buffer, ls->vol, &(undelete_File_Ctx->list));

            }
            break;
    }

    return EXIT_SUCCESS;
}

static int Update_DataRec_Ctx(void)
{
    undelete_File_Context *undelete_File_Ctx = g_my_Data_Ctx->testdisk_Ctx->undelete_File_Ctx;
    data_Recovery_Context *data_Rec_Ctx = g_my_Data_Ctx->data_Rec_Ctx;
    const struct td_list_head *file_walker = NULL;
    unsigned int i = 0;

    // 遍历文件夹
    td_list_for_each(file_walker, &(undelete_File_Ctx->list.list))
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
    
    return EXIT_SUCCESS;
}

static int Update_Testdisk_Ctx(char *error_Buffer)
{
    disk_t      *disk      = Access_Physical_Disk(atoi(Get_Param(0))); 
    partition_t *partition = Accecc_Logical_Disk(atoi(Get_Param(0)), atoi(Get_Param(1)));

    if ((NULL == disk) || (NULL == partition))
    {
        sprintf(error_Buffer, "Please Check Physical Disk Or Logical Disk No");

        return EXIT_FAILURE;
    }

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
            return Scan_NTFS_Partition_Undelete_File(error_Buffer, disk, partition);
        }
        else
        {
            sprintf(error_Buffer, "Unsupport Filesystem");

            return EXIT_FAILURE;
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
            return Scan_NTFS_Partition_Undelete_File(error_Buffer, disk, partition);
        }
        else
        {
            sprintf(error_Buffer, "Unsupport Filesystem");

            return EXIT_FAILURE;
        }
    }
}

static int Get_File_List(char *error_Buffer)
{
    return File_List_Serializer(error_Buffer);
}


static int Update_File_List(char *error_Buffer)
{
    if ((EXIT_SUCCESS == Update_Testdisk_Ctx(error_Buffer)) && 
        (EXIT_SUCCESS == Update_DataRec_Ctx()))
    {
        sprintf(error_Buffer, "Update Success");

        return EXIT_SUCCESS;
    }

    sprintf(error_Buffer, "Update Disk List Failure");

    return EXIT_FAILURE;
}

error_Code File_List_Handler(char *error_Buffer)
{
    if (0 == strcmp("set", Get_Action()))
    {
        if (EXIT_SUCCESS == Update_File_List(error_Buffer))
        {
            return ERROR_NONE_OK;
        }
    }
    else if (0 == strcmp("get", Get_Action()))
    {
        if (EXIT_SUCCESS == Get_File_List(error_Buffer))
        {
            return ERROR_NONE_OK;
        }
    }

    return ERROR_COMMON_ERROR;
}
