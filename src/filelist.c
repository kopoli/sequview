/***************************************************************************
  Name:         filelist.c
  Description:  A list of filenames
  Created:      20060529
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

#include <ctype.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#include <fcntl.h>

#include <common/defines.h>
#include <common/iolet.h>
#include <common/check_failure.h>

#include "tmpfile.h"
#include "filelist.h"
#include "configvars.h"

#include "util.h"

/*****************************************************************************/

void file_list_delete(file_list *list)
{
  if(!list)
    return;

  nullify(list->path);

  if(list->files)
    ptrarray_delete(list->files);

  nullify(list);
}

inline unsigned int get_file_list_count(file_list *list)
{
  return (list) ? list->count : 0;
}

inline tvalue file_list_valid(file_list *list)
{
  return (list && list->path && list->files);
}

void file_list_print(file_list *list)
{
  register unsigned int beta=0;

  if(!list)
    return;

  print_debug("=====================\n");
  print_debug("File \"%s\":\n",list->path);
  if(list->directory)
    print_debug("Is a directory.\n");

  print_debug("List of files:\n");

  if(list->files)
    while(list->files[beta] != NULL)
    {
      print_debug("%d. %s\n",beta,list->files[beta]);
      beta++;
    } 

  print_debug("=====================\n");
}

/*****************************************************************************/

static tvalue path_is_a_directory(char *filename)
{
  struct stat buf;

  CHECK_FAILURE_WITH_ERRNO(stat(filename,&buf), -1,FALSE);

  return S_ISDIR(buf.st_mode);
}

/*****************************************************************************/

#define WANTED_FILETYPE_LONGEST_SUFF 5
#define WANTED_FILETYPE_SHORTEST_SUFF 4
static const char *wanted_filetypes[] =
{
  ".jpg",
  ".png",
  ".gif",
  ".tif",
  ".bmp",
  ".jpe",
  ".jpeg",
  ".tiff",
  NULL
};

static tvalue is_wanted_filename(char *str)
{
  register unsigned int beta=0;
  unsigned int slen,length;
  char buf[WANTED_FILETYPE_LONGEST_SUFF+1];

  length=strlen(str);

  if(length < WANTED_FILETYPE_SHORTEST_SUFF)
    return FALSE;

  /* ultimately chop the ending of the str and lowercase it */
  slen = (length >= WANTED_FILETYPE_LONGEST_SUFF) ? 
    WANTED_FILETYPE_LONGEST_SUFF : length;

  memset(buf,0,WANTED_FILETYPE_LONGEST_SUFF+1);
  memcpy(buf,str+length-slen,slen);

  for(beta=0;beta<WANTED_FILETYPE_LONGEST_SUFF+1;beta++)
    buf[beta]=tolower(buf[beta]);

  beta=0;

  /* compare to all known wanted types */
  while(wanted_filetypes[beta] != NULL)
  {
    slen=strlen(wanted_filetypes[beta]);

    if(slen <= length && 
       strncmp(buf+WANTED_FILETYPE_LONGEST_SUFF-slen,
        wanted_filetypes[beta],slen) == 0)
      return TRUE;

    beta++;
  }

  return FALSE;
}

/* excludes unwanted files from a filelist. returns a newly reallocated 
  filelist. In case of a failure returns NULL and original filelist is left 
  untouched. */
char **exclude_unwanted_files(char **files)
{
  register unsigned int beta=0,gamma=0;
  unsigned int length=0;
  char **ret;

  ARG_ASSERT(!files,NULL);

  while(files[beta] != NULL)
  {

    //print_debug("%s: testing filename [%s]\n",THIS_FUNCTION,files[beta]);

    /* if string is found and is in the end of files[beta] */
    if(is_wanted_filename(files[beta]) == TRUE)
      length++;

    beta++;
  }

print_debug("%s: final count of files %d\n",THIS_FUNCTION,length);

  CHECK_FAILURE_WITH_ERRNO(
    ret=malloc(sizeof(char *)*(length+1)), NULL,NULL
  );

  ret[length] = NULL;

  beta=0;
  while(files[beta] != NULL)
  {
    if(is_wanted_filename(files[beta]) == TRUE)
      ret[gamma++]=files[beta];
    else 
      nullify(files[beta]);

    beta++;
  }

  nullify(files);
  
  return ret;
}

/*****************************************************************************/



/* creates the filelist */
file_list *get_file_list(char *filename)
{
  file_list *ret;
  register unsigned int beta=0;

  ARG_ASSERT(!filename,NULL);

  CHECK_FAILURE_WITH_ERRNO(
    ret=malloc(sizeof(file_list)), NULL,NULL
  );

  memset(ret,0,sizeof(file_list));
  
  CHECK_FAILURE_WITH_ERRNO_ACT(
    ret->path=strdup(filename), NULL,
    file_list_delete(ret);
    return NULL;
  );

  ret->directory=FALSE;
  ret->atype=NULL;

  /* if a directory */
  if(path_is_a_directory(filename) == TRUE)
  {
    ret->directory=TRUE;

    CHECK_FAILURE_ACT(
      ret->files=directory_list(filename),NULL,
        file_list_delete(ret);
        return NULL;
    );

  }
  else
  {
    if((ret->atype=archive_get_type(filename)) == NULL)
    {
      print_err("Error: The file format is unknown.\n");
      file_list_delete(ret);      
      return NULL;
    }

    /* get the filelist */
    CHECK_FAILURE_ACT(
      ret->files=archive_get_list(filename,ret->atype),
      NULL,
        file_list_delete(ret);
        return NULL;
    );
  }

  /* exclude unwanted */
  {
    char **tmp;
    CHECK_FAILURE_ACT(
      tmp=exclude_unwanted_files(ret->files),
      NULL,
        file_list_delete(ret);
        return NULL;
    );

    ret->files=tmp;

    /* process archive filenames */
    if(!ret->directory)
    {
      CHECK_FAILURE_ACT(
        archive_process_filenames(ret->files,ret->atype),FALSE,
          file_list_delete(ret);
          return NULL;
      );
    }
  }

  /* sort the list */
  {
    inline int scmp(const void *e, const void *t)
    { return strcasecmp(*(char **)e,*(char **)t); }

    beta=0;
    while(ret->files[beta] != NULL)
      beta++;

    qsort(ret->files,beta,sizeof(char *),scmp);
  }
  /* remember the number of files */
  ret->count=beta;

  return ret;
}


/*****************************************************************************/


char *file_list_get_file(file_list *list,unsigned int pos)
{
  ARG_ASSERT(!list,NULL);

  if(list->count <= pos)
    return NULL;

  /* if directory */
  if(list->directory == TRUE)
    return list->files[pos];

  /* if archive */
  if(!archive_get_file(list->path,list->atype,
       list->files[pos],tmpfile_filename))
    return NULL;

  return tmpfile_filename;
}

char *file_list_get_filename(file_list *list,unsigned int pos)
{
  ARG_ASSERT(!list,NULL);

  if(list->count <= pos)
    return NULL;

  return list->files[pos];
}
