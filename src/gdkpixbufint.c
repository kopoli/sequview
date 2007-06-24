/***************************************************************************
  Name:         gdkpixbufint.c
  Description:  gdk-pixbuf interface
  Created:      20070609 22:43
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
#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf-xlib/gdk-pixbuf-xlib.h>

#include "common/defines.h"
#include "common/iolet.h"

#include "gdkpixbufint.h"


static char *gdkpixbuf_get_formats();

static tvalue gdkpixbuf_initialized = FALSE;

typedef struct 
{
  Display *disp;

  GdkInterpType interp_type;

  XlibRgbDither dither;

  char *formats;

} gdkpixbuf_lib_priv;

#define GDKPIXBUF_LIB_PRIV(library) \
  ((gdkpixbuf_lib_priv *)library.privdata)

static void gdkpixbuf_deinit(void)
{
  nullify(GDKPIXBUF_LIB_PRIV(gdkpixbuf_lib)->formats);
  nullify(gdkpixbuf_lib.privdata);
}

static tvalue gdkpixbuf_init(Display *disp)
{
  gdkpixbuf_lib_priv *priv;

  if(!disp)
    return FALSE;

  priv=malloc(sizeof(gdkpixbuf_lib_priv));

  /* the private configuration */
  priv->disp=disp;
  priv->interp_type=GDK_INTERP_BILINEAR;
  priv->dither=XLIB_RGB_DITHER_NORMAL;
  priv->formats=NULL;

  gdkpixbuf_lib.privdata=priv;

  gdk_pixbuf_xlib_init(disp,DefaultScreen(disp));

  atexit(gdkpixbuf_deinit);

  gdkpixbuf_initialized=TRUE;

  //  print_debug("Tuetut formaatit ovat: %s\n",gdkpixbuf_get_formats());

  return TRUE;
}


static void img_from_gdkpixbuf(sequ_image *ret,GdkPixbuf *img)
{
  ret->lib=&gdkpixbuf_lib;
  ret->privdata=img;

  /* get the diameters */
  ret->width=gdk_pixbuf_get_width(img);
  ret->height=gdk_pixbuf_get_height(img);
}

static struct sequ_image *gdkpixbuf_image_open(char *name)
{
  sequ_image *ret;
  GdkPixbuf *img;
  GError *err=NULL;

  if(!name || !gdkpixbuf_initialized)
    return NULL;

  img=gdk_pixbuf_new_from_file(name,&err);
  if(!img)
  {
    print_err("Error: Loading an image with gdk-pixbuf failed: %s\n",
      err->message);
    g_error_free(err);
    return NULL;
  }

  ret=malloc(sizeof(struct sequ_image));
  img_from_gdkpixbuf(ret,img);

  return ret;
}

static void gdkpixbuf_image_remove(struct sequ_image *img)
{
  if(!img)
    return;

  g_object_unref(G_OBJECT(img->privdata));
  free(img);
}

static tvalue gdkpixbuf_blend(struct sequ_image *from, struct sequ_image *to,
  int x, int y, int w, int h)

{
  double sc_w,sc_h;

  if(!from || !to)
    return FALSE;

  sc_w=(double)w/from->width;
  sc_h=(double)h/from->height;

  print_debug("%s: orig [%dx%d] given [%dx%d] scw %.2f sch %.2f\n",
    THIS_FUNCTION,from->width,from->height,w,h,sc_w,sc_h);
  
  gdk_pixbuf_scale(GDK_PIXBUF(from->privdata),GDK_PIXBUF(to->privdata),
    x,y,w,h,x,y,sc_w,sc_h,
    GDKPIXBUF_LIB_PRIV(gdkpixbuf_lib)->interp_type);

  return TRUE;
}

static tvalue gdkpixbuf_draw(struct sequ_image *img,XID drw,
  int sx, int sy, int sw, int sh, 
  int tx, int ty, int tw, int th)
{
  Display *disp;
  XlibRgbDither dither;

  if(!img)
    return FALSE;
  disp=GDKPIXBUF_LIB_PRIV(gdkpixbuf_lib)->disp;
  dither=GDKPIXBUF_LIB_PRIV(gdkpixbuf_lib)->dither;

  /* WARNING, this throws out sw and sh */
  gdk_pixbuf_xlib_render_to_drawable(GDK_PIXBUF(img->privdata),drw,
    DefaultGC(disp,DefaultScreen(disp)),
    sx,sy,tx,ty,tw,th,
    dither,0,0);

  return TRUE;
}

static struct sequ_image *gdkpixbuf_resize(struct sequ_image *old,int w, int h)
{

  if(old != NULL)
  {
    if(w == old->width && h == old->height)
      return old;

    g_object_unref(G_OBJECT(old->privdata));
  }
  else
    old=malloc(sizeof(sequ_image));

  old->privdata=gdk_pixbuf_new(GDK_COLORSPACE_RGB,FALSE,8,w,h);
  old->lib=&gdkpixbuf_lib;
  img_from_gdkpixbuf(old,GDK_PIXBUF(old->privdata));

  return old;
}

static tvalue gdkpixbuf_blank_image(struct sequ_image *img)
{
  if(!img)
    return FALSE;

  gdk_pixbuf_fill(GDK_PIXBUF(img->privdata),0);

  return TRUE;
}

static char *gdkpixbuf_get_formats()
{
  unsigned int length,beta,slen=0;
  char *ret;
  GSList *fmts,*pos;

  if(!gdkpixbuf_initialized)
    return NULL;

  if(GDKPIXBUF_LIB_PRIV(gdkpixbuf_lib)->formats != NULL)
    return GDKPIXBUF_LIB_PRIV(gdkpixbuf_lib)->formats;

  fmts=gdk_pixbuf_get_formats();

  length=g_slist_length(fmts);

  /* get the length */
  for(pos=fmts,beta=0;beta<length;beta++,pos=pos->next)
    slen+=1+strlen(gdk_pixbuf_format_get_name((GdkPixbufFormat *)pos->data));

  ret=malloc(slen);
  memset(ret,0,slen);

  /* write the string */
  for(pos=fmts,beta=0;beta<length;beta++)
  {
    strcat(ret,gdk_pixbuf_format_get_name((GdkPixbufFormat *)pos->data));
    strcat(ret," ");
    pos=pos->next;
  }

  GDKPIXBUF_LIB_PRIV(gdkpixbuf_lib)->formats=ret;

  return ret;
}

/* the API for GDK pixbuf */
sequ_image_lib gdkpixbuf_lib =
{
  gdkpixbuf_init,
  gdkpixbuf_image_open,
  gdkpixbuf_image_remove,
  gdkpixbuf_blend,
  gdkpixbuf_draw,
  gdkpixbuf_resize,
  gdkpixbuf_blank_image,
  gdkpixbuf_get_formats,
  NULL
}; 
