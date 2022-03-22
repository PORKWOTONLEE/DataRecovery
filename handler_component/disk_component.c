#include <windows.h>

#include "handler_component.h"

extern my_Data_Context *g_my_Data_Ctx;

static error_Code Disk_List_Serializer(char *error_Buffer) 
{ 
    data_Recovery_Context *data_Rec_Ctx = g_my_Data_Ctx->data_Rec_Ctx;
    char disk_No_Buffer[25] = {};
    char partition_No_Buffer[25] = {};
    char disk_Size_Buffer[25] = {};
    char partition_Size_Buffer[25] = {};
    char *disk_List_Serialized = NULL;
    cJSON *disk_Json = NULL;
    int partition_Num = 0;
    static int j = 0;

    // 由于j是静态变量，所以使用前要置0
    j = 0;

    disk_Json = cJSON_CreateObject();

    // 硬盘总数量
    cJSON_AddNumberToObject(disk_Json, "disk_Total_Num", data_Rec_Ctx->disk_Ctx->total_Num);

    // 分区总数量
    cJSON_AddNumberToObject(disk_Json, "partition_Total_Num", data_Rec_Ctx->partition_Ctx->total_Num);

    // 更新硬盘信息
    for (int i=0; i<data_Rec_Ctx->disk_Ctx->total_Num; ++i)
    {
        Log_Debug("Serializing disk_%02d\n", i);

        cJSON *current_Disk_Info = NULL;
        current_Disk_Info = cJSON_CreateObject();

        // 更新硬盘序号
        cJSON_AddNumberToObject(current_Disk_Info, "no",               data_Rec_Ctx->partition_Ctx->info[i].no);

        // 更新硬盘名称
        cJSON_AddStringToObject(current_Disk_Info, "name",             data_Rec_Ctx->disk_Ctx->info[i].name);

        // 更新硬盘大小
        Convert_Size_To_Unit(data_Rec_Ctx->disk_Ctx->info[i].size, (char *)&disk_Size_Buffer, 1024);
        cJSON_AddStringToObject(current_Disk_Info, "size",             disk_Size_Buffer);

        // 更新硬盘中分区数量
        cJSON_AddNumberToObject(current_Disk_Info, "partition_Num", data_Rec_Ctx->disk_Ctx->partition_In_Per_Disk[i]);

        // 更新分区信息
        partition_Num = j + data_Rec_Ctx->disk_Ctx->partition_In_Per_Disk[i];
        for (; j<partition_Num; ++j)
        {
            Log_Debug("Serializing partition_%02d\n", j);

            cJSON *current_Logical_Disk_Info = NULL;
            current_Logical_Disk_Info = cJSON_CreateObject(); 

            // 更新分区序号
            cJSON_AddNumberToObject(current_Logical_Disk_Info, "no",                data_Rec_Ctx->partition_Ctx->info[j].no);

            // 更新分区盘符 
            cJSON_AddStringToObject(current_Logical_Disk_Info, "letter",            data_Rec_Ctx->partition_Ctx->info[j].letter);

            // 更新分区名称
            cJSON_AddStringToObject(current_Logical_Disk_Info, "name",              data_Rec_Ctx->partition_Ctx->info[j].name);

            // 更新分区大小
            Convert_Size_To_Unit(data_Rec_Ctx->partition_Ctx->info[j].size, (char *)&partition_Size_Buffer, 1024);
            cJSON_AddStringToObject(current_Logical_Disk_Info, "size",              partition_Size_Buffer);

            // 更新分区文件系统
            cJSON_AddStringToObject(current_Logical_Disk_Info, "file_System",       data_Rec_Ctx->partition_Ctx->info[j].file_System);

            // 更新分区文件系统额外信息
            cJSON_AddStringToObject(current_Logical_Disk_Info, "file_System_Extra", data_Rec_Ctx->partition_Ctx->info[j].file_System_Extra);

            // 更新分区编号
            sprintf(partition_No_Buffer,"partition_No_%u", data_Rec_Ctx->partition_Ctx->info[j].no);
            cJSON_AddItemToObject(current_Disk_Info, partition_No_Buffer, current_Logical_Disk_Info);
        }

        // 更新硬盘编号
        sprintf(disk_No_Buffer, "disk_No_%u", data_Rec_Ctx->disk_Ctx->info[i].no);
        cJSON_AddItemToObject(disk_Json, disk_No_Buffer, current_Disk_Info);
    }

    // 输出响应信息到buffer
    disk_List_Serialized = cJSON_Print(disk_Json); 
    strcpy(error_Buffer, disk_List_Serialized);

    // 清理临时数据
    free(disk_List_Serialized);
    cJSON_Delete(disk_Json);

    return ERROR_NONE_OK; 
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

static error_Code Update_Partition_Ctx(unsigned int i)
{
    data_Recovery_Context *data_Rec_Ctx = g_my_Data_Ctx->data_Rec_Ctx;
    testdisk_Context *testdisk_Ctx = g_my_Data_Ctx->testdisk_Ctx;
    static int j = 0;

    // 如果i=0，那么说明需要从头更新分区信息 
    if (0 == i)
    {
        j = 0;
    }

    // 防止当前硬盘的分区总数累加
    data_Rec_Ctx->disk_Ctx->partition_In_Per_Disk[i] = 0;

    PARTITION_LIST_ITERATOR(testdisk_Ctx->partition_List[i])
    {
        Log_Debug("Updating partition_%02d\n", j);

        partition_t *current_Logical_Disk = list_Walker->part;

        // 更新分区序号
        data_Rec_Ctx->partition_Ctx->info[j].no = j;

        // 更新分区盘符
        sprintf(data_Rec_Ctx->partition_Ctx->info[j].letter, "%c", Get_Partition_Letter(current_Logical_Disk));
        
        // 更新分区名称
        strcpy(data_Rec_Ctx->partition_Ctx->info[j].name, current_Logical_Disk->fsname);

        // 更新分区容量
        //Convert_Size_To_Unit(current_Logical_Disk->part_size, buffer_Logical_Disk_Size, 1024);
        data_Rec_Ctx->partition_Ctx->info[j].size = current_Logical_Disk->part_size;

        // 更新分区文件系统
        strcpy(data_Rec_Ctx->partition_Ctx->info[j].file_System, current_Logical_Disk->info);

        // 更新分区文件系统额外信息
        strcpy(data_Rec_Ctx->partition_Ctx->info[j].file_System_Extra, current_Logical_Disk->partname);

        // 更新当前硬盘的分区总数
        ++data_Rec_Ctx->disk_Ctx->partition_In_Per_Disk[i];

        ++j;
    }

// 更新分区总数
data_Rec_Ctx->partition_Ctx->total_Num = j;

return ERROR_NONE_OK;
}

static error_Code Update_Disk_Ctx(void)
{
    data_Recovery_Context * data_Rec_Ctx = g_my_Data_Ctx->data_Rec_Ctx;
    testdisk_Context *testdisk_Ctx = g_my_Data_Ctx->testdisk_Ctx;
    char buffer_Disk_Size[20];
    unsigned int i = 0;

    // 遍历硬盘链表
    DISK_LIST_ITERATOR(testdisk_Ctx->disk_List)
    {
        Log_Debug("Updating disk_%02d\n", i);

        disk_t *current_Disk = list_Walker->disk;

        // 更新硬盘序号
        data_Rec_Ctx->disk_Ctx->info[i].no = i;

        // 更新硬盘型号
        strcpy(data_Rec_Ctx->disk_Ctx->info[i].name, current_Disk->model);

        // 更新硬盘容量
        data_Rec_Ctx->disk_Ctx->info[i].size = current_Disk->disk_size;

        // 更新当前硬盘下分区信息
        Update_Partition_Ctx(i);

        ++i; 
    }

    // 更新硬盘总数
    data_Rec_Ctx->disk_Ctx->total_Num = i;

    return ERROR_NONE_OK;
}

static error_Code Update_DataRec_Ctx(void)
{
    // 除更新了硬盘环境外，还嵌套更新了分区环境
    return Update_Disk_Ctx();
}

static error_Code Update_Testdisk_Ctx(char *error_Buffer)
{
    // 检查硬盘链表可用性
    Disk_List_Free();

    // 更新硬盘链表
    Disk_List_Init(error_Buffer);

    // 检查分区链表可用性
    Partition_List_Free(); 

    // 更新分区链表
    Partition_List_Init();

    return ERROR_NONE_OK;
}

static error_Code Get_Disk_List(char *error_Buffer)
{
    return Disk_List_Serializer(error_Buffer);
}

static error_Code Update_Disk_List(char *error_Buffer)
{
    if ((ERROR_NONE_OK == Update_Testdisk_Ctx(error_Buffer)) &&
        (ERROR_NONE_OK == Update_DataRec_Ctx()))
    {
        Set_Respond_Buffer("Update Success");

        return ERROR_NONE_OK;
    }

    Set_Respond_Buffer("Update Fail");

    return ERROR_COMMON_ERROR;
}

error_Code Disk_Handler(char *error_Buffer)
{
    if (0 == strcmp("set", Get_Action()))
    {
        return Update_Disk_List(error_Buffer);
    }
    else if (0 == strcmp("get", Get_Action()))
    {
        return Get_Disk_List(error_Buffer);
    }

    return ERROR_COMMON_ERROR;
}
