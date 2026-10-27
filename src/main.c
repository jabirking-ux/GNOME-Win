#include <adwaita.h>
#include "ephy-window.h"
#include "ephy-engine.h"
#include "ephy-settings.h"
#include "ephy-theme.h"
#include "ephy-bookmarks.h"
#include "ephy-history.h"
#include "ephy-downloads.h"
#include "ephy-session.h"

static gboolean opt_private = FALSE;
static char **opt_urls = NULL;

static const GOptionEntry options[] = {
  { "incognito", 'i', 0, G_OPTION_ARG_NONE, &opt_private, "Open in a private incognito window", NULL },
  { "private-window", 'p', 0, G_OPTION_ARG_NONE, &opt_private, "Open in a private incognito window", NULL },
  { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &opt_urls, "URLs to open", "[URL...]" },
  { NULL }
};

static void activate (GtkApplication *app, gpointer user_data) {
  ephy_engine_init ();
  ephy_theme_init ();
  ephy_bookmarks_init ();
  ephy_history_init ();
  ephy_downloads_init ();
  ephy_session_init ();

  EphySettings *settings = ephy_settings_get_default ();

  /* Set color scheme */
  AdwStyleManager *sm = adw_style_manager_get_default ();
  if (settings->color_scheme == 1)
    adw_style_manager_set_color_scheme (sm, ADW_COLOR_SCHEME_FORCE_LIGHT);
  else if (settings->color_scheme == 2)
    adw_style_manager_set_color_scheme (sm, ADW_COLOR_SCHEME_FORCE_DARK);
  else
    adw_style_manager_set_color_scheme (sm, ADW_COLOR_SCHEME_DEFAULT);

  /* Set standard GNOME Web keyboard shortcuts */
  gtk_application_set_accels_for_action (app, "win.new-tab", (const char *[]){ "<Primary>t", NULL });
  gtk_application_set_accels_for_action (app, "win.new-window", (const char *[]){ "<Primary>n", NULL });
  gtk_application_set_accels_for_action (app, "win.new-private-window", (const char *[]){ "<Primary><Shift>n", NULL });
  gtk_application_set_accels_for_action (app, "win.close-tab", (const char *[]){ "<Primary>w", NULL });
  gtk_application_set_accels_for_action (app, "win.reopen-closed-tab", (const char *[]){ "<Primary><Shift>t", NULL });
  gtk_application_set_accels_for_action (app, "win.focus-location", (const char *[]){ "<Primary>l", "<Alt>d", "F6", NULL });
  gtk_application_set_accels_for_action (app, "win.find", (const char *[]){ "<Primary>f", NULL });
  gtk_application_set_accels_for_action (app, "win.find-next", (const char *[]){ "F3", NULL });
  gtk_application_set_accels_for_action (app, "win.find-prev", (const char *[]){ "<Shift>F3", NULL });
  gtk_application_set_accels_for_action (app, "win.reload", (const char *[]){ "<Primary>r", "F5", NULL });
  gtk_application_set_accels_for_action (app, "win.bookmarks", (const char *[]){ "<Primary>b", "<Primary>d", NULL });
  gtk_application_set_accels_for_action (app, "win.history", (const char *[]){ "<Primary>h", NULL });
  gtk_application_set_accels_for_action (app, "win.downloads", (const char *[]){ "<Primary>y", "<Primary>j", NULL });
  gtk_application_set_accels_for_action (app, "win.view-source", (const char *[]){ "<Primary>u", NULL });
  gtk_application_set_accels_for_action (app, "win.shortcuts", (const char *[]){ "<Primary>question", "F1", NULL });
  gtk_application_set_accels_for_action (app, "win.preferences", (const char *[]){ "<Primary>comma", NULL });
  gtk_application_set_accels_for_action (app, "win.zoom-in", (const char *[]){ "<Primary>plus", "<Primary>equal", NULL });
  gtk_application_set_accels_for_action (app, "win.zoom-out", (const char *[]){ "<Primary>minus", NULL });
  gtk_application_set_accels_for_action (app, "win.zoom-reset", (const char *[]){ "<Primary>0", NULL });
  gtk_application_set_accels_for_action (app, "win.fullscreen", (const char *[]){ "F11", NULL });

  EphyWindow *win = ephy_window_new (app, opt_private);

  if (opt_urls && opt_urls[0]) {
    ephy_tab_load_url (win->active_tab, opt_urls[0]);
    for (int i = 1; opt_urls[i] != NULL; i++) {
      ephy_window_open_url (win, opt_urls[i]);
    }
  }
}

int main (int argc, char *argv[]) {
  g_autoptr (AdwApplication) app = adw_application_new ("org.gnome.Epiphany.Win", G_APPLICATION_NON_UNIQUE);

  g_application_add_main_option_entries (G_APPLICATION (app), options);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

  return g_application_run (G_APPLICATION (app), argc, argv);
}
