#ifndef __HANDLER_H__
#define __HANDLER_H__

#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include "libdatarec/include/common.h"
#include "server/cjson/include/cJSON.h"
#include "server/civetweb/include/civetweb.h"
#include "log/log.h"
#include "handler_component/handler_component.h"
#include "common.h"

/**
 * brief：处理server接收到的数据
 *
 * param：conn                server用websocket连接结构体
 * param：message_type        接收到的数据类型
 * param：recive_Content      接收到的数据
 * param：recive_Content_Size 接受到的数据大小
 */
void Request_Handler(struct mg_connection *conn, int message_type, char *recive_Content, int recive_Content_Size);

#endif

