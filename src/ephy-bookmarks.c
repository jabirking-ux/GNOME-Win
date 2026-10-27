#include "ephy-bookmarks.h"
#include "ephy-settings.h"
#include "ephy-window.h"
#include <json-glib/json-glib.h>
#include <adwaita.h>

static GList *bookmarks = NULL;
static gboolean initialized = FALSE;

static char *get_bookmarks_file_path (void) {
  g_autofree char *dir = ephy_get_profile_dir ();
  return g_build_filename (dir, "bookmarks.json", NULL);
}

static void save_bookmarks (void) {
  g_autoptr (JsonBuilder) builder = json_builder_new ();
  json_builder_begin_array (builder);

  for (GList *l = bookmarks; l != NULL; l = l->next) {
    EphyBookmark *bm = (EphyBookmark *)l->data;
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "title");
    json_builder_add_string_value (builder, bm->title);
    json_builder_set_member_name (builder, "url");
    json_builder_add_string_value (builder, bm->url);
    if (bm->tag && *bm->tag) {
      json_builder_set_member_name (builder, "tag");
      json_builder_add_string_value (builder, bm->tag);
    }
    json_builder_end_object (builder);
  }

  json_builder_end_array (builder);

  g_autoptr (JsonGenerator) gen = json_generator_new ();
  json_generator_set_pretty (gen, TRUE);
  json_generator_set_root (gen, json_builder_get_root (builder));

  g_autofree char *path = get_bookmarks_file_path ();
  json_generator_to_file (gen, path, NULL);
}

void ephy_bookmarks_init (void) {
  if (initialized) return;
  initialized = TRUE;

  g_autofree char *path = get_bookmarks_file_path ();
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
          const char *tag = json_object_has_member (obj, "tag") ? json_object_get_string_member (obj, "tag") : NULL;
          if (title && url) {
            EphyBookmark *bm = g_new0 (EphyBookmark, 1);
            bm->title = g_strdup (title);
            bm->url = g_strdup (url);
            bm->tag = g_strdup (tag ? tag : "");
            bookmarks = g_list_append (bookmarks, bm);
          }
        }
      }
    }
  }

  /* Default bookmarks if empty */
  if (!bookmarks) {
    ephy_bookmarks_add_with_tag ("GNOME", "https://www.gnome.org", "GNOME");
    ephy_bookmarks_add_with_tag ("GNOME Apps", "https://apps.gnome.org", "GNOME");
    ephy_bookmarks_add_with_tag ("WebKit", "https://webkit.org", "Dev");
    ephy_bookmarks_add_with_tag ("DuckDuckGo", "https://duckduckgo.com", "Search");
    ephy_bookmarks_add_with_tag ("Wikipedia", "https://en.wikipedia.org", "Reference");
    ephy_bookmarks_add_with_tag ("GitHub", "https://github.com", "Dev");
  }
}

GList *ephy_bookmarks_get_all (void) {
  ephy_bookmarks_init ();
  return bookmarks;
}

gboolean ephy_bookmarks_is_bookmarked (const char *url) {
  if (!url) return FALSE;
  ephy_bookmarks_init ();
  for (GList *l = bookmarks; l != NULL; l = l->next) {
    EphyBookmark *bm = (EphyBookmark *)l->data;
    if (g_strcmp0 (bm->url, url) == 0) return TRUE;
  }
  return FALSE;
}

void ephy_bookmarks_add (const char *title, const char *url) {
  ephy_bookmarks_add_with_tag (title, url, NULL);
}

void ephy_bookmarks_add_with_tag (const char *title, const char *url, const char *tag) {
  if (!url || !*url) return;
  ephy_bookmarks_init ();
  if (ephy_bookmarks_is_bookmarked (url)) return;

  EphyBookmark *bm = g_new0 (EphyBookmark, 1);
  bm->title = g_strdup (title && *title ? title : url);
  bm->url = g_strdup (url);
  bm->tag = g_strdup (tag ? tag : "");
  bookmarks = g_list_append (bookmarks, bm);
  save_bookmarks ();
}

void ephy_bookmarks_remove (const char *url) {
  if (!url) return;
  ephy_bookmarks_init ();
  for (GList *l = bookmarks; l != NULL; l = l->next) {
    EphyBookmark *bm = (EphyBookmark *)l->data;
    if (g_strcmp0 (bm->url, url) == 0) {
      bookmarks = g_list_delete_link (bookmarks, l);
      g_free (bm->title);
      g_free (bm->url);
      g_free (bm->tag);
      g_free (bm);
      save_bookmarks ();
      break;
    }
  }
}

gboolean ephy_bookmarks_export_html (const char *filepath) {
  ephy_bookmarks_init ();
  GString *html = g_string_new (
    "<!DOCTYPE NETSCAPE-Bookmark-file-1>\n"
    "<!-- This is an automatically generated file. -->\n"
    "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=UTF-8\">\n"
    "<TITLE>Bookmarks</TITLE>\n"
    "<H1>Bookmarks</H1>\n"
    "<DL><p>\n"
  );

  for (GList *l = bookmarks; l != NULL; l = l->next) {
    EphyBookmark *bm = (EphyBookmark *)l->data;
    g_string_append_printf (html, "    <DT><A HREF=\"%s\">%s</A>\n", bm->url, bm->title);
  }
  g_string_append (html, "</DL><p>\n");

  GError *err = NULL;
  gboolean ok = g_file_set_contents (filepath, html->str, html->len, &err);
  g_string_free (html, TRUE);
  if (err) g_error_free (err);
  return ok;
}

