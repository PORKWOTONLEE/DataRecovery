/*

    File: fidentify.c

    Copyright (C) 2009 Christophe GRENIER <grenier@cgsecurity.org>

    This software is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write the Free Software Foundation, Inc., 51
    Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef __FRAMAC__
#undef HAVE_FTELLO
#endif

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <dirent.h>
#include "types.h"
#include "common.h"
#include "filegen.h"
#include "log.h"
#include "misc.h"
#include "file_jpg.h"
#include "file_gz.h"
#if defined(__FRAMAC__)
#include "__fc_builtin.h"
#endif

extern file_enable_t array_file_enable[];
extern file_check_list_t file_check_list;

#define READ_SIZE 1024*512
#define OPT_CHECK 1
#define OPT_TIME  2

/*@
  @ requires \valid(file_recovery);
  @ requires valid_file_recovery(file_recovery);
  @ requires \valid_function(file_recovery->data_check);
  @ requires 0 < blocksize <= READ_SIZE;
  @ requires READ_SIZE % blocksize == 0;
  @ requires \separated(file_recovery, &errno);
  @ ensures  valid_file_recovery(file_recovery);
  @*/
static data_check_t data_check(file_recovery_t *file_recovery, const unsigned int blocksize)
{
  unsigned char *buffer_start;
  const unsigned int buffer_size=blocksize + READ_SIZE;
  if( file_recovery->calculated_file_size >= PHOTOREC_MAX_FILE_SIZE )
    return DC_STOP;
  if(my_fseek(file_recovery->handle, 0, SEEK_SET) < 0)
    return DC_STOP;
  buffer_start=(unsigned char *)MALLOC(buffer_size);
  /*@
    @ loop invariant valid_file_recovery(file_recovery);
    @ loop invariant file_recovery == \at(file_recovery, Pre);
    @ loop invariant file_recovery->calculated_file_size < PHOTOREC_MAX_FILE_SIZE;
    @*/
  while(1)
  {
    unsigned char *buffer=buffer_start+blocksize;
    unsigned int i;
    size_t lu;
    memset(buffer, 0, READ_SIZE);
    lu=fread(buffer, 1, READ_SIZE, file_recovery->handle);
    if(lu <= 0)
    {
      free(buffer_start);
      return DC_STOP;
    }
    /*@ assert 0 < lu <= READ_SIZE; */
    /*@
      @ loop invariant valid_file_recovery(file_recovery);
      @ loop invariant file_recovery == \at(file_recovery, Pre);
      @ loop invariant \valid_read(buffer_start + (0 .. blocksize + READ_SIZE - 1));
      @ loop invariant file_recovery->calculated_file_size < PHOTOREC_MAX_FILE_SIZE;
      @ loop invariant \valid_function(file_recovery->data_check);
      @*/
    for(i=0; i<lu; i+=blocksize)
    {
      /*@ assert i + 2*blocksize <= buffer_size; */
      /*@ assert \valid_read(&buffer_start[i] + (0 .. 2*blocksize-1)); */
      const data_check_t res=file_recovery->data_check(&buffer_start[i], 2*blocksize, file_recovery);
      /*@ assert \valid_read(&buffer_start[i] + (0 .. 2*blocksize-1)); */
      file_recovery->file_size+=blocksize;
      if(res != DC_CONTINUE || file_recovery->data_check==NULL)
      {
	free(buffer_start);
	return res;
      }
      if( file_recovery->calculated_file_size >= PHOTOREC_MAX_FILE_SIZE ||
	  file_recovery->file_size >= PHOTOREC_MAX_FILE_SIZE)
      {
	free(buffer_start);
	return DC_STOP;
      }
      /*@ assert file_recovery->calculated_file_size < PHOTOREC_MAX_FILE_SIZE; */
    }
    if(lu != READ_SIZE)
    {
      free(buffer_start);
      return DC_STOP;
    }
    memcpy(buffer_start, &buffer_start[READ_SIZE], blocksize);
  }
}

/*@
  @ requires valid_read_string(filename);
  @ requires \separated(filename + (..), &errno, &Frama_C_entropy_source, stdout);
  @*/
