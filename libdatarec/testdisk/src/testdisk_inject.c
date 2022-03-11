#include "datarec_inject.h"

struct filename {
    struct td_list_head list;		/* Previous/Next links */
    ntfschar	*uname;		/* Filename in unicode */
    int		 uname_len;	/* and its length */
    uint64_t	 size_alloc;	/* Allocated size (multiple of cluster size) */
    uint64_t	 size_data;	/* Actual size of data */
    FILE_ATTR_FLAGS	 flags;
    time_t		 date_c;	/* Time created */
    time_t		 date_a;	/*	altered */
    time_t		 date_m;	/*	mft record changed */
    time_t		 date_r;	/*	read */
    char		*name;		/* Filename in current locale */
    FILE_NAME_TYPE_FLAGS name_space;
    uint64_t	 parent_mref;
    char		*parent_name;
};

struct data {
    struct td_list_head list;		/* Previous/Next links */
    char		*name;		/* Stream name in current locale */
    ntfschar	*uname;		/* Unicode stream name */
    int		 uname_len;	/* and its length */
    int		 resident;	/* Stream is resident */
    int		 compressed;	/* Stream is compressed */
    int		 encrypted;	/* Stream is encrypted */
    uint64_t	 size_alloc;	/* Allocated size (multiple of cluster size) */
    uint64_t	 size_data;	/* Actual size of data */
    uint64_t	 size_init;	/* Initialised size, may be less than data size */
    uint64_t	 size_vcn;	/* Highest VCN in the data runs */
    runlist_element *runlist;	/* Decoded data runs */
    unsigned int	 percent;	/* Amount potentially recoverable */
    void		*data;		/* If resident, a pointer to the data */
};

struct ufile {
    uint64_t	 inode;		/* MFT record number */
    time_t		 date;		/* Last modification date/time */
    struct td_list_head name;		/* A list of filenames */
    struct td_list_head data;		/* A list of data streams */
    char		*pref_name;	/* Preferred filename */
    char		*pref_pname;	/*	     parent filename */
    uint64_t	 max_size;	/* Largest size we find */
    int		 attr_list;	/* MFT record may be one of many */
    int		 directory;	/* MFT record represents a directory */
    MFT_RECORD	*mft;		/* Raw MFT record */
};

static const char *UNKNOWN   = "unknown";

static unsigned int calc_percentage(struct ufile *file, ntfs_volume *vol)
{
    struct td_list_head *pos;
    unsigned int percent = 0;

    if (!file || !vol)
        return -1;

    if (file->directory) {
        return 0;
    }

    if (td_list_empty(&file->data)) {
        return 0;
    }

    td_list_for_each(pos, &file->data) {
        runlist_element *rl = NULL;
        uint64_t i;
        unsigned int clusters_inuse, clusters_free;
        struct data *data;
        data  = td_list_entry(pos, struct data, list);
        clusters_inuse = 0;
        clusters_free  = 0;

        if (data->encrypted) {
            //log_debug("File is encrypted, recovery is impossible.\n");
            continue;
        }

        if (data->compressed) {
            //log_debug("File is compressed, recovery not yet implemented.\n");
            continue;
        }

        if (data->resident) {
            percent = 100;
            data->percent = 100;
            continue;
        }

        rl = data->runlist;
        if (!rl) {
            //log_debug("File has no runlist, hence no data.\n");
            continue;
        }

        if (rl[0].length <= 0) {
            //log_debug("File has an empty runlist, hence no data.\n");
            continue;
        }

        if (rl[0].lcn == LCN_RL_NOT_MAPPED) {	/* extended mft record */
            //log_debug("Missing segment at beginning, %lld clusters\n", (long long)rl[0].length);
            clusters_inuse += rl[0].length;
            rl++;
        }

        for (i = 0; rl[i].length > 0; i++) {
            uint64_t start, end;
            uint64_t j;
            if (rl[i].lcn == LCN_RL_NOT_MAPPED) {
                //log_debug("Missing segment at end, %lld clusters\n",
                //(long long)rl[i].length);
                clusters_inuse += rl[i].length;
                continue;
            }

            if (rl[i].lcn == LCN_HOLE) {
                clusters_free += rl[i].length;
                continue;
            }

            start = rl[i].lcn;
            end   = rl[i].lcn + rl[i].length;

            for (j = start; j < end; j++) {
                if (utils_cluster_in_use(vol, j))
                    clusters_inuse++;
                else
                    clusters_free++;
            }
        }

        if ((clusters_inuse + clusters_free) == 0) {
            //log_error("ERROR: Unexpected error whilst "
            //"calculating percentage for inode %llu\n",
            //(long long unsigned)file->inode);
            continue;
        }

        data->percent = (clusters_free * 100) /
            (clusters_inuse + clusters_free);

        percent = max(percent, data->percent);
    }
    return percent;
}

