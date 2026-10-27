#include "ephy-window.h"
#include "ephy-settings.h"
#include "ephy-theme.h"
#include "ephy-bookmarks.h"
#include "ephy-history.h"
#include "ephy-preferences.h"
#include "ephy-security.h"
#include <gdk/win32/gdkwin32.h>
#include <windows.h>

static BOOL CALLBACK enum_hide_consoles (HWND hwnd, LPARAM lParam) {
  char cname[256];
  if (GetClassNameA (hwnd, cname, sizeof (cname)) && strcmp (cname, "ConsoleWindowClass") == 0) {
    char title[512];
    if (GetWindowTextA (hwnd, title, sizeof (title))) {
      if (strstr (title, "WebKitWebProcess") || strstr (title, "WebKitNetworkProcess") || strstr (title, "WebKitGPUProcess")) {
        ShowWindow (hwnd, SW_HIDE);
      }
    }
  }
  return TRUE;
}

static gboolean on_geometry_timer (gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  if (window->active_tab) {
    ephy_tab_sync_geometry (window->active_tab);
    ephy_tab_poll_state (window->active_tab);
    ephy_window_update_tab_state (window, window->active_tab);
  }
  static int console_check = 0;
  if (++console_check % 15 == 0) {
    EnumWindows (enum_hide_consoles, 0);
  }
  return G_SOURCE_CONTINUE;
}

static void on_tab_selected (AdwTabView *tab_view, GParamSpec *pspec, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  AdwTabPage *selected = adw_tab_view_get_selected_page (tab_view);
  if (!selected) return;

  for (GList *l = window->tabs; l != NULL; l = l->next) {
    EphyTab *tab = (EphyTab *)l->data;
    if (tab->adw_page == selected) {
      window->active_tab = tab;
      ephy_tab_show (tab);
      ephy_window_update_tab_state (window, tab);
    } else {
      ephy_tab_hide (tab);
    }
  }
}

static gboolean on_tab_close_page (AdwTabView *tab_view, AdwTabPage *page, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  EphyTab *target_tab = NULL;

  for (GList *l = window->tabs; l != NULL; l = l->next) {
    EphyTab *tab = (EphyTab *)l->data;
    if (tab->adw_page == page) {
      target_tab = tab;
      window->tabs = g_list_delete_link (window->tabs, l);
      break;
    }
  }

  if (target_tab) {
    if (window->active_tab == target_tab)
      window->active_tab = NULL;
    ephy_tab_destroy (target_tab);
  }

  /* If all tabs closed, open a new blank tab */
  if (!window->tabs) {
    EphySettings *s = ephy_settings_get_default ();
    GdkSurface *surf = gtk_native_get_surface (GTK_NATIVE (window->app_window));
    HWND top_hwnd = gdk_win32_surface_get_handle (surf);
    EphyTab *new_tab = ephy_tab_new (window, top_hwnd, s->homepage, window->is_private);
    window->tabs = g_list_append (window->tabs, new_tab);
    window->active_tab = new_tab;
    adw_tab_view_set_selected_page (window->tab_view, new_tab->adw_page);
  }

  return GDK_EVENT_PROPAGATE;
}

static void on_location_activate (GtkEntry *entry, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  if (!window->active_tab) return;

  const char *text = gtk_editable_get_text (GTK_EDITABLE (entry));
  if (!text || !*text) return;

  char *url = NULL;
  if (g_strstr_len (text, -1, "://")) {
    url = g_strdup (text);
  } else if (g_strrstr (text, ".") && !strchr (text, ' ')) {
    url = g_strconcat ("https://", text, NULL);
  } else {
    EphySettings *s = ephy_settings_get_default ();
    url = ephy_settings_build_search_url (s, text);
  }

  ephy_tab_load_url (window->active_tab, url);
  g_free (url);
}

static void on_back_clicked (GtkButton *btn, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  if (window->active_tab) ephy_tab_go_back (window->active_tab);
}

static void on_forward_clicked (GtkButton *btn, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  if (window->active_tab) ephy_tab_go_forward (window->active_tab);
}

static void on_reload_clicked (GtkButton *btn, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  if (!window->active_tab) return;
  if (window->active_tab->is_loading)
    ephy_tab_stop (window->active_tab);
  else
    ephy_tab_reload (window->active_tab);
}

static void on_home_clicked (GtkButton *btn, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  if (!window->active_tab) return;
  EphySettings *s = ephy_settings_get_default ();
  ephy_tab_load_url (window->active_tab, s->homepage);
}

