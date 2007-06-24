/***************************************************************************
  Name:         configvars.c
  Description:  Configuration variables
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

#include <common/defines.h>

#include <stdlib.h>

#ifdef HAVE_CONFIG_H
# include "config.h"
#else
#define REAL_VERSION "Someversion"
#endif

#include "check_failure.h"
#include "configvars.h"

/*
sequ_config_
*/


/* cache size in megabytes */
unsigned long int sequ_config_imlib_cache_size  = 2;
unsigned char sequ_config_imlib_dither          = 1;
unsigned char sequ_config_imlib_anti_alias      = 1;
unsigned int sequ_config_imlib_color_usage      = 256;

int sequ_config_draw_fit      = SEQU_CFG_DRAW_CANVAS_FIT;
double sequ_config_draw_scale = 0.5;

/* how many images are loaded at most at one time has to be >=1. If
  sequ_config_images_loaded < sequ_config_images_displayed then 
  sequ_config_images_loaded = sequ_config_images_displayed. */

/* how many extra images are loaded. The real number of images is thus
   sequ_config_image_rows*sequ_config_image_cols+sequ_config_images_extra */
unsigned int sequ_config_images_extra = 1;

/* the number of images displayed at once, also at least 1 */
unsigned int sequ_config_image_rows = 1;
unsigned int sequ_config_image_cols = 2;

/* should wide images be drawn as 2 images */
tvalue sequ_config_draw_wide_as_2 = TRUE;

#define versionstr "Sequview v. " REAL_VERSION

char *sequ_config_versioncopy=versionstr "\nKalle Kankare 2004-2007";
char *sequ_config_gtk2int_mainwindow_title=versionstr;
unsigned int sequ_config_gtk2int_mainwindow_width =
  SEQU_CFG_GTK2_DEFAULT_MAINWINDOW_WIDTH;
unsigned int sequ_config_gtk2int_mainwindow_height=
  SEQU_CFG_GTK2_DEFAULT_MAINWINDOW_HEIGHT;


#define SEQU_CONFIG_CONFDIR ".sequview"

/* the configuration file path */
const char *sequ_config_configuration_file = "config";

/* tmpfile configuration */
const char *sequ_config_tmpfile_dir = "tmpdata";

/* Generated filenames */
char *sequ_config_generated_config_dir_path = NULL;
char *sequ_config_generated_config_file_path = NULL;
char *sequ_config_generated_tmpfile_dir_path = NULL;

/* the used keys of the gui */
char **sequ_config_gtk_gui_keys=NULL;

/* the default keys */
static const char *sequ_config_gtk_gui_keys_default[] =
{
  "O",           /* open file */
  "<Control>H",  /* help */
  "M",           /* maximize */
  "F",           /* fullscreen */
  "Home",        /* page_first */
  "End",         /* page_last */
  "Page_Down",   /* page_forward */
  "Page_up",     /* page_back */
  "C",           /* configuration */
  "A",           /* about */
  "<Control>Q",  /* quit */
  "I",           /* iconify */
  "Up",          /* scrolling up */
  "Down",        /*  down */
  "Left",        /*  left */
  "Right",       /*  right*/
  NULL
};

/* explanations for the above keys */
const char *sequ_config_gtk_gui_keys_names[] =
{
  "Open file",
  "Help",
  "Maximize",
  "Fullscreen",
  "First page",
  "Last page",
  "Forward page",
  "Back page",
  "Configuration",
  "About",
  "Quit",
  "Iconify",
  "Scroll up",
  "Scroll down",
  "Scroll left",
  "Scroll right",
  NULL
};

/* the decompress and file list commands used */
archive_cmd *sequ_config_archive_cmds=NULL;

