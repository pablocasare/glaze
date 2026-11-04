#!/bin/bash
# Installs Undertow (VST3 + AU instrument) for the current user and clears the download quarantine flag.
cd "$(dirname "$0")"
set -e
VST3="$HOME/Library/Audio/Plug-Ins/VST3"
AU="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$VST3" "$AU"
rm -rf "$VST3/Undertow.vst3" "$AU/Undertow.component"
cp -R Undertow.vst3 "$VST3/"
cp -R Undertow.component "$AU/"
xattr -dr com.apple.quarantine "$VST3/Undertow.vst3" "$AU/Undertow.component" 2>/dev/null || true
killall -9 AudioComponentRegistrar 2>/dev/null || true
echo ""
echo "  Undertow installed."
echo "  In FL Studio: Options > Manage plugins > Find plugins, then look for Undertow."
echo ""
