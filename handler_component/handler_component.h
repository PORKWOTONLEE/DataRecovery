#ifndef __HANDLER_COMPONENT_H__
#define __HANDLER_COMPONENT_H__

#include "../common.h"
#include "../handler.h"

// 遍历物理/逻辑磁盘 
#define TRAVERSE_IN_PHYSICAL_DISK_LIST(list) for(list_disk_t *list_Pointer=(list); NULL!=list_Pointer; list_Pointer=list_Pointer->next)
#define TRAVERSE_IN_LOGICAL_DISK_LIST(list)  for(list_part_t *list_Pointer=(list); NULL!=list_Pointer; list_Pointer=list_Pointer->next)

/**
 * brief：返回所有客户端可发送的指令
 *
 * param：error_Buffer 返回客户端数据的buffer
 *
 * return：错误响应码
 */
error_Code Help(char *error_Buffer);
#endif
