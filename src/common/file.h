/***************************************************************************
  Name:         file.h
  Description:  filesystem functionality of Infoleech
  Created:      Tue Feb  25 18:20:24 EET 2003
  Copyright:    (C) 2003 by Kalle "Gobol" Kankare
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

#ifndef IL_FILE_HEADER
#define IL_FILE_HEADER

#include <stdint.h>

#include "iolet.h"

/****************************************************************************
  Defines / Structures / data types
 ****************************************************************************/

/* 
  The file opening mode. The values can be either those IL_MODE:s below
  or 0 in case of an error.
 */
typedef uint32_t iolet_file_mode;

/* 
  The modes of iolet_file_mode. Should be self-explanatory. These normal modes
  should all be in the least significant half of 
 */
#define IOLET_FILE_MODE_READ      1
#define IOLET_FILE_MODE_WRITE     2
#define IOLET_FILE_MODE_APPEND    4
//#define IOLET_FILE_MODE_OPEN      8   /*  */
#define IOLET_FILE_MODE_CREATE    16
#define IOLET_FILE_MODE_TRUNCATE  32

/*
  Modes explained in fopen()'s modes. 
  Stripped IL_MODE_
  r   READ
  r+  READ|WRITE
  w   WRITE|TRUNC
  w+  READ|WRITE|TRUNC|CREATE
  a   WRITE|APPEND|CREATE
  a+  READ|WRITE|APPEND|CREATE
*/

/****************************************************************************
  Prototypes
 ****************************************************************************/

/* Create a file iolet */
iolet *iolet_file_create(const char *Path, iolet_file_mode Mode);

/* create from existing filedescriptor */
iolet *iolet_file_create_fd(int filedesc, iolet_file_mode Mode);

/* Converts a fopen() mode to a iolet_file_mode */
iolet_file_mode iolet_file_mode_convert(const char *fopenmode);

#endif
