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

#include <string.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include <common/defines.h>
#include <common/iolet.h>
#include <common/llist.h>

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

static tvalue clean_dir(char *path,tvalue rm_dir);

static linked_list *tmpdirs=NULL;

char *tmpdir_init(char *tmpdir_path)
{
  clean_dir("poista",FALSE);
  return NULL;
}

static int strendcmp(char *str, char *end)
{
  unsigned int elen=strlen(end);

  return strncmp(str+strlen(str)-elen,end,elen);
}

/* removes all files within a directory. 
   if rm_dir the directory itself is removed. */
static tvalue clean_dir(char *path,tvalue rm_dir)
{
  char **files=NULL;
  struct stat st;

  if(!path)
    return FALSE;

  if(stat(path,&st) == -1)
    goto badend;

  if(S_ISDIR(st.st_mode) == FALSE)
    goto badend;

  files=directory_list(path);
  if(!files)
    goto badend;
  
  for(unsigned int beta=0;files[beta] != NULL;beta++)
  {
    if(lstat(files[beta],&st) == -1)
      goto badend;

    if(S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode))
    {
      if(strendcmp(files[beta],"/.") == 0 ||
        strendcmp(files[beta],"/..") == 0)
        continue;

      if(!clean_dir(files[beta],TRUE))
        goto badend;

      continue;
    }

    print_debug("unlink %s\n",files[beta]);
  }

  ptrarray_delete(files);

  if(rm_dir)
  {
    print_debug("rmdir %s\n",path);
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
