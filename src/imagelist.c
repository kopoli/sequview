/***************************************************************************
  Name:         imagelist.c
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

#include <stdlib.h>

#include "common/defines.h"
#include "common/iolet.h"
#include "common/check_failure.h"

#include "configvars.h"
#include "filelist.h"
#include "imagelib.h"
#include "imagelist.h"

typedef struct 
{
  /*  which direction has the off-screen bitmaps. -1 left, +1 right. */
  char direction;

  /* used when changing the direction. The number of images shifted 
     in a direction. If this is bigger than the threshold, 
     direction is changed. */
  int times_shifted;

  /* Last position of the first image in the filelist, when last
     shifted*/
  unsigned int last_pos;

  /* pointer to the image library used */
  sequ_image_lib *lib;

}image_list_private;

static tvalue imagelist_flush(image_list *list);

inline tvalue imagelist_valid(image_list *list)
{
  return (list && list->positions && list->images && list->privdata &&
    file_list_valid(list->archive));
}

/* resizes the imagelist->images and imagelist->positions */
static tvalue imagelist_resize(image_list *imagelist, unsigned int allocated, 
  unsigned int visible)
{
  register unsigned int beta;
  sequ_image **tmpimg;
  int *tmppos;
  unsigned int realcount;

  ARG_ASSERT(allocated == 0,FALSE);

  imagelist->images_visible=visible;

  /* make sure that the number of allocated images is not greater than 
     the number of images in the filelist */
  realcount=get_file_list_count(imagelist->archive);

  if(realcount<allocated)
    allocated=realcount;

  /* nothing needs to be resized. (when using count as 0 
     during the creation of an image_list this test is passed). */
  if(allocated == imagelist->count)
    return TRUE;

  print_debug("%s: MUUTETATAAN imagelistin kokoa: ennen %d@%d nyt %d@%d\n",
    THIS_FUNCTION,imagelist->images_visible,imagelist->count,
    visible,allocated);

  /* (re)allocate */
  tmppos=malloc(sizeof(int)*allocated);
  tmpimg=malloc(sizeof(sequ_image *)*allocated);

  /* initialize */
  memset(tmpimg,0,sizeof(sequ_image *)*allocated);
  for(beta=0;beta<allocated;beta++)
    tmppos[beta]=-1;

  /* if there is old data */
  if(imagelist->count != 0)
  {
    unsigned int length=(allocated < imagelist->count) ?
      allocated : imagelist->count;

    /* copy the old data */
    memcpy(tmppos,imagelist->positions,sizeof(int)*length);
    memcpy(tmpimg,imagelist->images,sizeof(sequ_image *)*length);

    print_debug("%s: alloc %d count %d ja joudutaanko vapauttamaan %d\n",
      THIS_FUNCTION,allocated, imagelist->count, allocated < imagelist->count);

    /* free the extra images */
    if(allocated < imagelist->count)
      for(beta=allocated;beta<imagelist->count;beta++)
        if(imagelist->images[beta])
          imagelist->images[beta]->lib->image_remove(imagelist->images[beta]);
  }

  nullify(imagelist->positions);
  nullify(imagelist->images);

  imagelist->positions=tmppos;
  imagelist->images=tmpimg;
  imagelist->count=allocated;

  return TRUE;
}

