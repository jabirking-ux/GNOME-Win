#pragma once

#include <gtk/gtk.h>
#include <adwaita.h>
#include <WebKit/WebKit2_C.h>
#include <WebKit/WKView.h>
#include <windows.h>

typedef struct _EphyWindow EphyWindow;

typedef struct {
  EphyWindow *window;
  GtkWidget *holder;
  AdwTabPage *adw_page;
  WKViewRef wk_view;
  WKPageRef wk_page;

  gboolean is_private;
  gboolean is_loading;
  char *title;
  char *url;
  double progress;
  double zoom_level;

  gboolean can_go_back;
  gboolean can_go_forward;

  int last_x;
  int last_y;
  int last_w;
  int last_h;
} EphyTab;

EphyTab *ephy_tab_new (EphyWindow *window, HWND top_hwnd, const char *initial_url, gboolean is_private);
void ephy_tab_load_url (EphyTab *tab, const char *url);
void ephy_tab_load_html (EphyTab *tab, const char *html, const char *base_url);
void ephy_tab_go_back (EphyTab *tab);
void ephy_tab_go_forward (EphyTab *tab);
void ephy_tab_reload (EphyTab *tab);
void ephy_tab_stop (EphyTab *tab);
void ephy_tab_zoom_in (EphyTab *tab);
void ephy_tab_zoom_out (EphyTab *tab);
void ephy_tab_zoom_reset (EphyTab *tab);
void ephy_tab_find_string (EphyTab *tab, const char *str, gboolean backwards, gboolean case_sensitive);
void ephy_tab_hide_find_ui (EphyTab *tab);
void ephy_tab_view_source (EphyTab *tab);
void ephy_tab_sync_geometry (EphyTab *tab);
void ephy_tab_poll_state (EphyTab *tab);
void ephy_tab_show (EphyTab *tab);
void ephy_tab_hide (EphyTab *tab);
void ephy_tab_destroy (EphyTab *tab);
