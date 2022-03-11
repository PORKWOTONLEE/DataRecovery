#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "./libdatarec/include/common.h"
#include "./libdatarec/include/dir_common.h"
#include "./libdatarec/include/filegen.h"
#include "./libdatarec/include/photorec.h"
#include "./server/civetweb/include/civetweb.h"
#include "./server/cjson/include/cJSON.h"

#define RESPONSE_BUFFER_SIZE   4096
#define RECEIVE_BUFFER_SIZE    100 
#define PHYSICAL_DISK_NUM      30 
#define PHYSICAL_DISK_INFO_NUM 30
#define LOGICAL_DISK_INFO_NUM  128
#define FILE_INFO_NUM          1000000

// 默认文件恢复路径
#define DEFAULT_QUICKSEARCH_FILE_PATH ".\\tmp\\quick"
#define DEFAULT_DEEPSEARCH_FILE_PATH  ".\\tmp\\deep"

// 错误码
typedef enum 
{
    ERROR_NONE_OK = 0,
    ERROR_COMMON_ERROR = 1,
    ERROR_UNKNOWN_DATA_TYPE = 701,
    ERROR_ONLY_HEAD,
    ERROR_UNKNOWN_DATA_HEAD,
    ERROR_UNKNOWN_DATA_ACTION,
    ERROR_UNKNOWN_DATA_COMMAND,
    ERROR_UNKNOWN_DATA_PARAM,
    ERROR_DONT_HAVE_SUCH_COMMAND

}error_Code;

// 错误信息
typedef struct 
{
    error_Code Code;
    char       Detail[50];

}error_Info;

// 响应信息
typedef struct
{
    char       name[50];
    error_Code (*Function)(char *error_Buffer);

}command_Info;

// 数据接收环境
typedef struct
{
    char *receive_Buffer; // 数据接收buffer
    char datarec[10];     // datarec标记
    char action[10];      // 动作
    char command[30];     // 命令
    char param[5][6];     // 参数
    int  param_Num;       // 参数个数
    int  command_ID;      // 命令ID

}receive_Context;

// 数据响应环境
typedef struct
{
    error_Code result;          // 数据响应结果
    char       *respond_Buffer; // 数据响应buffer

}respond_Context;

// server数据环境
typedef struct
{
    receive_Context *receive_Ctx;
    respond_Context *respond_Ctx;

}server_Context;

// 恢复文件结构体
typedef struct
{
    file_info_t list; // 文件列表
    dir_data_t  data; // 文件数据

}testdisk_File_Context;

// 恢复文件结构体
typedef struct
{
    struct ph_param   param;   // 参数
    struct ph_options options; // 选项
    alloc_data_t      data;    // 数据

}photorec_File_Context;

// testdisk数据环境
typedef struct
{
    list_disk_t           *physical_Disk_List;                   // 物理磁盘信息链表头
    list_part_t           *logical_Disk_List[PHYSICAL_DISK_NUM]; // 逻辑磁盘信息链表头数组
    testdisk_File_Context *testdisk_File_Ctx;                    // testdisk恢复文件结构体
    photorec_File_Context *photorec_File_Ctx;                    // photorec恢复文件结构体

}testdisk_Context;

// 物理磁盘信息
typedef struct
{
    uint32_t no;       // 编号
    char     name[30]; // 名称
    uint64_t size;     // 容量

}physical_Disk_Info;

// 逻辑磁盘信息
typedef struct
{
    uint32_t no;                    // 编号
    char     letter[2];             // 盘符
    char     name[20];              // 名称
    uint64_t size;                  // 容量
    char     file_System[30];       // 文件系统
    char     file_System_Extra[30]; // 文件系统额外信息

}logical_Disk_Info;

// 文件信息
typedef struct
{
    unsigned int no;         // 编号
    char   path_Name[200];   // 路径/名称
    unsigned int size;       // 大小 
    uint64_t inode;          // inode
    time_t last_Access_Time; // 最后访问时间
    time_t last_Modify_Time; // 最后修改时间
    time_t last_Change_Time; // 最后改变时间

}file_Info;

// 物理磁盘环境
typedef struct
{
    physical_Disk_Info *info;                                         // 详细信息 
    unsigned int       total_Num;                                     // 总数
    unsigned int       logDisk_Num_In_Per_phyDisk[PHYSICAL_DISK_NUM]; // 单个物理磁盘所含逻辑磁盘数量

}physical_Disk_Context;

// 逻辑磁盘环境
typedef struct
{
    logical_Disk_Info *info;     // 详细信息
    unsigned int      total_Num; // 总数

}logical_Disk_Context;

// 快速恢复文件环境
typedef struct
{
    file_Info    *info;        // 详细信息
    unsigned int total_Num;    // 文件总个数
    unsigned int total_Page;   // 文件总页数
    unsigned int current_Page; // 当前文件页数
    char destination[200];     // 文件恢复目录

}recovery_File_Context;

// 数据恢复环境
typedef struct
{
    physical_Disk_Context *physical_Disk_Ctx;
    logical_Disk_Context  *logical_Disk_Ctx;
    recovery_File_Context *file_Ctx;

}data_Recovery_Context;

// 我的数据环境
typedef struct 
{
    server_Context        *server_Ctx;
    testdisk_Context      *testdisk_Ctx;
    data_Recovery_Context *data_Rec_Ctx;

}my_Data_Context;

/**
 * brief：清空文件夹数据
 */
void File_Info_Clear(void);

/**
 * brief：将data中datasize大小的数据置0
 */
void Data_Cleaner(void);

/**
 * brief：给数据大小添加单位
 *
 * param：source_Size      原始数据大小
 * param：destination_Size 带单位的数据大小
 * param：radix            进制，支持1000/1024
 */
void Convert_Size_To_Unit(const uint64_t source_Size, char *size_With_Unit, int radix);

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
 * brief：按序号获取server接收环境中解析到的动作
 *
 * return：set或get字符串的指针
 */
char *Get_Action(void);

/**
 * brief：获取参数个数
 *
 * return：参数个数
 */
unsigned int Get_Param_Num(void);

/**
 * brief：按序号获取server接收环境中的解析到的参数
 *
 * param：param_No 参数序号
 *
 * return：param_No对应参数
 */
char *Get_Param(unsigned int param_No);

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

