#ifndef __HANDLER_COMPONENT_H__
#define __HANDLER_COMPONENT_H__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../log/log.h"
#include "../common.h"
#include "../handler.h"

// 遍历物理/逻辑磁盘 
#define DISK_LIST_ITERATOR(list) for(list_disk_t *list_Walker=(list); NULL!=list_Walker; list_Walker=list_Walker->next)
#define PARTITION_LIST_ITERATOR(list)  for(list_part_t *list_Walker=(list); NULL!=list_Walker; list_Walker=list_Walker->next)

/**
 * brief：返回当前datarec的信息
 *
 * param：error_Buffer 返回客户端数据的buffer
 *
 * return：错误响应码
 */
error_Code Info_Handler(char *error_Buffer);

/**
 * brief：磁盘列表相关处理
 *
 * param：error_Buffer 返回客户端数据的buffer
 *
 * return：错误响应码
 */
error_Code Disk_Handler(char *error_Buffer);

/**
 * brief：文件扫描相关处理
 *
 * param：error_Buffer 返回客户端数据的buffer
 *
 * return：错误响应码
 */
error_Code Scan_Handler(char *error_Buffer);

/**
 * brief：文件恢复处理相关
 *
 * param：error_Buffer 返回客户端数据的buffer
 *
 * return：错误响应码
 */
error_Code Recovery_Handler(char *error_Buffer);

#endif
