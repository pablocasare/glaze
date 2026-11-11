UNDERTOW 2.0 - bassline generator by Token Wise
VST3 + AU instrument. Mac (Apple Silicon and Intel) and Windows (VST3, 64-bit).
Unsigned early-access build.

INSTALL (MAC)
  Easiest: double-click the .pkg installer and follow the prompts. It puts
  Undertow where every DAW looks (a password is normal; it installs for all users).

  Or, from the zip:
  1. Unzip it.
  2. Right-click "Install Undertow.command" and choose Open.
  If macOS says it can't be opened (macOS 15 Sequoia and later do this for
  unsigned downloads; Control-click > Open no longer skips the warning):
     System Settings > Privacy & Security > scroll down > Open Anyway > Open.
  Still stuck? Open Terminal, paste this, and press Return:
     cd ~/Downloads/Undertow && xattr -dr com.apple.quarantine . && ./"Install Undertow.command"

  Then:
  FL Studio: Options > Manage plugins > Find plugins. Look for Undertow.
  Ableton: Preferences > Plug-ins > Rescan. It shows up under Plug-ins > Token Wise.

INSTALL (WINDOWS)
  1. Unzip it and copy "Undertow.vst3" into  C:\Program Files\Common Files\VST3
  2. FL Studio: Options > Manage plugins > Find installed plugins. Ableton: rescan plug-ins.
  If Windows SmartScreen warns about the download, click More info > Run anyway / Keep.

USE
  Load Undertow as an instrument and press play (or hit the play button in the
  plugin to hear it before the DAW is running). KICK is a preview only: it plays
  while TRY is running, and it is left out of the host and out of bounces.

  1. Pick a STYLE. Each one is a different kind of bassline, and it sets a swing
     that fits (move Swing afterwards if you want your own). The tempo readout
     suggests a BPM; your DAW's tempo is what actually plays.
  2. Set KEY, OCTAVE, SCALE and BARS. Scales: Minor, Dorian, Phrygian,
     Harmonic Minor, Major, Minor Pentatonic, Mixolydian.
  3. GENERATE is a new bassline. VARY keeps the idea and changes a few notes.
     UNDO / REDO if you lose one you liked.
     LOCK RHYTHM keeps the groove and writes new notes.
     LOCK PITCH keeps the pitch roles and writes a new rhythm.
  4. SHAPE
       Density    how many notes. Every part of the knob adds or removes hits.
       Movement   how far it leaves the root
       Variation  0 = the hook loops, higher = more call and response, 100% = a new turn every bar
       Length     short and plucky, or held
  5. FEEL: Swing and Glide. Ghosts are the quiet pickups. Accent is how hard the
     offbeat hits.
  6. CHORDS moves the root each bar (Auto, stay on i, i-VI, i-VII, i-iv, and longer loops).
     The interval is the natural-minor one in every scale, including Minor Pentatonic.
     MIDI KEY is off by default. Turn it on and a note-on sets the root inside the
     plugin. It does not write the Key parameter, so it will not chase its own MIDI.
     PLAY MODE: Always (with the host), Note Held (the pattern plays only while a
     note is held, and the lowest note sets the root), or Synth (Undertow plays
     the MIDI you send it, no pattern).
     KICK GAP (default 12 ms) shortens holds so a note is silent just before the
     kick, in the synth and in exported MIDI.
  7. SOUND: Sub, Rubber, Reese, Organ, Acid, Growl. Tone, Drive, Volume.
     EVOLVE plays a new answer every time the loop comes around, counted from the
     host's position, so a bounce matches what you heard. The hook stays. The saved
     take does not change.

  THE PIANO ROLL
    CLICK add a note. DRAG move it. EDGE resize it. RIGHT-CLICK or DOUBLE-CLICK delete.
    ALT-CLICK toggles a slide. Drag in the VEL lane to change velocity.
    Wheel scrolls. Ctrl-wheel (cmd-wheel) zooms to 2x or 4x, which is how an
    8- or 16-bar roll stays editable.
    The dots above the grid are the kick for this style, so you can see the bass
    sitting in the pocket. Chord names sit on top. EDITED means you changed it by hand;
    those edits are saved with the project. The notes themselves are always saved,
    so reopening a session after an update plays the same bassline. CLEAR wipes the roll.

  IDEAS
    Eight slots under the roll. Click an empty slot to keep the current bassline,
    click it again to bring it back. Right-click a slot to replace or clear it.
    Slots are saved with the project.

  PRESETS
    Eighteen factory presets. Click the name, or use the arrows. A * means you've
    changed it. Save preset... stores yours (rename and delete live in that menu)
    under Token Wise/Undertow in your app-data folder.

  GET THE MIDI
    Drag the DRAG MIDI tile onto a clip in your DAW, or hit EXPORT.
    With EVOLVE on, the file is several takes back to back.
    Undertow also sends MIDI out while it plays, for Serpent or any other synth.
    Slides overlap the note they leave, so a mono synth set to glide will glide.
    Swing is in the file.

    Ableton: drop the tile on a MIDI track. To play another synth live, set the
    Undertow track's "MIDI To" to that track, and set the synth track's Monitor to In.
    FL Studio: drop the tile on the Playlist. For live MIDI, open the wrapper settings
    (the gear), set a MIDI output port, and set the other plugin's input port to match.

STYLES
  ROLLING        16ths rolling between the kicks. Tech house, around 126 BPM.
  BOUNCY         short notes, octave jumps.
  OFFBEAT        the classic house "&", held. Pickups show up as you raise Density.
  2-STEP         swung UK garage, long notes and slides. Try 132 BPM, swing around 60%.
  MINIMAL        a short hook that loops. Hypnotic at low Variation.
  DRIVING        16ths between the kicks. Low Density thins it to the offbeat.
  BROKEN         syncopated, with a broken kick in the preview.
  ACID           303-style 16ths, rests, accents and slides.
  SPEED GARAGE   held octave wobble, made for the Reese. Try 136 BPM.
  AFRO           tresillo 3-3-2. The 5th answers the root.
  DEEP TECH      long offbeat subs that slide. Try 125 BPM.
  BASSLINE       bouncy offbeat octaves. Try 138 BPM.

The bassline is saved with your project as the actual notes, so reopening the
session brings the same one back even after an update. A generator control
(or GENERATE / VARY) is what rolls a new one.

Fonts: Inter and Space Mono, SIL Open Font License 1.1 (OFL-Inter.txt, OFL-SpaceMono.txt).
