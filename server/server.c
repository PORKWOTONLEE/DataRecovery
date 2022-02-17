#include <stdlib.h>
#include <string.h>
#include "./cjson/include/cJSON.h"
#include "./civetweb/include/civetweb.h"
#include "../log/log.h"
#include "../common.h"
#include "../handler.h"
#include "server.h"

#define DOCUMENT_ROOT "./Docs"
#define PORT "8081"
#define WS_URL "/WebsocketURL"

// 全局Websocket环境结构体
typedef struct 
{
	uint32_t connectionNumber;
	uint32_t demo_var;
}clientContext;

// 初始化服务器选项
static const char *gA_server_Options[] =
{
    "document_root",
    DOCUMENT_ROOT,
    "listening_ports",
    PORT,
    "num_threads",
    "10",
    NULL,
    NULL
};

int WS_Connect_Handler(const struct mg_connection *conn, void *user_data)
{
    // 无用数据
    (void)user_data; 

    // 初始化Websocket客户端环境
    clientContext *ws_Cli_Ctx = (clientContext *)calloc(1, sizeof(clientContext));
    if (NULL == ws_Cli_Ctx) 
    {
        return EXIT_FAILURE;
    }
    
    // 客户端总连接数同步 
    static uint32_t connectionCounter = 0;
    ws_Cli_Ctx->connectionNumber = __sync_add_and_fetch(&connectionCounter, 1);
    // 关联客户端和连接
    mg_set_user_connection_data(conn, ws_Cli_Ctx); 

    Log_Info("Client %u Connected \n", ws_Cli_Ctx->connectionNumber);

    return EXIT_SUCCESS;
}

void WS_Ready_Handler(struct mg_connection *conn, void *user_data)
{
    // 无用数据
    (void)user_data; 

    // 初始化Websocket客户端环境
	clientContext *ws_Cli_Ctx = (clientContext *)mg_get_user_connection_data(conn);
    if (NULL == ws_Cli_Ctx) 
    {
        return;
    }

	Log_Info("Client %u Ready To Receive Data\n", ws_Cli_Ctx->connectionNumber);

    return;
}

int WS_Data_Handler(struct mg_connection *conn,
                int opcode,
                char *data,
                size_t datasize,
                void *user_data)
{
    // 无用数据
    (void)user_data; 

    // 初始化Websocket客户端环境
	clientContext *ws_Cli_Ctx = (clientContext *)mg_get_user_connection_data(conn);

	const char *message_type = "";
	switch (opcode & 0xf) 
    {
        case MG_WEBSOCKET_OPCODE_TEXT:
            message_type = "text";
            break;
        case MG_WEBSOCKET_OPCODE_BINARY:
            message_type = "binary";
            break;
        case MG_WEBSOCKET_OPCODE_PING:
            message_type = "ping";
            break;
        case MG_WEBSOCKET_OPCODE_PONG:
            message_type = "pong";
            break;
	}

	Log_Info("\nWebsocket Received %lu Bytes of %s Data From Client %u\n",
	       (unsigned long)datasize,
	       message_type,
	       ws_Cli_Ctx->connectionNumber);

    // Websocket数据处理
    Data_Handler(conn, opcode, data, datasize);

	return 1;
}

void WS_Close_Handler(const struct mg_connection *conn, void *user_data)
{
    // 无用数据
    (void)user_data; 

    // 初始化Websocket客户端环境
	clientContext *ws_Cli_Ctx = (clientContext *)mg_get_user_connection_data(conn);

	Log_Info("Client %u Closing Connection\n", ws_Cli_Ctx->connectionNumber);

	free(ws_Cli_Ctx);

    return;
}

int Civetweb_Init(struct mg_context **p_ctx)
{
    // 功能检查
    int error_Code = EXIT_SUCCESS;
    if (!mg_check_feature(1)) // 文件支持
    {
        error_Code = EXIT_FAILURE;
    }
    else if (!mg_check_feature(4)) // CGI支持
    {
        error_Code = EXIT_FAILURE;
    }
    else if (!mg_check_feature(8)) // IPv6支持
    {
        error_Code = EXIT_FAILURE;
    }
    else if (!mg_check_feature(16)) // WebSocket支持
    {
        error_Code = EXIT_FAILURE;
    }
    else
    {
        error_Code = EXIT_SUCCESS;
    }
    if (EXIT_FAILURE == error_Code)
    {
        Log_Info("CivetWeb_Init : Fail\n");

        return EXIT_FAILURE;
    }

    // 初始化库
    mg_init_library(0);

    // 启动服务器，获取服务器环境配置
    *p_ctx = mg_start(NULL, NULL, gA_server_Options);
    if (NULL == *p_ctx)
    {
        mg_exit_library();
        Log_Info("CivetWeb_Init : Fail\n");

        return EXIT_FAILURE;
    }

    Log_Info("CivetWeb_Init : Success\n");

    return EXIT_SUCCESS;
}

int Civetweb_Set_Handler(struct mg_context *ctx)
{
    // 无用数据
    void *user_Data = NULL;

    // 添加Handler
    mg_set_websocket_handler(ctx,
            WS_URL, 
            WS_Connect_Handler, 
            WS_Ready_Handler, 
            WS_Data_Handler, 
            WS_Close_Handler, 
            user_Data);

    Log_Info("CivetWeb_SetHandler: Success\n");

    return EXIT_SUCCESS;
}

int Civetweb_Stop(struct mg_context *ctx)
{
    // 停止服务器
    mg_stop(ctx);

    // 退出库
    mg_exit_library();

    Log_Info("CivetWeb_Stop: Success\n");
    Log_Close();

    return EXIT_SUCCESS;
}
