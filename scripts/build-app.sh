#!/usr/bin/env bash
set -e
export PATH="/ucrt64/bin:$PATH"
cd "$(dirname "$0")/.."
mkdir -p build

echo "Compiling GNOME Web for Windows..."
gcc -O2 -Wall \
  src/main.c \
  src/ephy-settings.c \
  src/ephy-theme.c \
  src/ephy-bookmarks.c \
  src/ephy-history.c \
  src/ephy-engine.c \
  src/ephy-tab.c \
  src/ephy-window.c \
  src/ephy-preferences.c \
  src/ephy-security.c \
  -o build/gnome-web.exe \
  -Ithird_party/webkit-headers \
  -Lthird_party/webkit-lib -lWebKit2 \
  $(pkg-config --cflags --libs libadwaita-1 json-glib-1.0 gtk4) \
  -mwindows

echo "Build successful: build/gnome-web.exe"
