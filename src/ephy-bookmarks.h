#pragma once

#include <glib.h>

typedef struct {
  char *title;
  char *url;
} EphyBookmark;

void ephy_bookmarks_init (void);
GList *ephy_bookmarks_get_all (void);
gboolean ephy_bookmarks_is_bookmarked (const char *url);
void ephy_bookmarks_add (const char *title, const char *url);
void ephy_bookmarks_remove (const char *url);
