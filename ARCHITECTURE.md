# Architecture

## Goal
Run **real GNOME Web (Epiphany 49.1 source)** on Windows with a real WebKit engine.

## Findings (verified)
- Epiphany needs GTK4 >= 4.13.3, libadwaita >= 1.8 and **WebKitGTK >= 2.43.4** (`webkitgtk-6.0`, plus
  `webkitgtk-web-process-extension-6.0`). It uses ~310 distinct WebKitGTK symbols in 45 source files.
- There is no Windows port of WebKitGTK and no MSYS2 package (only QtWebKit, which is excluded).
- MSYS2 UCRT64 provides GTK 4.24.1 and libadwaita 1.10.0. It lacks `libportal-gtk4` and `gcr-4`;
  `gtk4-unix-print`/`gio-unix` are Unix-only.
- **WebKit WinCairo** (Windows port of WebKit, WebKit2 multi-process) exists. Playwright publishes a prebuilt
  build; its `WebKit2.dll` exports the complete WebKit2 C API (`WKViewCreate`, `WKPageLoadURL`, non-persistent
  data stores, cookie store, ...). Verified by `dumpbin /exports`.

## Options
| | A. WebKitGTK on Windows | B. Epiphany + WebKit WinCairo shim (chosen) | C. New native frontend |
|---|---|---|---|
| Feasibility | Not feasible: GTK port needs Unix sockets/shm/bubblewrap/Wayland+X11; would be a new engine port | Feasible, large | Feasible, fastest |
| Engine | WebKitGTK | WebKit WinCairo (real WebKit, different port) | WebKit WinCairo |
| Source changes | Engine port | Reimplement the used part of `webkit/webkit.h` over the `WK*` C API; patch meson/Unix deps; port web extension | Not Epiphany code |
| Security | n/a | Depends on pinned third-party WinCairo build; Playwright's build is patched for automation | Same |
| Maintenance | Prohibitive | Shim must track Epiphany API use | Own codebase |
| Packaging | n/a | GTK runtime + WebKit DLLs + NSIS | Smaller |

## Decision
B, as selected by the project owner. Building WebKit WinCairo from source on GitHub Actions is possible
but does not remove the need for the shim (the shim is the missing piece, not the engine binary), so the
pinned Playwright build is used for now. Windows-specific code lives only in `shim/` (planned) behind the
WebKitGTK-shaped API so upstream files stay unmodified wherever possible.

## Status
Stage 1 (embedding in GTK4) is proven. The shim and Epiphany build are not done. See PORTING_STATUS.md.
