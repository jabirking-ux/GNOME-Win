#include "ephy-suggestions.h"
#include "ephy-window.h"
#include "ephy-bookmarks.h"
#include "ephy-history.h"
#include "ephy-settings.h"

static void on_row_activated (GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
  EphySuggestions *sug = (EphySuggestions *)user_data;
  const char *url = (const char *)g_object_get_data (G_OBJECT (row), "target-url");
  if (url && sug->window && sug->window->active_tab) {
    ephy_tab_load_url (sug->window->active_tab, url);
    ephy_suggestions_hide (sug);
  }
}

EphySuggestions *ephy_suggestions_new (EphyWindow *window) {
  EphySuggestions *sug = g_new0 (EphySuggestions, 1);
  sug->window = window;

  sug->popover = gtk_popover_new ();
  gtk_widget_set_parent (sug->popover, window->location_entry);
  gtk_popover_set_autohide (GTK_POPOVER (sug->popover), TRUE);
  gtk_popover_set_has_arrow (GTK_POPOVER (sug->popover), FALSE);
  gtk_popover_set_position (GTK_POPOVER (sug->popover), GTK_POS_BOTTOM);

  GtkWidget *scrolled = gtk_scrolled_window_new ();
  gtk_scrolled_window_set_max_content_height (GTK_SCROLLED_WINDOW (scrolled), 380);
  gtk_scrolled_window_set_propagate_natural_height (GTK_SCROLLED_WINDOW (scrolled), TRUE);
  gtk_widget_set_size_request (scrolled, 520, -1);

  sug->list_box = gtk_list_box_new ();
  gtk_widget_add_css_class (sug->list_box, "boxed-list");
  g_signal_connect (sug->list_box, "row-activated", G_CALLBACK (on_row_activated), sug);

  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled), sug->list_box);
  gtk_popover_set_child (GTK_POPOVER (sug->popover), scrolled);

  return sug;
}

static GtkWidget *create_suggestion_row (const char *icon_name, const char *title, const char *subtitle, const char *url) {
  GtkWidget *row = adw_action_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), title);
  if (subtitle && *subtitle)
    adw_action_row_set_subtitle (ADW_ACTION_ROW (row), subtitle);

  GtkWidget *icon = gtk_image_new_from_icon_name (icon_name);
  adw_action_row_add_prefix (ADW_ACTION_ROW (row), icon);

  gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (row), TRUE);
  g_object_set_data_full (G_OBJECT (row), "target-url", g_strdup (url), g_free);
  return row;
}

void ephy_suggestions_update (EphySuggestions *sug, const char *query) {
  if (!sug || !sug->list_box) return;

  /* Clear existing rows */
  GtkWidget *child = gtk_widget_get_first_child (sug->list_box);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling (child);
    gtk_list_box_remove (GTK_LIST_BOX (sug->list_box), child);
    child = next;
  }

  if (!query || strlen (query) < 1) {
    ephy_suggestions_hide (sug);
    return;
  }

  if (g_str_has_prefix (query, "file://") || g_str_has_prefix (query, "about:")) {
    ephy_suggestions_hide (sug);
    return;
  }

  int count = 0;
  EphySettings *s = ephy_settings_get_default ();

  /* 1. Default search query suggestion */
  char *search_url = ephy_settings_build_search_url (s, query);
  g_autofree char *search_title = g_strdup_printf ("Search for \"%s\"", query);
  GtkWidget *search_row = create_suggestion_row ("system-search-symbolic", search_title, s->search_engine ? s->search_engine : "DuckDuckGo", search_url);
  gtk_list_box_append (GTK_LIST_BOX (sug->list_box), search_row);
  g_free (search_url);
  count++;

  /* 2. Search Bookmarks */
  g_autofree char *query_lower = g_utf8_strdown (query, -1);
  GList *bms = ephy_bookmarks_get_all ();
  for (GList *l = bms; l != NULL && count < 8; l = l->next) {
    EphyBookmark *bm = (EphyBookmark *)l->data;
    g_autofree char *title_lower = g_utf8_strdown (bm->title, -1);
    g_autofree char *url_lower = g_utf8_strdown (bm->url, -1);
    if (strstr (title_lower, query_lower) || strstr (url_lower, query_lower)) {
      GtkWidget *row = create_suggestion_row ("starred-symbolic", bm->title, bm->url, bm->url);
      gtk_list_box_append (GTK_LIST_BOX (sug->list_box), row);
      count++;
    }
  }

  /* 3. Search History */
  GList *hist = ephy_history_get_all ();
  for (GList *l = hist; l != NULL && count < 10; l = l->next) {
    EphyHistoryEntry *h = (EphyHistoryEntry *)l->data;
    g_autofree char *title_lower = g_utf8_strdown (h->title, -1);
    g_autofree char *url_lower = g_utf8_strdown (h->url, -1);
    if (strstr (title_lower, query_lower) || strstr (url_lower, query_lower)) {
      GtkWidget *row = create_suggestion_row ("document-open-recent-symbolic", h->title, h->url, h->url);
      gtk_list_box_append (GTK_LIST_BOX (sug->list_box), row);
      count++;
    }
  }

  if (count > 0) {
    gtk_popover_popup (GTK_POPOVER (sug->popover));
  } else {
    ephy_suggestions_hide (sug);
  }
}

void ephy_suggestions_hide (EphySuggestions *sug) {
  if (sug && sug->popover)
    gtk_popover_popdown (GTK_POPOVER (sug->popover));
}
