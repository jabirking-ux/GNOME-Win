#include "ephy-session.h"
#include "ephy-window.h"
#include "ephy-settings.h"
#include <json-glib/json-glib.h>
#include <glib/gstdio.h>

typedef struct {
  char *title;
  char *url;
} ClosedTab;

static GList *closed_tabs = NULL;

void ephy_session_init (void) {
  /* Initialized */
}

static char *get_session_path (void) {
  g_autofree char *dir = ephy_get_profile_dir ();
  return g_build_filename (dir, "session.json", NULL);
}

void ephy_session_save (EphyWindow *window) {
  if (!window || window->is_private) return;

  g_autoptr (JsonBuilder) b = json_builder_new ();
  json_builder_begin_object (b);

  json_builder_set_member_name (b, "tabs");
  json_builder_begin_array (b);

  for (GList *l = window->tabs; l != NULL; l = l->next) {
    EphyTab *tab = (EphyTab *)l->data;
    if (tab && tab->url && *tab->url) {
      if (g_str_has_prefix (tab->url, "file://") && g_strrstr (tab->url, "startpage"))
        continue; /* Don't force save local file startpage path */

      json_builder_begin_object (b);
      json_builder_set_member_name (b, "title");
      json_builder_add_string_value (b, tab->title ? tab->title : "");
      json_builder_set_member_name (b, "url");
      json_builder_add_string_value (b, tab->url);
      json_builder_end_object (b);
    }
  }

  json_builder_end_array (b);
  json_builder_end_object (b);

  g_autoptr (JsonGenerator) gen = json_generator_new ();
  JsonNode *root = json_builder_get_root (b);
  json_generator_set_root (gen, root);
  json_generator_set_pretty (gen, TRUE);

  g_autofree char *path = get_session_path ();
  json_generator_to_file (gen, path, NULL);
  json_node_free (root);
}

gboolean ephy_session_restore (EphyWindow *window) {
  if (!window || window->is_private) return FALSE;

  g_autofree char *path = get_session_path ();
  if (!g_file_test (path, G_FILE_TEST_EXISTS)) return FALSE;

  g_autoptr (JsonParser) parser = json_parser_new ();
  if (!json_parser_load_from_file (parser, path, NULL)) return FALSE;

  JsonNode *root = json_parser_get_root (parser);
  if (!root || !JSON_NODE_HOLDS_OBJECT (root)) return FALSE;

  JsonObject *obj = json_node_get_object (root);
  if (!json_object_has_member (obj, "tabs")) return FALSE;

  JsonArray *arr = json_object_get_array_member (obj, "tabs");
  guint len = json_array_get_length (arr);
  if (len == 0) return FALSE;

  gboolean restored_any = FALSE;
  for (guint i = 0; i < len; i++) {
    JsonObject *tobj = json_array_get_object_element (arr, i);
    const char *url = json_object_get_string_member (tobj, "url");
    if (url && *url) {
      if (!restored_any && window->active_tab) {
        /* Reuse initial active tab */
        ephy_tab_load_url (window->active_tab, url);
        restored_any = TRUE;
      } else {
        ephy_window_open_url (window, url);
        restored_any = TRUE;
      }
    }
  }

  return restored_any;
}

void ephy_session_push_closed_tab (const char *title, const char *url) {
  if (!url || !*url) return;
  if (g_str_has_prefix (url, "file://") && g_strrstr (url, "startpage")) return;

  ClosedTab *ct = g_new0 (ClosedTab, 1);
  ct->title = g_strdup (title ? title : url);
  ct->url = g_strdup (url);
  closed_tabs = g_list_prepend (closed_tabs, ct);

  /* Limit to last 25 closed tabs */
  if (g_list_length (closed_tabs) > 25) {
    GList *last = g_list_last (closed_tabs);
    ClosedTab *old = (ClosedTab *)last->data;
    g_free (old->title);
    g_free (old->url);
    g_free (old);
    closed_tabs = g_list_delete_link (closed_tabs, last);
  }
}

gboolean ephy_session_can_reopen_closed_tab (void) {
  return closed_tabs != NULL;
}

char *ephy_session_pop_closed_tab (char **out_title) {
  if (!closed_tabs) return NULL;

  ClosedTab *ct = (ClosedTab *)closed_tabs->data;
  closed_tabs = g_list_delete_link (closed_tabs, closed_tabs);

  char *url = ct->url;
  if (out_title)
    *out_title = ct->title;
  else
    g_free (ct->title);

  g_free (ct);
  return url;
}
