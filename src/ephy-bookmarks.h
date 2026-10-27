#pragma once

#include <glib.h>
#include <gtk/gtk.h>

typedef struct _EphyWindow EphyWindow;

typedef struct {
  char *title;
  char *url;
  char *tag;
} EphyBookmark;

void ephy_bookmarks_init (void);
GList *ephy_bookmarks_get_all (void);
gboolean ephy_bookmarks_is_bookmarked (const char *url);
void ephy_bookmarks_add (const char *title, const char *url);
void ephy_bookmarks_add_with_tag (const char *title, const char *url, const char *tag);
void ephy_bookmarks_remove (const char *url);
gboolean ephy_bookmarks_export_html (const char *filepath);
gboolean ephy_bookmarks_import_html (const char *filepath);
void ephy_bookmarks_dialog_show (GtkWindow *parent, EphyWindow *window);
