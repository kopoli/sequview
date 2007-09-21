/***************************************************************************
  Name:         imgarch.c
  Description:  Image archve
  Created:      20070820 15:55
  Copyright:    (C) 2007 by Kalle Kankare
  Email:        kalle.kankare@tut.fi

  **

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include <common/defines.h>
#include <common/iolet.h>
#include <common/llist.h>

#include "configvars.h"
#include "util.h"
#include "imgarch.h"

/***************************************************************************
  image_archive
 ***************************************************************************/

image_archive *image_archive_open(char *name, char *base)
{
  return NULL;
}

void image_archive_delete(image_archive *ima)
{

}

/***************************************************************************
  archive
 ***************************************************************************/

static int strendcasecmp(char *str, char *end)
{
  unsigned int elen=strlen(end);

  return strncasecmp(str+strlen(str)-elen,end,elen);
}


static const archive_type2 default_archives[] =
{
  {"CBZ/zip","unzip -q","\x50\x4B",0,(char * []){"zip","cbz"}},
  {"CBR/rar","unrar x","\x52\x61\x72\x21\x1A",0,(char * []){"rar","cbr"}},
  {"CBG/tar.gz","tar xz -O -f","\x1F\x8B",0,(char * []){"tar.gz","cbg"}},
  {"CBB/tar.bz2","tar xj -O -f","\x42\x5A\x68\x39\x31",0,
   (char * []){"tar.bz2","cbb"}},
  {NULL}
};

static linked_list *archiver_list=NULL;

tvalue archive_register_type(const archive_type2 *type)
{
  archive_type2 *data;

  if(!type)
    return FALSE;

  data=malloc(sizeof(archive_type2));
  memcpy(data,type,sizeof(archive_type2));

  linked_list_add_data(archiver_list,data);

  return TRUE;
}

static void archive_unregister_formats(void)
{
  for(linked_list_cell *beta=NULL;
      (beta=linked_list_cycle(archiver_list,beta)) != NULL;)
    nullify(beta->Data);

  linked_list_delete(archiver_list);
}

tvalue archive_register_default_formats()
{
  unsigned int beta;

  if(archiver_list)
    return TRUE;

  archiver_list=linked_list_create();

  for(beta=0;default_archives[beta].name != NULL;beta++)
    archive_register_type(&default_archives[beta]);

  atexit(archive_unregister_formats);

  return TRUE;
}

/* determines the type of an archive */
static archive_type2 *archive_get_type2(char *path)
{
  linked_list_cell *beta;
  archive_type2 *ret;
  size_t readlen=0,gamma;
  char *str;
  int fd,count;

  if(!path || !archiver_list)
    return NULL;

  /* get the longest magic+offset */
  for(beta=NULL;(beta=linked_list_cycle(archiver_list,beta)) != NULL;)
  {
    ret=beta->Data;
    gamma=ret->magic_offset+strlen(ret->magic);
    if(gamma > readlen)
      readlen=gamma;
  }

  str=malloc(readlen+1);
  memset(str,0,readlen);

  if((fd=open(path,O_RDONLY)) == -1)
    goto badend;

  if((count=read(fd,str,readlen)) == -1)
    goto badend;

  str[count]=0;

  for(beta=NULL;(beta=linked_list_cycle(archiver_list,beta)) != NULL;)
  {
    ret=beta->Data;

    if(ret->magic)
    {
      /* skip too short files */
      if(ret->magic_offset+strlen(ret->magic) > count)
        continue;

      /* check if the magic bytes are found */
      if(strncmp(str+ret->magic_offset,ret->magic,strlen(ret->magic)) == 0)
        break;
    }

    /* if there are no magic bytes for the format, try to match the suffix */
    //#error tänne näin
  }

  nullify(str);

  return ret;

 badend: 
  
  print_err("Error: Opening/reading file \"%s\" failed: %s\n",path,
    strerror(errno));

  nullify(str);

  return NULL;
}

tvalue archive_extract(char *arch_name,char *extract_path)
{
  archive_type2 *type;

  if(!arch_name || !extract_path)
    return FALSE;

  type=archive_get_type2(arch_name);

  if(!type)
  {
    print_err("Error: Determining type of file \"%s\" failed.\n",arch_name);
    return FALSE;
  }

  print_debug("file %s: tyyppi %s\n",arch_name,type->name);

  return TRUE;
}

/***************************************************************************
  tmpdir
 ***************************************************************************/

static tvalue tmpdir_initialized=FALSE;

static char *tmpdir_name(tvalue only_format);
static tvalue clean_dir(char *path,tvalue rm_dir, 
  tvalue (*prunefunc)(char *name));
static void tmpdir_deinit(void);
static int tmpdir_getpid(char *tmpdir);

/* initializes the tmpdir. if tmpdir_path is NULL creates a new 
   directory with a name. Otherwise deletes all files within the given 
   directory. */
