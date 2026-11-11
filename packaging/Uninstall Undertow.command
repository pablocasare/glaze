#!/bin/bash
# Removes every copy of Undertow (VST3 + AU) from this Mac: the per-user install from
# "Install Undertow.command" and the all-users install from the .pkg.
# Your projects, presets and exported MIDI are not touched.

echo ""
echo "  This removes the Undertow plugin (VST3 + AU) from this Mac."
echo "  Your FL Studio / Ableton projects and saved presets are kept."
echo ""
read -r -p "  Remove Undertow? [y/N] " ok
case "$ok" in y|Y|yes|YES) ;; *) echo "  Nothing removed."; exit 0 ;; esac

found=0
for d in "$HOME/Library/Audio/Plug-Ins/VST3/Undertow.vst3" "$HOME/Library/Audio/Plug-Ins/Components/Undertow.component"; do
  if [ -e "$d" ]; then rm -rf "$d" && echo "  removed $d"; found=1; fi
done

SYS=()
for d in "/Library/Audio/Plug-Ins/VST3/Undertow.vst3" "/Library/Audio/Plug-Ins/Components/Undertow.component"; do
  [ -e "$d" ] && SYS+=("$d")
done
if [ ${#SYS[@]} -gt 0 ]; then
  echo ""
  echo "  The installer put a copy in /Library for all users. macOS needs your Mac password to remove it."
  sudo rm -rf "${SYS[@]}" && for d in "${SYS[@]}"; do echo "  removed $d"; done
  sudo pkgutil --forget com.tokenwise.undertow.pkg >/dev/null 2>&1 || true
  found=1
fi

# Refresh macOS's Audio Unit list so hosts stop showing the old AU.
killall -9 AudioComponentRegistrar 2>/dev/null || true

echo ""
if [ $found -eq 1 ]; then
  echo "  Undertow is uninstalled. Rescan plugins in your DAW."
else
  echo "  Undertow was not installed, nothing to remove."
fi
echo "  (Your saved presets stay in ~/Library/Token Wise/Undertow. Delete that folder too if you want them gone.)"
echo ""
