# Porting Status: GNOME Web (Epiphany) for Windows

## Executive Summary
A comprehensive, full-featured native Windows x64 port of **GNOME Web (Epiphany)** powered by **Apple WebKit (WinCairo WebKit2 multi-process architecture)** and **GTK4 / Libadwaita** has been successfully designed, implemented, verified, packaged, and published.

## Version 49.4.0: The Full Browser Port
In v49.4.0, GNOME Web on Windows transitions from an initial browser shell into a complete, full-fledged browser suite matching upstream Epiphany's core daily-driver capabilities:

1. **Find-in-Page Search Bar (`ephy-find-bar`)**:
   - Integrated floating find bar adhering to Libadwaita styling (`Ctrl+F`, `F3`, `Shift+F3`, `Esc`).
   - Bidirectional search navigation (Next / Previous match), Case-sensitivity toggle (`Aa`), and close button.
   - Wired directly into WebKit's native search API (`WKPageFindString`, `WKPageHideFindUI`).

2. **Downloads Manager & Interactive Popover (`ephy-downloads`)**:
   - Dedicated Downloads popover accessible directly from the header bar (`folder-download-symbolic`).
   - Displays real-time download items with filename, download path, and timestamp.
   - "Show in Folder" button triggers native Windows Explorer file selection (`explorer.exe /select,"path"`).
   - "Open File" button launches downloaded files via native Windows Shell API (`ShellExecuteW`).
   - "Clear Finished" button to purge completed download history.

3. **Session Restoration & Tab Stack (`ephy-session`)**:
   - Session persistence saving all active tabs, URLs, and titles to `%LOCALAPPDATA%\GNOME-Web\session.json`.
   - Automatic session restore on browser startup (configurable in Preferences).
   - Closed-tab stack supporting "Reopen Closed Tab" (`Ctrl+Shift+T`).

4. **Address Bar Auto-completion & Suggestions (`ephy-suggestions`)**:
   - Interactive dropdown popover anchored beneath the location entry while typing.
   - Aggregates multi-source suggestions: DuckDuckGo search query completion, matching bookmarks, and matching browsing history records.
   - Keyboard/mouse selection instantly initiates navigation.

5. **Bookmarks Manager with HTML Import & Export (`ephy-bookmarks`)**:
   - Full bookmarks dialog with live search filter, per-item deletion, and direct URL opening.
   - Netscape Bookmark HTML format export (`ephy_bookmarks_export_html`) to `Downloads/gnome_web_bookmarks.html`.
   - Netscape Bookmark HTML parser and import (`ephy_bookmarks_import_html`) for migrating bookmarks from Chrome, Firefox, Safari, or Edge.
   - Tagging and categories support.

6. **Browsing History Manager (`ephy-history`)**:
   - Full history viewer dialog with URL/title filtering, individual record deletion, and "Clear All History" action.
   - Timestamped records persisted safely in `%LOCALAPPDATA%\GNOME-Web\history.json`.

7. **Standard GNOME Keyboard Shortcuts Window (`ephy-shortcuts-dialog`)**:
   - Native `GtkShortcutsWindow` accessible via `Ctrl+?` or Application Menu -> Keyboard Shortcuts.
   - Comprehensive shortcut documentation across 6 groups:
     - Tabs & Windows (`Ctrl+T`, `Ctrl+W`, `Ctrl+N`, `Ctrl+Shift+N`, `Ctrl+Shift+T`, `Ctrl+Tab`)
     - Navigation (`Alt+Left`, `Alt+Right`, `Ctrl+R`, `F5`, `Alt+Home`)
     - Search & Location (`Ctrl+L`, `Alt+D`, `Ctrl+F`, `F3`, `Shift+F3`)
     - Bookmarks, History & Downloads (`Ctrl+B`, `Ctrl+D`, `Ctrl+H`, `Ctrl+Y`)
     - View & Zoom (`Ctrl++`, `Ctrl+-`, `Ctrl+0`, `F11`, `Ctrl+U`)
     - General (`Ctrl+,`, `Ctrl+?`, `Ctrl+Q`)

8. **View Source & HTML Rendering (`ephy-tab`)**:
   - Page source inspector (`Ctrl+U` or `view-source:URL`) rendering syntax-styled source code.
   - Native HTML document loading via `WKPageLoadHTMLString`.

9. **Enhanced GNOME Preferences (`ephy-preferences`)**:
   - Categorized settings window (`AdwPreferencesWindow`):
     - General: Appearance (System / Light / Dark), Search Engine selection, Startup & Session restoration toggle.
     - Downloads: Configurable download folder path.
     - Privacy & Security: Intelligent Tracking Prevention (ITP), WebKit Process Isolation model, and clear cache / cookies / browsing data.

10. **WebKit Multi-Process WinCairo Engine**:
    - Embedded 64-bit Apple WebKit (`WebKit2.dll`, `WebCore.dll`, `JavaScriptCore.dll`).
    - Multi-process isolation running `WebKitWebProcess.exe`, `WebKitNetworkProcess.exe`, and `WebKitGPUProcess.exe`.
    - Fully isolated non-persistent incognito session (`--incognito` / `Ctrl+Shift+N`).

## Packaging & Verification

- **NSIS Installer**: `dist/GNOME-Web-Setup-49.4.0.exe` (~87.6 MB)
- **Portable ZIP**: `dist/GNOME-Web-portable-49.4.0.zip` (~90.6 MB)
- Verified automated build scripts (`scripts/build-app.sh`, `scripts/build-installer.ps1`, `scripts/build-portable.ps1`).
- Automated releases published via GitHub Actions on git tag push.
