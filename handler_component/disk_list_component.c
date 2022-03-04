#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <windows.h>
#include "../common.h"
#include "../log/log.h"
#include "handler_component.h"

extern my_Data_Context *g_my_Data_Ctx;

static int Disk_List_Serializer(char *error_Buffer) 
{ 
    data_Recovery_Context *data_Rec_Ctx = g_my_Data_Ctx->data_Rec_Ctx;
    char physical_Disk_No_Buffer[25] = {};
    char logical_Disk_No_Buffer[25] = {};
    char physical_Disk_Size_Buffer[25] = {};
    char logical_Disk_Size_Buffer[25] = {};
    char *disk_List_Serialized = NULL;
    cJSON *physical_Disk_Json = NULL;
    int logical_Disk_Num = 0;
    static int j = 0;

    // 由于j是静态变量，所以使用前要置0
    j = 0;

    physical_Disk_Json = cJSON_CreateObject();

    // 物理磁盘总数量
    cJSON_AddNumberToObject(physical_Disk_Json, "physical_Disk_Total_Num", data_Rec_Ctx->physical_Disk_Ctx->total_Num);

    // 逻辑磁盘总数量
    cJSON_AddNumberToObject(physical_Disk_Json, "logical_Disk_Total_Num", data_Rec_Ctx->logical_Disk_Ctx->total_Num);

    // 更新物理磁盘信息
    for (int i=0; i<data_Rec_Ctx->physical_Disk_Ctx->total_Num; ++i)
    {
        Log_Debug("Serializing physical_Disk_%02d\n", i);

        cJSON *current_Physical_Disk_Info = NULL;
        current_Physical_Disk_Info = cJSON_CreateObject();

        // 更新物理磁盘序号
        cJSON_AddNumberToObject(current_Physical_Disk_Info, "no",               data_Rec_Ctx->logical_Disk_Ctx->info[i].no);

        // 更新物理磁盘名称
        cJSON_AddStringToObject(current_Physical_Disk_Info, "name",             data_Rec_Ctx->physical_Disk_Ctx->info[i].name);

        // 更新物理磁盘大小
        Convert_Size_To_Unit(data_Rec_Ctx->physical_Disk_Ctx->info[i].size, (char *)&physical_Disk_Size_Buffer, 1024);
        cJSON_AddStringToObject(current_Physical_Disk_Info, "size",             physical_Disk_Size_Buffer);

        // 更新物理磁盘中逻辑磁盘数量
        cJSON_AddNumberToObject(current_Physical_Disk_Info, "logical_Disk_Num", data_Rec_Ctx->physical_Disk_Ctx->logDisk_Num_In_Per_phyDisk[i]);

        // 更新逻辑磁盘信息
        logical_Disk_Num = j + data_Rec_Ctx->physical_Disk_Ctx->logDisk_Num_In_Per_phyDisk[i];
        for (; j<logical_Disk_Num; ++j)
        {
            Log_Debug("Serializing logical_Disk_%02d\n", j);

            cJSON *current_Logical_Disk_Info = NULL;
            current_Logical_Disk_Info = cJSON_CreateObject(); 

            // 更新逻辑磁盘序号
            cJSON_AddNumberToObject(current_Logical_Disk_Info, "no",                data_Rec_Ctx->logical_Disk_Ctx->info[j].no);

            // 更新逻辑磁盘盘符 
            cJSON_AddStringToObject(current_Logical_Disk_Info, "letter",            data_Rec_Ctx->logical_Disk_Ctx->info[j].letter);

            // 更新逻辑磁盘名称
            cJSON_AddStringToObject(current_Logical_Disk_Info, "name",              data_Rec_Ctx->logical_Disk_Ctx->info[j].name);

            // 更新逻辑磁盘大小
            Convert_Size_To_Unit(data_Rec_Ctx->logical_Disk_Ctx->info[j].size, (char *)&logical_Disk_Size_Buffer, 1024);
            cJSON_AddStringToObject(current_Logical_Disk_Info, "size",              logical_Disk_Size_Buffer);

            // 更新逻辑磁盘文件系统
            cJSON_AddStringToObject(current_Logical_Disk_Info, "file_System",       data_Rec_Ctx->logical_Disk_Ctx->info[j].file_System);

            // 更新逻辑磁盘文件系统额外信息
            cJSON_AddStringToObject(current_Logical_Disk_Info, "file_System_Extra", data_Rec_Ctx->logical_Disk_Ctx->info[j].file_System_Extra);

            // 更新逻辑磁盘编号
            sprintf(logical_Disk_No_Buffer,"logical_Disk_No_%u", data_Rec_Ctx->logical_Disk_Ctx->info[j].no);
            cJSON_AddItemToObject(current_Physical_Disk_Info, logical_Disk_No_Buffer, current_Logical_Disk_Info);
        }

        // 更新物理磁盘编号
        sprintf(physical_Disk_No_Buffer, "physical_Disk_No_%u", data_Rec_Ctx->physical_Disk_Ctx->info[i].no);
        cJSON_AddItemToObject(physical_Disk_Json, physical_Disk_No_Buffer, current_Physical_Disk_Info);
    }

    // 输出响应信息到buffer
    disk_List_Serialized = cJSON_Print(physical_Disk_Json); 
    strcpy(error_Buffer, disk_List_Serialized);

    // 清理临时数据
    free(disk_List_Serialized);
    cJSON_Delete(physical_Disk_Json);

    return EXIT_SUCCESS; 
}

