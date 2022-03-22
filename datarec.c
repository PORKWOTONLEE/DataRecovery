/* Copyright (C) 
 * 2022 - PORKWOTONLEE
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "common.h"
#include "./server/server.h"
#include "./log/log.h"

extern my_Data_Context *g_my_Data_Ctx;

int main(void)
{
    struct mg_context *ctx = NULL;
    char *log_Name= "datarec.log";
    int log_Mode = LOG_CREATE;
    int log_Errsv = 0;

    if (true != IsProcessRunAsAdmin())
    {
        Run_As_Admin(); 

        return 0;
    }

    // 初始化Log
    Log_Open(log_Name, log_Mode, &log_Errsv); 
    Log_Info("DataRec Build By PORKWOTONLEE\n");
    Log_Info("cjson lib : %s, civetweb lib : %s\n\n", cJSON_Version(), mg_version());

    // 初始化datarec环境
    My_Data_Context_Init();

    // 初始化server环境
    Civetweb_Init(&ctx);
    Civetweb_Set_Handler(ctx);

    while (0 != strcmp(Get_Action(), "exit"));

    // 清理server环境
    Civetweb_Stop(ctx);

    // 清理datarec环境
    My_Data_Context_Free();

    return EXIT_SUCCESS;
}