char *tmpdir_init(char *tmpdir_path)
{
  /* check if the path is already created */
  if(tmpdir_path)
  {
    int filepid;
    struct stat st;

    /* assure the proper format of the given string */
    filepid=tmpdir_getpid(tmpdir_path);
    if(filepid != getpid())
    {
      print_err("Error: unknown tmpdir \"%s\"\n",tmpdir_path);
      return NULL;
    }

    if(stat(tmpdir_path,&st) == -1)
      goto badend;
 
   /* clean the directory */
    if(!clean_dir(tmpdir_path,FALSE,NULL))
    {
      print_err("Error: tmpdir reinit failed.\n");
      return NULL;
    }
    return tmpdir_path;
  }

  /* new tmpdir */
  if(!(tmpdir_path=tmpdir_name(FALSE)))
    goto badend;

  if(!create_directory(tmpdir_path))
    goto badend;

  print_debug("Luotiin [%s]\n",tmpdir_path);

  if(tmpdir_initialized == FALSE)
  {
    atexit(tmpdir_deinit);
    tmpdir_initialized=TRUE;
  }

  return tmpdir_path;

 badend: ;

  print_err("Error: Could not init tmp directory \"%s\"",tmpdir_path);

  if(errno)
    print_err(":%s",strerror(errno));
  print_err("\n");
  
  return NULL;
}

/* removes all unused temporary directories within the tmpdata dir */
static void tmpdir_deinit(void)
{
  tvalue tmpdir_filter(char *name)
  {
    int filepid,killret;

    filepid=tmpdir_getpid(name);

    if(filepid == 0)
      return TRUE;

    /* check if a process with the parsed pid exists */
    killret=kill(filepid,0);
    if((killret < 0 && errno == EPERM) || 
      (killret == 0 && filepid != getpid()))
      return TRUE;
    
    /* all things cleared up. delete the file */
    return FALSE;
  }

  clean_dir(sequ_config_generated_tmpfile_dir_path,FALSE,tmpdir_filter);
}

static unsigned int get_dec_len(unsigned int num)
{
  unsigned int ret;
  for(ret=0;(num /= 10) != 0;ret++) ;
  return ret;
}


/* generates a name for the temporary directory */
static const char *tmpdir_fmt="sqvtmp-%d";
static char *tmpdir_name(tvalue only_format)
{
  char *ret,*tmp;
  unsigned int length=strlen(sequ_config_generated_tmpfile_dir_path)+2;

  length+=(only_format) ? strlen(tmpdir_fmt) : 
    strlen(tmpdir_fmt)+get_dec_len(getpid());

  ret=malloc(length);

  snprintf(ret,length,"%s/%s",
    sequ_config_generated_tmpfile_dir_path,tmpdir_fmt);

  if(only_format)
    return ret;

  tmp=strdup(ret);
  snprintf(ret,length,tmp,getpid());
  nullify(tmp);

  return ret;
}

/* checks that the string is in the proper format and returns the 
   parsed pid. */
static int tmpdir_getpid(char *tmpdir)
{
  int filepid=0,count;
  char *format=tmpdir_name(TRUE);

  count=sscanf(tmpdir, format, &filepid);
  nullify(format);

  return filepid;
}

/* removes all files within a directory. 
   if rm_dir the directory itself is removed.
   if filter != NULL, it is executed for each filename. returns:
    true -> file is saved, false -> the file is deleted.

   if some files are saved and rm_dir is true -> this returns an error.
*/
static tvalue clean_dir(char *path,tvalue rm_dir, 
  tvalue (*filter)(char *name))
{
  char **files=NULL;
  struct stat st;

  if(!path)
    return FALSE;

  if(stat(path,&st) == -1)
    goto badend;

  if(S_ISDIR(st.st_mode) == FALSE)
  {
    errno=ENOTDIR;
    goto badend;
  }

  files=directory_list(path);
  if(!files)
    goto badend;
  
  for(unsigned int beta=0;files[beta] != NULL;beta++)
  {
    if(lstat(files[beta],&st) == -1)
      goto badend;

    /* if filter && not pruned -> dont delete */
    if(filter && filter(files[beta]))
      continue;

    if(S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode))
    {
      if(strendcasecmp(files[beta],"/.") == 0 ||
        strendcasecmp(files[beta],"/..") == 0)
        continue;

      /* no filter for subdirs */
      if(!clean_dir(files[beta],TRUE,NULL))
        goto badend;

      continue;
    }

    print_debug("unlink %s\n",files[beta]);

    if(unlink(files[beta]) == -1)
    {
      print_err("Error: Couldn't remove file \"%s\": %s\n",
        files[beta],strerror(errno));
      errno=0;

      goto badend;
    }
  }

  ptrarray_delete(files);

  if(rm_dir)
  {
    print_debug("rmdir %s\n",path);
    if(rmdir(path) == -1)
      goto badend;

  }

  return TRUE;

 badend: ;
  
  print_err("Error: Couldn't remove directory \"%s\"",path);
  if(errno)
    print_err(": %s",strerror(errno));
  print_err("\n");

  ptrarray_delete(files);

  return FALSE;
}
