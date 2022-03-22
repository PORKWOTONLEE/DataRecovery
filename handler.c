#include "handler.h"
#include "common.h"

extern my_Data_Context *g_my_Data_Ctx;
extern const command_Info gSA_command_Info_List[];
extern const int ggSA_command_Info_List_Len;

// 响应命令表
const command_Info gSA_command_Info_List[] =
{
    {"info",     Info_Handler},
    {"disk",     Disk_Handler},
    {"scan",     Scan_Handler},
    {"recovery", Recovery_Handler}
};

// 响应命令表长度
const int ggSA_command_Info_List_Len = sizeof(gSA_command_Info_List) / sizeof(gSA_command_Info_List[0]);

// 错误信息表
const error_Info gSA_error_Info_List[] = 
{
    {ERROR_NONE_OK,                    "Everything Is Just Fine"},
    {ERROR_COMMON_ERROR,               "Common Error"},
    {ERROR_UNKNOWN_REQUEST_TYPE,       "Please Check Your WebSocket Client"},
    {ERROR_UNKNOWN_REQUEST_HEAD,       "Unknown Request's Head"},
    {ERROR_REQUEST_ONLY_HAS_HEAD,      "Request Incomplete"},
    {ERROR_UNKNOWN_REQUEST_ACTION,     "Unknown Request's Action"},
    {ERROR_UNKNOWN_REQUEST_COMMAND,    "Unknown Request's Command"},
    {ERROR_UNKNOWN_REQUEST_PARAM,      "Unknown Request's Param"},
    {ERROR_REQUEST_PARAM_LEN_TOO_LONG, "Param Len Is Over Limit"}
};

// 错误信息表长度
const int ggSA_error_Info_List_Len = sizeof(gSA_error_Info_List) / sizeof(gSA_error_Info_List[0]);

static void Check_request_Buffer(char *data)
{
    Set_Request_Buffer(data);
}

static void Check_datarec(char *str)
{
    char *t_str = NULL;

    // 分割字符
    t_str = strtok(str, "$");
    if (NULL == t_str)
    {
        Set_Result(ERROR_REQUEST_ONLY_HAS_HEAD);

        return; 
    }

    // 判断头部
    if (0 == strcmp(str, "datarec"))
    {
        Set_DataRec(str);

        Set_Result(ERROR_NONE_OK);

        return; 
    }
    else
    {
        Set_Result(ERROR_UNKNOWN_REQUEST_HEAD);

        return; 
    }
}

static void Check_Action(void)
{
    // 响应错误判断
    if (ERROR_NONE_OK != Get_Result())
    {
        return;
    }

    char *t_str = NULL;

    // 分割字符
    t_str = strtok(NULL, "$");
    if (NULL == t_str)
    {
        Set_Result(ERROR_UNKNOWN_REQUEST_ACTION);

        return; 
    }

    // 判断动作
    if ((0 == strcmp(t_str, "action=get")) ||
        (0 == strcmp(t_str, "action=set")) ||
        (0 == strcmp(t_str, "action=exit")))
    {
        Set_Action(t_str+strlen("action="));

        Set_Result(ERROR_NONE_OK);

        return; 
    }
    else
    {
        Set_Result(ERROR_UNKNOWN_REQUEST_ACTION);

        return; 
    }
}

static void Chek_Command(void)
{
    // 响应错误判断
    if (ERROR_NONE_OK != Get_Result())
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
        Set_Result(ERROR_UNKNOWN_REQUEST_COMMAND);

        return; 
    }

    // 判断命令
    if (0 == strncmp(t_str, "command=", strlen("command=")))
    {
        Set_Command_ID(Array_ID_Matcher((t_str+strlen("command=")), strlen(t_str+strlen("command=")), &(command_List[0].name), sizeof(command_List[0]), command_Info_List_Len));

        Set_Command(t_str+strlen("command="));

        if (-1 == Get_Command_ID())
        {
            Set_Result(ERROR_UNKNOWN_REQUEST_COMMAND);

            return; 
        }
    }
    else
    {
        Set_Result(ERROR_UNKNOWN_REQUEST_COMMAND);

        return; 
    }

    Set_Result(ERROR_NONE_OK);

    return; 
}

static void Check_Param()
{
    // 响应错误判断
    if (ERROR_NONE_OK != Get_Result()) 
    {
        return;
    }

    unsigned int i = 0;
    char *t_str = NULL;

    // 分割字符
    t_str = strtok(NULL, "$");
    if (NULL == t_str)
    {
        Set_Param_Num(0);

        Set_Result(ERROR_NONE_OK);

        return; 
    }

    // 判断参数
    if(NULL != strtok(t_str, "&"))
    {
        do
        {
            if (0 == strncmp(t_str, "param=", strlen("param=")))
            {
                if (PARAM_LEN > strlen(t_str+strlen("param=")))
                {
                    Set_Param(i, t_str+strlen("param="));
                }
                else
                {
                    Set_Result(ERROR_REQUEST_PARAM_LEN_TOO_LONG);
                }
            }
            else
            {
                Set_Result(ERROR_UNKNOWN_REQUEST_PARAM);

                return; 
            }

            ++i;

            t_str = strtok(NULL, "&");
        }
        while(NULL != t_str || i>PARAM_NUM);
    }
    else if (0 == strncmp(t_str, "param=", strlen("param=")))
    {
        Set_Param(i, t_str+strlen("param="));

        ++i;
    }
    else
    {
        Set_Result(ERROR_UNKNOWN_REQUEST_PARAM);

        return; 
    }

    // 更新参数个数
    Set_Param_Num(i);

    Set_Result(ERROR_NONE_OK);

    return; 
}

