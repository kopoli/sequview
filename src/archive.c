/***************************************************************************
  Name:         archive.c
  Description:  Interaction with archivers
  Created:      20060618
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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <common/iolet.h>
#include <common/check_failure.h>

#include "archive.h"
#include "configvars.h"

#include "util.h"
/*

Description:
-recognize and decompress archives
-list the contents of an archive
-processing of a list of filenames

*/

const archive_cmd archive_default_cmds[] =
{
  {"unzip -Z -1", "unzip -p -C", ARCHIVE_CMD_ESCAPE_WILDCARDS},
  {"unrar vb", "unrar -inul -- p",0},
  {"tar tzf", "tar xz -O -f",0},
  {"tar tjf", "tar xj -O -f",0},
  {NULL,NULL,0}
};

const archive_type archive_supported_formats[] =
{
  {"zip",(char []){0x50, 0x4B},2},
  {"rar",(char []){0x52, 0x61, 0x72, 0x21, 0x1A},5},
  {"tar.gz",(char []){0x1F, 0x8B},2},
  {"tar.bz2",(char []){0x42, 0x5A, 0x68, 0x39, 0x31},5},
  {NULL,NULL,0}
};

/*
const unsigned int archive_command_count = 
 sizeof(archive_supported_formats)/sizeof(archive_type);
*/

/* gets the archive type from the given archive file */
const archive_type *archive_get_type(char *archive)
{
  unsigned int beta,len,maxmagic=0;
  int fd;
  ssize_t rlen;
  char *buffer;
  const archive_type *ret=NULL;

  ARG_ASSERT(!archive,NULL);

  /* calculate the number of supported formats and get the maximum and
     minimum lengths of a magic-byte string. */
  for(len=0;archive_supported_formats[len].name != NULL;len++)
    if(maxmagic < archive_supported_formats[len].magic_length)
      maxmagic=archive_supported_formats[len].magic_length;

  /* read the first bytes of the archive */
  buffer=malloc(maxmagic+1);

  CHECK_FAILURE_WITH_ERRNO(
    fd=open(archive,O_RDONLY),-1,FALSE
  );

  CHECK_FAILURE_WITH_ERRNO_ACT(
    rlen=read(fd,buffer,maxmagic) ,-1,
      close(fd);
      nullify(buffer);
      return NULL;
  );

  close(fd);

  /* assumption: there should not be so small (usable) archives */
  if(rlen < maxmagic)
  {
    nullify(buffer);
    return NULL;
  }

  buffer[maxmagic]=0;

  /* compare the buffer to known formats */
  for(beta=0;beta<len;beta++)
    if(memcmp(buffer,archive_supported_formats[beta].magic,
      archive_supported_formats[beta].magic_length) == 0)
      ret=&archive_supported_formats[beta];

  nullify(buffer);

  return ret;
}

/* execs a command and pipes the (stdout) output into the returned 
  filedescriptor. If tofile is != NULL then output is redirected to 
  it and it's fd returned. */
static int exec_to_pipe(char **cmdlist, char *tofile)
{
  pid_t pidi;
  int fd[2];

  
  ARG_ASSERT(!cmdlist,FALSE);

  print_debug("%s: suoritetaan komento [",THIS_FUNCTION);
  for(int beta=0;cmdlist[beta] != NULL;beta++)
    print_debug("%s ",cmdlist[beta]);
  print_debug("]\n");
        
  /* create the needed pipe */
  if(tofile == NULL)
  {
    CHECK_FAILURE_WITH_ERRNO_ACT(
      pipe(fd),-1,
        return -1;
    );
  }
  /* write to a file */
  else
  {
    CHECK_FAILURE_WITH_ERRNO_ACT(
      fd[0]=open(tofile,O_RDWR|O_TRUNC|O_CREAT,0600),-1,
        return -1;
    );
    fd[1]=fd[0];
  }

  CHECK_FAILURE_WITH_ERRNO_ACT(
    pidi = fork(),-1,
      return -1;
  );

  /* the child process */
  if(pidi == 0)
  {
    //setenv("LC_CTYPE","C",1); 

    if(tofile == NULL)
    {
      CHECK_FAILURE_WITH_ERRNO_ACT(
        close(fd[0]) == -1,TRUE,
          close(fd[1]);
          _exit(1);
      );
    }

    /* close the reading end of the pipe and stdout */
    CHECK_FAILURE_WITH_ERRNO_ACT(
      close(1) == -1, TRUE,
        close(fd[1]);
        _exit(1);
    );

    /* duplicate the write end of the pipe as the new stdout */
    CHECK_FAILURE_WITH_ERRNO_ACT(
      dup2(fd[1],1), -1,
        close(fd[1]);
        _exit(1);
    );        

    close(fd[1]);

    CHECK_FAILURE_WITH_ERRNO_ACT(
      execvp(*cmdlist,cmdlist), -1,
        _exit(1);
    );

  }

  if(tofile == NULL)
    close(fd[1]);

  return fd[0];
}

/* "converts" archive_type to archive_cmd */
static archive_cmd *cmd_from_type(char *archive, const archive_type *type)
{
  unsigned int beta;

  if(archive && !type && 
    (type=archive_get_type(archive)) == NULL)
    return NULL;

  beta=type-archive_supported_formats;
  if(beta > sizeof(archive_supported_formats))
    return NULL;

  return &sequ_config_archive_cmds[beta];
}

/* Returns a filelist from the given archive. If type != NULL, then
   the archive is assumed to be in the given format. */
