# Architecture & Design Decisions: GNOME Web for Windows

## 1. Engine Selection & Multi-Process Architecture

### WebKit WinCairo
Epiphany on Linux uses **WebKitGTK**. However, WebKitGTK cannot run natively on Windows because its process IPC and rendering backends depend fundamentally on POSIX constructs (Unix sockets, shared memory file descriptors, Linux namespaces, bubblewrap sandboxing, and Wayland/X11 GDK display connections).

Instead of substituting Chromium or an outdated WebKit fork, this project targets **Apple WebKit (WinCairo port)**:
- **Port:** Official WinCairo 64-bit multi-process port.
- **Rendering:** Cairo 2D vector graphics and Angle/Direct3D/DirectComposition.
- **JavaScript Engine:** JavaScriptCore with full 64-bit DFG JIT compiler.
- **Process Architecture:**
  - **UIProcess (`gnome-web.exe`)**: Owns the GTK4 / Libadwaita window, manages tabs, navigation entry, menus, and user interaction.
  - **WebKitWebProcess.exe**: Isolates DOM parsing, CSS rendering, and JS execution into dedicated child processes.
  - **WebKitNetworkProcess.exe**: Handles all network traffic, HTTP/2/3, TLS handshakes, and disk caching.
  - **WebKitGPUProcess.exe**: Offloads 2D/3D hardware compositing and multimedia rendering.

Embedding is performed natively through the WebKit2 C API (`WKViewCreate`, `WKPageConfigurationCreate`, `WKContextCreateWithConfiguration`). The Win32 `HWND` created by `WKViewCreate` is embedded into the GTK4 widget tree using `GdkSurface` coordinate projection and `SetWindowPos` geometry synchronization.

---

## 2. UI Layer: GTK4 & Libadwaita

The application interface is built directly on GTK4 and Libadwaita:
- `AdwApplicationWindow`: Provides the modern rounded window frame, responsive layout, and adaptive styling.
- `AdwHeaderBar`: Hosts back/forward navigation, reload/stop, home, the address bar, and window actions.
- `AdwTabView` & `AdwTabBar`: Native Libadwaita tab management with smooth tab closing animations, reordering, and overflow handling.
- `AdwPreferencesWindow`: Delivers GNOME's clean preferences dialog with `AdwActionRow`, `AdwSwitchRow`, and `AdwComboRow`.
- `AdwAboutDialog`: Built-in application metadata and credits.

---

## 3. Theme System: GNOME Adwaita & macOS 26 Safari Community Theme

The browser features an extensible styling system that allows instant switching between two distinct design aesthetics:

### A. Standard GNOME Adwaita (Default)
Matches the authentic GNOME Web 47/48/49 visual identity on Linux:
- Clean Adwaita header bar.
- Standard platform window controls.
- Consistent GNOME typography, borders, and color tokens.

### B. macOS 26 Safari Community Theme (Optional)
Inspired by modern macOS Safari and community Epiphany customizations:
- **Traffic Light Controls**: Custom circular colored buttons placed at the top-left of the toolbar:
  - Close (#FF5F56)
  - Minimize (#FFBD2E)
  - Zoom / Maximize (#27C93F)
- **Frosted Translucent Toolbar**: Translucent header bar with alpha blending over the window surface.
- **Pill Location Entry**: Centered capsule-shaped address bar with soft inset shadows, refined borders, and prominent HTTPS lock indicator.
- **Compact Safari Tabs**: Rounded tab elements with active-tab elevation and subtle contrast.

### Theme Persistence & Opt-Out
The theme can be toggled at any moment from the main menu (`win.toggle-safari`) or within Preferences (`Appearance & Theme`). The selection is saved in `settings.json` and applied automatically on subsequent launches.

---

## 4. Privacy & Data Storage

- **Normal Profile**:
  - Local configuration: `%LOCALAPPDATA%\GNOME-Web\settings.json`
  - Bookmarks: `%LOCALAPPDATA%\GNOME-Web\bookmarks.json`
  - Browsing History: `%LOCALAPPDATA%\GNOME-Web\history.json`
  - WebKit persistent store: `%LOCALAPPDATA%\GNOME-Web\WebKitData\` (Disk cache, localStorage, IndexedDB, cookies).
- **Private Browsing (Incognito)**:
  - Uses `WKWebsiteDataStoreCreateNonPersistentDataStore()`.
  - Memory-only session: cookies, cache, and website data are isolated in RAM and permanently discarded upon window closure.
  - History tracking is completely bypassed.
