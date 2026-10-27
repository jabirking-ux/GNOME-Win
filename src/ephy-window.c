#include "ephy-window.h"
#include "ephy-settings.h"
#include "ephy-theme.h"
#include "ephy-bookmarks.h"
#include "ephy-history.h"
#include "ephy-downloads.h"
#include "ephy-session.h"
#include "ephy-preferences.h"
#include "ephy-security.h"
#include "ephy-shortcuts-dialog.h"
#include <gdk/win32/gdkwin32.h>
#include <windows.h>
#include <shellapi.h>

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
    if (!window->is_private && target_tab->url)
      ephy_session_push_closed_tab (target_tab->title, target_tab->url);

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

  ephy_session_save (window);
  return GDK_EVENT_PROPAGATE;
}

static void on_location_changed (GtkEditable *editable, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  if (!gtk_widget_has_focus (GTK_WIDGET (editable))) return;
  const char *text = gtk_editable_get_text (editable);
  if (window->suggestions)
    ephy_suggestions_update (window->suggestions, text);
}

static void on_location_activate (GtkEntry *entry, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  if (window->suggestions)
    ephy_suggestions_hide (window->suggestions);

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

static void on_entry_icon_press (GtkEntry *entry, GtkEntryIconPosition icon_pos, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  if (!window->active_tab) return;

  if (icon_pos == GTK_ENTRY_ICON_PRIMARY) {
    ephy_security_show_popover (GTK_WIDGET (entry), window->active_tab);
  } else if (icon_pos == GTK_ENTRY_ICON_SECONDARY) {
    const char *url = window->active_tab->url;
    if (!url) return;
    if (ephy_bookmarks_is_bookmarked (url)) {
      ephy_bookmarks_remove (url);
      gtk_entry_set_icon_tooltip_text (entry, GTK_ENTRY_ICON_SECONDARY, "Bookmark this page");
    } else {
      ephy_bookmarks_add (window->active_tab->title, url);
      gtk_entry_set_icon_tooltip_text (entry, GTK_ENTRY_ICON_SECONDARY, "Page is bookmarked");
    }
  }
}

static void on_new_tab_clicked (GtkButton *btn, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  g_autofree char *start_url = ephy_settings_get_startpage_url ();
  ephy_window_open_url (window, start_url);
}

static void on_downloads_btn_clicked (GtkButton *btn, gpointer user_data) {
  ephy_downloads_show_popover (GTK_WIDGET (btn));
}

/* About Dialog */
static void show_about_dialog (EphyWindow *window) {
  AdwAboutDialog *about = ADW_ABOUT_DIALOG (adw_about_dialog_new ());
  adw_about_dialog_set_application_name (about, "Web");
  adw_about_dialog_set_developer_name (about, "The GNOME Project");
  adw_about_dialog_set_version (about, "49.4 (Windows Native Port)");
  adw_about_dialog_set_comments (about, "The web browser for GNOME, natively ported to Windows powered by Apple WebKit WinCairo.");
  adw_about_dialog_set_website (about, "https://apps.gnome.org/Epiphany/");
  adw_about_dialog_set_issue_url (about, "https://github.com/jabirking-ux/GNOME-Win/issues");
  adw_about_dialog_set_license_type (about, GTK_LICENSE_GPL_3_0);
  adw_dialog_present (ADW_DIALOG (about), GTK_WIDGET (window->app_window));
}

static void on_action_focus_location (GSimpleAction *action, GVariant *param, gpointer user_data) {
  EphyWindow *w = (EphyWindow *)user_data;
  gtk_widget_grab_focus (w->location_entry);
  gtk_editable_select_region (GTK_EDITABLE (w->location_entry), 0, -1);
}

static void on_action_reload (GSimpleAction *action, GVariant *param, gpointer user_data) {
  EphyWindow *w = (EphyWindow *)user_data;
  if (w->active_tab) ephy_tab_reload (w->active_tab);
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

static void on_action_reopen_closed_tab (GSimpleAction *action, GVariant *param, gpointer user_data) {
  EphyWindow *window = (EphyWindow *)user_data;
  char *title = NULL;
  char *url = ephy_session_pop_closed_tab (&title);
  if (url) {
    ephy_window_open_url (window, url);
    g_free (url);
    g_free (title);
  }
}

static void on_action_find (GSimpleAction *action, GVariant *param, gpointer user_data) {
  EphyWindow *w = (EphyWindow *)user_data;
  if (w->find_bar) ephy_find_bar_show (w->find_bar);
}

static void on_action_find_next (GSimpleAction *action, GVariant *param, gpointer user_data) {
  EphyWindow *w = (EphyWindow *)user_data;
  if (w->find_bar) ephy_find_bar_find_next (w->find_bar);
}

static void on_action_find_prev (GSimpleAction *action, GVariant *param, gpointer user_data) {
  EphyWindow *w = (EphyWindow *)user_data;
  if (w->find_bar) ephy_find_bar_find_prev (w->find_bar);
}

static void on_action_bookmarks (GSimpleAction *action, GVariant *param, gpointer user_data) {
  EphyWindow *w = (EphyWindow *)user_data;
  ephy_bookmarks_dialog_show (GTK_WINDOW (w->app_window), w);
}

static void on_action_history (GSimpleAction *action, GVariant *param, gpointer user_data) {
  EphyWindow *w = (EphyWindow *)user_data;
  ephy_history_dialog_show (GTK_WINDOW (w->app_window), w);
}

static void on_action_downloads (GSimpleAction *action, GVariant *param, gpointer user_data) {
  EphyWindow *w = (EphyWindow *)user_data;
  ephy_downloads_show_popover (w->downloads_btn);
}

static void on_action_view_source (GSimpleAction *action, GVariant *param, gpointer user_data) {
  EphyWindow *w = (EphyWindow *)user_data;
  if (w->active_tab) ephy_tab_view_source (w->active_tab);
}

static void on_action_shortcuts (GSimpleAction *action, GVariant *param, gpointer user_data) {
  EphyWindow *w = (EphyWindow *)user_data;
  ephy_shortcuts_dialog_show (GTK_WINDOW (w->app_window));
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

static void on_action_fullscreen (GSimpleAction *action, GVariant *param, gpointer user_data) {
  EphyWindow *w = (EphyWindow *)user_data;
  if (gtk_window_is_fullscreen (GTK_WINDOW (w->app_window)))
    gtk_window_unfullscreen (GTK_WINDOW (w->app_window));
  else
    gtk_window_fullscreen (GTK_WINDOW (w->app_window));
}

static gboolean on_window_close_request (GtkWindow *window, gpointer user_data) {
  EphyWindow *w = (EphyWindow *)user_data;
  ephy_session_save (w);
  return FALSE; /* Allow normal window destruction */
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
  gtk_window_set_title (GTK_WINDOW (w->app_window), is_private ? "Web (Incognito)" : "Web");
  g_signal_connect (w->app_window, "close-request", G_CALLBACK (on_window_close_request), w);

  g_print ("win.c: setting up actions\n");
  /* Setup Actions */
  GSimpleActionGroup *actions = g_simple_action_group_new ();
  const GActionEntry entries[] = {
    { "new-tab", on_action_new_tab, NULL, NULL, NULL },
    { "new-window", on_action_new_window, NULL, NULL, NULL },
    { "new-private-window", on_action_new_private, NULL, NULL, NULL },
    { "close-tab", on_action_close_tab, NULL, NULL, NULL },
    { "reopen-closed-tab", on_action_reopen_closed_tab, NULL, NULL, NULL },
    { "focus-location", on_action_focus_location, NULL, NULL, NULL },
    { "find", on_action_find, NULL, NULL, NULL },
    { "find-next", on_action_find_next, NULL, NULL, NULL },
    { "find-prev", on_action_find_prev, NULL, NULL, NULL },
    { "reload", on_action_reload, NULL, NULL, NULL },
    { "bookmarks", on_action_bookmarks, NULL, NULL, NULL },
    { "history", on_action_history, NULL, NULL, NULL },
    { "downloads", on_action_downloads, NULL, NULL, NULL },
    { "view-source", on_action_view_source, NULL, NULL, NULL },
    { "shortcuts", on_action_shortcuts, NULL, NULL, NULL },
    { "preferences", on_action_preferences, NULL, NULL, NULL },
    { "about", on_action_about, NULL, NULL, NULL },
    { "zoom-in", on_action_zoom_in, NULL, NULL, NULL },
    { "zoom-out", on_action_zoom_out, NULL, NULL, NULL },
    { "zoom-reset", on_action_zoom_reset, NULL, NULL, NULL },
    { "fullscreen", on_action_fullscreen, NULL, NULL, NULL }
  };
  g_action_map_add_action_entries (G_ACTION_MAP (actions), entries, G_N_ELEMENTS (entries), w);
  gtk_widget_insert_action_group (GTK_WIDGET (w->app_window), "win", G_ACTION_GROUP (actions));

  g_print ("win.d: toolbar and header bar\n");
  w->toolbar_view = ADW_TOOLBAR_VIEW (adw_toolbar_view_new ());
  w->header_bar = ADW_HEADER_BAR (adw_header_bar_new ());
  adw_toolbar_view_add_top_bar (w->toolbar_view, GTK_WIDGET (w->header_bar));

  /* --- START ACTIONS: Upstream Epiphany Style --- */
  w->new_tab_btn = gtk_button_new_from_icon_name ("tab-new-symbolic");
  gtk_widget_set_tooltip_text (w->new_tab_btn, "New Tab (Ctrl+T)");
  g_signal_connect (w->new_tab_btn, "clicked", G_CALLBACK (on_new_tab_clicked), w);
  adw_header_bar_pack_start (w->header_bar, w->new_tab_btn);

  /* Navigation Cluster */
  w->nav_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_add_css_class (w->nav_box, "linked");
  gtk_widget_add_css_class (w->nav_box, "navigation-box");

  w->back_btn = gtk_button_new_from_icon_name ("go-previous-symbolic");
  gtk_widget_set_tooltip_text (w->back_btn, "Back (Alt+Left)");
  gtk_widget_set_sensitive (w->back_btn, FALSE);
  g_signal_connect (w->back_btn, "clicked", G_CALLBACK (on_back_clicked), w);
  gtk_box_append (GTK_BOX (w->nav_box), w->back_btn);

  w->forward_btn = gtk_button_new_from_icon_name ("go-next-symbolic");
  gtk_widget_set_tooltip_text (w->forward_btn, "Forward (Alt+Right)");
  gtk_widget_set_sensitive (w->forward_btn, FALSE);
  g_signal_connect (w->forward_btn, "clicked", G_CALLBACK (on_forward_clicked), w);
  gtk_box_append (GTK_BOX (w->nav_box), w->forward_btn);

  w->reload_btn = gtk_button_new_from_icon_name ("view-refresh-symbolic");
  gtk_widget_set_tooltip_text (w->reload_btn, "Reload (Ctrl+R / F5)");
  g_signal_connect (w->reload_btn, "clicked", G_CALLBACK (on_reload_clicked), w);
  gtk_box_append (GTK_BOX (w->nav_box), w->reload_btn);

  w->home_btn = gtk_button_new_from_icon_name ("go-home-symbolic");
  gtk_widget_set_tooltip_text (w->home_btn, "Homepage (Alt+Home)");
  g_signal_connect (w->home_btn, "clicked", G_CALLBACK (on_home_clicked), w);
  gtk_box_append (GTK_BOX (w->nav_box), w->home_btn);

  adw_header_bar_pack_start (w->header_bar, w->nav_box);

  /* --- CENTER TITLE: AdwClamp Location Entry --- */
  w->clamp = adw_clamp_new ();
  gtk_widget_set_hexpand (w->clamp, TRUE);
  adw_clamp_set_maximum_size (ADW_CLAMP (w->clamp), 860);
  adw_clamp_set_tightening_threshold (ADW_CLAMP (w->clamp), 560);

  w->location_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_hexpand (w->location_box, TRUE);
  gtk_widget_add_css_class (w->location_box, "ephy-location-box");

  w->location_entry = gtk_entry_new ();
  gtk_widget_set_hexpand (w->location_entry, TRUE);
  gtk_widget_add_css_class (w->location_entry, "ephy-location-entry");
  gtk_entry_set_placeholder_text (GTK_ENTRY (w->location_entry), "Search the web or enter an address");
  g_signal_connect (w->location_entry, "activate", G_CALLBACK (on_location_activate), w);
  g_signal_connect (w->location_entry, "changed", G_CALLBACK (on_location_changed), w);

  /* Security indicator icon */
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (w->location_entry), GTK_ENTRY_ICON_PRIMARY, is_private ? "channel-secure-symbolic" : "security-high-symbolic");
  gtk_entry_set_icon_activatable (GTK_ENTRY (w->location_entry), GTK_ENTRY_ICON_PRIMARY, TRUE);
  gtk_entry_set_icon_tooltip_text (GTK_ENTRY (w->location_entry), GTK_ENTRY_ICON_PRIMARY, "View site security information");

  /* Bookmark star icon */
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (w->location_entry), GTK_ENTRY_ICON_SECONDARY, "starred-symbolic");
  gtk_entry_set_icon_activatable (GTK_ENTRY (w->location_entry), GTK_ENTRY_ICON_SECONDARY, TRUE);
  gtk_entry_set_icon_tooltip_text (GTK_ENTRY (w->location_entry), GTK_ENTRY_ICON_SECONDARY, "Bookmark this page");

  g_signal_connect (w->location_entry, "icon-press", G_CALLBACK (on_entry_icon_press), w);

  gtk_box_append (GTK_BOX (w->location_box), w->location_entry);

  /* Suggestions Popover */
  w->suggestions = ephy_suggestions_new (w);

  /* Progress bar */
  w->progress_bar = gtk_progress_bar_new ();
  gtk_widget_set_visible (w->progress_bar, FALSE);
  gtk_widget_add_css_class (w->progress_bar, "ephy-progress");
  gtk_box_append (GTK_BOX (w->location_box), w->progress_bar);

  adw_clamp_set_child (ADW_CLAMP (w->clamp), w->location_box);
  adw_header_bar_set_title_widget (w->header_bar, w->clamp);

  /* --- END ACTIONS: Bookmarks, Downloads, Main Menu --- */
  w->bookmarks_nav_btn = gtk_button_new_from_icon_name ("user-bookmarks-symbolic");
  gtk_widget_set_tooltip_text (w->bookmarks_nav_btn, "Bookmarks (Ctrl+B)");
  g_signal_connect_swapped (w->bookmarks_nav_btn, "clicked", G_CALLBACK (on_action_bookmarks), w);
  adw_header_bar_pack_end (w->header_bar, w->bookmarks_nav_btn);

  w->downloads_btn = gtk_button_new_from_icon_name ("folder-download-symbolic");
  gtk_widget_set_tooltip_text (w->downloads_btn, "Downloads (Ctrl+Y)");
  g_signal_connect (w->downloads_btn, "clicked", G_CALLBACK (on_downloads_btn_clicked), w);
  adw_header_bar_pack_end (w->header_bar, w->downloads_btn);

  /* Main Menu */
  GMenu *menu = g_menu_new ();
  GMenu *section1 = g_menu_new ();
  g_menu_append (section1, "New Tab", "win.new-tab");
  g_menu_append (section1, "New Window", "win.new-window");
  g_menu_append (section1, "New Incognito Window", "win.new-private-window");
  g_menu_append (section1, "Reopen Closed Tab", "win.reopen-closed-tab");
  g_menu_append_section (menu, NULL, G_MENU_MODEL (section1));

  GMenu *section_nav = g_menu_new ();
  g_menu_append (section_nav, "Focus Location Entry", "win.focus-location");
  g_menu_append (section_nav, "Find in Page…", "win.find");
  g_menu_append (section_nav, "Fullscreen", "win.fullscreen");
  g_menu_append_section (menu, NULL, G_MENU_MODEL (section_nav));

  GMenu *section2 = g_menu_new ();
  g_menu_append (section2, "Bookmarks", "win.bookmarks");
  g_menu_append (section2, "History", "win.history");
  g_menu_append (section2, "Downloads", "win.downloads");
  g_menu_append_section (menu, NULL, G_MENU_MODEL (section2));

  GMenu *section3 = g_menu_new ();
  g_menu_append (section3, "Zoom In", "win.zoom-in");
  g_menu_append (section3, "Zoom Out", "win.zoom-out");
  g_menu_append (section3, "Reset Zoom", "win.zoom-reset");
  g_menu_append (section3, "View Page Source", "win.view-source");
  g_menu_append_section (menu, NULL, G_MENU_MODEL (section3));

  GMenu *section4 = g_menu_new ();
  g_menu_append (section4, "Preferences", "win.preferences");
  g_menu_append (section4, "Keyboard Shortcuts", "win.shortcuts");
  g_menu_append (section4, "About Web", "win.about");
  g_menu_append_section (menu, NULL, G_MENU_MODEL (section4));

  w->menu_btn = gtk_menu_button_new ();
  gtk_menu_button_set_icon_name (GTK_MENU_BUTTON (w->menu_btn), "open-menu-symbolic");
  gtk_widget_set_tooltip_text (w->menu_btn, "Main Menu");
  gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (w->menu_btn), G_MENU_MODEL (menu));
  adw_header_bar_pack_end (w->header_bar, w->menu_btn);

  /* Tab View & Tab Bar */
  w->tab_view = ADW_TAB_VIEW (adw_tab_view_new ());
  w->tab_bar = ADW_TAB_BAR (adw_tab_bar_new ());
  adw_tab_bar_set_view (w->tab_bar, w->tab_view);
  adw_tab_bar_set_autohide (w->tab_bar, TRUE);
  adw_toolbar_view_add_top_bar (w->toolbar_view, GTK_WIDGET (w->tab_bar));

  /* Find Bar */
  w->find_bar = ephy_find_bar_new (w);
  adw_toolbar_view_add_top_bar (w->toolbar_view, w->find_bar->container);

  adw_toolbar_view_set_content (w->toolbar_view, GTK_WIDGET (w->tab_view));
  adw_application_window_set_content (w->app_window, GTK_WIDGET (w->toolbar_view));

  g_signal_connect (w->tab_view, "notify::selected-page", G_CALLBACK (on_tab_selected), w);
  g_signal_connect (w->tab_view, "close-page", G_CALLBACK (on_tab_close_page), w);

  g_print ("win.k: realizing window\n");
  gtk_widget_realize (GTK_WIDGET (w->app_window));
  g_print ("win.l: presenting window\n");
  gtk_window_present (GTK_WINDOW (w->app_window));

  g_print ("win.m: opening initial tab or restoring session\n");
  EphySettings *s = ephy_settings_get_default ();
  GdkSurface *surf = gtk_native_get_surface (GTK_NATIVE (w->app_window));
  g_assert (surf != NULL);
  HWND top_hwnd = gdk_win32_surface_get_handle (surf);
  g_assert (top_hwnd != NULL);

  /* Open initial tab */
  EphyTab *initial_tab = ephy_tab_new (w, top_hwnd, s->homepage, is_private);
  w->tabs = g_list_append (w->tabs, initial_tab);
  w->active_tab = initial_tab;
  adw_tab_view_set_selected_page (w->tab_view, initial_tab->adw_page);

  if (!is_private && s->restore_session) {
    ephy_session_restore (w);
  }

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
  ephy_session_save (window);
}

