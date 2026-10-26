#include <adwaita.h>
#include <gdk/win32/gdkwin32.h>
#include <WebKit/WebKit2_C.h>
#include <WebKit/WKView.h>
#include <WebKit/WKContextConfigurationRef.h>
#include <windows.h>

typedef struct {
  GtkWidget *holder;
  WKViewRef view;
  WKPageRef page;
  AdwTabPage *page_handle;
  char *title;
  char *url;
} BrowserTab;

static GList *tabs = NULL;
static BrowserTab *current_tab = NULL;
static GtkWidget *window = NULL;
static AdwTabView *tab_view = NULL;
static AdwTabBar *tab_bar = NULL;
static GtkWidget *location_entry = NULL;
static WKContextRef wk_context = NULL;

static void sync_active_tab_geometry (void) {
  if (!current_tab || !current_tab->holder || !current_tab->view) return;
  GtkNative *native = gtk_widget_get_native (current_tab->holder);
  if (!native) return;
  graphene_rect_t bounds;
  if (!gtk_widget_compute_bounds (current_tab->holder, GTK_WIDGET (native), &bounds)) return;
  
  GdkSurface *surface = gtk_native_get_surface (native);
  int scale = surface ? (int)gdk_surface_get_scale_factor (surface) : 1;
  HWND hwnd = WKViewGetWindow (current_tab->view);
  if (hwnd) {
    SetWindowPos (hwnd, HWND_TOP,
                  (int)(bounds.origin.x * scale),
                  (int)(bounds.origin.y * scale),
                  (int)(bounds.size.width * scale),
                  (int)(bounds.size.height * scale),
                  SWP_SHOWWINDOW);
  }
}

static gboolean on_sync_timer (gpointer user_data) {
  sync_active_tab_geometry ();
  return G_SOURCE_CONTINUE;
}

static void on_tab_selected (AdwTabView *view, GParamSpec *pspec, gpointer user_data) {
  AdwTabPage *selected = adw_tab_view_get_selected_page (view);
  if (!selected) return;

  for (GList *l = tabs; l != NULL; l = l->next) {
    BrowserTab *tab = (BrowserTab *)l->data;
    HWND hwnd = WKViewGetWindow (tab->view);
    if (tab->page_handle == selected) {
      current_tab = tab;
      if (hwnd) ShowWindow (hwnd, SW_SHOW);
      if (tab->url) gtk_editable_set_text (GTK_EDITABLE (location_entry), tab->url);
    } else {
      if (hwnd) ShowWindow (hwnd, SW_HIDE);
    }
  }
  sync_active_tab_geometry ();
}

static BrowserTab *create_new_tab (const char *initial_url) {
  BrowserTab *tab = g_new0 (BrowserTab, 1);
  tab->holder = gtk_drawing_area_new ();
  gtk_widget_set_vexpand (tab->holder, TRUE);
  gtk_widget_set_hexpand (tab->holder, TRUE);

  tab->page_handle = adw_tab_view_append (tab_view, tab->holder);
  adw_tab_page_set_title (tab->page_handle, "New Tab");

  GdkSurface *surface = gtk_native_get_surface (GTK_NATIVE (window));
  HWND top_hwnd = gdk_win32_surface_get_handle (surface);

  WKPageConfigurationRef pc = WKPageConfigurationCreate ();
  WKPageConfigurationSetContext (pc, wk_context);

  RECT r = {0, 0, 800, 600};
  tab->view = WKViewCreate (r, pc, top_hwnd);
  tab->page = WKViewGetPage (tab->view);
  WKViewSetIsInWindow (tab->view, true);

  tab->url = g_strdup (initial_url ? initial_url : "https://example.com");
  WKPageLoadURL (tab->page, WKURLCreateWithUTF8CString (tab->url));

  tabs = g_list_append (tabs, tab);
  adw_tab_view_set_selected_page (tab_view, tab->page_handle);
  return tab;
}

static void on_new_tab_clicked (GtkButton *btn, gpointer user_data) {
  create_new_tab ("https://gnome.org");
}

static void on_location_activate (GtkEntry *entry, gpointer user_data) {
  if (!current_tab) return;
  const char *text = gtk_editable_get_text (GTK_EDITABLE (entry));
  char *url = g_strstr_len (text, -1, "://") ? g_strdup (text) : g_strconcat ("https://", text, NULL);
  g_free (current_tab->url);
  current_tab->url = url;
  WKPageLoadURL (current_tab->page, WKURLCreateWithUTF8CString (url));
}

static void activate (GtkApplication *app, gpointer user_data) {
  window = adw_application_window_new (app);
  gtk_window_set_default_size (GTK_WINDOW (window), 1024, 768);
  gtk_window_set_title (GTK_WINDOW (window), "GNOME Web");

  WKContextConfigurationRef cc = WKContextConfigurationCreate ();
  wk_context = WKContextCreateWithConfiguration (cc);

  GtkWidget *toolbar_view = adw_toolbar_view_new ();
  GtkWidget *header = adw_header_bar_new ();
  adw_toolbar_view_add_top_bar (ADW_TOOLBAR_VIEW (toolbar_view), header);

  location_entry = gtk_entry_new ();
  gtk_widget_set_hexpand (location_entry, TRUE);
  g_signal_connect (location_entry, "activate", G_CALLBACK (on_location_activate), NULL);
  adw_header_bar_set_title_widget (ADW_HEADER_BAR (header), location_entry);

  GtkWidget *new_tab_btn = gtk_button_new_from_icon_name ("tab-new-symbolic");
  g_signal_connect (new_tab_btn, "clicked", G_CALLBACK (on_new_tab_clicked), NULL);
  adw_header_bar_pack_end (ADW_HEADER_BAR (header), new_tab_btn);

  tab_view = adw_tab_view_new ();
  tab_bar = adw_tab_bar_new ();
  adw_tab_bar_set_view (tab_bar, tab_view);
  adw_toolbar_view_add_top_bar (ADW_TOOLBAR_VIEW (toolbar_view), GTK_WIDGET (tab_bar));

  adw_toolbar_view_set_content (ADW_TOOLBAR_VIEW (toolbar_view), GTK_WIDGET (tab_view));
  adw_application_window_set_content (ADW_APPLICATION_WINDOW (window), toolbar_view);

  g_signal_connect (tab_view, "notify::selected-page", G_CALLBACK (on_tab_selected), NULL);

  gtk_window_present (GTK_WINDOW (window));

  create_new_tab ("https://example.com");
  g_timeout_add (50, on_sync_timer, NULL);
}

int main (int argc, char *argv[]) {
  g_autoptr (AdwApplication) app = adw_application_new ("org.gnome.Epiphany.MultiTab", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  return g_application_run (G_APPLICATION (app), argc, argv);
}
