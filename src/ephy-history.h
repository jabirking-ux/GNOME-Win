#pragma once

#include <glib.h>
#include <gtk/gtk.h>

typedef struct {
  char *title;
  char *url;
  gint64 timestamp;
} EphyHistoryEntry;

typedef struct _EphyWindow EphyWindow;

void ephy_history_init (void);
GList *ephy_history_get_all (void);
void ephy_history_add (const char *title, const char *url);
void ephy_history_remove (const char *url);
void ephy_history_clear (void);
void ephy_history_clear_range (gint64 since_timestamp);
void ephy_history_dialog_show (GtkWindow *parent, EphyWindow *window);
