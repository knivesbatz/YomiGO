# Installing YomiGo

## Windows: Desktop and HTPC

1. Download `YomiGo-Windows.zip` and unzip it anywhere you can write to, such as `C:\YomiGO\App`. Avoid `Program Files`, since saves are kept beside the program.
2. Run `YomiGo.exe` for the PC, or `YomiGoHTPC.exe` for the TV.

Saves go in a `saves` folder beside the program. Keep your own study files in a folder called `Material`, either beside the program or one level up, or at `C:\YomiGO\Material`. New files are announced on the home screen.

**Keyboard:** arrows move, Enter selects, Esc goes back. On the TV app, Space means Start and F1 means speak.

**Listening:** to hear Japanese on Windows, add a Japanese voice under Settings > Time & language > Speech. Without one, YomiGo hides the speech options.

**Typing Japanese:** answer boxes convert romaji to kana for you. On Windows NT and later you can also use the Windows Japanese keyboard: Settings > Time & language > Language & region > Add a language > Japanese.

### Updates

YomiGo checks once a day on Windows 7 and later, and always asks first. Updates come from this project's GitHub releases, and each file's signature is checked before it's used. The old program is kept as `.old`, and put back if the swap fails.

Older Windows can't make modern secure connections, so update by hand: download on another PC, then copy the files over.

### TV and CRT

Turn on **Settings > Display: CRT / RF TV** for a tube TV. It moves everything inside the visible picture, and holds white below peak so it doesn't bloom.

Software can't produce an RF signal. To reach an RF or composite TV you need hardware: an HDMI to composite converter, or a graphics card with analog output, followed by an RF modulator for RF-only sets. Set Windows to the lowest resolution your converter accepts.

### Controllers

Xbox pads use XInput. DualShock 4 and other DirectInput pads work through the Windows joystick API, and need to appear in Windows' Game Controllers panel. Rebind under Settings on the TV app. Prompts change to match the pad YomiGo detects.

## Android

1. On your phone, download `YomiGo.apk` from the latest release.
2. Open it, and allow your browser to install apps when asked.

Under **More > Check for updates**, YomiGo fetches a new APK and Android asks you to confirm. Android doesn't allow silent installs for sideloaded apps. Google has announced developer verification for sideloaded apps on certified devices, rolling out worldwide from 2027, which may change this process.

## Browser extension

**Chrome or Edge:** unzip `YomiGoReader.zip`, open `chrome://extensions`, turn on Developer mode, choose Load unpacked, and pick the folder.

**Firefox:** open `about:debugging#/runtime/this-firefox`, choose Load Temporary Add-on, and pick `manifest.json`. Firefox removes it on restart until it's signed through Mozilla, so that needs a Mozilla account. It's a limit of Firefox, listed in [known issues](KNOWN_ISSUES.md).

Select Japanese on any page to look it up. Export saved words from the toolbar icon as a `.tsv` file, which Anki can import.

## Moving a save between devices

Windows saves are text files in `saves`. On Android, use **More > Show my save**, and copy the text. The format is the same everywhere. There's no automatic sync yet.

## Keep these safe

Release signing keys are kept in `%USERPROFILE%\.yomigo`, not in this project. Back that folder up privately. Without those keys, future Windows updates can't be signed, and Android updates would need a reinstall.
