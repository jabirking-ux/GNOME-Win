#pragma once

#include <WebKit/WebKit2_C.h>
#include <WebKit/WKView.h>
#include <WebKit/WKContextConfigurationRef.h>
#include <WebKit/WKWebsiteDataStoreRef.h>
#include <WebKit/WKWebsiteDataStoreConfigurationRef.h>
#include <glib.h>

void ephy_engine_init (void);
WKContextRef ephy_engine_get_context (void);
WKPageConfigurationRef ephy_engine_create_page_config (gboolean is_private);
void ephy_engine_clear_browsing_data (void);
