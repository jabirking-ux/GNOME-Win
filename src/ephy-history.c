#include "ephy-history.h"
#include "ephy-settings.h"
#include "ephy-window.h"
#include <json-glib/json-glib.h>
#include <adwaita.h>

static GList *history = NULL;
static gboolean initialized = FALSE;

static char *get_history_file_path (void) {
  g_autofree char *dir = ephy_get_profile_dir ();
  return g_build_filename (dir, "history.json", NULL);
}

static void save_history (void) {
  g_autoptr (JsonBuilder) builder = json_builder_new ();
  json_builder_begin_array (builder);

  int count = 0;
  for (GList *l = history; l != NULL && count < 2000; l = l->next, count++) {
    EphyHistoryEntry *item = (EphyHistoryEntry *)l->data;
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "title");
    json_builder_add_string_value (builder, item->title);
    json_builder_set_member_name (builder, "url");
    json_builder_add_string_value (builder, item->url);
    json_builder_set_member_name (builder, "time");
    json_builder_add_int_value (builder, item->timestamp);
    json_builder_end_object (builder);
  }

  json_builder_end_array (builder);

  g_autoptr (JsonGenerator) gen = json_generator_new ();
  json_generator_set_pretty (gen, TRUE);
  json_generator_set_root (gen, json_builder_get_root (builder));

  g_autofree char *path = get_history_file_path ();
  json_generator_to_file (gen, path, NULL);
}

void ephy_history_init (void) {
  if (initialized) return;
  initialized = TRUE;

  g_autofree char *path = get_history_file_path ();
  if (g_file_test (path, G_FILE_TEST_EXISTS)) {
    g_autoptr (JsonParser) parser = json_parser_new ();
    if (json_parser_load_from_file (parser, path, NULL)) {
      JsonNode *root = json_parser_get_root (parser);
      if (JSON_NODE_HOLDS_ARRAY (root)) {
        JsonArray *arr = json_node_get_array (root);
        guint len = json_array_get_length (arr);
        for (guint i = 0; i < len; i++) {
          JsonObject *obj = json_array_get_object_element (arr, i);
          const char *title = json_object_get_string_member (obj, "title");
          const char *url = json_object_get_string_member (obj, "url");
          gint64 time = json_object_get_int_member (obj, "time");
          if (url) {
            EphyHistoryEntry *item = g_new0 (EphyHistoryEntry, 1);
            item->title = g_strdup (title ? title : url);
            item->url = g_strdup (url);
            item->timestamp = time;
            history = g_list_append (history, item);
          }
        }
      }
    }
  }
}

GList *ephy_history_get_all (void) {
  ephy_history_init ();
  return history;
}

void ephy_history_add (const char *title, const char *url) {
  if (!url || !*url) return;
  if (g_str_has_prefix (url, "about:")) return;

  ephy_history_init ();

  /* Avoid duplicate sequential entry */
  if (history) {
    EphyHistoryEntry *first = (EphyHistoryEntry *)history->data;
    if (g_strcmp0 (first->url, url) == 0) {
      if (title && *title) {
        g_free (first->title);
        first->title = g_strdup (title);
      }
      return;
    }
  }

  EphyHistoryEntry *item = g_new0 (EphyHistoryEntry, 1);
  item->title = g_strdup (title && *title ? title : url);
  item->url = g_strdup (url);
  item->timestamp = g_get_real_time () / G_USEC_PER_SEC;

  history = g_list_prepend (history, item);
  save_history ();
}

void ephy_history_remove (const char *url) {
  if (!url) return;
  ephy_history_init ();
  for (GList *l = history; l != NULL; l = l->next) {
    EphyHistoryEntry *item = (EphyHistoryEntry *)l->data;
    if (g_strcmp0 (item->url, url) == 0) {
      history = g_list_delete_link (history, l);
      g_free (item->title);
      g_free (item->url);
      g_free (item);
      save_history ();
      break;
    }
  }
}