static int file_identify(const char *filename, const unsigned int options)
{
  const unsigned int blocksize=65536;
  /*@ assert blocksize <= READ_SIZE; */
  const unsigned int buffer_size=blocksize + READ_SIZE;
  unsigned char *buffer_start;
  unsigned char *buffer;
  buffer_start=(unsigned char *)MALLOC(buffer_size);
  buffer=buffer_start + blocksize;
#ifdef __AFL_COMPILER
  while(__AFL_LOOP(10000))
#endif
  {
    FILE *file;
    file=fopen(filename, "rb");
    if(file==NULL)
    {
      free(buffer_start);
      return -1;
    }
    if(fread(buffer, 1, READ_SIZE, file) >0)
    {
      const struct td_list_head *tmpl;
      file_recovery_t file_recovery_new;
      file_recovery_t file_recovery;
#if defined(__FRAMAC__)
      Frama_C_make_unknown((char *)buffer, READ_SIZE);
#endif
      reset_file_recovery(&file_recovery);
      reset_file_recovery(&file_recovery_new);
      file_recovery.blocksize=blocksize;
      file_recovery_new.blocksize=blocksize;
      /*@ assert valid_file_recovery(&file_recovery); */
      /*@ assert valid_file_recovery(&file_recovery_new); */
      /*@ assert file_recovery_new.file_stat==NULL; */
      /*@
        @ loop invariant \valid_read(tmpl);
	@ loop invariant valid_file_recovery(&file_recovery);
	@*/
      td_list_for_each(tmpl, &file_check_list.list)
      {
	const struct td_list_head *tmp;
	const file_check_list_t *pos=td_list_entry_const(tmpl, const file_check_list_t, list);
	/*@ assert \valid_read(pos); */
	/*@ assert pos->offset < READ_SIZE; */
	const struct td_list_head *tmp_list=&pos->file_checks[buffer[pos->offset]].list;
	/*@ loop invariant \valid_read(tmp); */
	td_list_for_each(tmp, tmp_list)
	{
	  /*TODO assert tmp!=tmp_list; */
	  const file_check_t *file_check=td_list_entry_const(tmp, const file_check_t, list);
	  /*@ assert \valid_read(file_check); */
	  if(
#ifdef __FRAMAC__
	    file_check->header_check!=NULL &&
#endif
	    (file_check->length==0 || memcmp(buffer + file_check->offset, file_check->value, file_check->length)==0) &&
	    file_check->header_check(buffer, blocksize, 0, &file_recovery, &file_recovery_new)!=0)
	  {
	    /*@ assert valid_file_check_node(file_check); */
	    /*@ assert valid_file_recovery(&file_recovery_new); */
	    file_recovery_new.file_stat=file_check->file_stat;
	    break;
	  }
	}
	if(file_recovery_new.file_stat!=NULL)
	  break;
      }
      if(file_recovery_new.file_stat!=NULL &&
	  file_recovery_new.file_stat->file_hint!=NULL &&
	  (file_recovery_new.file_check!=NULL || file_recovery_new.data_check!=NULL) &&
#ifdef __FRAMAC__
	  file_recovery_new.extension != NULL &&
#endif
	  ((options&OPT_CHECK)!=0 || ((options&OPT_TIME)!=0 && file_recovery_new.time==0))
	)
      {
	off_t file_size;
	/*@ assert valid_read_string(file_recovery_new.extension); */
	file_recovery_new.handle=file;
	/*@ assert valid_file_recovery(&file_recovery_new); */
	my_fseek(file_recovery_new.handle, 0, SEEK_END);
#if defined(HAVE_FTELLO)
	file_size=ftello(file_recovery_new.handle);
#else
	file_size=ftell(file_recovery_new.handle);
#endif
	if(file_size<=0)
	{
	  file_size=0;
	}
	if(file_size > 0 && file_recovery_new.data_check!=NULL)
	{
	  data_check_t data_check_status=data_check(&file_recovery_new, blocksize);
	  if(data_check_status==DC_ERROR)
	    file_recovery_new.file_size=0;
	  /* Adjust file_size so it's possible to fseek to the end of the file correctly */
	  if(file_recovery_new.file_size > file_size)
	    file_recovery_new.file_size=file_size;
	}
	if(file_recovery_new.data_check==NULL)
	{
	  file_recovery_new.file_size=file_size;
	  file_recovery_new.calculated_file_size=file_recovery_new.file_size;
	}
	/*@ assert \valid(&file_recovery_new); */
	/*@ assert valid_file_recovery(&file_recovery_new); */
	/*@ assert \valid(file_recovery_new.handle); */
	/*@ assert \separated(&file_recovery_new, file_recovery_new.handle, file_recovery_new.extension, &errno, &Frama_C_entropy_source); */
	/*@ assert valid_file_check_param(&file_recovery_new); */
	if(file_recovery_new.file_size>0 && file_recovery_new.file_check!=NULL)
	{
	  (file_recovery_new.file_check)(&file_recovery_new);
	}
	if(file_recovery_new.file_size < file_recovery_new.min_filesize)
	  file_recovery_new.file_size=0;
	if(file_recovery_new.file_size==0)
	  file_recovery_new.file_stat=NULL;
      }
      if(file_recovery_new.file_stat!=NULL && file_recovery_new.file_stat->file_hint!=NULL)
      {
	printf("%s: %s", filename,
	    ((file_recovery_new.extension!=NULL && file_recovery_new.extension[0]!='\0')?
	     file_recovery_new.extension:file_recovery_new.file_stat->file_hint->description));
	if((options&OPT_CHECK)!=0 && (file_recovery_new.file_check!=NULL || file_recovery_new.data_check!=NULL))
	  printf(" file_size=%llu", (long long unsigned)file_recovery_new.file_size);
	if((options&OPT_TIME)!=0 && file_recovery_new.time!=0 && file_recovery_new.time!=(time_t)-1)
#ifdef __FRAMAC__
	{
	  printf(" time=%lld", (long long)file_recovery_new.time);
	}
#else
	{
	  char outstr[200];
#if defined(__MINGW32__) || defined(__FRAMAC__)
	  const struct  tm *tmp = localtime(&file_recovery_new.time);
#else
	  struct tm tm_tmp;
	  const struct tm *tmp = localtime_r(&file_recovery_new.time,&tm_tmp);
#endif
	  if(tmp != NULL &&
	      strftime(outstr, sizeof(outstr), "%Y-%m-%dT%H:%M:%S%z", tmp) != 0)
	    printf(" time=%s", &outstr[0]);
	}
#endif
	printf("\n");
#ifdef __FRAMAC__
	if(file_recovery_new.file_rename!=NULL)
	{
	  file_recovery_new.file_rename(&file_recovery_new);
	}
#endif
      }
      else
      {
	printf("%s: unknown\n", filename);
      }
    }
    fclose(file);
  }
  free(buffer_start);
  return 0;
}

