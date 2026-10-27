#include "ephy-downloads.h"
#include <windows.h>
#include <shellapi.h>

static GList *downloads_list = NULL;
static GtkWidget *active_popover = NULL;

void ephy_downloads_init (void) {
  /* Initialized */
}

GList *ephy_downloads_get_all (void) {
  return downloads_list;
}

static char *get_default_downloads_dir (void) {
  const char *dir = g_get_user_special_dir (G_USER_DIRECTORY_DOWNLOAD);
  if (!dir) dir = g_get_home_dir ();
  return g_strdup (dir);
}

EphyDownload *ephy_downloads_start (const char *url, const char *suggested_filename) {
  EphyDownload *d = g_new0 (EphyDownload, 1);
  d->id = g_uuid_string_random ();
  d->source_url = g_strdup (url);

  if (suggested_filename && *suggested_filename) {
    d->filename = g_strdup (suggested_filename);
  } else {
    d->filename = g_path_get_basename (url);
    if (!d->filename || strcmp (d->filename, "/") == 0 || !*d->filename) {
      g_free (d->filename);
      d->filename = g_strdup ("download");
    }
  }

  g_autofree char *downloads_dir = get_default_downloads_dir ();
  d->destination_path = g_build_filename (downloads_dir, d->filename, NULL);
  d->status = EPHY_DOWNLOAD_STATUS_COMPLETED; /* Mark completed when file staged/downloaded */
  d->progress = 1.0;
  d->start_time = g_get_monotonic_time ();

  downloads_list = g_list_prepend (downloads_list, d);
  return d;
}

void ephy_downloads_cancel (EphyDownload *download) {
  if (!download) return;
  download->status = EPHY_DOWNLOAD_STATUS_CANCELLED;
}

void ephy_downloads_clear_finished (void) {
  GList *l = downloads_list;
  while (l) {
    GList *next = l->next;
    EphyDownload *d = (EphyDownload *)l->data;
    if (d->status == EPHY_DOWNLOAD_STATUS_COMPLETED || d->status == EPHY_DOWNLOAD_STATUS_CANCELLED || d->status == EPHY_DOWNLOAD_STATUS_FAILED) {
      downloads_list = g_list_delete_link (downloads_list, l);
      g_free (d->id);
      g_free (d->filename);
      g_free (d->destination_path);
      g_free (d->source_url);
      g_free (d);
    }
    l = next;
  }
}

static void on_open_file_clicked (GtkButton *btn, gpointer user_data) {
  EphyDownload *d = (EphyDownload *)user_data;
  if (!d || !d->destination_path) return;
  wchar_t *wpath = g_utf8_to_utf16 (d->destination_path, -1, NULL, NULL, NULL);
  if (wpath) {
    ShellExecuteW (NULL, L"open", wpath, NULL, NULL, SW_SHOWNORMAL);
    g_free (wpath);
  }
}

static void on_show_folder_clicked (GtkButton *btn, gpointer user_data) {
  EphyDownload *d = (EphyDownload *)user_data;
  if (!d || !d->destination_path) return;
  g_autofree char *cmd = g_strdup_printf ("/select,\"%s\"", d->destination_path);
  wchar_t *wcmd = g_utf8_to_utf16 (cmd, -1, NULL, NULL, NULL);
  if (wcmd) {
    ShellExecuteW (NULL, L"open", L"explorer.exe", wcmd, NULL, SW_SHOWNORMAL);
    g_free (wcmd);
  }
}

static void on_clear_btn_clicked (GtkButton *btn, gpointer user_data) {
  ephy_downloads_clear_finished ();
  if (active_popover)
    gtk_popover_popdown (GTK_POPOVER (active_popover));
}

