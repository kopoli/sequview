/***************************************************************************
  Name:         gtk2config.c
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

#include <string.h>
#include <errno.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <common/iolet.h>

#include "configvars.h"
#include "sequconfig.h"
#include "gtk2config.h"

#include "archive.h"
#include "im2int.h"

/*
  gtkconfig suunnitelma
  -dynaamiseksi nämä kaikki, eli dialogin lataus (staticit) ja nuo sivut.
  -tuo gtk2confpages.c kannattaa yhdistää tähän.
  -tuohon configuration_dialog rakenteeseen laitetaan kaikki tarvittava
  
  -voidaan kopioida nuo konffisivut templatesta, eli nuo osoittimet kuntoon
   -private-data generoidaan createfunktiossa.
  -configuration_page *pages on terminoitu lista osoittimia joiden privdata
   poistetaan deletefunktioilla ja lista freellä.

*/

#define CONFDIALOG_WIDTH  520
#define CONFDIALOG_HEIGHT 400
#define CONFDIALOG_FRAME_BORDER 3

/****************************************************************************
  Configuration pages
 ****************************************************************************/

/****************************************************************************
  Controls
 ****************************************************************************/

typedef struct
{
  GtkWidget **keys;
 } conf_controls_internal;


static gboolean conf_controls_detect_key(GtkWidget *widget,
  GdkEvent *event,gpointer data)
{
  if(event->type == GDK_KEY_PRESS)
  {
    GtkEntry *entry=GTK_ENTRY(data);
    char *tmp;
    GdkEventKey *evt=(GdkEventKey *)event;

    /* if the key is a modifier key (these are guesses) */
    if((evt->keyval >= GDK_Shift_L && evt->keyval <= GDK_Hyper_R) ||
      (evt->keyval == GDK_ISO_Level3_Shift))
      return FALSE;

    if(evt->keyval == GDK_Escape)
    {
      gtk_widget_hide(widget);
      return FALSE;
    }

    tmp=gtk_accelerator_name(evt->keyval,evt->state);

    print_debug("%s: str [%s] key %d state %d type %d\n",THIS_FUNCTION,
      evt->string,evt->keyval,evt->state,evt->type);
    print_debug("%s: sama accelina: [%s]\n",THIS_FUNCTION,tmp);

    /* set the text */
    gtk_entry_set_text(entry,tmp);

    nullify(tmp);

    gtk_widget_hide(widget);
  }

  return FALSE;
}

