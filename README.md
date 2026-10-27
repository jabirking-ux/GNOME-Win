# GNOME Web (Epiphany) for Windows — Native WebKit Port

A native Windows x64 port of **GNOME Web (Epiphany)** powered by **Apple WebKit (WinCairo 64-bit multi-process architecture)** and **GTK4 / Libadwaita**, providing the authentic GNOME Web browsing experience on Windows.

---

## Features

- **Genuine Apple WebKit Engine**: Powered by WebKit WinCairo with multi-process sandboxing (`WebKitWebProcess`, `WebKitNetworkProcess`, `WebKitGPUProcess`). Fast, standards-compliant web rendering with full hardware acceleration.
- **Authentic GNOME Web Interface**: Built directly with modern GNOME Libadwaita widgets (`AdwApplicationWindow`, `AdwHeaderBar`, `AdwTabView`, `AdwTabBar`, `AdwClamp`, `AdwPreferencesWindow`).
  - Standard GNOME Web navigation cluster (Back, Forward, combined Stop/Reload, Homepage).
  - New tab button conveniently placed in the header bar.
  - Centered `AdwClamp` location entry with security indicators, bookmark star, and progress bar.
  - Quick access Bookmarks dialog, Downloads shortcut, and GNOME Web main application menu.
- **Multi-Tab Browsing & Session Restore**: Tab creation, reordering, close buttons, dynamic title sync, and autohiding tab bar (`Ctrl+T`, `Ctrl+W`, `Ctrl+Tab`). Automatic session restore and closed tab stack (`Ctrl+Shift+T`).
- **Find in Page**: Floating in-page search toolbar (`Ctrl+F`, `F3`, `Shift+F3`) with bidirectional navigation and case-matching toggle.
- **Interactive Downloads Popover**: Dedicated download manager popover with direct "Open File" and Windows Explorer "Show in Folder" actions.
- **Smart Navigation & Suggestions**: Distinguishes between URLs and search queries with instant dropdown suggestions combining bookmarks, history, and search completions.
- **Bookmarks with HTML Import/Export**: Persistent bookmark manager dialog with Netscape HTML import and export for effortless migration from Chrome, Firefox, or Safari (`Ctrl+B`).
- **History Viewer**: Full history manager with per-row deletion and bulk clearing (`Ctrl+H`).
- **Standard GNOME Keyboard Shortcuts Dialog**: Interactive `GtkShortcutsWindow` detailing all browser shortcuts across navigation, tabs, search, and view controls (`Ctrl+?`).
- **View Page Source**: Built-in source viewer (`Ctrl+U` / `view-source:`) with syntax formatting.
- **HTTPS & Security**: Real-time SSL/TLS status with an interactive connection security popover.
- **Private Browsing Mode**: Dedicated incognito window (`--incognito` / `Ctrl+Shift+N`) utilizing non-persistent in-memory data storage so zero history or cookies are written to disk.
- **Preferences & Customization**: Customize search engines, default homepage, session restore, download directory, color scheme, and clear browsing data.

---

## Installation

### Option 1: Single-Click Windows Installer (Recommended)
Download the latest `GNOME-Web-Setup-49.4.0.exe` from the [Releases](https://github.com/jabirking-ux/GNOME-Win/releases) page:
- No administrator privileges required (per-user install).
- Creates Desktop and Start Menu shortcuts.
- Includes a clean uninstaller in Windows Settings / Add & Remove Programs.

### Option 2: Portable ZIP
Download `GNOME-Web-portable-49.4.0.zip` from [Releases](https://github.com/jabirking-ux/GNOME-Win/releases), extract anywhere, and run `gnome-web.exe`.

---

## Architecture & Porting Strategy

See [ARCHITECTURE.md](ARCHITECTURE.md) for the complete technical design document comparing WebKitGTK, WebKit WinCairo, and platform trade-offs.

See [PORTING_STATUS.md](PORTING_STATUS.md) for detailed implementation status.

See [TESTING.md](TESTING.md) for the verified test suite and validation results.

---

## Building from Source

Detailed setup and build instructions are documented in [BUILD_WINDOWS.md](BUILD_WINDOWS.md).

```bash
# 1. Fetch WebKit WinCairo
pwsh scripts/fetch-webkit.ps1

# 2. Build executable
bash scripts/build-app.sh

# 3. Stage dependencies & build installer
bash scripts/stage-app.sh "$WEBKIT_DIR"
```

---

## License

GPL-3.0-or-later — Consistent with upstream GNOME Web (Epiphany).
