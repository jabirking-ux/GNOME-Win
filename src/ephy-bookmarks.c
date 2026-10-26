#include "ephy-bookmarks.h"
#include "ephy-settings.h"
#include <json-glib/json-glib.h>

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
          if (title && url) {
            EphyBookmark *bm = g_new0 (EphyBookmark, 1);
            bm->title = g_strdup (title);
            bm->url = g_strdup (url);
            bookmarks = g_list_append (bookmarks, bm);
          }
        }
      }
    }
  }

  /* Default bookmarks if empty */
  if (!bookmarks) {
    ephy_bookmarks_add ("GNOME", "https://www.gnome.org");
    ephy_bookmarks_add ("WebKit", "https://webkit.org");
    ephy_bookmarks_add ("DuckDuckGo", "https://duckduckgo.com");
    ephy_bookmarks_add ("Wikipedia", "https://en.wikipedia.org");
    ephy_bookmarks_add ("GitHub", "https://github.com");
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
  if (!url) return;
  ephy_bookmarks_init ();
  if (ephy_bookmarks_is_bookmarked (url)) return;

  EphyBookmark *bm = g_new0 (EphyBookmark, 1);
  bm->title = g_strdup (title && *title ? title : url);
  bm->url = g_strdup (url);
  bookmarks = g_list_append (bookmarks, bm);
  save_bookmarks ();
}

void ephy_bookmarks_remove (const char *url) {
  if (!url) return;
  ephy_bookmarks_init ();
  for (GList *l = bookmarks; l != NULL; l = l->next) {
    EphyBookmark *bm = (EphyBookmark *)l->data;
    if (g_strcmp0 (bm->url, url) == 0) {
      g_free (bm->title);
      g_free (bm->url);
      g_free (bm);
      bookmarks = g_list_delete_link (bookmarks, l);
      save_bookmarks ();
      break;
    }
  }
}
