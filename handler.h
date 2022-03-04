#ifndef __HANDLER_H__
#define __HANDLER_H__

#include <stdio.h>
#include "./server/civetweb/include/civetweb.h"

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

