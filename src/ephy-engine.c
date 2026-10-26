#include <glib.h>
#include <sys/types.h>
#ifndef _PID_T_
#define _PID_T_
#endif

#include "ephy-engine.h"
#include "ephy-settings.h"
#include <glib/gstdio.h>

static WKContextRef shared_context = NULL;
static WKWebsiteDataStoreRef default_data_store = NULL;

void ephy_engine_init (void) {
  if (shared_context) return;

  g_autofree char *profile_dir = ephy_get_profile_dir ();
  g_autofree char *cache_dir = g_build_filename (profile_dir, "Cache", NULL);
  g_autofree char *storage_dir = g_build_filename (profile_dir, "Storage", NULL);
  g_mkdir_with_parents (cache_dir, 0700);
  g_mkdir_with_parents (storage_dir, 0700);

  WKContextConfigurationRef cc = WKContextConfigurationCreate ();
  WKStringRef cache_str = WKStringCreateWithUTF8CString (cache_dir);
  WKContextConfigurationSetDiskCacheDirectory (cc, cache_str);
  WKRelease (cache_str);

  WKStringRef storage_str = WKStringCreateWithUTF8CString (storage_dir);
  WKContextConfigurationSetLocalStorageDirectory (cc, storage_str);
  WKContextConfigurationSetIndexedDBDatabaseDirectory (cc, storage_str);
  WKRelease (storage_str);

  shared_context = WKContextCreateWithConfiguration (cc);
  WKRelease (cc);

  default_data_store = WKWebsiteDataStoreGetDefaultDataStore ();
}

WKContextRef ephy_engine_get_context (void) {
  ephy_engine_init ();
  return shared_context;
}

WKPageConfigurationRef ephy_engine_create_page_config (gboolean is_private) {
  ephy_engine_init ();
  WKPageConfigurationRef pc = WKPageConfigurationCreate ();
  WKPageConfigurationSetContext (pc, shared_context);

  if (is_private) {
    WKWebsiteDataStoreRef private_store = WKWebsiteDataStoreCreateNonPersistentDataStore ();
    WKPageConfigurationSetWebsiteDataStore (pc, private_store);
    WKRelease (private_store);
  } else {
    WKPageConfigurationSetWebsiteDataStore (pc, default_data_store);
  }

  /* Configure preferences */
  WKPreferencesRef prefs = WKPreferencesCreate ();
  WKPreferencesSetJavaScriptCanOpenWindowsAutomatically (prefs, true);
  WKPreferencesSetLoadsImagesAutomatically (prefs, true);
  WKPreferencesSetDeveloperExtrasEnabled (prefs, true);
  WKPageConfigurationSetPreferences (pc, prefs);
  WKRelease (prefs);

  return pc;
}

static void on_clear_done (void *context) {
  /* completion callback */
}

void ephy_engine_clear_browsing_data (void) {
  if (default_data_store) {
    WKWebsiteDataStoreRemoveAllFetchCaches (default_data_store, NULL, (WKWebsiteDataStoreRemoveFetchCacheRemovalFunction)on_clear_done);
    WKWebsiteDataStoreRemoveAllIndexedDatabases (default_data_store, NULL, (WKWebsiteDataStoreRemoveAllIndexedDatabasesCallback)on_clear_done);
    WKWebsiteDataStoreRemoveLocalStorage (default_data_store, NULL, (WKWebsiteDataStoreRemoveLocalStorageCallback)on_clear_done);
    WKWebsiteDataStoreRemoveNetworkCache (default_data_store, NULL, (WKWebsiteDataStoreRemoveNetworkCacheCallback)on_clear_done);
    WKWebsiteDataStoreRemoveMemoryCaches (default_data_store, NULL, (WKWebsiteDataStoreRemoveMemoryCachesRemovalFunction)on_clear_done);
  }
}
