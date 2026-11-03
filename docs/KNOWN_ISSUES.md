# Known issues

## Not tested on real hardware or software

- **Windows 95 and 98:** never run. See [compatibility](COMPATIBILITY.md).
- **Controllers:** the TV app detects a connected Xbox-style pad and shows its prompts, but no one has played with a pad yet, or tried DirectInput, N64 pads, rebinding, or reconnecting. The N64 layouts come from the launcher, and haven't been run with a real N64 pad in YomiGo.
- **CRT and RF:** not viewed on a real tube.
- **Windows Japanese keyboard:** the code is new. It's been tested with simulated keyboard messages only, because the build PC has no Japanese keyboard installed.
- **Windows speech:** this PC has no Japanese voice, so speech has never been heard.
- **Android:** speech input, text-to-speech, and self-update haven't run on a phone.
- **Reader:** not yet loaded as a real extension in Chrome or Firefox.

## Limits in this version

- The course ends at N4. Later levels are on the [roadmap](ROADMAP.md).
- The entrance exam tests reading only, not listening, typing, or speaking.
- Conversations are scripted and accept set replies, so a correct but unexpected reply can fail. After three tries, the app shows an accepted answer.
- Reader's dictionary has about 22,000 common words, and undoes common verb and adjective endings, but not every form. [Yomitan](https://yomitan.wiki/) goes much further.
- Web pages can detect that Reader is installed, because its dictionary file must be reachable from pages. No page can read your saved words.
- Words and main kanji readings have been [checked against JMdict](FACTCHECK.md). Notes, grammar explanations, example sentences, and listed on and kun readings haven't been checked against a reference. Kanji memory hooks are mnemonics, not etymology.
- Media difficulty is YomiGo's judgement, and availability isn't listed.
- Saves don't sync on their own. Copy them by hand.
- Firefox rejects Reader through Install Add-on From File, and removes a temporary install on restart, until Mozilla signs it. Someone has reported an error using it there. The exact message is still needed, as Firefox isn't on the build PC.
- Stroke order is shown, but there's no handwriting check. Youon cards such as きゃ don't show it.

## Security limits

- An updated release could replay an older signed build under a newer version tag, as signatures cover file contents rather than version numbers. It would still have to be a real signed YomiGo build. To fix when the release format next changes.
