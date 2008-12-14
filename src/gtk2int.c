/***************************************************************************
  Name:         gtk2int.c
  Description:  GTK+2 interface
  Created:      20060416
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


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libgen.h>

#include <common/defines.h>
#include <common/iolet.h>
#include <common/check_failure.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gui.h"
#include "archive.h"
#include "tmpfile.h"
#include "configvars.h"

#include "gdkpixbufint.h"
#include "im2int.h"

#include "imagelist.h"
#include "canvas.h"

#include "gtk2config.h"
#include "gtk2error.h"

#include "sequview.xpm"


/************************************************************************/
/* data structures */

/* the private data for sequ_gui */
typedef struct 
{
  /* widgets */
  GtkWindow *main;                       /* Main window */
  GtkMenu   *menu;                       /* Popup menu */
  GtkImage  *gtk_canvas;                 /* Canvas widget */
  GtkWidget *fileselector;               /* Fileselector */

  GtkAccelGroup *accels;                 /* Keyboard accelerators */

  /* the configuration dialog */
  configuration_dialog *config;

  /* Gdk */
  GdkPixmap *gdk_canvas;

  /* canvas */
  sequ_canvas *canvas;

  sequ_image_lib *lib;
  image_list *images;

  /* window states */
  tvalue main_maximized;
  tvalue main_iconified;
  tvalue main_fullscreen;

  /* file currently open */
  char *filename;

  /* the names of the keys for scrolling converted to keycodes */
  unsigned int scroll_keycodes[4];

} gtk2_gui;


/* transfer data and the context to a callback */
typedef struct
{
  /* context */
  sequ_gui *gui;

  /* payload */
  void *data;

} gtk2_gui_callback_data;

/**************************************************************************
  Public
 *************************************************************************/

/************************************************************************/
/* prototypes for the public section */

static tvalue gtk2_gui_init(struct sequ_gui *gui, int width, int height);
static tvalue gtk2_gui_run (struct sequ_gui *gui, char *filename);
static void   gtk2_gui_deinit(struct sequ_gui *gui);
static int    gtk2_gui_identify();

/************************************************************************/
/* the init functions */

/* How many gtk2_gui's have been created. This is incremented every 
   gtk2_gui_init -call and decremented every gtk2_gui_deinit -call.
   when this goes to 1, gtk_init_check is called and when to 0, 
   gtk_main_quit is called. */
static unsigned int gtk2_gui_count=0;

/* creates the gtk2_gui instance. argc and argv can only be NULL,
   when gtk is running (gtk_init_check has been called). */
sequ_gui *gtk2_gui_create(int *argc,char ***argv, int width, int height)
{
  sequ_gui *ret;
  gtk2_gui *gtkui;

  /* initialize gtk */
  if(gtk2_gui_count == 0)
    if(gtk_init_check(argc,argv) == FALSE)
      return NULL;

  /* allocate and create structures */
  CHECK_FAILURE(
    ret=malloc(sizeof(sequ_gui)),NULL,NULL
  );

  CHECK_FAILURE_ACT(
    gtkui=malloc(sizeof(gtk2_gui)),NULL,
      nullify(ret);
      return NULL;
  );

  memset(gtkui,0,sizeof(gtk2_gui));
  memset(ret,0,sizeof(sequ_gui));

  ret->init=gtk2_gui_init;
  ret->run=gtk2_gui_run;
  ret->deinit=gtk2_gui_deinit;
  ret->identify=gtk2_gui_identify;
  ret->private=gtkui;

  /* initialize imlib */
#warning katso mikä kirjasto on käytössä
  gtkui->lib=(sequ_image_lib *)&gdkpixbuf_lib;
  //  gtkui->lib=(sequ_image_lib *)&im2_lib;
  gtkui->lib->init(gdk_x11_get_default_xdisplay());

  ret->init(ret,width,height);

  return ret;
}

/* deletes the gui */
tvalue gtk2_gui_delete(sequ_gui *gui)
{
  if(gtk2_gui_count == 0)
    return TRUE;

  gui->deinit(gui);

  return TRUE;
}


/*************************************************************************
  Private
 ************************************************************************/

/************************************************************************/
/* creation prototypes */

static tvalue     gtk2_gui_create_main_window(sequ_gui *gui);
static tvalue     gtk2_gui_create_fileselector(sequ_gui *gui);
static tvalue     gtk2_gui_create_about_dialog();
static GtkWidget *gtk2_gui_create_main_menu(sequ_gui *gui,
  tvalue update_accels);

static tvalue gtk2_gui_resize_gdk_canvas(sequ_gui *gui,int width,int height);
static void gtk2_gui_set_window_title(GtkWindow *win,char *filename);

/************************************************************************/
/* callback prototypes */

/* the menu callback function template */
#define GTK2_GUI_CALLBACK_FUNC(name) gtk2_gui_callback_##name
#define GTK2_GUI_MENU_CALLBACK(name) \
  static void GTK2_GUI_CALLBACK_FUNC(name)\
    (GtkMenuItem *item,gpointer data)

static gint GTK2_GUI_CALLBACK_FUNC(main_window_delete)(
  GtkWidget *widget, GdkEvent *event,gpointer data);

static void GTK2_GUI_CALLBACK_FUNC(get_files)(GtkDialog *dialog, gint arg1,
  gpointer data);

static void GTK2_GUI_CALLBACK_FUNC(config_resp)(GtkDialog *dialog, gint arg1,
  gpointer data);

