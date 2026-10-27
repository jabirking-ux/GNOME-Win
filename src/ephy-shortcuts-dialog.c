#include "ephy-shortcuts-dialog.h"

static GtkWidget *create_shortcut (const char *title, const char *accelerator) {
  GtkWidget *sc = g_object_new (GTK_TYPE_SHORTCUTS_SHORTCUT,
                                "title", title,
                                "accelerator", accelerator,
                                "shortcut-type", GTK_SHORTCUT_ACCELERATOR,
                                NULL);
  return sc;
}

static GtkWidget *create_group (const char *title) {
  GtkWidget *grp = g_object_new (GTK_TYPE_SHORTCUTS_GROUP,
                                 "title", title,
                                 NULL);
  return grp;
}

void ephy_shortcuts_dialog_show (GtkWindow *parent) {
  GtkWidget *win = g_object_new (GTK_TYPE_SHORTCUTS_WINDOW,
                                 "transient-for", parent,
                                 "modal", TRUE,
                                 NULL);

  GtkWidget *section = g_object_new (GTK_TYPE_SHORTCUTS_SECTION,
                                     "max-columns", 2,
                                     NULL);

  /* 1. Tabs and Windows */
  GtkWidget *grp_tabs = create_group ("Tabs and Windows");
  gtk_box_append (GTK_BOX (grp_tabs), create_shortcut ("New tab", "<Primary>t"));
  gtk_box_append (GTK_BOX (grp_tabs), create_shortcut ("New window", "<Primary>n"));
  gtk_box_append (GTK_BOX (grp_tabs), create_shortcut ("New incognito window", "<Primary><Shift>n"));
  gtk_box_append (GTK_BOX (grp_tabs), create_shortcut ("Close current tab", "<Primary>w"));
  gtk_box_append (GTK_BOX (grp_tabs), create_shortcut ("Reopen closed tab", "<Primary><Shift>t"));
  gtk_box_append (GTK_BOX (grp_tabs), create_shortcut ("Next tab", "<Primary>Tab"));
  gtk_box_append (GTK_BOX (grp_tabs), create_shortcut ("Previous tab", "<Primary><Shift>Tab"));
  gtk_box_append (GTK_BOX (section), grp_tabs);

  /* 2. Navigation */
  GtkWidget *grp_nav = create_group ("Navigation");
  gtk_box_append (GTK_BOX (grp_nav), create_shortcut ("Go back", "<Alt>Left"));
  gtk_box_append (GTK_BOX (grp_nav), create_shortcut ("Go forward", "<Alt>Right"));
  gtk_box_append (GTK_BOX (grp_nav), create_shortcut ("Reload page", "<Primary>r"));
  gtk_box_append (GTK_BOX (grp_nav), create_shortcut ("Reload page (alt)", "F5"));
  gtk_box_append (GTK_BOX (grp_nav), create_shortcut ("Homepage", "<Alt>Home"));
  gtk_box_append (GTK_BOX (section), grp_nav);

  /* 3. Search & Location */
  GtkWidget *grp_search = create_group ("Search & Location");
  gtk_box_append (GTK_BOX (grp_search), create_shortcut ("Focus location bar", "<Primary>l"));
  gtk_box_append (GTK_BOX (grp_search), create_shortcut ("Find in page", "<Primary>f"));
  gtk_box_append (GTK_BOX (grp_search), create_shortcut ("Find next match", "F3"));
  gtk_box_append (GTK_BOX (grp_search), create_shortcut ("Find previous match", "<Shift>F3"));
  gtk_box_append (GTK_BOX (section), grp_search);

  /* 4. Bookmarks, History & Downloads */
  GtkWidget *grp_data = create_group ("Bookmarks, History & Downloads");
  gtk_box_append (GTK_BOX (grp_data), create_shortcut ("Bookmark this page", "<Primary>d"));
  gtk_box_append (GTK_BOX (grp_data), create_shortcut ("View bookmarks", "<Primary>b"));
  gtk_box_append (GTK_BOX (grp_data), create_shortcut ("View browsing history", "<Primary>h"));
  gtk_box_append (GTK_BOX (grp_data), create_shortcut ("View downloads", "<Primary>y"));
  gtk_box_append (GTK_BOX (section), grp_data);

  /* 5. View & Zoom */
  GtkWidget *grp_view = create_group ("View & Zoom");
  gtk_box_append (GTK_BOX (grp_view), create_shortcut ("Zoom in", "<Primary>plus"));
  gtk_box_append (GTK_BOX (grp_view), create_shortcut ("Zoom out", "<Primary>minus"));
  gtk_box_append (GTK_BOX (grp_view), create_shortcut ("Reset zoom", "<Primary>0"));
  gtk_box_append (GTK_BOX (grp_view), create_shortcut ("Toggle fullscreen", "F11"));
  gtk_box_append (GTK_BOX (grp_view), create_shortcut ("View page source", "<Primary>u"));
  gtk_box_append (GTK_BOX (section), grp_view);

  /* 6. General */
  GtkWidget *grp_gen = create_group ("General");
  gtk_box_append (GTK_BOX (grp_gen), create_shortcut ("Preferences", "<Primary>comma"));
  gtk_box_append (GTK_BOX (grp_gen), create_shortcut ("Keyboard shortcuts", "<Primary>question"));
  gtk_box_append (GTK_BOX (grp_gen), create_shortcut ("Quit", "<Primary>q"));
  gtk_box_append (GTK_BOX (section), grp_gen);

  gtk_box_append (GTK_BOX (win), section);
  gtk_window_present (GTK_WINDOW (win));
}
