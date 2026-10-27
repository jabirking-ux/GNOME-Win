#include "ephy-find-bar.h"
#include "ephy-window.h"
#include "ephy-tab.h"

static void do_find (EphyFindBar *bar, gboolean backwards) {
  if (!bar->window || !bar->window->active_tab) return;
  const char *text = gtk_editable_get_text (GTK_EDITABLE (bar->entry));
  if (!text || !*text) return;
  ephy_tab_find_string (bar->window->active_tab, text, backwards, bar->case_sensitive);
}

static void on_search_changed (GtkSearchEntry *entry, gpointer user_data) {
  EphyFindBar *bar = (EphyFindBar *)user_data;
  do_find (bar, FALSE);
}

static void on_entry_activate (GtkEntry *entry, gpointer user_data) {
  EphyFindBar *bar = (EphyFindBar *)user_data;
  do_find (bar, FALSE);
}

static void on_next_clicked (GtkButton *btn, gpointer user_data) {
  EphyFindBar *bar = (EphyFindBar *)user_data;
  do_find (bar, FALSE);
}

static void on_prev_clicked (GtkButton *btn, gpointer user_data) {
  EphyFindBar *bar = (EphyFindBar *)user_data;
  do_find (bar, TRUE);
}

static void on_case_toggled (GtkToggleButton *btn, gpointer user_data) {
  EphyFindBar *bar = (EphyFindBar *)user_data;
  bar->case_sensitive = gtk_toggle_button_get_active (btn);
  do_find (bar, FALSE);
}

static void on_close_clicked (GtkButton *btn, gpointer user_data) {
  EphyFindBar *bar = (EphyFindBar *)user_data;
  ephy_find_bar_hide (bar);
}

EphyFindBar *ephy_find_bar_new (EphyWindow *window) {
  EphyFindBar *bar = g_new0 (EphyFindBar, 1);
  bar->window = window;
  bar->case_sensitive = FALSE;

  bar->container = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_add_css_class (bar->container, "toolbar");
  gtk_widget_set_margin_start (bar->container, 8);
  gtk_widget_set_margin_end (bar->container, 8);
  gtk_widget_set_margin_top (bar->container, 4);
  gtk_widget_set_margin_bottom (bar->container, 4);
  gtk_widget_set_visible (bar->container, FALSE);

  bar->entry = gtk_search_entry_new ();
  gtk_widget_set_hexpand (bar->entry, TRUE);
  gtk_editable_set_width_chars (GTK_EDITABLE (bar->entry), 24);
  g_signal_connect (bar->entry, "search-changed", G_CALLBACK (on_search_changed), bar);
  g_signal_connect (bar->entry, "activate", G_CALLBACK (on_entry_activate), bar);
  gtk_box_append (GTK_BOX (bar->container), bar->entry);

  /* Next and Prev buttons linked */
  GtkWidget *nav_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_add_css_class (nav_box, "linked");

  bar->prev_btn = gtk_button_new_from_icon_name ("go-up-symbolic");
  gtk_widget_set_tooltip_text (bar->prev_btn, "Previous match (Shift+Enter)");
  g_signal_connect (bar->prev_btn, "clicked", G_CALLBACK (on_prev_clicked), bar);
  gtk_box_append (GTK_BOX (nav_box), bar->prev_btn);

  bar->next_btn = gtk_button_new_from_icon_name ("go-down-symbolic");
  gtk_widget_set_tooltip_text (bar->next_btn, "Next match (Enter)");
  g_signal_connect (bar->next_btn, "clicked", G_CALLBACK (on_next_clicked), bar);
  gtk_box_append (GTK_BOX (nav_box), bar->next_btn);

  gtk_box_append (GTK_BOX (bar->container), nav_box);

  /* Case-sensitive toggle */
  bar->case_btn = gtk_toggle_button_new_with_label ("Aa");
  gtk_widget_set_tooltip_text (bar->case_btn, "Match Case");
  g_signal_connect (bar->case_btn, "toggled", G_CALLBACK (on_case_toggled), bar);
  gtk_box_append (GTK_BOX (bar->container), bar->case_btn);

  /* Close button */
  bar->close_btn = gtk_button_new_from_icon_name ("window-close-symbolic");
  gtk_button_set_has_frame (GTK_BUTTON (bar->close_btn), FALSE);
  gtk_widget_set_tooltip_text (bar->close_btn, "Close find bar (Esc)");
  g_signal_connect (bar->close_btn, "clicked", G_CALLBACK (on_close_clicked), bar);
  gtk_box_append (GTK_BOX (bar->container), bar->close_btn);

  return bar;
}

void ephy_find_bar_show (EphyFindBar *bar) {
  if (!bar) return;
  gtk_widget_set_visible (bar->container, TRUE);
  gtk_widget_grab_focus (bar->entry);
  gtk_editable_select_region (GTK_EDITABLE (bar->entry), 0, -1);
}

void ephy_find_bar_hide (EphyFindBar *bar) {
  if (!bar) return;
  gtk_widget_set_visible (bar->container, FALSE);
  if (bar->window && bar->window->active_tab)
    ephy_tab_hide_find_ui (bar->window->active_tab);
}

void ephy_find_bar_find_next (EphyFindBar *bar) {
  do_find (bar, FALSE);
}

void ephy_find_bar_find_prev (EphyFindBar *bar) {
  do_find (bar, TRUE);
}
