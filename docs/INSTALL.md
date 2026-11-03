# Installing YomiGo

## Windows: Desktop and HTPC

1. Download `YomiGo-Windows.zip` and unzip it anywhere you can write to, such as `C:\YomiGO\App`. Avoid `Program Files`, since saves are kept beside the program.
2. Run `YomiGo.exe` for the PC, or `YomiGoHTPC.exe` for the TV.

Saves go in a `saves` folder beside the program. Keep your own study files in a folder called `Material`, either beside the program or one level up, or at `C:\YomiGO\Material`. New files are announced on the home screen.

**Keyboard:** arrows move, Enter selects, Esc goes back. Ctrl+C copies and Ctrl+V pastes. On the TV app, Space means Start, and F3 means I don't know.

**Listening:** to hear Japanese on Windows, add a Japanese voice under Settings > Time & language > Speech, then restart YomiGo. Without one, classes skip listening. **Settings > Japanese voice** shows whether YomiGo found it. On Android, the same setting opens your phone's voice download page.

**Typing Japanese:** answer boxes convert romaji to kana for you. On Windows NT and later you can also use the Windows Japanese keyboard: Settings > Time & language > Language & region > Add a language > Japanese. Switch to it with Win+Space and press Alt+` for hiragana. Text you're composing shows underlined in the answer box. Press Enter to finish it, then Enter again to submit. Learner names need English letters for now.

### Updates

YomiGo checks once a day on Windows 7 and later, and always asks first. Updates come from this project's GitHub releases, and each file's signature is checked before it's used. The old program is kept as `.old`, and put back if the swap fails.

Older Windows can't make modern secure connections, so update by hand: download on another PC, then copy the files over.

### TV and CRT

Turn on **Settings > Display: CRT / RF TV** for a tube TV. It moves everything inside the visible picture, and holds white below peak so it doesn't bloom.

Software can't produce an RF signal. To reach an RF or composite TV you need hardware: an HDMI to composite converter, or a graphics card with analog output, followed by an RF modulator for RF-only sets. Set Windows to the lowest resolution your converter accepts.

### Controllers

Xbox pads use XInput. DualShock 4, USB N64 pads, and other DirectInput pads work through the Windows joystick API, and need to appear in Windows' Game Controllers panel. The USB N64 pad `0E6D:111D` is detected and set up for you. For another N64 replica, choose **Settings > Buttons: N64**, then use Rebind pad if any button is wrong.

On an N64 layout: A selects, B goes back, Start confirms, Z means I don't know, C-Up speaks, and L, R, C-Left, and C-Right change tabs. **Settings > Buttons** also shows N64, Xbox, or PlayStation prompts on any pad.

### Text Reader

On the PC, **Text** in the side menu reads Japanese and lets you click any word to look it up and add it to My words, with its sentence.

- **From the screen:** press Win+Shift+S, select some Japanese, then press **Screenshot**. Windows needs Japanese installed under Settings > Time & language > Language & region, including its optical character recognition. This uses Windows 10 or later, and `YomiGoOCR.exe` beside YomiGo.
- **From games:** run a text hooker such as Textractor, set it to copy text to the clipboard, and turn on **Watch**. New lines appear as the game shows them. **On top** keeps YomiGo above the game.
- **From anywhere:** copy Japanese, then press **Paste**.
- **From audio and video:** press **Transcribe** and pick a file, such as an MP3, M4A, WAV, or MP4. Other files, such as MKV, work when Windows can play their sound. YomiGo writes down the Japanese in it, then shows the lines here. It also saves Japanese subtitles beside the file as `name.ja.srt`, unless one is already there. VLC, mpv, and most players load them by themselves.

The first time, Transcribe asks before downloading its speech model: 574 MB from Hugging Face, checked against a fixed fingerprint, and kept in `models` beside YomiGo. Everything runs on your PC, and nothing is uploaded. It needs 64-bit Windows 10 or 11, a processor from about 2013 or later, and `YomiGoListen.exe` beside YomiGo. Expect it to take about half as long as the file plays, or longer on a slower PC. You can keep studying while it works.

### Talk

**Talk** in the side menu (on the TV, in the carousel) is open conversation with Sakura, a partner that runs on your PC. It opens at the Grammar Gate. She replies in Japanese at your level, about today's lesson. Under each reply you'll see an English translation, which **English** turns off, and, when she thinks you made a mistake, a suggestion marked **Maybe better**.

The first time, Talk asks before downloading its model: 5.2 GB from Hugging Face, checked against a fixed fingerprint, and kept in `models` beside YomiGo. It needs 64-bit Windows 10 or 11, a processor from about 2013 or later, about 6 GB of free memory, and `YomiGoTalk.exe` beside YomiGo. The first reply takes a little while as the model loads. After that, replies take a few seconds on a recent PC.

On the TV, type on the on-screen keyboard and press Start to send. X starts a new topic, and Y shows or hides English.

## Android

1. On your phone, download `YomiGo.apk` from the latest release.
2. Open it, and allow your browser to install apps when asked.

Under **More > Check for updates**, YomiGo fetches a new APK and Android asks you to confirm. Android doesn't allow silent installs for sideloaded apps. Google has announced developer verification for sideloaded apps on certified devices, rolling out worldwide from 2027, which may change this process.

## Browser extension

**Chrome or Edge:** unzip `YomiGoReader.zip`, open `chrome://extensions`, turn on Developer mode, choose Load unpacked, and pick the folder.

**Firefox:**

1. Download `YomiGoReader.zip` from the release, not the source code from the repo's main page, and unzip it.
2. Open `about:debugging#/runtime/this-firefox`.
3. Choose **Load Temporary Add-on**, and pick `manifest.json` in that folder.

Don't use **Install Add-on From File** on the Add-ons page. Firefox only accepts extensions signed by Mozilla there, and it reports an unsigned one as corrupt or unverified. A temporary add-on is removed when Firefox restarts, so you'd repeat these steps until Reader is signed. Signing is free, and needs a Mozilla account.

Select Japanese on any page to look it up, and save it with its sentence. To study those words in YomiGo, press **Copy for YomiGo** on the toolbar icon, then go to **Flashcards > My words > Import**. To use them in Anki, press **Export for Anki**.

On the PC, **Copy** on the My words page puts your whole list on the clipboard, ready to paste into a text file for Anki.

## Moving a save between devices

Windows saves are text files in `saves`. On Android, use **More > Show my save**, and copy the text. The format is the same everywhere. There's no automatic sync yet.

## Keep these safe

Release signing keys are kept in `%USERPROFILE%\.yomigo`, not in this project. Back that folder up privately. Without those keys, future Windows updates can't be signed, and Android updates would need a reinstall.
