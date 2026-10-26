/* Stage-1 proof of concept: embed WebKit WinCairo (WebKit2 C API) in a GTK4 window.
 * Build: see poc/build.sh. Run with WebKit WinCairo DLLs on PATH / next to the exe. */
#include <gtk/gtk.h>
#include <gdk/win32/gdkwin32.h>
#include <WebKit/WebKit2_C.h>
#include <WebKit/WKView.h>
#include <WebKit/WKContextConfigurationRef.h>

static WKViewRef view;
static GtkWidget *holder;
static GtkWidget *entry;

static gboolean sync_geometry(gpointer d) {
  graphene_rect_t b;
  GtkNative *n = gtk_widget_get_native(holder);
  if (!view || !n) return G_SOURCE_CONTINUE;
  if (!gtk_widget_compute_bounds(holder, GTK_WIDGET(n), &b)) return G_SOURCE_CONTINUE;
  HWND h = WKViewGetWindow(view);
  int s = 1; /* GTK4 win32 surface coords are in device px scaled by surface scale */
  GdkSurface *surf = gtk_native_get_surface(n);
  if (surf) s = (int)gdk_surface_get_scale_factor(surf);
  SetWindowPos(h, HWND_TOP, (int)b.origin.x * s, (int)b.origin.y * s,
               (int)b.size.width * s, (int)b.size.height * s, SWP_SHOWWINDOW);
  return G_SOURCE_CONTINUE;
}

static void on_activate_entry(GtkEntry *e, gpointer d) {
  const char *t = gtk_editable_get_text(GTK_EDITABLE(e));
  char *u = g_strstr_len(t, -1, "://") ? g_strdup(t) : g_strconcat("https://", t, NULL);
  WKPageLoadURL(WKViewGetPage(view), WKURLCreateWithUTF8CString(u));
  g_free(u);
}

static gboolean poll(gpointer d) {
  WKPageRef p = WKViewGetPage(view);
  WKStringRef t = WKPageCopyTitle(p);
  if (t) {
    size_t n = WKStringGetMaximumUTF8CStringSize(t);
    char *buf = g_malloc(n);
    WKStringGetUTF8CString(t, buf, n);
    gtk_window_set_title(GTK_WINDOW(d), buf[0] ? buf : "GNOME-Win PoC");
    g_free(buf);
    WKRelease(t);
  }
  return G_SOURCE_CONTINUE;
}

static void on_realize(GtkWidget *win, gpointer url) {
  GdkSurface *surf = gtk_native_get_surface(GTK_NATIVE(win));
  HWND top = gdk_win32_surface_get_handle(surf);
  WKContextConfigurationRef cc = WKContextConfigurationCreate();
  WKContextRef ctx = WKContextCreateWithConfiguration(cc);
  WKPageConfigurationRef pc = WKPageConfigurationCreate();
  WKPageConfigurationSetContext(pc, ctx);
  RECT r = {0, 0, 800, 600};
  view = WKViewCreate(r, pc, top);
  WKViewSetIsInWindow(view, true);
  WKPageLoadURL(WKViewGetPage(view), WKURLCreateWithUTF8CString((const char *)url));
  g_timeout_add(500, poll, win);
  g_timeout_add(100, sync_geometry, NULL);
}

static void activate(GtkApplication *app, gpointer url) {
  GtkWidget *win = gtk_application_window_new(app);
  gtk_window_set_default_size(GTK_WINDOW(win), 1100, 750);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  entry = gtk_entry_new();
  g_signal_connect(entry, "activate", G_CALLBACK(on_activate_entry), NULL);
  holder = gtk_drawing_area_new();
  gtk_widget_set_vexpand(holder, TRUE);
  gtk_box_append(GTK_BOX(box), entry);
  gtk_box_append(GTK_BOX(box), holder);
  gtk_window_set_child(GTK_WINDOW(win), box);
  g_signal_connect(win, "realize", G_CALLBACK(on_realize), url);
  gtk_window_present(GTK_WINDOW(win));
}

int main(int argc, char **argv) {
  GtkApplication *app = gtk_application_new("org.gnomewin.poc", G_APPLICATION_NON_UNIQUE);
  g_signal_connect(app, "activate", G_CALLBACK(activate), (gpointer)"https://example.com");
  return g_application_run(G_APPLICATION(app), 1, argv);
}
