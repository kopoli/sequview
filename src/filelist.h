/***************************************************************************
  Name:         filelist.h
  Description:  A list of filenames
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

#ifndef FILELIST_HEADER
#define FILELIST_HEADER

#include "archive.h"

typedef struct
{
  char *path;
  tvalue directory;
  const archive_type *atype;
  char **files;
  unsigned int count;
} file_list;


void file_list_delete(file_list *list);
inline tvalue file_list_valid(file_list *list);
void file_list_print(file_list *list);
inline unsigned int get_file_list_count(file_list *list);

file_list *get_file_list(char *filename);

char *file_list_get_file(file_list *list,unsigned int pos);

char *file_list_get_filename(file_list *list,unsigned int pos);

#endif
