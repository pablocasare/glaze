#!/bin/bash
# Installs Glaze VST3 + AU for the current user and clears the download quarantine flag.
cd "$(dirname "$0")"
set -e
VST3="$HOME/Library/Audio/Plug-Ins/VST3"
AU="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$VST3" "$AU"
for P in Glaze Pump; do
  [ -d "$P.vst3" ] || continue
  rm -rf "$VST3/$P.vst3" "$AU/$P.component"
  cp -R "$P.vst3" "$VST3/"
  cp -R "$P.component" "$AU/"
  xattr -dr com.apple.quarantine "$VST3/$P.vst3" "$AU/$P.component" 2>/dev/null || true
  echo "  installed $P"
done
[ -d Glaze.app ] && { rm -rf /Applications/Glaze.app; cp -R Glaze.app /Applications/ && xattr -dr com.apple.quarantine /Applications/Glaze.app 2>/dev/null || true; }
killall -9 AudioComponentRegistrar 2>/dev/null || true
echo ""
echo "  Done."
echo "  VST3 -> $VST3"
echo "  AU   -> $AU"
echo "  Rescan plugins in your DAW (FL Studio: Options > Manage plugins > Find plugins)."
echo ""