#if !defined(__AFL_COMPILER) && !defined(MAIN_fidentify)
static void file_identify_dir(const char *current_dir, const unsigned int options)
{
  DIR *dir;
  struct dirent *entry;
  dir=opendir(current_dir);
  if(dir==NULL)
    return;
  while((entry=readdir(dir))!=NULL)
  {
    if(strcmp(entry->d_name,".")!=0 && strcmp(entry->d_name,"..")!=0)
    {
      struct stat buf_stat;
      char *current_file=(char *)MALLOC(strlen(current_dir)+1+strlen(entry->d_name)+1);
      strcpy(current_file, current_dir);
      strcat(current_file, "/");
      strcat(current_file, entry->d_name);
#ifdef HAVE_LSTAT
      if(lstat(current_file, &buf_stat)==0)
#else
	if(stat(current_file, &buf_stat)==0)
#endif
	{
	  if(S_ISDIR(buf_stat.st_mode))
	    file_identify_dir(current_file, options);
	  else if(S_ISREG(buf_stat.st_mode))
	    file_identify(current_file, options);
	}
      free(current_file);
    }
  }
  closedir(dir);
}
#endif

static void display_help(void)
{
  printf("\nUsage: fidentify [--check] [+file_format] [directory|file]\n"\
      "       fidentify --version\n" \
      "\n" \
      "fidentify determines the file type, the 'extension', by using the same database as PhotoRec.\n"
      "By default, all known file formats are searched unless one is specifically enabled.");
}

static void display_version(void)
{
  printf("fidentify %s, Data Recovery Utility, %s\nChristophe GRENIER <grenier@cgsecurity.org>\nhttps://www.cgsecurity.org\n",VERSION,TESTDISKDATE);
  printf("\n");
  printf("Version: %s\n", VERSION);
  printf("Compiler: %s\n", get_compiler());
#ifdef RECORD_COMPILATION_DATE
  printf("Compilation date: %s\n", get_compilation_date());
#endif
#ifndef MAIN_fidentify
  printf("libjpeg: %s, zlib: %s\n", td_jpeg_version(), td_zlib_version());
  printf("OS: %s\n" , get_os());
#endif
}