/* eventbox click */
static gboolean GTK2_GUI_CALLBACK_FUNC(eventbox_click)(GtkWidget *widget,
  GdkEventButton *event);

/* window status changed */
static gboolean GTK2_GUI_CALLBACK_FUNC(main_window_state_change)(
  GtkWidget *widget,GdkEvent *event,gpointer data);

/* generic eventhandler for the canvas */
static gboolean GTK2_GUI_CALLBACK_FUNC(canvas_eventhandler)(
  GtkWidget *widget,GdkEvent *event,gpointer data);

/* menu callbacks */
#ifdef DEBUG
GTK2_GUI_MENU_CALLBACK(execute_demo);
#endif
GTK2_GUI_MENU_CALLBACK(open_fileselector);
GTK2_GUI_MENU_CALLBACK(maximize);
GTK2_GUI_MENU_CALLBACK(iconify);
GTK2_GUI_MENU_CALLBACK(fullscreen);

GTK2_GUI_MENU_CALLBACK(page_first);
GTK2_GUI_MENU_CALLBACK(page_last);
GTK2_GUI_MENU_CALLBACK(page_forward);
GTK2_GUI_MENU_CALLBACK(page_backward);

GTK2_GUI_MENU_CALLBACK(open_about);
GTK2_GUI_MENU_CALLBACK(open_config);
GTK2_GUI_MENU_CALLBACK(quit);

/************************************************************************/
/* other prototypes */

static void gtk2_gui_scrolling_keys_to_codes(sequ_gui *gui);
static tvalue gtk2_gui_update_keys(sequ_gui *gui);

/************************************************************************/
/* static (common) widgets. These are common for all instances. */

//static GtkWindow *gtk2_gui_config=NULL;
static GtkWindow *gtk2_gui_about=NULL;


/* pointer conversion */
#define GTK2_GUI_CONVERT_GUI_POINTER()          \
  sequ_gui *gui=(sequ_gui *)data

#define GTK2_GUI_CONVERT_GTKUI_POINTER()        \
  gtk2_gui *gtkui=(gtk2_gui *)gui->private

/************************************************************************/
/* misc static functions */

/* draw the canvas and reload new images */
static void repaint_canvas(sequ_gui *gui,float rel_x,float rel_y)
{
  GTK2_GUI_CONVERT_GTKUI_POINTER();

  if(gtkui->canvas)
    sequ_canvas_draw(gtkui->canvas,rel_x,rel_y);

  if(gtkui->gtk_canvas)
    gtk_widget_queue_draw(GTK_WIDGET(gtkui->gtk_canvas));  
}

/* calculate positions for new images and redraw canvas */
static tvalue redraw_canvas(sequ_gui *gui, tvalue load,
  float rel_x,float rel_y)
{
  GTK2_GUI_CONVERT_GTKUI_POINTER();

  if(gtkui->canvas)
    sequ_canvas_realize(gtkui->canvas,gtkui->images);

  repaint_canvas(gui,rel_x,rel_y);

  if(load)
    return imagelist_load_empty(gtkui->images);

  return TRUE;
}

/* the delayed window redrawing */
static tvalue waiting_redraw=FALSE;
static gboolean redraw_event_timeout(gpointer data)
{
  GTK2_GUI_CONVERT_GUI_POINTER();
  GTK2_GUI_CONVERT_GTKUI_POINTER();

  sequ_canvas_props tmp;

  tmp.drawable=gdk_x11_drawable_get_xid(gtkui->gdk_canvas);
  tmp.view_width=gui->width;
  tmp.view_height=gui->height;

  sequ_canvas_update(gtkui->canvas,
    SEQU_CANVAS_PROP_DRAWABLE | SEQU_CANVAS_PROP_VIEWPORT,&tmp);

  redraw_canvas(gui,FALSE,0.0f,0.0f);

  waiting_redraw=FALSE;
  return FALSE;
}

/* image widget's management */
static tvalue gtk2_gui_resize_gdk_canvas(sequ_gui *gui,int width,int height)
{
  tvalue ret=FALSE;

  GTK2_GUI_CONVERT_GTKUI_POINTER();

  /*
  print_debug("%s: vanha %dx%d uusi %dx%d\n",
    THIS_FUNCTION,gui->width,gui->height,width,height);
  */

  /* if diameters were changed or initialization */
  if(gtkui->gdk_canvas == NULL || 
    gui->width != width || gui->height != height)
  {
    GdkGC *tmp;
    GdkColor black = {0,0,0,0};

    /* if only diameters were changed */
    if(gtkui->gdk_canvas != NULL)
      g_object_unref(G_OBJECT(gtkui->gdk_canvas));

    gtkui->gdk_canvas=gdk_pixmap_new(GTK_WIDGET(gtkui->main)->window,
      width,height,gdk_rgb_get_visual()->depth);

    /* blank the gdk_canvas */
    tmp=gdk_gc_new(gtkui->gdk_canvas);
    gdk_gc_set_foreground(tmp,&black);

    gdk_draw_rectangle(gtkui->gdk_canvas,tmp,TRUE,0,0,width,height);

    g_object_unref(G_OBJECT(tmp));

    ret=TRUE;
  }

  gui->width=width;
  gui->height=height;

  return ret;
}