static void on_security_clicked (GtkButton *btn, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  ephy_security_show_popover (GTK_WIDGET (btn), window->active_tab);
}

static void on_bookmark_clicked (GtkButton *btn, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  if (!window->active_tab || !window->active_tab->url) return;

  const char *url = window->active_tab->url;
  if (ephy_bookmarks_is_bookmarked (url)) {
    ephy_bookmarks_remove (url);
    gtk_button_set_icon_name (GTK_BUTTON (btn), "starred-symbolic");
  } else {
    ephy_bookmarks_add (window->active_tab->title, url);
    gtk_button_set_icon_name (GTK_BUTTON (btn), "starred-symbolic");
  }
}

static void on_new_tab_clicked (GtkButton *btn, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  EphySettings *s = ephy_settings_get_default ();
  ephy_window_open_url (window, s->homepage);
}

/* Traffic Light Handlers */
static void on_traffic_close (GtkButton *btn, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  gtk_window_close (GTK_WINDOW (window->app_window));
}

static void on_traffic_min (GtkButton *btn, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  gtk_window_minimize (GTK_WINDOW (window->app_window));
}

static void on_traffic_zoom (GtkButton *btn, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  if (gtk_window_is_maximized (GTK_WINDOW (window->app_window)))
    gtk_window_unmaximize (GTK_WINDOW (window->app_window));
  else
    gtk_window_maximize (GTK_WINDOW (window->app_window));
}

/* Bookmarks Viewer Dialog */
static void show_bookmarks_dialog (EphyWindow *window) {
  GtkWidget *dialog = adw_preferences_window_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window->app_window));
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (dialog), 600, 480);

  AdwPreferencesPage *page = ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
  adw_preferences_page_set_title (page, "Bookmarks");
  adw_preferences_page_set_icon_name (page, "starred-symbolic");

  AdwPreferencesGroup *grp = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (grp, "Saved Bookmarks");

  GList *list = ephy_bookmarks_get_all ();
  for (GList *l = list; l != NULL; l = l->next) {
    EphyBookmark *bm = (EphyBookmark *)l->data;
    GtkWidget *row = adw_action_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), bm->title);
    adw_action_row_set_subtitle (ADW_ACTION_ROW (row), bm->url);
    gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (row), TRUE);
    g_signal_connect_swapped (row, "activated", G_CALLBACK (ephy_tab_load_url), window->active_tab);
    adw_preferences_group_add (grp, row);
  }

  adw_preferences_page_add (page, grp);
  adw_preferences_window_add (ADW_PREFERENCES_WINDOW (dialog), page);
  gtk_window_present (GTK_WINDOW (dialog));
}

/* History Viewer Dialog */
static void show_history_dialog (EphyWindow *window) {
  GtkWidget *dialog = adw_preferences_window_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window->app_window));
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (dialog), 640, 520);

  AdwPreferencesPage *page = ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
  adw_preferences_page_set_title (page, "Browsing History");
  adw_preferences_page_set_icon_name (page, "document-open-recent-symbolic");

  AdwPreferencesGroup *grp = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (grp, "Recent Visits");

  GList *list = ephy_history_get_all ();
  int count = 0;
  for (GList *l = list; l != NULL && count < 100; l = l->next, count++) {
    EphyHistoryEntry *item = (EphyHistoryEntry *)l->data;
    GtkWidget *row = adw_action_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), item->title);
    adw_action_row_set_subtitle (ADW_ACTION_ROW (row), item->url);
    gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (row), TRUE);
    adw_preferences_group_add (grp, row);
  }

  adw_preferences_page_add (page, grp);
  adw_preferences_window_add (ADW_PREFERENCES_WINDOW (dialog), page);
  gtk_window_present (GTK_WINDOW (dialog));
}

/* About Dialog */
static void show_about_dialog (EphyWindow *window) {
  AdwAboutDialog *about = ADW_ABOUT_DIALOG (adw_about_dialog_new ());
  adw_about_dialog_set_application_name (about, "GNOME Web");
  adw_about_dialog_set_developer_name (about, "The GNOME Project & Community");
  adw_about_dialog_set_version (about, "49.1 (Windows Native Port)");
  adw_about_dialog_set_comments (about, "Native Windows port of Epiphany (GNOME Web) powered by Apple WebKit WinCairo with macOS 26 Safari community theme support.");
  adw_about_dialog_set_website (about, "https://apps.gnome.org/Epiphany/");
  adw_about_dialog_set_issue_url (about, "https://github.com/jabirking-ux/GNOME-Win/issues");
  adw_about_dialog_set_license_type (about, GTK_LICENSE_GPL_3_0);
  adw_dialog_present (ADW_DIALOG (about), GTK_WIDGET (window->app_window));
}

