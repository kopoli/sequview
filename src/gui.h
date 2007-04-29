/***************************************************************************
  Name:         gui.h
  Description:  GUI
  Created:      20060423
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

#ifndef GUI_HEADER
#define GUI_HEADER

/* gui-module. */
typedef struct sequ_gui
{
  /* starts */
  tvalue (*init)(struct sequ_gui *gui, int width, int height);

  /* runs. Possibly opens a file */
  tvalue (*run)(struct sequ_gui *gui, char *filename);

  /* frees and deinits itself */
  void (*deinit)(struct sequ_gui *gui);

  /* identification information. Possible values:
     -GTK2 1  */
  int (*identify)(void);

  /* forces redraw */
  void (*redraw)(void);

  /* the diameters of the window */
  unsigned int width,height;

  /* private data */
  void *private;

} sequ_gui;


#endif /* GUI_HEADER */
