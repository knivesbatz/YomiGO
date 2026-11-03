# Decision log

**One Windows program, from Windows 95 to 11.** Written in C with no C runtime, using only functions Windows 95 has. Anything newer is looked up while running. Modern frameworks such as .NET, Electron, and Qt can't reach 95.

**Draws its own type.** DotGothic16 is baked into 1-bit 16px glyphs inside the program, so Japanese renders identically on an English Windows 95 with no Japanese fonts. It matches the look of 90s dot-matrix Japanese screen fonts, and its OFL licence allows bundling.

**Updates live in a separate program.** `YomiGoUpdate.exe` needs Windows 7 or later for modern TLS, and keeping it separate leaves the main programs loadable on 95. Older Windows updates by hand, instead of weakening security.

**Windows' own crypto for signatures.** Releases are signed with RSA-3072 and SHA-256, and checked with CryptoAPI. No hand-written cryptography. The private key is on the owner's PC only, so CI can't sign Windows builds.

**Desktop and HTPC share one source.** Same flow and same logic, built twice, with different chrome, layout, resolution, and input. The TV app is 320×240 with hard pixels and its own carousel, following the N64 launcher's approach: native low resolution, stretched to fill, title-safe margin, and a CRT mode that holds white below peak. It uses none of the launcher's code or any Nintendo art.

**Two small cores, not one.** C for Windows, JavaScript for Mobile and Reader. They share content and a save format, and both run `tests/vectors.json`. A single core would mean an embedded runtime that couldn't reach Windows 95.

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

**GitHub releases here.** An older rule against builds on GitHub applies to the owner's game project. YomiGo's updater needs them.
