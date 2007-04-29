/***************************************************************************
  Name:         gtk2confpages.h
  Description:  The pages of the configuration dialog
  Created:      20060606
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

#ifndef GTK2CONFPAGES_HEADER
#define GTK2CONFPAGES_HEADER

typedef struct
{
  char *name;
  
  GtkWidget *(*create)();

  /* refreshes the possible values */
  void (*update)();

} config_page;

extern const config_page config_pages[];

#endif
