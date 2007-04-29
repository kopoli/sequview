/***************************************************************************
  Name:         canvas.h
  Description:  Management of the canvas
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

#ifndef CANVAS_HEADER
#define CANVAS_HEADER

#include <X11/Xlib.h>
#include "imagelib.h"
#include "imagelist.h"

/* canvas properties */
typedef struct 
{
  XID drawable;

  /* the viewport. The dimensions must be less or equal to the canvas' 
     dimensions. */
  int view_width,view_height;
  
  /* the relative viewport position. 
     values 0<=x<=1, where 0 means real 0 and 1 means real position
     canvas size-viewport size.
  */
  float view_rel_x,view_rel_y;

  /* canvas parameters */
  unsigned int rows;
  unsigned int cols;

  /* drawing style */
  int draw_fitstyle;
  float draw_scale_factor;
  
  /* the using layer can resize its canvas with this */
  /*
  void (*resize_func)(void *param,int width,int height);
  void *resize_param;
  */

} sequ_canvas_props;

/* the canvas */
typedef struct
{
  /* the changeable properties */
  sequ_canvas_props prp;

  /* internal variables */

  /* the image which is drawn onto the X drawable */
  sequ_image *canvas;

} sequ_canvas;

/* the possible values of props in sequ_canvas_update */
#define SEQU_CANVAS_PROP_ALL ((unsigned int)~0)
#define SEQU_CANVAS_PROP_DRAWABLE        (1<<0)
#define SEQU_CANVAS_PROP_VIEWPORT        (1<<1)
//#define SEQU_CANVAS_PROP_VIEWPORT_POS    (1<<2)
#define SEQU_CANVAS_PROP_LAYOUT          (1<<3)
#define SEQU_CANVAS_PROP_FITSTYLE_SCALE  (1<<4)
//#define SEQU_CANVAS_PROP_RESIZEFUNC      (1<<5)

/* creation */
sequ_canvas *sequ_canvas_create(sequ_canvas_props *prp);
void sequ_canvas_delete(sequ_canvas *seqc);

/* update selectively the properties of the canvas */
tvalue sequ_canvas_update(sequ_canvas *seqc,unsigned int props,
  sequ_canvas_props *prp);

/* draws the imagelist onto the canvas */
tvalue sequ_canvas_realize(sequ_canvas *seqc, image_list *list);

/* draw the canvas and increment the viewport position. */
tvalue sequ_canvas_draw(sequ_canvas *seqc, 
  float view_rel_inc_x, float view_rel_inc_y);

#endif