/* creates an image_list and a filelist associated to the filename file.
   allocated is the number of images that can be simultaneously loaded,
   visible is the number of images drawn (for example, when drawing the
   imagelist if the number of loaded images is greater than this, the 
   imagelist can be drawn without loading anything). If wideimages is true
   wide images are considered to be 2 images.
*/
image_list *imagelist_create(image_list *old,char *file,sequ_image_lib *lib,
  unsigned int allocated, unsigned int visible, tvalue wideimages)
{
  file_list *fl=NULL;
  image_list *ret;
  image_list_private *priv;

  ARG_ASSERT(!file || allocated == 0,NULL);

  if(!imagelist_valid(old) || strcmp(old->archive->path,file) != 0)
    if((fl=get_file_list(file)) == NULL)
      return NULL;

  /* existing data */
  if(imagelist_valid(old))
  {
    /* if changed a file */
    if(fl)
    {
      unsigned int beta;

      imagelist_flush(old);
      old->archive=fl;

      /* clear the list */
      memset(old->images,0,sizeof(sequ_image *)*old->count);
      for(beta=0;beta<allocated;beta++)
        old->positions[beta]=-1;
    }

    ret=old;
    priv=old->privdata;
  }
  else
  {
    ret=malloc(sizeof(image_list));
    priv=malloc(sizeof(image_list_private));
    memset(ret,0,sizeof(image_list));
    memset(priv,0,sizeof(image_list_private));
    
    ret->archive=fl;
    ret->privdata=priv;
  }

  /* allocate the imagelists and set the diameters */
  imagelist_resize(ret,allocated,visible);

  /* other public */
  ret->wideimages=wideimages;

  /* set the private values */
  if(fl)
  {
    priv->direction=1;
    priv->times_shifted=0;
    priv->last_pos=0;

  }
  priv->lib=lib;

  /* load images */
  imagelist_page_set(ret,priv->last_pos);

  return ret;
}

/* frees the images and the filelist, saves the structure */
static tvalue imagelist_flush(image_list *list)
{
  register unsigned int beta;

  ARG_ASSERT(!list,FALSE);

  for(beta=0;beta<list->count;beta++)
    if(list->images[beta])
      list->images[beta]->lib->image_remove(list->images[beta]);

  file_list_delete(list->archive);

  return TRUE;
}

/* deletes also the associated file_list */
tvalue imagelist_delete(image_list *list)
{
  if(!imagelist_flush(list))
    return FALSE;

  nullify(list->images);
  nullify(list->positions);
  nullify(list->privdata);
  nullify(list);

  return TRUE;
}

/* takes the page position and returns the position in the list of images */
int imagelist_page2image(image_list *imagelist, unsigned int page)
{
  register unsigned int beta;

  for(beta=0;beta<imagelist->count;beta++)
    if(imagelist->positions[beta] == page)
      return beta;

  return -1;
}

/* this checks if imagelist can be drawn without loading images */
static tvalue imagelist_is_perfect(image_list *imagelist,
  unsigned int filepos, unsigned int imgs_drawn,
  unsigned int filecount)
{
  register unsigned int beta;
  unsigned int last,pos,count;

  /* get to the the file position which has the number of filepos */
  for(beta=0;beta<imagelist->count;beta++)
    if(imagelist->positions[beta] == filepos)
      break;

  /* not enough images in the list */
  if(imgs_drawn+beta > imagelist->count)
    return FALSE;

  last=(imgs_drawn+filepos > filecount-1) ? filecount-1 : imgs_drawn+filepos;
  pos=filepos+1;
  count=1;
  beta++;

  for(;beta<imagelist->count && pos<last && count<imgs_drawn;beta++)
  {
print_debug("%s: etsitään %d ja nykyinen %d \n",THIS_FUNCTION,pos,
  imagelist->positions[beta]);

    /* the image must also be in ascending order */
    if(imagelist->positions[beta] == pos)
    {
      pos++;
      count++;
    }
    else
      return FALSE;

    /* wide images take extra space */
    if(imagelist->wideimages && IMAGE_IS_WIDE(imagelist->images[beta]))
      count++;
  }

print_debug("%s: laskujen mukaan piirrettäviä kuvia on %d kpl kun vaad. %d\n",
  THIS_FUNCTION,count,imgs_drawn);

  return TRUE;
}

/* Shifts and loads images as needed. filepos is the position in the
  file list. imgs_drawn is the number of
  images are to be drawn (this decides wether images should be loaded or not).
  If imgs_drawn is negative, this loads the list full of images.
  Return values: -1 if error, 0 success nothing was loaded, 1 success
  something was loaded */
