/***************************************************************************
  Name:         util.h
  Description:  Utilities
  Created:      20060525
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

#ifndef UTIL_HEADER
#define UTIL_HEADER

void ptrarray_delete(void *);

char **stringlist_create(char *str,char delim, unsigned int extra_space);

char **directory_list(char *path);

tvalue create_directory(char *dir);

#endif
