/***************************************************************************
  Name:         gtk2config.h
  Description:  The configuration dialog
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

#ifndef GTK2CONFIG_HEADER
#define GTK2CONFIG_HEADER

typedef struct configuration_page
{
  /* name of the page */
  char *name;

  /* creation, update and delete */
  tvalue (*create)(struct configuration_page *);
  void (*update)(struct configuration_page *);
  void (*remove)(struct configuration_page *);

  /* widget for the page */
  GtkWidget *page;

  /* the private data for each page */
  void *pagedata;

} configuration_page;

typedef struct
{
  /* the dialog widget */
  GtkWidget *dialog;

  /* the widget in which the pages are put */
  GtkWidget *dialog_contain;

  /* the name of the currently selected page */
  gchar *selected;

  /* the widget of the selected page */
  GtkWidget *selected_page;

  /* possible configuration pages */
  configuration_page *pages;

} configuration_dialog;


configuration_dialog *configuration_dialog_create();
void configuration_dialog_update_cfg(configuration_dialog *);
void configuration_dialog_delete(configuration_dialog *);

#endif