static GtkWidget *conf_controls_create_detect_wnd(gpointer data)
{
  GtkWidget *ret;

  /* the key detection window */
  ret=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_decorated(GTK_WINDOW(ret),FALSE);
  gtk_window_set_position(GTK_WINDOW(ret),
    GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_set_default_size(GTK_WINDOW(ret),100,100);
  gtk_container_add(GTK_CONTAINER(ret),
    gtk_label_new("Enter any key\n ESC to cancel."));
  gtk_window_set_modal(GTK_WINDOW(ret),TRUE);

  g_signal_connect(GTK_OBJECT(ret), "event",
    G_CALLBACK(conf_controls_detect_key), data);

  return ret;
}

static void conf_controls_read_pressed(GtkButton *button, gpointer data)
{
  GtkWidget *detect=conf_controls_create_detect_wnd(data);

  gtk_window_activate_focus(GTK_WINDOW(detect));
  gtk_widget_show_all(detect);
}

static tvalue conf_controls_create(configuration_page *pg)
{
  GtkWidget *frame,*vbox,*hbox,*btn;
  unsigned int ccilen=0;
  conf_controls_internal *cci;

  cci=malloc(sizeof(conf_controls_internal));

  for(;sequ_config_gtk_gui_keys_names[ccilen];ccilen++) 
    ;

  cci->keys=malloc(sizeof(GtkWidget *)*(ccilen+1));
  memset(cci->keys,sizeof(GtkWidget *)*(ccilen+1),0);
  /*
  cci->keys=malloc(sizeof(struct skey)*(ccilen+1));
  memset(cci->keys,sizeof(struct skey)*(ccilen+1),0);
  */

  vbox=gtk_vbox_new(TRUE,0);
  frame=gtk_frame_new("Key configuration");
  gtk_container_add(GTK_CONTAINER(frame),vbox);
  gtk_container_set_border_width(GTK_CONTAINER(frame),CONFDIALOG_FRAME_BORDER);

#if 0
  /* the key detection window */
  cci->detect_wnd=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_decorated(GTK_WINDOW(cci->detect_wnd),FALSE);
  gtk_window_set_position(GTK_WINDOW(cci->detect_wnd),
    GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_set_default_size(GTK_WINDOW(cci->detect_wnd),100,100);
  gtk_container_add(GTK_CONTAINER(cci->detect_wnd),
    gtk_label_new("Enter any key\n ESC to cancel."));
  gtk_window_set_modal(GTK_WINDOW(cci->detect_wnd),TRUE);

  g_signal_connect(GTK_OBJECT(cci->detect_wnd), "event",
    G_CALLBACK(conf_controls_detect_key), cci);
#endif

  /*  */
  for(unsigned int beta=0;sequ_config_gtk_gui_keys_names[beta]!=NULL;beta++)
  {
    hbox=gtk_hbox_new(TRUE,0);

    gtk_container_add(GTK_CONTAINER(hbox),
      gtk_label_new(sequ_config_gtk_gui_keys_names[beta]));

    cci->keys[beta]=gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(cci->keys[beta]),
      sequ_config_gtk_gui_keys[beta]);

    gtk_container_add(GTK_CONTAINER(hbox),cci->keys[beta]);

    btn=gtk_button_new_with_label("Read key");
    g_signal_connect(GTK_OBJECT(btn),"clicked",
      G_CALLBACK(conf_controls_read_pressed),cci->keys[beta]);

    gtk_container_add(GTK_CONTAINER(hbox),btn);
    gtk_container_add(GTK_CONTAINER(vbox),hbox);    
  }

  pg->page=frame;
  pg->pagedata=cci;
  return FALSE;
}

static void conf_controls_update(configuration_page *pg)
{
  conf_controls_internal *cci;
  unsigned int keylen=0;

  cci=(conf_controls_internal *)pg->pagedata;

  /* free old keys */
  for(;sequ_config_gtk_gui_keys[keylen]!=NULL;keylen++)
    free(sequ_config_gtk_gui_keys[keylen]);

  /* write the new */
  for(unsigned int beta=0;beta<keylen;beta++)
    sequ_config_gtk_gui_keys[beta]=strdup(
      gtk_entry_get_text(GTK_ENTRY(cci->keys[beta])));
}

static void conf_controls_remove(configuration_page *pg)
{
  conf_controls_internal *cci=(conf_controls_internal *)pg->pagedata;

  nullify(cci->keys);
  nullify(cci);
}

/****************************************************************************
  imlib2 configuration
 ****************************************************************************/

typedef struct 
{
  GtkWidget *imlib2_conf_cache;
  GtkWidget *imlib2_conf_dither;
  GtkWidget *imlib2_conf_antialias;
  GtkWidget *imlib2_conf_colorusage;
  GtkWidget *images_conf_extra_imgs;
  GtkWidget *images_conf_rows;
  GtkWidget *images_conf_cols;
  GtkWidget *images_conf_drawfit;
  GtkWidget *images_conf_drawscale;
} conf_images_internal;


static void images_drawfit_onchange(GtkComboBox *combo, gpointer data)
{
  conf_images_internal *cii=(conf_images_internal *)data;
  gboolean scale=FALSE;

  if(gtk_combo_box_get_active(GTK_COMBO_BOX(cii->images_conf_drawfit)) == 
    SEQU_CFG_DRAW_CANVAS_SCALE)
    scale=TRUE;

  gtk_widget_set_sensitive(cii->images_conf_drawscale,scale);
}

static tvalue conf_imlib2_create(configuration_page *pg)
{
  conf_images_internal *cii;
  
  GtkWidget *frame_im2, *frame_img;
  GtkWidget *im2_vbox, *img_vbox;

  GtkWidget *box_tmp, *label_tmp;

  GtkObject *spin_adj;

  /* allocate the private structure */
  cii=malloc(sizeof(conf_images_internal));


  pg->page=gtk_vbox_new(TRUE,0);
  gtk_container_set_border_width(GTK_CONTAINER(pg->page),
    CONFDIALOG_FRAME_BORDER);

  frame_im2=gtk_frame_new("Imlib2");
  frame_img=gtk_frame_new("Images");

  gtk_box_pack_start(GTK_BOX(pg->page), frame_im2, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(pg->page), frame_img, FALSE, TRUE, 0);


  /* internal containers */
  im2_vbox=gtk_vbox_new(FALSE,2);
  img_vbox=gtk_vbox_new(FALSE,3);

  gtk_container_add(GTK_CONTAINER(frame_im2),im2_vbox);
  gtk_container_add(GTK_CONTAINER(frame_img),img_vbox);

  gtk_container_set_border_width(GTK_CONTAINER(im2_vbox),4);
  gtk_container_set_border_width(GTK_CONTAINER(img_vbox),4);

  /* Imlib2 cache */
  spin_adj=gtk_adjustment_new(sequ_config_imlib_cache_size,0,
    SEQU_CFG_IMLIB_CACHE_SIZE_MAX,1,10,10);
  cii->imlib2_conf_cache=gtk_spin_button_new(GTK_ADJUSTMENT(spin_adj),1,0);

  label_tmp=gtk_label_new(" Imlib2's internal cache size (MiB:s)");
  box_tmp=gtk_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(box_tmp), cii->imlib2_conf_cache, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box_tmp), label_tmp, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(im2_vbox), box_tmp, FALSE, TRUE, 0);

  /* dither */
  cii->imlib2_conf_dither=gtk_check_button_new_with_label("Dither images");
  gtk_box_pack_start(GTK_BOX(im2_vbox), cii->imlib2_conf_dither, 
    FALSE, TRUE, 0);

  /* antialias */
  cii->imlib2_conf_antialias=gtk_check_button_new_with_label("Use Anti-alias");
  gtk_box_pack_start(GTK_BOX(im2_vbox), cii->imlib2_conf_antialias, 
    FALSE, TRUE, 0);

  /* 8-bit color usage */
  cii->imlib2_conf_colorusage=gtk_combo_box_entry_new_text();
  gtk_combo_box_append_text(GTK_COMBO_BOX(cii->imlib2_conf_colorusage),"128");
  gtk_combo_box_append_text(GTK_COMBO_BOX(cii->imlib2_conf_colorusage),"256");
  gtk_widget_set_size_request(cii->imlib2_conf_colorusage, 65,-1);

  box_tmp=gtk_hbox_new(FALSE,0);
  label_tmp=gtk_label_new(" Color usage (8-bit desktops only)");
  gtk_box_pack_start(GTK_BOX(box_tmp), cii->imlib2_conf_colorusage, 
    FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box_tmp), label_tmp, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(im2_vbox), box_tmp, FALSE, TRUE, 0);

  
  /* extra images */
  spin_adj=gtk_adjustment_new(sequ_config_images_extra,1,
    SEQU_CFG_IMAGES_EXTRA_MAX,1,10,10);
  cii->images_conf_extra_imgs=gtk_spin_button_new(
    GTK_ADJUSTMENT(spin_adj),1,0);

  label_tmp=gtk_label_new(" Extra images");
  box_tmp=gtk_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(box_tmp), cii->images_conf_extra_imgs, 
    FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box_tmp), label_tmp, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(img_vbox), box_tmp, FALSE, TRUE, 0);

  /* Images drawn */
  spin_adj=gtk_adjustment_new(sequ_config_image_rows,1,
    SEQU_CFG_IMAGES_ROWS_MAX,1,10,10);
  cii->images_conf_rows=gtk_spin_button_new(GTK_ADJUSTMENT(spin_adj),1,0);
  spin_adj=gtk_adjustment_new(sequ_config_image_cols,0,
    SEQU_CFG_IMAGES_COLS_MAX,1,10,10);
  cii->images_conf_cols=gtk_spin_button_new(GTK_ADJUSTMENT(spin_adj),1,0);

  label_tmp=gtk_label_new(" Rows   ");
  box_tmp=gtk_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(box_tmp), cii->images_conf_rows, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box_tmp), label_tmp, FALSE, TRUE, 0);
  label_tmp=gtk_label_new(" Cols");
  gtk_box_pack_start(GTK_BOX(box_tmp), cii->images_conf_cols, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box_tmp), label_tmp, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(img_vbox), box_tmp, FALSE, TRUE, 0);

  /*
  g_signal_connect(G_OBJECT(cii->images_conf_rows), "value-changed",
    G_CALLBACK(canvas_diams_change), cii);

  g_signal_connect(G_OBJECT(cii->images_conf_cols), "value-changed",
    G_CALLBACK(canvas_diams_change), cii);
  */

  /* Fitting style */
  label_tmp=gtk_label_new(" Fitting style");
  box_tmp=gtk_hbox_new(FALSE,0);
  cii->images_conf_drawfit=gtk_combo_box_entry_new_text();
  gtk_combo_box_append_text(GTK_COMBO_BOX(cii->images_conf_drawfit),
    "Manual scale");
  gtk_combo_box_append_text(GTK_COMBO_BOX(cii->images_conf_drawfit),"Image");
  gtk_combo_box_append_text(GTK_COMBO_BOX(cii->images_conf_drawfit),"Width");
  gtk_combo_box_append_text(GTK_COMBO_BOX(cii->images_conf_drawfit),"Height");
  gtk_widget_set_size_request(cii->images_conf_drawfit, 120,-1);
  gtk_box_pack_start(GTK_BOX(box_tmp), cii->images_conf_drawfit, 
    FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box_tmp), label_tmp, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(img_vbox), box_tmp, FALSE, TRUE, 0);

  /* Scale factor */
  label_tmp=gtk_label_new(" Scale factor");
  box_tmp=gtk_hbox_new(FALSE,0);
  cii->images_conf_drawscale=gtk_entry_new();
  gtk_widget_set_size_request(cii->images_conf_drawscale, 120,-1);
  gtk_entry_set_max_length(GTK_ENTRY(cii->images_conf_drawscale),7);
  gtk_box_pack_start(GTK_BOX(box_tmp), cii->images_conf_drawscale, 
    FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box_tmp), label_tmp, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(img_vbox), box_tmp, FALSE, TRUE, 0);

  g_signal_connect(G_OBJECT(cii->images_conf_drawfit), "changed",
    G_CALLBACK(images_drawfit_onchange), cii);

  /* set the values */
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cii->imlib2_conf_dither),
    sequ_config_imlib_dither);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cii->imlib2_conf_antialias),
    sequ_config_imlib_anti_alias);
  gtk_combo_box_set_active(GTK_COMBO_BOX(cii->imlib2_conf_colorusage),
    (sequ_config_imlib_color_usage==128) ? 0 : 1);
  gtk_combo_box_set_active(GTK_COMBO_BOX(cii->images_conf_drawfit),
    sequ_config_draw_fit);

  {
    gchar str[10];
    snprintf(str,10,"%.4f",sequ_config_draw_scale);
    gtk_entry_set_text(GTK_ENTRY(cii->images_conf_drawscale),str);
  }  

  pg->pagedata=cii;
  return TRUE;
}

