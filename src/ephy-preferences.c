#include "ephy-preferences.h"
#include "ephy-settings.h"
#include "ephy-theme.h"
#include "ephy-history.h"
#include "ephy-engine.h"

static void on_safari_toggled (GObject *object, GParamSpec *pspec, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  gboolean active = adw_switch_row_get_active (ADW_SWITCH_ROW (object));
  EphySettings *settings = ephy_settings_get_default ();
  ephy_settings_set_safari_theme (settings, active);
  ephy_theme_apply_safari (active);
  ephy_window_sync_safari_theme (window, active);
}

static void on_color_scheme_changed (GObject *object, GParamSpec *pspec, gpointer user_data) {
  guint selected = adw_combo_row_get_selected (ADW_COMBO_ROW (object));
  EphySettings *settings = ephy_settings_get_default ();
  ephy_settings_set_color_scheme (settings, (int)selected);

  AdwStyleManager *sm = adw_style_manager_get_default ();
  if (selected == 1)
    adw_style_manager_set_color_scheme (sm, ADW_COLOR_SCHEME_FORCE_LIGHT);
  else if (selected == 2)
    adw_style_manager_set_color_scheme (sm, ADW_COLOR_SCHEME_FORCE_DARK);
  else
    adw_style_manager_set_color_scheme (sm, ADW_COLOR_SCHEME_DEFAULT);
}

static void on_engine_changed (GObject *object, GParamSpec *pspec, gpointer user_data) {
  guint selected = adw_combo_row_get_selected (ADW_COMBO_ROW (object));
  const char *engines[] = { "duckduckgo", "google", "bing", "ecosia" };
  if (selected < G_N_ELEMENTS (engines)) {
    EphySettings *settings = ephy_settings_get_default ();
    ephy_settings_set_search_engine (settings, engines[selected]);
  }
}

static void on_homepage_changed (GtkEditable *editable, gpointer user_data) {
  const char *text = gtk_editable_get_text (editable);
  EphySettings *settings = ephy_settings_get_default ();
  ephy_settings_set_homepage (settings, text);
}

static void on_clear_history_clicked (GtkButton *btn, gpointer user_data) {
  ephy_history_clear ();
  gtk_button_set_label (btn, "Cleared!");
}

static void on_clear_data_clicked (GtkButton *btn, gpointer user_data) {
  ephy_engine_clear_browsing_data ();
  gtk_button_set_label (btn, "Cleared!");
}

