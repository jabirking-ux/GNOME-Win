#include "ephy-settings.h"
#include <json-glib/json-glib.h>
#include <glib/gstdio.h>
#include <windows.h>

static EphySettings default_settings = {
  .search_engine = NULL,
  .homepage = NULL,
  .color_scheme = 0,
  .restore_session = TRUE,
  .new_tab_page = NULL,
  .downloads_dir = NULL,
  .enable_itp = TRUE
};

char *ephy_settings_get_startpage_url (void) {
  wchar_t exe_wpath[MAX_PATH];
  if (GetModuleFileNameW (NULL, exe_wpath, MAX_PATH)) {
    char *exe_path = g_utf16_to_utf8 (exe_wpath, -1, NULL, NULL, NULL);
    if (exe_path) {
      char *dir = g_path_get_dirname (exe_path);
      char *candidate = g_build_filename (dir, "startpage", "index.html", NULL);
      g_free (dir);
      g_free (exe_path);
      if (g_file_test (candidate, G_FILE_TEST_EXISTS)) {
        char *uri = g_filename_to_uri (candidate, NULL, NULL);
        g_free (candidate);
        return uri;
      }
      g_free (candidate);
    }
  }

  /* Fallback: try data/startpage/index.html relative to current directory */
  if (g_file_test ("data/startpage/index.html", G_FILE_TEST_EXISTS)) {
    g_autofree char *abs_path = g_canonicalize_filename ("data/startpage/index.html", NULL);
    return g_filename_to_uri (abs_path, NULL, NULL);
  }

  return g_strdup ("https://duckduckgo.com");
}

char *ephy_get_profile_dir (void) {
  const char *appdata = g_get_user_config_dir ();
  char *dir = g_build_filename (appdata, "GNOME-Web", NULL);
  g_mkdir_with_parents (dir, 0700);
  return dir;
}

static char *get_settings_file_path (void) {
  g_autofree char *dir = ephy_get_profile_dir ();
  return g_build_filename (dir, "settings.json", NULL);
}

EphySettings *ephy_settings_get_default (void) {
  static gboolean initialized = FALSE;
  if (!initialized) {
    initialized = TRUE;
    default_settings.search_engine = g_strdup ("duckduckgo");
    default_settings.homepage = g_strdup ("about:start");
    default_settings.new_tab_page = g_strdup ("startpage");
    default_settings.restore_session = TRUE;
    default_settings.enable_itp = TRUE;

    const char *d = g_get_user_special_dir (G_USER_DIRECTORY_DOWNLOAD);
    default_settings.downloads_dir = g_strdup (d ? d : g_get_home_dir ());

    g_autofree char *path = get_settings_file_path ();
    if (g_file_test (path, G_FILE_TEST_EXISTS)) {
      g_autoptr (JsonParser) parser = json_parser_new ();
      if (json_parser_load_from_file (parser, path, NULL)) {
        JsonNode *root = json_parser_get_root (parser);
        if (JSON_NODE_HOLDS_OBJECT (root)) {
          JsonObject *obj = json_node_get_object (root);
          if (json_object_has_member (obj, "search_engine")) {
            g_free (default_settings.search_engine);
            default_settings.search_engine = g_strdup (json_object_get_string_member (obj, "search_engine"));
          }
          if (json_object_has_member (obj, "homepage")) {
            const char *h = json_object_get_string_member (obj, "homepage");
            if (h && *h) {
              g_free (default_settings.homepage);
              default_settings.homepage = g_strdup (h);
            }
          }
          if (json_object_has_member (obj, "color_scheme"))
            default_settings.color_scheme = (int)json_object_get_int_member (obj, "color_scheme");
          if (json_object_has_member (obj, "restore_session"))
            default_settings.restore_session = json_object_get_boolean_member (obj, "restore_session");
          if (json_object_has_member (obj, "new_tab_page")) {
            g_free (default_settings.new_tab_page);
            default_settings.new_tab_page = g_strdup (json_object_get_string_member (obj, "new_tab_page"));
          }
          if (json_object_has_member (obj, "downloads_dir")) {
            g_free (default_settings.downloads_dir);
            default_settings.downloads_dir = g_strdup (json_object_get_string_member (obj, "downloads_dir"));
          }
          if (json_object_has_member (obj, "enable_itp"))
            default_settings.enable_itp = json_object_get_boolean_member (obj, "enable_itp");
        }
      }
    }
  }
  return &default_settings;
}