static void Error_Info_Sender(struct mg_connection *conn, int message_type, const char *data, size_t data_len)
{
    char *respond_Buffer = g_my_Data_Ctx->server_Ctx->respond_Ctx->respond_Buffer;
    char *request_Buffer = g_my_Data_Ctx->server_Ctx->request_Ctx->request_Buffer;

    Log_Info("[Data Respond]:\n%s\n", (char *)data);

    mg_websocket_write(conn, message_type, data, data_len);

    return;
}

static void Error_Info_Splicer(void)
{
    respond_Context *respond_Ctx = g_my_Data_Ctx->server_Ctx->respond_Ctx;
    error_Info *error_Info_List = NULL;
    unsigned int error_Info_List_Len = 0;
    int error_ID = 0;
    char error_Detail_Buffer[100] = {0};
    unsigned int error_Detail_offset = 0;

    // 错误信息列表数据
    error_Info_List = (error_Info *)gSA_error_Info_List;
    error_Info_List_Len = ggSA_error_Info_List_Len;

    switch ((int)(Get_Result()))
    {
        case ERROR_UNKNOWN_REQUEST_TYPE:
        case ERROR_UNKNOWN_REQUEST_HEAD:
        case ERROR_UNKNOWN_REQUEST_ACTION:
        case ERROR_UNKNOWN_REQUEST_COMMAND:
        case ERROR_UNKNOWN_REQUEST_PARAM:
        case ERROR_COMMON_ERROR:
        case ERROR_NONE_OK:

            // 错误信息ID匹配
            error_ID = Array_ID_Matcher(&(respond_Ctx->result), 
                                        sizeof(respond_Ctx->result)-1, 
                                        &(error_Info_List[0].Code), 
                                        sizeof(error_Info_List[0]), 
                                        error_Info_List_Len);

            // 生成错误信息
            sprintf(error_Detail_Buffer, "Error %d %s\n", error_Info_List[error_ID].Code, error_Info_List[error_ID].Detail);

            error_Detail_offset = strlen(error_Detail_Buffer);
            
            // 响应信息=前半部分（错误信息）+后半部分（有效信息）
            //
            // 补充有效信息到响应信息后半部分
            memmove(respond_Ctx->respond_Buffer+error_Detail_offset, Get_Respond_Buffer(), strlen(Get_Respond_Buffer()));

            // 补充错误信息到响应信息前半部分
            strncpy(Get_Respond_Buffer(), error_Detail_Buffer, error_Detail_offset);

            break;
    }
}

static void Request_Type_Analyzer(int data_Type)
{
    // 判断数据是否为字符型
    if (MG_WEBSOCKET_OPCODE_TEXT != (data_Type&0xf)) 
    {
        Set_Result(ERROR_UNKNOWN_REQUEST_TYPE);

        return;
    }
    else
    {
        Set_Result(ERROR_NONE_OK);

        return; 
    }
}

static void Request_Cutter(char *data, int datasize)
{
    // 在数据末尾加上\0（由于server数据不带\0，会造成字符串读取错误）
    strcpy(data+datasize, "\0");

    return;
}

static void Request_Parser(char *data)
{
    // 判断之前的响应结果是否正常
    if (ERROR_NONE_OK != Get_Result())
    {
        return;
    }

    char t_data[100];
    char *pt_Data = NULL;
    
    // 临时变量初始化
    strcpy(t_data, data);
    pt_Data = (char *)&t_data;

    // 检查接收buffer
    Check_request_Buffer(data);

    // 检查datarec
    Check_datarec(pt_Data);

    // 检查动作
    Check_Action();

    // 检查命令
    Chek_Command();

    // 检查参数/参数个数
    Check_Param();

    // 输出解析结果
    Log_Info("[Request Content]:\n%s\n", Get_Request_Buffer());
    Log_Debug("[Request Parse]:\n");
    Log_Debug("%s\n",                    Get_DataRec());
    Log_Debug("Action Is %s\n",          Get_Action());
    Log_Debug("Command Is %s\n",         Get_Command());
    Log_Debug("Command ID Is %d\n",      Get_Command_ID());
    Log_Debug("Param Num Is %d\n",       Get_Param_Num());
    if (0 == Get_Param_Num())
    {
        ;
    }
    else
    {
        for (int i = 0; i<(Get_Param_Num()); ++i)
        {
            Log_Debug("param%d Is %s\n", i, Get_Char_Param(i));
        }
    }

    return;
}

static void Request_Function_Caller()
{
    // 判断之前的响应结果是否正常
    if (ERROR_NONE_OK != Get_Result())
    {
        return;
    }

    Log_Debug("[Request Function]:\n");

    // 调用command对应功能函数
    Set_Result(gSA_command_Info_List[Get_Command_ID()].Function(Get_Respond_Buffer()));

    return;
}

static void Request_Responder(struct mg_connection *conn)
{
    // 错误信息拼接
    Error_Info_Splicer();
    
    // 错误信息发送
    Error_Info_Sender(conn, MG_WEBSOCKET_OPCODE_TEXT, Get_Respond_Buffer(), strlen(Get_Respond_Buffer()));

    return;
}

void Request_Handler(struct mg_connection *conn, int data_Type, char *data, int data_Size)
{
    // 请求类型分析
    Request_Type_Analyzer(data_Type);

    // 请求信息裁剪
    Request_Cutter(data, data_Size);

    // 请求解析
    Request_Parser(data);

    // 请求函数调用
    Request_Function_Caller();

    // 响应请求
    Request_Responder(conn);

    // server数据清理
    Server_Context_Reset();

    return;
}

