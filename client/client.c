#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <windows.h>

#include "civetweb.h"

struct tclient_data 
{

	time_t started;
	time_t closed;
	struct tmsg_list_elem *msgs;
};

static int websocket_client_data_handler(struct mg_connection *conn,
                                         int    flags,
                                         char   *data,
                                         size_t data_len,
                                         void   *user_data)
{
	struct tclient_data *pclient_data = (struct tclient_data *)user_data;
	time_t now = time(NULL);

	int is_text = ((flags & 0xf) == MG_WEBSOCKET_OPCODE_TEXT);
	int is_bin = ((flags & 0xf) == MG_WEBSOCKET_OPCODE_BINARY);
	int is_close = ((flags & 0xf) == MG_WEBSOCKET_OPCODE_CONNECTION_CLOSE);

	if (is_text) {
        strcpy(data+data_len, "\0");
        printf("[Receive Data]\n%s\n\n", data);
	}

	if (is_bin) {
	}

	if (is_close) {
		return 0;
	}

	return 1;
}

static void websocket_client_close_handler(const struct mg_connection *conn, void *user_data)
{
	struct tclient_data *pclient_data = (struct tclient_data *)user_data;

	pclient_data->closed = time(NULL);
	printf("%10.0f - Client: Close handler\n", difftime(pclient_data->closed, pclient_data->started));
}

int main(void)
{
	const char *host = "localhost";
	const char *path = "/";
	const int port = 8081;
	const int secure = 0;

	mg_init_library(0);

	char err_buf[100] = {0};
	struct mg_connection *client_conn;
	struct tclient_data *pclient_data;
	int i;

	pclient_data = (struct tclient_data *)malloc(sizeof(struct tclient_data));

	pclient_data->started = time(NULL);
	pclient_data->closed = 0;
	pclient_data->msgs = NULL;

	printf("%10.0f - Connecting to %s:%i\n", 0.0, host, port);

	client_conn = mg_connect_websocket_client(host,
	                                          port,
	                                          secure,
	                                          err_buf,
	                                          sizeof(err_buf),
	                                          path,
	                                          NULL,
	                                          websocket_client_data_handler,
	                                          websocket_client_close_handler,
	                                          pclient_data);

	if (client_conn == NULL) {
		printf("mg_connect_websocket_client error: %s\n", err_buf);
        if ('q' == getchar())
        {
            return 0;
        }
	}

	printf("%10.0f - Connected\n\n", difftime(time(NULL), pclient_data->started));

    char *Send_buffer = NULL;
    char value;
    unsigned int physical_Disk_No = 0;
    unsigned int logical_Disk_No= 0;
    unsigned int page_No = 0;
    unsigned int file_No = 0;
    char file_Extension[10] = {};
    int file_Extension_Enable = 0;
    char file_Recovery_Path[1024] = {};
    Send_buffer = (char *)calloc(50,50);
    while (1)
    {
        Sleep(100);
        printf("======================================================\n");
        printf("????????????                                             =\n");
        printf("======================================================\n");
        printf("1.datarec???????????????????????????????????????????????????????????????  =\n");
        printf("======================================================\n");
        printf("2.??????????????????                                       =\n");
        printf("3.??????????????????                                       =\n");
        printf("======================================================\n");
        printf("4.??????????????????                                       =\n");
        printf("5.??????????????????                                       =\n");
        printf("6.????????????                                           =\n");
        printf("======================================================\n");
        printf("7.???????????????????????????????????????                         =\n");
        printf("8.???????????????????????????????????????                         =\n");
        printf("9.???????????????????????????????????????                         =\n");
        printf("10.???????????????????????????????????????                        =\n");
        printf("======================================================\n");
        printf("11.??????????????????????????????                              =\n");
        printf("======================================================\n");
        printf("12.?????????????????????no???????????????????????????                =\n");
        printf("13.????????????????????????????????????                          =\n");
        printf("======================================================\n");
        printf("14.??????datarec??????                                   =\n");
        printf("???????????????????????????:");
        scanf("%s", &value);    
        switch (atoi(&value))
        {
            case 1:
                sprintf(Send_buffer, "datarec$action=get$command=info");
                break;
            case 2:
                sprintf(Send_buffer, "datarec$action=set$command=disk");
                break;
            case 3:
                sprintf(Send_buffer, "datarec$action=get$command=disk");
                break;
            case 4:
                printf("???????????????????????????(no):");
                scanf("%d", &physical_Disk_No);    
                printf("??????????????????????????????no???:");
                scanf("%d", &logical_Disk_No);    
                sprintf(Send_buffer, "datarec$action=set$command=scan$param=0&param=%d&param=%d", physical_Disk_No, logical_Disk_No);
                break;
            case 5:
                printf("???????????????????????????(no):");
                scanf("%d", &physical_Disk_No);    
                printf("??????????????????????????????no???:");
                scanf("%d", &logical_Disk_No);    
                sprintf(Send_buffer, "datarec$action=set$command=scan$param=1&param=%d&param=%d", physical_Disk_No, logical_Disk_No);
                break;
            case 6:
                sprintf(Send_buffer, "datarec$action=set$command=scan$param=2");
                break;
            case 7:
                sprintf(Send_buffer, "datarec$action=get$command=scan$param=0");
                break;
            case 8:
                sprintf(Send_buffer, "datarec$action=get$command=scan$param=0&param=0");
                break;
            case 9:
                sprintf(Send_buffer, "datarec$action=get$command=scan$param=0&param=1");
                break;
            case 10:
                printf("???????????????:");
                scanf("%d", &page_No);    
                sprintf(Send_buffer, "datarec$action=get$command=scan$param=0&param=2&param=%d", page_No);
                break;
            case 11:
                printf("????????????????????????:");
                scanf("%s", (char *)&file_Extension);    
                printf("????????????????????????0?????????/1????????????:");
                scanf("%d", &file_Extension_Enable);    
                sprintf(Send_buffer, "datarec$action=set$command=scan$param=3&param=%s&param=%d", file_Extension, file_Extension_Enable);
                break;
            case 12:
                printf("?????????????????????(no):");
                scanf("%d", &file_No);    
                sprintf(Send_buffer, "datarec$action=get$command=recovery$param=%d", page_No); 
                break;
            case 13:
                printf("???????????????????????????:");
                scanf("%s", (char *)&file_Recovery_Path);    
                sprintf(Send_buffer, "datarec$action=set$command=recovery$param=%s", file_Recovery_Path);
                break;
            case 14:
                sprintf(Send_buffer, "datarec$action=exit");
                break;
            default:
                sprintf(Send_buffer, "datarec$action=get$command=info");
                break;
        }
        printf("[Request Send]\n%s\n", Send_buffer);
        mg_websocket_client_write(client_conn,
                MG_WEBSOCKET_OPCODE_TEXT,
                Send_buffer,
                strlen(Send_buffer));
    }

    free(Send_buffer);

    return 0;
}
