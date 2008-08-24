/**
 * GENS: (GTK+) Game Genie Window - Miscellaneous Functions.
 */


#include "game_genie_window.h"
#include "game_genie_window_callbacks.h"
#include "game_genie_window_misc.h"
#include "genswindow.h"
#include "ggenie.h"
#include "mem_m68k.h"
#include "vdp_io.h"

#include <gtk/gtk.h>
#include "gtk-misc.h"

// Stores the entries in the TreeView.
GtkListStore *listmodel = NULL;

/**
 * Open_Game_Genie(): Opens the Game Genie window.
 */
void Open_Game_Genie(void)
{
	GtkWidget *gg, *treeview;
	GtkCellRenderer *toggle_renderer, *text_renderer;
	GtkTreeViewColumn *col_enabled, *col_code, *col_name;
	GtkTreeSelection *select;
	int i;
	
	gg = create_game_genie_window();
	gtk_window_set_transient_for(GTK_WINDOW(gg), GTK_WINDOW(gens_window));
	
	// Populate the TreeView.
	treeview = lookup_widget(gg, "treeview_gg_list");
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	gtk_tree_selection_set_mode(select, GTK_SELECTION_MULTIPLE);
	
	// Check if the listmodel is already created.
	// If it is, clear it; if not, create a new one.
	if (listmodel)
		gtk_list_store_clear(listmodel);
	else
		listmodel = gtk_list_store_new(3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING);
	
	gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(listmodel));
	
	// Create the renderer and columns.
	toggle_renderer = gtk_cell_renderer_toggle_new();
	col_enabled = gtk_tree_view_column_new_with_attributes("Enabled", toggle_renderer, "active", 0, NULL);
	text_renderer = gtk_cell_renderer_text_new();
	col_code = gtk_tree_view_column_new_with_attributes("Code", text_renderer, "text", 1, NULL);
	col_name = gtk_tree_view_column_new_with_attributes("Name", text_renderer, "text", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col_enabled);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col_code);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col_name);
	
	// Connect the toggle renderer to the callback.
	g_signal_connect((gpointer)toggle_renderer, "toggled",
			 G_CALLBACK(on_treeview_gg_list_item_toggled), (gpointer)listmodel);
	
	// Go through the list of codes and add them to the treeview.
	for (i = 0; i < 256; i++)
	{
		if (Liste_GG[i].code[0] == 0)
			continue;
		
		GG_AddCode(treeview, Liste_GG[i].name, Liste_GG[i].code, Liste_GG[i].active);
		
		// If the ROM is loaded, and this code applies to ROM data, apply the code.
		// Or something.
		// TODO: Figure out what this actually does.
		if ((Liste_GG[i].restore != 0xFFFFFFFF) &&
		    (Liste_GG[i].addr < Rom_Size) &&
		    (Genesis_Started))
		{
			Rom_Data[Liste_GG[i].addr] = (unsigned char)(Liste_GG[i].restore & 0xFF);
			Rom_Data[Liste_GG[i + 1].addr] = (unsigned char)((Liste_GG[i].restore & 0xFF00) >> 8);
		}
	}
	
	gtk_widget_show_all(gg);
}


/**
 * GG_AddCode(): Add a code to the Game Genie treeview.
 * @param treeview GtkTreeView.
 * @param name Name of the code.
 * @param code Actual code.
 * @param enabled If non-zero, the code is enabled.
 */
void GG_AddCode(GtkWidget *treeview, const char *name, const char *code, int enabled)
{
	GtkTreeIter iter;
	gtk_list_store_append(listmodel, &iter);
	gtk_list_store_set(GTK_LIST_STORE(listmodel), &iter, 0, enabled, 1, code, 2, name, -1);
	
	// TODO: What is this for?
	if (enabled && treeview)
	{
		GtkTreeSelection *select;
		select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		gtk_tree_selection_select_iter(select, &iter);
	}
}


/**
 * GG_DelSelectedCode(): Delete the selected code.
 * @param treeview Treeview widget.
 */
void GG_DelSelectedCode(GtkWidget *treeview)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gboolean need_check, row_erased, valid;
	
	if (!treeview)
		return;
	
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	
	// Delete all selected codes.
	need_check = TRUE;
	while (need_check)
	{
		row_erased = FALSE;
		valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(listmodel), &iter);
		while (valid && !row_erased)
		{
			if (gtk_tree_selection_iter_is_selected(selection, &iter))
			{
				gtk_list_store_remove(listmodel, &iter);
				row_erased = TRUE;
			}
			else
				valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(listmodel), &iter);
		}
		if (!valid && !row_erased)
			need_check = FALSE;
	}
}