static void conf_imlib2_update(configuration_page *pg)
{
  const char *str;
  conf_images_internal *cii=(conf_images_internal *)pg->pagedata;

  /* get values from the widgets */
  sequ_config_imlib_dither= 
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cii->imlib2_conf_dither));
  sequ_config_imlib_anti_alias=gtk_toggle_button_get_active(
    GTK_TOGGLE_BUTTON(cii->imlib2_conf_antialias));
  sequ_config_imlib_color_usage=
    gtk_combo_box_get_active(GTK_COMBO_BOX(cii->imlib2_conf_colorusage)) == 0 ?
    128 : 256;
  sequ_config_draw_fit=
    gtk_combo_box_get_active(GTK_COMBO_BOX(cii->images_conf_drawfit));
  str=gtk_entry_get_text(GTK_ENTRY(cii->images_conf_drawscale));
  sequ_config_draw_scale=strtod(str,NULL);

print_debug("%s: skaalauksen jälkeen errno on: %s\n",THIS_FUNCTION,
  strerror(errno));

  sequ_config_imlib_cache_size=
    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(cii->imlib2_conf_cache));

  sequ_config_images_extra=gtk_spin_button_get_value_as_int(
    GTK_SPIN_BUTTON(cii->images_conf_extra_imgs));
  sequ_config_image_rows=
    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(cii->images_conf_rows));
  sequ_config_image_cols=
    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(cii->images_conf_cols));  

  im2_lib.init(NULL);
}

