#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>
#include <time.h>
#include "./libtestdisk/include/common.h"
#include "./server/civetweb/include/civetweb.h"
#include "./server/cjson/include/cJSON.h"

#define physical_Disk_Info_Num 30
#define logical_Disk_Info_Num  128

// 逻辑磁盘上下文
typedef struct
{
    list_disk_t *physical_Disk_List;                        // 全局物理磁盘信息链表头
    list_part_t *logical_Disk_List[physical_Disk_Info_Num]; // 全局逻辑磁盘信息链表头

}testdisk_Context;

// 物理磁盘信息
typedef struct
{
   char No[20];
   char name[30];
   char size[20];

}physical_Disk_Info;

// 逻辑磁盘信息
typedef struct
{
   char No[20];
   char letter[5];
   char name[20];
   char size[20];
   char file_System[30];
   char file_System_Extra[30];

}logical_Disk_Info;

// 物理磁盘上下文
typedef struct
{
    physical_Disk_Info *info; 
    unsigned int total_Num;
    unsigned int logDisk_Num_In_Per_phyDisk[physical_Disk_Info_Num]; // 单个物理磁盘所含逻辑磁盘数量

}physical_Disk_Context;

// 逻辑磁盘上下文
typedef struct
{
    logical_Disk_Info *info;
    unsigned int total_Num;

}logical_Disk_Context;

// 数据恢复上下文
typedef char error_Info_Buffer;
typedef struct 
{
    // 信息回传buffer
    error_Info_Buffer *mg_websocket_write_Buffer;

    // testdisk数据 
    testdisk_Context* testdisk_Ctx;

    physical_Disk_Context *physical_Disk_Ctx;

    logical_Disk_Context *logical_Disk_Ctx;

}dataRec_Context;

/**
 * brief：在字符串data末尾添加'\0'
 *
 * param：data     字符串数据
 * param：datasize 字符串长度
 */
void Data_Cutter(char *data, int datasize);


/**
 * brief：将data中datasize大小的数据置0
 *
 * param：data     数据
 * param：datasize 数据大小
 */
void Data_Cleaner(char *data, int datasize);

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
 * brief：初始化数据恢复环境
 *
 * return：EXIT_SUCCESS成功或者EXIT_FAILURE失败
 */
int DataRec_Context_Init(void);

/**
 * brief：清理数据恢复环境
 *
 * return：EXIT_SUCCESS成功或者EXIT_FAILURE失败
 */
int DataRec_Context_Free(void);

#endif