static tvalue resize_canvas(sequ_gui *gui)
{
  GTK2_GUI_CONVERT_GTKUI_POINTER();
  int width,height;

  /*
  print_debug("%s: main %p gtkcanv %p gdkcanv %p\n",
    THIS_FUNCTION,gtkui->main,gtkui->gtk_canvas,gtkui->gdk_canvas);
  */

  gtk_window_get_size(gtkui->main,&width,&height);

  if(gtk2_gui_resize_gdk_canvas(gui,width,height) == FALSE)
    return FALSE;

  gtk_image_set_from_pixmap(gtkui->gtk_canvas,gtkui->gdk_canvas,NULL);

  /* redraw if empty window */
  if(!gtkui->canvas)
    gtk_widget_queue_draw(GTK_WIDGET(gtkui->gtk_canvas));

    /*
    while(gtk_events_pending())
      gtk_main_iteration();
    */

  /* redraw when images are on the canvas */
  else if(waiting_redraw == FALSE)
  {
    /* this forces some pause between the resize and redraw. This is for
       the window managers which opaquely resize windows. Other window 
       managers or resize-styles may experience a small flicker if the 
       drawn images are bright (because the default pixmap is black). */
    waiting_redraw=TRUE;
    //    g_timeout_add(50,redraw_event_timeout,gui);
    g_idle_add(redraw_event_timeout,gui);
  }

  return TRUE;
}

/* sequ_canvas layer resizes the pixmap */
static inline void resize_pixmap_callback(void *data,int width,int height)
{
  GTK2_GUI_CONVERT_GUI_POINTER();

  resize_canvas(gui);
}

/* set the window title */
static void gtk2_gui_set_window_title(GtkWindow *win,char *filename)
{
  char *str=sequ_config_gtk2int_mainwindow_title;

  if(filename)
  {
    unsigned int length=strlen(sequ_config_gtk2int_mainwindow_title)+
      strlen(filename)+3+1;

    if((str=malloc(length)) != NULL)
    {
      strcpy(str,filename);
      strcat(str," - ");
      strcat(str,sequ_config_gtk2int_mainwindow_title);
      gtk_window_set_title(win, str);
      nullify(str);
      return;
    }
  }

  gtk_window_set_title(win, str);

  if(filename)
    nullify(str);
}

/* calls sequ_canvas_load with the appropriate arguments*/
static tvalue gtk2_gui_load_create_canvas(sequ_gui *gui, char *file)
{
  GTK2_GUI_CONVERT_GTKUI_POINTER();

  sequ_canvas_props tmp;

  if(file == NULL && gtkui->canvas == NULL)
    return FALSE;

  print_debug("Luodaan imagelist!!!\n");

  if(file)
  {
    nullify(gtkui->filename);
    gtkui->filename=strdup(file);
    print_debug("LADATTAVA TIEDOSTO ON %s\n",gtkui->filename);    

  }

  /* create the imagelist */
  CHECK_FAILURE(
    gtkui->images=imagelist_create(gtkui->images,gtkui->filename,gtkui->lib,
      sequ_config_image_rows*sequ_config_image_cols+sequ_config_images_extra,
      sequ_config_image_rows*sequ_config_image_cols,
      sequ_config_draw_wide_as_2),
    NULL,FALSE
  );

  /* create the canvas */
  sequ_canvas_delete(gtkui->canvas);

  memset(&tmp,0,sizeof(sequ_canvas_props));

  tmp.drawable=gdk_x11_drawable_get_xid(gtkui->gdk_canvas);
  tmp.view_width=gui->width;
  tmp.view_height=gui->height;
  tmp.rows=sequ_config_image_rows;
  tmp.cols=sequ_config_image_cols;
  tmp.draw_fitstyle=sequ_config_draw_fit;
  tmp.draw_scale_factor=sequ_config_draw_scale;

  /*
  //  tmp.extra_images=sequ_config_images_extra;
  //  tmp.list=gtkui->images;
  tmp.resize_func=resize_pixmap_callback;
  tmp.resize_param=gui;
  //  tmp.wide=sequ_config_draw_wide_as_2;
  tmp.view_rel_x=0.0f;
  tmp.view_rel_y=0.0f;
  */

  CHECK_FAILURE(
    gtkui->canvas=sequ_canvas_create(&tmp),
    NULL,FALSE
  );            

  gtk2_gui_set_window_title(GTK_WINDOW(gtkui->main),gtkui->filename);

  return TRUE;
}

/* navigating the material */
/* the maximum number of pages that can be relatively skipped */
#define MAX_DIFF_PAGE_JUMP 10
/* the default relative jump */
#define DEFAULT_DIFF_PAGE_JUMP 1

/* change absolute page if pos >= 0. 
   change relative page backwards if -20 <= pos < -10
    and forwards if -10 <= pos < 0 */
static void change_page(gpointer data,int pos)
{
  GTK2_GUI_CONVERT_GUI_POINTER();
  GTK2_GUI_CONVERT_GTKUI_POINTER();

  /* get to the last page with this */
  if(pos < -2*MAX_DIFF_PAGE_JUMP)
    imagelist_page_set_last(gtkui->images);

  /* absolute page */
  else if(pos >= 0)
    imagelist_page_set(gtkui->images,pos);
  
  /* relative page */
  else
    imagelist_page_set_diff(gtkui->images,pos+MAX_DIFF_PAGE_JUMP);

  redraw_canvas(gui,TRUE,0.0f,0.0f);
}

/************************************************************************/
/* functions for the prototypes in sequ_gui */

