#!/usr/bin/env bash
set -e
export PATH="/ucrt64/bin:$PATH"
cd "$(dirname "$0")/.."
mkdir -p build/poc
gcc poc/test_multitab.c -o build/poc/test_multitab.exe \
  -Ithird_party/webkit-headers -Lthird_party/webkit-lib -lWebKit2 \
  $(pkg-config --cflags --libs libadwaita-1)
echo "Multitab build successful!"
