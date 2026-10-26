#include "ephy-history.h"
#include "ephy-settings.h"
#include <json-glib/json-glib.h>

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
  /* Don't record about: blank or private internal URLs */
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

  /* Prepend newest */
  history = g_list_prepend (history, item);
  save_history ();
}

void ephy_history_clear (void) {
  ephy_history_init ();
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