/* 
  allocates and constructs the configuration file path.
  which is as follows:
  0 config dir
  1 config file 
  2 tmpdata directory
*/ 
static char *construct_config_path(unsigned int which)
{
  unsigned int length;
  char *home,*ret;

  if(which > 2)
    return NULL;

  if(which == 1 && sequ_config_generated_config_file_path != NULL)
    return sequ_config_generated_config_file_path;

  /* construct the configuration file path */
  if((home=getenv("HOME")) == NULL)
  {
    print_err("Error: There is no environment variable $HOME?\n");
    return NULL;
  }

  length=strlen(home)+2+strlen(SEQU_CONFIG_CONFDIR);

  if(which == 1)
    length+=strlen(sequ_config_configuration_file)+1;
  else if(which == 2)
    length+=strlen(sequ_config_tmpfile_dir)+1;

  CHECK_FAILURE(
    ret=malloc(length),
    NULL, NULL
  );

  memset(ret,0,length);
  strncat(ret,home,strlen(home));
  strcat(ret,"/");
  strcat(ret,SEQU_CONFIG_CONFDIR);

  if(which == 1)
  {
    strcat(ret,"/");
    strncat(ret,sequ_config_configuration_file,
      strlen(sequ_config_configuration_file));

    sequ_config_generated_config_file_path=ret;
  }
  else if(which == 2)
  {
    strcat(ret,"/");
    strncat(ret,sequ_config_tmpfile_dir,strlen(sequ_config_tmpfile_dir));
  }

  return ret;
}

static tvalue sequ_config_generate_conf_filenames()
{
  sequ_config_generated_config_dir_path=construct_config_path(0);
  if(!sequ_config_generated_config_dir_path)
    return FALSE;

  sequ_config_generated_config_file_path=construct_config_path(1);
  if(!sequ_config_generated_config_file_path)
    return FALSE;

  sequ_config_generated_tmpfile_dir_path=construct_config_path(2);
  if(!sequ_config_generated_tmpfile_dir_path)
    return FALSE;
  
  return TRUE;
}


/* deallocate the keys */
static void sequ_config_keys_delete()
{
  unsigned int beta;

  for(beta=0;sequ_config_gtk_gui_keys[beta] != NULL;beta++)
    nullify(sequ_config_gtk_gui_keys[beta]);
  nullify(sequ_config_gtk_gui_keys);  
}


/* set up the default keys */
void sequ_config_keys_default()
{
  unsigned int beta,len;

  if(sequ_config_gtk_gui_keys)
    sequ_config_keys_delete();

  for(len=0;sequ_config_gtk_gui_keys_default[len] != NULL;len++);
 
  sequ_config_gtk_gui_keys=malloc((len+1)*sizeof(char*));

  /* copy the default keys into place */
  for(beta=0;beta<len;beta++)
    sequ_config_gtk_gui_keys[beta]=
    strdup(sequ_config_gtk_gui_keys_default[beta]);

  sequ_config_gtk_gui_keys[len]=NULL;
}

/* deallocate the archive commands */
static void sequ_config_commands_delete()
{
  unsigned int beta;
  for(beta=0;sequ_config_archive_cmds[beta].list != NULL;beta++)
  {
    nullify(sequ_config_archive_cmds[beta].list);
    nullify(sequ_config_archive_cmds[beta].decompress);
  }
  nullify(sequ_config_archive_cmds);
}

/* set up the archive commands */
void sequ_config_commands_default()
{
  unsigned int beta,len;
  extern const archive_cmd archive_default_cmds[];

  if(sequ_config_archive_cmds)
    sequ_config_commands_delete();

  for(len=0;archive_default_cmds[len].list != NULL;len++);

  sequ_config_archive_cmds=malloc((len+1)*sizeof(archive_cmd));

  for(beta=0;beta<len;beta++)
    sequ_config_archive_cmds[beta]=(archive_cmd)
      {
        strdup(archive_default_cmds[beta].list),
        strdup(archive_default_cmds[beta].decompress),
        archive_default_cmds[beta].flags
      };

  sequ_config_archive_cmds[len]=(archive_cmd){NULL,NULL,0};

  /*
    for(beta=0;beta<len;beta++)
    print_debug("%s: listauskomento %d: [%s]\n",THIS_FUNCTION,
    beta,archive_default_cmds[beta].list);
  */
}

static void sequ_config_deinit(void)
{
  sequ_config_keys_delete();
  sequ_config_commands_delete();

  nullify(sequ_config_generated_config_file_path);
  nullify(sequ_config_generated_config_dir_path);
  nullify(sequ_config_generated_tmpfile_dir_path);
}

/* does all the necessary initialization of this module */
tvalue sequ_config_init()
{
  sequ_config_keys_default();
  sequ_config_commands_default();

  /* set up the filenames */
  if(!sequ_config_generate_conf_filenames())
    return FALSE;
  
  atexit(sequ_config_deinit);

  return TRUE;
}
