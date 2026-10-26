# Testing (actual results only)
| Test | Result |
|---|---|
| MinGW build of poc/embed.c | Pass (2026-10-26) |
| Launch, WebKit processes spawn | Pass |
| Load https://example.com, title read back | Pass (`Example Domain`, 16 s stable) |
| Visual rendering / screenshot | Not tested |
| CI (windows-latest): fetch WebKit, build, stage, makensis installer, ZIP | Pass (run 40358466950; first run failed on missing 7z, fixed) |`n| Installer/ZIP executed on a clean Windows machine | Not tested |`n| Release upload on tag | Not tested (skipped on manual run) |
| Tabs, downloads, private mode, themes, HiDPI | Not implemented |
