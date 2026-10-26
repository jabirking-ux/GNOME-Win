# Fetches the pinned Playwright WebKit WinCairo build (real WebKit, Windows port) and
# generates the MinGW import library for WebKit2.dll.  Run from repo root.
param([string]$Out = "$PSScriptRoot\..\build\webkit")
$ErrorActionPreference = 'Stop'
$Out = [IO.Path]::GetFullPath($Out); New-Item -Force -ItemType Directory $Out | Out-Null
$env:PLAYWRIGHT_BROWSERS_PATH = "$Out\pw"
Push-Location $Out; if (-not (Test-Path package.json)) { npm init -y | Out-Null }
npm i playwright-core@1.64.1 | Out-Null
npx playwright-core install webkit
Pop-Location
$wk = (Get-ChildItem "$Out\pw" -Directory -Filter 'webkit-*' | Select-Object -First 1).FullName
Write-Host "WEBKIT_DIR=$wk"
if ($env:GITHUB_ENV) { "WEBKIT_DIR=$wk" | Out-File -Append -Encoding utf8 $env:GITHUB_ENV }
