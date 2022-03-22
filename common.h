#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "libdatarec/include/common.h"
#include "libdatarec/include/dir_common.h"
#include "libdatarec/include/filegen.h"
#include "libdatarec/include/photorec.h"
#include "server/cjson/include/cJSON.h"
#include "server/civetweb/include/civetweb.h"

// 响应buffer大小
#define RESPONSE_BUFFER_SIZE 4096

// 请求buffer大小
#define REQUEST_BUFFER_SIZE 100 

// 请求参数个数
#define PARAM_NUM 10 
// 请求参数长度
#define PARAM_LEN 10 

// 物理硬盘个数
#define DISK_NUM 30 

// 逻辑分区个数
#define PARTITION_NUM 128

// 快速搜索文件个数
#define FILE_NUM 1000000

// 默认快速/深度文件恢复路径
#define DEFAULT_QUICKSEARCH_FILE_PATH ".\\tmp\\quick"
#define DEFAULT_DEEPSEARCH_FILE_PATH  ".\\tmp\\deep"
#define DEFAULT_DEEPSEARCH_FILE_RECUP_PATH ".\\tmp\\deep\\recup"

// 请求错误码
typedef enum 
{
    ERROR_NONE_OK           = 0,
    ERROR_COMMON_ERROR      = 1,
    ERROR_UNKNOWN_REQUEST_TYPE = 701,
    ERROR_UNKNOWN_REQUEST_HEAD,
    ERROR_REQUEST_ONLY_HAS_HEAD,
    ERROR_UNKNOWN_REQUEST_ACTION,
    ERROR_UNKNOWN_REQUEST_COMMAND,
    ERROR_UNKNOWN_REQUEST_PARAM,
    ERROR_REQUEST_PARAM_LEN_TOO_LONG

}error_Code;

// 请求错误信息
typedef struct 
{
    error_Code Code;
    char       Detail[50];

}error_Info;

// 命令信息
typedef struct
{
    char       name[50];                        
    error_Code (*Function)(char *error_Buffer); // 命令对应处理函数

}command_Info;

// 状态码
typedef enum 
{
    IDLE = 0,
    QUICK_SCANNING,
    DEEP_SCANNING,

}status_Code;

// 控制码
typedef enum 
{
    SCAN_CONTINUE= 0,
    SCAN_STOP

}control_Code;

// 快速/深度扫描文件列表可用性
typedef enum
{
    LIST_NOT_READY = 0,
    LIST_READY

}scan_File_List_Available_Code;

// 扫描处理程序识别码
typedef enum
{
    QUICK_SCAN = 0,
    DEEP_SCAN,
    STOP_SCAN,
    SET_FILE_EXTENSION,

}scan_Handler_Set_Code;

// 请求环境
typedef struct
{
    char *request_Buffer;
    char datarec[10];                 // datarec头
    char action[10];      
    char command[30];     
    int  command_ID;                  // 命令ID
    char param[PARAM_NUM][PARAM_LEN]; // 参数
    int  param_Num;       

}request_Context;

// 响应环境
typedef struct
{
    char       *respond_Buffer;
    error_Code result;

}respond_Context;

// server环境
typedef struct
{
    request_Context *request_Ctx;
    respond_Context *respond_Ctx;

}server_Context;

typedef struct
{
    disk_t      *disk;      // 物理磁盘信息链表
    partition_t *partition; // 逻辑磁盘信息链表

}scan_Object_Context;

// testdisk恢复文件环境
typedef struct
{
    file_info_t list;
    dir_data_t  data;

}testdisk_File_Context;

// photorec恢复文件环境
typedef struct
{
    struct ph_param   param;
    struct ph_options options;
    alloc_data_t      data;
    file_enable_t     *file_Extension_Surpport_List;

}photorec_File_Context;

