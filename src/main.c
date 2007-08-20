/***************************************************************************
  Name:         main.c
  Description:  The main program.
  Created:      20060621
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

#include <locale.h>
#include <stdio.h>

#include <common/defines.h>
#include <common/iolet.h>
#include <common/check_failure.h>
#include <common/gen_cli.h>
#include <common/getopt_clone.h>
#include <common/file.h>

#include "archive.h"
#include "tmpfile.h"
#include "sequconfig.h"
#include "configvars.h"
#include "gui.h"
#include "gtk2int.h"

#include "util.h"

static char *file_to_open=NULL;

static gen_cli_argument cmdargs;

static int argopts_parsefunc(int ident,int argc,char **argv,int getopt_ret)
{
  if(getopt_ret == -3 && file_to_open == NULL)
    file_to_open=argv[argc];

  if(getopt_ret != GETOPT_RETURN_FLAG)
    return 0;
  
  switch(ident)
  {
  /* purge temps */
  case 1:
    tmpfile_init();
    print_out("Purging temporary files.\n");
    return -1;
  break;

  /* truncate config */
  case 2:
  {
    char *tmp;
    print_out("Generating the default config.\n");

    if(!init_config_files())
      return -1;

    /* backup the old config */
    tmp=malloc(strlen(sequ_config_generated_config_file_path)+5);
    strcpy(tmp,sequ_config_generated_config_file_path);
    strcat(tmp,".old");

    rename(sequ_config_generated_config_file_path,tmp);
    write_config(sequ_config_generated_config_file_path);

    nullify(tmp);
    return -1;
  }
  break;

  /* help */
  case 3:
    gen_cli_print_help(argv[0],&cmdargs);
    print_out("Send bug-reports to <kalle.kankare@tut.fi>\n");
    return -1;
  break;

  /* version */
  case 4:
    print_out("%s\n",sequ_config_gtk2int_mainwindow_title);
    return -1;
  break;

  /* possible filename(s) */
  case -1: 
    /*
    print_debug("argc on tässä %d kun %d ja argv [%s]\n",argc,
      optarg_pos_clone,argv[optarg_pos_clone]);

    file_to_open=argv[optarg_clone_pos];
    */

  break;

#ifdef DEBUG
  /* debug-log */
  case 5:
  {
    iolet *tmp;
    CHECK_FAILURE(
      tmp=iolet_file_create(optarg_clone,iolet_file_mode_convert("a")),
      NULL,-1);

    print_debug("Diverting debug_out to file: %s\n",optarg_clone);

    IL_OutDebug=tmp;
  }
  break;
#endif

  }
  
  return TRUE;
}

static gen_cli_argument cmdargs =
{
  NULL,
  0,
  NULL,
  "[filename]",
  "Sequview is a program for displaying sequences of images and scaling them. "
  "Usually within a compressed archive.",
  {
    (option_clone [])
    {
      {"purge-temps",'p',0,1},
      {"trunc-config",'t',0,2},
      {"help",'h',0,3},
      {"version",'v',0,4},
#ifdef DEBUG
      {"debug-log",'l',GETOPT_REQUIRED_ARGUMENT,5},
#endif
      {0,0,0,0}
    },
    (gen_cli_helpstr [])
    {
      {"Removes the possible temporary files left from a crash.",NULL},
      {"Generates the default \'config\' -file. The old is renamed as"
       " \'config.old\'",NULL },
      {"Displays this help.",NULL },
      {"Displays the version.",NULL},
#ifdef DEBUG
      {"Diverts the debugoutput to \"file\".","file"},
#endif
    }
  },
  NULL,
  NULL,
  GEN_CLI_FLAGS_OPTIONAL,
  argopts_parsefunc
};

#include "imgarch.h"

/* the main */
int main (int argc, char ** argv)
{
  int ret;

  iolet_init();

  if(sequ_config_init() == FALSE)
    return 1;

  tmpdir_init(NULL);

  return 0;

  ret=gen_cli_parse_args(&cmdargs,argc,argv);

  if(ret < 0)
    return 1;

  /* read the configuration file */
  if(read_config_proper() == FALSE)
    return 1;

  if(tmpfile_init() == FALSE)
    return 1;

  {
    sequ_gui *gui;

    gui=gtk2_gui_create(&argc,&argv,
      sequ_config_gtk2int_mainwindow_width,
      sequ_config_gtk2int_mainwindow_height);

    if(gui)
      gui->run(gui,file_to_open);
  }

  return 0;
}