/* Menu Actions */
static void on_action_new_tab (GSimpleAction *action, GVariant *param, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  on_new_tab_clicked (NULL, window);
}

static void on_action_new_window (GSimpleAction *action, GVariant *param, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  GtkApplication *app = gtk_window_get_application (GTK_WINDOW (window->app_window));
  ephy_window_new (app, FALSE);
}

static void on_action_new_private (GSimpleAction *action, GVariant *param, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  GtkApplication *app = gtk_window_get_application (GTK_WINDOW (window->app_window));
  ephy_window_new (app, TRUE);
}

static void on_action_close_tab (GSimpleAction *action, GVariant *param, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  if (window->active_tab && window->active_tab->adw_page)
    adw_tab_view_close_page (window->tab_view, window->active_tab->adw_page);
}

static void on_action_bookmarks (GSimpleAction *action, GVariant *param, gpointer user_data) {
  show_bookmarks_dialog ((EphyWindow *)user_data);
}

static void on_action_history (GSimpleAction *action, GVariant *param, gpointer user_data) {
  show_history_dialog ((EphyWindow *)user_data);
}

static void on_action_preferences (GSimpleAction *action, GVariant *param, gpointer user_data) {
  ephy_preferences_show ((EphyWindow *)user_data);
}

static void on_action_about (GSimpleAction *action, GVariant *param, gpointer user_data) {
  show_about_dialog ((EphyWindow *)user_data);
}

static void on_action_zoom_in (GSimpleAction *action, GVariant *param, gpointer user_data) {
  EphyWindow *w = (EphyWindow *)user_data;
  if (w->active_tab) ephy_tab_zoom_in (w->active_tab);
}

static void on_action_zoom_out (GSimpleAction *action, GVariant *param, gpointer user_data) {
  EphyWindow *w = (EphyWindow *)user_data;
  if (w->active_tab) ephy_tab_zoom_out (w->active_tab);
}

static void on_action_zoom_reset (GSimpleAction *action, GVariant *param, gpointer user_data) {
  EphyWindow *w = (EphyWindow *)user_data;
  if (w->active_tab) ephy_tab_zoom_reset (w->active_tab);
}

static void on_action_toggle_safari (GSimpleAction *action, GVariant *param, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  EphySettings *s = ephy_settings_get_default ();
  gboolean new_val = !s->safari_theme_enabled;
  ephy_settings_set_safari_theme (s, new_val);
  ephy_theme_apply_safari (new_val);
  ephy_window_sync_safari_theme (window, new_val);
}

void ephy_window_sync_safari_theme (EphyWindow *window, gboolean safari_enabled) {
  if (window->traffic_box)
    gtk_widget_set_visible (window->traffic_box, safari_enabled);

  if (safari_enabled) {
    gtk_widget_add_css_class (GTK_WIDGET (window->header_bar), "safari-header");
    gtk_widget_add_css_class (window->location_entry, "safari-location-entry");
    gtk_widget_add_css_class (GTK_WIDGET (window->tab_bar), "safari-tab-bar");
  } else {
    gtk_widget_remove_css_class (GTK_WIDGET (window->header_bar), "safari-header");
    gtk_widget_remove_css_class (window->location_entry, "safari-location-entry");
    gtk_widget_remove_css_class (GTK_WIDGET (window->tab_bar), "safari-tab-bar");
  }
}

