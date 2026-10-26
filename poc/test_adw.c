#include <adwaita.h>

static void activate (GtkApplication *app, gpointer user_data) {
  GtkWidget *window = adw_application_window_new (app);
  GtkWidget *toolbar_view = adw_toolbar_view_new ();
  GtkWidget *header = adw_header_bar_new ();
  adw_toolbar_view_add_top_bar (ADW_TOOLBAR_VIEW (toolbar_view), header);
  adw_application_window_set_content (ADW_APPLICATION_WINDOW (window), toolbar_view);
  gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);
  gtk_window_set_title (GTK_WINDOW (window), "GNOME Web");
  gtk_window_present (GTK_WINDOW (window));
}

int main (int argc, char *argv[]) {
  g_autoptr (AdwApplication) app = adw_application_new ("org.gnome.Epiphany.Win", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  return g_application_run (G_APPLICATION (app), argc, argv);
}