void ephy_settings_save (EphySettings *settings) {
  g_autoptr (JsonBuilder) builder = json_builder_new ();
  json_builder_begin_object (builder);

  json_builder_set_member_name (builder, "search_engine");
  json_builder_add_string_value (builder, settings->search_engine ? settings->search_engine : "duckduckgo");

  json_builder_set_member_name (builder, "homepage");
  json_builder_add_string_value (builder, settings->homepage ? settings->homepage : "about:start");

  json_builder_set_member_name (builder, "color_scheme");
  json_builder_add_int_value (builder, settings->color_scheme);

  json_builder_set_member_name (builder, "restore_session");
  json_builder_add_boolean_value (builder, settings->restore_session);

  json_builder_set_member_name (builder, "new_tab_page");
  json_builder_add_string_value (builder, settings->new_tab_page ? settings->new_tab_page : "startpage");

  json_builder_set_member_name (builder, "downloads_dir");
  json_builder_add_string_value (builder, settings->downloads_dir ? settings->downloads_dir : "");

  json_builder_set_member_name (builder, "enable_itp");
  json_builder_add_boolean_value (builder, settings->enable_itp);

  json_builder_end_object (builder);

  g_autoptr (JsonGenerator) gen = json_generator_new ();
  JsonNode *root = json_builder_get_root (builder);
  json_generator_set_root (gen, root);
  json_generator_set_pretty (gen, TRUE);

  g_autofree char *path = get_settings_file_path ();
  json_generator_to_file (gen, path, NULL);
  json_node_free (root);
}

void ephy_settings_set_search_engine (EphySettings *settings, const char *engine) {
  g_free (settings->search_engine);
  settings->search_engine = g_strdup (engine);
  ephy_settings_save (settings);
}

void ephy_settings_set_homepage (EphySettings *settings, const char *homepage) {
  g_free (settings->homepage);
  settings->homepage = g_strdup (homepage);
  ephy_settings_save (settings);
}

void ephy_settings_set_color_scheme (EphySettings *settings, int scheme) {
  settings->color_scheme = scheme;
  ephy_settings_save (settings);
}

void ephy_settings_set_restore_session (EphySettings *settings, gboolean restore) {
  settings->restore_session = restore;
  ephy_settings_save (settings);
}

void ephy_settings_set_new_tab_page (EphySettings *settings, const char *mode) {
  g_free (settings->new_tab_page);
  settings->new_tab_page = g_strdup (mode);
  ephy_settings_save (settings);
}

void ephy_settings_set_downloads_dir (EphySettings *settings, const char *dir) {
  g_free (settings->downloads_dir);
  settings->downloads_dir = g_strdup (dir);
  ephy_settings_save (settings);
}

char *ephy_settings_build_search_url (EphySettings *settings, const char *query) {
  g_autofree char *escaped = g_uri_escape_string (query, NULL, TRUE);
  const char *engine = settings->search_engine ? settings->search_engine : "duckduckgo";

  if (g_strcmp0 (engine, "google") == 0) {
    return g_strdup_printf ("https://www.google.com/search?q=%s", escaped);
  } else if (g_strcmp0 (engine, "bing") == 0) {
    return g_strdup_printf ("https://www.bing.com/search?q=%s", escaped);
  } else if (g_strcmp0 (engine, "ecosia") == 0) {
    return g_strdup_printf ("https://www.ecosia.org/search?q=%s", escaped);
  }
  return g_strdup_printf ("https://duckduckgo.com/?q=%s", escaped);
}
