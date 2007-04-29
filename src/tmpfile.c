/***************************************************************************
  Name:         tmpfile.c
  Description:  Temporary file management
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include <common/iolet.h>
#include <common/check_failure.h>

#include "configvars.h"
#include "util.h"

char *tmpfile_filename = NULL;

//static int sequ_tmpfile_fd=-1;
static tvalue tmpfile_inited=FALSE;

static unsigned int get_uint_decimal_length(unsigned int num)
{
  register unsigned int beta=1;

  while((num /= 10) != 0) beta++;
  return beta;
}

/* the format of the temporary file's name is approximately
   tmpdir/tmpdatasv.pid.tmp. if the argument fmtscanf is 0
   the function generate's a filename by the aforementioned format.
   otherwise it returns a generic scanf-format for parsing the pid from
   a filename. */
static char *generate_tmpname(tvalue fmtscanf)
{
  unsigned int length=0,pos;
  char *ret;

  length+=strlen(sequ_config_generated_tmpfile_dir_path)+1;
  length+=strlen(sequ_config_tmpfile_dir);
  length+=strlen(SEQU_CFG_TMPFILE_NAME);
  pos=length+1;
  if(fmtscanf == 0)
    length+=get_uint_decimal_length(getpid())+1;
  else
    length+=3;
  length+=strlen(SEQU_CFG_TMPFILE_SUFFIX)+1;

  CHECK_FAILURE_WITH_ERRNO(ret=malloc(length),NULL,NULL);

  /* write the string */
  snprintf(ret,length,"%s/%s%s.",sequ_config_generated_tmpfile_dir_path,
    sequ_config_tmpfile_dir,SEQU_CFG_TMPFILE_NAME);

  /* generate the tempfilename for this process */
  if(fmtscanf == 0)
    snprintf(ret+pos,length,"%d",getpid());
  else
    snprintf(ret+pos,length,"%%d");
  strcat(ret,SEQU_CFG_TMPFILE_SUFFIX);
  
  print_debug("Generoitiin nimi [%s]\n",ret);

  return ret;
}

/* deletes the temporary files from the tmpdir */
tvalue tmpfile_clean_directory()
{
  unsigned int beta;
  char **files;
  char *fmt;
  int filepid,count,killret;

  print_debug("etsit‰‰n paskaa kansiosta: %s\n",
    sequ_config_generated_tmpfile_dir_path);

  files=directory_list(sequ_config_generated_tmpfile_dir_path);
  if(!files)
    return FALSE;

  fmt=generate_tmpname(TRUE);
  if(!fmt)
    return FALSE;

  for(beta=0;files[beta] != NULL;beta++)
  {
    /* make sure that the file is the appropriate format */
    count=sscanf(files[beta], fmt, &filepid);

    if((count) <= 0)
      continue;

    /* check if a process with the parsed pid exists */
    killret=kill(filepid,0);
    if((killret < 0 && errno == EPERM) || 
      (killret == 0 && filepid != getpid()))
      continue;    

    print_debug("poistetaan tiedosto: %s\n",files[beta]);

    /* if it does not, remove the file */
    CHECK_FAILURE_WITH_ERRNO_ACT(
        unlink(files[beta]), -1,
          nullify(fmt);
          ptrarray_delete(files); 
          return FALSE;
    );    
  }

  nullify(fmt);
  ptrarray_delete(files);

  return TRUE;
}

/* ends the tmpfile subsystem */
static void tmpfile_deinit()
{
  if(tmpfile_inited == FALSE)
    return;

  tmpfile_clean_directory();

  nullify(tmpfile_filename);
  tmpfile_inited = FALSE;
  //  sequ_tmpfile_fd = -1;
}

/* starts the tmpfile subsystem */
tvalue tmpfile_init()
{
  if(tmpfile_inited == TRUE)
    return TRUE;

  CHECK_FAILURE(
    tmpfile_filename=generate_tmpname(FALSE), 
      NULL, FALSE
  );

  /*
  CHECK_FAILURE_ACT(
    truncate(tmpfile_filename,0),FALSE,
      nullify(tmpfile_filename);
      return FALSE;
  );
  */

  atexit(tmpfile_deinit);

  tmpfile_inited = TRUE;
  return TRUE;
}


