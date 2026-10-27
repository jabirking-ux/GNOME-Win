#include "ephy-tab.h"
#include "ephy-window.h"
#include "ephy-engine.h"
#include "ephy-history.h"
#include <gdk/win32/gdkwin32.h>

static void extract_active_url (EphyTab *tab) {
  WKURLRef url_ref = WKPageCopyActiveURL (tab->wk_page);
  if (url_ref) {
    WKStringRef str_ref = WKURLCopyString (url_ref);
    if (str_ref) {
      size_t max = WKStringGetMaximumUTF8CStringSize (str_ref);
      g_free (tab->url);
      tab->url = g_malloc (max);
      WKStringGetUTF8CString (str_ref, tab->url, max);
      WKRelease (str_ref);
    }
    WKRelease (url_ref);
  }
}

EphyTab *ephy_tab_new (EphyWindow *window, HWND top_hwnd, const char *initial_url, gboolean is_private) {
  g_print ("tab.1: allocating EphyTab\n");
  EphyTab *tab = g_new0 (EphyTab, 1);
  tab->window = window;
  tab->is_private = is_private;
  tab->zoom_level = 1.0;
  tab->last_x = -1;
  tab->last_y = -1;
  tab->last_w = -1;
  tab->last_h = -1;

  g_print ("tab.2: drawing area holder\n");
  tab->holder = gtk_drawing_area_new ();
  gtk_widget_set_vexpand (tab->holder, TRUE);
  gtk_widget_set_hexpand (tab->holder, TRUE);

  g_print ("tab.3: adw_tab_view_append\n");
  tab->adw_page = adw_tab_view_append (window->tab_view, tab->holder);
  adw_tab_page_set_title (tab->adw_page, "Loading...");

  g_print ("tab.4: ephy_engine_create_page_config\n");
  WKPageConfigurationRef pc = ephy_engine_create_page_config (is_private);
  RECT r = {0, 0, 800, 600};

  g_print ("tab.5: WKViewCreate top_hwnd=%p\n", (void*)top_hwnd);
  tab->wk_view = WKViewCreate (r, pc, top_hwnd);
  g_print ("tab.6: WKViewGetPage\n");
  tab->wk_page = WKViewGetPage (tab->wk_view);
  g_print ("tab.7: WKViewSetIsInWindow\n");
  WKViewSetIsInWindow (tab->wk_view, true);
  WKRelease (pc);

  g_print ("tab.8: initial url\n");
  if (initial_url && *initial_url) {
    ephy_tab_load_url (tab, initial_url);
  }

  g_print ("tab.done!\n");
  return tab;
}

void ephy_tab_load_url (EphyTab *tab, const char *url) {
  if (!tab || !url) return;
  g_free (tab->url);
  tab->url = g_strdup (url);
  WKURLRef wk_url = WKURLCreateWithUTF8CString (url);
  WKPageLoadURL (tab->wk_page, wk_url);
  WKRelease (wk_url);
}

void ephy_tab_go_back (EphyTab *tab) {
  if (tab && WKPageCanGoBack (tab->wk_page))
    WKPageGoBack (tab->wk_page);
}

void ephy_tab_go_forward (EphyTab *tab) {
  if (tab && WKPageCanGoForward (tab->wk_page))
    WKPageGoForward (tab->wk_page);
}

void ephy_tab_reload (EphyTab *tab) {
  if (tab)
    WKPageReload (tab->wk_page);
}

void ephy_tab_stop (EphyTab *tab) {
  if (tab)
    WKPageStopLoading (tab->wk_page);
}

void ephy_tab_zoom_in (EphyTab *tab) {
  if (tab) {
    tab->zoom_level += 0.1;
    WKPageSetPageZoomFactor (tab->wk_page, tab->zoom_level);
  }
}

void ephy_tab_zoom_out (EphyTab *tab) {
  if (tab && tab->zoom_level > 0.3) {
    tab->zoom_level -= 0.1;
    WKPageSetPageZoomFactor (tab->wk_page, tab->zoom_level);
  }
}

void ephy_tab_zoom_reset (EphyTab *tab) {
  if (tab) {
    tab->zoom_level = 1.0;
    WKPageSetPageZoomFactor (tab->wk_page, 1.0);
  }
}

void ephy_tab_sync_geometry (EphyTab *tab) {
  if (!tab || !tab->holder || !tab->wk_view) return;
  GtkNative *native = gtk_widget_get_native (tab->holder);
  if (!native) return;

  graphene_rect_t bounds;
  if (!gtk_widget_compute_bounds (tab->holder, GTK_WIDGET (native), &bounds)) return;

  double surf_x = 0.0, surf_y = 0.0;
  gtk_native_get_surface_transform (native, &surf_x, &surf_y);

  GdkSurface *surface = gtk_native_get_surface (native);
  int scale = surface ? (int)gdk_surface_get_scale_factor (surface) : 1;

  int x = (int)((surf_x + bounds.origin.x) * scale);
  int y = (int)((surf_y + bounds.origin.y) * scale);
  int w = (int)(bounds.size.width * scale);
  int h = (int)(bounds.size.height * scale);

  if (x == tab->last_x && y == tab->last_y && w == tab->last_w && h == tab->last_h)
    return;

  tab->last_x = x;
  tab->last_y = y;
  tab->last_w = w;
  tab->last_h = h;

  HWND hwnd = WKViewGetWindow (tab->wk_view);
  if (hwnd) {
    SetWindowPos (hwnd, NULL, x, y, w, h,
                  SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW);
  }
}

void ephy_tab_poll_state (EphyTab *tab) {
  if (!tab || !tab->wk_page) return;

  /* Poll Title */
  WKStringRef title_ref = WKPageCopyTitle (tab->wk_page);
  if (title_ref) {
    size_t max = WKStringGetMaximumUTF8CStringSize (title_ref);
    char *buf = g_malloc (max);
    WKStringGetUTF8CString (title_ref, buf, max);
    if (buf[0] && g_strcmp0 (tab->title, buf) != 0) {
      g_free (tab->title);
      tab->title = buf;
      if (tab->adw_page)
        adw_tab_page_set_title (tab->adw_page, tab->title);
      if (!tab->is_private && tab->url)
        ephy_history_add (tab->title, tab->url);
    } else {
      g_free (buf);
    }
    WKRelease (title_ref);
  }

  /* Poll URL */
  extract_active_url (tab);

  /* Poll Progress and Navigation State */
  tab->progress = WKPageGetEstimatedProgress (tab->wk_page);
  tab->can_go_back = WKPageCanGoBack (tab->wk_page);
  tab->can_go_forward = WKPageCanGoForward (tab->wk_page);
  tab->is_loading = (tab->progress > 0.0 && tab->progress < 1.0);
}

void ephy_tab_show (EphyTab *tab) {
  if (!tab || !tab->wk_view) return;
  HWND hwnd = WKViewGetWindow (tab->wk_view);
  if (hwnd) ShowWindow (hwnd, SW_SHOW);
  ephy_tab_sync_geometry (tab);
}

void ephy_tab_hide (EphyTab *tab) {
  if (!tab || !tab->wk_view) return;
  HWND hwnd = WKViewGetWindow (tab->wk_view);
  if (hwnd) ShowWindow (hwnd, SW_HIDE);
}

void ephy_tab_destroy (EphyTab *tab) {
  if (!tab) return;
  HWND hwnd = WKViewGetWindow (tab->wk_view);
  if (hwnd) DestroyWindow (hwnd);
  g_free (tab->title);
  g_free (tab->url);
  g_free (tab);
}