// testdisk数据环境
typedef struct
{
    list_disk_t           *disk_List;                // 物理磁盘信息链表头
    list_part_t           *partition_List[DISK_NUM]; // 逻辑磁盘信息链表头数组
    scan_Object_Context   *scan_Object_Ctx;
    testdisk_File_Context *testdisk_File_Ctx;
    photorec_File_Context *photorec_File_Ctx;

}testdisk_Context;

// 物理硬盘信息
typedef struct
{
    uint32_t no;
    char     name[30];
    uint64_t size;

}disk_Info;

// 逻辑分区信息
typedef struct
{
    uint32_t no;
    char     letter[2];
    char     name[20];
    uint64_t size;
    char     file_System[30];
    char     file_System_Extra[30];

}partition_Info;

// 文件信息
typedef struct
{
    unsigned int no;
    char         name[200];        // 带路径的名称
    unsigned int size;
    uint64_t     inode;            // inode
    time_t       last_Access_Time; // 最后访问时间
    time_t       last_Modify_Time; // 最后修改时间
    time_t       last_Change_Time; // 最后改变时间

}file_Info;

// 物理硬盘环境
typedef struct
{
    disk_Info    *info;
    unsigned int total_Num;
    unsigned int partition_In_Per_Disk[DISK_NUM]; // 单个物理磁盘所含逻辑磁盘数量

}disk_Context;

// 逻辑分区环境
typedef struct
{
    partition_Info *info;
    unsigned int   total_Num;

}partition_Context;

// 文件环境
typedef struct
{
    file_Info    *info;
    unsigned int total_Num;
    unsigned int total_Page;
    unsigned int current_Page;
    char         destination[200];

}file_Context;

// 搜索处理环境
typedef struct
{
    double percent;
    control_Code control;

}process_Context;

typedef struct
{
    status_Code                   status;
    scan_File_List_Available_Code quick_Scan_File_List;
    scan_File_List_Available_Code deep_Scan_File_List;
    process_Context               *process_Ctx;                          

}info_Context;

// 数据恢复环境
typedef struct
{
    disk_Context      *disk_Ctx;
    partition_Context *partition_Ctx;
    file_Context      *file_Ctx;
    info_Context      *info_Ctx;

}data_Recovery_Context;

// 我的数据环境
typedef struct 
{
    server_Context        *server_Ctx;
    testdisk_Context      *testdisk_Ctx;
    data_Recovery_Context *data_Rec_Ctx;

}my_Data_Context;

/**
 * brief：清空快速搜索文件结构体
 */
void File_Info_Clear(void);

/**
 * brief：重置本次request环境中的所有信息
 */
void Server_Cleaner(void);

/**
 * brief：给数据大小添加单位
 *
 * param：source_Size      原始数据大小
 * param：destination_Size 带单位的数据大小
 * param：radix            进制，支持1000/1024
 */
void Convert_Size_To_Unit(const uint64_t source_Size, char *size_With_Unit, int radix);

/**
 * brief：将datarec状态转换成字符串
 *
 * param：status 状态码
 *
 * return：状态码对应字符串
 */
char *Convert_Status_To_Char(status_Code status);

/**
 * brief：将快速/深度扫描文件列表可用性转换成字符串
 *
 * param：available 文件列表可用码
 *
 * return：文件列表可用码对应字符串
 */
char *Convert_List_Available_To_Char(scan_File_List_Available_Code available);

/**
 * brief：自动设置物理磁盘类型
 *
 * param：disk 物理磁盘指针
 *
 * return：EXIT_SUCCESS成功或者EXIT_FAILURE失败
 */
int Autoset_Arch_Type(disk_t *disk);

/**
 * brief：在数组array中查找是否存在成员match_Member，可以匹配结构体类型数组
 *
 * param：match_Member     待匹配项
 * param：match_Member_Len 待匹配项长度
 * param：array            数组
 * param：array_Member_Len 数组成员大小
 * param：array_Len        数组长度
 *
 * return：待匹配项在数组中的编号
 */
int Array_ID_Matcher(const void *match_Member, const int match_Member_Len, const void *array, const int array_Member_Len, const int array_Len);