static tvalue gtk2_gui_init(struct sequ_gui *gui, int width, int height)
{
  gtk2_gui *gtkui;

  gtkui=(gtk2_gui *)gui->private;
  gui->width=width;
  gui->height=height;

  /* create the widgets */
  if(gtk2_gui_create_main_window(gui) == FALSE)
    return FALSE;

  gtk2_gui_count++;
  return TRUE;
}

static tvalue gtk2_gui_run(struct sequ_gui *gui, char *filename)
{
  gtk2_gui *gtkui;
  gtkui=(gtk2_gui *)gui->private;

  /* draw the window */
  gtk_widget_show_all(GTK_WIDGET(gtkui->main));

  /* load a file */
  if(filename)
  {
    gtkui->filename=strdup(filename);
    
    gtk2_gui_load_create_canvas(gui,filename);
    redraw_canvas(gui,TRUE,0.0f,0.0f);

    /* if absolute path */
    if(gtkui->filename[0] == '/')
    {
      gtk_file_chooser_set_current_folder(
        GTK_FILE_CHOOSER(gtkui->fileselector),
        dirname(gtkui->filename));

      /* the dirname may have changed the filename */
      nullify(gtkui->filename);
      gtkui->filename=strdup(filename);
    }
  }

  /* start gtk if not yet */
  if(gtk2_gui_count <= 1)
    gtk_main();

  return TRUE;
}

static void gtk2_gui_deinit(struct sequ_gui *gui)
{
  gtk2_gui *gtkui;

  gtkui=(gtk2_gui *)gui->private;

  /* unreference all gtk objects */
  gtk_widget_destroy(GTK_WIDGET(gtkui->main));

  nullify(gtkui->filename);
  sequ_canvas_delete(gtkui->canvas);
  imagelist_delete(gtkui->images);

  /* free the structures */
  nullify(gtkui);
  nullify(gui);

  print_debug("%s: gui count on %d\n",THIS_FUNCTION,gtk2_gui_count);

  /* deinit gtk */
  if(--gtk2_gui_count == 0)
  {
    /* destroy the common dialogs */
    if(gtk2_gui_about)
      gtk_widget_destroy(GTK_WIDGET(gtk2_gui_about));
    /*
    if(gtk2_gui_config)
      gtk_widget_destroy(GTK_WIDGET(gtk2_gui_config));
    */

    gtk_main_quit();
  }
}

/* return the identification */
static int gtk2_gui_identify()
{
  return 1;
}


/************************************************************************/
/* creation of the widgets for the gtk2_gui */

/* generate all the parts of the main window */
static tvalue gtk2_gui_create_main_window(sequ_gui *gui)
{
  GtkWidget *eventbox;
  gtk2_gui *gtkui;

  if(gui == NULL)
    return FALSE;

  gtkui=(gtk2_gui *)gui->private;

  /* create the window */
  {
    GtkWidget *window;
    GdkPixbuf *img;

    CHECK_FAILURE(
      window=gtk_window_new(GTK_WINDOW_TOPLEVEL),
      NULL,FALSE
    );

    /* set the icon */
    img=gdk_pixbuf_new_from_xpm_data((const char **)sequview_xpm);
    gtk_window_set_icon(GTK_WINDOW(window),img);

    /* set window properties */
    gtk_window_resize(GTK_WINDOW(window),gui->width,gui->height);
    gtk2_gui_set_window_title(GTK_WINDOW(window),NULL);
    gtk_widget_realize(window);

    gtkui->main=GTK_WINDOW(window);
  }

  /* create the image */
  {
    GdkGeometry geo;

    gtk2_gui_resize_gdk_canvas(gui,gui->width,gui->height);
    gtkui->gtk_canvas=
      GTK_IMAGE(gtk_image_new_from_pixmap(gtkui->gdk_canvas,NULL));

    /* set the minimum size */
    geo.min_width = SEQU_CFG_GTK2_MAINWINDOW_WIDTH_MIN;
    geo.min_height= SEQU_CFG_GTK2_MAINWINDOW_HEIGHT_MIN;

    gtk_window_set_geometry_hints(GTK_WINDOW(gtkui->main),
      GTK_WIDGET(gtkui->gtk_canvas),&geo,GDK_HINT_MIN_SIZE);
  }  

  /* create the popup-menu */
  gtkui->menu=GTK_MENU(gtk2_gui_create_main_menu(gui,FALSE));

  gtk2_gui_scrolling_keys_to_codes(gui);

  /* create the fileselector */
  gtk2_gui_create_fileselector(gui);

  gtk_window_set_transient_for(GTK_WINDOW(gtkui->fileselector),
    GTK_WINDOW(gtkui->main));

  /* hide the cursor */
  {
    GdkPixbuf *img;
    GdkCursor *curs;
    
    const char *cursor[] = {
      "1 1 1 1",
      "  c White",
      " "};

    img=gdk_pixbuf_new_from_xpm_data(cursor);
    curs=gdk_cursor_new_from_pixbuf(gdk_display_get_default(),img,0,0);

    gdk_window_set_cursor(GTK_WIDGET(gtkui->main)->window,curs);
  }
  
  /* create the errorlog */
  IL_OutErr=gtk_error_iolet_create(gtkui->main);

  /* construct the main window */
  /* eventbox */
  eventbox=gtk_event_box_new();
  
  gtk_container_add(GTK_CONTAINER(gtkui->main),eventbox);
  gtk_container_add(GTK_CONTAINER(eventbox),
    GTK_WIDGET(gtkui->gtk_canvas));


  /* callbacks for the main window */
  g_signal_connect(G_OBJECT(gtkui->main), "delete-event",
    G_CALLBACK(GTK2_GUI_CALLBACK_FUNC(main_window_delete)), gui);

  g_signal_connect(G_OBJECT(gtkui->main), "destroy-event",
    G_CALLBACK(GTK2_GUI_CALLBACK_FUNC(main_window_delete)), gui);

  g_signal_connect(GTK_OBJECT(gtkui->main), "event",
    G_CALLBACK(GTK2_GUI_CALLBACK_FUNC(canvas_eventhandler)), gui);

  g_signal_connect(G_OBJECT(gtkui->main), "window-state-event",
    G_CALLBACK(GTK2_GUI_CALLBACK_FUNC(main_window_state_change)), gui);

  /* right click the window (eventbox) to popup the menu */
  g_signal_connect_swapped(GTK_OBJECT(eventbox), "button_press_event",
    G_CALLBACK(GTK2_GUI_CALLBACK_FUNC(eventbox_click)),
    gtkui);

  return TRUE;
}

