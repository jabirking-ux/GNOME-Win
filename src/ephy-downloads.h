#pragma once

#include <gtk/gtk.h>
#include <adwaita.h>

typedef enum {
  EPHY_DOWNLOAD_STATUS_RUNNING,
  EPHY_DOWNLOAD_STATUS_COMPLETED,
  EPHY_DOWNLOAD_STATUS_FAILED,
  EPHY_DOWNLOAD_STATUS_CANCELLED
} EphyDownloadStatus;

typedef struct {
  char *id;
  char *filename;
  char *destination_path;
  char *source_url;
  gint64 total_bytes;
  gint64 received_bytes;
  double progress;
  EphyDownloadStatus status;
  gint64 start_time;
} EphyDownload;

void ephy_downloads_init (void);
GList *ephy_downloads_get_all (void);
EphyDownload *ephy_downloads_start (const char *url, const char *suggested_filename);
void ephy_downloads_cancel (EphyDownload *download);
void ephy_downloads_clear_finished (void);
void ephy_downloads_show_popover (GtkWidget *relative_to);