static int imagelist_shift_load_images(image_list *imagelist,
  unsigned int filepos, int imgs_drawn)
{
  register unsigned int beta;
  unsigned int filecount,start,end;
  int tmp,diff;

  image_list_private *priv;

  ARG_ASSERT(!imagelist_valid(imagelist),-1);

  priv=(image_list_private *)imagelist->privdata;

  filecount=get_file_list_count(imagelist->archive);
  if(filecount <= filepos)
    return -1;

  print_debug("%s: filepos %d imgsdrawn %d\n",THIS_FUNCTION,
    filepos,imgs_drawn); 

#if 0
  //does not work with wide images
  /* make sure that the screen has always (especially in the end) a full
     complement of drawn images */
  if(imgs_drawn > 0 && filepos > filecount-imgs_drawn)
    filepos=filecount-imgs_drawn;
#endif 

  /* check if nothing needs to be loaded. */
  if(imgs_drawn > 0 && imagelist_is_perfect(imagelist,
    filepos,imgs_drawn,filecount) == TRUE)
  {
    priv->last_pos=filepos;
    return 0;
  }

  /* check if the offscreen images should be before or after
     the visible ones.*/ 
  /* if position changed */
  if((diff=filepos-priv->last_pos) != 0)
  {
    int lasts=priv->times_shifted;

    priv->last_pos=filepos;

    print_debug("!!!!!!!!!!%s: diff %d last %d ts %d\n",THIS_FUNCTION,diff,
      priv->last_pos,priv->times_shifted);

    /* if direction was not changed */
    if(priv->times_shifted*diff >= 0)
    {
      int dir=0;
      priv->times_shifted+=diff;

      /* make sure that times_shifted does not overflow */
      dir=(priv->times_shifted > 0) ? 1 : -1;
      if(abs(priv->times_shifted) > IMAGELIST_DIRECTION_THRESH)
        priv->times_shifted=IMAGELIST_DIRECTION_THRESH*dir;
    }
    /* shift into opposite direction */
    else 
      priv->times_shifted=diff;

    print_debug("!!!!!!!!!!%s: nyt ts %d lasts %d\n",THIS_FUNCTION,
      priv->times_shifted,lasts);

    /* determine the new direction */
    if(priv->times_shifted <= -IMAGELIST_DIRECTION_THRESH)
      priv->direction=-1;
    else if(priv->times_shifted >= IMAGELIST_DIRECTION_THRESH)
      priv->direction=1;
    
    print_debug("!!!!!!!!!!!!!%s: suunta on %d\n",THIS_FUNCTION,
      priv->direction);
  }

  /* calculate the start and endpositions in the filelist */
  start=(filepos > filecount-imagelist->count) ? filecount-imagelist->count :
    filepos;
  end=start+imagelist->count;
  if(end > filecount)
    end=filecount;
  end--;

  /* correct the start and end if direction is negative  */
  if(priv->direction == -1 && 
    start > (diff=imagelist->count-imagelist->images_visible))
  {
    start-=diff;
    end-=diff;
  }

  print_debug("%s: start %d end %d diff %d\n",THIS_FUNCTION,start,end,diff);

  /* free the images that are left out */
  for(beta=0;beta<imagelist->count;beta++)
  {
    tmp=imagelist->positions[beta];

    print_debug("  pos %d img %d\n",beta,tmp);

    if(tmp == -1)
      continue;

    /* free images if out of bounds */
    if(tmp < start || tmp > end)
    {

      print_debug("%s: deleting %d image %d @ %p\n",THIS_FUNCTION,beta,tmp,
        imagelist->images[beta]);

      imagelist->images[beta]->lib->image_remove(imagelist->images[beta]); 
      imagelist->positions[beta]=-1;
      imagelist->images[beta]=NULL;
    }
  }

  {
    register unsigned int gamma;
    sequ_image * tmpimg; int tmppos;
    unsigned int loadcount=0;
    char *filename;

    /* if the list will be loaded fully */
    if(imgs_drawn == -1)
      imgs_drawn=imagelist->count;

    /* shift the images */
    for(gamma=start;gamma<=end;gamma++)
      for(beta=0;beta<imagelist->count;beta++)
        if(beta != gamma-start && imagelist->positions[beta] == gamma)
        {
          print_debug("   [%d:%d] <-> [%d:%d]\n",
            beta,imagelist->positions[beta],gamma-start,
            imagelist->positions[gamma-start]);

#define SWAP(a,b,tmpv) do {(tmpv)=(a);(a)=(b);(b)=(tmpv);}while(0)

          SWAP(imagelist->images[gamma-start],
            imagelist->images[beta],tmpimg);
          SWAP(imagelist->positions[gamma-start],
            imagelist->positions[beta],tmppos);
        }

    print_debug("%s: shiftauksen jälkeen lista:\n",THIS_FUNCTION);
    for(beta=0;beta<imagelist->count;beta++)
      print_debug("  pos %d kuva %d\n",beta,imagelist->positions[beta]);

    /* load images */
    for(beta=0;beta<imagelist->count;beta++)
      /* load only images on empty places and a certain amount */
      if(loadcount<imgs_drawn && loadcount<filecount && 
        imagelist->positions[beta] == -1)
      {

        print_debug("%s: ladataan kuva %d: %s paikkaan %d\n",THIS_FUNCTION,
          beta+start,imagelist->archive->files[beta+start],beta);

        /* decompress and load the image */
        if((filename=file_list_get_file(imagelist->archive,beta+start)) == NULL
          || (imagelist->images[beta]=priv->lib->image_open(filename)) == NULL)
        {
          print_err("Error: Could not load image \"%s\"\n",
            file_list_get_filename(imagelist->archive,beta+start));
          imagelist->images[beta]=NULL;
          continue;
        }

        imagelist->positions[beta]=beta+start;
        loadcount++;
      }
  }

  return 1;
}

