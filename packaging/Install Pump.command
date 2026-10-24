#!/bin/bash
# Installs Pump (VST3 + AU) for the current user and clears the download quarantine flag.
cd "$(dirname "$0")"
set -e
VST3="$HOME/Library/Audio/Plug-Ins/VST3"
AU="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$VST3" "$AU"
rm -rf "$VST3/Pump.vst3" "$AU/Pump.component"
cp -R Pump.vst3 "$VST3/"
cp -R Pump.component "$AU/"
xattr -dr com.apple.quarantine "$VST3/Pump.vst3" "$AU/Pump.component" 2>/dev/null || true
killall -9 AudioComponentRegistrar 2>/dev/null || true
echo ""
echo "  Pump installed."
echo "  VST3 -> $VST3"
echo "  AU   -> $AU"
echo "  In FL Studio: Options > Manage plugins > Find plugins, then look for Pump."
echo ""