static FILE_NAME_ATTR* verify_parent(const struct filename* name, MFT_RECORD* rec)
{
    ATTR_RECORD *attr30;
    FILE_NAME_ATTR *filename_attr = NULL, *lowest_space_name = NULL;
    ntfs_attr_search_ctx *ctx;
    int found_same_space = 1;

    if (!name || !rec)
        return NULL;

    if (!(rec->flags & MFT_RECORD_IS_DIRECTORY)) {
        return NULL;
    }

    ctx = ntfs_attr_get_search_ctx(NULL, rec);
    if (!ctx) {
        //log_error("ERROR: Couldn't create a search context.\n");
        return NULL;
    }

    attr30 = find_attribute(AT_FILE_NAME, ctx);
    if (!attr30) {
        return NULL;
    }

    filename_attr = (FILE_NAME_ATTR*)((char*)attr30 + le16_to_cpu(attr30->value_offset));
    /* if name is older than this dir -> can't determine */
    if (td_ntfs2utc(filename_attr->creation_time) > name->date_c) {
        return NULL;
    }
    if (filename_attr->file_name_type != name->name_space) {
        found_same_space = 0;
        lowest_space_name = filename_attr;

        while (!found_same_space && (attr30 = find_attribute(AT_FILE_NAME, ctx))) {
            filename_attr = (FILE_NAME_ATTR*)((char*)attr30 + le16_to_cpu(attr30->value_offset));

            if (filename_attr->file_name_type == name->name_space) {
                found_same_space = 1;
            } else {
                if (filename_attr->file_name_type < lowest_space_name->file_name_type) {
                    lowest_space_name = filename_attr;
                }
            }
        }
    }

    ntfs_attr_put_search_ctx(ctx);

    return (found_same_space ? filename_attr : lowest_space_name);
}

static void get_parent_name(struct filename* name, ntfs_volume* vol)
{
    ntfs_attr* mft_data;
    MFT_RECORD* rec;

    if (!name || !vol)
        return;

    mft_data = ntfs_attr_open(vol->mft_ni, AT_DATA, AT_UNNAMED, 0);
    if (!mft_data) {
        //log_error("ERROR: Couldn't open $MFT/$DATA\n");
        return;
    }
    rec = (MFT_RECORD*) calloc(1, vol->mft_record_size);
    if (!rec) {
        //log_error("ERROR: Couldn't allocate memory in get_parent_name()\n");
        ntfs_attr_close(mft_data);
        return;
    }

    {
        uint64_t inode_num;
        int ok;
        inode_num = MREF(name->parent_mref);
        name->parent_name = NULL;
        do
        {
            FILE_NAME_ATTR* filename_attr;
            ok=0;
            if (ntfs_attr_pread(mft_data, vol->mft_record_size * inode_num, vol->mft_record_size, rec) < 1)
            {
                //log_error("ERROR: Couldn't read MFT Record %llu.\n", (long long unsigned)inode_num);
            }
            else if ((filename_attr = verify_parent(name, rec)))
            {
                char *parent_name=NULL;
                if (ntfs_ucstombs(filename_attr->file_name,
                            filename_attr->file_name_length,
                            &parent_name, 0) < 0)
                {
                    //log_error("ERROR: Couldn't translate filename to current locale.\n");
                    parent_name = NULL;
                }
                else
                {
                    if(name->parent_name==NULL || parent_name==NULL)
                        name->parent_name=parent_name;
                    else
                    {
                        char *npn;
                        if(inode_num==5 && strcmp(parent_name,".")==0)
                        {
                            /* root directory */
                            npn=(char *)MALLOC(strlen(name->parent_name)+2);
                            sprintf(npn, "/%s", name->parent_name);
                        }
                        else
                        {
                            npn=(char *)MALLOC(strlen(parent_name)+strlen(name->parent_name)+2);
                            sprintf(npn, "%s/%s", parent_name, name->parent_name);
                        }
                        free(name->parent_name);
                        name->parent_name=npn;
                        free(parent_name);
                    }
                    if((unsigned)inode_num!=MREF(filename_attr->parent_directory))
                    {
                        inode_num=MREF(filename_attr->parent_directory);
                        ok=1;
                    }
                }
            }
        } while(ok);
    }
    free(rec);
    ntfs_attr_close(mft_data);
    return;
}

