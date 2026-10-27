# Porting Status: GNOME Web (Epiphany) for Windows

## Executive Summary
A fully functional, native Windows x64 port of **GNOME Web (Epiphany)** powered by **Apple WebKit (WinCairo WebKit2 multi-process architecture)** and **GTK4 / Libadwaita** has been successfully designed, implemented, verified, and packaged.

## Completed & Verified Work

- [x] **WebKit Engine Integration**:
  - Embedded real WebKit (`WebKit2.dll`, `WebCore.dll`, `JavaScriptCore.dll`) via WinCairo WebKit2 C API (`WKViewCreate`, `WKPageLoadURL`, `WKContextCreateWithConfiguration`).
  - True multi-process rendering architecture: `gnome-web.exe` spawns native `WebKitWebProcess.exe`, `WebKitNetworkProcess.exe`, and `WebKitGPUProcess.exe`.
  - Tested live HTTPS navigation, rendering, and page title extraction against `https://duckduckgo.com`, `https://example.com`, etc.

- [x] **GNOME Web (Epiphany) Native UI**:
  - Implemented with **GTK4** (`4.24.1`) and **Libadwaita** (`1.10.0`).
  - Epiphany header bar (`AdwHeaderBar`) with Back, Forward, Reload/Stop, and Home buttons.
  - Centered address bar (`EphyLocationEntry`) with smart search query / URL detection.
  - Interactive HTTPS security certificate popover showing connection security status.
  - Tab management via `AdwTabView` and `AdwTabBar`:
    - Multiple tabs support with dynamic title and favicon updates.
    - Tab switching (`Ctrl+Tab`, `Ctrl+PageDown`/`Up`, mouse click).
    - Tab closing (`Ctrl+W` / close button), new tab (`Ctrl+T` / '+' button).
  - Main application menu (`GMenu` / `GtkMenuButton`).

- [x] **Authentic GNOME Web (Epiphany) Interface Alignment**:
  - Replaced third-party custom styling with authentic upstream GNOME Web (Epiphany) Libadwaita layout:
    - Restored standard system window controls (Minimize, Maximize, Close on Windows top-right).
    - Upstream navigation cluster (`linked` Back, Forward, combined Stop/Reload, and Homepage buttons).
    - New Tab button placed at the start of the header bar (`tab-new-symbolic`).
    - Centered `AdwClamp` containing `EphyLocationEntry` with interactive security padlock, bookmark star, and progress bar.
    - End action cluster featuring Bookmarks (`user-bookmarks-symbolic`), Downloads (`folder-download-symbolic`), and main menu (`open-menu-symbolic`).
    - Standard Libadwaita `AdwTabBar` with autohide on single tab.
    - Official GNOME Web (Epiphany) Start Page featuring the official Epiphany SVG icon, GNOME blue theme, DuckDuckGo search, and community shortcuts.

- [x] **Browsing Features & Data Isolation**:
  - **Bookmarks**: Persistent storage in `%LOCALAPPDATA%\GNOME-Web\bookmarks.json` with preloaded default bookmarks (GNOME, WebKit, DuckDuckGo, Wikipedia, GitHub) and a Bookmarks manager dialog.
  - **History**: Persistent browsing history tracking in `%LOCALAPPDATA%\GNOME-Web\history.json` with timestamped records and History viewer dialog.
  - **Private Browsing / Incognito**: Dedicated private window mode (`--incognito` / `Ctrl+Shift+N`) utilizing `WKWebsiteDataStoreCreateNonPersistentDataStore()`. Zero cookies, cache, or history are written to disk.
  - **Preferences**: `AdwPreferencesWindow` allowing search engine selection (DuckDuckGo, Google, Bing, Ecosia), homepage configuration, color scheme (System / Light / Dark), and clearing browsing data / cache.
  - **Zooming**: Full page zoom controls (`Ctrl++`, `Ctrl+-`, `Ctrl+0`) via `WKPageSetPageZoomFactor`.

- [x] **Single-Click NSIS Installer & Portable Packaging**:
  - Single standalone NSIS installer `GNOME-Web-Setup-49.1.0.exe` (~87 MB).
  - Installs cleanly to `%LOCALAPPDATA%\Programs\GNOME Web` without requiring administrator privileges or MSYS2.
  - Creates Desktop shortcut, Start Menu shortcut, and standard Windows Add/Remove Programs registry entry with uninstaller.
  - Fully tested clean installation and silent uninstallation.
  - Portable ZIP distribution `GNOME-Web-portable-49.1.0.zip` (~90 MB) for zero-install usage.

- [x] **Automated GitHub Releases via CI**:
  - `.github/workflows/release.yml` configured to build, stage all DLLs, package the NSIS installer and portable ZIP, and publish releases on tag push (`v*`).

## Upstream GNOME Incompatibilities Handled
- **WebKitGTK on Windows**: WebKitGTK is strictly Linux/POSIX (relying on Unix domain sockets, bubblewrap, Wayland/X11). This was addressed by embedding the official Windows port of WebKit (**WinCairo**) via its native Win32/C API.
- **Linux-only Dependencies**: Epiphany's reliance on `gio-unix`, `gtk4-unix-print`, `libportal` (Flatpak portals), and `gcr-4`/`gck-2` (Linux PKCS#11 smartcard daemon) was decoupled into native Windows GLib/Win32 APIs and native WebKit data stores.
