#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include "./libtestdisk/include/common.h"
#include "./server/cjson/include/cJSON.h"
#include "./server/civetweb/include/civetweb.h"
#include "./log/log.h"
#include "./handler_component/handler_component.h"
#include "common.h"
#include "handler.h"

extern my_Data_Context *g_my_Data_Ctx;
extern const command_Info gSA_command_Info_List[];
extern const int ggSA_command_Info_List_Len;

// 响应命令表
const command_Info gSA_command_Info_List[] =
{
    {"Help",     Help_Handler},
    {"DiskList", Disk_List_Handler},
    {"FileList", File_List_Handler},
    {"File",     File_Handler}
};

// 响应命令表长度
const int ggSA_command_Info_List_Len = sizeof(gSA_command_Info_List) / sizeof(gSA_command_Info_List[0]);

// 错误信息表
const error_Info gSA_error_Info_List[] = 
{
    {ERROR_NONE_OK,                 "Everything Is Just Fine"},
    {ERROR_COMMON_ERROR,            "Common Error"},
    {ERROR_UNKNOWN_DATA_TYPE,       "Please Check Your WebSocket Client"},
    {ERROR_UNKNOWN_DATA_HEAD,       "Unknown Data's Head"},
    {ERROR_UNKNOWN_DATA_ACTION,     "Unknown Data's Action"},
    {ERROR_UNKNOWN_DATA_COMMAND,    "Unknown Data's Command"},
    {ERROR_UNKNOWN_DATA_PARAM,      "Unknown Data's Param"},
    {ERROR_DONT_HAVE_SUCH_COMMAND,  "Nothing To Respond Accordiong To You Command"},
};

// 错误信息表长度
const int ggSA_error_Info_List_Len = sizeof(gSA_error_Info_List) / sizeof(gSA_error_Info_List[0]);

static void Check_receive_Buffer(char *data, receive_Context *receive_Ctx)
{
    strcpy(receive_Ctx->receive_Buffer, data);
}

static void Check_datarec(char *str, receive_Context *receive_Ctx, respond_Context *respond_Ctx)
{
    char *t_str = NULL;

    // 分割字符
    t_str = strtok(str, "$");
    if (NULL == t_str)
    {
        respond_Ctx->result = ERROR_ONLY_HEAD;

        return; 
    }

    // 判断头部
    if (0 == strcmp(str, "datarec"))
    {
        strcpy(receive_Ctx->datarec, str);

        respond_Ctx->result = ERROR_NONE_OK;

        return; 
    }
    else
    {
        respond_Ctx->result = ERROR_UNKNOWN_DATA_HEAD;

        return; 
    }
}

static void Check_Action(receive_Context *receive_Ctx, respond_Context *respond_Ctx)
{
    // 响应错误判断
    if (ERROR_NONE_OK != respond_Ctx->result)
    {
        return;
    }

    char *t_str = NULL;

    // 分割字符
    t_str = strtok(NULL, "$");
    if (NULL == t_str)
    {
        respond_Ctx->result = ERROR_UNKNOWN_DATA_ACTION;

        return; 
    }

    // 判断动作
    if ((0 == strcmp(t_str, "action=get")) || (0 == strcmp(t_str, "action=set")))
    {
        strcpy(receive_Ctx->action, t_str+strlen("action="));

        respond_Ctx->result = ERROR_NONE_OK;

        return; 
    }
    else
    {
        respond_Ctx->result = ERROR_UNKNOWN_DATA_ACTION;

        return; 
    }
}

static void Chek_Command(receive_Context *receive_Ctx, respond_Context *respond_Ctx)
{
    // 响应错误判断
    if (ERROR_NONE_OK != respond_Ctx->result)
    {
        return;
    }

    command_Info *command_List = NULL;
    int command_Info_List_Len = 0;
    char *t_str = NULL;

    command_List = (command_Info *)gSA_command_Info_List;
    command_Info_List_Len = ggSA_command_Info_List_Len;

    // 分割字符
    t_str = strtok(NULL, "$");
    if (NULL == t_str)
    {
        respond_Ctx->result = ERROR_UNKNOWN_DATA_COMMAND;

        return; 
    }

    // 判断命令
    if (0 == strncmp(t_str, "command=", strlen("command=")))
    {
        receive_Ctx->command_ID = Array_ID_Matcher((t_str+strlen("command=")), strlen(t_str+strlen("command=")), &(command_List[0].name), sizeof(command_List[0]), command_Info_List_Len);

        strcpy(receive_Ctx->command, t_str+strlen("command="));

        if (-1 == receive_Ctx->command_ID)
        {
            respond_Ctx->result = ERROR_UNKNOWN_DATA_COMMAND;

            return; 
        }
    }
    else
    {
        respond_Ctx->result = ERROR_UNKNOWN_DATA_COMMAND;

        return; 
    }

    respond_Ctx->result = ERROR_NONE_OK;

    return; 
}

