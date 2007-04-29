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

#ifndef CONFIGVARS_HEADER
#define CONFIGVARS_HEADER

#include "archive.h"

extern unsigned long int  sequ_config_imlib_cache_size;
extern unsigned char      sequ_config_imlib_dither;
extern unsigned char      sequ_config_imlib_anti_alias;
extern unsigned int       sequ_config_imlib_color_usage;

/* imagefitting styles */
#define SEQU_CFG_DRAW_CANVAS_SCALE        0
#define SEQU_CFG_DRAW_CANVAS_FIT          1
#define SEQU_CFG_DRAW_CANVAS_FIT_WIDTH    2
#define SEQU_CFG_DRAW_CANVAS_FIT_HEIGHT   3

extern int    sequ_config_draw_fit;
extern double sequ_config_draw_scale;

extern unsigned int sequ_config_images_extra;
//extern unsigned int sequ_config_images_loaded;
extern unsigned int sequ_config_image_rows;
extern unsigned int sequ_config_image_cols;

extern tvalue sequ_config_draw_wide_as_2;

extern char *sequ_config_versioncopy;

extern char *sequ_config_gtk2int_mainwindow_title;
extern unsigned int sequ_config_gtk2int_mainwindow_width;
extern unsigned int sequ_config_gtk2int_mainwindow_height;

extern const char *sequ_config_configuration_file;

extern char *sequ_config_generated_config_dir_path;
extern char *sequ_config_generated_config_file_path;
extern char *sequ_config_generated_tmpfile_dir_path;

extern char **sequ_config_gtk_gui_keys;

extern archive_cmd *sequ_config_archive_cmds;

/* maximums and minimums for the above variables */
#define SEQU_CFG_IMLIB_CACHE_SIZE_MIN 0
#define SEQU_CFG_IMLIB_CACHE_SIZE_MAX 50
#define SEQU_CFG_IMLIB_DITHER_MIN 0
#define SEQU_CFG_IMLIB_DITHER_MAX 1
#define SEQU_CFG_IMLIB_ANTI_ALIAS_MIN 0
#define SEQU_CFG_IMLIB_ANTI_ALIAS_MAX 1
#define SEQU_CFG_IMLIB_COLOR_USAGE_MIN 0
#define SEQU_CFG_IMLIB_COLOR_USAGE_MAX 256


#define SEQU_CFG_GTK2_DEFAULT_MAINWINDOW_WIDTH   600
#define SEQU_CFG_GTK2_DEFAULT_MAINWINDOW_HEIGHT  400

#define SEQU_CFG_GTK2_MAINWINDOW_WIDTH_MIN \
  (SEQU_CFG_GTK2_DEFAULT_MAINWINDOW_WIDTH/3)
#define SEQU_CFG_GTK2_MAINWINDOW_HEIGHT_MIN \
  (SEQU_CFG_GTK2_DEFAULT_MAINWINDOW_HEIGHT/3)

#define SEQU_CFG_GTK2_MAINWINDOW_WIDTH_MAX 10000 
#define SEQU_CFG_GTK2_MAINWINDOW_HEIGHT_MAX 10000 

#define SEQU_CFG_IMAGES_EXTRA_MAX 100
#define SEQU_CFG_IMAGES_ROWS_MAX 10
#define SEQU_CFG_IMAGES_COLS_MAX 10

#define SEQU_CFG_TMPFILE_NAME   "sv"
#define SEQU_CFG_TMPFILE_SUFFIX ".tmp"
extern const char *sequ_config_tmpfile_dir;

tvalue sequ_config_init();

void sequ_config_keys_default();
void sequ_config_commands_default();

#endif