int main(int argc, char **argv)
{
  int i;
#ifdef MAIN_fidentify
  unsigned int options=OPT_CHECK|OPT_TIME;
#else
  unsigned int options=0;
#endif
  int log_errno=0;
  int enable_all_formats=1;
  int scan_dir=1;
  file_stat_t *file_stats;
  log_set_levels(LOG_LEVEL_DEBUG|LOG_LEVEL_TRACE|LOG_LEVEL_QUIET|LOG_LEVEL_INFO|LOG_LEVEL_VERBOSE|LOG_LEVEL_PROGRESS|LOG_LEVEL_WARNING|LOG_LEVEL_ERROR|LOG_LEVEL_PERROR|LOG_LEVEL_CRITICAL);
#ifndef MAIN_fidentify
  for(i=1; i<argc; i++)
  {
    if( strcmp(argv[i], "/check")==0 || strcmp(argv[i], "-check")==0 || strcmp(argv[i], "--check")==0)
    {
      options|=OPT_CHECK;
    }
    if( strcmp(argv[i], "/time")==0 || strcmp(argv[i], "-time")==0 || strcmp(argv[i], "--time")==0)
    {
      options|=OPT_TIME;
    }
    else if(strcmp(argv[i],"/help")==0 || strcmp(argv[i],"-help")==0 || strcmp(argv[i],"--help")==0 ||
      strcmp(argv[i],"/h")==0 || strcmp(argv[i],"-h")==0 ||
      strcmp(argv[i],"/?")==0 || strcmp(argv[i],"-?")==0)
    {
      display_help();
      return 0;
    }
    else if((strcmp(argv[i],"/version")==0) || (strcmp(argv[i],"-version")==0) || (strcmp(argv[i],"--version")==0) ||
      (strcmp(argv[i],"/v")==0) || (strcmp(argv[i],"-v")==0))
    {
      display_version();
      return 0;
    }
  }
#endif
#ifndef __AFL_COMPILER
  log_open("fidentify.log", TD_LOG_CREATE, &log_errno);
  {
    time_t my_time;
    my_time=time(NULL);
    log_info("\n\n%s",ctime(&my_time));
  }
  log_info("Command line: fidentify");
#ifndef MAIN_fidentify
  for(i=1;i<argc;i++)
    log_info(" %s", argv[i]);
#endif
  log_info("\n\n");
  log_info("fidentify %s, Data Recovery Utility, %s\nChristophe GRENIER <grenier@cgsecurity.org>\nhttps://www.cgsecurity.org\n", VERSION, TESTDISKDATE);
  log_flush();
#endif
#ifndef MAIN_fidentify
  for(i=1; i<argc; i++)
  {
    file_enable_t *file_enable;
    for(file_enable=array_file_enable;file_enable->file_hint!=NULL;file_enable++)
      if(argv[i][0]=='+' &&
	  file_enable->file_hint->extension!=NULL &&
	  strcmp(file_enable->file_hint->extension,&argv[i][1])==0)
      {
	file_enable->enable=1;
	enable_all_formats=0;
      }
  }
#endif
  if(enable_all_formats)
  {
    /* Enable all file formats */
    file_enable_t *file_enable;
    for(file_enable=array_file_enable;file_enable->file_hint!=NULL;file_enable++)
      file_enable->enable=1;
  }
  file_stats=init_file_stats(array_file_enable);
#ifndef MAIN_fidentify
  for(i=1; i<argc; i++)
  {
    if(strcmp(argv[i], "/check")==0 || strcmp(argv[i], "-check")==0 || strcmp(argv[i], "--check")==0 ||
	strcmp(argv[i], "/time")==0 || strcmp(argv[i], "-time")==0 || strcmp(argv[i], "--time")==0 ||
	argv[i][0]=='+')
    {
    }
    else
    {
      struct stat buf_stat;
      scan_dir=0;
#ifdef HAVE_LSTAT
      if(lstat(argv[i], &buf_stat)==0)
#else
	if(stat(argv[i], &buf_stat)==0)
#endif
	{
	  if(S_ISREG(buf_stat.st_mode))
	    file_identify(argv[i], options);
#ifndef __AFL_COMPILER
	  else if(S_ISDIR(buf_stat.st_mode))
	    file_identify_dir(argv[i], options);
#endif
	}
    }
  }
#ifndef __AFL_COMPILER
  if(scan_dir)
    file_identify_dir(".", options);
#endif
#else
  file_identify("demo", options);
#endif
  free_header_check();
  free(file_stats);
  log_close();
  return 0;
}