static void conf_imlib2_remove(configuration_page *pg)
{
  nullify(pg->pagedata);
}

/****************************************************************************
  Compressed files and types
 ****************************************************************************/

typedef struct 
{
  GtkWidget **list;
  GtkWidget **decompress;
} conf_filetypes_internal;


/* get the defaults */
static void ft_defaults_pressed(GtkButton *button, gpointer data)
{
  unsigned int beta;
  conf_filetypes_internal *cfi=(conf_filetypes_internal *)data;

  sequ_config_commands_default();

  for(beta=0;cfi->list[beta] != NULL;beta++)
  {
    gtk_entry_set_text(GTK_ENTRY(cfi->list[beta]),
      sequ_config_archive_cmds[beta].list);

    gtk_entry_set_text(GTK_ENTRY(cfi->decompress[beta]),
      sequ_config_archive_cmds[beta].decompress);
  }
}

static tvalue conf_filetypes_create(configuration_page *pg)
{
  unsigned int beta,len;

  GtkWidget *layout,*entry,*tmp;
  conf_filetypes_internal *cfi;

  for(len=0;archive_supported_formats[len].name != NULL;len++)
    ;

  cfi=malloc(sizeof(conf_filetypes_internal));
  cfi->list=malloc(sizeof(GtkWidget*)*(len+1));
  cfi->decompress=malloc(sizeof(GtkWidget*)*(len+1));

  cfi->list[len]=cfi->decompress[len]=NULL;

  layout=gtk_table_new(3,len,FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(layout),5);
  gtk_table_set_col_spacings(GTK_TABLE(layout),12);
  gtk_container_set_border_width(GTK_CONTAINER(layout),5);

#define ATTACH_TO_TABLE(table,widget,x,y) \
  gtk_table_attach(GTK_TABLE(table),GTK_WIDGET(widget), \
    x,x+1,y,y+1,GTK_SHRINK | GTK_EXPAND,0,0,0)

  /* the header */
  ATTACH_TO_TABLE(layout,gtk_label_new("Filetype"),0,0);
  ATTACH_TO_TABLE(layout,gtk_label_new("List"),1,0);
  ATTACH_TO_TABLE(layout,gtk_label_new("Decompress"),2,0);

  for(beta=0;archive_supported_formats[beta].name != NULL;beta++)
  {
    /* name */
    ATTACH_TO_TABLE(layout,
      gtk_label_new(archive_supported_formats[beta].name),0,beta+1);
  
    /* list command */
    entry=gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry),sequ_config_archive_cmds[beta].list);

    cfi->list[beta]=entry;

    ATTACH_TO_TABLE(layout,entry,1,beta+1);

    /* decompress command */
    entry=gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry),
      sequ_config_archive_cmds[beta].decompress);

    cfi->decompress[beta]=entry;

    ATTACH_TO_TABLE(layout,entry,2,beta+1);
  }

  tmp=gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(tmp),layout);
  layout=tmp;

  /* add some buttons */
  {
    GtkWidget *align,*defaults; 

    align=gtk_alignment_new(1,1,1,1);
    gtk_alignment_set_padding(GTK_ALIGNMENT(align),0,0,0,7);

    gtk_container_add(GTK_CONTAINER(layout),align);    
    
    tmp=gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(tmp),GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(tmp), 10);

    defaults=gtk_button_new_with_mnemonic("_Defaults");

    //    help=gtk_button_new_with_mnemonic("_Help");

    g_signal_connect(GTK_OBJECT(defaults),"clicked",
      G_CALLBACK(ft_defaults_pressed),cfi);
    /*
    g_signal_connect(GTK_OBJECT(help),"clicked",
      G_CALLBACK(ft_help_pressed),cfi);
    */

    gtk_container_add(GTK_CONTAINER(tmp),defaults);
    //    gtk_container_add(GTK_CONTAINER(tmp),help);
    gtk_container_add(GTK_CONTAINER(align),tmp);
  }

  pg->page=gtk_frame_new("Archive commands");
  gtk_container_add(GTK_CONTAINER(pg->page),layout);
  gtk_container_set_border_width(GTK_CONTAINER(pg->page),
    CONFDIALOG_FRAME_BORDER);

  pg->pagedata=cfi;

  return TRUE;
}

