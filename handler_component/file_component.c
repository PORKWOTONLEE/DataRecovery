#include <stdlib.h>
#include <string.h>
#include "../common.h"
#include "../log/log.h"
#include "../libtestdisk/include/dir_common.h"
#include "../libtestdisk/ntfsprogs/include/ntfs/volume.h"
#include "../libtestdisk/include/datarec_inject.h"

#define HAVE_LIBNTFS 1
#include "../libtestdisk/include/ntfs_inc.h"

#include "handler_component.h"

extern my_Data_Context *g_my_Data_Ctx;

static int Update_Recovery_File_Path(char *error_Buffer)
{
    data_Recovery_Context *data_Rec_Ctx = g_my_Data_Ctx->data_Rec_Ctx;
    testdisk_Context *testdisk_Ctx = g_my_Data_Ctx->testdisk_Ctx;
    
    if (0 != Get_Param_Num())
    {
        strcpy(data_Rec_Ctx->file_Ctx->destination, Get_Param(0));
    }
    else
    {
        strcpy(data_Rec_Ctx->file_Ctx->destination, DEFAULT_RECOVERY_FILE_PATH);
    }

    testdisk_Ctx->undelete_File_Ctx->data.local_dir = (char *)&(data_Rec_Ctx->file_Ctx->destination);

    Log_Debug("Recovery_File_Path Set To %s\n", data_Rec_Ctx->file_Ctx->destination);

    return EXIT_SUCCESS;
}

static int Recovery_File(char *error_Buffer)
{
    data_Recovery_Context *data_Rec_Ctx = g_my_Data_Ctx->data_Rec_Ctx;
    testdisk_Context *testdisk_Ctx = g_my_Data_Ctx->testdisk_Ctx;
    const struct ntfs_dir_struct *ls=(const struct ntfs_dir_struct *)testdisk_Ctx->undelete_File_Ctx->data.private_dir_data;
    int result = 0;

    result = undelete_file(error_Buffer, ls->vol, data_Rec_Ctx->file_Ctx->info[atoi(Get_Param(0))].inode);

    if (result < -1)
    {
        sprintf(error_Buffer, "Undelete_File Fail");

        return EXIT_FAILURE;
    }
    else if (result < 0)
    {
        sprintf(error_Buffer, "Undelete_File Success(With Broken Data)");

        return EXIT_SUCCESS;
    }
    else
    {
        sprintf(error_Buffer, "Undelete_File Success");

        return EXIT_SUCCESS;
    }
}

error_Code File_Handler(char *error_Buffer)
{
    if (0 == strcmp("set", Get_Action()))
    {
        if (EXIT_SUCCESS == Update_Recovery_File_Path(error_Buffer))
        {
            return ERROR_NONE_OK;
        }
    }
    else if (0 == strcmp("get", Get_Action()))
    {
        if (EXIT_SUCCESS == Recovery_File(error_Buffer))
        {
            return ERROR_NONE_OK;
        }
    }

    return ERROR_COMMON_ERROR;

}

