#include <string.h>
#include "../handler.h"
#include "handler_component.h"

extern const command_Info gSA_command_Info_List[];
extern const int ggSA_command_Info_List_Len;

error_Code Help_Handler(char *error_Buffer)
{
    char *p_error_Buffer= error_Buffer;

    memmove(p_error_Buffer, gSA_command_Info_List[0].name, sizeof(gSA_command_Info_List[0].name)); 
    p_error_Buffer += strlen(gSA_command_Info_List[0].name);
    strcat(p_error_Buffer, "\n");

    for (int i=1; i<ggSA_command_Info_List_Len; ++i)
    {
        strcat(p_error_Buffer, gSA_command_Info_List[i].name);
        p_error_Buffer += strlen(gSA_command_Info_List[i].name);
        if (i != (ggSA_command_Info_List_Len - 1))
        {
            strcat(p_error_Buffer, "\n");
        }
    }

    strcat(p_error_Buffer, "\0");

    return ERROR_NONE_OK;
}
