#include <stdlib.h>
#include <string.h>
#include "../common.h"
#include "../log/log.h"
#include "../libdatarec/include/dir_common.h"
#include "../libdatarec/ntfsprogs/include/ntfs/volume.h"
#include "../libdatarec/include/datarec_inject.h"

#define HAVE_LIBNTFS 1
#include "../libdatarec/include/ntfs_inc.h"

#include "handler_component.h"

extern my_Data_Context *g_my_Data_Ctx;
extern int undelete_file(ntfs_volume *vol, uint64_t inode);

static error_Code Update_Quick_Scan_File_Path(void)
{
    data_Recovery_Context *data_Rec_Ctx = g_my_Data_Ctx->data_Rec_Ctx;
    testdisk_Context *testdisk_Ctx = g_my_Data_Ctx->testdisk_Ctx;
    
    if (0 != Get_Param_Num())
    {
        strcpy(data_Rec_Ctx->file_Ctx->destination, Get_Char_Param(0));
    }
    else
    {
        strcpy(data_Rec_Ctx->file_Ctx->destination, DEFAULT_QUICKSEARCH_FILE_PATH);
    }

    testdisk_Ctx->testdisk_File_Ctx->data.local_dir = (char *)&(data_Rec_Ctx->file_Ctx->destination);

    Set_Respond_Buffer("Recovery_File_Path Set To %s\n", data_Rec_Ctx->file_Ctx->destination);

    Log_Debug("Recovery_File_Path Set To %s\n", data_Rec_Ctx->file_Ctx->destination);

    return ERROR_NONE_OK;
}

static error_Code Quick_Scan_Recovery_File(void)
{
    data_Recovery_Context *data_Rec_Ctx = g_my_Data_Ctx->data_Rec_Ctx;
    testdisk_Context *testdisk_Ctx = g_my_Data_Ctx->testdisk_Ctx;
    struct ntfs_dir_struct *ls = NULL;
    int result = 0;

    ls = (struct ntfs_dir_struct *)(g_my_Data_Ctx->testdisk_Ctx->testdisk_File_Ctx->data).private_dir_data;

    result = undelete_file(ls->vol, data_Rec_Ctx->file_Ctx->info[Get_Int_Param(0)].inode);

    if (result < -1)
    {
        Set_Respond_Buffer("Undelete_File Fail");

        return ERROR_COMMON_ERROR;
    }
    else if (result < 0)
    {
        Set_Respond_Buffer("Undelete_File Success(With Broken Data)");

        return ERROR_COMMON_ERROR;
    }
    else
    {
        Set_Respond_Buffer("Undelete_File Success");

        return ERROR_NONE_OK;
    }
}

error_Code Recovery_Handler(char *error_Buffer)
{
    if (0 == strcmp("set", Get_Action()))
    {
        return Update_Quick_Scan_File_Path();
    }
    else if (0 == strcmp("get", Get_Action()))
    {
        return Quick_Scan_Recovery_File();
    }

    return ERROR_COMMON_ERROR;
}

