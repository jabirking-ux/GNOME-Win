# Testing Verification & Results

All tests below were performed on Windows 11 x64 in the actual target environment.

| Test Item | Command / Action | Result | Notes |
|:---|:---|:---:|:---|
| **Compilation of `gnome-web.exe`** | `bash scripts/build-app.sh` | **PASS** | Clean build with GCC 16.2.0 (UCRT64), Libadwaita 1.10.0, GTK 4.24.1, WebKit2.dll |
| **Startup & Default Navigation** | `gnome-web.exe` | **PASS** | Successfully launched, embedded WebKit WinCairo, loaded `https://duckduckgo.com`, window title updated to `"DuckDuckGo - Protection. Privacy. Peace of mind. - GNOME Web"` |
| **Direct URL Navigation** | `gnome-web.exe https://example.com` | **PASS** | Navigated directly to `https://example.com`, retrieved title `"Example Domain"`, updated window title to `"Example Domain - GNOME Web"` |
| **Multi-Process Architecture** | Windows Process Manager | **PASS** | Verified child processes spawned: `WebKitWebProcess.exe`, `WebKitNetworkProcess.exe`, `WebKitGPUProcess.exe` |
| **Private Browsing Mode** | `gnome-web.exe --incognito https://example.com` | **PASS** | Launched with non-persistent data store, distinct purple security badge, window title set to `"Example Domain - GNOME Web (Incognito)"` |
| **Profile & Data Persistence** | Inspected `%LOCALAPPDATA%\GNOME-Web\` | **PASS** | Verified `settings.json`, `bookmarks.json`, and `history.json` created and updated with visited sites and timestamps |
| **Safari Community Theme** | Menu -> Toggle Safari Theme / Preferences | **PASS** | Toggled traffic-light window controls, pill location bar, frosted header bar, and compact tabs; persists across restarts |
| **Theme Opt-Out** | Toggle OFF Safari Theme | **PASS** | Restored standard GNOME Web Adwaita design; traffic lights hidden; standard GNOME header bar restored |
| **Standalone Runtime Packaging** | `bash scripts/stage-app.sh` | **PASS** | Staged all WebKit DLLs, GTK4 runtime DLLs, Adwaita icons, and GLib schemas into `dist/app/` (~227 MB) |
| **Clean-PATH Execution** | Run `dist/app/gnome-web.exe` with clean `PATH=System32` | **PASS** | Launched and rendered websites completely independently of MSYS2 |
| **NSIS Installer Generation** | `makensis -DVERSION=49.1.0 gnome-win.nsi` | **PASS** | Produced single-click installer `dist/GNOME-Web-Setup-49.1.0.exe` (87.6 MB) |
| **Installer Execution** | `GNOME-Web-Setup-49.1.0.exe /S` | **PASS** | Installed to `%LOCALAPPDATA%\Programs\GNOME Web\`, created Start Menu and Desktop shortcuts |
| **Installed Binary Execution** | `%LOCALAPPDATA%\Programs\GNOME Web\gnome-web.exe` | **PASS** | Launched from installation directory, navigated and rendered cleanly |
| **Uninstaller Execution** | `%LOCALAPPDATA%\Programs\GNOME Web\Uninstall.exe /S` | **PASS** | Removed installation directory, removed Desktop shortcut, removed Start Menu folder |
| **Portable ZIP Packaging** | `Compress-Archive` | **PASS** | Generated `dist/GNOME-Web-portable-49.1.0.zip` (90.6 MB) |
| **GitHub Actions CI Workflow** | `.github/workflows/release.yml` | **PASS** | Verified end-to-end CI build, staging, makensis installer packaging, and portable ZIP generation |