/* the fileselector */
static tvalue gtk2_gui_create_fileselector(sequ_gui *gui)
{

  GtkWidget *filesel;
  GTK2_GUI_CONVERT_GTKUI_POINTER();

  /* create the fileselector */
  filesel = gtk_file_chooser_dialog_new("Open file.",gtkui->main,
    GTK_FILE_CHOOSER_ACTION_OPEN,
    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
    NULL);

  /* attributes of the selector */
  gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(filesel),TRUE);

  /* all responses */
  g_signal_connect(filesel,"response", 
    G_CALLBACK(GTK2_GUI_CALLBACK_FUNC(get_files)),gui);

  g_signal_connect(G_OBJECT(filesel), "delete_event",
    G_CALLBACK(gtk_widget_hide), (gpointer) filesel);

  gtkui->fileselector=filesel;

  return TRUE;
}

/************************************************************************/
/* create the menu. */

typedef struct gtk2_gui_menu
{
  /* Acceptable values:
     0 separator
     1 normal item
     -1 last element
   */
  char type;
  char *label;

  /* the accelerator string */
  char *accelerator;

  /* activation function */
  void (*activate)(GtkMenuItem *item,gpointer data);

  /* possible submenu of the element */
  struct gtk2_gui_menu *submenu;

} gtk2_gui_menu;


/* create and append an element into a menu */
static GtkMenuItem *gtk2_gui_menu_append_element(GtkMenuShell *menu,
  gtk2_gui_menu *menu_item, char *path, sequ_gui *gui)
{
  GtkMenuItem *item = NULL;

  /* if separator */
  if(menu_item->type == 0)
    item = GTK_MENU_ITEM(gtk_separator_menu_item_new());
  /* if element */
  else if(menu_item->type == 1)
  {
    item = GTK_MENU_ITEM(gtk_menu_item_new_with_label(menu_item->label));

    /* hookup the activate -function */
    if(menu_item->activate)
      g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_item->activate), gui);

    /* set the accelerator */
    if(menu_item->accelerator != NULL)
    {
      guint key;
      GdkModifierType mod;

      /* add the associated accelerator */
      gtk_accelerator_parse(menu_item->accelerator,&key,&mod);

      gtk_accel_map_add_entry(path,key,mod);
      gtk_menu_item_set_accel_path(item,path);
    }
  }

  if(item)
    gtk_menu_shell_append(menu,GTK_WIDGET(item));

  /* return it for possible submenu generation */
  return item;
}


/* generates a menu based upon gtk_gui_menu. A poor clone of 
   gtkitemfactory. rootname is the name of the window (for
   GtkAccelMap). Assumes that rootname's last character is '/' */
static GtkWidget *gtk2_gui_generate_menu(sequ_gui *gui, 
  gtk2_gui_menu *menu, GtkAccelGroup *accels,
  char *rootname)
{
  register unsigned int beta=0;

  GtkWidget *ret;
  GtkMenuItem *tmp;

  /* assume that this is enough for the accelerator path */
  char pathstr[512];

  ret = gtk_menu_new();

  while(menu[beta].type != -1)
  {
    strcpy(pathstr,rootname);
    if(menu[beta].type != 0)
      strcat(pathstr,menu[beta].label);

    tmp=gtk2_gui_menu_append_element(GTK_MENU_SHELL(ret),
      &menu[beta],pathstr,gui);

    /* if the item has a submenu */
    if(menu[beta].submenu != NULL)
    {
      GtkWidget *sub;

      strcat(pathstr,"/");
      sub=gtk2_gui_generate_menu(gui,menu[beta].submenu,accels,pathstr);

      gtk_menu_item_set_submenu(tmp,sub);
    }

    beta++;
  }

  /* tie the accelerators */
  gtk_menu_set_accel_path(GTK_MENU(ret),rootname);
  gtk_menu_set_accel_group(GTK_MENU(ret),accels);

  return ret;
}

/* rewrite the accelerator map */
static tvalue gtk2_gui_update_accelmap(gtk2_gui_menu *menu,char *path)
{
  char pathstr[512];

  for(unsigned int beta=0; menu[beta].type != -1; beta++)
  {
    strcpy(pathstr,path);
    if(menu[beta].type != 0)
      strcat(pathstr,menu[beta].label);

    /* a submenu */
    if(menu[beta].submenu)
    {
      strcat(pathstr,"/");
      gtk2_gui_update_accelmap(menu[beta].submenu,pathstr);
    }

    /* update the accelerator */
    if(menu[beta].accelerator)
    {
      guint key;
      GdkModifierType mod;

      gtk_accelerator_parse(menu[beta].accelerator,&key,&mod);
      gtk_accel_map_change_entry(pathstr,key,mod,FALSE);
    }
  }
  
  return TRUE;
}