static int get_filenames(struct ufile *file, ntfs_volume* vol)
{
    ATTR_RECORD *rec;
    ntfs_attr_search_ctx *ctx;
    int count = 0;
    int space = 4;

    if (!file)
        return -1;

    ctx = ntfs_attr_get_search_ctx(NULL, file->mft);
    if (!ctx)
        return -1;

    while ((rec = find_attribute(AT_FILE_NAME, ctx))) {
        struct filename *name;
        FILE_NAME_ATTR *attr;
        /* We know this will always be resident. */
        attr = (FILE_NAME_ATTR *) ((char *) rec + le16_to_cpu(rec->value_offset));

        name = (struct filename *)calloc(1, sizeof(*name));
        if (!name) {
            //log_error("ERROR: Couldn't allocate memory in get_filenames().\n");
            count = -1;
            break;
        }

        name->uname      = attr->file_name;
        name->uname_len  = attr->file_name_length;
        name->name_space = attr->file_name_type;
        name->size_alloc = sle64_to_cpu(attr->allocated_size);
        name->size_data  = sle64_to_cpu(attr->data_size);
        name->flags      = attr->file_attributes;

        name->date_c     = td_ntfs2utc(attr->creation_time);
        name->date_a     = td_ntfs2utc(attr->last_data_change_time);
        name->date_m     = td_ntfs2utc(attr->last_mft_change_time);
        name->date_r     = td_ntfs2utc(attr->last_access_time);

        if (ntfs_ucstombs(name->uname, name->uname_len, &name->name,
                    0) < 0) {
            //log_error("ERROR: Couldn't translate filename to current locale.\n");
        }

        name->parent_name = NULL;
        name->parent_mref = attr->parent_directory;
        get_parent_name(name, vol);

        if (name->name_space < space) {
            file->pref_name = name->name;
            file->pref_pname = name->parent_name;
            space = name->name_space;
        }

        file->max_size = max(file->max_size, name->size_alloc);
        file->max_size = max(file->max_size, name->size_data);

        td_list_add_tail(&name->list, &file->name);
        count++;
    }

    ntfs_attr_put_search_ctx(ctx);
    //log_debug("File has %d names.\n", count);
    return count;
}

static int get_data(struct ufile *file, const ntfs_volume *vol)
{
    ATTR_RECORD *rec;
    ntfs_attr_search_ctx *ctx;
    int count = 0;

    if (!file)
        return -1;

    ctx = ntfs_attr_get_search_ctx(NULL, file->mft);
    if (!ctx)
        return -1;

    while ((rec = find_attribute(AT_DATA, ctx))) {
        struct data *data;
        data = (struct data *)calloc(1, sizeof(*data));
        if (!data) {
            //log_error("ERROR: Couldn't allocate memory in get_data().\n");
            count = -1;
            break;
        }

        data->resident   = !rec->non_resident;
        data->compressed = rec->flags & ATTR_IS_COMPRESSED;
        data->encrypted  = rec->flags & ATTR_IS_ENCRYPTED;

        if (rec->name_length) {
            data->uname     = (ntfschar *) ((char *) rec + le16_to_cpu(rec->name_offset));
            data->uname_len = rec->name_length;

            if (ntfs_ucstombs(data->uname, data->uname_len, &data->name,
                        0) < 0) {
                //log_error("ERROR: Cannot translate name into current locale.\n");
            }
        }

        if (data->resident) {
            data->size_data  = le32_to_cpu(rec->value_length);
            data->data	 = ((char*) (rec)) + le16_to_cpu(rec->value_offset);
        } else {
            data->size_alloc = sle64_to_cpu(rec->allocated_size);
            data->size_data  = sle64_to_cpu(rec->data_size);
            data->size_init  = sle64_to_cpu(rec->initialized_size);
            data->size_vcn   = sle64_to_cpu(rec->highest_vcn) + 1;
        }

        data->runlist = ntfs_mapping_pairs_decompress(vol, rec, NULL);
        if (!data->runlist) {
            //log_debug("Couldn't decompress the data runs.\n");
        }

        file->max_size = max(file->max_size, data->size_data);
        file->max_size = max(file->max_size, data->size_init);

        td_list_add_tail(&data->list, &file->data);
        count++;
    }

    ntfs_attr_put_search_ctx(ctx);
    //log_debug("File has %d data streams.\n", count);
    return count;
}

