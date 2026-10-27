#pragma once

#include <gtk/gtk.h>
#include <adwaita.h>

typedef struct _EphyWindow EphyWindow;

typedef struct _EphyFindBar EphyFindBar;

struct _EphyFindBar {
  GtkWidget *container;
  GtkWidget *entry;
  GtkWidget *prev_btn;
  GtkWidget *next_btn;
  GtkWidget *status_label;
  GtkWidget *case_btn;
  GtkWidget *close_btn;
  EphyWindow *window;
  gboolean case_sensitive;
};

EphyFindBar *ephy_find_bar_new (EphyWindow *window);
void ephy_find_bar_show (EphyFindBar *bar);
void ephy_find_bar_hide (EphyFindBar *bar);
void ephy_find_bar_find_next (EphyFindBar *bar);
void ephy_find_bar_find_prev (EphyFindBar *bar);
