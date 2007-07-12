/***************************************************************************
  Name:         util.c
  Description:  Utilities
  Created:      20060525
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <dirent.h>

#include <common/iolet.h>
#include <common/check_failure.h>

/* deletes all kinds of null-terminated pointer arrays */
void ptrarray_delete(void *ptr_list)
{
  unsigned int beta=0;
  void **list;

  if(!ptr_list)
    return;

  list=(void **)ptr_list;

  while(list[beta] != NULL)
    free(list[beta++]);

  free(list);
}

char **stringlist_create(char *str,char delim, unsigned int extra_space)
{
  unsigned int count,len,beta,gamma,pos=0;
  char **ret;

  if(!str)
    return NULL;

  for(count=0,len=0;str[len] != 0;len++)
    if(str[len] == delim)
      count++;

  ret=malloc((count+2+extra_space)*sizeof(char*));

  for(beta=0;beta<=len;beta=gamma+1)
    for(gamma=beta;gamma<=len+1;gamma++)
      if(gamma == len+1 || str[gamma] == delim)
      {
        ret[pos++]=strndup(str+beta,gamma-beta);
        break;
      }

  ret[pos]=NULL;

  return ret;
}


char **directory_list(char *path)
{
  unsigned int beta,count,pathlen;
  DIR *dir;
  struct dirent *dent=NULL;
  char **ret;

  ARG_ASSERT(!path,NULL);

  CHECK_FAILURE_WITH_ERRNO(
    dir=opendir(path),NULL,
    NULL
  );

  /* count the number of files in the directory */
  for(count=0;(dent=readdir(dir)) != NULL;count++) 
    ;

  rewinddir(dir);

  /* get the list */
  ret=malloc((count+1)*sizeof(char*));

  pathlen=strlen(path);

  for(beta=0;beta<count;beta++)
  {
    dent=readdir(dir);

    if(!dent)
      break;

    /* construct the full path filename */
    ret[beta]=malloc(pathlen+1+strlen(dent->d_name)+1);
    memcpy(ret[beta],path,pathlen);
    ret[beta][pathlen]='/';
    strcpy(ret[beta]+pathlen+1,dent->d_name);
  }

  ret[beta]=NULL;

  closedir(dir);

  return ret;
}
