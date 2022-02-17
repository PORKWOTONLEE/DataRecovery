#ifndef __HANDLER_H__
#define __HANDLER_H__

#include <stdio.h>
#include "./server/civetweb/include/civetweb.h"

// 错误码
typedef enum 
{
    ERROR_NONE_OK = 0,
    ERROR_COMMON_ERROE = 1,
    ERROR_NOTHING_MATCH = 701,
    ERROR_UNKNOW_CONTENT_TYPE,
    ERROR_UPDATE_DISK_DATA_FAIL

}error_Code;

// 错误信息结构体
typedef char error_Detail; 
typedef struct 
{
    error_Code Code;
    error_Detail Detail[50];

}error_Info;

// 响应信息
typedef struct
{
    char name[50];
    error_Code (*Function)(char *error_Buffer);

}command_Info;


/**
 * brief：处理server接收到的数据
 *
 * param：conn                server用websocket连接结构体
 * param：message_type        接收到的数据类型
 * param：recive_Content      接收到的数据
 * param：recive_Content_Size 接受到的数据大小
 */
void Data_Handler(struct mg_connection *conn, int message_type, char *recive_Content, int recive_Content_Size);

#endif