/**
 * brief：移动文件
 *
 * param：dest 文件目标路径
 * param：src 文件源路径
 */
void Move_File(const char *dest, const char *src);

/**
 * brief：删除文件
 *
 * param：file_Name 文件名字
 */
void Delete_File(const char *file_Name);

/**
 * brief：设置请求buffer
 *
 * param：format 格式化语句
 * param：... 参数
 */
void Set_Request_Buffer(const char *format, ...);

/**
 * brief：获取请求buffer
 *
 * return：请求buffer指针
 */
char *Get_Request_Buffer(void);

/**
 * brief：重置请求buffer
 */
void Request_Buffer_Reset(void);

/**
 * brief：设置请求datarec头
 *
 * param：datarec 要设置的字符串
 */
void Set_DataRec(char *datarec);

/**
 * brief：获取请求datarec头
 *
 * return：datarec中的字符串
 */
char *Get_DataRec(void);

/**
 * brief：设置请求动作
 *
 * param：action 要设置的字符串
 */
void Set_Action(char *action);

/**
 * brief：获取请求动作
 *
 * return：动作中的字符串
 */
char *Get_Action(void);

/**
 * brief：设置请求命令
 *
 * param：command 要设置的字符串
 */
void Set_Command(char *command);

/**
 * brief：获取请求命令
 *
 * return：命令中的字符串
 */
char *Get_Command(void);

/**
 * brief：设置请求命令ID
 *
 * param：command_ID 命令ID值
 */
void Set_Command_ID(int command_ID);

/**
 * brief：获取请求命令ID
 *
 * return：命令ID值
 */
unsigned int Get_Command_ID(void);

/**
 * brief：设置请求参数
 *
 * param：param_No 参数序号
 * param：param 参数
 */
void Set_Param(unsigned int param_No, char *param);

/**
 * brief：重置请求参数
 */
void Param_Reset(void);

/**
 * brief：按照序号获取字符串类型的参数
 *
 * param：param_No 参数序号
 *
 * return：序号对应参数字符串
 */
char *Get_Char_Param(unsigned int param_No);

/**
 * brief：按照序号获取整型类型的参数
 *
 * param：param_No 参数序号
 *
 * return：序号对应参数整数值
 */
int Get_Int_Param(unsigned int param_No);

/**
 * brief：设置请求参数个数
 *
 * param：param_Num 参数个数
 */
void Set_Param_Num(int param_Num);

/**
 * brief：获取请求参数个数
 *
 * return：返回参数个数
 */
unsigned int Get_Param_Num(void);

/**
 * brief：设置响应buffer
 *
 * param：format 格式化语句
 * param：... 参数
 */
void Set_Respond_Buffer(const char *format, ...);

/**
 * brief：获取响应buffer
 *
 * return：响应buffer指针
 */
char *Get_Respond_Buffer(void);

/**
 * brief：重置响应buffer
 */
void Respond_Buffer_Reset(void);

/**
 * brief：设置响应结果
 *
 * param：result 结果值
 */
void Set_Result(error_Code result);

/**
 * brief：获取响应结果
 *
 * return：结果值
 */
error_Code Get_Result(void);

/**
 * brief：重置server环境
 */
void Server_Context_Reset(void);

/**
 * brief：设置datarec状态
 *
 * param：status 状态码
 */
void Set_Status(status_Code status);

/**
 * brief：获取datarec状态
 *
 * return：状态码
 */
status_Code Get_Status(void);

/**
 * brief：设置扫描控制标识，用于控制扫描流程
 *
 * param：control 控制码
 */
void Set_Control(control_Code control);

/**
 * brief：获取扫描控制标识
 *
 * return：控制码
 */
control_Code Get_Control(void);

/**
 * brief：设置扫描百分比
 *
 * param：percent 百分比值
 */
void Set_Percent(double percent);

/**
 * brief：获取扫描百分比
 *
 * return：百分比值
 */
double Get_Percent(void);

/**
 * brief：重置datarec信息
 */
void Info_Context_Reset(void);

