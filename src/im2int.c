/***************************************************************************
  Name:         im2int.c
  Description:  The imlib2 interface
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

#include <stdlib.h>

#include <X11/Xlib.h>
#include <Imlib2.h>

#include "common/defines.h"
#include "common/iolet.h"

#include "configvars.h"
#include "imagelib.h"

#include "im2int.h"

static tvalue im2_initialized = FALSE;

/* initialisation function.   */
static tvalue im2_init(Display *disp)
{
  Visual    *vis;
  Colormap  cm;
  int       depth;

  imlib_set_cache_size(sequ_config_imlib_cache_size*1048576);
  imlib_set_color_usage(sequ_config_imlib_color_usage);
  imlib_context_set_dither(sequ_config_imlib_dither);
  imlib_context_set_anti_alias(sequ_config_imlib_anti_alias);  
  
  if(im2_initialized == TRUE)
    return TRUE;
 
  if(!disp)
    return FALSE;

  vis   = DefaultVisual(disp, DefaultScreen(disp));
  depth = DefaultDepth(disp, DefaultScreen(disp));
  cm    = DefaultColormap(disp, DefaultScreen(disp));

  imlib_context_set_display(disp);
  imlib_context_set_visual(vis);
  imlib_context_set_colormap(cm);
  
  im2_initialized=TRUE;

  return TRUE;
}

static void img_from_im2(sequ_image *ret,Imlib_Image img)
{
  ret->lib=&im2_lib;
  ret->privdata=img;

  /* get the diameters */
  imlib_context_set_image(img);
  ret->width=imlib_image_get_width();
  ret->height=imlib_image_get_height();
}

static struct sequ_image *im2_image_open(char *name)
{
  sequ_image *ret;
  Imlib_Image img;

  if(!name || !im2_initialized)
    return NULL;

  /* load the data */
  img=imlib_load_image_immediately_without_cache(name);

  if(img == NULL) 
  {
    print_err("Error: Loading an image with imlib2 failed\n");
    return NULL;
  }

  print_debug("%s: kuvan osoite on %p\n",THIS_FUNCTION,img);

  /* construct the image */
  ret=malloc(sizeof(sequ_image));
  img_from_im2(ret,img);

  return ret;
}

static void im2_image_remove(struct sequ_image *img)
{
  if(!img)
    return;

  imlib_context_set_image((Imlib_Image)img->privdata);  
  imlib_free_image_and_decache();
  
  free(img);

  return;
}

static tvalue im2_blend(struct sequ_image *from, struct sequ_image *to,
  int x, int y, int w, int h)
{
  if(!from || !to)
    return FALSE;
  
  imlib_context_set_image((Imlib_Image)to->privdata);

  imlib_blend_image_onto_image((Imlib_Image)from->privdata,
    0,0,0,
    from->width,from->height,x,y,w,h);

  print_debug("%s: kuvan osoite on %p\n",THIS_FUNCTION,
    (Imlib_Image)from->privdata);

  return TRUE;
}

static tvalue im2_draw(struct sequ_image *img,XID drw,
  int sx, int sy, int sw, int sh, 
  int tx, int ty, int tw, int th)
{
  if(!img)
    return FALSE;

  imlib_context_set_image((Imlib_Image)img->privdata);
  imlib_context_set_drawable(drw);

  imlib_render_image_part_on_drawable_at_size
    (sx,sy,sw,sh,tx,ty,tw,th);

  return TRUE;
}

static struct sequ_image *im2_resize(struct sequ_image *old,int w, int h)
{

  if(old != NULL)
  {
    if(w == old->width && h == old->height)
      return old;
    
    imlib_context_set_image((Imlib_Image)old->privdata);
    imlib_free_image_and_decache();
  }
  else
    old=malloc(sizeof(sequ_image));

  old->privdata=imlib_create_image(w,h);
  old->lib=&im2_lib;
  img_from_im2(old,(Imlib_Image)old->privdata);

  return old;
}

static tvalue im2_blank_image(struct sequ_image *img)
{
  if(!img)
    return FALSE;

  imlib_context_set_image((Imlib_Image)img->privdata);
  
  imlib_context_set_color(0,0,0,255);
  imlib_image_fill_rectangle(0,0,img->width,img->height);
  
  return TRUE;
}

static char *im2_get_formats(void)
{
  return "N/A, check /usr/lib/imlib2/loaders or similar.";
}

/* the API for imlib2 */
const sequ_image_lib im2_lib =
{
  im2_init,
  im2_image_open,
  im2_image_remove,
  im2_blend,
  im2_draw,
  im2_resize,
  im2_blank_image,
  im2_get_formats,
  NULL
}; 

