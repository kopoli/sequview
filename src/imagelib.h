/***************************************************************************
  Name:         imagelib.h
  Description:  The image library interface
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

#ifndef IMAGELIB_HEADER
#define IMAGELIB_HEADER

#include <X11/Xlib.h>

#include <common/defines.h>

struct sequ_image;

/* the API for an image library */
typedef struct
{
  /* initialization 
     This is also called when some parameters are configured. */
  tvalue (*init)(Display *);
  
  /* handling images */
  struct sequ_image *(*image_open)(char *);
  void (*image_remove)(struct sequ_image *);

  /* drawing */
  tvalue (*blend_to_image)(struct sequ_image *from, struct sequ_image *to,
    int x, int y, int w, int h);
  tvalue (*blend_to_drawable)(struct sequ_image *,XID ,
    int sx, int sy, int sw, int sh, /* source diams */
    int tx, int ty, int tw, int th);/* target */

  /* creating/resizing images */
  struct sequ_image *(*image_resize)(struct sequ_image *old,int w, int h);

  /* other drawing */
  tvalue (*blank_image)(struct sequ_image *);

  void *privdata;

} sequ_image_lib;

/* the generic image */
typedef struct sequ_image 
{
  /* a pointer to its handler */
  const sequ_image_lib *lib;

  unsigned int width, height;

  void *privdata;

} sequ_image;

#define IMAGE_IS_WIDE(img) ((img->width>img->height)?TRUE:FALSE)

#endif
