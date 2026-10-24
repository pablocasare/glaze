LIFTOFF - buildup plugin (VST3 + AU, Apple Silicon + Intel)

INSTALL
  1. Right-click "Install Liftoff.command" > Open > Open
  2. FL Studio: Options > Manage plugins > Find plugins.
  If macOS refuses, open Terminal and paste:
     cd ~/Downloads/Liftoff && xattr -dr com.apple.quarantine . && ./"Install Liftoff.command"

USE
  Put Liftoff on the track (or bus / master) you want to build up.
  Turn up BUILD over the 4-8 bars before your drop, then snap it back to 0 on the drop:
    low cut sweeps up (+ resonance), delay + reverb grow, white-noise riser rises.
    At 0% it's completely clean, and the tails are cut the moment you drop so it hits dry.

  EASIEST - AUTO BUILD (no automation needed):
    Turn on AUTO BUILD, set "drop at bar" to the bar your drop starts on, pick how long
    the build is (e.g. 8 bars). Press play: it builds over those bars and drops on that bar.

  Or automate it in FL Studio: drag the BUILD dial once, then
  Tools > Last tweaked > Create automation clip (or right-click the dial).
  Or record it: arm recording with Automation on, press play, hold HOLD TO BUILD, let go on the drop.

  Quick test without automation: hold the HOLD TO BUILD button, let go to drop.
  The cards switch each effect on/off (click the dot) and set how much BUILD affects it.