void ephy_history_clear (void) {
  for (GList *l = history; l != NULL; l = l->next) {
    EphyHistoryEntry *item = (EphyHistoryEntry *)l->data;
    g_free (item->title);
    g_free (item->url);
    g_free (item);
  }
  g_list_free (history);
  history = NULL;
  save_history ();
}

void ephy_history_clear_range (gint64 since_timestamp) {
  GList *l = history;
  while (l) {
    GList *next = l->next;
    EphyHistoryEntry *item = (EphyHistoryEntry *)l->data;
    if (item->timestamp >= since_timestamp) {
      history = g_list_delete_link (history, l);
      g_free (item->title);
      g_free (item->url);
      g_free (item);
    }
    l = next;
  }
  save_history ();
}

static void on_history_row_delete (GtkButton *btn, gpointer user_data) {
  EphyHistoryEntry *item = (EphyHistoryEntry *)user_data;
  GtkWidget *row = GTK_WIDGET (g_object_get_data (G_OBJECT (btn), "parent-row"));
  GtkWidget *list_box = gtk_widget_get_parent (row);
  ephy_history_remove (item->url);
  if (list_box && row)
    gtk_list_box_remove (GTK_LIST_BOX (list_box), row);
}

static void on_clear_history_dialog_clicked (GtkButton *btn, gpointer user_data) {
  ephy_history_clear ();
  gtk_button_set_label (btn, "Cleared!");
}

static void on_history_row_activated (GtkListBoxRow *row, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  const char *url = (const char *)g_object_get_data (G_OBJECT (row), "hist-url");
  if (url && window && window->active_tab)
    ephy_tab_load_url (window->active_tab, url);
}

void ephy_history_dialog_show (GtkWindow *parent, EphyWindow *window) {
  GtkWidget *dialog = adw_preferences_window_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (dialog), 680, 540);

  AdwPreferencesPage *page = ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
  adw_preferences_page_set_title (page, "Browsing History");
  adw_preferences_page_set_icon_name (page, "document-open-recent-symbolic");

  AdwPreferencesGroup *grp = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (grp, "Recent Visits");
  adw_preferences_group_set_description (grp, "Websites you have visited recently. Click any entry to re-open.");

  GtkWidget *clear_btn = gtk_button_new_with_label ("Clear All");
  gtk_widget_add_css_class (clear_btn, "destructive-action");
  g_signal_connect (clear_btn, "clicked", G_CALLBACK (on_clear_history_dialog_clicked), NULL);
  adw_preferences_group_set_header_suffix (grp, clear_btn);

  GList *list = ephy_history_get_all ();
  int count = 0;
  for (GList *l = list; l != NULL && count < 100; l = l->next, count++) {
    EphyHistoryEntry *item = (EphyHistoryEntry *)l->data;
    GtkWidget *row = adw_action_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), item->title);
    adw_action_row_set_subtitle (ADW_ACTION_ROW (row), item->url);

    GtkWidget *icon = gtk_image_new_from_icon_name ("document-open-recent-symbolic");
    adw_action_row_add_prefix (ADW_ACTION_ROW (row), icon);

    GtkWidget *del_btn = gtk_button_new_from_icon_name ("user-trash-symbolic");
    gtk_button_set_has_frame (GTK_BUTTON (del_btn), FALSE);
    gtk_widget_set_tooltip_text (del_btn, "Delete from history");
    g_object_set_data (G_OBJECT (del_btn), "parent-row", row);
    g_signal_connect (del_btn, "clicked", G_CALLBACK (on_history_row_delete), item);
    adw_action_row_add_suffix (ADW_ACTION_ROW (row), del_btn);

    gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (row), TRUE);
    g_object_set_data_full (G_OBJECT (row), "hist-url", g_strdup (item->url), g_free);
    g_signal_connect (row, "activated", G_CALLBACK (on_history_row_activated), window);

    adw_preferences_group_add (grp, row);
  }

  adw_preferences_page_add (page, grp);
  adw_preferences_window_add (ADW_PREFERENCES_WINDOW (dialog), page);
  gtk_window_present (GTK_WINDOW (dialog));
}
