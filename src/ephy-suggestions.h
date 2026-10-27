#pragma once

#include <gtk/gtk.h>
#include <adwaita.h>

typedef struct _EphyWindow EphyWindow;

typedef struct _EphySuggestions EphySuggestions;

struct _EphySuggestions {
  GtkWidget *popover;
  GtkWidget *list_box;
  EphyWindow *window;
};

EphySuggestions *ephy_suggestions_new (EphyWindow *window);
void ephy_suggestions_update (EphySuggestions *sug, const char *query);
void ephy_suggestions_hide (EphySuggestions *sug);
