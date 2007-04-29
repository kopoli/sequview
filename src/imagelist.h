/***************************************************************************
  Name:         imagelist.h
  Description:  A list of images
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


#ifndef IMAGELIST_HEADER
#define IMAGELIST_HEADER

#include "imagelib.h"
#include "filelist.h"

/* 2 images of threshold before changing direction */
#define IMAGELIST_DIRECTION_THRESH 2

typedef struct
{
  file_list *archive;

  /* the number of elements in both positions and images. */
  unsigned int count;

  /* position in archive. -1 is the invalid value */
  int *positions;

  /* if positions[n] != -1 then images[n] != NULL */
  sequ_image **images;

  /* how many images are visible at maximum */
  unsigned int images_visible;

  /* if wide images are considered as 2 images */
  tvalue wideimages;

  /* the private data */
  void *privdata;

} image_list;


inline tvalue imagelist_valid(image_list *list);

image_list *imagelist_create(image_list *old,char *file,sequ_image_lib *lib,
  unsigned int allocated, unsigned int visible, tvalue wideimages);
tvalue imagelist_delete(image_list *list);

sequ_image_lib *imagelist_get_image_lib(image_list *imagelist);

tvalue imagelist_page_set(image_list *list,unsigned int page);
tvalue imagelist_page_set_diff(image_list *list,int diff);
tvalue imagelist_page_set_last(image_list *list);

tvalue imagelist_load_empty(image_list *list);

int imagelist_get_pos(image_list *imagelist);
int imagelist_page2image(image_list *imagelist, unsigned int page);
int imagelist_first_drawn_image(image_list *list);

#endif