static void conf_filetypes_update(configuration_page *pg)
{
  unsigned int beta;

  conf_filetypes_internal *cfi=(conf_filetypes_internal *)pg->pagedata;

  for(beta=0;cfi->list[beta] != NULL;beta++)
  {
    nullify(sequ_config_archive_cmds[beta].list);
    nullify(sequ_config_archive_cmds[beta].decompress);

    sequ_config_archive_cmds[beta].list=
      strdup(gtk_entry_get_text(GTK_ENTRY(cfi->list[beta])));

    sequ_config_archive_cmds[beta].decompress=
      strdup(gtk_entry_get_text(GTK_ENTRY(cfi->decompress[beta])));
  }
}

static void conf_filetypes_remove(configuration_page *pg)
{
  unsigned int beta;
  conf_filetypes_internal *cfi=(conf_filetypes_internal *)pg->pagedata;

  for(beta=0;cfi->list[beta] != NULL;beta++)
  {
    free(cfi->list[beta]);
    free(cfi->decompress[beta]);
  }

  nullify(cfi);
}

/****************************************************************************
  The interface
 ****************************************************************************/

/* the template for config pages */
static const configuration_page config_pages_template[] =
{
  {"Controls",conf_controls_create,conf_controls_update,conf_controls_remove,
   NULL},
  {"Filetypes",conf_filetypes_create,
   conf_filetypes_update,conf_filetypes_remove,NULL},
  {"Images",conf_imlib2_create,conf_imlib2_update,conf_imlib2_remove,NULL},
  {NULL}
};


