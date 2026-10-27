#include "ephy-theme.h"

static const char *GNOME_WEB_CSS =
  "/* Authentic GNOME Web (Epiphany) Stylesheet */\n"
  "\n"
  "/* Centered Location Entry */\n"
  ".ephy-location-box {\n"
  "  min-width: 320px;\n"
  "}\n"
  ".ephy-location-entry {\n"
  "  border-radius: 9999px;\n"
  "  padding: 3px 10px;\n"
  "  font-size: 13.5px;\n"
  "}\n"
  "\n"
  "/* Progress Indicator */\n"
  ".ephy-progress {\n"
  "  min-height: 2px;\n"
  "  margin-top: 1px;\n"
  "}\n"
  ".ephy-progress trough {\n"
  "  background: transparent;\n"
  "}\n"
  ".ephy-progress progress {\n"
  "  background-color: @accent_color;\n"
  "  border-radius: 9999px;\n"
  "}\n"
  "\n"
  "/* Linked Navigation Cluster */\n"
  ".navigation-box button {\n"
  "  padding: 5px 8px;\n"
  "}\n";

void ephy_theme_init (void) {
  GtkCssProvider *provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_string (provider, GNOME_WEB_CSS);
  GdkDisplay *display = gdk_display_get_default ();
  if (display) {
    gtk_style_context_add_provider_for_display (
      display,
      GTK_STYLE_PROVIDER (provider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
  }
}