void ephy_window_update_tab_state (EphyWindow *window, EphyTab *tab) {
  if (window->active_tab != tab) return;

  if (tab->url && !gtk_widget_has_focus (window->location_entry)) {
    if (g_str_has_prefix (tab->url, "file://") && g_strrstr (tab->url, "startpage")) {
      gtk_editable_set_text (GTK_EDITABLE (window->location_entry), "");
      gtk_entry_set_placeholder_text (GTK_ENTRY (window->location_entry), "Search the web or enter an address");
    } else {
      gtk_editable_set_text (GTK_EDITABLE (window->location_entry), tab->url);
    }
  }

  gtk_widget_set_sensitive (window->back_btn, tab->can_go_back);
  gtk_widget_set_sensitive (window->forward_btn, tab->can_go_forward);

  if (tab->is_loading) {
    gtk_button_set_icon_name (GTK_BUTTON (window->reload_btn), "process-stop-symbolic");
    gtk_widget_set_tooltip_text (window->reload_btn, "Stop Loading");
    gtk_widget_set_visible (window->progress_bar, TRUE);
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (window->progress_bar), tab->progress);
  } else {
    gtk_button_set_icon_name (GTK_BUTTON (window->reload_btn), "view-refresh-symbolic");
    gtk_widget_set_tooltip_text (window->reload_btn, "Reload (Ctrl+R / F5)");
    gtk_widget_set_visible (window->progress_bar, FALSE);
  }

  /* Update title */
  if (tab->title && *tab->title) {
    if (strstr (tab->title, "Web")) {
      gtk_window_set_title (GTK_WINDOW (window->app_window), tab->title);
    } else {
      g_autofree char *full_title = g_strdup_printf ("%s — %s", tab->title, window->is_private ? "Web (Incognito)" : "Web");
      gtk_window_set_title (GTK_WINDOW (window->app_window), full_title);
    }
  } else if (tab->url && g_strrstr (tab->url, "startpage")) {
    gtk_window_set_title (GTK_WINDOW (window->app_window), window->is_private ? "Start Page — Web (Incognito)" : "Start Page — Web");
  }
}