static configuration_page *config_pages_create()
{
  configuration_page *ret;
  unsigned int beta;

  ret=malloc(sizeof(config_pages_template));
  memcpy(ret,config_pages_template,sizeof(config_pages_template));

  for(beta=0;ret[beta].name != NULL;beta++)
  {
    ret[beta].create(&ret[beta]);

    /* make a reference to the created widget */
    g_object_ref(ret[beta].page);
  }

  return ret;
}

static void config_pages_delete(configuration_page *cfg)
{
  unsigned int beta;

  if(!cfg)
    return;

  for(beta=0;cfg[beta].name != NULL; beta++)
  {
    if(cfg[beta].remove)
      cfg[beta].remove(&cfg[beta]);
    
    gtk_widget_destroy(cfg[beta].page);
    g_object_unref(cfg[beta].page);
  }

  nullify(cfg);
}

/****************************************************************************
  The controls for the tree selector
 ****************************************************************************/

static void select_option_from_tree(gchar *selection,
 configuration_dialog *dlg)
{
  register unsigned int beta;

print_debug("Selected on tässä: %p\n",dlg->selected);

  if(dlg->selected != NULL)
  {
    gtk_container_remove(GTK_CONTAINER(dlg->dialog_contain),
      dlg->selected_page);
    g_free(dlg->selected);
  }

  dlg->selected=selection;

  for(beta=0;dlg->pages[beta].name != NULL;beta++)
    if(strcmp(dlg->pages[beta].name,selection) == 0)
    {
      /* assume that selection is found */
      dlg->selected_page=dlg->pages[beta].page;
      gtk_container_add(GTK_CONTAINER(dlg->dialog_contain),
        dlg->selected_page);

      gtk_widget_show_all(dlg->selected_page);
      break;
    }
}

