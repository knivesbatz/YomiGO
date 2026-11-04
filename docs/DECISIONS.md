# Decision log

**Windows is Rust around the shared web UI (since 0.21).** Until 0.20 the PC and TV apps were C that ran back to Windows 95, with their own pixel UI. Keeping two UIs meant every feature was built twice, and the pixel look couldn't be modern. Now Tauri hosts the same screens as the phone in Edge WebView2, and Rust does what a page can't: saves beside the program, the clipboard, the helper programs, the Material folder, and Wi-Fi sync. The cost is Windows 10 or later.

**The macOS Big Sur design language, on the PC and the phone.** Chosen by the owner as a precise target: unified toolbars, a Finder-style sidebar, System Preferences boxes, Big Sur controls, notification banners and alerts, short quiet motion. Inter stands in for SF Pro and YomiGo draws its own SF Symbols-style icons, because Apple's fonts and symbols may only be used on Apple systems. The TV keeps a game-console look instead, made for a pad and a sofa.

**Updates live in a separate program.** `YomiGoUpdate.exe` needs Windows 7 or later for modern TLS, and keeping it separate leaves the main programs loadable on 95. Older Windows updates by hand, instead of weakening security.

**Windows' own crypto for signatures.** Releases are signed with RSA-3072 and SHA-256, and checked with CryptoAPI. No hand-written cryptography. The private key is on the owner's PC only, so CI can't sign Windows builds.

**One program for the PC and the TV.** Named `YomiGoHTPC.exe`, or started with `--tv`, it opens full screen in TV mode: big tiles, a cursor that moves to whatever is most in line, then nearest, then top-left, a sound for each kind of action, and answers in about a fifth of a second, following Nintendo's published Switch UI principles without using any of its art or sounds. Sizes follow the screen height, so 480-line CRTs and 1080p TVs both work.

**One JavaScript core.** The C core went with the C apps, so `shared/core.js` is the only one. Sync's merge moved into it.

**Android is a thin Java shell around a web UI.** The web UI needs little native code and can be tested in any browser. Java keeps the build free of extra plugins.

**Integer SM-2 scheduling.** Established, simple, and easy to keep identical in two languages. FSRS is stronger and is on the roadmap.

**Course content is fact-checked by machine where possible.** The build fails if a word's romaji doesn't produce its kana, a card is taught twice or never, a media pick breaks the before-1999 or after-2020 rule, or a conversation reply can't be typed. Human-style checks against a dictionary arrive with JMdict.

**Romaji is spelled as typed.** `toukyou`, not `Tōkyō`. It maps 1:1 to kana and doubles as typing practice.

**N0 doesn't officially exist.** The JLPT stops at N1, so YomiGo plans its own Beyond N1 stage. YomiGo's exams never claim to be JLPT tests.

**"Identical to a native" became "advanced communicative competence".** No course can promise the former. Speech feedback only reports what the recogniser heard, and never claims native-like pronunciation.

**Conversations are scripted for now.** Real AI conversation needs a local model or a paid service, and the owner hasn't approved a cost. Scripted scenarios work offline, and still block progress until passed.

**Private material stays private.** The family's study files aren't in the repo, and the course doesn't copy them. The apps open them from a local folder, so nothing needs unlocking. No DRM bypass or unlock patch has been made.

**Genki practice uses YomiGo's cards.** It's grouped by Genki lesson (3rd edition order), with topics described in YomiGo's own words, so nothing from the book is copied into a public repo.

**Repo name.** `knivesbatz/YomiGO` already existed, and GitHub names ignore case, so `yomigo` resolves to it.

**Transcription with whisper.cpp, in its own 64-bit helper.** The main program stays Windows 95 safe. `YomiGoListen.exe` decodes files with Windows' own Media Foundation, so MP4 and M4A work without bundling FFmpeg, and shows the file picker itself, so YomiGo never handles a Unicode path. The model comes from Hugging Face at a fixed revision and is checked against its SHA-256, instead of being uploaded to every release. Large-v3-turbo was chosen over smaller models because they make far more mistakes in Japanese, and a learner can't tell.

**Talk runs a local model in its own 64-bit helper.** The owner chose free and private over a paid service. `YomiGoTalk.exe` keeps the model and the conversation in memory while YomiGo is open, so each reply only reads what's new, and it closes itself when YomiGo does. A 4B-class model can't keep to a reply format, so Sakura just talks, and the translation and the check are separate short questions asked afterwards, then dropped from the conversation. Gemma 4 E4B was picked for Google's good record in Japanese and its official build.

**Speaking aloud is optional on PC and TV.** The owner asked for it. Listening stays in the class, since it only needs a voice to play.

**Which words, and how they're taught.** New words come from the most frequent words in real learning material that the course doesn't teach yet, so the common ones come first. In English-language studies, the most frequent 3,000 words cover about 95% of spoken text, and spoken language leans on high-frequency words more than writing does. Each word comes with a short sentence people actually say, and the course teaches set phrases and reactions as wholes, because research links fixed phrases with more fluent speech. That evidence is still limited. Sources are in the bibliography.

**GitHub releases here.** An older rule against builds on GitHub applies to the owner's game project. YomiGo's updater needs them.
