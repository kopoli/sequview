/***************************************************************************
  Name:         gtk2error.c
  Description:  a GTK+ based error outlet
  Created:      20070224 13:48
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

#include <common/iolet.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>

static const char *tag_new="new_tag",*tag_old="old_tag";

typedef struct
{
  GtkWidget *wnd;

  GtkTextBuffer *buf;

} gtk_error_iolet_priv;

static GtkWidget *create_error_log_window(GtkWindow *parent, 
  GtkTextBuffer *buf)
{
  GtkWidget *wnd,*scroll,*text;

  /* create the widgets */
  wnd=gtk_dialog_new_with_buttons("Error Log",parent,
    GTK_DIALOG_DESTROY_WITH_PARENT,
    GTK_STOCK_OK, GTK_RESPONSE_OK,
    NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(wnd),GTK_RESPONSE_OK);
  gtk_widget_set_size_request(wnd,400,220);

  scroll=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
    GTK_POLICY_AUTOMATIC,GTK_POLICY_ALWAYS);

  text=gtk_text_view_new_with_buffer(buf);
  gtk_widget_set_sensitive(text,FALSE);
  gtk_container_add(GTK_CONTAINER(scroll),text);

  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(wnd)->vbox),scroll);

  g_signal_connect(G_OBJECT(wnd), "delete_event",
    G_CALLBACK(gtk_widget_hide), wnd);

  g_signal_connect(GTK_DIALOG(wnd), "response",
    G_CALLBACK(gtk_widget_hide), wnd);  

  gtk_widget_realize(wnd);

  return wnd;
}

static void create_text_tags(GtkTextBuffer *buf)
{
  gtk_text_buffer_create_tag(buf,tag_new,
    "weight", PANGO_WEIGHT_BOLD,NULL);

  gtk_text_buffer_create_tag(buf,tag_old,
    "foreground","darkgrey",NULL);

  gtk_text_buffer_create_tag(buf, "word_wrap",
    "wrap_mode",GTK_WRAP_WORD,NULL);
}

static tvalue gtk_error_iolet_end(iolet *io)
{
  nullify(io->PrivData);
  nullify(io);
  return TRUE;
}

static tvalue gtk_error_iolet_out_format(iolet *out,
  const char *fmt, va_list list)
{
  char *str;
  gtk_error_iolet_priv *priv=(gtk_error_iolet_priv *)out->PrivData;
  GtkTextIter iter,start,end;

  g_vasprintf(&str,fmt,list);

  /* grey out all the old messages */
  gtk_text_buffer_get_bounds(priv->buf,&start,&end);
  gtk_text_buffer_apply_tag_by_name(priv->buf,tag_old,&start,&end);

  /* write the string */
  gtk_text_buffer_get_iter_at_offset(priv->buf, &iter, 0);
  gtk_text_buffer_insert(priv->buf,&iter,str,-1);

  /* apply word wrap */
  gtk_text_buffer_get_bounds(priv->buf,&start,&end);
  gtk_text_buffer_apply_tag_by_name(priv->buf,"word_wrap",&start,&end);

  gtk_widget_show_all(priv->wnd);
  
  g_free(str);

  return TRUE;
}

iolet *gtk_error_iolet_create(GtkWindow *parent)
{
  iolet *ret;
  gtk_error_iolet_priv *priv;

  ret=malloc(sizeof(iolet));
  priv=malloc(sizeof(gtk_error_iolet_priv));

  memset(ret,0,sizeof(iolet));

  /* create the window */
  priv->buf=gtk_text_buffer_new(NULL);
  priv->wnd=create_error_log_window(parent,priv->buf);

  create_text_tags(priv->buf);

  gtk_text_buffer_set_text(priv->buf, "", 0);

  /* create the iolet */
  ret->PrivData=priv;
  ret->PDataLen=sizeof(gtk_error_iolet_priv);
  ret->Type=0xFF;
  ret->Flags=IL_OUTLET_FORMAT;

  ret->OutputFormat=gtk_error_iolet_out_format;
  ret->End=gtk_error_iolet_end;

  iolet_add(ret);

  return ret;
}
