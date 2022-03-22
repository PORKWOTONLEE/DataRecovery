#include "../common.h"
#include "handler_component.h"

extern my_Data_Context *g_my_Data_Ctx;

error_Code Info_Handler(char *error_Buffer)
{
    photorec_File_Context *photorec_File_Ctx = g_my_Data_Ctx->testdisk_Ctx->photorec_File_Ctx;
    char *info_Serialized= NULL;
    cJSON *info_Json = NULL;
    cJSON *file_Extension_Surpport_Json = NULL;
    char percent_Buffer[10];

    info_Json = cJSON_CreateObject();

    cJSON_AddStringToObject(info_Json, "status",  Convert_Status_To_Char(Get_Status()));
    cJSON_AddStringToObject(info_Json, "quick_Scan_File_List",  Convert_List_Available_To_Char(Get_Quick_Scan_File_List_Available()));
    cJSON_AddStringToObject(info_Json, "deep_Scan_File_List",  Convert_List_Available_To_Char(Get_Deep_Scan_File_List_Available()));

    sprintf(percent_Buffer, "%3.2f%%", Get_Percent());
    cJSON_AddStringToObject(info_Json, "percent", percent_Buffer);

    file_Extension_Surpport_Json = cJSON_CreateObject();

    for (int i=0; NULL!=photorec_File_Ctx->file_Extension_Surpport_List[i].file_hint; ++i)
    {
        cJSON_AddStringToObject(file_Extension_Surpport_Json, 
                                (char *)(photorec_File_Ctx->file_Extension_Surpport_List[i].file_hint->extension), 
                                (photorec_File_Ctx->file_Extension_Surpport_List[i].enable==1?"Yes":"No"));
    }

    cJSON_AddItemToObject(info_Json, "file_Extension_Support", file_Extension_Surpport_Json);

    info_Serialized = cJSON_Print(info_Json); 
    strcpy(error_Buffer, info_Serialized);

    free(info_Serialized);
    cJSON_Delete(info_Json);

    return ERROR_NONE_OK;
}
