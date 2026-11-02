# Compatibility

| Platform | Status | What was actually checked |
|---|---|---|
| Windows 11 | Tested | Whole course played through the screens with no window. Both programs also opened in real windows at 640×480, took keyboard and Japanese text, and closed cleanly. A real update from GitHub worked. Mouse use and long sessions need a person |
| Windows 10, 8.1, 8, 7 | Expected to work | Same program. Not run on these versions |
| Windows Vista, XP | Expected to work, manual updates | Not run. The updater won't run; the app says so |
| Windows 98, 95 | Unproven | The program header and imports pass an automated Windows 95 check. **It has never been run on real 95 or 98**, as no VM is available here. No updates; no speech |
| Android 8 and later | Unproven until a device test | UI tested in a browser at phone width. CI builds the APK. Speech and updates need a real phone |
| Chrome, Edge | Unproven until installed | Reader's logic tested on a page with stand-in storage. Not yet loaded as an extension |
| Firefox 115 and later | Temporary install only | As Chrome, and needs Mozilla signing to stay installed |
| HDTV | Expected to work | The stretched output is simple, but hasn't been viewed on a TV |
| CRT | Unproven | Margins and white limit are in place. Not seen on a tube. RF needs external hardware |

## Windows 95 notes

- **Needs:** a 486 or better, and any 32-bit-capable display. Colours are reduced in 256-colour mode.
- **Japanese:** works with no Japanese fonts or language pack, since YomiGo draws its own type. Answers use the built-in romaji to kana conversion, because the Windows IME path needs NT.
- **Controllers:** the standard joystick API exists on 95, but XInput doesn't.
- **File names:** Japanese file names in Material only work on NT.

## How to test on an old Windows

Use 86Box, VirtualBox, or real hardware, with your own Windows licence. Copy `YomiGo.exe`, then run it. Please report any error dialog word for word: a missing function name tells us exactly what to fix.