static void free_file(struct ufile *file)
{
    struct td_list_head *item, *tmp;

    if (!file)
        return;

    td_list_for_each_safe(item, tmp, &file->name) { /* List of filenames */
        struct filename *f = td_list_entry(item, struct filename, list);
        free(f->name);
        free(f->parent_name);
        free(f);
    }

    td_list_for_each_safe(item, tmp, &file->data) { /* List of data streams */
        struct data *d = td_list_entry(item, struct data, list);
        free(d->name);
        free(d->runlist);
        free(d);
    }

    free(file->mft);
    free(file);
}

static struct ufile * read_record(char *error_Buffer, ntfs_volume *vol, uint64_t record)
{
    ATTR_RECORD *attr10, *attr20, *attr90;
    struct ufile *file;
    ntfs_attr *mft;

    if (!vol)
        return NULL;

    file = (struct ufile *)calloc(1, sizeof(*file));
    if (!file) {
        sprintf(error_Buffer, "ERROR: Couldn't allocate memory in read_record()\n");
        return NULL;
    }

    TD_INIT_LIST_HEAD(&file->name);
    TD_INIT_LIST_HEAD(&file->data);
    file->inode = record;

    file->mft = (MFT_RECORD *)MALLOC(vol->mft_record_size);

    mft = ntfs_attr_open(vol->mft_ni, AT_DATA, AT_UNNAMED, 0);
    if (!mft) {
        sprintf(error_Buffer, "ERROR: Couldn't open $MFT/$DATA\n");
        free_file(file);
        return NULL;
    }

    if (ntfs_attr_mst_pread(mft, vol->mft_record_size * record, 1, vol->mft_record_size, file->mft) < 1) {
        sprintf(error_Buffer, "ERROR: Couldn't read MFT Record %llu.\n", (long long unsigned)record);
        ntfs_attr_close(mft);
        free_file(file);
        return NULL;
    }

    ntfs_attr_close(mft);
    mft = NULL;

    attr10 = find_first_attribute(AT_STANDARD_INFORMATION,	file->mft);
    attr20 = find_first_attribute(AT_ATTRIBUTE_LIST,	file->mft);
    attr90 = find_first_attribute(AT_INDEX_ROOT,		file->mft);

    sprintf(error_Buffer, "Attributes present: %s %s %s.\n", attr10?"0x10":"",
    attr20?"0x20":"", attr90?"0x90":"");

    if (attr10) {
        STANDARD_INFORMATION *si;
        si = (STANDARD_INFORMATION *) ((char *) attr10 + le16_to_cpu(attr10->value_offset));
        file->date = td_ntfs2utc(si->last_data_change_time);
    }

    if (attr20 || !attr10)
        file->attr_list = 1;
    if (attr90)
        file->directory = 1;

    if (get_filenames(file, vol) < 0) {
        sprintf(error_Buffer, "ERROR: Couldn't get filenames.\n");
    }
    if (get_data(file, vol) < 0) {
        sprintf(error_Buffer, "ERROR: Couldn't get data streams.\n");
    }

    return file;
}

