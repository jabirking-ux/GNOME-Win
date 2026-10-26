# Porting status

> This repository is **not yet GNOME Web on Windows.** It contains a verified engine-embedding proof of
> concept and the release pipeline. Do not describe the release artifacts as Epiphany.

## Done and verified (on the dev machine)
- [x] Toolchain: MSYS2 UCRT64 gcc 16, GTK 4.24.1, libadwaita 1.10.0 installed.
- [x] Upstream Epiphany 49.1 cloned to `upstream/` (not committed; fetched by script).
- [x] WebKit WinCairo C API headers vendored (`third_party/webkit-headers`, BSD) and import lib generated.
- [x] `poc/embed.c` compiles with MinGW, links `WebKit2.dll`, runs: WebKit web/network/GPU processes spawn and
      `https://example.com` loads over HTTPS (page title "Example Domain" read back via `WKPageCopyTitle`).

## Not verified
- [ ] Visual correctness of the HWND embedded in the GTK4 window (no desktop session for screenshots in the agent
      environment), resize/HiDPI behaviour, mixing MSVC-built WebKit DLLs with MinGW DLLs of the same name.
- [x] CI workflow builds and packages installer + ZIP (run 40358466950). Artifacts not yet installed/run; tag-release upload untested.
- [ ] Clean-machine install (needs the Visual C++ runtime DLLs; Playwright bundle ships the vcruntime/msvcp ones).

## Not started
- [ ] `shim/`: `WebKitWebView` & friends over `WK*` API (310 symbols to triage).
- [ ] Patching Epiphany meson for Windows (missing libportal, gcr-4, gtk4-unix-print, gio-unix, `/etc/os-release`).
- [ ] Web process extension (injected bundle) port.
- [ ] Safari theme (CSS for libadwaita, opt-out switch in preferences), downloads, private mode, permissions.

## Remaining upstream incompatibilities (known)
WebKitGTK API, web-process-extension API, libportal, gcr-4, Unix print, sandboxing, GStreamer media path.
