#include "ephy-theme.h"
#include "ephy-settings.h"

static GtkCssProvider *safari_provider = NULL;
static gboolean safari_active = FALSE;

static const char *SAFARI_CSS =
  "/* macOS 26 Safari Inspired Community Theme */\n"
  ".safari-header {\n"
  "  background-color: alpha(@window_bg_color, 0.85);\n"
  "  border-bottom: 1px solid alpha(@borders, 0.7);\n"
  "  padding-left: 6px;\n"
  "  padding-right: 6px;\n"
  "}\n"
  "\n"
  "/* Traffic Light Window Controls */\n"
  ".traffic-lights {\n"
  "  margin-right: 12px;\n"
  "  margin-left: 4px;\n"
  "}\n"
  ".traffic-btn {\n"
  "  min-width: 12px;\n"
  "  min-height: 12px;\n"
  "  border-radius: 9999px;\n"
  "  padding: 0;\n"
  "  margin: 0 4px;\n"
  "  border: 1px solid rgba(0, 0, 0, 0.15);\n"
  "  transition: filter 150ms ease, transform 100ms ease;\n"
  "}\n"
  ".traffic-btn:hover {\n"
  "  filter: brightness(0.9);\n"
  "  transform: scale(1.08);\n"
  "}\n"
  ".traffic-btn.close {\n"
  "  background-color: #ff5f56;\n"
  "  border-color: #e0443e;\n"
  "}\n"
  ".traffic-btn.minimize {\n"
  "  background-color: #ffbd2e;\n"
  "  border-color: #dea123;\n"
  "}\n"
  ".traffic-btn.zoom {\n"
  "  background-color: #27c93f;\n"
  "  border-color: #1aab29;\n"
  "}\n"
  "\n"
  "/* Safari Pill Smart Location Entry */\n"
  ".safari-location-box {\n"
  "  min-width: 380px;\n"
  "  margin: 0 12px;\n"
  "}\n"
  ".safari-location-entry {\n"
  "  border-radius: 12px;\n"
  "  font-size: 13.5px;\n"
  "  border: 1px solid alpha(@borders, 0.6);\n"
  "  background: alpha(@card_bg_color, 0.8);\n"
  "  box-shadow: inset 0 1px 2px alpha(black, 0.04);\n"
  "  padding: 4px 10px;\n"
  "  transition: border-color 200ms ease, box-shadow 200ms ease, background 200ms ease;\n"
  "}\n"
  ".safari-location-entry:focus-within {\n"
  "  border-color: @accent_color;\n"
  "  box-shadow: 0 0 0 2px alpha(@accent_color, 0.25);\n"
  "  background: @card_bg_color;\n"
  "}\n"
  "\n"
  "/* Compact Safari Tabs */\n"
  "tabbar tabbox {\n"
  "  padding: 3px 6px;\n"
  "}\n"
  "tabbar tab {\n"
  "  border-radius: 8px;\n"
  "  margin: 2px 3px;\n"
  "  font-weight: 500;\n"
  "  font-size: 13px;\n"
  "  border: 1px solid transparent;\n"
  "  transition: background-color 150ms ease, box-shadow 150ms ease;\n"
  "}\n"
  "tabbar tab:checked {\n"
  "  background-color: @card_bg_color;\n"
  "  border: 1px solid alpha(@borders, 0.6);\n"
  "  box-shadow: 0 1px 3px alpha(black, 0.08);\n"
  "}\n"
  "\n"
  "/* Security Badge */\n"
  ".security-badge-secure {\n"
  "  color: @accent_color;\n"
  "}\n"
  ".security-badge-insecure {\n"
  "  color: #e01b24;\n"
  "}\n";

void ephy_theme_init (void) {
  safari_provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_string (safari_provider, SAFARI_CSS);

  EphySettings *settings = ephy_settings_get_default ();
  ephy_theme_apply_safari (settings->safari_theme_enabled);
}

void ephy_theme_apply_safari (gboolean enabled) {
  GdkDisplay *display = gdk_display_get_default ();
  if (!display) return;

  if (enabled && !safari_active) {
    gtk_style_context_add_provider_for_display (
      display,
      GTK_STYLE_PROVIDER (safari_provider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    safari_active = TRUE;
  } else if (!enabled && safari_active) {
    gtk_style_context_remove_provider_for_display (
      display,
      GTK_STYLE_PROVIDER (safari_provider)
    );
    safari_active = FALSE;
  }
}

gboolean ephy_theme_is_safari (void) {
  return safari_active;
}
