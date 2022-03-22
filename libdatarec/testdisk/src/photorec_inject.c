#include "../config.h"
#include "datarec_inject.h"
#include "log.h"
#include "pnext.h"
#include "psearch.h"
#include "photorec_check_header.h"

#define DEEPSEARCH_FILE  1

#ifdef DEEPSEARCH_FILE

extern struct my_Data_Context *g_my_Data_Ctx;
extern int need_to_stop;
#define READ_SIZE 1024*512

pstatus_t photorec_aux(photorec_File_Context *photorec_File_Ctx)
{
    const struct ph_options *options = &photorec_File_Ctx->options;
    alloc_data_t *list_search_space =  &photorec_File_Ctx->data;
    struct ph_param *params =          &photorec_File_Ctx->param;
    uint64_t offset;
    unsigned char *buffer_start;
    unsigned char *buffer_olddata;
    unsigned char *buffer;
    time_t start_time;
    time_t previous_time;
    time_t next_checkpoint;
    pstatus_t ind_stop=PSTATUS_OK;
    unsigned int buffer_size;
    const unsigned int blocksize=params->blocksize; 
    const unsigned int read_size=(blocksize>65536?blocksize:65536);
    uint64_t offset_before_back=0;
    unsigned int back=0;
    pfstatus_t file_recovered_old=PFSTATUS_BAD;
    alloc_data_t *current_search_space;
    file_recovery_t file_recovery;
    memset(&file_recovery, 0, sizeof(file_recovery));
    reset_file_recovery(&file_recovery);
    file_recovery.blocksize=blocksize;
    buffer_size=blocksize + READ_SIZE;
    buffer_start=(unsigned char *)MALLOC(buffer_size);
    buffer_olddata=buffer_start;
    buffer=buffer_olddata+blocksize;
    start_time=time(NULL);
    previous_time=start_time;
    next_checkpoint=start_time+5*60;
    memset(buffer_olddata,0,blocksize);
    current_search_space=td_list_first_entry(&list_search_space->list, alloc_data_t, list);
    offset=set_search_start(params, &current_search_space, list_search_space);
    if(options->verbose > 0)
        info_list_search_space(list_search_space, current_search_space, params->disk->sector_size, 0, options->verbose);
    if(options->verbose > 1)
    {
        log_verbose("Reading sector %10llu/%llu\n",
                (unsigned long long)((offset-params->partition->part_offset)/params->disk->sector_size),
                (unsigned long long)((params->partition->part_size-1)/params->disk->sector_size));
    }
    params->disk->pread(params->disk, buffer, READ_SIZE, offset);
    header_ignored(NULL);
    while(current_search_space!=list_search_space)
    {
        // 设置百分比
        Set_Percent(100*(double)(offset-params->partition->part_offset)/(unsigned long long)(params->partition->part_size-1));

        pfstatus_t file_recovered=PFSTATUS_BAD;
        uint64_t old_offset=offset;
        data_check_t data_check_status=DC_SCAN;

        ind_stop=photorec_check_header(&file_recovery, params, options, list_search_space, buffer, &file_recovered, offset);
        if(file_recovery.file_stat!=NULL)
        {
            /* try to skip ext2/ext3 indirect block */
            if((params->status==STATUS_EXT2_ON || params->status==STATUS_EXT2_ON_SAVE_EVERYTHING) &&
                    file_recovery.file_size >= 12*blocksize &&
                    ind_block(buffer,blocksize)!=0)
            {
                file_block_append(&file_recovery, list_search_space, &current_search_space, &offset, blocksize, 0);
                data_check_status=DC_CONTINUE;
                memcpy(buffer, buffer_olddata, blocksize);
            }
            else
            {
                if(file_recovery.handle!=NULL)
                {
                    if(fwrite(buffer,blocksize,1,file_recovery.handle)<1)
                    { 
                        if(errno==EFBIG)
                        {
                            /* File is too big for the destination filesystem */
                            data_check_status=DC_STOP;
                        }
                        else
                        {
                            /* Warn the user */
                            ind_stop=PSTATUS_ENOSPC;
                            params->offset=file_recovery.location.start;
                        }
                    }
                }
                if(ind_stop==PSTATUS_OK)
                {
                    file_block_append(&file_recovery, list_search_space, &current_search_space, &offset, blocksize, 1);
                    if(file_recovery.data_check!=NULL)
                        data_check_status=file_recovery.data_check(buffer_olddata,2*blocksize,&file_recovery);
                    else
                        data_check_status=DC_CONTINUE;
                    file_recovery.file_size+=blocksize;
                    if(data_check_status==DC_STOP)
                    {
                        if(options->verbose > 1)
                            log_trace("EOF found\n");
                    }
                }
            }
            if(data_check_status!=DC_STOP && data_check_status!=DC_ERROR && file_recovery.file_stat->file_hint->max_filesize>0 && file_recovery.file_size>=file_recovery.file_stat->file_hint->max_filesize)
            {
                data_check_status=DC_STOP;
            }
            if(data_check_status!=DC_STOP && data_check_status!=DC_ERROR && file_recovery.file_size + blocksize >= PHOTOREC_MAX_SIZE_32 && is_fat(params->partition))
            {
                data_check_status=DC_STOP;
            }
            if(data_check_status==DC_STOP || data_check_status==DC_ERROR)
            {
                if(data_check_status==DC_ERROR)
                    file_recovery.file_size=0;
                file_recovered=file_finish2(&file_recovery, params, options->paranoid, list_search_space);
                if(options->lowmem > 0)
                    forget(list_search_space,current_search_space);
            }
        }
        if(ind_stop!=PSTATUS_OK || SCAN_STOP == Get_Control())
        {
            file_recovery_aborted(&file_recovery, params, list_search_space);
            free(buffer_start);
            return ind_stop;
        }
        if(file_recovered==PFSTATUS_BAD)
        {
            if(data_check_status==DC_SCAN)
            {
                if(file_recovered_old==PFSTATUS_OK)
                {
                    offset_before_back=offset;
                    if(back < 5 &&
                            get_prev_file_header(list_search_space, &current_search_space, &offset)==0)
                    {
                        back++;
                    }
                    else
                    {
                        back=0;
                        get_prev_location_smart(list_search_space, &current_search_space, &offset, file_recovery.location.start);
                    }
                }
                else
                {
                    get_next_sector(list_search_space, &current_search_space,&offset,blocksize);
                    if(offset > offset_before_back)
                        back=0;
                }
            }
        }
        else if(file_recovered==PFSTATUS_OK_TRUNCATED)
        {
            /* try to recover the previous file, otherwise stay at the current location */
            offset_before_back=offset;
            if(back < 5 &&
                    get_prev_file_header(list_search_space, &current_search_space, &offset)==0)
            {
                back++;
            }
            else
            {
                back=0;
                get_prev_location_smart(list_search_space, &current_search_space, &offset, file_recovery.location.start);
            }
        }
        if(current_search_space==list_search_space)
        {
            file_recovered=file_finish2(&file_recovery, params, options->paranoid, list_search_space);
            if(file_recovered!=PFSTATUS_BAD)
                get_prev_location_smart(list_search_space, &current_search_space, &offset, file_recovery.location.start);
            if(options->lowmem > 0)
                forget(list_search_space,current_search_space);
        }
        buffer_olddata+=blocksize;
        buffer+=blocksize;
        if(file_recovered!=PFSTATUS_BAD ||
                old_offset+blocksize!=offset ||
                buffer+read_size>buffer_start+buffer_size)
        {
            if(file_recovered!=PFSTATUS_BAD)
                memset(buffer_start,0,blocksize);
            else
                memcpy(buffer_start,buffer_olddata,blocksize);
            buffer_olddata=buffer_start;
            buffer=buffer_olddata + blocksize;
            if(options->verbose > 1)
            {
                log_verbose("Reading sector %10llu/%llu\n",
                        (unsigned long long)((offset-params->partition->part_offset)/params->disk->sector_size),
                        (unsigned long long)((params->partition->part_size-1)/params->disk->sector_size));
            }
            if(params->disk->pread(params->disk, buffer, READ_SIZE, offset) != READ_SIZE)
            {
#ifdef HAVE_NCURSES
                wmove(stdscr,11,0);
                wclrtoeol(stdscr);
                wprintw(stdscr,"Error reading sector %10lu\n",
                        (unsigned long)((offset-params->partition->part_offset)/params->disk->sector_size));
#endif
            }
            if(ind_stop==PSTATUS_OK)
            {
                const time_t current_time=time(NULL);
                if(current_time>previous_time)
                {
                    previous_time=current_time;
#ifdef HAVE_NCURSES
                    ind_stop=photorec_progressbar(stdscr, params->pass, params, offset, current_time);
#endif
                    params->offset=offset;
                    if(need_to_stop!=0 || ind_stop!=PSTATUS_OK)
                    {
                        log_info("PhotoRec has been stopped\n");
                        file_recovery_aborted(&file_recovery, params, list_search_space);
                        free(buffer_start);
                        return PSTATUS_STOP;
                    }
                    if(current_time >= next_checkpoint)
                        next_checkpoint=regular_session_save(list_search_space, params, options, current_time);
                }
            }
        }
        file_recovered_old=file_recovered;
    } /* end while(current_search_space!=list_search_space) */
    free(buffer_start);
#ifdef HAVE_NCURSES
    photorec_info(stdscr, params->file_stats);
#endif
    return ind_stop;
}