static file_info_t *ufile_to_file_data(const struct ufile *file, const struct data *d)
{
    file_info_t *new_file=(file_info_t *)MALLOC(sizeof(*new_file));
    char inode_name[32];
    const unsigned int len=(file->pref_pname==NULL?0:strlen(file->pref_pname)) +
        (file->pref_name==NULL?sizeof(inode_name):strlen(file->pref_name) + 1) +
        (d->name==NULL?0:strlen(d->name) + 1) + 1;
    sprintf(inode_name, "inode_%llu", (long long unsigned)file->inode);
    new_file->name=(char *)MALLOC(len);
    sprintf(new_file->name, "%s%s%s%s%s",
            (file->pref_pname?file->pref_pname:""),
            (file->pref_pname?"/":""),
            (file->pref_name?file->pref_name:inode_name),
            (d->name?":":""),
            (d->name?d->name:""));
    new_file->st_ino=file->inode;
    new_file->st_mode = (file->directory ?LINUX_S_IFDIR| LINUX_S_IRUGO | LINUX_S_IXUGO:LINUX_S_IFREG | LINUX_S_IRUGO);
    new_file->st_uid=0;
    new_file->st_gid=0;

    new_file->st_size=max(d->size_init, d->size_data);
    new_file->td_atime=new_file->td_ctime=new_file->td_mtime=file->date;
    new_file->status=0;
    return new_file;
}

int Quick_Scan_File(char *error_Buffer, ntfs_volume *vol, file_info_t *dir_list)
{
    uint64_t nr_mft_records;
    const unsigned int BUFSIZE = 8192;
    char *buffer = NULL;
    unsigned int results = 0;
    ntfs_attr *attr;
    uint64_t bmpsize;
    uint64_t i;
    struct ufile *file;

    if (!vol)
    {
        sprintf(error_Buffer, "NTFS Volume Open Fail"); 

        return EXIT_FAILURE;
    }

    attr = ntfs_attr_open(vol->mft_ni, AT_BITMAP, AT_UNNAMED, 0);
    if (!attr)
    {
        sprintf(error_Buffer, "$MFT/$BITMAP Open Fail\n");

        return EXIT_FAILURE;
    }
    bmpsize = attr->initialized_size;

    buffer = (char *) MALLOC(BUFSIZE);

    nr_mft_records = vol->mft_na->initialized_size >>
        vol->mft_record_size_bits;

    for (i = 0; i < bmpsize; i += BUFSIZE) {
        int64_t size;
        unsigned int j;
        uint64_t read_count = min((bmpsize - i), BUFSIZE);
        size = ntfs_attr_pread(attr, i, read_count, buffer);
        if (size < 0)
            break;

        for (j = 0; j < size; j++) {
            unsigned int k;
            unsigned int b;
            b = buffer[j];
            for (k = 0; k < 8; k++, b>>=1)
            {
                unsigned int percent;
                if (((i+j)*8+k) >= nr_mft_records)
                    goto done;
                if (b & 1)
                    continue;
                file = read_record(error_Buffer, vol, (i+j)*8+k);
                if (!file) {
                    continue;
                }

                percent = calc_percentage(file, vol);
                if (percent >0)
                {
                    struct td_list_head *item;
                    td_list_for_each(item, &file->data)
                    {
                        const struct data *d = td_list_entry_const(item, const struct data, list);
                        file_info_t *new_file;
                        new_file=ufile_to_file_data(file, d);
                        if(new_file!=NULL)
                        {
                            td_list_add_tail(&new_file->list, &dir_list->list);
                            results++;
                        }
                    }
                }
                free_file(file);
            }
        }
    }
done:
    free(buffer);
    ntfs_attr_close(attr);
    td_list_sort(&dir_list->list, filesort);
}

static int create_pathname(const char *dir, const char *dir2, const char *name,
        const char *stream, char *buffer, int bufsize)
{
    char *namel;
    if (name==NULL)
        name = UNKNOWN;
    namel=gen_local_filename(name);
    if(dir2)
    {
        char *dir2l=gen_local_filename(dir2);
        if (stream)
        {
            char *streaml=gen_local_filename(stream);
            snprintf(buffer, bufsize, "%s/%s/%s:%s", dir, dir2l, namel, streaml);
            free(streaml);
        }
        else
            snprintf(buffer, bufsize, "%s/%s/%s", dir, dir2l, namel);
        free(dir2l);
    }
    else
    {
        if (stream)
        {
            char *streaml=gen_local_filename(stream);
            snprintf(buffer, bufsize, "%s/%s:%s", dir, namel, streaml);
            free(streaml);
        }
        else
            snprintf(buffer, bufsize, "%s/%s", dir, namel);
    }
    free(namel);
    return strlen(buffer);
}