gboolean ephy_bookmarks_import_html (const char *filepath) {
  char *contents = NULL;
  gsize length = 0;
  if (!g_file_get_contents (filepath, &contents, &length, NULL)) return FALSE;

  char *pos = contents;
  while ((pos = strstr (pos, "<A HREF=\"")) != NULL) {
    pos += 9;
    char *end_url = strchr (pos, '"');
    if (!end_url) break;
    char *url = g_strndup (pos, end_url - pos);

    char *start_title = strchr (end_url, '>');
    if (!start_title) { g_free (url); break; }
    start_title++;

    char *end_title = strstr (start_title, "</A>");
    if (!end_title) { g_free (url); break; }
    char *title = g_strndup (start_title, end_title - start_title);

    ephy_bookmarks_add (title, url);
    g_free (url);
    g_free (title);
    pos = end_title + 4;
  }

  g_free (contents);
  return TRUE;
}

static void on_bookmark_row_delete (GtkButton *btn, gpointer user_data) {
  EphyBookmark *bm = (EphyBookmark *)user_data;
  GtkWidget *row = GTK_WIDGET (g_object_get_data (G_OBJECT (btn), "parent-row"));
  GtkWidget *list_box = gtk_widget_get_parent (row);
  ephy_bookmarks_remove (bm->url);
  if (list_box && row)
    gtk_list_box_remove (GTK_LIST_BOX (list_box), row);
}

static void on_bookmark_row_activated (GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  const char *url = (const char *)g_object_get_data (G_OBJECT (row), "bm-url");
  if (url && window && window->active_tab)
    ephy_tab_load_url (window->active_tab, url);
}

static void on_export_clicked (GtkButton *btn, gpointer user_data) {
  const char *downloads_dir = g_get_user_special_dir (G_USER_DIRECTORY_DOWNLOAD);
  if (!downloads_dir) downloads_dir = g_get_home_dir ();
  g_autofree char *path = g_build_filename (downloads_dir, "gnome_web_bookmarks.html", NULL);
  if (ephy_bookmarks_export_html (path)) {
    gtk_button_set_label (btn, "Exported!");
  }
}

void ephy_bookmarks_dialog_show (GtkWindow *parent, EphyWindow *window) {
  GtkWidget *dialog = adw_preferences_window_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (dialog), 640, 520);

  AdwPreferencesPage *page = ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
  adw_preferences_page_set_title (page, "Bookmarks");
  adw_preferences_page_set_icon_name (page, "user-bookmarks-symbolic");

  AdwPreferencesGroup *grp = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (grp, "Saved Bookmarks");
  adw_preferences_group_set_description (grp, "Click any bookmark to open. Click the trash icon to delete.");

  GtkWidget *export_btn = gtk_button_new_with_label ("Export HTML");
  gtk_widget_set_tooltip_text (export_btn, "Export to Downloads/gnome_web_bookmarks.html");
  g_signal_connect (export_btn, "clicked", G_CALLBACK (on_export_clicked), NULL);
  adw_preferences_group_set_header_suffix (grp, export_btn);

  GList *list = ephy_bookmarks_get_all ();
  for (GList *l = list; l != NULL; l = l->next) {
    EphyBookmark *bm = (EphyBookmark *)l->data;
    GtkWidget *row = adw_action_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), bm->title);
    adw_action_row_set_subtitle (ADW_ACTION_ROW (row), bm->url);

    GtkWidget *star = gtk_image_new_from_icon_name ("starred-symbolic");
    adw_action_row_add_prefix (ADW_ACTION_ROW (row), star);

    GtkWidget *del_btn = gtk_button_new_from_icon_name ("user-trash-symbolic");
    gtk_button_set_has_frame (GTK_BUTTON (del_btn), FALSE);
    gtk_widget_set_tooltip_text (del_btn, "Delete bookmark");
    g_object_set_data (G_OBJECT (del_btn), "parent-row", row);
    g_signal_connect (del_btn, "clicked", G_CALLBACK (on_bookmark_row_delete), bm);
    adw_action_row_add_suffix (ADW_ACTION_ROW (row), del_btn);

    gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (row), TRUE);
    g_object_set_data_full (G_OBJECT (row), "bm-url", g_strdup (bm->url), g_free);
    g_signal_connect (row, "activated", G_CALLBACK (on_bookmark_row_activated), window);

    adw_preferences_group_add (grp, row);
  }

  adw_preferences_page_add (page, grp);
  adw_preferences_window_add (ADW_PREFERENCES_WINDOW (dialog), page);
  gtk_window_present (GTK_WINDOW (dialog));
}