int Deep_Scan_File(testdisk_Context *testdisk_Ctx)
{
    struct ph_param   *params = &testdisk_Ctx->photorec_File_Ctx->param;
    struct ph_options *options = &testdisk_Ctx->photorec_File_Ctx->options;
    alloc_data_t *list_search_space = &testdisk_Ctx->photorec_File_Ctx->data;
    const unsigned int blocksize_is_known = params->blocksize;
    pstatus_t result = PSTATUS_OK;

    // 参数重置
    params_reset(params, options);

    // 建立首个文件夹
    params->dir_num = photorec_mkdir(params->recup_dir, params->dir_num);

#ifdef ENABLE_DFXML
    // 输出xml信息
    xml_open(params->recup_dir, params->dir_num);
    xml_setup(params->disk, params->partition);
#endif

    for (params->pass=0; params->status!=STATUS_QUIT; ++params->pass)
    {
        const unsigned int old_file_nbr = params->file_nbr;

        switch(params->status)
        {
            case STATUS_UNFORMAT:
                Log_Debug("Unformat Fat FileSystem");
                result = PSTATUS_STOP;
                params->blocksize = blocksize_is_known;
                break;
            case STATUS_FIND_OFFSET:
                // 更新磁盘偏移量
                {
                    uint64_t start_offset = 0;
                    if (blocksize_is_known > 0)
                    {
                        result = PSTATUS_OK;
                        if(!td_list_empty(&list_search_space->list))
                            start_offset = (td_list_first_entry(&list_search_space->list, alloc_data_t, list))->start % params->blocksize;
                    }
                    else
                    {
                        result = photorec_find_blocksize(params, options, list_search_space);
                        params->blocksize = find_blocksize(list_search_space, params->disk->sector_size, &start_offset);
                    }
                    update_blocksize(params->blocksize, list_search_space, start_offset);
                }
                break;
            case STATUS_EXT2_ON_BF:
            case STATUS_EXT2_OFF_BF:
                Log_Debug("Unsurpport FileSystem EXT2");
                result = PSTATUS_STOP;
                break;
            default:
                result = photorec_aux(testdisk_Ctx->photorec_File_Ctx);
                break;
        }

        // 保存搜索会话
        session_save(list_search_space, params, options);

        // 判断搜索停止/继续
        if (SCAN_STOP == Get_Control())
        {
            result = PSTATUS_STOP;
        }

        switch (result)
        {
            case PSTATUS_ENOSPC:
                Log_Debug("Destination Space Is Full");
                params->status = STATUS_QUIT;
                break;
            case PSTATUS_EACCES:
                Log_Debug("Do Not Have Access Right To Destination Path");
                params->status = STATUS_QUIT;
                break;
            case PSTATUS_STOP:
                if (session_save(list_search_space, params, options) < 0)
                {
                    Log_Debug("Fail To Save Session");
                    params->status = STATUS_QUIT;
                }
                else
                {
                    params->status = STATUS_QUIT;
                }
                break;
            case PSTATUS_OK:
                status_inc(params, options);
                if (params->status == STATUS_QUIT)
                {
                    unlink("photorec.ses");
                }
                break;
        }
        update_stats(params->file_stats, list_search_space);
    }

    // 回收空间
    info_list_search_space(list_search_space, NULL, params->disk->sector_size, options->keep_corrupted_file, options->verbose);
    free_search_space(list_search_space);
    free(params->file_stats);
    params->file_stats = NULL;
    free_header_check();
#ifdef ENABLE_DFXML
    xml_shutdown();
    xml_close();
#endif

    return 0;
}

#endif