/**
 * brief：初始化硬盘链表
 *
 * param：error_Buffer 响应buffer
 */
void Disk_List_Init(char *error_Buffer);

/**
 * brief：清空硬盘链表
 */
void Disk_List_Free(void);

/**
 * brief：初始化分区链表
 */
void Partition_List_Init();

/**
 * brief：清空分区链表数组
 */
void Partition_List_Free(void);

/**
 * brief：初始化testdisk恢复文件表
 */
void File_List_Init(void);

/**
 * brief：清空testdisk恢复文件表
 */
void File_List_Free(void);

/**
 * brief：初始化testdisk恢复文件数据
 */
void File_Data_Init(int *result);

/**
 * brief：清空testdisk恢复文件数据
 */
void File_Data_Free(void);

/**
 * brief：初始化testdisk恢复文件结构体
 */
void Testdisk_File_Context_Init(int *result);

/**
 * brief：清空testdisk恢复文件结构体
 */
void Testdisk_File_Context_Free(void);

/**
 * brief：设置要搜索的硬盘
 *
 * param：disk_No 硬盘编号
 */
void Set_Scan_Disk(unsigned int disk_No);

/**
 * brief：获取要搜索的硬盘
 *
 * return：硬盘指针
 */
disk_t *Get_Scan_Disk(void);

/**
 * brief：设置要搜索的硬盘中的分区
 *
 * param：disk_No 硬盘编号
 * param：partition_No 硬盘对应的分区编号
 */
void Set_Scan_Partition(unsigned int disk_No, unsigned int partition_No);

/**
 * brief：获取要搜索的硬盘中的分区
 *
 * return：分区指针
 */
partition_t *Get_Scan_Partition(void);

/**
 * brief：初始化搜索目标环境
 *
 * param：disk_No 硬盘编号
 * param：partition_No 硬盘对应的分区编号
 *
 * return：初始化成功与否
 */
error_Code Scan_Object_Context_Init(unsigned int disk_No, unsigned int partition_No);

/**
 * brief：释放搜索目标环境
 */
void Scan_Object_Context_Free(void);

/**
 * brief：初始化photorec恢复文件参数
 */
void Param_Init(void);

/**
 * brief：初始化photorec恢复文件选项
 */
void Option_Init(void);

/**
 * brief：初始化photorec恢复文件数据
 */
void Data_Init(void);

/**
 * brief：移除photorec恢复文件已用空间
 */
void Remove_Used_Space(void);

/**
 * brief：设置快速扫描文件列表可用性
 *
 * param：available 文件列表可用码
 */
void Set_Quick_Scan_File_List_Available(scan_File_List_Available_Code available);

/**
 * brief：获取快速扫描文件列表可用性
 *
 * return：文件列表可用码
 */
scan_File_List_Available_Code Get_Quick_Scan_File_List_Available(void);

/**
 * brief：设置深度扫描文件列表可用性
 *
 * param：available 文件列表可用码
 */
void Set_Deep_Scan_File_List_Available(scan_File_List_Available_Code available);

/**
 * brief：获取深度扫描文件列表可用性
 *
 * return：文件列表可用码
 */
scan_File_List_Available_Code Get_Deep_Scan_File_List_Available(void);

bool  IsProcessRunAsAdmin();
int Run_As_Admin(void);

/**
 * brief：应用深度扫描文件支持列表配置
 */
void Apply_File_Extension_Support_Config(void);

/**
 * brief：保存深度扫描文件支持列表配置
 */
void Save_File_Extension_Support_List_Config(void);

/**
 * brief：初始化photorec恢复文件环境
 */
void Photorec_Context_Init(void);

/**
 * brief：初始化数据恢复环境
 *
 * return：EXIT_SUCCESS成功或者EXIT_FAILURE失败
 */
int My_Data_Context_Init(void);

/**
 * brief：清理数据恢复环境
 *
 * return：EXIT_SUCCESS成功或者EXIT_FAILURE失败
 */
int My_Data_Context_Free(void);

#endif
