# Building on Windows
1. Install MSYS2 (`winget install MSYS2.MSYS2`), open the **UCRT64** shell.
2. `pacman -S mingw-w64-ucrt-x86_64-{gcc,gtk4,libadwaita,pkgconf,tools,nsis,meson,ninja}`
3. Install Node 20+ and run `pwsh scripts/fetch-webkit.ps1` (downloads Playwright's WebKit WinCairo build, prints `WEBKIT_DIR`).
4. `bash scripts/build-poc.sh /c/path/to/WEBKIT_DIR`
5. Run with `WEBKIT_DIR` and `C:\msys64\ucrt64\bin` on `PATH`: `build/poc/embed.exe`.

Releases: pushing a `v*` tag runs `.github/workflows/release.yml` (portable ZIP + NSIS installer, prerelease). Untested so far.
