/***************************************************************************
  Name:         gen_cli.c
  Description:  a generic command-line interface
  Created:      Sat Nov  6 17:09:45 EET 2004
  Copyright:    (C) 2004 by Kalle Kankare
  Email:        kalle.kankare@tut.fi
 ***************************************************************************/

/***************************************************************************

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

#include "commonconf.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "iolet.h"
#include "useful.h"

#include "gen_cli.h"

/***************************************************************************
  Functions
 ***************************************************************************/

/* parses the arguments. */
int gen_cli_parse_args(gen_cli_argument *arg,int argc, char ** argv)
{
  register unsigned int beta;
  int ret,pret=1;

  ARG_ASSERT(!arg || argc < 0 || !argv,0);

  if(argc > 1)
  {
    /* recursively call this function with new data */
    for(beta=0;beta<arg->subcmdcount;beta++)
      if(strcmp(argv[1],arg->subcmds[beta]->cmd) == 0)
        return gen_cli_parse_args(arg->subcmds[beta],argc-1,&argv[1]);
  }

  beta=1;

  while(1)
  {
    ret=getopt_clone(argc,argv,arg->options);
    if(ret == -1)
      break;
    else if(ret == -2)
      return 0;

    /* the handling of the commands */
    if((pret=arg->parsefunc((ret<0) ? -1 : ret,optarg_clone_pos,argv)) < 0)
      return pret;

    beta++;
  }

  return pret;
}


/* prints chopped lines into stdout. printpos is the position which printing 
  will be started from in each line. This assumes that the first line has 
  already been fixed into position. columns is the width of the terminal */
static void print_explain(unsigned int printpos, char *text, 
  unsigned int columns)
{
  register unsigned int beta = 0;
  unsigned int begin = 0, end = 0, length=strlen(text), pos = 0;

  while(1)
  {
    /* if the text is too long for one terminal line */
    if(length-begin > columns-printpos)
      while(end < length)
      {
        /* search the last space */
        if(isspace(text[end]) && end < columns+begin-printpos)
          pos = end+1;

        if(end >= columns+begin-printpos)
          break;

        end++;
      }
    /* if this fits then bloat away. */
    else
      end = length;

    /* if some progress was made */
    if(begin < pos)
      end = pos;

    /* the printing */
    iolet_out_stream(IL_IOStd,text+begin,end-begin);
    iolet_out_char(IL_IOStd,'\n');

    /* check if there is data left in the pointer */
    if(end < length)
      for(beta = 0; beta < printpos; beta++)
        iolet_out_char(IL_IOStd,' ');
    else
      break;

    pos = begin = end;
  }
}

#define PRINTSPACE "  "
#define PRINTSPACELEN 2

//rather complex, but saves time when adding and removing flags
/* Prints the helptext for options in a generated form */
static tvalue print_flags(struct option_clone *opts,struct gen_cli_helpstr *opts_explain,
  unsigned int arglen,unsigned int cols)
{
  register unsigned int beta=0, gamma;
  unsigned int optcount = 0, curlen;
  unsigned int conslen = PRINTSPACELEN+2;

  while(opts[optcount].longflag != NULL || opts[optcount].has_arg != 0
   || opts[optcount].shortflag != 0)
    optcount++;

  /* do the printing */
  for(beta = 0; beta < optcount; beta++)
  {
    curlen = conslen;

    print_out(PRINTSPACE);
    if(opts[beta].shortflag != 0)
      print_out("-%c", opts[beta].shortflag);

    if(opts[beta].longflag != NULL)
    {
      curlen += 4 + strlen(opts[beta].longflag);

      if(opts[beta].shortflag != 0)
        iolet_out_char(IL_IOStd,',');

      else
        print_out("   ");

      print_out(" --%s", opts[beta].longflag);

      if(opts_explain[beta].arg != NULL)
      {
        curlen += 1 + strlen(opts_explain[beta].arg);
        print_out(" %s", opts_explain[beta].arg);
      }
    }

    for(gamma = curlen; gamma < arglen; gamma++)
      iolet_out_char(IL_IOStd,' ');
  
    print_explain(arglen, opts_explain[beta].desc,cols);
  }

  iolet_out_char(IL_IOStd,'\n');

  return TRUE;
}

