PUMP - sidechain ducker (VST3 + AU, Apple Silicon + Intel)

INSTALL
  1. Right-click "Install Pump.command" > Open > Open
  2. FL Studio: Options > Manage plugins > Find plugins.

  If macOS refuses, open Terminal and paste:
     cd ~/Downloads/Pump && xattr -dr com.apple.quarantine . && ./"Install Pump.command"

USE
  TEMPO mode: put Pump on the bass/pad/beat and press play. It ducks on every beat. No routing.
  SIDECHAIN mode (follows your real kick), FL Studio:
    1. Kick on its own mixer track (e.g. 1), bass on another (e.g. 2).
    2. Select the kick's mixer track, right-click the arrow under the bass track > "Sidechain to this track".
    3. Load Pump on the bass track.
    4. Pump's gear icon > Processing tab > set Pump's sidechain input to the kick's track.
    5. Click SIDECHAIN in Pump. The purple meter on the right should jump with each kick.
       Turn Threshold down if it doesn't duck on every kick.
