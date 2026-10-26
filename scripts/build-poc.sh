#!/usr/bin/env bash
# MSYS2 UCRT64 shell, repo root. Usage: scripts/build-poc.sh <WEBKIT_DIR in msys path>
set -e
WK="$1"
mkdir -p third_party/webkit-lib build/poc
(cd third_party/webkit-lib && gendef "$WK/WebKit2.dll" && dlltool -D WebKit2.dll -d WebKit2.def -l libWebKit2.dll.a)
bash poc/build.sh