/* creates the main menu */
static GtkWidget *gtk2_gui_create_main_menu(sequ_gui *gui,tvalue update_accels)
{
  /* the control submenu */
  gtk2_gui_menu control_submenu[] =
  {
    {1, "First",   sequ_config_gtk_gui_keys[4],
     GTK2_GUI_CALLBACK_FUNC(page_first),NULL},
    {1, "Last",    sequ_config_gtk_gui_keys[5],
     GTK2_GUI_CALLBACK_FUNC(page_last),NULL},
    {1, "Forward", sequ_config_gtk_gui_keys[6],
     GTK2_GUI_CALLBACK_FUNC(page_forward),NULL},
    {1, "Back",    sequ_config_gtk_gui_keys[7],
     GTK2_GUI_CALLBACK_FUNC(page_backward),NULL},
    {-1, NULL,     NULL,NULL,NULL}
  };

  /* the popup menu */
  gtk2_gui_menu main_menu[] =
  {
#ifdef DEBUG
    {1,"Demo","D",
     GTK2_GUI_CALLBACK_FUNC(execute_demo),NULL},
#endif
    {1,"Open",sequ_config_gtk_gui_keys[0],
     GTK2_GUI_CALLBACK_FUNC(open_fileselector),NULL},
    /*
    {1,"Help",sequ_config_gtk_gui_keys[1],
     NULL,NULL},
    */
    {0,NULL,NULL,NULL,NULL},
    {1,"Maximize",sequ_config_gtk_gui_keys[2],
     GTK2_GUI_CALLBACK_FUNC(maximize),NULL},
    {1,"Iconify",sequ_config_gtk_gui_keys[11],
     GTK2_GUI_CALLBACK_FUNC(iconify),NULL},
    {1,"Fullscreen",sequ_config_gtk_gui_keys[3],
     GTK2_GUI_CALLBACK_FUNC(fullscreen),NULL},
    {1,"Control",NULL,
     NULL,control_submenu},
    {0,NULL,NULL,NULL,NULL},
    {1,"Configuration",sequ_config_gtk_gui_keys[8],
     GTK2_GUI_CALLBACK_FUNC(open_config),NULL},
    {1,"About",sequ_config_gtk_gui_keys[9],
     GTK2_GUI_CALLBACK_FUNC(open_about),NULL},
    {0,NULL,NULL,NULL,NULL},
    {1,"Quit",sequ_config_gtk_gui_keys[10],
     GTK2_GUI_CALLBACK_FUNC(quit),NULL},
    {-1, NULL,     NULL,NULL,NULL}    
  };

  char *menu_name="<main>/";
  GtkAccelGroup  *accels;
  GtkWidget *ret;

  GTK2_GUI_CONVERT_GTKUI_POINTER();

  /* update the accelerators */
  if(update_accels)
  {
    gtk2_gui_update_accelmap(main_menu,menu_name);
    return GTK_WIDGET(gtkui->menu);
  }

  /* creation of the menu */
  print_debug("maksimoinnin key on %s\n",
    main_menu[3].accelerator);

  accels = gtk_accel_group_new();

  ret = gtk2_gui_generate_menu(gui,main_menu,accels,menu_name);

  gtk_window_add_accel_group(gtkui->main,accels);

  gtk_widget_show_all(ret);

  return ret;
}

/************************************************************************/
/* update the keys */

static void gtk2_gui_scrolling_keys_to_codes(sequ_gui *gui)
{
  GTK2_GUI_CONVERT_GTKUI_POINTER();

  for(unsigned int beta=0,gamma=12;beta<4;beta++,gamma++)
    gtkui->scroll_keycodes[beta]=
      gdk_keyval_from_name(sequ_config_gtk_gui_keys[gamma]);
}

/* update the keyconfiguration */
static tvalue gtk2_gui_update_keys(sequ_gui *gui)
{
  gtk2_gui_create_main_menu(gui,TRUE);
  gtk2_gui_scrolling_keys_to_codes(gui);

  return TRUE;
}

/************************************************************************/
/* creation of the common widgets */

/* about dialog */
static tvalue gtk2_gui_create_about_dialog()
{
  GtkWidget *about,*label,*button,*vbox,*icon;
  GdkPixbuf *img;

  if(gtk2_gui_about != NULL)
    return TRUE;

  /* convert the image */
  img=gdk_pixbuf_new_from_xpm_data((const char **)sequview_xpm);
  icon=gtk_image_new_from_pixbuf(img);

  /* construct the window */
  about=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(about),GTK_WIN_POS_CENTER);

  gtk_container_set_border_width(GTK_CONTAINER(about),10);

  button=gtk_button_new_with_label("OK");

  label=gtk_label_new(sequ_config_versioncopy);
  vbox=gtk_vbox_new(FALSE,5);

  gtk_box_pack_start(GTK_BOX(vbox),icon,TRUE,TRUE,10);
  gtk_box_pack_start(GTK_BOX(vbox),label,TRUE,TRUE,10);
  gtk_box_pack_start(GTK_BOX(vbox),
    gtk_label_new(
      "Gtk+ version: " GTK2_VERSION 
#ifdef IMLIB2_VERSION
      "\nImlib2 version: " IMLIB2_VERSION
#endif
    ),TRUE,TRUE,10);

  gtk_box_pack_start(GTK_BOX(vbox),button,TRUE,TRUE,0);

  gtk_container_add(GTK_CONTAINER(about),vbox);

  /* connect the signals */
  g_signal_connect_swapped (GTK_OBJECT(button),
    "clicked",G_CALLBACK(gtk_widget_hide), (gpointer) about); 

  g_signal_connect(G_OBJECT(about),"delete_event",
    G_CALLBACK(gtk_widget_hide),(gpointer) about);

  gtk2_gui_about=GTK_WINDOW(about);

  return TRUE;
}