EphyWindow *ephy_window_new (GtkApplication *app, gboolean is_private) {
  g_print ("win.a: allocating EphyWindow\n");
  EphyWindow *w = g_new0 (EphyWindow, 1);
  w->is_private = is_private;

  g_print ("win.b: creating adw_application_window_new\n");
  w->app_window = ADW_APPLICATION_WINDOW (adw_application_window_new (app));

  RECT work;
  int def_w = 1100;
  int def_h = 720;
  if (SystemParametersInfo (SPI_GETWORKAREA, 0, &work, 0)) {
    int avail_w = work.right - work.left;
    int avail_h = work.bottom - work.top;
    if (def_w > avail_w - 60) def_w = avail_w - 60;
    if (def_h > avail_h - 80) def_h = avail_h - 80;
    if (def_w < 640) def_w = avail_w;
    if (def_h < 480) def_h = avail_h;
  }
  gtk_window_set_default_size (GTK_WINDOW (w->app_window), def_w, def_h);
  gtk_window_set_title (GTK_WINDOW (w->app_window), is_private ? "GNOME Web (Incognito)" : "GNOME Web");

  g_print ("win.c: setting up actions\n");
  /* Setup Actions */
  GSimpleActionGroup *actions = g_simple_action_group_new ();
  const GActionEntry entries[] = {
    { "new-tab", on_action_new_tab, NULL, NULL, NULL },
    { "new-window", on_action_new_window, NULL, NULL, NULL },
    { "new-private-window", on_action_new_private, NULL, NULL, NULL },
    { "close-tab", on_action_close_tab, NULL, NULL, NULL },
    { "bookmarks", on_action_bookmarks, NULL, NULL, NULL },
    { "history", on_action_history, NULL, NULL, NULL },
    { "preferences", on_action_preferences, NULL, NULL, NULL },
    { "about", on_action_about, NULL, NULL, NULL },
    { "zoom-in", on_action_zoom_in, NULL, NULL, NULL },
    { "zoom-out", on_action_zoom_out, NULL, NULL, NULL },
    { "zoom-reset", on_action_zoom_reset, NULL, NULL, NULL },
    { "toggle-safari", on_action_toggle_safari, NULL, NULL, NULL }
  };
  g_action_map_add_action_entries (G_ACTION_MAP (actions), entries, G_N_ELEMENTS (entries), w);
  gtk_widget_insert_action_group (GTK_WIDGET (w->app_window), "win", G_ACTION_GROUP (actions));

  g_print ("win.d: toolbar and header bar\n");
  w->toolbar_view = ADW_TOOLBAR_VIEW (adw_toolbar_view_new ());
  w->header_bar = ADW_HEADER_BAR (adw_header_bar_new ());
  adw_toolbar_view_add_top_bar (w->toolbar_view, GTK_WIDGET (w->header_bar));

  g_print ("win.e: traffic lights\n");
  /* Traffic lights */
  w->traffic_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_add_css_class (w->traffic_box, "traffic-lights");

  GtkWidget *btn_close = gtk_button_new ();
  gtk_widget_add_css_class (btn_close, "traffic-btn");
  gtk_widget_add_css_class (btn_close, "close");
  g_signal_connect (btn_close, "clicked", G_CALLBACK (on_traffic_close), w);
  gtk_box_append (GTK_BOX (w->traffic_box), btn_close);

  GtkWidget *btn_min = gtk_button_new ();
  gtk_widget_add_css_class (btn_min, "traffic-btn");
  gtk_widget_add_css_class (btn_min, "minimize");
  g_signal_connect (btn_min, "clicked", G_CALLBACK (on_traffic_min), w);
  gtk_box_append (GTK_BOX (w->traffic_box), btn_min);

  GtkWidget *btn_zoom = gtk_button_new ();
  gtk_widget_add_css_class (btn_zoom, "traffic-btn");
  gtk_widget_add_css_class (btn_zoom, "zoom");
  g_signal_connect (btn_zoom, "clicked", G_CALLBACK (on_traffic_zoom), w);
  gtk_box_append (GTK_BOX (w->traffic_box), btn_zoom);

  adw_header_bar_pack_start (w->header_bar, w->traffic_box);

  g_print ("win.f: navigation buttons\n");
  /* Navigation Buttons */
  GtkWidget *nav_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  w->back_btn = gtk_button_new_from_icon_name ("go-previous-symbolic");
  gtk_widget_set_tooltip_text (w->back_btn, "Back (Alt+Left)");
  gtk_widget_set_sensitive (w->back_btn, FALSE);
  g_signal_connect (w->back_btn, "clicked", G_CALLBACK (on_back_clicked), w);
  gtk_box_append (GTK_BOX (nav_box), w->back_btn);

  w->forward_btn = gtk_button_new_from_icon_name ("go-next-symbolic");
  gtk_widget_set_tooltip_text (w->forward_btn, "Forward (Alt+Right)");
  gtk_widget_set_sensitive (w->forward_btn, FALSE);
  g_signal_connect (w->forward_btn, "clicked", G_CALLBACK (on_forward_clicked), w);
  gtk_box_append (GTK_BOX (nav_box), w->forward_btn);

  w->reload_btn = gtk_button_new_from_icon_name ("view-refresh-symbolic");
  gtk_widget_set_tooltip_text (w->reload_btn, "Reload (Ctrl+R / F5)");
  g_signal_connect (w->reload_btn, "clicked", G_CALLBACK (on_reload_clicked), w);
  gtk_box_append (GTK_BOX (nav_box), w->reload_btn);

  w->home_btn = gtk_button_new_from_icon_name ("user-home-symbolic");
  gtk_widget_set_tooltip_text (w->home_btn, "Homepage");
  g_signal_connect (w->home_btn, "clicked", G_CALLBACK (on_home_clicked), w);
  gtk_box_append (GTK_BOX (nav_box), w->home_btn);

  adw_header_bar_pack_start (w->header_bar, nav_box);

  g_print ("win.g: location box\n");
  /* Location Entry Box */
  w->location_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_hexpand (w->location_box, TRUE);
  gtk_widget_add_css_class (w->location_box, "safari-location-box");

  GtkWidget *entry_row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  w->location_entry = gtk_entry_new ();
  gtk_widget_set_hexpand (w->location_entry, TRUE);
  gtk_entry_set_placeholder_text (GTK_ENTRY (w->location_entry), "Search or enter address");
  g_signal_connect (w->location_entry, "activate", G_CALLBACK (on_location_activate), w);

  /* Security indicator icon */
  w->security_btn = gtk_button_new_from_icon_name (is_private ? "channel-secure-symbolic" : "security-high-symbolic");
  gtk_button_set_has_frame (GTK_BUTTON (w->security_btn), FALSE);
  gtk_widget_set_tooltip_text (w->security_btn, "View site security");
  g_signal_connect (w->security_btn, "clicked", G_CALLBACK (on_security_clicked), w);
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (w->location_entry), GTK_ENTRY_ICON_PRIMARY, is_private ? "channel-secure-symbolic" : "security-high-symbolic");

  /* Bookmark star button */
  w->bookmark_btn = gtk_button_new_from_icon_name ("starred-symbolic");
  gtk_button_set_has_frame (GTK_BUTTON (w->bookmark_btn), FALSE);
  gtk_widget_set_tooltip_text (w->bookmark_btn, "Bookmark this page");
  g_signal_connect (w->bookmark_btn, "clicked", G_CALLBACK (on_bookmark_clicked), w);
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (w->location_entry), GTK_ENTRY_ICON_SECONDARY, "starred-symbolic");

  gtk_box_append (GTK_BOX (entry_row), w->location_entry);
  gtk_box_append (GTK_BOX (w->location_box), entry_row);

  /* Progress bar */
  w->progress_bar = gtk_progress_bar_new ();
  gtk_widget_set_visible (w->progress_bar, FALSE);
  gtk_widget_add_css_class (w->progress_bar, "osd");
  gtk_box_append (GTK_BOX (w->location_box), w->progress_bar);

  adw_header_bar_set_title_widget (w->header_bar, w->location_box);

  g_print ("win.h: new tab and menu buttons\n");
  /* End Controls: New Tab, Menu */
  w->new_tab_btn = gtk_button_new_from_icon_name ("tab-new-symbolic");
  gtk_widget_set_tooltip_text (w->new_tab_btn, "New Tab (Ctrl+T)");
  g_signal_connect (w->new_tab_btn, "clicked", G_CALLBACK (on_new_tab_clicked), w);
  adw_header_bar_pack_end (w->header_bar, w->new_tab_btn);

  /* Main Menu */
  GMenu *menu = g_menu_new ();
  GMenu *section1 = g_menu_new ();
  g_menu_append (section1, "New Tab", "win.new-tab");
  g_menu_append (section1, "New Window", "win.new-window");
  g_menu_append (section1, "New Incognito Window", "win.new-private-window");
  g_menu_append_section (menu, NULL, G_MENU_MODEL (section1));

  GMenu *section_theme = g_menu_new ();
  g_menu_append (section_theme, "Toggle Safari Community Theme", "win.toggle-safari");
  g_menu_append_section (menu, NULL, G_MENU_MODEL (section_theme));

  GMenu *section2 = g_menu_new ();
  g_menu_append (section2, "Bookmarks", "win.bookmarks");
  g_menu_append (section2, "History", "win.history");
  g_menu_append_section (menu, NULL, G_MENU_MODEL (section2));

  GMenu *section3 = g_menu_new ();
  g_menu_append (section3, "Zoom In", "win.zoom-in");
  g_menu_append (section3, "Zoom Out", "win.zoom-out");
  g_menu_append (section3, "Reset Zoom", "win.zoom-reset");
  g_menu_append_section (menu, NULL, G_MENU_MODEL (section3));

  GMenu *section4 = g_menu_new ();
  g_menu_append (section4, "Preferences", "win.preferences");
  g_menu_append (section4, "About GNOME Web", "win.about");
  g_menu_append_section (menu, NULL, G_MENU_MODEL (section4));

  w->menu_btn = gtk_menu_button_new ();
  gtk_menu_button_set_icon_name (GTK_MENU_BUTTON (w->menu_btn), "open-menu-symbolic");
  gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (w->menu_btn), G_MENU_MODEL (menu));
  adw_header_bar_pack_end (w->header_bar, w->menu_btn);

  g_print ("win.i: tab view and tab bar\n");
  /* Tab View & Tab Bar */
  w->tab_view = ADW_TAB_VIEW (adw_tab_view_new ());
  w->tab_bar = ADW_TAB_BAR (adw_tab_bar_new ());
  adw_tab_bar_set_view (w->tab_bar, w->tab_view);
  adw_toolbar_view_add_top_bar (w->toolbar_view, GTK_WIDGET (w->tab_bar));

  adw_toolbar_view_set_content (w->toolbar_view, GTK_WIDGET (w->tab_view));
  adw_application_window_set_content (w->app_window, GTK_WIDGET (w->toolbar_view));

  g_signal_connect (w->tab_view, "notify::selected-page", G_CALLBACK (on_tab_selected), w);
  g_signal_connect (w->tab_view, "close-page", G_CALLBACK (on_tab_close_page), w);

  g_print ("win.j: applying theme\n");
  /* Apply theme */
  EphySettings *s = ephy_settings_get_default ();
  ephy_window_sync_safari_theme (w, s->safari_theme_enabled);

  g_print ("win.k: realizing window\n");
  gtk_widget_realize (GTK_WIDGET (w->app_window));
  g_print ("win.l: presenting window\n");
  gtk_window_present (GTK_WINDOW (w->app_window));

  g_print ("win.m: opening initial tab\n");
  /* Open initial tab */
  GdkSurface *surf = gtk_native_get_surface (GTK_NATIVE (w->app_window));
  g_assert (surf != NULL);
  HWND top_hwnd = gdk_win32_surface_get_handle (surf);
  g_assert (top_hwnd != NULL);
  EphyTab *initial_tab = ephy_tab_new (w, top_hwnd, s->homepage, is_private);
  w->tabs = g_list_append (w->tabs, initial_tab);
  w->active_tab = initial_tab;
  adw_tab_view_set_selected_page (w->tab_view, initial_tab->adw_page);

  g_print ("win.n: timer add\n");
  /* Sync timer */
  g_timeout_add (40, on_geometry_timer, w);

  g_print ("win.done!\n");
  return w;
}

