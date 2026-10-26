#include "ephy-security.h"
#include <adwaita.h>

void ephy_security_show_popover (GtkWidget *relative_to, EphyTab *tab) {
  GtkWidget *popover = gtk_popover_new ();
  gtk_popover_set_position (GTK_POPOVER (popover), GTK_POS_BOTTOM);

  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start (box, 16);
  gtk_widget_set_margin_end (box, 16);
  gtk_widget_set_margin_top (box, 16);
  gtk_widget_set_margin_bottom (box, 16);

  gboolean is_https = tab && tab->url && g_str_has_prefix (tab->url, "https://");
  gboolean is_priv = tab && tab->is_private;

  const char *icon_name = is_priv ? "channel-secure-symbolic" : (is_https ? "security-high-symbolic" : "security-low-symbolic");
  GtkWidget *icon = gtk_image_new_from_icon_name (icon_name);
  gtk_image_set_pixel_size (GTK_IMAGE (icon), 32);
  gtk_box_append (GTK_BOX (box), icon);

  const char *title_text = is_priv ? "Private Browsing" : (is_https ? "Connection is Secure" : "Not Secure");
  GtkWidget *title_lbl = gtk_label_new (title_text);
  gtk_widget_add_css_class (title_lbl, "title-3");
  gtk_box_append (GTK_BOX (box), title_lbl);

  const char *desc = is_priv ?
    "In private browsing, history and cookies are isolated and never saved to your drive." :
    (is_https ?
      "Your information (passwords, credit cards) is private when it is sent to this site." :
      "You should not enter sensitive info on this site (passwords, cards), as it may be intercepted.");

  GtkWidget *desc_lbl = gtk_label_new (desc);
  gtk_label_set_wrap (GTK_LABEL (desc_lbl), TRUE);
  gtk_label_set_max_width_chars (GTK_LABEL (desc_lbl), 32);
  gtk_box_append (GTK_BOX (box), desc_lbl);

  gtk_popover_set_child (GTK_POPOVER (popover), box);
  gtk_widget_set_parent (popover, relative_to);
  gtk_popover_popup (GTK_POPOVER (popover));
}