static char Get_Partition_Letter(partition_t *partiton)
{
    char device[] = "\\\\.\\C:";
    int size_devicename = strlen(device);
	HANDLE handle = INVALID_HANDLE_VALUE;
	bool result = false;
    PARTITION_INFORMATION_EX partition_Info;
	int mode = IOCTL_DISK_GET_PARTITION_INFO_EX;
	DWORD bytes_Return = 0;

    for (char i='C'; i<='Z'; ++i)
    {
        // 查看设备可用性
        device[size_devicename - 2] = i;
        handle = CreateFile(device, FILE_READ_DATA | FILE_WRITE_DATA, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (handle == INVALID_HANDLE_VALUE)
        {
			CloseHandle(handle);
            continue;
        }
        
        // 获取设备信息
		result = DeviceIoControl(handle, mode, NULL, 0, &partition_Info, sizeof(partition_Info), &bytes_Return, NULL);
		if (!result)
		{
			CloseHandle(handle);
			continue;
		}

        // 判断设备盘符正确性
		if (partiton->part_offset == partition_Info.StartingOffset.QuadPart &&
		    partiton->part_size == partition_Info.PartitionLength.QuadPart)
        {
            return i;
        }
    }

    return '?'; 
}

static int Update_Logical_Disk(unsigned int i)
{
    data_Recovery_Context *data_Rec_Ctx = g_my_Data_Ctx->data_Rec_Ctx;
    testdisk_Context *testdisk_Ctx = g_my_Data_Ctx->testdisk_Ctx;
    static int j = 0;

    // 如果i=0，那么说明需要从头更新逻辑磁盘信息 
    if (0 == i)
    {
        j = 0;
    }

    // 防止当前物理磁盘的逻辑磁盘总数累加
    data_Rec_Ctx->physical_Disk_Ctx->logDisk_Num_In_Per_phyDisk[i] = 0;

    LOGICAL_DISK_LIST_ITERATOR(testdisk_Ctx->logical_Disk_List[i])
    {
        Log_Debug("Updating logical_Disk_%02d\n", j);

        partition_t *current_Logical_Disk = list_Walker->part;

        // 更新逻辑磁盘序号
        data_Rec_Ctx->logical_Disk_Ctx->info[j].no = j;

        // 更新逻辑磁盘盘符
        sprintf(data_Rec_Ctx->logical_Disk_Ctx->info[j].letter, "%c", Get_Partition_Letter(current_Logical_Disk));
        
        // 更新逻辑磁盘名称
        strcpy(data_Rec_Ctx->logical_Disk_Ctx->info[j].name, current_Logical_Disk->fsname);

        // 更新逻辑磁盘容量
        //Convert_Size_To_Unit(current_Logical_Disk->part_size, buffer_Logical_Disk_Size, 1024);
        data_Rec_Ctx->logical_Disk_Ctx->info[j].size = current_Logical_Disk->part_size;

        // 更新逻辑磁盘文件系统
        strcpy(data_Rec_Ctx->logical_Disk_Ctx->info[j].file_System, current_Logical_Disk->info);

        // 更新逻辑磁盘文件系统额外信息
        strcpy(data_Rec_Ctx->logical_Disk_Ctx->info[j].file_System_Extra, current_Logical_Disk->partname);

        // 更新当前物理磁盘的逻辑磁盘总数
        ++data_Rec_Ctx->physical_Disk_Ctx->logDisk_Num_In_Per_phyDisk[i];

        ++j;
    }

// 更新逻辑磁盘总数
data_Rec_Ctx->logical_Disk_Ctx->total_Num = j;

return EXIT_SUCCESS;
}

static int Update_Physical_Disk_Ctx(void)
{
    data_Recovery_Context * data_Rec_Ctx = g_my_Data_Ctx->data_Rec_Ctx;
    testdisk_Context *testdisk_Ctx = g_my_Data_Ctx->testdisk_Ctx;
    char buffer_Physical_Disk_Size[20];
    unsigned int i = 0;

    // 遍历物理磁盘链表
    PHYSICAL_DISK_LIST_ITERATOR(testdisk_Ctx->physical_Disk_List)
    {
        Log_Debug("Updating physical_Disk_%02d\n", i);

        disk_t *current_Physical_Disk = list_Walker->disk;

        // 更新物理磁盘序号
        data_Rec_Ctx->physical_Disk_Ctx->info[i].no = i;

        // 更新物理磁盘型号
        strcpy(data_Rec_Ctx->physical_Disk_Ctx->info[i].name, current_Physical_Disk->model);

        // 更新物理磁盘容量
        data_Rec_Ctx->physical_Disk_Ctx->info[i].size = current_Physical_Disk->disk_size;

        // 更新当前物理磁盘下逻辑磁盘信息
        Update_Logical_Disk(i);

        ++i; 
    }

    // 更新物理磁盘总数
    data_Rec_Ctx->physical_Disk_Ctx->total_Num = i;

    return EXIT_SUCCESS;
}

static int Update_DataRec_Ctx(void)
{
    // 除更新了物理磁盘环境外，还嵌套更新了逻辑磁盘环境
    return Update_Physical_Disk_Ctx();
}

static int Update_Testdisk_Ctx(char *error_Buffer)
{
    testdisk_Context *testdisk_Ctx = g_my_Data_Ctx->testdisk_Ctx;
    int testdisk_Mode = TESTDISK_O_RDWR|TESTDISK_O_READAHEAD_8K; // 磁盘读取模式
    char *current_Cmd = NULL;
    int verbose = 1;
    int saveheader = 0;
    unsigned int i = 0;

    // 检查物理磁盘链表可用性
    if (NULL != testdisk_Ctx->physical_Disk_List)
    {
        delete_list_disk(testdisk_Ctx->physical_Disk_List);
    }
    testdisk_Ctx->physical_Disk_List= NULL;

    // 更新物理磁盘链表
    testdisk_Ctx->physical_Disk_List = hd_parse(testdisk_Ctx->physical_Disk_List, verbose, testdisk_Mode);
    if (NULL == testdisk_Ctx->physical_Disk_List)
    {
        sprintf(error_Buffer, "Can't Parse Disk, Please Run With Administator Access");
        
        return EXIT_FAILURE;
    }
    hd_update_all_geometry(testdisk_Ctx->physical_Disk_List, verbose);

    // 遍历物理磁盘链表
    PHYSICAL_DISK_LIST_ITERATOR(testdisk_Ctx->physical_Disk_List)
    {
        disk_t *current_Physical_Disk = list_Walker->disk;

        // 逻辑磁盘配置
        autodetect_arch(current_Physical_Disk, NULL);
        autoset_unit(current_Physical_Disk);

        // 逻辑磁盘读取前检查
        if (0==interface_check_disk_capacity(current_Physical_Disk) &&
            0==interface_check_disk_access(current_Physical_Disk, &current_Cmd) &&
            0==Autoset_Arch_Type(current_Physical_Disk))
        {
            // 检查逻辑磁盘链表可用性
            if (NULL != testdisk_Ctx->logical_Disk_List[i])
            {
                part_free_list(testdisk_Ctx->logical_Disk_List[i]);
            }
            testdisk_Ctx->logical_Disk_List[i]= NULL;

            // 更新逻辑磁盘链表头数组
            testdisk_Ctx->logical_Disk_List[i] = current_Physical_Disk->arch->read_part(current_Physical_Disk, verbose, saveheader);

            ++i; 
        }
    }

    return EXIT_SUCCESS;
}

static int Get_Disk_List(char *error_Buffer)
{
    return Disk_List_Serializer(error_Buffer);
}

static int Update_Disk_List(char *error_Buffer)
{
    if ((EXIT_SUCCESS == Update_Testdisk_Ctx(error_Buffer)) &&
        (EXIT_SUCCESS == Update_DataRec_Ctx()))
    {
        sprintf(error_Buffer, "Update Disk List Success");

        return EXIT_SUCCESS;
    }

    sprintf(error_Buffer, "Update Disk List Failure");

    return EXIT_FAILURE;
}

error_Code Disk_List_Handler(char *error_Buffer)
{
    if (0 == strcmp("set", Get_Action()))
    {
        if (EXIT_SUCCESS == Update_Disk_List(error_Buffer));
        {
            return ERROR_NONE_OK;
        }
    }
    else if (0 == strcmp("get", Get_Action()))
    {
        if (EXIT_SUCCESS == Get_Disk_List(error_Buffer));
        {
            return ERROR_NONE_OK;
        }
    }

    return ERROR_COMMON_ERROR;
}
