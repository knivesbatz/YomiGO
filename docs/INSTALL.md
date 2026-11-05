# Installing YomiGo

## Mac

1. Download `YomiGo-macOS.dmg` from the [latest release](https://github.com/knivesbatz/YomiGO/releases/latest), open it, and drag **YomiGo** to **Applications**. It runs on Apple silicon and Intel Macs, on macOS 11 Big Sur or later.
2. The first time, macOS checks who made it. YomiGo isn't registered with Apple, so open it like this once:
   - **macOS 11 to 14:** in Applications, Control-click YomiGo, choose **Open**, then **Open** again.
   - **macOS 15 and later:** open YomiGo, close the warning, then go to **System Settings > Privacy & Security** and choose **Open Anyway** next to YomiGo.
   - If macOS says the app is damaged, open Terminal and run `xattr -cr /Applications/YomiGo.app`, then open it again. Nothing is wrong with it: that removes the download mark macOS adds.

Saves are kept in `~/Library/Application Support/YomiGo/saves`. Your own study files can go in a folder named `YomiGO/Material` in your home folder; they show under **Library > Material**.

**Keyboard:** ⌘1 to ⌘7 open each place, ⌘F the dictionary, ⌘, Settings, and Esc goes back.

**Listening:** add a Japanese voice in **System Preferences > Accessibility > Spoken Content > System Voice > Manage Voices**, such as Kyoko, then restart YomiGo.

**Updates:** YomiGo tells you when a new version is out. **Settings > Check for updates** opens the download page; install it the same way, replacing the old app.

**Not on the Mac yet:** Talk with Sakura, reading screenshots, and transcription. They use Windows programs.

## Windows: Desktop and HTPC

1. Download `YomiGo-Setup.exe` for the PC, or `YomiGo-TV-Setup.exe` for a TV with a controller, from the [latest release](https://github.com/knivesbatz/YomiGO/releases/latest).
2. Open it and follow the steps. It installs for you only, with no administrator, into `%LOCALAPPDATA%\Programs\YomiGo`, with a Start menu entry and, if you like, a desktop shortcut. Install both and they share one folder and the same learners.
3. To remove it, use **Settings > Apps**, or **Uninstall YomiGo** in its folder. Your saves stay unless you untick that.

Prefer no installer? `YomiGo-Windows.zip` still works: unzip it anywhere you can write to, such as `C:\YomiGO\App`, and run `YomiGo.exe`, or `YomiGoHTPC.exe` for the TV. They're the same program; the TV name opens it full screen with the TV menu. Avoid `Program Files`, since saves are kept beside the program.

YomiGo for Windows needs Windows 10 or 11. It uses the Edge WebView2 runtime, which both already have.

Saves go in a `saves` folder beside the program. Keep your own study files in a folder called `Material`, either beside the program or one level up, or at `C:\YomiGO\Material`. They're listed under **Library > Material**, and an immersion task offers a matching file.

**Keyboard:** Ctrl+1 to Ctrl+8 open each place in the sidebar, Ctrl+F the dictionary, and Ctrl+, Settings. Esc goes back. In a quiz, 1 to 4 pick an answer and Enter goes on. Right-click a dictionary result, one of your words, or a Text Reader line for more. In the TV app, the arrows move, Enter chooses, Esc or Backspace goes back, and Page Up and Page Down flip between places.

**Listening:** to hear Japanese on Windows, add a Japanese voice under Settings > Time & language > Speech, then restart YomiGo. Without one, classes skip listening. **Settings > Japanese voice** shows whether YomiGo found it. On Android, the same setting opens your phone's voice download page.

**Typing Japanese:** answer boxes convert romaji to kana for you. You can also use the Windows Japanese keyboard: Settings > Time & language > Language & region > Add a language > Japanese. Switch to it with Win+Space. On the TV, an on-screen keyboard opens when you choose an answer box; it types romaji. Learner names need English letters for now.

### Updates

YomiGo checks each time it starts. Updates come from this project's GitHub releases, and each file's signature is checked before it's used. The old program is kept as `.old`, and put back if the swap fails.


### TV and CRT

Turn on **Settings > CRT or RF screen** for a tube TV. It moves everything well inside the visible picture. The TV app sizes itself to the screen's height, so 640×480 works.

Software can't produce an RF signal. To reach an RF or composite TV you need hardware: an HDMI to composite converter, or a graphics card with analog output, followed by an RF modulator for RF-only sets. Set Windows to the lowest resolution your converter accepts.

### Controllers

The TV app reads any pad Windows sees. The d-pad or left stick moves, A chooses, B goes back, Start finishes typing, and L and R flip between places. The USB N64 pad `0E6D:111D` is set up for you, with C-Left and C-Right as L and R too.

For any other pad, choose **Settings > Set up controller** and press each button when asked. **Button labels** switches the prompts to ✕ and ○, and **Stick deadzone** helps a stick that drifts.

### Text Reader

On the PC, **Reader** in the sidebar reads Japanese and lets you click any word to look it up and add it to My words, with its sentence.

- **From the screen:** press Win+Shift+S, select some Japanese, then press **Screenshot**. Windows needs Japanese installed under Settings > Time & language > Language & region, including its optical character recognition. This uses Windows 10 or later, and `YomiGoOCR.exe` beside YomiGo.
- **From games:** run a text hooker such as Textractor, set it to copy text to the clipboard, and turn on **Watch clipboard**. New lines appear as the game shows them. **Keep on top** keeps YomiGo above the game.
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