static void Check_Param(receive_Context *receive_Ctx, respond_Context *respond_Ctx)
{
    // 响应错误判断
    if (ERROR_NONE_OK != respond_Ctx->result) 
    {
        return;
    }

    unsigned int i = 0;
    char *t_str = NULL;

    // 分割字符
    t_str = strtok(NULL, "$");
    if (NULL == t_str)
    {
        receive_Ctx->param_Num = 0;

        respond_Ctx->result = ERROR_NONE_OK;

        return; 
    }

    // 判断参数
    if(NULL != strtok(t_str, "&"))
    {
        do
        {
            if (0 == strncmp(t_str, "param=", strlen("param=")))
            {
                strcpy(receive_Ctx->param[i], t_str+strlen("param="));
            }
            else
            {
                respond_Ctx->result = ERROR_UNKNOWN_DATA_PARAM;

                return; 
            }

            ++i;

            t_str = strtok(NULL, "&");
        }
        while(NULL != t_str);
    }
    else if (0 == strncmp(t_str, "param=", strlen("param=")))
    {
        strcpy(receive_Ctx->param[i], t_str+strlen("param="));

        ++i;
    }
    else
    {
        respond_Ctx->result = ERROR_UNKNOWN_DATA_PARAM;

        return; 
    }

    // 更新参数个数
    receive_Ctx->param_Num += i;

    respond_Ctx->result = ERROR_NONE_OK;

    return; 
}

static void Error_Info_Sender(struct mg_connection *conn, int message_type, const char *data, size_t data_len)
{
    char *respond_Buffer = g_my_Data_Ctx->server_Ctx->respond_Ctx->respond_Buffer;
    char *receive_Buffer = g_my_Data_Ctx->server_Ctx->receive_Ctx->receive_Buffer;

    Log_Info("[Data Respond]:\n%s\n", (char *)data);

    mg_websocket_write(conn, message_type, data, data_len);

    return;
}

static void Error_Info_Splicer(respond_Context *respond_Ctx)
{
    error_Info *error_Info_List = NULL;
    unsigned int error_Info_List_Len = 0;
    int error_ID = 0;
    char error_Detail_Buffer[100] = {0};
    unsigned int error_Detail_offset = 0;

    // 错误信息列表数据
    error_Info_List = (error_Info *)gSA_error_Info_List;
    error_Info_List_Len = ggSA_error_Info_List_Len;

    switch ((int)(respond_Ctx->result))
    {
        case ERROR_UNKNOWN_DATA_TYPE:
        case ERROR_UNKNOWN_DATA_HEAD:
        case ERROR_UNKNOWN_DATA_ACTION:
        case ERROR_UNKNOWN_DATA_COMMAND:
        case ERROR_UNKNOWN_DATA_PARAM:
        case ERROR_DONT_HAVE_SUCH_COMMAND:
        case ERROR_COMMON_ERROR:
        case ERROR_NONE_OK:

            // 错误信息ID匹配
            error_ID = Array_ID_Matcher(&(respond_Ctx->result), sizeof(respond_Ctx->result)-1, &(error_Info_List[0].Code), sizeof(error_Info_List[0]), error_Info_List_Len);

            // 生成错误信息
            sprintf(error_Detail_Buffer, "Error %d %s\n", error_Info_List[error_ID].Code, error_Info_List[error_ID].Detail);

            error_Detail_offset = strlen(error_Detail_Buffer);
            
            // 响应信息=前半部分（错误信息）+后半部分（有效信息）
            //
            // 补充有效信息到响应信息后半部分
            memmove(respond_Ctx->respond_Buffer+error_Detail_offset, respond_Ctx->respond_Buffer, strlen(respond_Ctx->respond_Buffer));

            // 补充错误信息到响应信息前半部分
            strncpy(respond_Ctx->respond_Buffer, error_Detail_Buffer, error_Detail_offset);

            break;
    }
}

