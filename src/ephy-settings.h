#pragma once

#include <glib.h>

typedef struct {
  char *search_engine; /* "duckduckgo", "google", "bing", "ecosia" */
  char *homepage;
  int color_scheme; /* 0 = default, 1 = force light, 2 = force dark */
} EphySettings;

EphySettings *ephy_settings_get_default (void);
void ephy_settings_save (EphySettings *settings);
void ephy_settings_set_search_engine (EphySettings *settings, const char *engine);
void ephy_settings_set_homepage (EphySettings *settings, const char *homepage);
void ephy_settings_set_color_scheme (EphySettings *settings, int scheme);
char *ephy_settings_build_search_url (EphySettings *settings, const char *query);
char *ephy_settings_get_startpage_url (void);
char *ephy_get_profile_dir (void);