char **archive_get_list(char *archive, const archive_type *type)
{
  int status;
  unsigned int beta;
  char **list;
  int fd,bytes=0;

  char buf[256];
  const unsigned int buflen=sizeof(buf);

  char *data=NULL;

  archive_cmd *cmd;

  ARG_ASSERT(!archive,NULL);

  /* get the archive type */
  cmd=cmd_from_type(archive,type);
  if(!cmd)
    return NULL;

  /* generate the stringlist for execv */
  list=stringlist_create(cmd->list,' ',1);

  for(beta=0;list[beta]!=NULL;beta++) ;

  list[beta]=strdup(archive);
  list[beta+1]=NULL;


  /* call the decompress program */
  fd=exec_to_pipe(list,NULL);

  ptrarray_delete(list);

  if(fd == -1)
    return NULL;

  /* gather the output to a string */
  beta=0;
  while(1)
  {
    if((bytes=read(fd,buf,buflen)) == -1 && errno != EINTR)
    {
      print_err("Error: Could not read from a pipe: %s\n",strerror(errno));
      nullify(data);
      close(fd);
      return NULL;
    }

    if(bytes <= 0)
      break;

    data=realloc(data,beta+bytes+1);
    memcpy(data+beta,buf,bytes);

    beta+=(unsigned)bytes;
  }

  /* skip the last \n */
  data[beta-1]=0;
  close(fd);

  /* chop the string */
  list=stringlist_create(data,'\n',0);
  nullify(data);

  waitpid(-1,&status,WNOHANG);

  if(WIFEXITED(status) && WEXITSTATUS(status) != 0)
    print_err("Error: The decompressor returned %d. File might be corrupt.\n",
      WEXITSTATUS(status));
    
  /*
  //DEBUG
  if(list)
    for(beta=0;list[beta] != NULL;beta++)
      print_debug("%d. str [%s] @ %p\n",beta,list[beta],list[beta]);
  */

  return list;
}

#include <sys/time.h>

/* gets the file with name name and writes it to file tofile. Same rules 
   apple with type as in function archive_get_list. */
tvalue archive_get_file(char *archive,const archive_type *type,
  char *name,char *tofile)
{
  unsigned int beta;
  char **list;
  int fd,status;
  archive_cmd *cmd;
  pid_t child;

  struct timeval tv1,tv2;
  double t1,t2;

  ARG_ASSERT(!archive || !name,FALSE);

  /* get the archive type */
  cmd=cmd_from_type(archive,type);
  if(!cmd)
    return FALSE;

  list=stringlist_create(cmd->decompress,' ',2);

  for(beta=0;list[beta]!=NULL;beta++) ;

  list[beta]=strdup(archive);
  list[beta+1]=strdup(name);
  list[beta+2]=NULL;

  gettimeofday(&tv1,NULL);

  /* wait for previous processes */
  wait(NULL);

  /* call the decompress program */
  fd=exec_to_pipe(list,tofile);

  print_debug("%s: tiedoston fd on %d\n",THIS_FUNCTION,fd);

  ptrarray_delete(list);

  if(fd == -1)
    return FALSE;  

  /* wait for the decompression */
  if((child=wait(&status)) == -1)
    return FALSE;

  close(fd);


  gettimeofday(&tv2,NULL);

  t1=tv1.tv_sec+(tv1.tv_usec/1000000.0);
  t2=tv2.tv_sec+(tv2.tv_usec/1000000.0);
  print_debug("%s: Lukeminen kesti %f s\n",THIS_FUNCTION,t2-t1);
  print_debug("%s: pidit par %d child %d\n",THIS_FUNCTION,getpid(),child);


  if(WIFEXITED(status) == TRUE && WEXITSTATUS(status) != 0)
  {
    print_err("Error: The decompression failed.\n");
    return FALSE;
  }

  return TRUE;
}

/************************************************************************/

static const char wildcard_chars[] = {']','[','\\','*','?',0};

/* infozip's unzip supports wildcards and only literal names are queried */
static char *escape_wildcards(char *str)
{
  register unsigned int beta,gamma,count=0;
  tvalue rege=FALSE;
  char *ret;

  if(!str)
    return NULL;

  /* count the number of regexp characters in the string */
  for(beta=0;str[beta] != 0;beta++)
  {
    for(gamma=0;wildcard_chars[gamma] != 0;gamma++)
      if(str[beta]==wildcard_chars[gamma])
        count++;
  }

  if((ret=malloc(beta+count+1)) == NULL)
    return NULL;

  count=0;

  /* add the escapes to a copy of the original string */
  for(beta=0;str[beta] != 0;beta++)
  {
    rege=FALSE;
    for(gamma=0;wildcard_chars[gamma] != 0;gamma++)
      if(str[beta]==wildcard_chars[gamma])
      {
        ret[count++]='\\';
        ret[count++]=str[beta];
        rege=TRUE;
      }

    if(rege == FALSE)
      ret[count++]=str[beta];
  }

  ret[count]=0;

  nullify(str);
  return ret;
}

static char *process_filename(char *filename,unsigned int flags)
{
  char *ret=filename;

  if(flags & ARCHIVE_CMD_ESCAPE_WILDCARDS)
    ret=escape_wildcards(ret);

  return ret;
}

/* process all given filenames according to the type */
tvalue archive_process_filenames(char **files,const archive_type *type)
{
  archive_cmd *cmd;
  unsigned int beta;
  
  ARG_ASSERT(!files||!type,FALSE);

  cmd=cmd_from_type(NULL,type);

  for(beta=0;files[beta] != NULL;beta++)
    files[beta]=process_filename(files[beta],cmd->flags);

  return TRUE;
}
