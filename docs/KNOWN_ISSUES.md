# Known issues

## Not tested on real hardware or software

- **Windows 95 and 98:** never run. See [compatibility](COMPATIBILITY.md).
- **Controllers:** the XInput, DirectInput, rebinding, and disconnect code has only been compiled. No real pad has driven it.
- **CRT and RF:** not viewed on a real tube.
- **Windows speech:** this PC has no Japanese voice, so speech has never been heard.
- **Android:** speech input, text-to-speech, and self-update haven't run on a phone.
- **Reader:** not yet loaded as a real extension in Chrome or Firefox.

## Limits in this version

- The course ends at N5. Later levels are on the [roadmap](ROADMAP.md).
- The entrance exam tests reading only, not listening, typing, or speaking.
- Conversations are scripted and accept set replies, so a correct but unexpected reply can fail. After three tries, the app shows an accepted answer.
- Reader's dictionary only has YomiGo's course words. It can't look up general Japanese until JMdict arrives. Use [Yomitan](https://yomitan.wiki/) alongside it for now.
- Course text has been checked by machine for internal consistency, and by review, but not yet against a dictionary. Kanji memory hooks are mnemonics, not etymology.
- Media difficulty is YomiGo's judgement, and availability isn't listed.
- Saves don't sync on their own. Copy them by hand.
- Firefox removes Reader on restart until it's signed.
- Stroke order isn't taught in the app yet. The homework points to outside charts.

## Security limits

- An updated release could replay an older signed build under a newer version tag, as signatures cover file contents rather than version numbers. It would still have to be a real signed YomiGo build. To fix when the release format next changes.
