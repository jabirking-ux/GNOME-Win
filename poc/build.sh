#!/usr/bin/env bash
# Run from repo root inside an MSYS2 UCRT64 shell.
set -e
mkdir -p build/poc
gcc poc/embed.c -o build/poc/embed.exe \
  -Ithird_party/webkit-headers -Lthird_party/webkit-lib -lWebKit2 \
  $(pkg-config --cflags --libs gtk4)
echo built build/poc/embed.exe
