# Known issues

## Not tested on real hardware or software

- **Windows 95 and 98:** never run. See [compatibility](COMPATIBILITY.md).
- **Controllers:** the TV app detects a connected Xbox-style pad and shows its prompts, but no one has played with a pad yet, or tried DirectInput, N64 pads, rebinding, or reconnecting. The N64 layouts come from the launcher, and haven't been run with a real N64 pad in YomiGo.
- **CRT and RF:** not viewed on a real tube.
- **Windows Japanese keyboard:** the code is new. It's been tested with simulated keyboard messages only, because the build PC has no Japanese keyboard installed.
- **Windows speech:** this PC has no Japanese voice, so speech has never been heard.
- **Chimes:** made by YomiGo itself and checked for level, but never heard through the program's own speakers on this PC, or on Windows 95 or 98 sound cards.
- **Automatic updates:** the swap and restart are the same as Check for updates, but the start-up path first runs for real when 0.14 comes out.
- **Android:** speech input, text-to-speech, and self-update haven't run on a phone.
- **Reader:** not yet loaded as a real extension in Chrome or Firefox.

- **Japanese OCR:** the build PC has only English OCR, so screenshot reading has been tested end to end in English. Japanese uses the same code with a different language.
- **Android photo reading:** built by CI, never run on a phone.
- **Transcription:** tested on Japanese lesson recordings and an English video. MKV files haven't been tried, and depend on which sound formats Windows has. It hasn't been tried on anime or films, where music and overlapping voices make mistakes more likely.

## Limits in this version

- Windows OCR is made for horizontal text, so vertical manga text often comes out poorly. A manga-specific reader is on the roadmap.
- Text appears from games only through a text hooker that copies to the clipboard. YomiGo doesn't hook games itself.
- Transcription runs on the processor only, at a little under twice the speed the file plays on the build PC. Over music or silence, Whisper sometimes writes lines no one said, such as a thank-you for watching. Check a line by ear before you learn from it.
- Transcription is on the PC app only. The TV app shows text you copy, but has no file picker, and the phone app doesn't transcribe.

- The course ends at N3. Later levels are on the [roadmap](ROADMAP.md). The entrance exam places nobody past N5 Street, so a learner who already knows N4 or N3 still starts there.
- The entrance exam tests reading only, not listening, typing, or speaking.
- Sakura, in Talk, is a small model running on your PC. Her Japanese is usually natural, but her checks miss some mistakes, sometimes flag a right sentence, and can explain a fix wrongly. That's why they say Maybe better. Trust the class notes over her when they disagree.
- Talk opens at the Grammar Gate, because the model writes poor beginner-only Japanese. It isn't on the phone, and the TV shows only her newest reply.
- The class conversations are scripted and accept set replies, so a correct but unexpected reply can fail. After three tries, the app shows an accepted answer.
- Reader's dictionary has about 22,000 common words, and undoes common verb and adjective endings, but not every form. [Yomitan](https://yomitan.wiki/) goes much further.
- Web pages can detect that Reader is installed, because its dictionary file must be reachable from pages. No page can read your saved words.
- Words and main kanji readings have been [checked against JMdict](FACTCHECK.md). Notes, grammar explanations, example sentences, and listed on and kun readings haven't been checked against a reference. Kanji memory hooks are mnemonics, not etymology.
- Media difficulty is YomiGo's judgement, and availability isn't listed.
- Saves don't sync on their own. Copy them by hand.
- Firefox rejects Reader through Install Add-on From File, and removes a temporary install on restart, until Mozilla signs it. Someone has reported an error using it there. The exact message is still needed, as Firefox isn't on the build PC.
- The example sentences on word cards are written for YomiGo and haven't had a native speaker's review yet.
- The pronoun notes and examples haven't had a native speaker's review yet, like the N4 and N3 grammar, readings, and conversations.
- Genki practice uses only YomiGo's own cards, and some Genki points have none yet, such as 〜てみる, 〜てある, 〜方, ほしい, 〜し, and 〜ばよかった. Those lessons practise the points YomiGo does teach.
- Stroke order is shown, but there's no handwriting check. Youon cards such as きゃ don't show it.

## Security limits

- An updated release could replay an older signed build under a newer version tag, as signatures cover file contents rather than version numbers. It would still have to be a real signed YomiGo build. To fix when the release format next changes.
