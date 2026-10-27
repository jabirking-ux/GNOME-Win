#!/usr/bin/env bash
set -e
export PATH="/ucrt64/bin:$PATH"
cd "$(dirname "$0")/.."

WK_DIR="${1:-/c/Users/User/AppData/Local/Temp/pw/browsers/webkit-2370}"

echo "Staging GNOME Web into dist/app from WebKit at $WK_DIR..."
rm -rf dist/app
mkdir -p dist/app

# 1. Main executable
cp build/gnome-web.exe dist/app/

# 2. WebKit WinCairo binaries and resources
cp -r "$WK_DIR"/* dist/app/

# 3. All DLL dependencies of gnome-web.exe from UCRT64
echo "Collecting DLL dependencies..."
ldd build/gnome-web.exe | grep ucrt64 | awk '{print $3}' | while read -r dll; do
  cp -n "$dll" dist/app/ 2>/dev/null || true
done

# Recursively collect DLLs of all copied DLLs
for i in 1 2 3; do
  for d in dist/app/*.dll; do
    ldd "$d" 2>/dev/null | grep ucrt64 | awk '{print $3}' | while read -r subdll; do
      cp -n "$subdll" dist/app/ 2>/dev/null || true
    done
  done
done

# 4. GLib schemas
mkdir -p dist/app/share/glib-2.0/schemas
cp -r /ucrt64/share/glib-2.0/schemas/* dist/app/share/glib-2.0/schemas/ 2>/dev/null || true
glib-compile-schemas dist/app/share/glib-2.0/schemas/ 2>/dev/null || true

# 5. GDK Pixbuf loaders and cache
mkdir -p dist/app/lib/gdk-pixbuf-2.0
cp -r /ucrt64/lib/gdk-pixbuf-2.0/* dist/app/lib/gdk-pixbuf-2.0/ 2>/dev/null || true
gdk-pixbuf-query-loaders > dist/app/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache 2>/dev/null || true

# 6. Icons
mkdir -p dist/app/share/icons
cp -r /ucrt64/share/icons/Adwaita dist/app/share/icons/ 2>/dev/null || true
cp -r /ucrt64/share/icons/hicolor dist/app/share/icons/ 2>/dev/null || true

# 7. Start Page assets
mkdir -p dist/app/startpage
cp -r data/startpage/* dist/app/startpage/ 2>/dev/null || true

echo "Staging complete! Total size:"
du -sh dist/app
