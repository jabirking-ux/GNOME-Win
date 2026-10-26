#pragma once

#include <glib.h>

typedef struct {
  char *title;
  char *url;
  gint64 timestamp;
} EphyHistoryEntry;

void ephy_history_init (void);
GList *ephy_history_get_all (void);
void ephy_history_add (const char *title, const char *url);
void ephy_history_clear (void);
