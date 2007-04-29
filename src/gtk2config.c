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

#include <gtk/gtk.h>

#include <common/iolet.h>

#include "configvars.h"
#include "sequconfig.h"
#include "gtk2config.h"

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
#define CONFDIALOG_HEIGHT 300

/****************************************************************************
  Configuration pages
 ****************************************************************************/

#warning tämän on oltava väliaikainen ratkaisu
#include "gtk2confpages.c"

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
    GTK_DIALOG_MODAL,
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
