#!/usr/bin/env bash
export PATH="/c/Users/User/AppData/Local/Temp/pw/browsers/webkit-2370:/ucrt64/bin:$PATH"
gdb --batch -ex r -ex "bt 20" /c/Users/User/Documents/GNOME-Win/build/gnome-web.exe
