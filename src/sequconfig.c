/***************************************************************************
  Name:         sequconfig.c
  Description:  Configuration file management
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

#include <common/check_failure.h>
#include <common/iolet.h>
#include <common/conf.h>
#include <common/file.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "util.h"
#include "configvars.h"
#include "gtk2int.h"
#include "sequconfig.h"

/****************************************************************/

/* the configuration */
static conf_header config_identifiers[] =
{
  {"imlib2", (conf_identifier [])
   {
     {"cachesize",  CONF_INTEGER},
     {"dither",     CONF_INTEGER},
     {"antialias",  CONF_INTEGER},
     {"colorusage", CONF_INTEGER},
     {"drawstyle",  CONF_STRING},
     {"scalefactor",CONF_FLOAT},
     {NULL,CONF_INVALID}
   }
  },
  {"gui", (conf_identifier [])
   {
     {"canvas_width",   CONF_INTEGER},
     {"canvas_height",  CONF_INTEGER},
     {"images_extra",  CONF_INTEGER},
     {"images_rows",    CONF_INTEGER},
     {"images_cols",    CONF_INTEGER},  
     {NULL,CONF_INVALID}
   }
  },
  {"gtk2_keys", (conf_identifier [])
   {
     {"open_file",     CONF_STRING},
     {"help",          CONF_STRING},
     {"maximize",      CONF_STRING},
     {"fullscreen",    CONF_STRING},
     {"page_first",    CONF_STRING},
     {"page_last",     CONF_STRING},
     {"page_forward",  CONF_STRING},
     {"page_back",     CONF_STRING},
     {"configuration", CONF_STRING},
     {"about",         CONF_STRING},
     {"quit",          CONF_STRING},
     {"iconify",       CONF_STRING},
     {"scroll_up",     CONF_STRING},
     {"scroll_down",   CONF_STRING},
     {"scroll_left",   CONF_STRING},
     {"scroll_right",  CONF_STRING},
     {NULL,CONF_INVALID}
   }
  },
  {"filetypes", (conf_identifier [])
   {
     {"zip_list",       CONF_STRING},
     {"zip_decompress", CONF_STRING},
     {"rar_list",       CONF_STRING},
     {"rar_decompress", CONF_STRING},
     {"tgz_list",       CONF_STRING},
     {"tgz_decompress", CONF_STRING},
     {"tbz2_list",      CONF_STRING},
     {"tbz2_decompress",CONF_STRING},
     {NULL,CONF_INVALID}
   }
  },
  {NULL,NULL}
};

static char *interpret_drawstyles[] =
{
  "scale",
  "fit",
  "fit-width",
  "fit-height"
};


static int set_within_limits(int min, int val, int max)
{
  if(val < min)
    return min;
  else if(val > max)
    return max;
  return val;
}

static int whichone(char **strs, char *cmpstr)
{
  register int beta=0;

  while(strs[beta] != NULL && strcmp(strs[beta],cmpstr) != 0)
    beta++;

  if(strs[beta] == NULL)
    return -1;

  return beta;
}

static void interpret_imlib2(conf_out_value *outitem)
{
  unsigned int min=0,max=0;
  unsigned long int *target=NULL;

  int pos;

  switch(outitem->ident)
  {
  case 0:
    min=SEQU_CFG_IMLIB_CACHE_SIZE_MIN;
    max=SEQU_CFG_IMLIB_CACHE_SIZE_MAX;
    target=&sequ_config_imlib_cache_size;
  break;
  case 1:
    min=SEQU_CFG_IMLIB_DITHER_MIN;
    max=SEQU_CFG_IMLIB_DITHER_MAX;
    target=(unsigned long int *) &sequ_config_imlib_dither;
  break;
  case 2:
    min=SEQU_CFG_IMLIB_ANTI_ALIAS_MIN;
    max=SEQU_CFG_IMLIB_ANTI_ALIAS_MAX;
    target=(unsigned long int *) &sequ_config_imlib_anti_alias;
  break;
  case 3:
    min=SEQU_CFG_IMLIB_COLOR_USAGE_MIN;
    max=SEQU_CFG_IMLIB_COLOR_USAGE_MAX;
    target=(unsigned long int *) &sequ_config_imlib_color_usage;
  break;
  case 4:
    print_debug("%s: fittaustyyli on [%s]\n",THIS_FUNCTION,
      outitem->value.string);
    pos=whichone(interpret_drawstyles,outitem->value.string);
    if(pos == -1)
      sequ_config_draw_fit=SEQU_CFG_DRAW_CANVAS_FIT;
    else
      sequ_config_draw_fit=pos;
  break;

  case 5:
    sequ_config_draw_scale=outitem->value.floatp;
    break;

  default:
    return;
  }

  if(outitem->ident <= 3)
    *target=set_within_limits(min,outitem->value.integer,max);

}