void ephy_preferences_show (EphyWindow *window) {
  EphySettings *settings = ephy_settings_get_default ();

  GtkWidget *pref_win = adw_preferences_window_new ();
  adw_preferences_window_set_search_enabled (ADW_PREFERENCES_WINDOW (pref_win), TRUE);
  gtk_window_set_transient_for (GTK_WINDOW (pref_win), GTK_WINDOW (window->app_window));
  gtk_window_set_modal (GTK_WINDOW (pref_win), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (pref_win), 640, 520);

  /* Page: General */
  AdwPreferencesPage *page_general = ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
  adw_preferences_page_set_title (page_general, "General");
  adw_preferences_page_set_icon_name (page_general, "preferences-system-symbolic");

  /* Group: Appearance */
  AdwPreferencesGroup *grp_appearance = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (grp_appearance, "Appearance & Theme");
  adw_preferences_group_set_description (grp_appearance, "Choose between the standard GNOME Web look and the Safari community theme.");

  GtkWidget *safari_row = adw_switch_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (safari_row), "Safari Community Theme");
  adw_action_row_set_subtitle (ADW_ACTION_ROW (safari_row), "Enable macOS 26 Safari inspired traffic-light controls and compact styling");
  adw_switch_row_set_active (ADW_SWITCH_ROW (safari_row), settings->safari_theme_enabled);
  g_signal_connect (safari_row, "notify::active", G_CALLBACK (on_safari_toggled), window);
  adw_preferences_group_add (grp_appearance, safari_row);

  GtkWidget *color_row = adw_combo_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (color_row), "Color Scheme");
  const char *schemes[] = { "Follow System", "Always Light", "Always Dark", NULL };
  adw_combo_row_set_model (ADW_COMBO_ROW (color_row), G_LIST_MODEL (gtk_string_list_new (schemes)));
  adw_combo_row_set_selected (ADW_COMBO_ROW (color_row), settings->color_scheme);
  g_signal_connect (color_row, "notify::selected", G_CALLBACK (on_color_scheme_changed), window);
  adw_preferences_group_add (grp_appearance, color_row);

  adw_preferences_page_add (page_general, grp_appearance);

  /* Group: Search & Navigation */
  AdwPreferencesGroup *grp_nav = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (grp_nav, "Search & Startup");

  GtkWidget *search_row = adw_combo_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (search_row), "Default Search Engine");
  const char *engine_names[] = { "DuckDuckGo", "Google", "Bing", "Ecosia", NULL };
  adw_combo_row_set_model (ADW_COMBO_ROW (search_row), G_LIST_MODEL (gtk_string_list_new (engine_names)));
  int engine_idx = 0;
  if (g_ascii_strcasecmp (settings->search_engine, "google") == 0) engine_idx = 1;
  else if (g_ascii_strcasecmp (settings->search_engine, "bing") == 0) engine_idx = 2;
  else if (g_ascii_strcasecmp (settings->search_engine, "ecosia") == 0) engine_idx = 3;
  adw_combo_row_set_selected (ADW_COMBO_ROW (search_row), engine_idx);
  g_signal_connect (search_row, "notify::selected", G_CALLBACK (on_engine_changed), window);
  adw_preferences_group_add (grp_nav, search_row);

  GtkWidget *home_row = adw_entry_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (home_row), "Homepage URL");
  gtk_editable_set_text (GTK_EDITABLE (home_row), settings->homepage ? settings->homepage : "");
  g_signal_connect (home_row, "changed", G_CALLBACK (on_homepage_changed), window);
  adw_preferences_group_add (grp_nav, home_row);

  adw_preferences_page_add (page_general, grp_nav);
  adw_preferences_window_add (ADW_PREFERENCES_WINDOW (pref_win), page_general);

  /* Page: Privacy */
  AdwPreferencesPage *page_privacy = ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
  adw_preferences_page_set_title (page_privacy, "Privacy");
  adw_preferences_page_set_icon_name (page_privacy, "security-high-symbolic");

  AdwPreferencesGroup *grp_data = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (grp_data, "Browsing Data");

  GtkWidget *clear_hist_row = adw_action_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (clear_hist_row), "Browsing History");
  GtkWidget *btn_hist = gtk_button_new_with_label ("Clear History");
  gtk_widget_set_valign (btn_hist, GTK_ALIGN_CENTER);
  g_signal_connect (btn_hist, "clicked", G_CALLBACK (on_clear_history_clicked), NULL);
  adw_action_row_add_suffix (ADW_ACTION_ROW (clear_hist_row), btn_hist);
  adw_preferences_group_add (grp_data, clear_hist_row);

  GtkWidget *clear_cache_row = adw_action_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (clear_cache_row), "Cookies & Cache");
  GtkWidget *btn_cache = gtk_button_new_with_label ("Clear Data");
  gtk_widget_set_valign (btn_cache, GTK_ALIGN_CENTER);
  g_signal_connect (btn_cache, "clicked", G_CALLBACK (on_clear_data_clicked), NULL);
  adw_action_row_add_suffix (ADW_ACTION_ROW (clear_cache_row), btn_cache);
  adw_preferences_group_add (grp_data, clear_cache_row);

  adw_preferences_page_add (page_privacy, grp_data);
  adw_preferences_window_add (ADW_PREFERENCES_WINDOW (pref_win), page_privacy);

  gtk_window_present (GTK_WINDOW (pref_win));
}