/************************************************************************/
/* main window callbacks */

static gint GTK2_GUI_CALLBACK_FUNC(main_window_delete)(GtkWidget *widget,
  GdkEvent *event,gpointer data)
{
  sequ_gui *gui=(sequ_gui *)data;

  gtk2_gui_delete(gui);

  return FALSE;
}

/* window's status has changed */
static gboolean GTK2_GUI_CALLBACK_FUNC(main_window_state_change)(
  GtkWidget *widget,GdkEvent *event,gpointer data)
{
  GTK2_GUI_CONVERT_GUI_POINTER();
  GTK2_GUI_CONVERT_GTKUI_POINTER();

  print_debug("Ikkunan tila vaihtui!\n");

  /* read the new state */ 
  gtkui->main_maximized=(((GdkEventWindowState *)event)->new_window_state) & 
    GDK_WINDOW_STATE_MAXIMIZED;
  gtkui->main_iconified=(((GdkEventWindowState *)event)->new_window_state) &
    GDK_WINDOW_STATE_ICONIFIED;
  gtkui->main_fullscreen=(((GdkEventWindowState *)event)->new_window_state) & 
    GDK_WINDOW_STATE_FULLSCREEN;

  /* resize the canvas */
  resize_canvas(gui);

  return FALSE;
}


/* generic eventhandler for the canvas */
static gboolean GTK2_GUI_CALLBACK_FUNC(canvas_eventhandler)(
  GtkWidget *widget,GdkEvent *event,gpointer data)
{
  GTK2_GUI_CONVERT_GUI_POINTER();
  GTK2_GUI_CONVERT_GTKUI_POINTER();

  //  print_debug("%s: eventti oli %d\n",THIS_FUNCTION,event->type);

  if(event->type == GDK_KEY_PRESS)
  {
    char *tmp;
    const float rel_skip=0.1f,
      rel_skip_x[]={0.0f, 0.0f,-rel_skip,rel_skip},
      rel_skip_y[]={-rel_skip,rel_skip,0.0f, 0.0f};

    float rel_x=0,rel_y=0;
    
    GdkEventKey *evt=(GdkEventKey *)event;
    print_debug("%s: ja merkkijono oli [%s]\n",THIS_FUNCTION,evt->string);

    tmp=gtk_accelerator_name(evt->keyval,0);
    print_debug("%s: sama accelina: [%s]\n",THIS_FUNCTION,tmp);
    nullify(tmp);

    /* move the viewport  */
    for(unsigned int beta=0;beta<4;beta++)
      if(evt->keyval == gtkui->scroll_keycodes[beta])
      {
        rel_x+=rel_skip_x[beta];
        rel_y+=rel_skip_y[beta];
        break;
      }

    if(rel_x != 0.0f || rel_y != 0.0f)
      repaint_canvas(gui,rel_x,rel_y);
  }
  else if(event->type == GDK_CONFIGURE)
  {
    if(resize_canvas(gui) == FALSE)
      return FALSE;
  }
  /* enable scrolling */
  else if(event->type == GDK_SCROLL)
  {
    GdkEventScroll *evt=(GdkEventScroll *)event;
    int jump=0;

    switch(evt->direction)
    {
    case GDK_SCROLL_UP:
    case GDK_SCROLL_LEFT:
      jump=-MAX_DIFF_PAGE_JUMP-1;
      break;
    case GDK_SCROLL_DOWN:
    case GDK_SCROLL_RIGHT:
      jump=-MAX_DIFF_PAGE_JUMP+DEFAULT_DIFF_PAGE_JUMP;
      break;
    default:
      break;
    }

    if(jump)
      change_page(data,jump);
  }

  return FALSE;
}


/************************************************************************/
/* eventbox callbacks */

static gboolean GTK2_GUI_CALLBACK_FUNC(eventbox_click)(GtkWidget *widget,
  GdkEventButton *event)
{
  gtk2_gui *gtkui=(gtk2_gui *)widget;

  /* popup the menu */
  if (event->type == GDK_BUTTON_PRESS && event->button == 3)
  {
    gtk_menu_popup(GTK_MENU(gtkui->menu),NULL,NULL,NULL,NULL,event->button,
      event->time);
    return TRUE;
  }

  return FALSE;
}


/************************************************************************/
/* menu callbacks */

#ifdef DEBUG
GTK2_GUI_MENU_CALLBACK(execute_demo)
{
  print_debug("DEMOA!!\n");

  print_err("Error: testia demoa.\n");
}
#endif