static void interpret_gui(conf_out_value *outitem)
{
  unsigned int min,max;
  unsigned int *target;

  switch(outitem->ident)
  {
  case 0:
    min=SEQU_CFG_GTK2_MAINWINDOW_WIDTH_MIN;
    max=SEQU_CFG_GTK2_MAINWINDOW_WIDTH_MAX;
    target=&sequ_config_gtk2int_mainwindow_width;
  break;

  case 1:
    min=SEQU_CFG_GTK2_MAINWINDOW_HEIGHT_MIN;
    max=SEQU_CFG_GTK2_MAINWINDOW_HEIGHT_MAX;
    target=&sequ_config_gtk2int_mainwindow_height;
  break;

  case 2:
    min=0;
    max=SEQU_CFG_IMAGES_EXTRA_MAX;
    target=&sequ_config_images_extra;
  break;

  case 3:
    min=0;
    max=SEQU_CFG_IMAGES_ROWS_MAX;
    target=&sequ_config_image_rows;
  break;

  case 4:
    min=0;
    max=SEQU_CFG_IMAGES_COLS_MAX;
    target=&sequ_config_image_cols;
  break;

  default: 
    return;
  }

  *target=set_within_limits(min,outitem->value.integer,max);
}

static void interpret_gtk2_keys(conf_out_value *outitem)
{
  nullify(sequ_config_gtk_gui_keys[outitem->ident]);
  sequ_config_gtk_gui_keys[outitem->ident]=strdup(outitem->value.string);
}

static void interpret_filetypes(conf_out_value *outitem)
{
  unsigned int pos=outitem->ident/2;

  /* if list */
  if(!(outitem->ident % 2))
  {
    nullify(sequ_config_archive_cmds[pos].list);
    sequ_config_archive_cmds[pos].list=strdup(outitem->value.string);
  }
  else
  {
    nullify(sequ_config_archive_cmds[pos].decompress);
    sequ_config_archive_cmds[pos].decompress=strdup(outitem->value.string);
  }

  /*
  print_debug("%s: täällä pos on %d decomp %d data [%s]\n",THIS_FUNCTION,pos,
    outitem->ident % 2,outitem->value.string);
  */
}

static void interpret_configuration(conf_out_value *out)
{
  register unsigned int beta;

  for(beta=0;out[beta].header != -1;beta++)
    switch(out[beta].header)
    {
    case 0:
      interpret_imlib2(&out[beta]);
      break;
    case 1:
      interpret_gui(&out[beta]);
      break;
    case 2:
      interpret_gtk2_keys(&out[beta]);
      break;
    case 3:
      interpret_filetypes(&out[beta]);
      break;

    /* ignore phony headers */
    default:
      break;
    }
}

tvalue read_config(const char *filename)
{
  iolet *file;
  conf_out_value *out;

  ARG_ASSERT(!filename,FALSE);

  if((file=iolet_file_create(filename,iolet_file_mode_convert("r"))) == NULL)
  {
    print_err("Error: Could not read config from file: \"%s\". Errno: %d\n",
      filename,strerror(errno));
    return FALSE;
  }

  /* read the configuration s*/
  CHECK_FAILURE_ACT(
    out=conf_parse_iolet(config_identifiers,file), NULL,
      iolet_del(file);
      return FALSE;
  );

  iolet_del(file);

  /* interpret the config */
  interpret_configuration(out);

  conf_free_output(out);

  return TRUE;
}

