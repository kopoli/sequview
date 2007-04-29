/***************************************************************************
  Name:         gen_cli.h
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

#ifndef PR_GEN_CLI_HEADER
#define PR_GEN_CLI_HEADER

#include "getopt_clone.h"

/****************************************************************************
  Defines / Structures / data types
 ****************************************************************************/

typedef struct gen_cli_helpstr
{
	char *desc,*arg;
} gen_cli_helpstr;

typedef struct gen_cli_argument
{
  /* the command */
  char *cmd;

  /* the command's parameters. For clarification purposes only */
  char *helpcmdparameter;

  /* possible argument after the command/options part. */
  char *helpcmdextra;

  /* the flags for current command */
  struct option_clone *options;

  /* subcommands */
  struct gen_cli_argument **subcmds;
  unsigned int subcmdcount;

  /* the helptext for the command */
  char *cmdhelp;

  /* the helptext for the options/flags */
  gen_cli_helpstr *helptext;

  /* previous argument */
  struct gen_cli_argument *prev;

  /*
    The handling function of the parsed arguments. pos is the position of 
    supplied option in options or -1 if the argument is not an option. 
    parsefunc should return a negative value in case of error and
    positive otherwise.
   */
  int (*parsefunc)(int pos,int argc,char **argv);

} gen_cli_argument;


//extern gen_cli_argument *PR_CliArgs;

/****************************************************************************
  Prototypes
 ****************************************************************************/

/* Recursive help */
tvalue gen_cli_print_help(char *PR_ProgramName,gen_cli_argument *arg);

/* Handle the command-line. This function returns the negative value supplied by
  a parsefunc or 0 if something went wrong in the function. It returns the last
  positive return-value of a parsefunc when successful. If no arguments were 
  given this will return 1. */
int gen_cli_parse_args(gen_cli_argument *arg,int argc, char ** argv);


#endif
