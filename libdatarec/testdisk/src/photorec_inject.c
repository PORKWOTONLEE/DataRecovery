#include "../config.h"
#include "datarec_inject.h"

#define DEEPSEARCH_FILE  1

#ifdef DEEPSEARCH_FILE

int need_to_stop = 0;
#define READ_SIZE 1024*512

int Deep_Scan_File(char* error_Buffer, struct ph_param *params, const struct ph_options *options, alloc_data_t *list_search_space)
{
    pstatus_t ind_stop=PSTATUS_OK;
    const unsigned int blocksize_is_known=params->blocksize;
    params_reset(params, options);

    /* make the first recup_dir */
    params->dir_num=photorec_mkdir(params->recup_dir, params->dir_num);

#ifdef ENABLE_DFXML
    /* Open the XML output file */
    xml_open(params->recup_dir, params->dir_num);
    xml_setup(params->disk, params->partition);
#endif

    for(params->pass=0; params->status!=STATUS_QUIT; params->pass++)
    {
        const unsigned int old_file_nbr=params->file_nbr;

        switch(params->status)
        {
            case STATUS_UNFORMAT:
#ifndef __FRAMAC__
                ind_stop=fat_unformat(params, options, list_search_space);
#endif
                params->blocksize=blocksize_is_known;
                break;
            case STATUS_FIND_OFFSET:
                {
                    uint64_t start_offset=0;
                    if(blocksize_is_known>0)
                    {
                        ind_stop=PSTATUS_OK;
                        if(!td_list_empty(&list_search_space->list))
                            start_offset=(td_list_first_entry(&list_search_space->list, alloc_data_t, list))->start % params->blocksize;
                    }
                    else
                    {
                        ind_stop=photorec_find_blocksize(params, options, list_search_space);
                        params->blocksize=find_blocksize(list_search_space, params->disk->sector_size, &start_offset);
                    }
                    update_blocksize(params->blocksize, list_search_space, start_offset);
                }
                break;
            case STATUS_EXT2_ON_BF:
            case STATUS_EXT2_OFF_BF:
#ifndef __FRAMAC__
                ind_stop=photorec_bf(params, options, list_search_space);
#endif
                break;
            default:
                ind_stop=photorec_aux(params, options, list_search_space);
                break;
        }
        session_save(list_search_space, params, options);
        if(need_to_stop!=0)
            ind_stop=PSTATUS_STOP;
        switch(ind_stop)
        {
            case PSTATUS_ENOSPC:
                { /* no more space */
                    params->status=STATUS_QUIT;
                }
                break;
            case PSTATUS_EACCES:
                params->status=STATUS_QUIT;
                break;
            case PSTATUS_STOP:
                if(session_save(list_search_space, params, options) < 0)
                {
                    /* Failed to save the session! */
                    params->status=STATUS_QUIT;
                }
                else
                {
                    params->status=STATUS_QUIT;
                }
                break;
            case PSTATUS_OK:
                status_inc(params, options);
                if(params->status==STATUS_QUIT)
                    unlink("photorec.ses");
                break;
        }
        {
        }
        update_stats(params->file_stats, list_search_space);
        if(params->pass>0)
        {
            write_stats_log(params->file_stats);
        }
    }
    info_list_search_space(list_search_space, NULL, params->disk->sector_size, options->keep_corrupted_file, options->verbose);
    /* Free memory */
    free_search_space(list_search_space);
    free(params->file_stats);
    params->file_stats=NULL;
    free_header_check();
#ifdef ENABLE_DFXML
    xml_shutdown();
    xml_close();
#endif
    return 0;
}

#endif
