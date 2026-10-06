VOODOO VINCE REMASTERED - FREE CAMERA  (v1.0, for the Steam version 1.14.2.0)
==========================================================================

Fly the camera anywhere, including during cutscenes.

INSTALL
  1. Extract this zip anywhere.
  2. Double-click Install.bat.
     It finds the game through Steam automatically (any drive or library).
     If it cannot, it asks you to pick the folder that contains Vince.exe.
  3. Start the game, load a level, press F5.

  Manual install: copy files\XINPUT9_1_0.dll and files\freecam.ini next to
  Vince.exe (usually C:\Program Files (x86)\Steam\steamapps\common\
  Voodoo Vince Remastered).

UNINSTALL
  Double-click Uninstall.bat, or delete XINPUT9_1_0.dll from the game folder.
  The game's own files are never modified on disk.

CONTROLS
  Toggle freecam      F5                        Click both sticks (L3+R3)
  Move                W A S D                   Left stick
  Up / down           E or Space / Q            Right trigger / Left trigger
  Look                Hold right mouse, arrows  Right stick
  Fast / slow         Shift / Ctrl              Hold Y / hold X
  Zoom in / out       Z / X  (R resets)         RB / LB

  Turn it on mid-cutscene and the camera starts exactly where the cutscene
  camera was while the scene keeps playing. While flying, controller face
  buttons are hidden from the game so you cannot skip a cutscene by
  accident (Start and Back still work). Vince stops responding to input
  while you fly during gameplay.

SETTINGS  (freecam.ini, next to Vince.exe)
  toggle_key         key code for the toggle (0x74 = F5, 0x75 = F6, ...)
  move_speed         flying speed (default 1.5)
  mouse_sensitivity  default 0.0025
  pad_look_speed / key_look_speed
  invert_x / invert_y   1 to invert
  freeze_vince       0 to let Vince keep moving while you fly

TROUBLESHOOTING
  Check freecam.log next to Vince.exe. It should say "hooks installed".
  "unsupported Vince.exe version" means your game build differs from
  Steam 1.14.2.0; the mod switches itself off safely and does nothing.
  Some antivirus tools flag any DLL that modifies a game in memory; the
  full source code is in the source folder.

HOW IT WORKS
  The game still contains the developers' unused debug camera (DEBUGCAM).
  This mod loads as a stand-in for XInput (it passes all controller calls
  through to the real Windows XInput), switches that debug camera on, and
  steers it with your input.

  Source: source\freecam.c - build with source/build.sh (clang + lld).
  Unofficial fan mod, not affiliated with Microsoft or Beep Industries.
