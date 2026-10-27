#pragma once

#include <gtk/gtk.h>
#include <adwaita.h>
#include "ephy-tab.h"

struct _EphyWindow {
  AdwApplicationWindow *app_window;
  AdwToolbarView *toolbar_view;
  AdwHeaderBar *header_bar;
  AdwTabView *tab_view;
  AdwTabBar *tab_bar;

  /* Traffic lights */
  GtkWidget *traffic_box;

  /* Navigation */
  GtkWidget *back_btn;
  GtkWidget *forward_btn;
  GtkWidget *reload_btn;
  GtkWidget *home_btn;

  /* Location entry */
  GtkWidget *location_box;
  GtkWidget *location_entry;
  GtkWidget *security_btn;
  GtkWidget *bookmark_btn;
  GtkWidget *progress_bar;

  /* Buttons */
  GtkWidget *new_tab_btn;
  GtkWidget *menu_btn;

  /* Favorites Bar */
  GtkWidget *favorites_bar;

  /* State */
  gboolean is_private;
  GList *tabs;
  EphyTab *active_tab;
};

EphyWindow *ephy_window_new (GtkApplication *app, gboolean is_private);
void ephy_window_open_url (EphyWindow *window, const char *url);
void ephy_window_update_tab_state (EphyWindow *window, EphyTab *tab);
void ephy_window_sync_safari_theme (EphyWindow *window, gboolean safari_enabled);
