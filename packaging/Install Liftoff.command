#!/bin/bash
# Installs Liftoff (VST3 + AU) for the current user and clears the download quarantine flag.
cd "$(dirname "$0")"
set -e
VST3="$HOME/Library/Audio/Plug-Ins/VST3"
AU="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$VST3" "$AU"
rm -rf "$VST3/Liftoff.vst3" "$AU/Liftoff.component"
cp -R Liftoff.vst3 "$VST3/"
cp -R Liftoff.component "$AU/"
xattr -dr com.apple.quarantine "$VST3/Liftoff.vst3" "$AU/Liftoff.component" 2>/dev/null || true
killall -9 AudioComponentRegistrar 2>/dev/null || true
echo ""
echo "  Liftoff installed."
echo "  In FL Studio: Options > Manage plugins > Find plugins, then look for Liftoff."
echo ""