static int open_file(const char *pathname)
{
    int fh;
    fh=open(pathname, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if(fh != -1 || errno!=ENOENT)
        return fh;
    mkdir_local_for_file(pathname);
    return open(pathname, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
}

static unsigned int write_data(int fd, const char *buffer,
        unsigned int bufsize)
{
    ssize_t result1, result2;

    if (!buffer) {
        errno = EINVAL;
        return -1;
    }

    result1 = write(fd, buffer, bufsize);
    if ((result1 == (ssize_t) bufsize) || (result1 < 0))
        return result1;

    /* Try again with the rest of the buffer */
    buffer  += result1;
    bufsize -= result1;

    result2 = write(fd, buffer, bufsize);
    if (result2 < 0)
        return result1;

    return result1 + result2;
}

int undelete_file(char *error_Buffer, ntfs_volume *vol, uint64_t inode)
{
    char *buffer = NULL;
    unsigned int bufsize;
    struct ufile *file;
    struct td_list_head *item;

    if (!vol)
        return -2;

    /* try to get record */
    file = read_record(error_Buffer, vol, inode);
    if (!file || !file->mft) {
        sprintf(error_Buffer, "Can't read info from mft record %llu.\n", (long long unsigned)inode);
        return -2;
    }


    bufsize = vol->cluster_size;
    buffer = (char *)MALLOC(bufsize);

    /* calc_percentage() must be called before 
     * list_record(). Otherwise, when undeleting, a file will always be
     * listed as 0% recoverable even if successfully undeleted. +mabs
     */
    if (file->mft->flags & MFT_RECORD_IN_USE) {
        sprintf(error_Buffer, "Record is in use by the mft\n");
        //log_error();
        free(buffer);
        free_file(file);
        return -2;
    }

    if (calc_percentage(file, vol) == 0) {
        sprintf(error_Buffer, "File has no recoverable data.\n");
        goto free;
    }

    if (td_list_empty(&file->data)) {
        sprintf(error_Buffer, "File has no data.  There is nothing to recover.\n");
        goto free;
    }

    td_list_for_each(item, &file->data) {
        char pathname[256];
        char defname[64];
        char *name;
        struct data *d = td_list_entry(item, struct data, list);
        if(file->pref_name)
        {
            name = file->pref_name;
        }
        else
        {
            sprintf(defname, "inode_%llu", (long long unsigned)file->inode);
            name = defname;
        }

        //dir_data->local_dir;
        extern my_Data_Context *g_my_Data_Ctx;
        create_pathname(g_my_Data_Ctx->data_Rec_Ctx->file_Ctx->destination, file->pref_pname, name, d->name, pathname, sizeof(pathname));
        if (d->resident) {
            int fd;
            fd = open_file(pathname);
            if (fd < 0) {
                sprintf(error_Buffer, "Couldn't create file %s\n", pathname);
                goto free;
            }

            //log_verbose("File has resident data.\n");
            if (write_data(fd, (const char *)d->data, d->size_data) < d->size_data) {
                sprintf(error_Buffer, "Write failed\n");
                close(fd);
                goto free;
            }

            if (close(fd) < 0) {
                sprintf(error_Buffer, "Close failed\n");
            }
        } else {
            int i;
            int fd;
            uint64_t cluster_count;	/* I'll need this variable (see below). +mabs */
            runlist_element *rl;
            rl = d->runlist;
            if (!rl) {
                sprintf(error_Buffer, "File has no runlist, hence no data.\n");
                continue;
            }

            if (rl[0].length <= 0) {
                sprintf(error_Buffer, "File has an empty runlist, hence no data.\n");
                continue;
            }

            fd = open_file(pathname);
            if (fd < 0) {
                sprintf(error_Buffer, "Couldn't create output file %s\n", pathname);
                goto free;
            }

            if (rl[0].lcn == LCN_RL_NOT_MAPPED) {	/* extended mft record */
                uint64_t k;
                sprintf(error_Buffer, "Missing segment at beginning, %lld "
                        "clusters.\n",
                        (long long)rl[0].length);
                memset(buffer, 0, bufsize);
                for (k = 0; k < (uint64_t)rl[0].length * vol->cluster_size; k += bufsize) {
                    if (write_data(fd, buffer, bufsize) < bufsize) {
                        sprintf(error_Buffer, "Write failed\n");
                        close(fd);
                        goto free;
                    }
                }
            }

            cluster_count = 0;
            for (i = 0; rl[i].length > 0; i++) {
                uint64_t start, end;
                uint64_t j;

                if (rl[i].lcn == LCN_RL_NOT_MAPPED) {
                    uint64_t k;
                    sprintf(error_Buffer, "Missing segment at end, "
                            "%lld clusters.\n",
                            (long long)rl[i].length);
                    memset(buffer, 0, bufsize);
                    for (k = 0; k < (uint64_t)rl[i].length * vol->cluster_size; k += bufsize) {
                        if (write_data(fd, buffer, bufsize) < bufsize) {
                            sprintf(error_Buffer, "Write failed\n");
                            close(fd);
                            goto free;
                        }
                        cluster_count++;
                    }
                    continue;
                }

                if (rl[i].lcn == LCN_HOLE) {
                    uint64_t k;
                    sprintf(error_Buffer, "File has a sparse section.\n");
                    memset(buffer, 0, bufsize);
                    for (k = 0; k < (uint64_t)rl[i].length * vol->cluster_size; k += bufsize) {
                        if (write_data(fd, buffer, bufsize) < bufsize) {
                            sprintf(error_Buffer, "Write failed\n");
                            close(fd);
                            goto free;
                        }
                    }
                    continue;
                }

                start = rl[i].lcn;
                end   = rl[i].lcn + rl[i].length;

                for (j = start; j < end; j++)
                {
                    /* Don't check if clusters are in used or not */
#if 0
                    if (utils_cluster_in_use(vol, j) && !opts.optimistic)
                    {
                        memset(buffer, 0, bufsize);
                        if (write_data(fd, buffer, bufsize) < bufsize)
                        {
                            //log_error("Write failed\n");
                            close(fd);
                            goto free;
                        }
                    }
                    else
#endif
                    {
                        if (ntfs_cluster_read(vol, j, 1, buffer) < 1) {
                            sprintf(error_Buffer, "Read failed\n");
                            close(fd);
                            goto free;
                        }
                        if (write_data(fd, buffer, bufsize) < bufsize) {
                            sprintf(error_Buffer, "Write failed\n");
                            close(fd);
                            goto free;
                        }
                        cluster_count++;
                    }
                }
            }

            /*
             * IF data stream currently being recovered is
             * non-resident AND data stream has no holes (100% recoverability) AND
             * 0 <= (data->size_alloc - data->size_data) <= vol->cluster_size AND
             * cluster_count * vol->cluster_size == data->size_alloc THEN file
             * currently being written is truncated to data->size_data bytes before
             * it's closed.
             * This multiple checks try to ensure that only files with consistent
             * values of size/occupied clusters are eligible for truncation. Note
             * that resident streams need not be truncated, since the original code
             * already recovers their exact length.                           +mabs
             */
            if (d->percent == 100 && d->size_alloc >= d->size_data &&
                    (d->size_alloc - d->size_data) <= (uint64_t)vol->cluster_size &&
                    cluster_count * (uint64_t)vol->cluster_size == d->size_alloc)
            {
                if (ftruncate(fd, (off_t)d->size_data))
                    sprintf(error_Buffer, "Truncation failed\n");
            }
            else
                sprintf(error_Buffer, "Truncation not performed because file has an "
                        "inconsistent $MFT record.\n");

            if (close(fd) < 0) {
                sprintf(error_Buffer, "Close failed\n");
            }

        }
        set_date(pathname, file->date, file->date);
    }
    free(buffer);
    free_file(file);
    return 0;
free:
    free(buffer);
    free_file(file);
    return -2;
}

