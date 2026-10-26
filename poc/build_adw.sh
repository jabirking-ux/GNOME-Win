#!/usr/bin/env bash
set -e
export PATH="/ucrt64/bin:$PATH"
cd "$(dirname "$0")/.."
mkdir -p build
gcc poc/test_adw.c -o build/test_adw.exe $(pkg-config --cflags --libs libadwaita-1)
echo "Libadwaita build successful!"
