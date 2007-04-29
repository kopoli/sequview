/***************************************************************************
  Name:         archive.h
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

#ifndef ARCHIVE_HEADER
#define ARCHIVE_HEADER

typedef struct 
{
  char *list;
  char *decompress;
  unsigned int flags;
} archive_cmd;

#define ARCHIVE_CMD_ESCAPE_WILDCARDS 1

typedef struct
{
  char *name;
  char *magic;
  unsigned int magic_length;
} archive_type;

extern const archive_type archive_supported_formats[];
//const unsigned int archive_command_count;

const archive_type *archive_get_type(char *archive);

char **archive_get_list(char *archive, const archive_type *type);
tvalue archive_get_file(char *archive, const archive_type *type,
  char *name,char *tofile);
tvalue archive_process_filenames(char **files, const archive_type *type);

#endif