static void option_tree_changed(GtkTreeSelection *sel, gpointer ptr)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
  gchar *selection;

  if(!gtk_tree_selection_get_selected(sel,&model,&iter))
    return;

  gtk_tree_model_get(model,&iter,0,&selection,-1);  

print_debug("Selectioni oli [%s]\n",selection);

  select_option_from_tree(selection,(configuration_dialog *)ptr);

}

static GtkWidget *create_option_tree(configuration_dialog *dlg)
{
  register unsigned int beta;
	GtkTreeStore *ts;
	GtkTreeSelection  *sel;
  GtkWidget *tree;
  GtkTreeIter iter;
  GtkTreeViewColumn *trevic;

  configuration_page *list=dlg->pages;
  
  ts=gtk_tree_store_new(1,G_TYPE_STRING);

  /* put the names of the configuration pages into the list */
  for(beta=0;list[beta].name != NULL;beta++)
  {
    gtk_tree_store_append(ts, &iter, NULL );
    gtk_tree_store_set(ts, &iter, 0, list[beta].name, -1);
  }

  tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ts));

  trevic=gtk_tree_view_column_new_with_attributes(NULL,
    gtk_cell_renderer_text_new(), "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree),trevic);

  gtk_widget_set_size_request(tree, 120, -1);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);
  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (tree), FALSE);

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
  gtk_tree_selection_set_mode(sel,GTK_SELECTION_SINGLE);

  g_signal_connect(G_OBJECT(sel), "changed", 
    G_CALLBACK(option_tree_changed), dlg);

  return tree;
}


/****************************************************************************
  The interface
 ****************************************************************************/

configuration_dialog *configuration_dialog_create()
{
  //  GtkWidget *dialog;
  GtkWidget *dialog_vbox1;
  GtkWidget *hbox1;
  GtkWidget *vbox1;
  GtkWidget *scroll;
  GtkWidget *tree;

  configuration_dialog *ret;

  ret=malloc(sizeof(configuration_dialog));
  memset(ret,0,sizeof(configuration_dialog));

  ret->pages=config_pages_create();

  /* the widgets */
  ret->dialog=gtk_dialog_new_with_buttons("Configuration",NULL,
    //    GTK_DIALOG_MODAL,
    GTK_DIALOG_DESTROY_WITH_PARENT,
    GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
    GTK_STOCK_OK, GTK_RESPONSE_OK,
    NULL);

  gtk_dialog_set_default_response(GTK_DIALOG(ret->dialog),GTK_RESPONSE_OK);

  //  config_pages_get_size(ret->pages,&width,&height);
  gtk_widget_set_size_request(ret->dialog,CONFDIALOG_WIDTH,CONFDIALOG_HEIGHT);


  dialog_vbox1 = GTK_DIALOG (ret->dialog)->vbox;

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_container_add(GTK_CONTAINER(dialog_vbox1),vbox1);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 0);

  /* the tree */
  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (hbox1), scroll, FALSE, TRUE, 0);
  gtk_widget_set_size_request (scroll, 120, -1);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), 
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), 
    GTK_SHADOW_IN);

  tree=create_option_tree(ret);
  gtk_container_add (GTK_CONTAINER (scroll), tree);

  /* set up the structure */
  ret->dialog_contain=hbox1;
  
  select_option_from_tree(strdup(ret->pages[0].name),ret);

  g_signal_connect(G_OBJECT(ret->dialog), "delete_event",
    G_CALLBACK(gtk_widget_hide), ret->dialog);

  return ret;
}

void configuration_dialog_update_cfg(configuration_dialog *dlg)
{
  if(!dlg)
    return;

  for(unsigned int beta=0;dlg->pages[beta].name != NULL; beta++)
    if(dlg->pages[beta].update)
      dlg->pages[beta].update(&dlg->pages[beta]);
  
  write_config(sequ_config_generated_config_file_path);    
}

void configuration_dialog_delete(configuration_dialog *dlg)
{
  if(!dlg)
    return;

  gtk_widget_destroy(GTK_WIDGET(dlg->dialog));
  nullify(dlg->selected);
  config_pages_delete(dlg->pages);
  nullify(dlg);
}
