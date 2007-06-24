/***************************************************************************
  Name:         canvas.c
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

#include <stdlib.h>
#include <errno.h>

#include <common/defines.h>
#include <common/iolet.h>
#include <common/check_failure.h>

#include "configvars.h"
#include "canvas.h"

/* updates the selected values in already created sequ_canvas */
tvalue sequ_canvas_update(sequ_canvas *seqc,
  unsigned int props,
  sequ_canvas_props *prp)
{
  if(!seqc)
    return FALSE;

  if(props & SEQU_CANVAS_PROP_DRAWABLE)
    seqc->prp.drawable=prp->drawable;
  if(props & SEQU_CANVAS_PROP_VIEWPORT)
  {
    seqc->prp.view_width=prp->view_width;
    seqc->prp.view_height=prp->view_height;
  }
  if(props & SEQU_CANVAS_PROP_LAYOUT)
  {
    seqc->prp.rows=prp->rows;
    seqc->prp.cols=prp->cols;
  }
  if(props & SEQU_CANVAS_PROP_FITSTYLE_SCALE)
  {
    seqc->prp.draw_fitstyle=prp->draw_fitstyle;
    seqc->prp.draw_scale_factor=prp->draw_scale_factor;
  }

  return TRUE;
}

sequ_canvas *sequ_canvas_create(sequ_canvas_props *prp)
{
  sequ_canvas *ret;

  /* alloc the memory */
  ret=malloc(sizeof(sequ_canvas));
  memset(ret,0,sizeof(sequ_canvas));

  /* write the structure */
  sequ_canvas_update(ret,SEQU_CANVAS_PROP_ALL,prp);

  ret->canvas=NULL;

  return ret;
}

void sequ_canvas_delete(sequ_canvas *seqc)
{
  if(!seqc)
    return;

  if(seqc->canvas)
    seqc->canvas->lib->image_remove(seqc->canvas);

  nullify(seqc);
}


/***/

/* Fits the source into the target retaining the ratio and returns the
   modified size in end. If type is 1, fits the width and if 2 fits height;
   automatically chooses the best fit otherwise. */
static void image_fit(
  unsigned int source_w, unsigned int source_h,
  unsigned int target_w, unsigned int target_h,
  unsigned int *end_w, unsigned int *end_h,char type)
{
  float source_r, target_r;

  source_r=(float) source_h / (float) source_w;
  target_r=(float) target_h / (float) target_w;

  if(type < 1 || type > 2)
    type=(target_r > source_r) ? 1 : 2;

  /*
  print_debug("%s: type %d target %f source %f\n",THIS_FUNCTION,type,
    target_r,source_r);
  */

  if(type == 1)
  {
    *end_h=target_w*source_r;
    *end_w=target_w;
  }
  else
  {
    *end_w=target_h/source_r;
    *end_h=target_h;    
  }
}

struct dims
{
  int wc;  /* width of the image in slots */
  int row; /* in which row the image resides */

  unsigned int w; 
  unsigned int h;
  unsigned int x;
  unsigned int y;
};

/* draws the imagelist onto the canvas image. Creates a new canvas image 
   if needed */