GTK2_GUI_MENU_CALLBACK(open_fileselector)
{
  GTK2_GUI_CONVERT_GUI_POINTER();
  GTK2_GUI_CONVERT_GTKUI_POINTER();

  gtk_window_set_position(GTK_WINDOW(gtkui->fileselector),
    GTK_WIN_POS_CENTER_ALWAYS);
  gtk_widget_show_all(gtkui->fileselector);
}

GTK2_GUI_MENU_CALLBACK(maximize)
{
  GTK2_GUI_CONVERT_GUI_POINTER();
  GTK2_GUI_CONVERT_GTKUI_POINTER();

  if(!gtkui->main_maximized)
    gtk_window_maximize(GTK_WINDOW(gtkui->main));
  else
    gtk_window_unmaximize(GTK_WINDOW(gtkui->main));
  
}

GTK2_GUI_MENU_CALLBACK(iconify)
{
  GTK2_GUI_CONVERT_GUI_POINTER();
  GTK2_GUI_CONVERT_GTKUI_POINTER();

  if(!gtkui->main_iconified)
    gtk_window_iconify(GTK_WINDOW(gtkui->main));
  else
    gtk_window_deiconify(GTK_WINDOW(gtkui->main));
}


GTK2_GUI_MENU_CALLBACK(fullscreen)
{
  GTK2_GUI_CONVERT_GUI_POINTER();
  GTK2_GUI_CONVERT_GTKUI_POINTER();

  if(!gtkui->main_fullscreen)
    gtk_window_fullscreen(GTK_WINDOW(gtkui->main));
  else
    gtk_window_unfullscreen(GTK_WINDOW(gtkui->main));

}

GTK2_GUI_MENU_CALLBACK(page_first)
{
  change_page(data,0);
}

GTK2_GUI_MENU_CALLBACK(page_last)
{
  change_page(data,-2*MAX_DIFF_PAGE_JUMP-DEFAULT_DIFF_PAGE_JUMP);
}

GTK2_GUI_MENU_CALLBACK(page_forward)
{
  change_page(data,-MAX_DIFF_PAGE_JUMP+DEFAULT_DIFF_PAGE_JUMP);
}

GTK2_GUI_MENU_CALLBACK(page_backward)
{
  change_page(data,-MAX_DIFF_PAGE_JUMP-1);
}

GTK2_GUI_MENU_CALLBACK(open_about)
{
  GTK2_GUI_CONVERT_GUI_POINTER();
  GTK2_GUI_CONVERT_GTKUI_POINTER();

  /* create this window only if needed */
  if(!gtk2_gui_about)
    gtk2_gui_create_about_dialog();
  
  /* put the aboutbox on top of this window */
  gtk_window_set_transient_for(GTK_WINDOW(gtk2_gui_about),
    GTK_WINDOW(gtkui->main));

  gtk_widget_show_all(GTK_WIDGET(gtk2_gui_about));
}

GTK2_GUI_MENU_CALLBACK(open_config)
{
  GTK2_GUI_CONVERT_GUI_POINTER();
  GTK2_GUI_CONVERT_GTKUI_POINTER();

  /* create this once */
  if(!gtkui->config)
  {
    gtkui->config=configuration_dialog_create();
    gtk_window_set_transient_for(GTK_WINDOW(gtkui->config->dialog),
      GTK_WINDOW(gtkui->main));

    g_signal_connect(gtkui->config->dialog,"response", 
      G_CALLBACK(GTK2_GUI_CALLBACK_FUNC(config_resp)),gui);
  }

  gtk_widget_show_all(GTK_WIDGET(gtkui->config->dialog));
}

GTK2_GUI_MENU_CALLBACK(quit)
{
  GTK2_GUI_CONVERT_GUI_POINTER();

  gtk2_gui_delete(gui);
}

/************************************************************************/
/* fileselector callbacks */

static void GTK2_GUI_CALLBACK_FUNC(get_files)(GtkDialog *dialog,gint arg1,
  gpointer data)
{
  GTK2_GUI_CONVERT_GUI_POINTER();
  //  GTK2_GUI_CONVERT_GTKUI_POINTER();

  /* get the filenames */

  print_debug("Callbackissa: arg1 on %d\n",arg1);

  /* opening a file */
  if(arg1 == GTK_RESPONSE_ACCEPT)
  {
    print_debug("Avataan tiedosto [%s]\n",
      gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)));

    gtk2_gui_load_create_canvas(gui,
      gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)));

    /* draw */
    redraw_canvas(gui,TRUE,0.0f,0.0f);
  }

  gtk_widget_hide(GTK_WIDGET(dialog));
}


/************************************************************************/
/* configuration-dialog callbacks */

static void GTK2_GUI_CALLBACK_FUNC(config_resp)(GtkDialog *dialog,gint arg1,
  gpointer data)
{
  GTK2_GUI_CONVERT_GUI_POINTER();
  GTK2_GUI_CONVERT_GTKUI_POINTER();

  print_debug("%s: TÄÄLLÄ!!\n",THIS_FUNCTION);

  if(arg1 == GTK_RESPONSE_OK || arg1 == GTK_RESPONSE_APPLY)
  {
    /* write the configuration */
    configuration_dialog_update_cfg(gtkui->config);

    /* reapply the keyconfig */
    gtk2_gui_update_keys(gui);

    /* reapply the configuration */
    gtk2_gui_load_create_canvas(gui,NULL);
    redraw_canvas(gui,TRUE,0.0f,0.0f);
  }

  if(arg1 != GTK_RESPONSE_APPLY)
    gtk_widget_hide(GTK_WIDGET(dialog));
}
