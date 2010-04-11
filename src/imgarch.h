/***************************************************************************
  Name:         imgarch.h
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

#ifndef IMGARCH_HEADER
#define IMGARCH_HEADER


typedef struct 
{
  char **images;
  char **extra;
} image_archive_filenames;

typedef struct
{
  char *name;
  char *base;

  image_archive_filenames iaf;

} image_archive;

typedef struct
{
  char *name;
  char *decompress_cmd;

  char *magic;
  int magic_offset;
  char **suffices;

} archive_type2;

char *tmpdir_init(char *tmpdir_path);

tvalue archive_register_default_formats();


tvalue archive_extract(char *arch_name,char *extract_path);

#endif