int imagelist_get_pos(image_list *list)
{
  if(!imagelist_valid(list))
    return -1;

  return ((image_list_private *)list->privdata)->last_pos;
}

sequ_image_lib *imagelist_get_image_lib(image_list *list)
{
  if(!imagelist_valid(list))
    return NULL;

  return ((image_list_private *)list->privdata)->lib;
}

/* functions for navigation */
tvalue imagelist_page_set(image_list *list,unsigned int page)
{
  if(!list)
    return FALSE;

  if(imagelist_shift_load_images(list,page,list->images_visible) == -1)
    return FALSE;

  return TRUE;
}

tvalue imagelist_page_set_diff(image_list *list,int diff)
{
  if(!list)
    return FALSE;

  if(imagelist_shift_load_images(list,
       ((image_list_private *)list->privdata)->last_pos+diff,
       list->images_visible) == -1)
    return FALSE;
  
  return TRUE;
}

tvalue imagelist_page_set_last(image_list *list)
{
  if(!list)
    return FALSE;

  if(imagelist_shift_load_images(list,
       get_file_list_count(list->archive)-1,list->images_visible) == -1)
    return FALSE;

  return TRUE;
}

/* loads images to empty places */
tvalue imagelist_load_empty(image_list *list)
{
  if(!list)
    return FALSE;

  if(imagelist_shift_load_images(list,
       ((image_list_private *)list->privdata)->last_pos,-1) == -1)
    return FALSE;

  return TRUE;
}

/* returns the number of the first image drawn from *positions */
int imagelist_first_drawn_image(image_list *list)
{
  if(!imagelist_valid(list))
    return 0;

#if 0
#warning tämä ottaa huomioon vain suunnan muutoksen. Entä tiedoston lopussa ?
  /* if the direction is -1, the offscreen images are first */
  if(((image_list_private *)list->privdata)->direction == -1)
    return list->count-list->images_visible;
#endif

  for(unsigned int beta=0;beta<list->count;beta++)
  {
    print_debug("***etitään positiota %d kun %d\n",
      ((image_list_private *)list->privdata)->last_pos,
      list->positions[beta]);
    if(((image_list_private *)list->privdata)->last_pos == 
      list->positions[beta])
      return beta;
  }

  return 0;
}