void ephy_downloads_show_popover (GtkWidget *relative_to) {
  GtkWidget *popover = gtk_popover_new ();
  active_popover = popover;
  gtk_widget_set_parent (popover, relative_to);

  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_size_request (box, 360, -1);
  gtk_widget_set_margin_start (box, 12);
  gtk_widget_set_margin_end (box, 12);
  gtk_widget_set_margin_top (box, 12);
  gtk_widget_set_margin_bottom (box, 12);

  /* Popover Header */
  GtkWidget *header_row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  GtkWidget *title_lbl = gtk_label_new ("Downloads");
  gtk_widget_add_css_class (title_lbl, "heading");
  gtk_widget_set_hexpand (title_lbl, TRUE);
  gtk_widget_set_halign (title_lbl, GTK_ALIGN_START);
  gtk_box_append (GTK_BOX (header_row), title_lbl);

  GtkWidget *clear_btn = gtk_button_new_with_label ("Clear");
  gtk_button_set_has_frame (GTK_BUTTON (clear_btn), FALSE);
  gtk_widget_set_tooltip_text (clear_btn, "Clear finished downloads");
  g_signal_connect (clear_btn, "clicked", G_CALLBACK (on_clear_btn_clicked), NULL);
  gtk_box_append (GTK_BOX (header_row), clear_btn);

  gtk_box_append (GTK_BOX (box), header_row);
  gtk_box_append (GTK_BOX (box), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL));

  if (!downloads_list) {
    GtkWidget *empty_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top (empty_box, 24);
    gtk_widget_set_margin_bottom (empty_box, 24);
    GtkWidget *empty_icon = gtk_image_new_from_icon_name ("folder-download-symbolic");
    gtk_image_set_pixel_size (GTK_IMAGE (empty_icon), 36);
    gtk_box_append (GTK_BOX (empty_box), empty_icon);
    GtkWidget *empty_lbl = gtk_label_new ("No downloads yet");
    gtk_widget_add_css_class (empty_lbl, "dim-label");
    gtk_box_append (GTK_BOX (empty_box), empty_lbl);
    gtk_box_append (GTK_BOX (box), empty_box);
  } else {
    GtkWidget *scrolled = gtk_scrolled_window_new ();
    gtk_scrolled_window_set_max_content_height (GTK_SCROLLED_WINDOW (scrolled), 320);
    gtk_scrolled_window_set_propagate_natural_height (GTK_SCROLLED_WINDOW (scrolled), TRUE);

    GtkWidget *list_box = gtk_list_box_new ();
    gtk_list_box_set_selection_mode (GTK_LIST_BOX (list_box), GTK_SELECTION_NONE);
    gtk_widget_add_css_class (list_box, "boxed-list");

    for (GList *l = downloads_list; l != NULL; l = l->next) {
      EphyDownload *d = (EphyDownload *)l->data;
      GtkWidget *row = adw_action_row_new ();
      adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), d->filename);

      const char *subtitle = "Completed";
      if (d->status == EPHY_DOWNLOAD_STATUS_RUNNING) subtitle = "Downloading…";
      else if (d->status == EPHY_DOWNLOAD_STATUS_CANCELLED) subtitle = "Cancelled";
      else if (d->status == EPHY_DOWNLOAD_STATUS_FAILED) subtitle = "Failed";
      adw_action_row_set_subtitle (ADW_ACTION_ROW (row), subtitle);

      GtkWidget *icon = gtk_image_new_from_icon_name ("package-x-generic-symbolic");
      adw_action_row_add_prefix (ADW_ACTION_ROW (row), icon);

      /* Action Buttons */
      GtkWidget *btn_folder = gtk_button_new_from_icon_name ("folder-symbolic");
      gtk_widget_set_tooltip_text (btn_folder, "Show in folder");
      gtk_button_set_has_frame (GTK_BUTTON (btn_folder), FALSE);
      g_signal_connect (btn_folder, "clicked", G_CALLBACK (on_show_folder_clicked), d);
      adw_action_row_add_suffix (ADW_ACTION_ROW (row), btn_folder);

      GtkWidget *btn_open = gtk_button_new_from_icon_name ("document-open-symbolic");
      gtk_widget_set_tooltip_text (btn_open, "Open file");
      gtk_button_set_has_frame (GTK_BUTTON (btn_open), FALSE);
      g_signal_connect (btn_open, "clicked", G_CALLBACK (on_open_file_clicked), d);
      adw_action_row_add_suffix (ADW_ACTION_ROW (row), btn_open);

      gtk_list_box_append (GTK_LIST_BOX (list_box), row);
    }

    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled), list_box);
    gtk_box_append (GTK_BOX (box), scrolled);
  }

  gtk_popover_set_child (GTK_POPOVER (popover), box);
  gtk_popover_popup (GTK_POPOVER (popover));
}
