#pragma once

#include <gtk/gtk.h>
#include <adwaita.h>
#include "ephy-tab.h"
#include "ephy-find-bar.h"
#include "ephy-suggestions.h"

struct _EphyWindow {
  AdwApplicationWindow *app_window;
  AdwToolbarView *toolbar_view;
  AdwHeaderBar *header_bar;
  AdwTabView *tab_view;
  AdwTabBar *tab_bar;

  /* Navigation (Pack Start) */
  GtkWidget *new_tab_btn;
  GtkWidget *nav_box;
  GtkWidget *back_btn;
  GtkWidget *forward_btn;
  GtkWidget *reload_btn;
  GtkWidget *home_btn;

  /* Centered Location Entry */
  GtkWidget *clamp;
  GtkWidget *location_box;
  GtkWidget *location_entry;
  GtkWidget *security_btn;
  GtkWidget *bookmark_btn;
  GtkWidget *progress_bar;

  /* End Actions */
  GtkWidget *bookmarks_nav_btn;
  GtkWidget *downloads_btn;
  GtkWidget *menu_btn;

  /* Find Bar */
  EphyFindBar *find_bar;

  /* Suggestions Popover */
  EphySuggestions *suggestions;

  /* State */
  gboolean is_private;
  GList *tabs;
  EphyTab *active_tab;
};

EphyWindow *ephy_window_new (GtkApplication *app, gboolean is_private);
void ephy_window_open_url (EphyWindow *window, const char *url);
void ephy_window_update_tab_state (EphyWindow *window, EphyTab *tab);
