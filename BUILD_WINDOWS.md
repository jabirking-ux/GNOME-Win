# Building GNOME Web for Windows

This guide explains how to compile and package GNOME Web (Epiphany for Windows) from source.

## Prerequisites

1. **Windows 10/11 x64**
2. **MSYS2**: Install from [msys2.org](https://www.msys2.org/) or via `winget install MSYS2.MSYS2`.
3. **Node.js**: Node 20+ (for downloading the pinned WebKit WinCairo binaries).
4. **Git**: Installed and available in PATH.

---

## 1. Install Build Dependencies

Open the **MSYS2 UCRT64** terminal and install the required compiler and libraries:

```bash
pacman -S --needed \
  mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-gtk4 \
  mingw-w64-ucrt-x86_64-libadwaita \
  mingw-w64-ucrt-x86_64-json-glib \
  mingw-w64-ucrt-x86_64-pkgconf \
  mingw-w64-ucrt-x86_64-tools \
  mingw-w64-ucrt-x86_64-nsis \
  mingw-w64-ucrt-x86_64-adwaita-icon-theme
```

---

## 2. Fetch WebKit WinCairo Binaries & Generate Import Library

In PowerShell from the repository root:

```powershell
pwsh scripts/fetch-webkit.ps1
```

Then in the MSYS2 UCRT64 terminal, generate the import library for MinGW GCC:

```bash
mkdir -p third_party/webkit-lib
cd third_party/webkit-lib
gendef "$WEBKIT_DIR/WebKit2.dll"
dlltool -D WebKit2.dll -d WebKit2.def -l libWebKit2.dll.a
cd ../..
```

---

## 3. Compile GNOME Web

Run the build script in MSYS2 UCRT64:

```bash
bash scripts/build-app.sh
```

This compiles `src/*.c` with optimizations into `build/gnome-web.exe`.

---

## 4. Stage Application & Dependencies

To package all required DLLs, icons, and GLib schemas into a self-contained folder:

```bash
bash scripts/stage-app.sh "$WEBKIT_DIR"
```

The output will be placed in `dist/app/` (~227 MB uncompressed).

---

## 5. Build NSIS Installer & Portable ZIP

To generate the single-click NSIS installer:

```bash
cd installer
makensis -DVERSION=49.1.0 gnome-win.nsi
cd ..
```

To create the portable ZIP:

```powershell
Compress-Archive -Path dist/app/* -DestinationPath "dist/GNOME-Web-portable-49.1.0.zip" -Force
```

The generated files in `dist/`:
- `GNOME-Web-Setup-49.1.0.exe` (~87 MB single-click installer)
- `GNOME-Web-portable-49.1.0.zip` (~90 MB standalone portable package)