tvalue sequ_canvas_realize(sequ_canvas *seqc, image_list *list)
{
  unsigned int beta;
  unsigned int lst_start, lst_count;

  struct dims *img;

  unsigned int imgs_drawn;

  unsigned int img_w_mean=0,img_h_mean=0;

  unsigned int canvas_w,canvas_h;
  unsigned int canvas_resize_w,canvas_resize_h;

  unsigned int slot_w,slot_h;

  unsigned int basex=0,basey=0;

  sequ_image_lib *lib;

  if(!seqc || !imagelist_valid(list))
    return FALSE;

  /* from the imagelist get the first image drawn and the number of 
     images, that can be drawn */
  lst_start=imagelist_first_drawn_image(list);
  lst_count=0;
  for(beta=lst_start;
      beta<list->count && list->positions[beta] != -1;
      beta++) ;

  lst_count=beta-lst_start;

  print_debug("%s: lst_start %d lst_count %d\n",THIS_FUNCTION, 
    lst_start,lst_count);

  /* no images in the imagelist */
  if(lst_count == 0)
    return TRUE;

  img=malloc(lst_count*sizeof(struct dims));
  memset(img,0,lst_count*sizeof(struct dims));

  imgs_drawn=0;

  /* for each image get wc and row */
  for(unsigned int pos=0,row=0,beta=0;
      beta<lst_count;
      beta++)
  {
    /* read the slotwidth */
    img[beta].wc=1;
    if(list->wideimages && seqc->prp.cols > 1)
      img[beta].wc+=IMAGE_IS_WIDE(list->images[beta+lst_start]);

    /* read the diameters of the images */
    img[beta].w=list->images[beta+lst_start]->width;
    img[beta].h=list->images[beta+lst_start]->height;

    /* change the row if slotwidth exceeds the number of columns*/
    if(pos+img[beta].wc > seqc->prp.cols)
    {
      row++;
      pos=img[beta].wc;
    }
    else
      pos+=img[beta].wc;

    /* if the image is drawn */
    if(row < seqc->prp.rows)
    {
      /* the mean */
      img_w_mean+=img[beta].w;
      img_h_mean+=img[beta].h;
      
      /* the number of images to be drawn */
      imgs_drawn++;
    }

    img[beta].row=row;

    print_debug(" #%d: [%dx%d] row %d wc %d pos %d => drawn %d\n",
      beta,img[beta].w,img[beta].h,img[beta].row,img[beta].wc,pos, 
      (row < seqc->prp.rows));
  }

  /* fit images to the mean of images' sizes */
  img_w_mean /= imgs_drawn;
  img_h_mean /= imgs_drawn;
  
  for(unsigned int beta=0;beta<lst_count;beta++)
    image_fit(img[beta].w,img[beta].h,
      img_w_mean,img_h_mean,
      &img[beta].w,&img[beta].h,0);

  /* calculate the canvas' size */
  canvas_h=canvas_w=0;
  for(unsigned int row=0,ch=0,rw=0,beta=0,rowwc=0;
      ;
      beta++)
  {
    /* still on the same row */
    if(img[beta].row == row)
    {
      rw+=img[beta].w;
      rowwc+=img[beta].wc;

      if(img[beta].h > ch)
        ch=img[beta].h;
    }

    /* new row or last image */
    if(img[beta].row != row || beta == lst_count-1)
    {
      if(rw > canvas_w)
        canvas_w=rw;

      /* if there is empty space in the row */
      if(rowwc < seqc->prp.cols)
      {
        unsigned int pos=(img[beta].row != row) ? beta-1 : beta;

        print_debug("%s: TYHJÄÄ TILAA rwc %d cols %d ja pos %d\n",
          THIS_FUNCTION,rowwc,seqc->prp.cols,pos);

        /* give the last image the extra width */
        img[pos].wc+=seqc->prp.cols-rowwc;
      }

      canvas_h+=ch;
      row++;

      rw=ch=0;

      rowwc=img[beta].wc;

      /* if extra row or no more images */
      if(row == seqc->prp.rows || beta == lst_count-1)
        break;

      /* re-evaluate the current image now that the row has been changed */
      beta--;
    }
  }

  print_debug("%s: img_mean [%dx%d] canvas [%dx%d] -> ",
    THIS_FUNCTION,img_w_mean,img_h_mean,canvas_w,canvas_h);

  /* resize the canvas */
  if(seqc->prp.draw_fitstyle == SEQU_CFG_DRAW_CANVAS_SCALE)
  {
    canvas_resize_w=canvas_w*seqc->prp.draw_scale_factor;
    canvas_resize_h=canvas_h*seqc->prp.draw_scale_factor;
  }
  else
    /* fit the canvas into the viewport */
    image_fit(canvas_w,canvas_h,seqc->prp.view_width,seqc->prp.view_height,
      &canvas_resize_w,&canvas_resize_h,
      seqc->prp.draw_fitstyle-1);

  /* create/resize the canvas image */
  {
    unsigned int canvas_real_w=(canvas_resize_w < seqc->prp.view_width) ? 
      seqc->prp.view_width : canvas_resize_w;
    unsigned int canvas_real_h=(canvas_resize_h < seqc->prp.view_height) ? 
      seqc->prp.view_height : canvas_resize_h;

    lib=imagelist_get_image_lib(list);
    seqc->canvas=lib->image_resize(seqc->canvas,
      canvas_real_w,canvas_real_h);
    lib->blank_image(seqc->canvas);
  }

  print_debug("[%dx%d] viewport [%dx%d]",canvas_resize_w,canvas_resize_h,
    seqc->prp.view_width,seqc->prp.view_height);

  /* the slots the images will be fitted into */
  if(seqc->prp.draw_fitstyle == SEQU_CFG_DRAW_CANVAS_FIT)
  {
    slot_w=seqc->prp.view_width/seqc->prp.cols;
    slot_h=seqc->prp.view_height/seqc->prp.rows;
  }
  else
  {
    slot_w=canvas_resize_w/seqc->prp.cols;
    slot_h=canvas_resize_h/seqc->prp.rows;
  }

  print_debug(" slot [%dx%d]\n",slot_w,slot_h);

#if 0
  /* center the canvas into the viewport if canvas is smaller */
  if(seqc->prp.view_width >= slot_w*seqc->prp.cols)
    basex=(seqc->prp.view_width-slot_w*seqc->prp.cols)/2;
  if(seqc->prp.view_height >= slot_h*seqc->prp.rows)
    basey=(seqc->prp.view_height-slot_h*seqc->prp.rows)/2;
#endif

  /* center the canvas into the viewport if canvas is smaller */
  if(seqc->prp.view_width > canvas_resize_w)
    basex=(seqc->prp.view_width-canvas_resize_w)/2;
  if(seqc->prp.view_height > canvas_resize_h)
    //    basey=(seqc->prp.view_height-slot_h*seqc->prp.rows)/2;
    basey=(seqc->prp.view_height-canvas_resize_h)/2;
  

  print_debug("%s: Alussa base [%dx%d]\n",THIS_FUNCTION,basex,basey);

  for(unsigned int beta=0,tmpw=0,tmph=0,row=0,origx=basex,maxh=0;
      beta < lst_count && img[beta].row < seqc->prp.rows;
      beta++)
  {
  /* scale the images */
    image_fit(img[beta].w,img[beta].h,
      slot_w*img[beta].wc,slot_h,
      &tmpw,&tmph,0);

    print_debug(" %%%d: [%dx%d]=>[%dx%d]",beta,
      img[beta].w,img[beta].h,tmpw,tmph);

    img[beta].w=tmpw;
    img[beta].h=tmph;

    /* set the coordinates of the images */
    if(img[beta].row != row)
    {
      row++;
      basey+=maxh;
      maxh=0;
      //      basey+=slot_h;
      basex=origx;
    }

    img[beta].x=basex;
    img[beta].y=basey;
    
    //    basex+=slot_w*img[beta].wc;
    basex+=img[beta].w;
    if(maxh<img[beta].h)
      maxh=img[beta].h;

    print_debug(" @ %dx%d\n",img[beta].x,img[beta].y);

    /* draw the images */
    lib->blend_to_image(list->images[beta+lst_start],seqc->canvas,
      img[beta].x,img[beta].y,img[beta].w,img[beta].h);
  }

  nullify(img);

  return TRUE;
}

