#ifndef __HANDLER_COMPONENT_H__
#define __HANDLER_COMPONENT_H__

#include "../common.h"
#include "../handler.h"

// 遍历物理/逻辑磁盘 
#define PHYSICAL_DISK_LIST_ITERATOR(list) for(list_disk_t *list_Walker=(list); NULL!=list_Walker; list_Walker=list_Walker->next)
#define LOGICAL_DISK_LIST_ITERATOR(list)  for(list_part_t *list_Walker=(list); NULL!=list_Walker; list_Walker=list_Walker->next)

/**
 * brief：返回所有客户端可发送的指令
 *
 * param：error_Buffer 返回客户端数据的buffer
 *
 * return：错误响应码
 */
error_Code Help_Handler(char *error_Buffer);

/**
 * brief：磁盘列表相关处理
 *
 * param：error_Buffer 返回客户端数据的buffer
 *
 * return：错误响应码
 */
error_Code Disk_List_Handler(char *error_Buffer);

/**
 * brief：文件列表相关处理
 *
 * param：error_Buffer 返回客户端数据的buffer
 *
 * return：错误响应码
 */
error_Code File_List_Handler(char *error_Buffer);

/**
 * brief：文件处理相关
 *
 * param：error_Buffer 返回客户端数据的buffer
 *
 * return：错误响应码
 */
error_Code File_Handler(char *error_Buffer);

#endif
