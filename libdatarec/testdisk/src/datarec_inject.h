#ifndef __DATAREC_INJECT_H__
#define __DATAREC_INJECT_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(__FRAMAC__) || defined(MAIN_photorec)
#undef HAVE_LIBNTFS
#undef HAVE_LIBNTFS3G
#endif

#include <stdio.h>
#ifdef HAVE_FEATURES_H
#include <features.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include "types.h"
#include "common.h"
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#if !defined(REG_NOERROR) || (REG_NOERROR != 0)
#define REG_NOERROR 0
#endif

#include "list.h"
#include "list_sort.h"
#include "ntfs_udl.h"
#include "intrf.h"
#include "intrfn.h"

#ifdef HAVE_LIBNTFS
#include <ntfs/bootsect.h>
#include <ntfs/mft.h>
#include <ntfs/attrib.h>
#include <ntfs/layout.h>
#include <ntfs/inode.h>
#include <ntfs/device.h>
#include <ntfs/debug.h>
#include <ntfs/ntfstime.h>
#ifdef HAVE_NTFS_VERSION_H
#include <ntfs/version.h>
#endif
#endif

#ifdef HAVE_LIBNTFS3G
#include <ntfs-3g/bootsect.h>
#include <ntfs-3g/mft.h>
#include <ntfs-3g/attrib.h>
#include <ntfs-3g/layout.h>
#include <ntfs-3g/inode.h>
#include <ntfs-3g/device.h>
#include <ntfs-3g/debug.h>
#include <ntfs-3g/ntfstime.h>
#endif

#if defined(HAVE_LIBNTFS) || defined(HAVE_LIBNTFS3G)
#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif
#include "dir.h"
#include "ntfs_inc.h"
#include "ntfs_dir.h"
#include "ntfs_utl.h"
#include "askloc.h"
#include "setdate.h"
#include "datarec_inject.h"
#include "../../../log/log.h"
#include "../../../common.h"

int Quick_Scan_File(ntfs_volume *vol, file_info_t *dir_list);
int Deep_Scan_File(testdisk_Context *testdisk_Ctx);

#endif

#endif