#if 0
  CHECK_FAILURE_ACT(
    tmpfile_trunc(tmpfile_filename),FALSE,
      nullify(tmpfile_filename);
      return FALSE;
  );
#endif

#if 0
/* creates or truncates a file by filename */
static tvalue tmpfile_trunc(char *filename)
{
  int fd;
  ARG_ASSERT(!filename,FALSE);

  /* just truncate the file if it is not open */
  if(sequ_tmpfile_fd != -1)
  {
    CHECK_FAILURE_WITH_ERRNO(
      ftruncate(sequ_tmpfile_fd,0), 
      -1, FALSE
    );
  }

  CHECK_FAILURE_WITH_ERRNO(
    fd=open(filename,O_CREAT|O_TRUNC|O_RDWR,0600),
    -1,FALSE
  );

  sequ_tmpfile_fd=fd;

  return TRUE;
}
#endif

#if 0
  /* lock the file, so that multiple instances won't delete the tmpfile */
  fl.l_type=F_WRLCK;
  fl.l_whence=SEEK_SET;
  fl.l_start=0;
  fl.l_len=0;

  if(fcntl(fd,F_SETLK,&fl) == -1)
  {
    print_err("Error: Could not lock tmpfile: \"%s\" because: \"%s\"\n",
      filename,strerror(errno));
    return FALSE;
  }
#endif


#if 0
  /* deal with the tmpfile of this instance of the program */
  if(sequ_tmpfile_fd != -1)
  {
    struct flock fl;

    /* freeing the lock from own tmpfile */
    fl.l_type=F_UNLCK;
    fl.l_whence=SEEK_SET;
    fl.l_start=0;
    fl.l_len=0;
  
    if(fcntl(sequ_tmpfile_fd,F_SETLK,&fl) == -1)
    {
      print_err("Error: Could not free lock from tmpfile: \"%s\""
        " because: \"%s\"\n", tmpfile_filename,strerror(errno));
      nullify(tmppath);
      return FALSE;
    }
    close(sequ_tmpfile_fd);
  }
  else
    print_err("Error: Could not open tmpfile: %s for unlocking.\n",
      tmpfile_filename);

  /* clear the tmpfile directory, unlink those which are not locked */
  for(beta=0;beta<num;beta++)
  {
    if(fnmatch("*" SEQU_CFG_TMPFILE_SUFFIX, files[beta]->d_name, 0) == 0)
    {
      strcpy(tmppath,sequ_config_generated_tmpfile_dir_path);
      strcat(tmppath,"/");
      strcat(tmppath,files[beta]->d_name);

      //testi‰
      {
        int count,leni,pidi=0;

        char *str=SEQU_CFG_TMPFILE_NAME ".%d" 
          SEQU_CFG_TMPFILE_SUFFIX;

        char *fmt=generate_tmpname(TRUE);

        count=sscanf(tmppath,fmt, &pidi);

        print_debug("Sscanf: count %d pidi %d ja format [%s]\n",count,pidi,
          fmt);

        nullify(fmt);
      }


      if((fd=open(tmppath, O_RDWR)) != -1)
      {
        struct flock fip;
        fip.l_type=F_WRLCK;
        fip.l_whence=SEEK_SET;
        fip.l_start=0;
        fip.l_len=0;

        if(fcntl(fd,F_GETLK,&fip) == -1)
        {
          print_err("Error: querying lock from tmpfile: \"%s\""
            " errno: \"%s\"\n", tmppath,strerror(errno));
          close(fd);
          nullify(tmppath);
          ptrarray_delete(files,num);
          return FALSE;
        }

print_debug("Lukitus on tyyppi‰: %d\n",fip.l_type);
 
        close(fd);
        if(fip.l_type != F_UNLCK)
        {
print_debug("Could not unlock file: %s\n",tmppath);
          continue;
        }
      }
      /* ignore if the file could not be opened */
      else
      {
print_debug("ERRRRORORRI: Could not open() file %s\n",tmppath);
        continue;
      }
  
print_debug("poistetaan tiedosto: %s\n",tmppath);

      CHECK_FAILURE_WITH_ERRNO_ACT(
        unlink(tmppath), -1,
          nullify(tmppath);
          ptrarray_delete(files,num); 
          return FALSE;
      );

    }
    nullify(files[beta]);
  }
#endif