static unsigned int getprintpos(gen_cli_argument *arg)
{
  register unsigned int beta = 0;
  struct option_clone *opts=arg->options;
  struct gen_cli_helpstr *opts_explain=arg->helptext;
  unsigned int arglen = 0, optcount = 0, curlen;
  unsigned int conslen = PRINTSPACELEN+2;

  while(opts[optcount].longflag != NULL || opts[optcount].has_arg != 0
   || opts[optcount].shortflag != 0)
    optcount++;

  //check the maximum length of the flag (with its argument)
  for(beta = 0; beta < optcount; beta++)
  {
    curlen = conslen;

    if(opts[beta].longflag != NULL)
      curlen += 4 + strlen(opts[beta].longflag);

    if(opts_explain[beta].arg != NULL)
      curlen += 1 + strlen(opts_explain[beta].arg);

    if(curlen > arglen)
      arglen = curlen;
  }

  /* check the lengths of the commands (it is doubtful that they are longer 
     than the flags) */
  for(beta=0;beta<arg->subcmdcount;beta++)
  {
    curlen = conslen + strlen(arg->subcmds[beta]->cmd);

    if(curlen > arglen)
      arglen = curlen;
  }

  arglen+=conslen;

  return arglen;
}

static tvalue print_command_path(char *PR_ProgramName,gen_cli_argument *arg)
{
  register gen_cli_argument *beta=arg;
  register unsigned int gamma=0;
  unsigned int count=0,constcount;
  char **cmds=NULL;
  
  if(!arg)
    return FALSE;

  /* count the number of supcommands */
  while(beta != NULL && beta->prev != NULL)
  {
    count++;
    beta=beta->prev;
  }

  cmds=use_malloc(count*sizeof(char *));
  if(!cmds)
  {
    print_err("Error: Ran out of memory in helpprinting.\n");
    return FALSE;
  }

  constcount=count;

  /* copy the pointers */
  beta=arg;
  while(count > 0)
  {
    count--;
    cmds[count] = beta->cmd;
    beta=beta->prev;
  }

  print_out("%s ",PR_ProgramName);
  for(gamma=0;gamma<constcount;gamma++)
    print_out("%s ",cmds[gamma]);

  if(arg->helpcmdparameter != NULL)
    print_out("%s",arg->helpcmdparameter);

  nullify(cmds);
  return TRUE;  
}

/* Recursive help */
tvalue gen_cli_print_help(char *PR_ProgramName,gen_cli_argument *arg)
{
  unsigned int arglen;
  unsigned int rows,cols;

  ARG_ASSERT(!PR_ProgramName || !arg,FALSE);

  print_out("\nUsage: ");

  if(arg->cmd != NULL)
    print_command_path(PR_ProgramName,arg);
  else
    print_out("%s",PR_ProgramName);

  if(arg->subcmdcount > 0)
    print_out(" <command>");
  if(arg->options != NULL)
    print_out(" <options>");

  if(arg->helpcmdextra != NULL)
    print_out(" %s",arg->helpcmdextra);

  print_out("\n\n");
  get_win_size(&rows,&cols);
  print_explain(0,arg->cmdhelp,cols);
  print_out("\n\n");

  arglen = getprintpos(arg);

  /* print short descriptions of the subcommands */
  if(arg->subcmdcount != 0)
  {
    register unsigned int beta,gamma;
    unsigned int curlen=0;

    if(arg->cmd != NULL)
      print_out("Subc");
    else
      print_out("C");
    print_out("ommands:\n");

    for(beta=0;beta<arg->subcmdcount;beta++)
    {
      curlen=PRINTSPACELEN + strlen(arg->subcmds[beta]->cmd);

      print_out(PRINTSPACE "%s",arg->subcmds[beta]->cmd);

      for(gamma = curlen; gamma < arglen; gamma++)
        print_out(" ");

      print_explain(arglen-1,arg->subcmds[beta]->cmdhelp,cols);

    }

    print_out("\n");
  }

  /* print out the flags */
  print_out("Options:\n");
  print_flags(arg->options,arg->helptext,arglen,cols);

  return TRUE;
}