/* renders the canvas image onto an X drawable. Updates the viewport 
   position */
tvalue sequ_canvas_draw(sequ_canvas *seqc, 
  float view_rel_inc_x, float view_rel_inc_y)
{
  unsigned int x,y;

  if(!seqc)
    return FALSE;

  if(!seqc->canvas)
    return TRUE;

#define WITHIN_LIMITS(value,low,high) \
(((value) < (low)) ? (low) : ((value) > (high)) ? (high) : (value))

  /* update the viewport position */
  seqc->prp.view_rel_x=WITHIN_LIMITS(seqc->prp.view_rel_x+view_rel_inc_x,
    0.0f,1.0f);
  seqc->prp.view_rel_y=WITHIN_LIMITS(seqc->prp.view_rel_y+view_rel_inc_y,
    0.0f,1.0f);

  /* convert to real coordinates */
  x=(unsigned int)(seqc->prp.view_rel_x*
    (float)(seqc->canvas->width-seqc->prp.view_width));
  y=(unsigned int)(seqc->prp.view_rel_y*
    (float)(seqc->canvas->height-seqc->prp.view_height));

  /* render the canvas */
  return seqc->canvas->lib->
    blend_to_drawable(seqc->canvas,seqc->prp.drawable,
      x,y,seqc->prp.view_width,seqc->prp.view_height,
      0,0,seqc->prp.view_width,seqc->prp.view_height);
}
