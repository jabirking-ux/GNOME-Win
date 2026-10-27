#pragma once

#include <glib.h>

typedef struct _EphyWindow EphyWindow;

void ephy_session_init (void);
void ephy_session_save (EphyWindow *window);
gboolean ephy_session_restore (EphyWindow *window);
void ephy_session_push_closed_tab (const char *title, const char *url);
gboolean ephy_session_can_reopen_closed_tab (void);
char *ephy_session_pop_closed_tab (char **out_title);
