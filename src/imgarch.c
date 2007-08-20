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

image_archive *image_archive_read(char *name, char *base)
{
  return NULL;
}

void image_archive_delete(image_archive *ima)
{

}


/* tmpdir */

static tvalue tmpdir_initialized=FALSE;

static char *tmpdir_name(tvalue only_format);
static tvalue clean_dir(char *path,tvalue rm_dir, 
  tvalue (*prunefunc)(char *name));
static void tmpdir_deinit(void);

char *tmpdir_init(char *tmpdir_path)
{
  /* check if the path is already created */
  if(tmpdir_path)
  {
    struct stat st;

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

  if(mkdir(tmpdir_path,0755) == -1)
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


static void tmpdir_deinit(void)
{
  tvalue tmpdir_filter(char *name)
  {
    int filepid,count,killret;

    char *format=tmpdir_name(TRUE);

    /* make sure that the file is the appropriate format */
    count=sscanf(name, format, &filepid);

    nullify(format);

    if(count <= 0)
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

const char *tmpdir_fmt="sqvtmp-%d";
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

static int strendcmp(char *str, char *end)
{
  unsigned int elen=strlen(end);

  return strncmp(str+strlen(str)-elen,end,elen);
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
      if(strendcmp(files[beta],"/.") == 0 ||
        strendcmp(files[beta],"/..") == 0)
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
