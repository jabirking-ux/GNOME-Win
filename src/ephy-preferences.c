#include "ephy-preferences.h"
#include "ephy-settings.h"
#include "ephy-history.h"
#include "ephy-engine.h"

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

static void on_restore_session_toggled (GObject *object, GParamSpec *pspec, gpointer user_data) {
  gboolean active = adw_switch_row_get_active (ADW_SWITCH_ROW (object));
  EphySettings *settings = ephy_settings_get_default ();
  ephy_settings_set_restore_session (settings, active);
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
  adw_preferences_group_set_title (grp_appearance, "Appearance");
  adw_preferences_group_set_description (grp_appearance, "Customize the look and feel of GNOME Web.");

  GtkWidget *color_row = adw_combo_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (color_row), "Color Scheme");
  const char *schemes[] = { "Follow System", "Always Light", "Always Dark", NULL };
  adw_combo_row_set_model (ADW_COMBO_ROW (color_row), G_LIST_MODEL (gtk_string_list_new (schemes)));
  adw_combo_row_set_selected (ADW_COMBO_ROW (color_row), (guint)settings->color_scheme);
  g_signal_connect (color_row, "notify::selected", G_CALLBACK (on_color_scheme_changed), NULL);
  adw_preferences_group_add (grp_appearance, color_row);

  /* Group: Browsing & Search */
  AdwPreferencesGroup *grp_search = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (grp_search, "Search & Navigation");

  GtkWidget *engine_row = adw_combo_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (engine_row), "Default Search Engine");
  const char *engines[] = { "DuckDuckGo", "Google", "Bing", "Ecosia", NULL };
  adw_combo_row_set_model (ADW_COMBO_ROW (engine_row), G_LIST_MODEL (gtk_string_list_new (engines)));

  int sel_engine = 0;
  if (g_strcmp0 (settings->search_engine, "google") == 0) sel_engine = 1;
  else if (g_strcmp0 (settings->search_engine, "bing") == 0) sel_engine = 2;
  else if (g_strcmp0 (settings->search_engine, "ecosia") == 0) sel_engine = 3;
  adw_combo_row_set_selected (ADW_COMBO_ROW (engine_row), sel_engine);
  g_signal_connect (engine_row, "notify::selected", G_CALLBACK (on_engine_changed), NULL);
  adw_preferences_group_add (grp_search, engine_row);

  GtkWidget *home_row = adw_entry_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (home_row), "Homepage");
  gtk_editable_set_text (GTK_EDITABLE (home_row), settings->homepage ? settings->homepage : "about:start");
  g_signal_connect (home_row, "changed", G_CALLBACK (on_homepage_changed), NULL);
  adw_preferences_group_add (grp_search, home_row);

  /* Group: Startup & Session */
  AdwPreferencesGroup *grp_session = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (grp_session, "Startup & Session");

  GtkWidget *restore_row = adw_switch_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (restore_row), "Restore Previous Session");
  adw_action_row_set_subtitle (ADW_ACTION_ROW (restore_row), "Automatically restore open tabs on browser startup");
  adw_switch_row_set_active (ADW_SWITCH_ROW (restore_row), settings->restore_session);
  g_signal_connect (restore_row, "notify::active", G_CALLBACK (on_restore_session_toggled), NULL);
  adw_preferences_group_add (grp_session, restore_row);

  adw_preferences_page_add (page_general, grp_appearance);
  adw_preferences_page_add (page_general, grp_search);
  adw_preferences_page_add (page_general, grp_session);
  adw_preferences_window_add (ADW_PREFERENCES_WINDOW (pref_win), page_general);

  /* Page: Downloads */
  AdwPreferencesPage *page_downloads = ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
  adw_preferences_page_set_title (page_downloads, "Downloads");
  adw_preferences_page_set_icon_name (page_downloads, "folder-download-symbolic");

  AdwPreferencesGroup *grp_dl = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (grp_dl, "Download Destination");

  GtkWidget *dl_row = adw_action_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (dl_row), "Save Files To");
  adw_action_row_set_subtitle (ADW_ACTION_ROW (dl_row), settings->downloads_dir ? settings->downloads_dir : "Downloads");
  adw_preferences_group_add (grp_dl, dl_row);

  adw_preferences_page_add (page_downloads, grp_dl);
  adw_preferences_window_add (ADW_PREFERENCES_WINDOW (pref_win), page_downloads);

  /* Page: Privacy & Security */
  AdwPreferencesPage *page_privacy = ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
  adw_preferences_page_set_title (page_privacy, "Privacy & Security");
  adw_preferences_page_set_icon_name (page_privacy, "security-high-symbolic");

  AdwPreferencesGroup *grp_privacy = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (grp_privacy, "Tracking & Protection");
  adw_preferences_group_set_description (grp_privacy, "WebKit WinCairo Multi-Process Security and Tracking Prevention.");

  GtkWidget *itp_row = adw_action_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (itp_row), "Intelligent Tracking Prevention");
  adw_action_row_set_subtitle (ADW_ACTION_ROW (itp_row), "Active (Blocks known cross-site tracking cookies and profiling)");
  GtkWidget *itp_icon = gtk_image_new_from_icon_name ("object-select-symbolic");
  adw_action_row_add_suffix (ADW_ACTION_ROW (itp_row), itp_icon);
  adw_preferences_group_add (grp_privacy, itp_row);

  GtkWidget *isolation_row = adw_action_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (isolation_row), "Process Isolation Model");
  adw_action_row_set_subtitle (ADW_ACTION_ROW (isolation_row), "Multi-Process (Separate processes for Network, GPU, and Web content)");
  GtkWidget *iso_icon = gtk_image_new_from_icon_name ("dialog-information-symbolic");
  adw_action_row_add_suffix (ADW_ACTION_ROW (isolation_row), iso_icon);
  adw_preferences_group_add (grp_privacy, isolation_row);

  AdwPreferencesGroup *grp_data = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (grp_data, "Clear Browsing Data");

  GtkWidget *hist_row = adw_action_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (hist_row), "Browsing History");
  GtkWidget *btn_hist = gtk_button_new_with_label ("Clear History");
  gtk_widget_set_valign (btn_hist, GTK_ALIGN_CENTER);
  gtk_widget_add_css_class (btn_hist, "destructive-action");
  g_signal_connect (btn_hist, "clicked", G_CALLBACK (on_clear_history_clicked), NULL);
  adw_action_row_add_suffix (ADW_ACTION_ROW (hist_row), btn_hist);
  adw_preferences_group_add (grp_data, hist_row);

  GtkWidget *cache_row = adw_action_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (cache_row), "Cookies and Website Data");
  GtkWidget *btn_cache = gtk_button_new_with_label ("Clear Data");
  gtk_widget_set_valign (btn_cache, GTK_ALIGN_CENTER);
  gtk_widget_add_css_class (btn_cache, "destructive-action");
  g_signal_connect (btn_cache, "clicked", G_CALLBACK (on_clear_data_clicked), NULL);
  adw_action_row_add_suffix (ADW_ACTION_ROW (cache_row), btn_cache);
  adw_preferences_group_add (grp_data, cache_row);

  adw_preferences_page_add (page_privacy, grp_privacy);
  adw_preferences_page_add (page_privacy, grp_data);
  adw_preferences_window_add (ADW_PREFERENCES_WINDOW (pref_win), page_privacy);

  gtk_window_present (GTK_WINDOW (pref_win));
}
