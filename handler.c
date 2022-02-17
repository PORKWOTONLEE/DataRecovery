#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "./libtestdisk/include/common.h"
#include "./server/cjson/include/cJSON.h"
#include "./server/civetweb/include/civetweb.h"
#include "./log/log.h"
#include "./handler_component/handler_component.h"
#include "common.h"
#include "handler.h"

extern dataRec_Context *g_dataRec_Ctx;
extern const command_Info gSA_command_Info_List[];
extern const int g_gSA_command_Info_List_Len;

// 响应命令表
const command_Info gSA_command_Info_List[] =
{
    {"Help",             Help},
};

// 响应命令表长度
const int g_gSA_command_Info_List_Len = sizeof(gSA_command_Info_List) / sizeof(gSA_command_Info_List[0]);

// 错误信息表
const error_Info gSA_error_Info_List[] = 
{
    {ERROR_COMMON_ERROE,          "Common Error"},
    {ERROR_NOTHING_MATCH,         "No Command Match"},
    {ERROR_UNKNOW_CONTENT_TYPE,   "Unknown Recived Content Type"},
    {ERROR_UPDATE_DISK_DATA_FAIL, "Update Disk Data Fail"}
};

// 错误信息表长度
const int g_gSA_error_Info_List_Len = sizeof(gSA_error_Info_List) / sizeof(gSA_error_Info_List[0]);

static void Error_Info_Sender(struct mg_connection *conn, int message_type, const char *data, size_t data_len)
{
    Log_Info("[Response Content]:\n%s\n", (char *)data);

    mg_websocket_write(conn, message_type, data, data_len);

    Data_Cleaner(g_dataRec_Ctx->mg_websocket_write_Buffer, strlen(g_dataRec_Ctx->mg_websocket_write_Buffer));

    return;
}

static void Error_Info_Handler(struct mg_connection *conn, error_Code error_Code)
{
    error_Info *error_Info_List = NULL;
    int error_Info_List_Len = 0;
    int error_ID = 0;

    error_Info_List = (error_Info *)gSA_error_Info_List;
    error_Info_List_Len = g_gSA_error_Info_List_Len;

    switch (error_Code)
    {
        // 标准错误信息发送
        case ERROR_NOTHING_MATCH:
        case ERROR_UNKNOW_CONTENT_TYPE:
        case ERROR_UPDATE_DISK_DATA_FAIL:
            error_ID = Array_ID_Matcher(&error_Code, sizeof(error_Code), &(error_Info_List[0].Code), sizeof(error_Info_List[0]), error_Info_List_Len);
            sprintf(g_dataRec_Ctx->mg_websocket_write_Buffer, "Error %d %s", error_Info_List[error_ID].Code, error_Info_List[error_ID].Detail);
        // 标准错误信息发送
        case ERROR_COMMON_ERROE:
        // 匹配正确信息发送
        case ERROR_NONE_OK:
            Error_Info_Sender(conn, MG_WEBSOCKET_OPCODE_TEXT, g_dataRec_Ctx->mg_websocket_write_Buffer, strlen(g_dataRec_Ctx->mg_websocket_write_Buffer));
            break;
    }

    return;
}

static void Command_Function_Caller(struct mg_connection *conn, int command_ID)
{
    if (-1 == command_ID)
    {
        Error_Info_Handler(conn, ERROR_NOTHING_MATCH);
        Log_Debug("No Command ID Matched\n");

        return;
    }

    // 调用响应函数，同时将响应结果通过错误信息发送器发送
    Error_Info_Handler(conn, gSA_command_Info_List[command_ID].Function(g_dataRec_Ctx->mg_websocket_write_Buffer));

    return;
}

static int Command_Name_Matcher(char *command_Name, command_Info *command_List)
{
    int command_ID = -1;
    int command_Info_List_Len = 0;

    Log_Debug("Command Is %s\n", command_Name);
    command_Info_List_Len = g_gSA_command_Info_List_Len;
    command_ID = Array_ID_Matcher(command_Name, strlen(command_Name), &(command_List[0].name), sizeof(command_List[0]), command_Info_List_Len);
    Log_Debug("Command ID Is %d\n", command_ID);

    return command_ID;
}

void Data_Handler(struct mg_connection *conn, int message_type, char *recive_Content, int recive_Content_Size)
{
    int command_ID = -1;

    // 收到内容类型判断
    if (MG_WEBSOCKET_OPCODE_TEXT != (message_type & 0xf)) 
    {
        Log_Debug("Recive Content Type Unknown\n");
        Error_Info_Handler(conn, ERROR_UNKNOW_CONTENT_TYPE);

        return;
    }

    // data是重复使用的，会出现数据重叠，所以需要进行裁剪
    Data_Cutter(recive_Content, recive_Content_Size);

    Log_Info("[Recive Content]:\n%s\n", recive_Content);
    Log_Debug("[Handle Action]:\n");

    command_ID = Command_Name_Matcher(recive_Content, (command_Info *)gSA_command_Info_List);
    Command_Function_Caller(conn, command_ID);

    return;
}