tvalue write_config(const char *filename)
{
  iolet *file;

  /* read the config from variables */
  conf_out_value vals[] =
  {
    /* imlib2 header */
    {0,0,0,{.integer=sequ_config_imlib_cache_size}},
    {0,1,0,{.integer=sequ_config_imlib_dither}},
    {0,2,0,{.integer=sequ_config_imlib_anti_alias}},
    {0,3,0,{.integer=sequ_config_imlib_color_usage}},
    {0,4,0,{.string=interpret_drawstyles[sequ_config_draw_fit]}},
    {0,5,0,{.floatp=sequ_config_draw_scale}},

    /* gui header */
    {1,0,0,{.integer=sequ_config_gtk2int_mainwindow_width}},
    {1,1,0,{.integer=sequ_config_gtk2int_mainwindow_height}},
    {1,2,0,{.integer=sequ_config_images_extra}},
    {1,3,0,{.integer=sequ_config_image_rows}},
    {1,4,0,{.integer=sequ_config_image_cols}},

    /* gtk2_keys header */
    {2,0,0,{.string=sequ_config_gtk_gui_keys[0]}},
    {2,1,0,{.string=sequ_config_gtk_gui_keys[1]}},
    {2,2,0,{.string=sequ_config_gtk_gui_keys[2]}},
    {2,3,0,{.string=sequ_config_gtk_gui_keys[3]}},
    {2,4,0,{.string=sequ_config_gtk_gui_keys[4]}},
    {2,5,0,{.string=sequ_config_gtk_gui_keys[5]}},
    {2,6,0,{.string=sequ_config_gtk_gui_keys[6]}},
    {2,7,0,{.string=sequ_config_gtk_gui_keys[7]}},
    {2,8,0,{.string=sequ_config_gtk_gui_keys[8]}},
    {2,9,0,{.string=sequ_config_gtk_gui_keys[9]}},
    {2,10,0,{.string=sequ_config_gtk_gui_keys[10]}},
    {2,11,0,{.string=sequ_config_gtk_gui_keys[11]}},
    {2,12,0,{.string=sequ_config_gtk_gui_keys[12]}},
    {2,13,0,{.string=sequ_config_gtk_gui_keys[13]}},
    {2,14,0,{.string=sequ_config_gtk_gui_keys[14]}},
    {2,15,0,{.string=sequ_config_gtk_gui_keys[15]}},

    /* filetypes header */
    {3,0,0,{.string=sequ_config_archive_cmds[0].list}},
    {3,1,0,{.string=sequ_config_archive_cmds[0].decompress}},
    {3,2,0,{.string=sequ_config_archive_cmds[1].list}},
    {3,3,0,{.string=sequ_config_archive_cmds[1].decompress}},
    {3,4,0,{.string=sequ_config_archive_cmds[2].list}},
    {3,5,0,{.string=sequ_config_archive_cmds[2].decompress}},
    {3,6,0,{.string=sequ_config_archive_cmds[3].list}},
    {3,7,0,{.string=sequ_config_archive_cmds[3].decompress}},
    {-1,0,0,{0}}    
  };

  ARG_ASSERT(!filename,FALSE);

  CHECK_FAILURE_WITH_ERRNO(
    file=iolet_file_create(filename,iolet_file_mode_convert("w")),NULL,FALSE
  );

  CHECK_FAILURE_WITH_ERRNO_ACT(
    conf_print_output(file,config_identifiers,vals),FALSE,
      iolet_del(file);
      return FALSE;
  );

  iolet_del(file);
  return TRUE;
}


/* creates the directory, file and writes the default config, if they dont
  already exist. Also reads the configuration. This should be executed first 
  of all functions */
tvalue read_config_proper()
{

  if(init_config_files() == FALSE)
    return FALSE;

  if(read_config(sequ_config_generated_config_file_path) == FALSE)
    return FALSE;

  return TRUE;
}

/* Makes sure that the config dir and file exists. Returns TRUE if
  the files exist and are usable, FALSE otherwise */
tvalue init_config_files()
{
  int fd;
  struct stat st;

  if(create_directory(sequ_config_generated_config_dir_path) == FALSE)
    return FALSE;

  if(create_directory(sequ_config_generated_tmpfile_dir_path) == FALSE)
    return FALSE;

  if(stat(sequ_config_generated_config_file_path,&st) == -1)
  {
    if(errno == ENOENT)
    {
      /* create the file */
      if((fd=open(sequ_config_generated_config_file_path,O_CREAT, 0600)) == -1)
      {
        print_err("Error: open() failed for \"%s\" with: \"%s\"\n",
          sequ_config_generated_config_file_path,strerror(errno));
        return FALSE;
      }
    
      close(fd);

      /* write the default configuration */
      if(write_config(sequ_config_generated_config_file_path) == FALSE)
      {
        print_err("Error: Could not write default config to file: \"%s\"\n",
          sequ_config_generated_config_file_path);
        return FALSE;
      }
    }
    else
    {
      print_err("Error: stat() for file \"%s\" failed with: \"%s\"\n",
        sequ_config_generated_config_file_path,strerror(errno)); 
      return FALSE;
    }
  }

  return TRUE;
}