void ephy_window_open_url (EphyWindow *window, const char *url) {
  GdkSurface *surf = gtk_native_get_surface (GTK_NATIVE (window->app_window));
  HWND top_hwnd = gdk_win32_surface_get_handle (surf);
  EphyTab *tab = ephy_tab_new (window, top_hwnd, url, window->is_private);
  window->tabs = g_list_append (window->tabs, tab);
  window->active_tab = tab;
  adw_tab_view_set_selected_page (window->tab_view, tab->adw_page);
}

void ephy_window_update_tab_state (EphyWindow *window, EphyTab *tab) {
  if (window->active_tab != tab) return;

  if (tab->url && !gtk_widget_has_focus (window->location_entry))
    gtk_editable_set_text (GTK_EDITABLE (window->location_entry), tab->url);

  gtk_widget_set_sensitive (window->back_btn, tab->can_go_back);
  gtk_widget_set_sensitive (window->forward_btn, tab->can_go_forward);

  if (tab->is_loading) {
    gtk_button_set_icon_name (GTK_BUTTON (window->reload_btn), "process-stop-symbolic");
    gtk_widget_set_visible (window->progress_bar, TRUE);
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (window->progress_bar), tab->progress);
  } else {
    gtk_button_set_icon_name (GTK_BUTTON (window->reload_btn), "view-refresh-symbolic");
    gtk_widget_set_visible (window->progress_bar, FALSE);
  }

  /* Update title */
  if (tab->title && *tab->title) {
    g_autofree char *full_title = g_strdup_printf ("%s — %s", tab->title, window->is_private ? "GNOME Web (Incognito)" : "GNOME Web");
    gtk_window_set_title (GTK_WINDOW (window->app_window), full_title);
  }
}