static void Data_Type_Analyzer(int data_Type, respond_Context *respond_Ctx)
{
    // 判断数据是否为字符型
    if (MG_WEBSOCKET_OPCODE_TEXT != (data_Type&0xf)) 
    {
        respond_Ctx->result = ERROR_UNKNOWN_DATA_TYPE;

        return;
    }
    else
    {
        respond_Ctx->result = ERROR_NONE_OK;

        return; 
    }
}

static void Data_Cutter(char *data, int datasize)
{
    // 在数据末尾加上\0（由于server数据不带\0，会造成字符串读取错误）
    strcpy(data+datasize, "\0");

    return;
}

static void Data_Parser(char *data, receive_Context *receive_Ctx, respond_Context *respond_Ctx)
{
    // 判断之前的响应结果是否正常
    if (ERROR_NONE_OK != respond_Ctx->result)
    {
        return;
    }

    char t_data[100];
    char *pt_Data = NULL;
    
    // 临时变量初始化
    strcpy(t_data, data);
    pt_Data = (char *)&t_data;

    // 检查接收buffer
    Check_receive_Buffer(data, receive_Ctx);

    // 检查datarec
    Check_datarec(pt_Data, receive_Ctx, respond_Ctx);

    // 检查动作
    Check_Action(receive_Ctx, respond_Ctx);

    // 检查命令
    Chek_Command(receive_Ctx, respond_Ctx);

    // 检查参数/参数个数
    Check_Param(receive_Ctx, respond_Ctx);

    // 输出解析结果
    Log_Info("[Data Recive]:\n%s\n", receive_Ctx->receive_Buffer);
    Log_Debug("[Data Parse]:\n");
    Log_Debug("%s\n", receive_Ctx->datarec);
    Log_Debug("Action Is %s\n", receive_Ctx->action);
    Log_Debug("Command Is %s\n", receive_Ctx->command);
    Log_Debug("Command ID Is %d\n", receive_Ctx->command_ID);
    Log_Debug("Param Num Is %d\n", receive_Ctx->param_Num);
    if (0 == receive_Ctx->param_Num)
    {
        ;
    }
    else
    {
        for (int i = 0; i<(receive_Ctx->param_Num); ++i)
        {
            Log_Debug("param%d Is %s\n", i, receive_Ctx->param[i]);
        }
    }

    return;
}

static void Data_Function_Caller(receive_Context *receive_Ctx, respond_Context *respond_Ctx)
{
    // 判断之前的响应结果是否正常
    if (ERROR_NONE_OK != respond_Ctx->result)
    {
        return;
    }

    Log_Debug("[Data Function]:\n");

    // 调用command对应功能函数
    respond_Ctx->result = gSA_command_Info_List[receive_Ctx->command_ID].Function(respond_Ctx->respond_Buffer);

    return;
}

static void Data_Responder(struct mg_connection *conn, respond_Context *respond_Ctx)
{
    // 错误信息拼接
    Error_Info_Splicer(respond_Ctx);
    
    // 错误信息发送
    Error_Info_Sender(conn, MG_WEBSOCKET_OPCODE_TEXT, respond_Ctx->respond_Buffer, strlen(respond_Ctx->respond_Buffer));

    return;
}

void Data_Handler(struct mg_connection *conn, int data_Type, char *data, int data_Size)
{
    receive_Context *receive_Ctx = g_my_Data_Ctx->server_Ctx->receive_Ctx;
    respond_Context *respond_Ctx = g_my_Data_Ctx->server_Ctx->respond_Ctx;

    // 数据类型分析
    Data_Type_Analyzer(data_Type, respond_Ctx);

    // 数据裁剪
    Data_Cutter(data, data_Size);

    // 数据解析
    Data_Parser(data, receive_Ctx, respond_Ctx);

    // 数据功能调用
    Data_Function_Caller(receive_Ctx, respond_Ctx);

    // 数据响应
    Data_Responder(conn, respond_Ctx);

    // 数据清理
    Data_Cleaner();

    return;
}

