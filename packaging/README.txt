GLAZE (vocal chain) + PUMP (sidechain) - VST3 + AU, Apple Silicon + Intel

INSTALL
  1. Right-click "Install Glaze.command" > Open > Open
     (macOS asks because the plugin isn't from the App Store / a paid developer ID.)
  2. Rescan plugins in your DAW.

  If macOS still refuses, open Terminal and paste:
     cd ~/Downloads/Glaze && xattr -dr com.apple.quarantine . && ./"Install Glaze.command"

  Manual install: copy Glaze.vst3 to ~/Library/Audio/Plug-Ins/VST3
                  copy Glaze.component to ~/Library/Audio/Plug-Ins/Components

Put it on the vocal track insert. Set KEY + SCALE on the home page, pick a preset, sing.

PUMP - sidechain ducker
  TEMPO mode: pumps in time with your project. No routing needed.
  SIDECHAIN mode: ducks whenever your kick hits. Send the kick to Pump's sidechain input:
    FL Studio: on the kick's mixer track, right-click the arrow to the track Pump is on > "Sidechain to this track",
               then in Pump's wrapper (gear icon) > Processing > set the sidechain input to that track.
    Logic: Pump's header > Side Chain > pick the kick track.
    Ableton: Pump's Sidechain section > Audio From > kick track.
  Drag the dots on the curve: bottom dot = depth, top dot = how long until it's fully back.
